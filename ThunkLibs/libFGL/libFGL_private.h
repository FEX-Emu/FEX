//FEX_TODO("Make these static")

extern "C" {
Display *IMPL(fgl_AddGuestX11)(Display *Guest, const char *DisplayName);
void IMPL(fgl_RemoveGuestX11)(Display *Guest);
Display *IMPL(fgl_GuestToHostX11)(Display *Guest);
Display *IMPL(fgl_HostToGuestX11)(Display *Host);

void IMPL(fgl_FlushFromGuestX11)(Display *Guest);
void IMPL(fgl_XFree)(void *p);
Display *IMPL(fgl_AddXGuestEGL)(Display *XGuest, const char *DisplayName);
void IMPL(fgl_RemoveXGuestEGL)(Display *XGuest);
Display *IMPL(fgl_XGuestToXHostEGL)(Display *XGuest);
Display *IMPL(fgl_HostToXGuestEGL)(EGLDisplay Host);
Display *IMPL(fgl_FlushFromHostEGL)(EGLDisplay Host);
}

//#define TRACE

#define CONCAT(a, b, c) a##_##b##_##c
#define EVAL(a, b) CONCAT(fexldr_ptr, a, b)
#define LDR_PTR(fn) EVAL(LIBLIB_NAME, fn)

//FEX_TODO("Not the best place for these?")
#if defined(DEBUG) || defined(TRACE)
#define dbgf printf
#else
static int nilprintf(...) { return 0; }
#define dbgf nilprintf
#endif

#define errf(...) fprintf(stderr, __VA_ARGS__)