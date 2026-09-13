#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "flags-test/wrappers.h"

CPU_Regs cpu_regs = {};
unsigned log_count = 0;
static uint64_t checks = 0;
static uint32_t seed = 0x53637232;

static uint32_t random32() {
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    return seed;
}

static void check() {
    const LazyFlags state = lflags;
    const Bitu flags = reg_flags;
    for (unsigned branch = 0; branch != 16; ++branch) {
        log_count = 0;
        const bool expected = reference[branch]() != 0;
        const unsigned expected_logs = log_count;
        const LazyFlags expected_state = lflags;
        const Bitu expected_flags = reg_flags;
        lflags = state;
        reg_flags = flags;
        log_count = 0;
        const bool actual = candidate[branch]() != 0;
        if (actual != expected || log_count != expected_logs ||
            std::memcmp(&lflags, &expected_state, sizeof(lflags)) || reg_flags != expected_flags ||
            std::memcmp(&lflags, &state, sizeof(lflags)) || reg_flags != flags) {
            std::fprintf(stderr, "FAIL type=%u branch=%u a=%08x b=%08x r=%08x cf=%u flags=%08x expected=%u actual=%u\n",
                         unsigned(state.type), branch, lf_var1d, lf_var2d, lf_resd,
                         unsigned(lflags.oldcf), unsigned(flags), unsigned(expected), unsigned(actual));
            std::exit(1);
        }
        ++checks;
    }
}

static void state(unsigned type, uint32_t a, uint32_t b, uint32_t r, unsigned cf) {
    lflags = {};
    lflags.type = type;
    lflags.prev_type = random32() % t_LASTFLAG;
    lf_var1d = a;
    lf_var2d = b;
    lf_resd = r;
    lflags.oldcf = uint8_t(cf);
    reg_flags = random32();
    check();
}

static void arithmetic(unsigned width, uint32_t a, uint32_t b, unsigned cf) {
    const unsigned delta = width == 8 ? 0 : width == 16 ? 1 : 2;
    const uint32_t mask = width == 32 ? UINT32_MAX : (uint32_t(1) << width) - 1;
    const uint32_t poison = random32() & ~mask;
    const uint32_t results[] = {a + b, a | b, a + b + cf, a - b - cf,
                               a & b, a - b, a ^ b, a - b, a + 1, a - 1, a & b};
    for (unsigned op = 0; op != sizeof(results) / sizeof(*results); ++op)
        state(t_ADDb + op * 3 + delta, a | poison, b | poison, (results[op] & mask) | poison, cf);
    state(t_NEGb + delta, a | poison, b | poison, ((0u - a) & mask) | poison, cf);
}

static uint64_t unary_checks = 0;

static void check_unary(unsigned operation, uint32_t operand) {
    const LazyFlags before = lflags;
    const Bitu flags = reg_flags;
    log_count = 0;
    const uint32_t expected = reference_unary[operation](operand);
    const LazyFlags expected_state = lflags;
    const Bitu expected_flags = reg_flags;
    const unsigned expected_logs = log_count;
    lflags = before;
    reg_flags = flags;
    log_count = 0;
    const uint32_t actual = candidate_unary[operation](operand);
    if (actual != expected || std::memcmp(&lflags, &expected_state, sizeof(lflags)) ||
        reg_flags != expected_flags || log_count != expected_logs) {
        std::fprintf(stderr, "FAIL unary=%u type=%u operand=%08x flags=%08x expected=%08x actual=%08x\n",
                     operation, unsigned(before.type), operand, unsigned(flags), expected, actual);
        std::exit(1);
    }
    ++unary_checks;
    check();
}

int main() {
    static_assert(sizeof(Bitu) == 4, "ARM Bitu width");
    static_assert(BL_INDEX == 0 && W_INDEX == 0, "little endian target");
    for (unsigned a = 0; a != 256; ++a)
        for (unsigned b = 0; b != 256; ++b)
            for (unsigned cf = 0; cf != 2; ++cf)
                arithmetic(8, a, b, cf);
    const uint32_t edges[] = {0, 1, 2, 0x7e, 0x7f, 0x80, 0x81, 0xfe, 0xff, 0x100,
                             0x7ffe, 0x7fff, 0x8000, 0x8001, 0xfffe, 0xffff, 0x10000,
                             0x7ffffffe, 0x7fffffff, 0x80000000, 0x80000001, 0xfffffffe, 0xffffffff};
    for (unsigned width : {16u, 32u}) {
        const uint32_t mask = width == 32 ? UINT32_MAX : 0xffff;
        for (uint32_t a : edges)
            for (uint32_t b : edges)
                for (unsigned cf = 0; cf != 2; ++cf)
                    arithmetic(width, a & mask, b & mask, cf);
        for (unsigned i = 0; i != 10000; ++i)
            arithmetic(width, random32() & mask, random32() & mask, random32() & 1);
    }
    for (unsigned type = 0; type <= t_LASTFLAG + 1; ++type)
        for (unsigned i = 0; i != 1000; ++i) {
            uint32_t b = random32();
            if (type >= t_SHLb && type <= t_DSHRd)
                b = (b & ~0xffu) | (1 + b % 31);
            state(type, random32(), b, random32(), random32() & 1);
        }
    state(UINT32_MAX, UINT32_MAX, 1, 0, 1);
    for (unsigned flags = 0; flags != 0x1000; ++flags) {
        lflags = {};
        lflags.type = t_UNKNOWN;
        reg_flags = flags;
        check();
    }
    for (unsigned type = 0; type <= t_LASTFLAG + 1; ++type)
        for (unsigned i = 0; i != 512; ++i) {
            state(type, random32(), 1 + random32() % 31, random32(), random32() & 1);
            const LazyFlags initial = lflags;
            const Bitu flags = reg_flags;
            for (unsigned operation = 0; operation != 6; ++operation) {
                lflags = initial;
                reg_flags = flags;
                check_unary(operation, i < 256 ? i : random32());
                for (unsigned next = 0; next != 6; ++next)
                    check_unary(next, edges[(i + next) % (sizeof(edges) / sizeof(*edges))]);
            }
        }
    std::printf("PASS %llu differential INC/DEC transitions; all widths, lazy types, carry values, boundary operands and mixed-width chains\n",
                static_cast<unsigned long long>(unary_checks));
    std::printf("PASS %llu differential branch checks; exhaustive byte arithmetic, word/dword edges and random, all lazy types, fallback diagnostics and unchanged state\n",
                static_cast<unsigned long long>(checks));
}
