// SPDX-License-Identifier: MIT
/*
$info$
category: Thread shared code buffer management
tags: backend|shared
$end_info$
*/
#pragma once
#include <FEXCore/fextl/memory.h>
#include <FEXCore/Utils/SignalScopeGuards.h>

namespace FEXCore {
struct GuestToHostMap;
}

namespace FEXCore::CPU {
struct CodeBuffer {
  uint8_t* Ptr;
  size_t AllocatedSize; // including guard page; see UsableSize()

  fextl::unique_ptr<GuestToHostMap> LookupCache;

  CodeBuffer(size_t Size);
  CodeBuffer(const CodeBuffer&) = delete;
  CodeBuffer& operator=(const CodeBuffer&) = delete;
  CodeBuffer(CodeBuffer&& oth) = delete;
  CodeBuffer& operator=(CodeBuffer&&) = delete;

  ~CodeBuffer();

  /// Returns the number of bytes available for storing code
  size_t UsableSize() const {
    return AllocatedSize - FEXCore::Utils::FEX_PAGE_SIZE;
  }
};

/**
 * A manager that coordinates access to the CodeBuffer used for compiling new code across threads.
 *
 * The CodeBuffer is managed as a partially persistent data structure:
 * - Exactly one CodeBuffer is now designated as "active", which means data can be appended to it
 * - Lossy modifications to the active CodeBuffer will not invalidate any data in use by other threads (which is what enables save CodeBuffer sharing across threads)
 * - Instead, such lossy modifications trigger a new "version" of the data in the modifying thread. Old versions of the CodeBuffer persist as read-only data for use by the other threads.
 * - The other threads can update their version of the CodeBuffer. This will decrease the reference count and eventually trigger deallocation of the old version
 */
class SharedCodeBufferManager {
public:
  // Get the CodeBuffer that was most recently allocated.
  // This is the only CodeBuffer that data may be written to.
  fextl::shared_ptr<CodeBuffer> GetLatest();

  // Allocate a new CodeBuffer with geometric growth up to an internal maximum.
  // Subsequent calls to GetLatest will point to the returned buffer.
  fextl::shared_ptr<CodeBuffer> StartLargerCodeBuffer();

  // Allocate a new CodeBuffer with maximum internal size.
  // Subsequent calls to GetLatest will point to the returned buffer.
  fextl::shared_ptr<CodeBuffer> StartMaximalCodeBuffer();

  // Write offset into the latest CodeBuffer
  std::size_t LatestOffset {};

  // Protects writes to the latest CodeBuffer and changes to LatestOffset
  FEXCore::ForkableUniqueMutex CodeBufferWriteMutex;

  virtual void OnCodeBufferAllocated(const std::shared_ptr<CodeBuffer>&) {};

private:
  fextl::shared_ptr<CodeBuffer> Latest;

  fextl::shared_ptr<CodeBuffer> AllocateNew(size_t Size);
};
} // namespace FEXCore::CPU
