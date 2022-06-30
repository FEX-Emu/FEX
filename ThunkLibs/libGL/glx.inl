
XVisualInfo *IMPL(glXChooseVisual)(Display *dpy, int screen, int *attribList) {
    OPTIONAL_HOSTCALL_ABI
	SYNC_GUEST_HOST();
	auto rv = MapVisualInfoHostToGuest(dpy, PACKER_OPTIONAL_HOSTCALL(glXChooseVisual)(GuestToHostX11(dpy), screen, attribList OPTIONAL_HOSTCALL_LASTARG));
	SYNC_HOST_GUEST();
	return rv;
}

GLXContext IMPL(glXCreateContext)(Display *dpy, XVisualInfo *vis, GLXContext shareList, Bool direct) {
    OPTIONAL_HOSTCALL_ABI
	SYNC_GUEST_HOST();
	auto rv = PACKER_OPTIONAL_HOSTCALL(glXCreateContext)(GuestToHostX11(dpy), MapVisualInfoGuestToHost(dpy, vis), shareList, direct OPTIONAL_HOSTCALL_LASTARG);
	SYNC_HOST_GUEST();
	return rv;
}

void IMPL(glXDestroyContext)(Display *dpy, GLXContext ctx) {
    OPTIONAL_HOSTCALL_ABI
	SYNC_GUEST_HOST();
	PACKER_OPTIONAL_HOSTCALL(glXDestroyContext)(GuestToHostX11(dpy), ctx OPTIONAL_HOSTCALL_LASTARG);
	SYNC_GUEST_HOST();
}

Bool IMPL(glXMakeCurrent)(Display *dpy, GLXDrawable drawable, GLXContext ctx) {
    OPTIONAL_HOSTCALL_ABI
	SYNC_GUEST_HOST();
	auto rv = PACKER_OPTIONAL_HOSTCALL(glXMakeCurrent)(GuestToHostX11(dpy), drawable, ctx OPTIONAL_HOSTCALL_LASTARG);
	SYNC_HOST_GUEST();
	return rv;
}

void IMPL(glXCopyContext)(Display *dpy, GLXContext src, GLXContext dst, unsigned long mask) {
    OPTIONAL_HOSTCALL_ABI
	SYNC_GUEST_HOST();
	PACKER_OPTIONAL_HOSTCALL(glXCopyContext)(GuestToHostX11(dpy), src, dst, mask OPTIONAL_HOSTCALL_LASTARG);
	SYNC_HOST_GUEST();
}

void IMPL(glXSwapBuffers)(Display *dpy, GLXDrawable drawable) {
    OPTIONAL_HOSTCALL_ABI
	SYNC_GUEST_HOST();
	PACKER_OPTIONAL_HOSTCALL(glXSwapBuffers)(GuestToHostX11(dpy), drawable OPTIONAL_HOSTCALL_LASTARG);
	SYNC_HOST_GUEST();
}

GLXPixmap IMPL(glXCreateGLXPixmap)(Display *dpy, XVisualInfo *visual, Pixmap pixmap) {
    OPTIONAL_HOSTCALL_ABI
	SYNC_GUEST_HOST();
	auto rv = PACKER_OPTIONAL_HOSTCALL(glXCreateGLXPixmap)(GuestToHostX11(dpy), MapVisualInfoGuestToHost(dpy, visual), pixmap OPTIONAL_HOSTCALL_LASTARG);
	SYNC_HOST_GUEST();
	return rv;
}

void IMPL(glXDestroyGLXPixmap)(Display *dpy, GLXPixmap pixmap) {
    OPTIONAL_HOSTCALL_ABI
	SYNC_GUEST_HOST();
	PACKER_OPTIONAL_HOSTCALL(glXDestroyGLXPixmap)(GuestToHostX11(dpy), pixmap OPTIONAL_HOSTCALL_LASTARG);
	SYNC_HOST_GUEST();
}

