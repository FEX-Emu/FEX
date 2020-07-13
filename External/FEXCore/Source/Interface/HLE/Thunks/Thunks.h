namespace FEXCore::Context {
  struct Context;
}

namespace FEXCore {
    typedef void ThunkedFunction(FEXCore::Context::Context *CTX, void* ArgsRv);

    class ThunkHandler {
    public:
        virtual ThunkedFunction* LookupThunk(const char *name) = 0;

        static ThunkHandler* Create();
    };
};