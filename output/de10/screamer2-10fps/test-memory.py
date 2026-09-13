from pathlib import Path
import argparse
import os
import re
import subprocess

root = Path(__file__).resolve().parent
parser = argparse.ArgumentParser()
parser.add_argument('--thumb', action='store_true')
parser.add_argument('--compact', action='store_true')
parser.add_argument('--arm-header', type=Path)
parser.add_argument('--paging-header', type=Path)
parser.add_argument('--check-tlb-padding', action='store_true')
args = parser.parse_args()
source = root / 'emulator-o3/src/cpu/core_dynrec'
build = root / 'memory-test'
build.mkdir(exist_ok=True)
arm = (args.arm_header or source / 'risc_armv4le-o3.h').read_text()
common = (source / 'risc_armv4le-common.h').read_text()
basic = (source / 'decoder_basic.h').read_text()

def function(text, signature):
    start = text.index(signature)
    brace = text.index('{', start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end] + '\n'

paging = (args.paging_header or root / 'emulator-o3/include/paging.h').read_text()
layout = '\n'.join(re.findall(r'^#define\s+(?:TLB_SIZE|PAGING_LINKS|LINK_START)\s+.+$', paging, re.MULTILINE)) + '\n'
layout += function(paging, 'struct PagingBlock {') + ';\nPagingBlock paging = {};\n'
(build / 'paging-struct.h').write_text(layout)
code = '#define ARMV7LE 7\n#define C_TARGETCPU ARMV7LE\n#define C_DEBUG 0\n#define INLINE inline\n'
cache_source = (source / 'cache.h').read_text()
code += function(cache_source, 'static INLINE void cache_check_space(')
code += function(cache_source, 'static INLINE void cache_addd(uint32_t val,uint8_t *pos)')
code += function(cache_source, 'static INLINE void cache_addd(uint32_t val)')
code += common[:common.index('static void cache_block_closing')]
code += arm[:arm.index('// move a full register')]
for signature in ('static void gen_mov_regs(', 'static uint32_t gen_create_branch_on_zero(', 'static void INLINE gen_fill_branch(', 'static void INLINE gen_fill_branch_long(', 'static uint32_t gen_create_branch_long_nonzero(', 'template <typename T> static void INLINE gen_call_function_raw(', 'static void gen_mov_regval32_to_reg(', 'static void gen_mov_regword_to_reg('):
    code += function(arm, signature)
code += '#define DRC_SHARED_MEMORY_HELPERS\nstatic uint8_t* memory_helpers[2][3];\n'
code += function(arm, 'static void gen_call_memory_helper(')
code += function(arm, 'template <typename T> static void gen_memory_helper(')
code += '#define CACHE_MAXSIZE 8192\n'
code += function(arm, 'static void cache_block_before_close(')
for name, value_type in [('mem_writed_checked_drc', 'uint32_t'), ('mem_writew_checked_drc', 'uint16_t'), ('mem_writeb_checked_drc', 'uint8_t')]:
    signature = 'bool DRC_CALL_CONV ' + name + '(PhysPt address,' + value_type + ' val) {'
    code += function(basic, signature).replace(name, name + '_reference')
    code += 'bool ' + name + '(PhysPt address,' + value_type + ' val) { ++slow_calls; return ' + name + '_reference(address, val); }\n'
for name in ('mem_readd_checked_drc', 'mem_readw_checked_drc', 'mem_readb_checked_drc'):
    signature = 'bool DRC_CALL_CONV ' + name + '(PhysPt address) {'
    code += function(basic, signature).replace(name, name + '_reference')
    code += 'bool ' + name + '(PhysPt address) { ++slow_calls; return ' + name + '_reference(address); }\n'
code += function(basic, 'static void gen_memory_helpers(')
code += function(arm, 'static void gen_run_code(')
code += function(arm, 'static void gen_return_function(')
code += '''static uint32_t exception_branch;
static void dyn_check_exception(HostReg reg) {
    exception_branch = gen_create_branch_long_nonzero(reg, false);
}
static void gen_mov_word_to_reg(HostReg reg, void* data, bool dword) {
    if (data != &core_dynrec.readdata) std::abort();
    cache_addd(dword ? LDR_IMM(reg, readdata_addr, 0) : LDRH_IMM(reg, readdata_addr, 0));
}
'''
code += '''static void gen_mov_byte_to_reg_low(HostReg reg, void* data) {
    if (data != &core_dynrec.readdata) std::abort();
    cache_addd(LDRB_IMM(reg, readdata_addr, 0));
}
static void gen_mov_byte_to_reg_low_canuseword(HostReg reg, void* data) {
    gen_mov_byte_to_reg_low(reg, data);
}
'''
for name in ('dyn_read_byte', 'dyn_read_byte_canuseword', 'dyn_write_byte'):
    code += function(basic, 'static void ' + name + '(')
code += function(basic, 'static void dyn_read_word(')
code += function(basic, 'static void dyn_write_word(')
(build / 'generated.h').write_text(code)
sysroot = root.parent / 'sysroot'
compiler = '/nix/store/1qxqw8s8ccwxk2m47k9dvdbqqb0qlcpm-clang-21.1.7/bin/clang++'
flags = ['--target=armv7a-linux-gnueabihf', '--sysroot=' + str(sysroot), '--gcc-install-dir=' + str(sysroot / 'usr/lib/gcc/arm-linux-gnueabihf/12'), '-mcpu=cortex-a9', '-mfpu=neon-vfpv3', '-mfloat-abi=hard', '-O3', '-fno-pie', '-no-pie', '--ld-path=' + str(root.parent / 'toolchain-bin/ld.lld'), '-Wl,--dynamic-linker=/lib/ld-linux-armhf.so.3', '-Wl,-rpath-link,' + str(sysroot / 'usr/lib/arm-linux-gnueabihf'), '-std=c++14', '-Wall', '-Wextra', '-Werror']
env = {k: v for k, v in os.environ.items() if k not in ('CPATH', 'C_INCLUDE_PATH', 'CPLUS_INCLUDE_PATH', 'LIBRARY_PATH', 'COMPILER_PATH', 'NIX_CFLAGS_COMPILE', 'NIX_LDFLAGS')}
if args.thumb:
    flags.append('-mthumb')
if args.compact:
    flags.append('-DTEST_COMPACTION')
if args.check_tlb_padding:
    flags.append('-DTEST_TLB_PADDING')
name = ('memory-test-thumb' if args.thumb else 'memory-test-arm') + ('-compact' if args.compact else '') + ('-tlb-padding' if args.check_tlb_padding else '')
subprocess.run([compiler, *flags, str(root / 'memory-test.cpp'), '-o', str(build / name)], env=env, check=True)
