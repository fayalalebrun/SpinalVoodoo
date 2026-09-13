#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <sys/mman.h>

using Bitu = uint32_t;
using Bits = int32_t;
using PhysPt = uint32_t;
using HostPt = uint8_t*;
using PageNum = uint32_t;
using tlbentry_t = uint32_t;
struct Handler;
using PageHandler = Handler;
#include "memory-test/paging-struct.h"
struct { uint32_t readdata; } core_dynrec = {};
static uint32_t Segs, cpu_regs;
struct { uint8_t *pos, *emit_start; Bitu emit_size; } cache = {};
static uint8_t backing[8192] __attribute__((aligned(4096)));
static uint32_t snapshot[16];
static unsigned slow_calls, handler_calls, cross_calls;
static bool fault;

[[noreturn]] static void E_Exit(const char*, ...) { throw 1; }
static uint8_t* cache_rwtox(uint8_t* pos) { return pos; }
static uint8_t host_readb(HostPt address) { return *address; }
static void host_writeb(HostPt address, uint8_t value) { *address = value; }
static HostPt get_tlb_read(PhysPt address) { return paging.tlb.read[address >> 12]; }
static HostPt get_tlb_write(PhysPt address) { return paging.tlb.write[address >> 12]; }
static void host_writed(HostPt address, uint32_t value) { std::memcpy(address, &value, 4); }
static void host_writew(HostPt address, uint16_t value) { std::memcpy(address, &value, 2); }
static bool checked_write(PhysPt address, uint32_t value, unsigned width) {
    if (fault) {
        if ((address & 4095) + width > 4096) backing[address & 8191] = uint8_t(value);
        return true;
    }
    for (unsigned i = 0; i < width; ++i) backing[(address + i) & 8191] = uint8_t(value >> (8 * i)) ^ 0x55;
    return false;
}
static uint32_t host_readd(HostPt address) {
    uint32_t value;
    std::memcpy(&value, address, 4);
    return value;
}
static uint16_t host_readw(HostPt address) {
    uint16_t value;
    std::memcpy(&value, address, 2);
    return value;
}
static bool checked(PhysPt address, void* out, unsigned width) {
    if (fault) return true;
    auto bytes = static_cast<uint8_t*>(out);
    for (unsigned i = 0; i < width; ++i) bytes[i] = backing[(address + i) & 8191] ^ 0x55;
    return false;
}
struct Handler {
    bool writeb_checked(PhysPt address, uint8_t value) {
        ++handler_calls;
        return checked_write(address, value, 1);
    }
    bool readb_checked(PhysPt address, uint8_t* out) {
        ++handler_calls;
        return checked(address, out, 1);
    }
    bool writed_checked(PhysPt address, uint32_t value) {
        ++handler_calls;
        return checked_write(address, value, 4);
    }
    bool writew_checked(PhysPt address, uint16_t value) {
        ++handler_calls;
        return checked_write(address, value, 2);
    }
    bool readd_checked(PhysPt address, uint32_t* out) {
        ++handler_calls;
        return checked(address, out, 4);
    }
    bool readw_checked(PhysPt address, uint16_t* out) {
        ++handler_calls;
        return checked(address, out, 2);
    }
} handler;
static Handler* get_tlb_readhandler(PhysPt) { return &handler; }
static Handler* get_tlb_writehandler(PhysPt) { return &handler; }
static bool mem_unalignedwrited_checked(PhysPt address, uint32_t value) {
    ++cross_calls;
    return checked_write(address, value, 4);
}
static bool mem_unalignedwritew_checked(PhysPt address, uint16_t value) {
    ++cross_calls;
    return checked_write(address, value, 2);
}
static bool mem_unalignedreadd_checked(PhysPt address, uint32_t* out) {
    ++cross_calls;
    return checked(address, out, 4);
}
static bool mem_unalignedreadw_checked(PhysPt address, uint16_t* out) {
    ++cross_calls;
    return checked(address, out, 2);
}

#include "memory-test/generated.h"

static void immediate(HostReg reg, uint32_t value) {
    cache_addd(MOVW(reg, value & 0xffff));
    cache_addd(MOVT(reg, value >> 16));
}
static void finish(unsigned status) {
    cache_addd(NOP);
    cache_addd(NOP);
    immediate(temp1, reinterpret_cast<uint32_t>(snapshot));
    for (HostReg reg : {HOST_r0, HOST_r1, HOST_r4, HOST_r5, HOST_r8, HOST_r9, HOST_r10, HOST_r11})
        cache_addd(STR_IMM(reg, temp1, 4 * reg));
    cache_addd(MOV_IMM(HOST_r0, status, 0));
    cache_addd(0xe8bd8ff8);
}

