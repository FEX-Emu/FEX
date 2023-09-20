// SPDX-License-Identifier: MIT
#pragma once

#include <cstddef>

namespace FEX::LinuxEmulation::Threads {
  /**
   * @brief Size of the stack that this interface creates.
   */
  constexpr size_t STACK_SIZE = 8 * 1024 * 1024;

  /**
   * @brief Allocates a stack object from the internally managed stack pool.
   */
  void *AllocateStackObject();

  /**
   * @brief Deallocates a stack from the internally managed stack pool.
   *
   * Will not free the memory immediately, instead saving for reuse temporarily to solve race conditions on stack usage while stack tears down.
   *
   * @param Ptr The stack base from `AllocateStackObject`
   */
  void DeallocateStackObject(void *Ptr);

  /**
   * @brief Registers thread creation handlers with FEXCore.
   */
  void SetupThreadHandlers();

  /**
   * @brief Cleans up any remaining stack objects in the pools.
   */
  void Shutdown();
}
