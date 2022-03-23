namespace FEXCore::Assert {
  // This function can not be inlined
  [[noreturn]]
  __attribute__((noinline, naked))
  void ForcedAssert() {
#ifdef _M_X86_64
    asm volatile("ud2");
#elif defined(_M_ARM_64)
    asm volatile("hlt #1");
#elif defined(_M_RISCV_64)
    // XXX: Is this correct?
    asm volatile("ebreak");
#else
#error Unknown architecture forced assert
#endif
  }
}


