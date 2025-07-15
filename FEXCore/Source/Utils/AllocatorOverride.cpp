// SPDX-License-Identifier: MIT
#include <FEXCore/Utils/Allocator.h>
#include <FEXCore/Utils/CompilerDefs.h>
#include <FEXCore/Utils/LogManager.h>

#include <fmt/format.h>

#include <cstddef>
#include <cstdint>
#include <sstream>
#include <unistd.h>

extern "C" {
// The majority of FEX internal code should avoid using the glibc allocator. To ensure glibc allocations don't accidentally slip
// in, FEX overrides these glibc functions with faulting variants.
//
// A notable exception is thunks, which should still use glibc allocations and avoid using `fextl::` namespace.
//
// Other minor exceptions throughout FEX use the `YesIKnowImNotSupposedToUseTheGlibcAllocator` helper to temporarily disable faulting.
#define GLIBC_ALIAS_FUNCTION(func) __attribute__((alias(#func), visibility("default")))
extern void* __libc_calloc(size_t, size_t);
void* calloc(size_t, size_t) GLIBC_ALIAS_FUNCTION(fault_calloc);

extern void __libc_free(void*);
void free(void*) GLIBC_ALIAS_FUNCTION(fault_free);

extern void* __libc_malloc(size_t);
void* malloc(size_t) GLIBC_ALIAS_FUNCTION(fault_malloc);

extern void* __libc_memalign(size_t, size_t);
void* memalign(size_t, size_t) GLIBC_ALIAS_FUNCTION(fault_memalign);

extern void* __libc_realloc(void*, size_t);
void* realloc(void*, size_t) GLIBC_ALIAS_FUNCTION(fault_realloc);

extern void* __libc_valloc(size_t);
void* valloc(size_t) GLIBC_ALIAS_FUNCTION(fault_valloc);

extern int __posix_memalign(void**, size_t, size_t);
int posix_memalign(void**, size_t, size_t) GLIBC_ALIAS_FUNCTION(fault_posix_memalign);

extern size_t __malloc_usable_size(void*);
size_t malloc_usable_size(void*) GLIBC_ALIAS_FUNCTION(fault_malloc_usable_size);

// Reuse __libc_memalign
void* aligned_alloc(size_t, size_t) GLIBC_ALIAS_FUNCTION(fault_aligned_alloc);
}

namespace FEXCore::Allocator {
// Enable or disable allocation faulting globally.
static bool GlobalEvaluate {};

// Enable or disable allocation faulting per-thread.
static thread_local uint64_t SkipEvalForThread {};

// Internal memory allocation hooks to allow non-faulting allocations through.
auto calloc_ptr = __libc_calloc;
auto free_ptr = __libc_free;
auto malloc_ptr = __libc_malloc;
auto memalign_ptr = __libc_memalign;
auto realloc_ptr = __libc_realloc;
auto valloc_ptr = __libc_valloc;
auto posix_memalign_ptr = ::posix_memalign;
auto malloc_usable_size_ptr = ::malloc_usable_size;
auto aligned_alloc_ptr = __libc_memalign;

// Constructor for per-thread allocation faulting check.
YesIKnowImNotSupposedToUseTheGlibcAllocator::YesIKnowImNotSupposedToUseTheGlibcAllocator() {
  ++SkipEvalForThread;
}

// Destructor for per-thread allocation faulting check.
YesIKnowImNotSupposedToUseTheGlibcAllocator::~YesIKnowImNotSupposedToUseTheGlibcAllocator() {
  --SkipEvalForThread;
}

// Hard disabling of per-thread allocation fault checking.
// No coming back from this, used on thread destruction.
FEX_DEFAULT_VISIBILITY void YesIKnowImNotSupposedToUseTheGlibcAllocator::HardDisable() {
  // Just set it to half of its maximum value so it never wraps back around.
  SkipEvalForThread = std::numeric_limits<decltype(SkipEvalForThread)>::max() / 2;
}

// Enable global fault checking.
void SetupFaultEvaluate() {
  GlobalEvaluate = true;
}

// Disable global fault checking.
void ClearFaultEvaluate() {
  GlobalEvaluate = false;
}

// Evaluate if a glibc hooked allocation should fault.
void EvaluateReturnAddress(void* Return) {
  if (!GlobalEvaluate) {
    // Fault evaluation disabled globally.
    return;
  }

  if (SkipEvalForThread) {
    // Fault evaluation currently disabled for this thread.
    return;
  }

  // We don't know where we are when allocating. Make sure to be safe and generate the string on the stack.
  // Print an error message to let a developer know that an allocation faulted.
  char Tmp[512];
  auto Res = fmt::format_to_n(Tmp, 512, "ERROR: Requested memory using non-FEX allocator at 0x{:x}\n", reinterpret_cast<uint64_t>(Return));
  Tmp[Res.size] = 0;
  write(STDERR_FILENO, Tmp, Res.size);

  // Trap the execution to stop FEX in its tracks.
  FEX_TRAP_EXECUTION;
}
} // namespace FEXCore::Allocator

extern "C" {
// These are the glibc allocator override symbols.
// These will override the glibc allocators and then check if the allocation should fault.
void* fault_calloc(size_t n, size_t size) {
  FEXCore::Allocator::EvaluateReturnAddress(__builtin_extract_return_addr(__builtin_return_address(0)));
  return FEXCore::Allocator::calloc_ptr(n, size);
}
void fault_free(void* ptr) {
  FEXCore::Allocator::EvaluateReturnAddress(__builtin_extract_return_addr(__builtin_return_address(0)));
  FEXCore::Allocator::free_ptr(ptr);
}
void* fault_malloc(size_t size) {
  FEXCore::Allocator::EvaluateReturnAddress(__builtin_extract_return_addr(__builtin_return_address(0)));
  return FEXCore::Allocator::malloc_ptr(size);
}
void* fault_memalign(size_t align, size_t s) {
  FEXCore::Allocator::EvaluateReturnAddress(__builtin_extract_return_addr(__builtin_return_address(0)));
  return FEXCore::Allocator::memalign_ptr(align, s);
}
void* fault_realloc(void* ptr, size_t size) {
  FEXCore::Allocator::EvaluateReturnAddress(__builtin_extract_return_addr(__builtin_return_address(0)));
  return FEXCore::Allocator::realloc_ptr(ptr, size);
}
void* fault_valloc(size_t size) {
  FEXCore::Allocator::EvaluateReturnAddress(__builtin_extract_return_addr(__builtin_return_address(0)));
  return FEXCore::Allocator::valloc_ptr(size);
}
int fault_posix_memalign(void** r, size_t a, size_t s) {
  FEXCore::Allocator::EvaluateReturnAddress(__builtin_extract_return_addr(__builtin_return_address(0)));
  return FEXCore::Allocator::posix_memalign_ptr(r, a, s);
}
size_t fault_malloc_usable_size(void* ptr) {
  FEXCore::Allocator::EvaluateReturnAddress(__builtin_extract_return_addr(__builtin_return_address(0)));
  return FEXCore::Allocator::malloc_usable_size_ptr(ptr);
}
void* fault_aligned_alloc(size_t a, size_t s) {
  FEXCore::Allocator::EvaluateReturnAddress(__builtin_extract_return_addr(__builtin_return_address(0)));
  return FEXCore::Allocator::aligned_alloc_ptr(a, s);
}
}
