#include <FEXCore/Utils/Allocator.h>
#include <FEXCore/Utils/CompilerDefs.h>
#include <FEXCore/Utils/LogManager.h>

#include <fmt/format.h>

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <unistd.h>

extern "C" {
#ifdef GLIBC_ALLOCATOR_FAULT
  // glibc allocator faulting. If anything uses a glibc allocator symbol then cause a fault.
  // Ensures that FEX only ever allocates memory through its allocator routines.
  // If thunks are enabled then this should crash immediately. Thunks should still go through glibc allocator.
  // These need to be symbol definitions /only/, don't let their visibility to the rest of the source happen.
#define GLIBC_ALIAS_FUNCTION(func) __attribute__((alias(#func), visibility("default")))
  extern void *__libc_calloc(size_t, size_t);
  void *calloc(size_t, size_t) GLIBC_ALIAS_FUNCTION(fault_calloc);

  extern void __libc_free(void*);
  void free(void*) GLIBC_ALIAS_FUNCTION(fault_free);

  extern void *__libc_malloc(size_t);
  void *malloc(size_t) GLIBC_ALIAS_FUNCTION(fault_malloc);

  extern void *__libc_memalign(size_t, size_t);
  void *memalign(size_t, size_t) GLIBC_ALIAS_FUNCTION(fault_memalign);

  extern void *__libc_realloc(void*, size_t);
  void *realloc(void*, size_t) GLIBC_ALIAS_FUNCTION(fault_realloc);

  extern void *__libc_valloc(size_t);
  void *valloc(size_t) GLIBC_ALIAS_FUNCTION(fault_valloc);

  extern int __posix_memalign(void **, size_t, size_t);
  int posix_memalign(void **, size_t, size_t) GLIBC_ALIAS_FUNCTION(fault_posix_memalign);

  extern size_t __malloc_usable_size(void*);
  size_t malloc_usable_size(void*) GLIBC_ALIAS_FUNCTION(fault_malloc_usable_size);

  // Reuse __libc_memalign
  void *aligned_alloc(size_t, size_t) GLIBC_ALIAS_FUNCTION(fault_aligned_alloc);
}

namespace FEXCore::Allocator {
  static bool Evaluate{};
  thread_local uint64_t SkipEvalForThread{};

  CALLOC_Hook calloc_ptr = __libc_calloc;
  FREE_Hook free_ptr = __libc_free;
  MALLOC_Hook malloc_ptr = __libc_malloc;
  MEMALIGN_Hook memalign_ptr = __libc_memalign;
  REALLOC_Hook realloc_ptr = __libc_realloc;
  VALLOC_Hook valloc_ptr = __libc_valloc;
  POSIX_MEMALIGN_Hook posix_memalign_ptr = ::posix_memalign;
  MALLOC_USABLE_SIZE_Hook malloc_usable_size_ptr  = ::malloc_usable_size;
  ALIGNED_ALLOC_Hook aligned_alloc_ptr = __libc_memalign;

  YesIKnowImNotSupposedToUseTheGlibcAllocator::YesIKnowImNotSupposedToUseTheGlibcAllocator() {
    ++SkipEvalForThread;
  }

  YesIKnowImNotSupposedToUseTheGlibcAllocator::~YesIKnowImNotSupposedToUseTheGlibcAllocator() {
    --SkipEvalForThread;
  }

  FEX_DEFAULT_VISIBILITY void YesIKnowImNotSupposedToUseTheGlibcAllocator::HardDisable() {
    // Just set it to half of its maximum value so it never wraps back around.
    SkipEvalForThread = std::numeric_limits<decltype(SkipEvalForThread)>::max() / 2;
  }

  void SetupFaultEvaluate() {
    Evaluate = true;
  }

  void ClearFaultEvaluate() {
    Evaluate = false;
  }

  void EvaluateReturnAddress(void* Return) {
    if (!Evaluate) {
      return;
    }

    if (SkipEvalForThread) {
      return;
    }

    // We don't know where we are when allocating. Make sure to be safe and generate the string on the stack.
    char Tmp[512];
    auto Res = fmt::format_to_n(Tmp, 512, "Allocation from 0x{:x}\n", reinterpret_cast<uint64_t>(Return));
    Tmp[Res.size] = 0;
    write(STDERR_FILENO, Tmp, Res.size);
    LogMan::Msg::AFmt(Tmp);
    FEX_UNREACHABLE;
  }
}

extern "C" {
  void *fault_calloc(size_t n, size_t size) {
    FEXCore::Allocator::EvaluateReturnAddress(__builtin_extract_return_addr (__builtin_return_address (0)));
    return FEXCore::Allocator::calloc_ptr(n, size);
  }
  void fault_free(void* ptr) {
    FEXCore::Allocator::EvaluateReturnAddress(__builtin_extract_return_addr (__builtin_return_address (0)));
    FEXCore::Allocator::free_ptr(ptr);
  }
  void *fault_malloc(size_t size) {
    FEXCore::Allocator::EvaluateReturnAddress(__builtin_extract_return_addr (__builtin_return_address (0)));
    return FEXCore::Allocator::malloc_ptr(size);
  }
  void *fault_memalign(size_t align, size_t s) {
    FEXCore::Allocator::EvaluateReturnAddress(__builtin_extract_return_addr (__builtin_return_address (0)));
    return FEXCore::Allocator::memalign_ptr(align, s);
  }
  void *fault_realloc(void* ptr, size_t size) {
    FEXCore::Allocator::EvaluateReturnAddress(__builtin_extract_return_addr (__builtin_return_address (0)));
    return FEXCore::Allocator::realloc_ptr(ptr, size);
  }
  void *fault_valloc(size_t size) {
    FEXCore::Allocator::EvaluateReturnAddress(__builtin_extract_return_addr (__builtin_return_address (0)));
    return FEXCore::Allocator::valloc_ptr(size);
  }
  int fault_posix_memalign(void ** r, size_t a, size_t s) {
    FEXCore::Allocator::EvaluateReturnAddress(__builtin_extract_return_addr (__builtin_return_address (0)));
    return FEXCore::Allocator::posix_memalign_ptr(r, a, s);
  }
  size_t fault_malloc_usable_size(void *ptr) {
    FEXCore::Allocator::EvaluateReturnAddress(__builtin_extract_return_addr (__builtin_return_address (0)));
    return FEXCore::Allocator::malloc_usable_size_ptr(ptr);
  }
  void *fault_aligned_alloc(size_t a, size_t s) {
    FEXCore::Allocator::EvaluateReturnAddress(__builtin_extract_return_addr (__builtin_return_address (0)));
    return FEXCore::Allocator::aligned_alloc_ptr(a, s);
  }
#endif
}
