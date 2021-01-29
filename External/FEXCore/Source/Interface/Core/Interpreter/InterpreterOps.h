namespace FEXCore::Core {
    struct InternalThreadState;
}

namespace FEXCore::IR {
    template<bool copy>
    class IRListView;
}

namespace FEXCore::Core{
    struct DebugData;
}

namespace FEXCore::CPU {
    class InterpreterOps {
        public:
        static void InterpretIR(FEXCore::Core::InternalThreadState *Thread, FEXCore::IR::IRListView<true> *CurrentIR, FEXCore::Core::DebugData *DebugData);
    };
};