#include "mister_runtime.h"
#include <assert.h>
#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>

int main(void) {
    void *glide = dlopen("libglide2x.so", RTLD_NOW | RTLD_LOCAL);
    if (!glide) { fprintf(stderr, "%s\n", dlerror()); return 1; }
    const char *symbols[] = {"grGlideInit", "grSstWinOpen", "grBufferSwap", "grDrawTriangle",
                             "grTexDownloadMipMap", "grLfbReadRegion", "grGlideShutdown"};
    for (unsigned i = 0; i < sizeof(symbols) / sizeof(*symbols); ++i) assert(dlsym(glide, symbols[i]));
    assert(dlsym(glide, "mister_runtime_present") == dlsym(RTLD_DEFAULT, "mister_runtime_present"));
    assert(mister_runtime_acquire() == -1);
    printf("MISTER_ARM_ABI shared runtime, Glide exports, fail-closed reservation: PASS (errno %d)\n", errno);
    assert(!mister_runtime_release());
    assert(!dlclose(glide));
    return 0;
}
