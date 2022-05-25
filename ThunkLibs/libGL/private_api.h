//FEX_TODO("Make these static")
Display *fgl_HostToGuest(Display *Host);
Display *fgl_GuestToHost(Display *Guest, const char *DisplayName);
void fgl_XFree(void *p);

GLXContextID fgl_GetContextID(GLXContext context);

void fgl_FlushFromGuest(Display *Guest, const char *DisplayName);

//FEX_TODO("Not the best place for these?")
#if defined(DEBUG_GLX) || defined(TRACE_GLX)
#define dbgf printf
#define errf printf
#else
static int nilprintf(...) { return 0; }
#define dbgf nilprintf
#define errf printf
#endif