Bool IMPL(glXQueryExtension)(Display *dpy, int *errorb, int *event) {
    OPTIONAL_HOSTCALL_ABI
	SYNC_GUEST_HOST();
	auto rv = PACKER_OPTIONAL_HOSTCALL(glXQueryExtension)(GuestToHostX11(dpy), errorb, event OPTIONAL_HOSTCALL_LASTARG);
	SYNC_HOST_GUEST();
	return rv;
}

Bool IMPL(glXQueryVersion)(Display *dpy, int *maj, int *min) {
    OPTIONAL_HOSTCALL_ABI
	SYNC_GUEST_HOST();
	auto rv = PACKER_OPTIONAL_HOSTCALL(glXQueryVersion)(GuestToHostX11(dpy), maj, min OPTIONAL_HOSTCALL_LASTARG);
	SYNC_HOST_GUEST();
	return rv;
}

Bool IMPL(glXIsDirect)(Display *dpy, GLXContext ctx) {
    OPTIONAL_HOSTCALL_ABI
	SYNC_GUEST_HOST();
	auto rv = PACKER_OPTIONAL_HOSTCALL(glXIsDirect)(GuestToHostX11(dpy), ctx OPTIONAL_HOSTCALL_LASTARG);
	SYNC_HOST_GUEST();
	return rv;
}

int IMPL(glXGetConfig)(Display *dpy, XVisualInfo *visual, int attrib, int *value) {
    OPTIONAL_HOSTCALL_ABI
	SYNC_GUEST_HOST();
	auto rv = PACKER_OPTIONAL_HOSTCALL(glXGetConfig)(GuestToHostX11(dpy), MapVisualInfoGuestToHost(dpy, visual), attrib, value OPTIONAL_HOSTCALL_LASTARG);
	SYNC_HOST_GUEST();
	return rv;
}

/*GLX 1.1 and later */
const char *IMPL(glXQueryExtensionsString)(Display *dpy, int screen) {
    OPTIONAL_HOSTCALL_ABI
	SYNC_GUEST_HOST();
	auto rv = PACKER_OPTIONAL_HOSTCALL(glXQueryExtensionsString)(GuestToHostX11(dpy), screen OPTIONAL_HOSTCALL_LASTARG);
	SYNC_HOST_GUEST();
	return rv;
}

const char *IMPL(glXQueryServerString)(Display *dpy, int screen, int name) {
    OPTIONAL_HOSTCALL_ABI
	SYNC_GUEST_HOST();
	auto rv = PACKER_OPTIONAL_HOSTCALL(glXQueryServerString)(GuestToHostX11(dpy), screen, name OPTIONAL_HOSTCALL_LASTARG);
	SYNC_HOST_GUEST();
	return rv;
}

const char *IMPL(glXGetClientString)(Display *dpy, int name) {
    OPTIONAL_HOSTCALL_ABI
	SYNC_GUEST_HOST();
	auto rv = PACKER_OPTIONAL_HOSTCALL(glXGetClientString)(GuestToHostX11(dpy), name OPTIONAL_HOSTCALL_LASTARG);
	SYNC_HOST_GUEST();
	return rv;
}


/*GLX 1.2 and later */
Display *IMPL(glXGetCurrentDisplay)(void) {
    OPTIONAL_HOSTCALL_ABI
	return PACKER(px11_HostToGuestX11)(PACKER_OPTIONAL_HOSTCALL(glXGetCurrentDisplay)(OPTIONAL_HOSTCALL_ONLYARG));
}


/*GLX 1.3 and later */
GLXFBConfig *IMPL(glXChooseFBConfig)(Display *dpy, int screen, const int *attribList, int *nitems) {
    OPTIONAL_HOSTCALL_ABI
	SYNC_GUEST_HOST();
	auto rv = PACKER_OPTIONAL_HOSTCALL(glXChooseFBConfig)(GuestToHostX11(dpy), screen, attribList, nitems OPTIONAL_HOSTCALL_LASTARG);
	SYNC_HOST_GUEST();
	return MapGLXFBConfigHostToGuest(rv, *nitems);
}

