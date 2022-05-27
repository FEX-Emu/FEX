//FEX_TODO("Make these static")

Display *fgl_HostToGuestX11(Display *Host);
Display *fgl_GuestToHostX11(Display *Guest, const char *DisplayName);
void fgl_FlushFromGuestX11(Display *Guest, const char *DisplayName);
void fgl_XFree(void *p);

Display *fgl_HostToXGuestEGL(EGLDisplay Host);
Display *fgl_XGuestToXHostEGL(Display *XGuest, const char *DisplayName);
Display *fgl_FlushFromHostEGL(EGLDisplay Host);

#define CONCAT(a, b, c) a##_##b##_##c
#define EVAL(a, b) CONCAT(fexldr_ptr, a, b)
#define LDR_PTR(fn) EVAL(LIBLIB_NAME, fn)

//FEX_TODO("Not the best place for these?")
#if defined(DEBUG) || defined(TRACE)
#define dbgf printf
#define errf printf
#else
static int nilprintf(...) { return 0; }
#define dbgf nilprintf
#define errf printf
#endif
