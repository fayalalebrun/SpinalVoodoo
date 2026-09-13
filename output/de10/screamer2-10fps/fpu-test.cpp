#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include "fpu-test/fpu.h"

FPU_rec fpu;
int FPU_ArchitectureType = 1;
unsigned log_count = 0;
static std::array<uint8_t, 128> memory;
static std::vector<uint8_t> trace;
static unsigned accesses, fail_at;
struct Fault {};

static void append(const void* data, size_t size) {
    const auto bytes = static_cast<const uint8_t*>(data);
    trace.insert(trace.end(), bytes, bytes + size);
}
static void access(PhysPt address, uint32_t width, bool write, uint32_t value) {
    const uint32_t event[] = {address, width, uint32_t(write), value};
    append(event, sizeof(event));
    append(&fpu, sizeof(fpu));
    if (++accesses == fail_at || address > memory.size() - width) throw Fault{};
}
template <typename T> static T read_memory(PhysPt address) {
    access(address, sizeof(T), false, 0);
    T value;
    std::memcpy(&value, memory.data() + address, sizeof(value));
    return value;
}
template <typename T> static void write_memory(PhysPt address, T value) {
    access(address, sizeof(T), true, value);
    std::memcpy(memory.data() + address, &value, sizeof(value));
}
static uint8_t mem_readb(PhysPt address) { return read_memory<uint8_t>(address); }
static uint16_t mem_readw(PhysPt address) { return read_memory<uint16_t>(address); }
static uint32_t mem_readd(PhysPt address) { return read_memory<uint32_t>(address); }
static void mem_writeb(PhysPt address, uint8_t value) { write_memory(address, value); }
static void mem_writew(PhysPt address, uint16_t value) { write_memory(address, value); }
static void mem_writed(PhysPt address, uint32_t value) { write_memory(address, value); }

#include "fpu-test/helpers.h"

static void original_fld_sti(Bitu rm) {
    const Bitu from = ((FPUSW >> 11) + rm) & 7;
    FPU_PREP_PUSH();
    const Bitu top = (FPUSW >> 11) & 7;
    FPU_FST(from, top);
}
static void original_fxch_sti(Bitu rm) {
    const Bitu other = ((FPUSW >> 11) + rm) & 7;
    const Bitu top = (FPUSW >> 11) & 7;
    FPU_FXCH(top, other);
}
static void original_fstp_sti(Bitu rm) {
    const Bitu other = ((FPUSW >> 11) + rm) & 7;
    const Bitu top = (FPUSW >> 11) & 7;
    FPU_FST(top, other);
    FPU_FPOP();
}
template <void (*load)(PhysPt, Bitu)> static void original_load(PhysPt address) {
    FPU_PREP_PUSH();
    const Bitu top = (FPUSW >> 11) & 7;
    load(address, top);
}
template <void (*store)(PhysPt)> static void original_store_pop(PhysPt address) {
    store(address);
    FPU_FPOP();
}
using Operation = void (*)(Bitu);
static Operation original[] = {
    original_fld_sti, original_fxch_sti, original_fstp_sti,
    original_load<FPU_FLD_F32>, original_load<FPU_FLD_F64>, original_load<FPU_FLD_I16>,
    original_load<FPU_FLD_I32>, original_load<FPU_FLD_I64>, original_load<FPU_FBLD>,
    original_store_pop<FPU_FST_F32>, original_store_pop<FPU_FST_F64>, original_store_pop<FPU_FST_F80>,
    original_store_pop<FPU_FST_I16>, original_store_pop<FPU_FST_I32>, original_store_pop<FPU_FST_I64>, original_store_pop<FPU_FBST>
};
static Operation candidate[] = {
    dynrec_fpu_fld_sti, dynrec_fpu_fxch_sti, dynrec_fpu_fstp_sti,
    dynrec_fpu_load<FPU_FLD_F32>, dynrec_fpu_load<FPU_FLD_F64>, dynrec_fpu_load<FPU_FLD_I16>,
    dynrec_fpu_load<FPU_FLD_I32>, dynrec_fpu_load<FPU_FLD_I64>, dynrec_fpu_load<FPU_FBLD>,
    dynrec_fpu_store_pop<FPU_FST_F32>, dynrec_fpu_store_pop<FPU_FST_F64>, dynrec_fpu_store_pop<FPU_FST_F80>,
    dynrec_fpu_store_pop<FPU_FST_I16>, dynrec_fpu_store_pop<FPU_FST_I32>, dynrec_fpu_store_pop<FPU_FST_I64>, dynrec_fpu_store_pop<FPU_FBST>
};
using Image = std::array<uint8_t, sizeof(FPU_rec)>;
static uint64_t checks, failures_exercised;