int IMPL(glXGetFBConfigAttrib)(Display *dpy, GLXFBConfig config, int attribute, int *value) {
    OPTIONAL_HOSTCALL_ABI
	SYNC_GUEST_HOST();
	auto rv = PACKER_OPTIONAL_HOSTCALL(glXGetFBConfigAttrib)(GuestToHostX11(dpy), config, attribute, value OPTIONAL_HOSTCALL_LASTARG);
	SYNC_HOST_GUEST();
	return rv;
}

GLXFBConfig *IMPL(glXGetFBConfigs)(Display *dpy, int screen, int *nelements) {
    OPTIONAL_HOSTCALL_ABI
	SYNC_GUEST_HOST();
	auto rv = PACKER_OPTIONAL_HOSTCALL(glXGetFBConfigs)(GuestToHostX11(dpy), screen, nelements OPTIONAL_HOSTCALL_LASTARG);
	SYNC_HOST_GUEST();
	return MapGLXFBConfigHostToGuest(rv, *nelements);
}

XVisualInfo *IMPL(glXGetVisualFromFBConfig)(Display *dpy, GLXFBConfig config) {
    OPTIONAL_HOSTCALL_ABI
	SYNC_GUEST_HOST();
	auto rv = MapVisualInfoHostToGuest(dpy, PACKER_OPTIONAL_HOSTCALL(glXGetVisualFromFBConfig)(GuestToHostX11(dpy), config OPTIONAL_HOSTCALL_LASTARG));
	SYNC_HOST_GUEST();
	return rv;
}

GLXWindow IMPL(glXCreateWindow)(Display *dpy, GLXFBConfig config, Window win, const int *attribList) {
    OPTIONAL_HOSTCALL_ABI
	SYNC_GUEST_HOST();
	auto rv = PACKER_OPTIONAL_HOSTCALL(glXCreateWindow)(GuestToHostX11(dpy), config, win, attribList OPTIONAL_HOSTCALL_LASTARG);
	SYNC_HOST_GUEST();
	return rv;
}

void IMPL(glXDestroyWindow)(Display *dpy, GLXWindow window) {
    OPTIONAL_HOSTCALL_ABI
	SYNC_GUEST_HOST();
	PACKER_OPTIONAL_HOSTCALL(glXDestroyWindow)(GuestToHostX11(dpy), window OPTIONAL_HOSTCALL_LASTARG);
	SYNC_HOST_GUEST();
}

GLXPixmap IMPL(glXCreatePixmap)(Display *dpy, GLXFBConfig config, Pixmap pixmap, const int *attribList) {
    OPTIONAL_HOSTCALL_ABI
	SYNC_GUEST_HOST();
	auto rv = PACKER_OPTIONAL_HOSTCALL(glXCreatePixmap)(GuestToHostX11(dpy), config, pixmap, attribList OPTIONAL_HOSTCALL_LASTARG);
	SYNC_HOST_GUEST();
	return rv;
}

void IMPL(glXDestroyPixmap)(Display *dpy, GLXPixmap pixmap) {
    OPTIONAL_HOSTCALL_ABI
	SYNC_GUEST_HOST();
	PACKER_OPTIONAL_HOSTCALL(glXDestroyPixmap)(GuestToHostX11(dpy), pixmap OPTIONAL_HOSTCALL_LASTARG);
	SYNC_HOST_GUEST();
}

GLXPbuffer IMPL(glXCreatePbuffer)(Display *dpy, GLXFBConfig config, const int *attribList) {
    OPTIONAL_HOSTCALL_ABI
	SYNC_GUEST_HOST();
	auto rv = PACKER_OPTIONAL_HOSTCALL(glXCreatePbuffer)(GuestToHostX11(dpy), config, attribList OPTIONAL_HOSTCALL_LASTARG);
	SYNC_HOST_GUEST();
	return rv;
}

