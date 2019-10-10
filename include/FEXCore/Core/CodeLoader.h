#pragma once
#include <cstdint>
#include <functional>

namespace FEXCore {
/**
 * @brief Code loader class so the CPU backend can load code in a generic fashion
 *
 * This class is expected to have multiple different style of code loaders
*/
class CodeLoader {
public:

  /**
   * @brief CPU Core uses this to choose what the stack size should be for this code
   */
  virtual uint64_t StackSize() const = 0;
  /**
   * @brief Allows the code loader to set up the stack the way it wants
   *
   * @param HostPtr The host facing pointer to the base of the stack.
   * Size of memory will be at least the size that StackSize() returns
   *
   * @param GuestPtr The guest facing memory location where the base of the stack lives
   *
   * @return The location that the guest stack pointer register should be set to
   *
   * Probably will be GuestPtr + StackSize() - <Some amount>
   */
  virtual uint64_t SetupStack(void *HostPtr, uint64_t GuestPtr) const = 0;

  /**
   * @brief Function to return the guest RIP that the code should start out at
   */
  virtual uint64_t DefaultRIP() const = 0;

  virtual void GetInitLocations(std::vector<uint64_t> *Locations) {}
  virtual uint64_t InitializeThreadSlot(std::function<void(void const*, uint64_t)> Writer) const { return 0; };

  using MemoryLayout = std::tuple<uint64_t, uint64_t, uint64_t>;
  /**
   * @brief Gets the default memory layout of the memory object being loaded
   *
   * This will be mapped in to the guest memory space automatically
   *
   * @return A MemoryLayout object describing the layout of the region
   */
  virtual MemoryLayout GetLayout() const = 0;

  /**
   * @brief Allows the loader to map memory regions that it needs
   *
   * Code loader is expected to call the Mapper function with a memory offset and size for mapping
   *
   * @param Mapper Returns the host facing pointer for memory setup if the codfe loader needs to do things to it
   */
  virtual void MapMemoryRegion(std::function<void*(uint64_t, uint64_t)> Mapper) {}

  /**
   * @brief Memory writer function for loading code in to guest memory
   *
   * First argument = Data to write
   * Second argument = Guest memory data location
   * Third argument = Guest memory size
   */
  using MemoryWriter = std::function<void(void const*, uint64_t, uint64_t)>;
  virtual void LoadMemory(MemoryWriter Writer) = 0;

  /**
   * @brief Get the final RIP we are supposed to end up on in a debugger
   *
   * @return When the debugger reaches this RIP then we know that we have completed
   */
  virtual uint64_t GetFinalRIP() { return ~0ULL; }

  virtual char const *FindSymbolNameInRange(uint64_t Address) { return nullptr; }

};


}
