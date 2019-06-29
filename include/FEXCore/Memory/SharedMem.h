#pragma once
#include <stddef.h>
#include <stdint.h>

namespace FEXCore::SHM {

  struct SHMObject {
    void *Ptr;
    uint8_t InternalState[0];
  };

  /**
   * @brief Allocate a shared memory region that will be the base of our VM's memory
   *
   * @param Size The size of the SHM region
   *
   * @return An object representing our internal SHM state
   */
  SHMObject *AllocateSHMRegion(size_t Size);

  /**
   * @brief Destroy the SHM region
   *
   * @param SHM The region previously created with AllocateSHMRegion
   */
  void DestroyRegion(SHMObject *SHM);
}