void IMPL(glXDestroyPbuffer)(Display *dpy, GLXPbuffer pbuf) {
    OPTIONAL_HOSTCALL_ABI
	SYNC_GUEST_HOST();
	PACKER_OPTIONAL_HOSTCALL(glXDestroyPbuffer)(GuestToHostX11(dpy), pbuf OPTIONAL_HOSTCALL_LASTARG);
	SYNC_HOST_GUEST();
}

void IMPL(glXQueryDrawable)(Display *dpy, GLXDrawable draw, int attribute, unsigned int *value) {
    OPTIONAL_HOSTCALL_ABI
	SYNC_GUEST_HOST();
	PACKER_OPTIONAL_HOSTCALL(glXQueryDrawable)(GuestToHostX11(dpy), draw, attribute, value OPTIONAL_HOSTCALL_LASTARG);
	SYNC_HOST_GUEST();
}

GLXContext IMPL(glXCreateNewContext)(Display *dpy, GLXFBConfig config, int renderType, GLXContext shareList, Bool direct) {
    OPTIONAL_HOSTCALL_ABI
	SYNC_GUEST_HOST();
	auto rv = PACKER_OPTIONAL_HOSTCALL(glXCreateNewContext)(GuestToHostX11(dpy), config, renderType, shareList, direct OPTIONAL_HOSTCALL_LASTARG);
	SYNC_HOST_GUEST();
	return rv;
}

Bool IMPL(glXMakeContextCurrent)(Display *dpy, GLXDrawable draw, GLXDrawable read, GLXContext ctx) {
    OPTIONAL_HOSTCALL_ABI
	SYNC_GUEST_HOST();
	auto rv = PACKER_OPTIONAL_HOSTCALL(glXMakeContextCurrent)(GuestToHostX11(dpy), draw, read, ctx OPTIONAL_HOSTCALL_LASTARG);
	SYNC_HOST_GUEST();
	return rv;
}

// FEX_TODO("Sync?")
GLXDrawable IMPL(glXGetCurrentReadDrawable)(void) {
    OPTIONAL_HOSTCALL_ABI
	auto rv = PACKER_OPTIONAL_HOSTCALL(glXGetCurrentReadDrawable)( OPTIONAL_HOSTCALL_ONLYARG);
	return rv;
}

int IMPL(glXQueryContext)(Display *dpy, GLXContext ctx, int attribute, int *value) {
    OPTIONAL_HOSTCALL_ABI
	SYNC_GUEST_HOST();
	auto rv = PACKER_OPTIONAL_HOSTCALL(glXQueryContext)(GuestToHostX11(dpy), ctx, attribute, value OPTIONAL_HOSTCALL_LASTARG);
	SYNC_HOST_GUEST();
	return rv;
}

void IMPL(glXSelectEvent)(Display *dpy, GLXDrawable drawable, unsigned long mask) {
    OPTIONAL_HOSTCALL_ABI
	SYNC_GUEST_HOST();
	PACKER_OPTIONAL_HOSTCALL(glXSelectEvent)(GuestToHostX11(dpy), drawable, mask OPTIONAL_HOSTCALL_LASTARG);
	SYNC_HOST_GUEST();
}

void IMPL(glXGetSelectedEvent)(Display *dpy, GLXDrawable drawable, unsigned long *mask) {
    OPTIONAL_HOSTCALL_ABI
	SYNC_GUEST_HOST();
	PACKER_OPTIONAL_HOSTCALL(glXGetSelectedEvent)(GuestToHostX11(dpy), drawable, mask OPTIONAL_HOSTCALL_LASTARG);
	SYNC_HOST_GUEST();
}


/*GLX 1.3 function pointer typedefs */
// Special implementation in main file
// voidFunc *IMPL(glXGetProcAddressARB)(const GLubyte *procname);

