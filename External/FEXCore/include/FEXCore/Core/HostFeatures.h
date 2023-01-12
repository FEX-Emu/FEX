#pragma once
#include <cstdint>

namespace FEXCore {
class HostFeatures final {
  public:
    HostFeatures();

    /**
     * @brief Backend features that change how codegen is generated from IR
     *
     * Specifically things that affect the IR->Codegen process
     * Not the x86->IR process
     */
    uint32_t DCacheLineSize{};
    uint32_t ICacheLineSize{};
    bool SupportsAES{};
    bool SupportsCRC{};
    bool SupportsCLZERO{};
    bool SupportsAtomics{};
    bool SupportsRCPC{};
    bool SupportsTSOImm9{};
    bool SupportsRAND{};
    bool Supports3DNow{};
    bool SupportsSSE4A{};
    bool SupportsAVX{};
    bool SupportsSHA{};
    bool SupportsBMI1{};
    bool SupportsBMI2{};
    bool SupportsCLWB{};
    bool SupportsPMULL_128Bit{};

    // Float exception behaviour
    bool SupportsFlushInputsToZero{};
    bool SupportsFloatExceptions{};
};
}
