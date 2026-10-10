import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]


class GlideIdleTest(unittest.TestCase):
    def test_idle_polling(self):
        source = (ROOT / 'vendor/glide/glide2x/sst1/init/initvg/util.c').read_text()
        function = 'void sst1InitIdleLoop' + source.split('void sst1InitIdleLoop', 1)[1].split('\n}\n', 1)[0] + '\n}\n'
        harness = r'''
#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
typedef uint32_t FxU32;
typedef uint8_t FxU8;
typedef int FxBool;
typedef struct { FxU32 status, nopCMD; } Sstregs;
#define SST_BUSY 1
static FxU32 memory[256];
static unsigned status_reads, debug_reads, nops, timeouts;
static int verbose, stuck;
static FxBool de10IdleVerbose(void) { return verbose; }
static FxU32 sst1InitReturnStatus(FxU32 *base) {
    static const FxU32 sequence[] = {1, 1, 0, 0, 1, 0, 0, 0};
    assert(base == memory);
    unsigned index = status_reads++;
    if (index < 2 || stuck) return SST_BUSY;
    assert(index - 2 < sizeof(sequence) / sizeof(sequence[0]));
    return sequence[index - 2];
}
static FxU32 read_debug(volatile FxU32 *address) {
    assert(address == memory + 0x240 / 4 || address == memory + 0x14c / 4 ||
           address == memory + 0x15c / 4);
    ++debug_reads;
    return 0;
}
static int quiet_print(FILE *file, const char *format, ...) {
    (void)file;
    (void)format;
    return 0;
}
#define IGET(value) read_debug(&(value))
#define ISET(target, value) ((target) = (value), ++nops)
#define INIT_PRINTF(args) (++timeouts)
#define fprintf quiet_print
'''
        harness += function + r'''
int main(int argc, char **argv) {
    assert(argc == 3);
    verbose = atoi(argv[1]);
    stuck = atoi(argv[2]);
    sst1InitIdleLoop(memory);
    assert(nops == 1);
    assert(status_reads == (stuck ? 1000002u : 10u));
    assert(timeouts == (unsigned)stuck);
    assert(verbose ? debug_reads > 0 : debug_reads == 0);
    if (verbose && !stuck) assert(debug_reads == 30);
    return 0;
}
'''
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)
            (path / 'test.c').write_text(harness)
            for backend in ('SIM_BACKEND', 'DE10_BACKEND'):
                binary = path / backend
                subprocess.run(shlex.split(os.environ.get('CC', 'cc')) +
                               ['-std=c11', '-O2', '-Wall', '-Wextra', '-Werror',
                                '-D' + backend, str(path / 'test.c'), '-o', str(binary)], check=True)
                for verbose in (0, 1):
                    for stuck in (0, 1):
                        with self.subTest(backend=backend, verbose=verbose, stuck=stuck):
                            result = subprocess.run([str(binary), str(verbose), str(stuck)], capture_output=True)
                            self.assertEqual(result.returncode, 0, result.stderr.decode())


if __name__ == '__main__':
    unittest.main()
