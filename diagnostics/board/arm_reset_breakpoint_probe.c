#define _GNU_SOURCE
#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sys/prctl.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static volatile unsigned *value;
__attribute__((naked, noinline)) static void guarded(unsigned argument __attribute__((unused)),
                                                    volatile unsigned *target __attribute__((unused))) {
    __asm__ volatile("nop\nstr r0, [r1]\nbx lr");
}

static void check(int condition, const char *message) {
    if (!condition) { perror(message); exit(1); }
}

int main(int argc, char **argv) {
    int crash = argc == 2 && !strcmp(argv[1], "--crash");
    if (argc != 1 && !crash) return 2;
    value = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    check(value != MAP_FAILED, "mmap");
    if (crash) {
        check(!prctl(PR_SET_CHILD_SUBREAPER, 1, 0, 0, 0), "subreaper");
        pid_t tracer = fork();
        check(tracer >= 0, "tracer fork");
        if (tracer) {
            int status;
            check(waitpid(tracer, &status, 0) == tracer && WIFEXITED(status) && !WEXITSTATUS(status), "tracer exit");
            check(waitpid(-1, &status, 0) > 0 && WIFSIGNALED(status) && WTERMSIG(status) == SIGKILL && *value == 0, "EXITKILL crash containment");
            puts("PASS tracer death kills stopped tracee before side effect; no Main attach");
            munmap((void *)value, 4096);
            return 0;
        }
    }
    pid_t child = fork();
    check(child >= 0, "fork");
    if (!child) {
        check(!ptrace(PTRACE_TRACEME, 0, 0, 0), "TRACEME");
        raise(SIGSTOP);
        guarded(1, value);
        guarded(2, value);
        _exit(0);
    }
    int status;
    check(waitpid(child, &status, 0) == child && WIFSTOPPED(status), "initial stop");
    check(!ptrace(PTRACE_SETOPTIONS, child, 0, PTRACE_O_EXITKILL), "EXITKILL");
    unsigned resource = 0;
    check(!ptrace(29, child, 0, &resource), "GETHBPREGS");
    printf("resources=0x%x address=0x%lx\n", resource, (unsigned long)guarded);
    unsigned address = (uintptr_t)guarded & ~1u;
#ifdef __thumb__
    unsigned width = 2;
    unsigned control = (3 << 5) | (2 << 1) | 1;
#else
    unsigned width = 4;
    unsigned control = (15 << 5) | (2 << 1) | 1;
#endif
    check(!(address & (width - 1)), "instruction alignment");
    check(!ptrace(30, child, 1, &address), "breakpoint address");
    check(!ptrace(30, child, 2, &control), "breakpoint control");
    for (unsigned expected = 1; expected <= 2; ++expected) {
        check(!ptrace(PTRACE_CONT, child, 0, 0), "continue");
        check(waitpid(child, &status, 0) == child && WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP, "hardware stop");
        unsigned registers[18];
        check(!ptrace(PTRACE_GETREGS, child, 0, registers), "GETREGS");
        check(registers[15] == address && registers[0] == expected && *value == expected - 1, "pre-instruction state");
        printf("PASS trap%d before side effect pc=0x%x r0=%u previous=%u\n", expected, registers[15], registers[0], *value);
        if (crash) { fflush(stdout); _exit(0); }
        unsigned disabled = control & ~1u;
        check(!ptrace(30, child, 2, &disabled), "disable breakpoint");
        unsigned next = address + width;
        check(!ptrace(30, child, 1, &next), "step breakpoint address");
        check(!ptrace(30, child, 2, &control), "step breakpoint control");
        check(!ptrace(PTRACE_CONT, child, 0, 0), "hardware step");
        check(waitpid(child, &status, 0) == child && WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP, "step stop");
        check(!ptrace(PTRACE_GETREGS, child, 0, registers) && registers[15] == next, "step PC");
        check(!ptrace(30, child, 2, &disabled), "disable step breakpoint");
        check(!ptrace(30, child, 1, &address), "restore breakpoint address");
        check(!ptrace(30, child, 2, &control), "rearm breakpoint");
    }
    check(!ptrace(PTRACE_CONT, child, 0, 0), "final continue");
    check(waitpid(child, &status, 0) == child && WIFEXITED(status) && WEXITSTATUS(status) == 0 && *value == 2, "final state");
    puts("PASS repeated hardware interception/rearm; no Main attach or FPGA access");
    munmap((void *)value, 4096);
    return 0;
}