static void emit_helpers(uint8_t* code) {
    auto saved = cache;
    cache.pos = cache.emit_start = code + 2048;
    cache.emit_size = 2048;
    gen_memory_helpers();
#ifdef DRC_TLB_BASE_REG
    if (host_readd(memory_helpers[0][0]) != MOV_REG_LSR_IMM(HOST_r2, FC_OP1, 12)) std::abort();
    if (host_readd(memory_helpers[0][0] + 4) != LDR_REG_LSL_IMM(temp1, DRC_TLB_BASE_REG, HOST_r2, 2)) std::abort();
    const uint32_t delta = reinterpret_cast<uint32_t>(paging.tlb.write) - reinterpret_cast<uint32_t>(paging.tlb.read);
    const uint32_t write_base = delta == 0x400000 ? ADD_IMM(temp1, DRC_TLB_BASE_REG, 1, 10) : MOVW(temp1, reinterpret_cast<uint32_t>(paging.tlb.write) & 0xffff);
    if (host_readd(memory_helpers[1][0]) != write_base) std::abort();
#endif
    __builtin___clear_cache(reinterpret_cast<char*>(code + 2048), reinterpret_cast<char*>(cache.pos));
    cache = saved;
}

static void compact_block() {
#ifdef TEST_COMPACTION
    cache_block_before_close();
#else
    (void)cache_block_before_close;
#endif
}

