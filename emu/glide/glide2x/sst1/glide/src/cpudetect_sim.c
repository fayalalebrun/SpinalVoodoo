/* CPU detection stub for non-x86 backends - no CPUID or x87 precision needed */

int _cpu_detect_asm(void) {
  return 6; /* report Pentium Pro / P6 class */
}

void single_precision_asm(void) {
  /* no-op in simulation */
}

void double_precision_asm(void) {
  /* no-op in simulation */
}
