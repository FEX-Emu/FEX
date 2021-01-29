namespace FEXCore::Core {
  struct InternalThreadState;
}

namespace FEXCore::IR {
    class IRListView;
}

namespace FEXCore::Core{
  struct DebugData;
}

namespace FEXCore::CPU {
  enum FallbackABI {
    FABI_UNKNOWN,
    FABI_VOID_U16,
    FABI_F80_F32,
    FABI_F80_F64,
    FABI_F80_I16,
    FABI_F80_I32,
    FABI_F32_F80,
    FABI_F64_F80,
    FABI_I16_F80,
    FABI_I32_F80,
    FABI_I64_F80,
    FABI_I64_F80_F80,
    FABI_F80_F80,
    FABI_F80_F80_F80,
  };

  struct FallbackInfo {
    FallbackABI ABI;
    void *fn;
  };
  
  class InterpreterOps {

    public:
      static void InterpretIR(FEXCore::Core::InternalThreadState *Thread, FEXCore::IR::IRListView *CurrentIR, FEXCore::Core::DebugData *DebugData);
      static bool GetFallbackHandler(IR::IROp_Header *IROp, FallbackInfo *Info);
  };
};