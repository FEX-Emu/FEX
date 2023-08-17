#pragma once
#include <FEXCore/fextl/string.h>
#include <FEXCore/fextl/vector.h>

#include <cstdint>
#include <functional>

namespace FEXCore {
namespace IR {
class IREmitter;
}

/**
 * @brief Code loader class so the CPU backend can load code in a generic fashion
 *
 * This class is expected to have multiple different style of code loaders
*/
class CodeLoader {
public:
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

  virtual fextl::vector<fextl::string> const *GetApplicationArguments() { return nullptr; }
  virtual void GetExecveArguments(fextl::vector<char const*> *Args) {}

  virtual void GetAuxv(uint64_t& addr, uint64_t& size) {}

  using IRHandler = std::function<void(uint64_t Addr, FEXCore::IR::IREmitter *IR)>;
  virtual void AddIR(IRHandler Handler) {}

  virtual uint64_t GetBaseOffset() const { return 0; }
};


}