/*GLX 1.4 and later */
// Special implementation in main file
// voidFunc *IMPL(glXGetProcAddress)(const GLubyte *procname);

// FEX_TODO("what about plain glXCreateContextAttribs ?")
GLXContext IMPL(glXCreateContextAttribsARB)(Display *dpy, GLXFBConfig config, GLXContext share_context, Bool direct, const int *attrib_list) {
    OPTIONAL_HOSTCALL_ABI
	SYNC_GUEST_HOST();
	auto rv = PACKER_OPTIONAL_HOSTCALL(glXCreateContextAttribsARB)(GuestToHostX11(dpy), config, share_context, direct, attrib_list OPTIONAL_HOSTCALL_LASTARG);
	SYNC_HOST_GUEST();
	return rv;
}

void IMPL(glXSwapIntervalEXT)(Display *dpy, GLXDrawable drawable, int interval) {
    OPTIONAL_HOSTCALL_ABI
	SYNC_GUEST_HOST();
	PACKER_OPTIONAL_HOSTCALL(glXSwapIntervalEXT)(GuestToHostX11(dpy), drawable, interval OPTIONAL_HOSTCALL_LASTARG);
	SYNC_HOST_GUEST();
}

Bool IMPL(glXQueryRendererIntegerMESA) (Display *dpy, int screen, int renderer, int attribute, unsigned int *value) {
    OPTIONAL_HOSTCALL_ABI
	SYNC_GUEST_HOST();
	auto rv = PACKER_OPTIONAL_HOSTCALL(glXQueryRendererIntegerMESA)(GuestToHostX11(dpy), screen, renderer, attribute, value OPTIONAL_HOSTCALL_LASTARG);
	SYNC_HOST_GUEST();
	return rv;
}

const char *IMPL(glXQueryRendererStringMESA) (Display *dpy, int screen, int renderer, int attribute) {
    OPTIONAL_HOSTCALL_ABI
	SYNC_GUEST_HOST();
	auto rv = PACKER_OPTIONAL_HOSTCALL(glXQueryRendererStringMESA)(GuestToHostX11(dpy), screen, renderer, attribute OPTIONAL_HOSTCALL_LASTARG);
	SYNC_HOST_GUEST();
	return rv;
}

#if 0
// This is not direcly exported ?
void *glXAllocateMemoryNV(GLsizei size, GLfloat readfreq, GLfloat writefreq, GLfloat priority);
void glXFreeMemoryNV(GLvoid *pointer);
#endif

#if 0
// This is not direcly exported ?

Bool glXBindTexImageARB(Display *dpy, GLXPbuffer pbuffer, int buffer);
Bool glXReleaseTexImageARB(Display *dpy, GLXPbuffer pbuffer, int buffer);
Bool glXDrawableAttribARB(Display *dpy, GLXDrawable draw, const int *attribList);
#endif

#if 0
// This is not direcly exported ?
extern int glXGetFrameUsageMESA(Display *dpy, GLXDrawable drawable, float *usage);
extern int glXBeginFrameTrackingMESA(Display *dpy, GLXDrawable drawable);
extern int glXEndFrameTrackingMESA(Display *dpy, GLXDrawable drawable);
extern int glXQueryFrameTrackingMESA(Display *dpy, GLXDrawable drawable, int64_t *swapCount, int64_t *missedFrames, float *lastMissedUsage);
#endif

//?
GLXContext IMPL(glXImportContextEXT)(Display *dpy, GLXContextID contextID) {
    OPTIONAL_HOSTCALL_ABI
    SYNC_GUEST_HOST();
    auto rv = PACKER_OPTIONAL_HOSTCALL(glXImportContextEXT)(GuestToHostX11(dpy), contextID OPTIONAL_HOSTCALL_LASTARG);
    SYNC_HOST_GUEST();
    return rv;
}

