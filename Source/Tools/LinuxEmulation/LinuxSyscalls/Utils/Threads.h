// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/fextl/deque.h>
#include <FEXCore/fextl/memory.h>

#include <cstddef>

namespace FEX::LinuxEmulation::Threads {
/**
 * @brief Size of the stack that this interface creates.
 */
constexpr size_t STACK_SIZE = 8 * 1024 * 1024;
// Stack pool handling
struct StackPoolItem {
  void* Ptr;
  size_t Size;
};

struct DeadStackPoolItem {
  void* Ptr;
  size_t Size;
  bool ReadyToBeReaped;
};

class StackTracker final {
public:
  void* AllocateStackObject();
  bool* AddStackToDeadPool(void* Ptr);
  void AddStackToLivePool(void* Ptr);
  void RemoveStackFromLivePool(void* Ptr);

  [[noreturn]]
  void DeallocateStackObjectAndExit(void* Ptr, int Status);

  void CleanupAfterFork_PThread();

  void Shutdown();

private:
  std::mutex DeadStackPoolMutex {};
  std::mutex LiveStackPoolMutex {};

  fextl::deque<DeadStackPoolItem> DeadStackPool {};
  fextl::deque<StackPoolItem> LiveStackPool {};
};

/**
 * @brief Allocates a stack object from the internally managed stack pool.
 */
void* AllocateStackObject();

/**
 * @brief Deallocates a stack from the internally managed stack pool.
 *
 * Will not free the memory immediately, instead saving for reuse temporarily to solve race conditions on stack usage while stack tears down.
 *
 * @param Ptr The stack base from `AllocateStackObject`
 * @param Status The status to pass to the exit syscall.
 */
[[noreturn]]
void DeallocateStackObjectAndExit(void* Ptr, int Status);

/**
 * @brief Registers thread creation handlers with FEXCore.
 */
fextl::unique_ptr<StackTracker> SetupThreadHandlers();

/**
 * @brief Cleans up any remaining stack objects in the pools.
 */
void Shutdown(fextl::unique_ptr<StackTracker> STracker);
} // namespace FEX::LinuxEmulation::Threads