static uint64_t test_writes() {
    uint64_t checks = 0;
    for (HostReg address_reg : {HOST_r4, HOST_r5})
        for (HostReg value_reg : {HOST_r0, HOST_r1, HOST_r4, HOST_r5})
            for (unsigned width : {1u, 2u, 4u}) {
                const bool dword = width == 4;
                auto code = static_cast<uint8_t*>(mmap(nullptr, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
                if (code == MAP_FAILED) std::abort();
                cache.pos = cache.emit_start = code;
                cache.emit_size = 2048;
                emit_helpers(code);
                cache_addd(0xe92d4ff8);
#ifdef DRC_TLB_BASE_REG
                immediate(DRC_TLB_BASE_REG, reinterpret_cast<uint32_t>(paging.tlb.read));
#endif
                immediate(HOST_r4, 0x44444444);
                immediate(HOST_r5, 0x55555555);
                immediate(readdata_addr, reinterpret_cast<uint32_t>(&core_dynrec.readdata));
                immediate(FC_REGS_ADDR, 0xaaaaaaaa);
                immediate(FC_SEGS_ADDR, 0xbbbbbbbb);
                gen_mov_regs(address_reg, HOST_r0);
                if (value_reg != address_reg) gen_mov_regs(value_reg, HOST_r1);
                cache_addd(NOP);
                if (width == 1) dyn_write_byte(address_reg, value_reg);
                else dyn_write_word(address_reg, value_reg, dword);
                finish(0);
                gen_fill_branch(exception_branch);
                finish(1);
                compact_block();
                __builtin___clear_cache(reinterpret_cast<char*>(code), reinterpret_cast<char*>(cache.pos));
                if (mprotect(code, 4096, PROT_READ | PROT_EXEC)) std::abort();
                auto execute = reinterpret_cast<unsigned (*)(PhysPt, uint32_t)>(code);
                for (uint32_t page : {0u, 0x12345u, 0x80000u, 0xfffffu})
                    for (unsigned mode = 0; mode < 4; ++mode) {
                        paging.tlb.write[page] = (mode == 0 || mode == 3) ? reinterpret_cast<HostPt>(reinterpret_cast<uint32_t>(backing) - (page << 12)) : nullptr;
                        fault = mode >= 2;
                        for (unsigned offset = 0; offset < 4096; ++offset) {
                            const uint32_t address = (page << 12) | offset;
                            const uint32_t input = (address * 0x91431u) ^ 0xb5f83a01u;
                            const uint32_t value = address_reg == value_reg ? address : input;
                            uint8_t before[16], expected[16];
                            for (unsigned i = 0; i < 16; ++i) before[i] = backing[(address - 4 + i) & 8191];
                            core_dynrec.readdata = 0xa5f0c3b6;
                            handler_calls = cross_calls = 0;
                            const bool expected_error = width == 1 ? mem_writeb_checked_drc_reference(address, uint8_t(value)) : dword ? mem_writed_checked_drc_reference(address, value) : mem_writew_checked_drc_reference(address, uint16_t(value));
                            const unsigned expected_handler = handler_calls, expected_cross = cross_calls;
                            for (unsigned i = 0; i < 16; ++i) {
                                expected[i] = backing[(address - 4 + i) & 8191];
                                backing[(address - 4 + i) & 8191] = before[i];
                            }
                            handler_calls = cross_calls = slow_calls = 0;
                            const unsigned error = execute(address, input);
                            const unsigned expected_slow = !(!(address & (width - 1)) && paging.tlb.write[page]);
                            bool ok = error == expected_error && handler_calls == expected_handler && cross_calls == expected_cross && slow_calls == expected_slow && core_dynrec.readdata == 0xa5f0c3b6;
                            for (unsigned i = 0; i < 16; ++i) ok = ok && backing[(address - 4 + i) & 8191] == expected[i];
                            for (HostReg reg : {HOST_r4, HOST_r5}) {
                                const uint32_t expected_reg = reg == address_reg ? address : reg == value_reg ? input : reg == HOST_r4 ? 0x44444444 : 0x55555555;
                                ok = ok && snapshot[reg] == expected_reg;
                            }
                            ok = ok && snapshot[readdata_addr] == reinterpret_cast<uint32_t>(&core_dynrec.readdata) && snapshot[FC_REGS_ADDR] == 0xaaaaaaaa && snapshot[FC_SEGS_ADDR] == 0xbbbbbbbb;
#ifdef DRC_TLB_BASE_REG
                            ok = ok && snapshot[DRC_TLB_BASE_REG] == reinterpret_cast<uint32_t>(paging.tlb.read);
#endif
                            if (!ok) {
                                std::fprintf(stderr, "FAIL write addr=%08x addrreg=%u value_reg=%u dword=%u mode=%u error=%u/%u slow=%u/%u handler=%u/%u cross=%u/%u\n", address, address_reg, value_reg, unsigned(dword), mode, error, unsigned(expected_error), slow_calls, expected_slow, handler_calls, expected_handler, cross_calls, expected_cross);
                                std::exit(1);
                            }
                            ++checks;
                        }
                        paging.tlb.write[page] = nullptr;
                    }
                if (munmap(code, 4096)) std::abort();
            }
    return checks;
}

#ifdef DRC_REGISTER_LOAD_FORWARDING
static void test_register_load_emission() {
    uint32_t code[8];
    unsigned checks = 0;
    for (unsigned src = 0; src < 16; ++src)
        for (unsigned dst = 0; dst < 16; ++dst)
            for (unsigned index : {0u, 4u, 4092u, 4096u})
                for (unsigned mode = 0; mode < 7; ++mode)
                    for (bool dword : {false, true}) {
                        std::memset(code, 0xa5, sizeof(code));
                        cache.emit_start = reinterpret_cast<uint8_t*>(code + 1);
                        cache.pos = cache.emit_start + 4;
                        cache.emit_size = 16;
                        uint32_t prior = STR_IMM(src, FC_REGS_ADDR, index);
                        if (mode == 2) prior |= 0x00400000;
                        if (mode == 3) prior ^= 4;
                        if (mode == 4) prior ^= 0x00010000;
                        if (mode == 5) prior ^= 0xf0000000;
                        if (mode == 6) prior = NOP;
                        code[1] = prior;
                        arm_last_branch_target = mode == 1 ? cache.pos : nullptr;
                        if (dword && (dst & 1)) gen_mov_regval32_to_reg(dst, index);
                        else gen_mov_regword_to_reg(dst, index, dword);
                        const bool forward = dword && !mode && index < 4096 && src != HOST_pc && dst != HOST_pc;
                        const uint32_t expected = forward ? src == dst ? 0xa5a5a5a5 : MOV_REG_LSL_IMM(dst, src, 0) : dword ? LDR_IMM(dst, FC_REGS_ADDR, index) : LDRH_IMM(dst, FC_REGS_ADDR, index);
                        if (code[1] != prior || code[2] != expected || cache.pos != cache.emit_start + 4 + (forward && src == dst ? 0 : 4) || code[0] != 0xa5a5a5a5 || code[7] != 0xa5a5a5a5) std::abort();
                        ++checks;
                    }
    for (unsigned pos : {0u, 6u}) {
        code[0] = code[5] = STR_IMM(HOST_r4, FC_REGS_ADDR, 0);
        uint32_t saved[8];
        std::memcpy(saved, code, sizeof(code));
        cache.pos = reinterpret_cast<uint8_t*>(code + pos);
        arm_last_branch_target = nullptr;
        bool rejected = false;
        try { gen_mov_regval32_to_reg(HOST_r4, 0); } catch (int) { rejected = true; }
        if (!rejected || std::memcmp(saved, code, sizeof(code)) || cache.pos != reinterpret_cast<uint8_t*>(code + pos)) std::abort();
        ++checks;
    }
    std::printf("PASS %u on-emission forwarding cases: aliases, widths, offsets, conditions, bases, joins, intervening instructions and bounds\n", checks);
}
#endif

static void test_forwarding() {
    uint64_t checks = 0;
    static volatile uint32_t cell;
    for (unsigned join : {0u, 1u, 2u})
        for (HostReg destination : {HOST_r4, HOST_r5}) {
            auto code = static_cast<uint8_t*>(mmap(nullptr, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
            if (code == MAP_FAILED) std::abort();
            cache.pos = cache.emit_start = code;
            cache.emit_size = 4096;
#ifdef DRC_REGISTER_LOAD_FORWARDING
            arm_last_branch_target = nullptr;
#endif
            cache_addd(0xe92d4430);
            immediate(FC_REGS_ADDR, reinterpret_cast<uint32_t>(&cell));
            gen_mov_regs(HOST_r4, HOST_r0);
            const uint32_t branch = join ? gen_create_branch_on_zero(HOST_r0, true) : 0;
            cache_addd(STR_IMM(HOST_r4, FC_REGS_ADDR, 0));
            if (join == 1) gen_fill_branch(branch);
            if (join == 2) gen_fill_branch_long(branch);
#ifndef DRC_REGISTER_LOAD_FORWARDING
            cache_addd(NOP);
#endif
            if (destination == HOST_r4) gen_mov_regval32_to_reg(destination, 0);
            else gen_mov_regword_to_reg(destination, 0, true);
            gen_mov_regs(HOST_r0, destination);
            cache_addd(ADD_IMM(HOST_r0, HOST_r0, 1, 0));
            cache_addd(0xe8bd8430);
            compact_block();
#if defined(TEST_COMPACTION) || defined(DRC_REGISTER_LOAD_FORWARDING)
            bool load = false;
            for (auto p = code; p < cache.pos; p += 4) load |= host_readd(p) == LDR_IMM(destination, FC_REGS_ADDR, 0);
            if (load != (join != 0)) std::abort();
#endif
            __builtin___clear_cache(reinterpret_cast<char*>(code), reinterpret_cast<char*>(cache.pos));
            if (mprotect(code, 4096, PROT_READ | PROT_EXEC)) std::abort();
            auto execute = reinterpret_cast<uint32_t (*)(uint32_t)>(code);
            const uint32_t edges[] = {0, 1, 0x7fffffff, 0x80000000, 0xffffffff};
            for (unsigned i = 0; i < 65536; ++i) {
                const uint32_t input = i < 5 ? edges[i] : i * 0x91431579u;
                cell = 0x12345678;
                const uint32_t expected = join && !input ? cell : input;
                if (execute(input) != expected + 1 || cell != expected) std::abort();
                ++checks;
            }
            if (munmap(code, 4096)) std::abort();
        }
    std::printf("PASS %llu executed ARM register-forwarding checks: aliases, branch joins, zero/high/overflow operands and memory effects\n", static_cast<unsigned long long>(checks));
}

static void test_runcode() {
    auto code = static_cast<uint8_t*>(mmap(nullptr, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
    if (code == MAP_FAILED) std::abort();
    cache.pos = cache.emit_start = code;
    cache.emit_size = 4096;
    gen_run_code();
    auto body = cache.pos;
    dyn_read_word(HOST_r1, HOST_r4, true);
    gen_mov_regs(FC_RETOP, HOST_r4);
    gen_return_function();
    gen_fill_branch(exception_branch);
    immediate(FC_RETOP, 0xfefefefe);
    gen_return_function();
    __builtin___clear_cache(reinterpret_cast<char*>(code), reinterpret_cast<char*>(cache.pos));
    if (mprotect(code, 4096, PROT_READ | PROT_EXEC)) std::abort();
    auto run = reinterpret_cast<uint32_t (*)(uint8_t*, PhysPt)>(code);
    unsigned checks = 0;
    for (unsigned mode = 0; mode < 3; ++mode) {
        paging.tlb.read[0] = mode == 0 ? backing : nullptr;
        fault = mode == 2;
        for (uint32_t address : {0u, 1u, 4092u, 4093u, 4095u}) {
            core_dynrec.readdata = 0xa5f0c3b6;
            const bool error = mem_readd_checked_drc_reference(address);
            const uint32_t expected = core_dynrec.readdata;
            core_dynrec.readdata = 0xa5f0c3b6;
            if (run(body, address) != (error ? 0xfefefefe : expected) || core_dynrec.readdata != expected) std::abort();
            ++checks;
        }
    }
    paging.tlb.read[0] = nullptr;
    if (munmap(code, 4096)) std::abort();
    std::printf("PASS %u emitted runcode/return interworking checks with TLB hits, misses, cross-page accesses and faults\n", checks);
}

static void test_call_bounds() {
    auto code = static_cast<uint8_t*>(mmap(nullptr, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
    if (code == MAP_FAILED) std::abort();
    unsigned checks = 0;
    for (int32_t delta : {-0x2000004, -0x2000000, -4, -1, 0, 1, 4, 0x1fffffc, 0x2000000})
        for (unsigned space : {0u, 3u, 4u}) {
            cache.pos = cache.emit_start = code;
            cache.emit_size = space;
            std::memset(code, 0xa5, 4);
            memory_helpers[0][0] = reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(code + 8) + delta);
            const bool reject = (delta & 3) || delta < -0x2000000 || delta >= 0x2000000 || space < 4;
            bool rejected = false;
            try { gen_call_memory_helper(false, 1); } catch (int) { rejected = true; }
            const uint32_t expected = reject ? 0xa5a5a5a5 : 0xeb000000 | ((delta >> 2) & 0xffffff);
            if (rejected != reject || host_readd(code) != expected || cache.pos != code + (reject ? 0 : 4)) std::abort();
            ++checks;
        }
    if (munmap(code, 4096)) std::abort();
    std::printf("PASS %u shared-helper branch range/alignment/emission-bound checks\n", checks);
}

int main() {
#ifdef TEST_TLB_PADDING
    const uintptr_t arrays[] = {uintptr_t(paging.tlb.read), uintptr_t(paging.tlb.write), uintptr_t(paging.tlb.readhandler), uintptr_t(paging.tlb.writehandler), uintptr_t(paging.tlb.phys_page)};
    const size_t sizes[] = {sizeof(paging.tlb.read), sizeof(paging.tlb.write), sizeof(paging.tlb.readhandler), sizeof(paging.tlb.writehandler), sizeof(paging.tlb.phys_page)};
    for (unsigned i = 0; i < 5; ++i) {
        if (sizes[i] != 4u * (1u << 20) || arrays[i] - arrays[0] != i * (4u * (1u << 20) + 32u)) {
            std::fprintf(stderr, "FAIL TLB layout array=%u offset=%u size=%u\n", i, unsigned(arrays[i] - arrays[0]), unsigned(sizes[i]));
            return 1;
        }
    }
    std::puts("PASS actual PagingBlock array sizes, four-byte strides and distinct 32-byte offsets modulo 4096");
#endif
    test_call_bounds();
#ifdef DRC_REGISTER_LOAD_FORWARDING
    test_register_load_emission();
#endif
    test_forwarding();
    uint64_t checks = 0;
    for (unsigned i = 0; i < sizeof(backing); ++i) backing[i] = uint8_t((i * 73) ^ (i >> 5));
    test_runcode();
    for (HostReg address_reg : {HOST_r0, HOST_r1, HOST_r4, HOST_r5})
        for (HostReg destination : {HOST_r0, HOST_r1, HOST_r4, HOST_r5})
            for (unsigned width : {1u, 2u, 4u}) {
                const bool dword = width == 4;
                auto code = static_cast<uint8_t*>(mmap(nullptr, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
                if (code == MAP_FAILED) { std::perror("mmap"); return 1; }
                cache.pos = cache.emit_start = code;
                cache.emit_size = 2048;
                emit_helpers(code);
                cache_addd(0xe92d4ff8);
#ifdef DRC_TLB_BASE_REG
                immediate(DRC_TLB_BASE_REG, reinterpret_cast<uint32_t>(paging.tlb.read));
#endif
                immediate(HOST_r1, 0x11111111);
                immediate(HOST_r4, 0x44444444);
                immediate(HOST_r5, 0x55555555);
                immediate(readdata_addr, reinterpret_cast<uint32_t>(&core_dynrec.readdata));
                immediate(FC_REGS_ADDR, 0xaaaaaaaa);
                immediate(FC_SEGS_ADDR, 0xbbbbbbbb);
                gen_mov_regs(address_reg, HOST_r0);
                cache_addd(NOP);
                if (width != 1) dyn_read_word(address_reg, destination, dword);
                else if (destination <= HOST_r1) dyn_read_byte_canuseword(address_reg, destination);
                else dyn_read_byte(address_reg, destination);
                finish(0);
                gen_fill_branch(exception_branch);
                finish(1);
                compact_block();
                if (cache.pos - code >= 4096) std::abort();
                __builtin___clear_cache(reinterpret_cast<char*>(code), reinterpret_cast<char*>(cache.pos));
                if (mprotect(code, 4096, PROT_READ | PROT_EXEC)) { std::perror("mprotect"); return 1; }
                auto execute = reinterpret_cast<unsigned (*)(PhysPt)>(code);
                for (uint32_t page : {0u, 0x12345u, 0x80000u, 0xfffffu})
                    for (unsigned mode = 0; mode < 4; ++mode) {
                        paging.tlb.read[page] = (mode == 0 || mode == 3) ? reinterpret_cast<HostPt>(reinterpret_cast<uint32_t>(backing) - (page << 12)) : nullptr;
                        fault = mode >= 2;
                        for (unsigned offset = 0; offset < 4096; ++offset) {
                            const uint32_t address = (page << 12) | offset;
                            core_dynrec.readdata = 0xa5f0c3b6;
                            handler_calls = cross_calls = 0;
                            const bool expected_error = width == 1 ? mem_readb_checked_drc_reference(address) : dword ? mem_readd_checked_drc_reference(address) : mem_readw_checked_drc_reference(address);
                            const uint32_t expected_data = core_dynrec.readdata;
                            const unsigned expected_handler = handler_calls, expected_cross = cross_calls;
                            const unsigned expected_slow = !(!(address & (width - 1)) && paging.tlb.read[page]);
                            core_dynrec.readdata = 0xa5f0c3b6;
                            handler_calls = cross_calls = slow_calls = 0;
                            const unsigned error = execute(address);
                            bool ok = error == expected_error && core_dynrec.readdata == expected_data &&
                                      handler_calls == expected_handler && cross_calls == expected_cross && slow_calls == expected_slow;
                            const uint32_t result = width == 1 ? uint8_t(expected_data) : dword ? expected_data : uint16_t(expected_data);
                            if (!error) ok = ok && snapshot[destination] == result;
                            for (HostReg reg : {HOST_r4, HOST_r5}) {
                                uint32_t expected = address_reg == reg ? address : reg == HOST_r4 ? 0x44444444 : 0x55555555;
                                if (!error && destination == reg) expected = result;
                                ok = ok && snapshot[reg] == expected;
                            }
                            ok = ok && snapshot[readdata_addr] == reinterpret_cast<uint32_t>(&core_dynrec.readdata) &&
                                 snapshot[FC_REGS_ADDR] == 0xaaaaaaaa && snapshot[FC_SEGS_ADDR] == 0xbbbbbbbb;
#ifdef DRC_TLB_BASE_REG
                            ok = ok && snapshot[DRC_TLB_BASE_REG] == reinterpret_cast<uint32_t>(paging.tlb.read);
#endif
                            if (!ok) {
                                std::fprintf(stderr, "FAIL addr=%08x addrreg=%u dst=%u dword=%u mode=%u error=%u/%u data=%08x/%08x slow=%u/%u handler=%u/%u cross=%u/%u\n", address, address_reg, destination, unsigned(dword), mode, error, unsigned(expected_error), core_dynrec.readdata, expected_data, slow_calls, expected_slow, handler_calls, expected_handler, cross_calls, expected_cross);
                                return 1;
                            }
                            ++checks;
                        }
                        paging.tlb.read[page] = nullptr;
                    }
                if (munmap(code, 4096)) { std::perror("munmap"); return 1; }
            }
    std::printf("PASS %llu emitted ARM write checks: byte/word/dword, all page offsets, TLB hits/misses, handler side effects, partial cross-page faults, register aliases and preservation\n", static_cast<unsigned long long>(test_writes()));
    std::printf("PASS %llu emitted ARM memory checks: byte/word/dword, all page offsets, low/high/wrapping addresses, TLB hits/misses, handlers/faults, cross-page fallback, register aliases and preservation\n", static_cast<unsigned long long>(checks));
}