void IMPL(glXCopySubBufferMESA)(Display *dpy, GLXDrawable drawable, int x, int y, int width, int height) {
    OPTIONAL_HOSTCALL_ABI
	SYNC_GUEST_HOST();
	PACKER_OPTIONAL_HOSTCALL(glXCopySubBufferMESA)(GuestToHostX11(dpy), drawable, x, y, width, height OPTIONAL_HOSTCALL_LASTARG);
	SYNC_HOST_GUEST();
}

Bool IMPL(glXMakeCurrentReadSGI)(Display* dpy, GLXDrawable draw, GLXDrawable read, GLXContext ctx) {
    OPTIONAL_HOSTCALL_ABI
    SYNC_GUEST_HOST();
    auto rv = PACKER_OPTIONAL_HOSTCALL(glXMakeCurrentReadSGI)(GuestToHostX11(dpy), draw, read, ctx OPTIONAL_HOSTCALL_LASTARG);
    SYNC_HOST_GUEST();
    return rv;
}

Bool IMPL(glXGetSyncValuesOML)(Display* dpy, GLXDrawable drawable, int64_t* ust, int64_t* msc, int64_t* sbc) {
  OPTIONAL_HOSTCALL_ABI
  SYNC_GUEST_HOST();
  auto rv = PACKER_OPTIONAL_HOSTCALL(glXGetSyncValuesOML)(GuestToHostX11(dpy), drawable, ust, msc, sbc OPTIONAL_HOSTCALL_LASTARG);
  SYNC_HOST_GUEST();
  return rv;
}

Bool IMPL(glXGetMscRateOML)(Display* dpy, GLXDrawable drawable, int32_t* numerator, int32_t* denominator) {
  OPTIONAL_HOSTCALL_ABI
  SYNC_GUEST_HOST();
  auto rv = PACKER_OPTIONAL_HOSTCALL(glXGetMscRateOML)(GuestToHostX11(dpy), drawable, numerator, denominator OPTIONAL_HOSTCALL_LASTARG);
  SYNC_HOST_GUEST();
  return rv;
}

int64_t IMPL(glXSwapBuffersMscOML)(Display* dpy, GLXDrawable drawable, int64_t target_msc, int64_t divisor, int64_t remainder) {
  OPTIONAL_HOSTCALL_ABI
  SYNC_GUEST_HOST();
  auto rv = PACKER_OPTIONAL_HOSTCALL(glXSwapBuffersMscOML)(GuestToHostX11(dpy), drawable, target_msc, divisor, remainder OPTIONAL_HOSTCALL_LASTARG);
  SYNC_HOST_GUEST();
  return rv;
}

Bool IMPL(glXWaitForMscOML)(Display* dpy, GLXDrawable drawable, int64_t target_msc, int64_t divisor, int64_t remainder, int64_t* ust, int64_t* msc, int64_t* sbc) {
  OPTIONAL_HOSTCALL_ABI
  SYNC_GUEST_HOST();
  auto rv = PACKER_OPTIONAL_HOSTCALL(glXWaitForMscOML)(GuestToHostX11(dpy), drawable, target_msc, divisor, remainder, ust, msc, sbc OPTIONAL_HOSTCALL_LASTARG);
  SYNC_HOST_GUEST();
  return rv;
}

Bool IMPL(glXWaitForSbcOML)(Display* dpy, GLXDrawable drawable, int64_t target_sbc, int64_t* ust, int64_t* msc, int64_t* sbc) {
  OPTIONAL_HOSTCALL_ABI
  SYNC_GUEST_HOST();
  auto rv = PACKER_OPTIONAL_HOSTCALL(glXWaitForSbcOML)(GuestToHostX11(dpy), drawable, target_sbc, ust, msc, sbc OPTIONAL_HOSTCALL_LASTARG);
  SYNC_HOST_GUEST();
  return rv;
}

