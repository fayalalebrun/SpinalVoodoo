#include "dosbox_jitdump.h"

int main() {
    void *code = mmap(nullptr, 4096, PROT_READ | PROT_WRITE | PROT_EXEC,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (code == MAP_FAILED) return 1;
#if defined(__arm__)
    const uint32_t instructions[] = {0xe3a00001, 0xe12fff1e};
#elif defined(__x86_64__)
    const unsigned char instructions[] = {0xb8, 1, 0, 0, 0, 0xc3};
#else
#error Unsupported smoke-test architecture
#endif
    std::memcpy(code, instructions, sizeof(instructions));
    __builtin___clear_cache(static_cast<char *>(code), static_cast<char *>(code) + sizeof(instructions));
    mister_jit::load(code, code, sizeof(instructions), "dynrec-smoke-generation-1");
    auto function = reinterpret_cast<int (*)()>(code);
    const uint64_t start = mister_jit::timestamp();
    while (mister_jit::timestamp() - start < 1000000000) {
        for (unsigned i = 0; i < 10000; ++i) if (function() != 1) return 2;
    }
    mister_jit::load(code, code, sizeof(instructions), "dynrec-smoke-generation-2");
    const uint64_t second = mister_jit::timestamp();
    while (mister_jit::timestamp() - second < 1000000000) {
        for (unsigned i = 0; i < 10000; ++i) if (function() != 1) return 3;
    }
    return munmap(code, 4096) ? 4 : 0;
}
