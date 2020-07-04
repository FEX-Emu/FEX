namespace FEXCore {
    typedef void ThunkedFunction(void* ArgsRv);

    class ThunkHandler {
    public:
        virtual ThunkedFunction* LookupThunk(const char *name) = 0;

        static ThunkHandler* Create();
    };
};