int IMPL(glXQueryContextInfoEXT)(Display * dpy, GLXContext ctx, int attribute, int *value) {
  OPTIONAL_HOSTCALL_ABI
  SYNC_GUEST_HOST();
  auto rv = PACKER_OPTIONAL_HOSTCALL(glXQueryContextInfoEXT)(GuestToHostX11(dpy), ctx, attribute, value OPTIONAL_HOSTCALL_LASTARG);
  SYNC_HOST_GUEST();
  return rv;
}

void IMPL(glXBindTexImageEXT)(Display *dpy, GLXDrawable drawable, int buffer, const int *attrib_list) {
  OPTIONAL_HOSTCALL_ABI
  SYNC_GUEST_HOST();
  PACKER_OPTIONAL_HOSTCALL(glXBindTexImageEXT)(GuestToHostX11(dpy), drawable, buffer, attrib_list OPTIONAL_HOSTCALL_LASTARG);
  SYNC_HOST_GUEST();
}

void IMPL(glXReleaseTexImageEXT) (Display *dpy, GLXDrawable drawable, int buffer) {
  OPTIONAL_HOSTCALL_ABI
  SYNC_GUEST_HOST();
  PACKER_OPTIONAL_HOSTCALL(glXReleaseTexImageEXT)(GuestToHostX11(dpy), drawable, buffer OPTIONAL_HOSTCALL_LASTARG);
  SYNC_HOST_GUEST();
}

void IMPL(glXFreeContextEXT)(Display * dpy, GLXContext ctx) {
  OPTIONAL_HOSTCALL_ABI
  SYNC_GUEST_HOST();
  PACKER_OPTIONAL_HOSTCALL(glXFreeContextEXT)(GuestToHostX11(dpy), ctx OPTIONAL_HOSTCALL_LASTARG);
  SYNC_HOST_GUEST();
}


GLXPbuffer IMPL(glXCreateGLXPbufferSGIX)(Display *dpy, GLXFBConfig config, unsigned int width, unsigned int height, int *attrib_list) {
  OPTIONAL_HOSTCALL_ABI
  SYNC_GUEST_HOST();
  auto rv = PACKER_OPTIONAL_HOSTCALL(glXCreateGLXPbufferSGIX)(GuestToHostX11(dpy), config, width, height, attrib_list OPTIONAL_HOSTCALL_LASTARG);
  SYNC_HOST_GUEST();
  return rv;
}

void IMPL(glXDestroyGLXPbufferSGIX)(Display *dpy, GLXPbuffer pbuf) {
  OPTIONAL_HOSTCALL_ABI
  SYNC_GUEST_HOST();
  PACKER_OPTIONAL_HOSTCALL(glXDestroyGLXPbufferSGIX)(GuestToHostX11(dpy), pbuf OPTIONAL_HOSTCALL_LASTARG);
  SYNC_HOST_GUEST();
}

void IMPL(glXQueryGLXPbufferSGIX)(Display *dpy, GLXPbuffer pbuf, int attribute, unsigned int *value) {
  OPTIONAL_HOSTCALL_ABI
  SYNC_GUEST_HOST();
  PACKER_OPTIONAL_HOSTCALL(glXQueryGLXPbufferSGIX)(GuestToHostX11(dpy), pbuf, attribute, value OPTIONAL_HOSTCALL_LASTARG);
  SYNC_HOST_GUEST();
}

void IMPL(glXSelectEventSGIX)(Display *dpy, GLXDrawable drawable, unsigned long mask) {
  OPTIONAL_HOSTCALL_ABI
  SYNC_GUEST_HOST();
  PACKER_OPTIONAL_HOSTCALL(glXSelectEventSGIX)(GuestToHostX11(dpy), drawable, mask OPTIONAL_HOSTCALL_LASTARG);
  SYNC_HOST_GUEST();
}