static void check(unsigned operation, Bitu argument, unsigned failure) {
    Image expected;
    const FPU_rec before = fpu;
    const auto before_memory = memory;
    fail_at = failure;
    accesses = 0;
    trace.clear();
    bool expected_fault = false;
    try { original[operation](argument); } catch (const Fault&) { expected_fault = true; }
    std::memcpy(expected.data(), &fpu, sizeof(fpu));
    const auto expected_memory = memory;
    const auto expected_trace = trace;
    const unsigned expected_accesses = accesses;
    fpu = before;
    memory = before_memory;
    accesses = 0;
    trace.clear();
    bool actual_fault = false;
    try { candidate[operation](argument); } catch (const Fault&) { actual_fault = true; }
    if (actual_fault != expected_fault || std::memcmp(&fpu, expected.data(), sizeof(fpu)) ||
        memory != expected_memory || trace != expected_trace || accesses != expected_accesses) {
        std::fprintf(stderr, "FAIL operation=%u arg=%u fail_at=%u fault=%u/%u accesses=%u/%u\n",
                     operation, unsigned(argument), failure, unsigned(actual_fault), unsigned(expected_fault), accesses, expected_accesses);
        std::exit(1);
    }
    failures_exercised += expected_fault;
    ++checks;
    fpu = before;
    memory = before_memory;
}

static void initialize(unsigned seed, uint16_t status, bool raw) {
    static const uint64_t bits[] = {0, UINT64_C(0x8000000000000000), UINT64_C(0x7ff0000000000000),
        UINT64_C(0xfff0000000000000), UINT64_C(0x7ff8000000001234), UINT64_C(0x7ff0000000001234),
        UINT64_C(0x0000000000000001), UINT64_C(0x8000000000000001)};
    static const double values[] = {0.0, -0.0, 1.0, -1.0, 0.5, -0.5, 1.5, -1.5, 32767.0, -32768.0,
        2.25, -2.25, 1e-100, -1e-100, 12345.678, -12345.678};
    std::memset(static_cast<void*>(&fpu), 0, sizeof(fpu));
    for (unsigned i = 0; i < 9; ++i) {
        if (raw) std::memcpy(&fpu.regs[i], &bits[(seed + i) % 8], 8);
        else fpu.regs[i].d = values[(seed + i) % 16];
        fpu.regs_80[i].raw.l = UINT64_C(0xa5a5123456789abc) ^ (uint64_t(seed) << 32) ^ i;
        fpu.regs_80[i].raw.h = uint16_t(FPU_Reg_80_exponent_bias + 63 + ((seed & 1) ? 0x8000 : 0));
        fpu.tags[i] = static_cast<FPU_Tag>((seed + i) & 3);
        fpu.use80[i] = ((seed >> (i & 7)) & 1) != 0;
    }
    fpu.cw.reg = uint16_t(0x037f | ((seed & 3) << 10));
    fpu.sw.reg = status;
    for (unsigned i = 0; i < memory.size(); ++i) memory[i] = uint8_t(seed * 17 + i * 73);
}

int main() {
    static_assert(sizeof(Bitu) == 4, "ARM Bitu width");
    static_assert(sizeof(FPU_Reg_80) == 10, "raw 80-bit precision");
    static_assert(sizeof(original) == sizeof(candidate), "operation coverage");
    for (unsigned status = 0; status < 65536; ++status) {
        initialize(status, uint16_t(status), true);
        for (unsigned operation = 0; operation < 3; ++operation) check(operation, status & 7, 0);
    }
    for (unsigned seed = 0; seed < 128; ++seed) {
        initialize(seed, uint16_t(seed * 509 ^ 0x5a5a), false);
        for (unsigned operation = 3; operation < sizeof(candidate) / sizeof(*candidate); ++operation)
            for (Bitu address : {0u, 1u, 120u, 127u})
                for (unsigned failure = 0; failure < 12; ++failure) check(operation, address, failure);
    }
    if (!failures_exercised) return 1;
    std::printf("PASS %llu FPU fusion checks, %llu fault cases: all status words/TOPs, register aliases, tags/80-bit shadows, signed zero/NaNs, rounding, load/store order and partial memory faults\n",
                static_cast<unsigned long long>(checks), static_cast<unsigned long long>(failures_exercised));
}
