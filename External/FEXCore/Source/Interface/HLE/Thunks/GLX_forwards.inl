// initializers
void* fexthunks_impl_libGL_so;
typedef XVisualInfo * fexthunks_type_libGL_glXChooseVisual(Display * a_0,int a_1,int * a_2);
fexthunks_type_libGL_glXChooseVisual *fexthunks_impl_libGL_glXChooseVisual;
typedef GLXContext fexthunks_type_libGL_glXCreateContext(Display * a_0,XVisualInfo * a_1,GLXContext a_2,Bool a_3);
fexthunks_type_libGL_glXCreateContext *fexthunks_impl_libGL_glXCreateContext;
typedef void fexthunks_type_libGL_glXDestroyContext(Display * a_0,GLXContext a_1);
fexthunks_type_libGL_glXDestroyContext *fexthunks_impl_libGL_glXDestroyContext;
typedef Bool fexthunks_type_libGL_glXMakeCurrent(Display * a_0,GLXDrawable a_1,GLXContext a_2);
fexthunks_type_libGL_glXMakeCurrent *fexthunks_impl_libGL_glXMakeCurrent;
typedef void fexthunks_type_libGL_glXCopyContext(Display * a_0,GLXContext a_1,GLXContext a_2,long unsigned int a_3);
fexthunks_type_libGL_glXCopyContext *fexthunks_impl_libGL_glXCopyContext;
typedef void fexthunks_type_libGL_glXSwapBuffers(Display * a_0,GLXDrawable a_1);
fexthunks_type_libGL_glXSwapBuffers *fexthunks_impl_libGL_glXSwapBuffers;
typedef GLXPixmap fexthunks_type_libGL_glXCreateGLXPixmap(Display * a_0,XVisualInfo * a_1,Pixmap a_2);
fexthunks_type_libGL_glXCreateGLXPixmap *fexthunks_impl_libGL_glXCreateGLXPixmap;
typedef void fexthunks_type_libGL_glXDestroyGLXPixmap(Display * a_0,GLXPixmap a_1);
fexthunks_type_libGL_glXDestroyGLXPixmap *fexthunks_impl_libGL_glXDestroyGLXPixmap;
typedef Bool fexthunks_type_libGL_glXQueryExtension(Display * a_0,int * a_1,int * a_2);
fexthunks_type_libGL_glXQueryExtension *fexthunks_impl_libGL_glXQueryExtension;
typedef Bool fexthunks_type_libGL_glXQueryVersion(Display * a_0,int * a_1,int * a_2);
fexthunks_type_libGL_glXQueryVersion *fexthunks_impl_libGL_glXQueryVersion;
typedef Bool fexthunks_type_libGL_glXIsDirect(Display * a_0,GLXContext a_1);
fexthunks_type_libGL_glXIsDirect *fexthunks_impl_libGL_glXIsDirect;
typedef int fexthunks_type_libGL_glXGetConfig(Display * a_0,XVisualInfo * a_1,int a_2,int * a_3);
fexthunks_type_libGL_glXGetConfig *fexthunks_impl_libGL_glXGetConfig;
typedef GLXContext fexthunks_type_libGL_glXGetCurrentContext();
fexthunks_type_libGL_glXGetCurrentContext *fexthunks_impl_libGL_glXGetCurrentContext;
typedef GLXDrawable fexthunks_type_libGL_glXGetCurrentDrawable();
fexthunks_type_libGL_glXGetCurrentDrawable *fexthunks_impl_libGL_glXGetCurrentDrawable;
typedef void fexthunks_type_libGL_glXWaitGL();
fexthunks_type_libGL_glXWaitGL *fexthunks_impl_libGL_glXWaitGL;
typedef void fexthunks_type_libGL_glXWaitX();
fexthunks_type_libGL_glXWaitX *fexthunks_impl_libGL_glXWaitX;
typedef void fexthunks_type_libGL_glXUseXFont(Font a_0,int a_1,int a_2,int a_3);
fexthunks_type_libGL_glXUseXFont *fexthunks_impl_libGL_glXUseXFont;
typedef const char * fexthunks_type_libGL_glXQueryExtensionsString(Display * a_0,int a_1);
fexthunks_type_libGL_glXQueryExtensionsString *fexthunks_impl_libGL_glXQueryExtensionsString;
typedef const char * fexthunks_type_libGL_glXQueryServerString(Display * a_0,int a_1,int a_2);
fexthunks_type_libGL_glXQueryServerString *fexthunks_impl_libGL_glXQueryServerString;
typedef const char * fexthunks_type_libGL_glXGetClientString(Display * a_0,int a_1);
fexthunks_type_libGL_glXGetClientString *fexthunks_impl_libGL_glXGetClientString;
typedef GLXContext fexthunks_type_libGL_glXCreateContextAttribsARB(Display * a_0,GLXFBConfig a_1,GLXContext a_2,Bool a_3,const int * a_4);
fexthunks_type_libGL_glXCreateContextAttribsARB *fexthunks_impl_libGL_glXCreateContextAttribsARB;
typedef GLXFBConfig * fexthunks_type_libGL_glXChooseFBConfig(Display * a_0,int a_1,const int * a_2,int * a_3);
fexthunks_type_libGL_glXChooseFBConfig *fexthunks_impl_libGL_glXChooseFBConfig;
typedef void fexthunks_type_libGL_glClearColor(GLclampf a_0,GLclampf a_1,GLclampf a_2,GLclampf a_3);
fexthunks_type_libGL_glClearColor *fexthunks_impl_libGL_glClearColor;
typedef void fexthunks_type_libGL_glClear(GLbitfield a_0);
fexthunks_type_libGL_glClear *fexthunks_impl_libGL_glClear;
bool fexthunks_init_libGL() {
fexthunks_impl_libGL_so = dlopen("libGL.so", RTLD_LOCAL | RTLD_LAZY);
if (!fexthunks_impl_libGL_so) { return false; }
(void*&)fexthunks_impl_libGL_glXChooseVisual = dlsym(fexthunks_impl_libGL_so, "glXChooseVisual");
(void*&)fexthunks_impl_libGL_glXCreateContext = dlsym(fexthunks_impl_libGL_so, "glXCreateContext");
(void*&)fexthunks_impl_libGL_glXDestroyContext = dlsym(fexthunks_impl_libGL_so, "glXDestroyContext");
(void*&)fexthunks_impl_libGL_glXMakeCurrent = dlsym(fexthunks_impl_libGL_so, "glXMakeCurrent");
(void*&)fexthunks_impl_libGL_glXCopyContext = dlsym(fexthunks_impl_libGL_so, "glXCopyContext");
(void*&)fexthunks_impl_libGL_glXSwapBuffers = dlsym(fexthunks_impl_libGL_so, "glXSwapBuffers");
(void*&)fexthunks_impl_libGL_glXCreateGLXPixmap = dlsym(fexthunks_impl_libGL_so, "glXCreateGLXPixmap");
(void*&)fexthunks_impl_libGL_glXDestroyGLXPixmap = dlsym(fexthunks_impl_libGL_so, "glXDestroyGLXPixmap");
(void*&)fexthunks_impl_libGL_glXQueryExtension = dlsym(fexthunks_impl_libGL_so, "glXQueryExtension");
(void*&)fexthunks_impl_libGL_glXQueryVersion = dlsym(fexthunks_impl_libGL_so, "glXQueryVersion");
(void*&)fexthunks_impl_libGL_glXIsDirect = dlsym(fexthunks_impl_libGL_so, "glXIsDirect");
(void*&)fexthunks_impl_libGL_glXGetConfig = dlsym(fexthunks_impl_libGL_so, "glXGetConfig");
(void*&)fexthunks_impl_libGL_glXGetCurrentContext = dlsym(fexthunks_impl_libGL_so, "glXGetCurrentContext");
(void*&)fexthunks_impl_libGL_glXGetCurrentDrawable = dlsym(fexthunks_impl_libGL_so, "glXGetCurrentDrawable");
(void*&)fexthunks_impl_libGL_glXWaitGL = dlsym(fexthunks_impl_libGL_so, "glXWaitGL");
(void*&)fexthunks_impl_libGL_glXWaitX = dlsym(fexthunks_impl_libGL_so, "glXWaitX");
(void*&)fexthunks_impl_libGL_glXUseXFont = dlsym(fexthunks_impl_libGL_so, "glXUseXFont");
(void*&)fexthunks_impl_libGL_glXQueryExtensionsString = dlsym(fexthunks_impl_libGL_so, "glXQueryExtensionsString");
(void*&)fexthunks_impl_libGL_glXQueryServerString = dlsym(fexthunks_impl_libGL_so, "glXQueryServerString");
(void*&)fexthunks_impl_libGL_glXGetClientString = dlsym(fexthunks_impl_libGL_so, "glXGetClientString");
(void*&)fexthunks_impl_libGL_glXCreateContextAttribsARB = dlsym(fexthunks_impl_libGL_so, "glXCreateContextAttribsARB");
(void*&)fexthunks_impl_libGL_glXChooseFBConfig = dlsym(fexthunks_impl_libGL_so, "glXChooseFBConfig");
(void*&)fexthunks_impl_libGL_glClearColor = dlsym(fexthunks_impl_libGL_so, "glClearColor");
(void*&)fexthunks_impl_libGL_glClear = dlsym(fexthunks_impl_libGL_so, "glClear");
return true;
}
void* fexthunks_impl_libX11_so;
typedef Display * fexthunks_type_libX11_XOpenDisplay(const char * a_0);
fexthunks_type_libX11_XOpenDisplay *fexthunks_impl_libX11_XOpenDisplay;
typedef Colormap fexthunks_type_libX11_XCreateColormap(Display * a_0,Window a_1,Visual * a_2,int a_3);
fexthunks_type_libX11_XCreateColormap *fexthunks_impl_libX11_XCreateColormap;
typedef Window fexthunks_type_libX11_XCreateWindow(Display * a_0,Window a_1,int a_2,int a_3,unsigned int a_4,unsigned int a_5,unsigned int a_6,int a_7,unsigned int a_8,Visual * a_9,unsigned long a_10,XSetWindowAttributes * a_11);
fexthunks_type_libX11_XCreateWindow *fexthunks_impl_libX11_XCreateWindow;
typedef int fexthunks_type_libX11_XMapWindow(Display * a_0,Window a_1);
fexthunks_type_libX11_XMapWindow *fexthunks_impl_libX11_XMapWindow;
bool fexthunks_init_libX11() {
fexthunks_impl_libX11_so = dlopen("libX11.so", RTLD_LOCAL | RTLD_LAZY);
if (!fexthunks_impl_libX11_so) { return false; }
(void*&)fexthunks_impl_libX11_XOpenDisplay = dlsym(fexthunks_impl_libX11_so, "XOpenDisplay");
(void*&)fexthunks_impl_libX11_XCreateColormap = dlsym(fexthunks_impl_libX11_so, "XCreateColormap");
(void*&)fexthunks_impl_libX11_XCreateWindow = dlsym(fexthunks_impl_libX11_so, "XCreateWindow");
(void*&)fexthunks_impl_libX11_XMapWindow = dlsym(fexthunks_impl_libX11_so, "XMapWindow");
return true;
}
// forwards
void fexthunks_forward_libGL_glXChooseVisual(void *argsv){
struct arg_t {Display * a_0;int a_1;int * a_2;XVisualInfo * rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libGL_glXChooseVisual
(args->a_0,args->a_1,args->a_2);
}
void fexthunks_forward_libGL_glXCreateContext(void *argsv){
struct arg_t {Display * a_0;XVisualInfo * a_1;GLXContext a_2;Bool a_3;GLXContext rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libGL_glXCreateContext
(args->a_0,args->a_1,args->a_2,args->a_3);
}
void fexthunks_forward_libGL_glXDestroyContext(void *argsv){
struct arg_t {Display * a_0;GLXContext a_1;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glXDestroyContext
(args->a_0,args->a_1);
}
void fexthunks_forward_libGL_glXMakeCurrent(void *argsv){
struct arg_t {Display * a_0;GLXDrawable a_1;GLXContext a_2;Bool rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libGL_glXMakeCurrent
(args->a_0,args->a_1,args->a_2);
}
void fexthunks_forward_libGL_glXCopyContext(void *argsv){
struct arg_t {Display * a_0;GLXContext a_1;GLXContext a_2;long unsigned int a_3;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glXCopyContext
(args->a_0,args->a_1,args->a_2,args->a_3);
}
void fexthunks_forward_libGL_glXSwapBuffers(void *argsv){
struct arg_t {Display * a_0;GLXDrawable a_1;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glXSwapBuffers
(args->a_0,args->a_1);
}
void fexthunks_forward_libGL_glXCreateGLXPixmap(void *argsv){
struct arg_t {Display * a_0;XVisualInfo * a_1;Pixmap a_2;GLXPixmap rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libGL_glXCreateGLXPixmap
(args->a_0,args->a_1,args->a_2);
}
void fexthunks_forward_libGL_glXDestroyGLXPixmap(void *argsv){
struct arg_t {Display * a_0;GLXPixmap a_1;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glXDestroyGLXPixmap
(args->a_0,args->a_1);
}
void fexthunks_forward_libGL_glXQueryExtension(void *argsv){
struct arg_t {Display * a_0;int * a_1;int * a_2;Bool rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libGL_glXQueryExtension
(args->a_0,args->a_1,args->a_2);
}
void fexthunks_forward_libGL_glXQueryVersion(void *argsv){
struct arg_t {Display * a_0;int * a_1;int * a_2;Bool rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libGL_glXQueryVersion
(args->a_0,args->a_1,args->a_2);
}
void fexthunks_forward_libGL_glXIsDirect(void *argsv){
struct arg_t {Display * a_0;GLXContext a_1;Bool rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libGL_glXIsDirect
(args->a_0,args->a_1);
}
void fexthunks_forward_libGL_glXGetConfig(void *argsv){
struct arg_t {Display * a_0;XVisualInfo * a_1;int a_2;int * a_3;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libGL_glXGetConfig
(args->a_0,args->a_1,args->a_2,args->a_3);
}
void fexthunks_forward_libGL_glXGetCurrentContext(void *argsv){
struct arg_t {GLXContext rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libGL_glXGetCurrentContext
();
}
void fexthunks_forward_libGL_glXGetCurrentDrawable(void *argsv){
struct arg_t {GLXDrawable rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libGL_glXGetCurrentDrawable
();
}
void fexthunks_forward_libGL_glXWaitGL(void *argsv){
struct arg_t {};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glXWaitGL
();
}
void fexthunks_forward_libGL_glXWaitX(void *argsv){
struct arg_t {};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glXWaitX
();
}
void fexthunks_forward_libGL_glXUseXFont(void *argsv){
struct arg_t {Font a_0;int a_1;int a_2;int a_3;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glXUseXFont
(args->a_0,args->a_1,args->a_2,args->a_3);
}
void fexthunks_forward_libGL_glXQueryExtensionsString(void *argsv){
struct arg_t {Display * a_0;int a_1;const char * rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libGL_glXQueryExtensionsString
(args->a_0,args->a_1);
}
void fexthunks_forward_libGL_glXQueryServerString(void *argsv){
struct arg_t {Display * a_0;int a_1;int a_2;const char * rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libGL_glXQueryServerString
(args->a_0,args->a_1,args->a_2);
}
void fexthunks_forward_libGL_glXGetClientString(void *argsv){
struct arg_t {Display * a_0;int a_1;const char * rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libGL_glXGetClientString
(args->a_0,args->a_1);
}
void fexthunks_forward_libGL_glXCreateContextAttribsARB(void *argsv){
struct arg_t {Display * a_0;GLXFBConfig a_1;GLXContext a_2;Bool a_3;const int * a_4;GLXContext rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libGL_glXCreateContextAttribsARB
(args->a_0,args->a_1,args->a_2,args->a_3,args->a_4);
}
void fexthunks_forward_libGL_glXChooseFBConfig(void *argsv){
struct arg_t {Display * a_0;int a_1;const int * a_2;int * a_3;GLXFBConfig * rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libGL_glXChooseFBConfig
(args->a_0,args->a_1,args->a_2,args->a_3);
}
void fexthunks_forward_libGL_glClearColor(void *argsv){
struct arg_t {GLclampf a_0;GLclampf a_1;GLclampf a_2;GLclampf a_3;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glClearColor
(args->a_0,args->a_1,args->a_2,args->a_3);
}
void fexthunks_forward_libGL_glClear(void *argsv){
struct arg_t {GLbitfield a_0;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glClear
(args->a_0);
}
void fexthunks_forward_libX11_XOpenDisplay(void *argsv){
struct arg_t {const char * a_0;Display * rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XOpenDisplay
(args->a_0);
}
void fexthunks_forward_libX11_XCreateColormap(void *argsv){
struct arg_t {Display * a_0;Window a_1;Visual * a_2;int a_3;Colormap rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XCreateColormap
(args->a_0,args->a_1,args->a_2,args->a_3);
}
void fexthunks_forward_libX11_XCreateWindow(void *argsv){
struct arg_t {Display * a_0;Window a_1;int a_2;int a_3;unsigned int a_4;unsigned int a_5;unsigned int a_6;int a_7;unsigned int a_8;Visual * a_9;unsigned long a_10;XSetWindowAttributes * a_11;Window rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XCreateWindow
(args->a_0,args->a_1,args->a_2,args->a_3,args->a_4,args->a_5,args->a_6,args->a_7,args->a_8,args->a_9,args->a_10,args->a_11);
}
void fexthunks_forward_libX11_XMapWindow(void *argsv){
struct arg_t {Display * a_0;Window a_1;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XMapWindow
(args->a_0,args->a_1);
}