void IMPL(glXGetSelectedEventSGIX)(Display *dpy, GLXDrawable drawable, unsigned long *mask) {
  OPTIONAL_HOSTCALL_ABI
  SYNC_GUEST_HOST();
  PACKER_OPTIONAL_HOSTCALL(glXGetSelectedEventSGIX)(GuestToHostX11(dpy), drawable, mask OPTIONAL_HOSTCALL_LASTARG);
  SYNC_HOST_GUEST();
}

XVisualInfo *IMPL(glXGetVisualFromFBConfigSGIX)(Display *dpy, GLXFBConfigSGIX config) {
  OPTIONAL_HOSTCALL_ABI
  SYNC_GUEST_HOST();
  auto rv = MapVisualInfoHostToGuest(dpy, PACKER_OPTIONAL_HOSTCALL(glXGetVisualFromFBConfigSGIX)(GuestToHostX11(dpy), config OPTIONAL_HOSTCALL_LASTARG));
  SYNC_HOST_GUEST();
  return rv;
}


GLXContext IMPL(glXCreateContextWithConfigSGIX)(Display *dpy, GLXFBConfigSGIX config, int render_type, GLXContext share_list, Bool direct)	{
  OPTIONAL_HOSTCALL_ABI
  SYNC_GUEST_HOST();
  auto rv = PACKER_OPTIONAL_HOSTCALL(glXCreateContextWithConfigSGIX)(GuestToHostX11(dpy), config, render_type, share_list, direct OPTIONAL_HOSTCALL_LASTARG);
  SYNC_HOST_GUEST();
  return rv;
}


GLXFBConfigSGIX *IMPL(glXChooseFBConfigSGIX)(Display *dpy, int screen, int *attrib_list, int *nelements) {
  OPTIONAL_HOSTCALL_ABI
  SYNC_GUEST_HOST();
  auto rv = PACKER_OPTIONAL_HOSTCALL(glXChooseFBConfigSGIX)(GuestToHostX11(dpy), screen, attrib_list, nelements OPTIONAL_HOSTCALL_LASTARG);
  SYNC_HOST_GUEST();
  return MapGLXFBConfigSGIXHostToGuest(rv, *nelements);
}


GLXFBConfigSGIX IMPL(glXGetFBConfigFromVisualSGIX)(Display *dpy, XVisualInfo *vis) {
  OPTIONAL_HOSTCALL_ABI
  SYNC_GUEST_HOST();
  auto rv = PACKER_OPTIONAL_HOSTCALL(glXGetFBConfigFromVisualSGIX)(GuestToHostX11(dpy), MapVisualInfoGuestToHost(dpy, vis) OPTIONAL_HOSTCALL_LASTARG);
  SYNC_HOST_GUEST();
  return rv;
}


GLXPixmap IMPL(glXCreateGLXPixmapWithConfigSGIX)(Display *dpy, GLXFBConfigSGIX config, Pixmap pixmap)	{
  OPTIONAL_HOSTCALL_ABI
  SYNC_GUEST_HOST();
  auto rv = PACKER_OPTIONAL_HOSTCALL(glXCreateGLXPixmapWithConfigSGIX)(GuestToHostX11(dpy), config, pixmap OPTIONAL_HOSTCALL_LASTARG);
  SYNC_HOST_GUEST();
  return rv;
}


int IMPL(glXGetFBConfigAttribSGIX)(Display *dpy, GLXFBConfigSGIX config, int attribute, int *value)	{
  OPTIONAL_HOSTCALL_ABI
  SYNC_GUEST_HOST();
  auto rv = PACKER_OPTIONAL_HOSTCALL(glXGetFBConfigAttribSGIX)(GuestToHostX11(dpy), config, attribute, value OPTIONAL_HOSTCALL_LASTARG);
  SYNC_HOST_GUEST();
  return rv;
}

/*
	glx also exports __glXGLLoadGLXFunction and 
	__GLXGL_CORE_FUNCTIONS (data symbol)
*/