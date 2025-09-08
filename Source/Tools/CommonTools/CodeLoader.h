// SPDX-License-Identifier: MIT
#pragma once

#include <FEXCore/fextl/string.h>
#include <FEXCore/fextl/vector.h>

#include <cstdint>

namespace FEX {

/**
 * @brief Code loader class so the CPU backend can load code in a generic fashion
 *
 * This class is expected to have multiple different style of code loaders
 */
class CodeLoader {
public:
  struct AuxvResult {
    uint64_t address;
    uint64_t size;
  };

  virtual ~CodeLoader() = default;

  /**
   * @brief CPU Core uses this to choose what the stack size should be for this code
   */
  virtual uint64_t StackSize() const = 0;

  /**
   * Returns the initial stack pointer
   */
  virtual uint64_t GetStackPointer() = 0;

  /**
   * @brief Function to return the guest RIP that the code should start out at
   */
  virtual uint64_t DefaultRIP() const = 0;

  virtual fextl::vector<const char*> GetExecveArguments() const {
    return {};
  }

  virtual AuxvResult GetAuxv() const {
    return {};
  }

  virtual uint64_t GetBaseOffset() const {
    return 0;
  }

  const fextl::vector<fextl::string>& GetApplicationArguments() const {
    return ApplicationArgs;
  }

protected:
  fextl::vector<fextl::string> ApplicationArgs;
};

} // namespace FEX
