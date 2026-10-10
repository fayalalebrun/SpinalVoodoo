/*
 * FPU handling code
 */

/*
 * This routine sets the precision to single which effects all
 * adds, mults, and divs.
 */
#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86)
void single_precision_asm()
{
#if defined(__MSC__)
  __asm {
    push  eax       ; make room
    fnclex          ; clear pending exceptions
    fstcw WORD PTR [esp]
    mov   eax, DWORD PTR [esp]
    and   eax, 0000fcffh  ; clear bits 9:8
    mov   DWORD PTR [esp], eax
    fldcw WORD PTR [esp]
    pop   eax
    }
#elif defined(__GNUC__)
  asm("push %eax\n"
      "fnclex\n"
      "fstcw (%esp)\n"
      "movl (%esp), %eax\n"
      "and $0x0000fcff, %eax\n"
      "movl %eax, (%esp)\n"
      "fldcw (%esp)\n"
      "pop %eax");
#else
#error "Need to implement single_precision_asm() for this compiler"
#endif
}
#else
#warning "Using a stub for single_precision_asm() for this architecture"
void single_precision_asm()
{
}
#endif

/*
 * This routine sets the precision to double which effects all
 * adds, mults, and divs.
 */
#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86)
void double_precision_asm()
{
#if defined(__MSC__)
  __asm {
    push  eax       ; make room
    fnclex          ; clear pending exceptions
    fstcw WORD PTR [esp]
    mov   eax, DWORD PTR [esp]
    and   eax, 0000fcffh  ; clear bits 9:8
    or    eax, 000002ffh  ; set 9:8 to 10
    mov   DWORD PTR [esp], eax
    fldcw WORD PTR [esp]
    pop   eax
    }
#elif defined(__GNUC__)
  asm("push %eax\n"
      "fnclex\n"
      "fstcw (%esp)\n"
      "movl (%esp), %eax\n"
      "and $0x0000fcff, %eax\n"
      "or $0x000002ff, %eax\n"
      "movl %eax, (%esp)\n"
      "fldcw (%esp)\n"
      "pop %eax");
#else
#error "Need to implement double_precision_asm() for this compiler"
#endif
}
#else
#warning "Using a stub for double_precision_asm() for this architecture"
void double_precision_asm()
{
}
#endif
