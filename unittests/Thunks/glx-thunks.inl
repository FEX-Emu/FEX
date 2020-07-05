// thunks
extern "C" {
MAKE_THUNK(libGL, glXChooseVisual)

XVisualInfo * glXChooseVisual(Display * a_0,int a_1,int * a_2){
struct {Display * a_0;int a_1;int * a_2;XVisualInfo * rv;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
fexthunks_libGL_glXChooseVisual(&args);
return args.rv;
}

MAKE_THUNK(libGL, glXCreateContext)

GLXContext glXCreateContext(Display * a_0,XVisualInfo * a_1,GLXContext a_2,Bool a_3){
struct {Display * a_0;XVisualInfo * a_1;GLXContext a_2;Bool a_3;GLXContext rv;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;
fexthunks_libGL_glXCreateContext(&args);
return args.rv;
}

MAKE_THUNK(libGL, glXDestroyContext)

void glXDestroyContext(Display * a_0,GLXContext a_1){
struct {Display * a_0;GLXContext a_1;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libGL_glXDestroyContext(&args);
}

MAKE_THUNK(libGL, glXMakeCurrent)

Bool glXMakeCurrent(Display * a_0,GLXDrawable a_1,GLXContext a_2){
struct {Display * a_0;GLXDrawable a_1;GLXContext a_2;Bool rv;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
fexthunks_libGL_glXMakeCurrent(&args);
return args.rv;
}

MAKE_THUNK(libGL, glXCopyContext)

void glXCopyContext(Display * a_0,GLXContext a_1,GLXContext a_2,long unsigned int a_3){
struct {Display * a_0;GLXContext a_1;GLXContext a_2;long unsigned int a_3;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;
fexthunks_libGL_glXCopyContext(&args);
}

MAKE_THUNK(libGL, glXSwapBuffers)

void glXSwapBuffers(Display * a_0,GLXDrawable a_1){
struct {Display * a_0;GLXDrawable a_1;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libGL_glXSwapBuffers(&args);
}

MAKE_THUNK(libGL, glXCreateGLXPixmap)

GLXPixmap glXCreateGLXPixmap(Display * a_0,XVisualInfo * a_1,Pixmap a_2){
struct {Display * a_0;XVisualInfo * a_1;Pixmap a_2;GLXPixmap rv;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
fexthunks_libGL_glXCreateGLXPixmap(&args);
return args.rv;
}

MAKE_THUNK(libGL, glXDestroyGLXPixmap)

void glXDestroyGLXPixmap(Display * a_0,GLXPixmap a_1){
struct {Display * a_0;GLXPixmap a_1;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libGL_glXDestroyGLXPixmap(&args);
}

MAKE_THUNK(libGL, glXQueryExtension)

Bool glXQueryExtension(Display * a_0,int * a_1,int * a_2){
struct {Display * a_0;int * a_1;int * a_2;Bool rv;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
fexthunks_libGL_glXQueryExtension(&args);
return args.rv;
}

MAKE_THUNK(libGL, glXQueryVersion)

Bool glXQueryVersion(Display * a_0,int * a_1,int * a_2){
struct {Display * a_0;int * a_1;int * a_2;Bool rv;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
fexthunks_libGL_glXQueryVersion(&args);
return args.rv;
}

MAKE_THUNK(libGL, glXIsDirect)

Bool glXIsDirect(Display * a_0,GLXContext a_1){
struct {Display * a_0;GLXContext a_1;Bool rv;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libGL_glXIsDirect(&args);
return args.rv;
}

MAKE_THUNK(libGL, glXGetConfig)

int glXGetConfig(Display * a_0,XVisualInfo * a_1,int a_2,int * a_3){
struct {Display * a_0;XVisualInfo * a_1;int a_2;int * a_3;int rv;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;
fexthunks_libGL_glXGetConfig(&args);
return args.rv;
}

MAKE_THUNK(libGL, glXGetCurrentContext)

GLXContext glXGetCurrentContext(){
struct {GLXContext rv;} args;

fexthunks_libGL_glXGetCurrentContext(&args);
return args.rv;
}

MAKE_THUNK(libGL, glXGetCurrentDrawable)

GLXDrawable glXGetCurrentDrawable(){
struct {GLXDrawable rv;} args;

fexthunks_libGL_glXGetCurrentDrawable(&args);
return args.rv;
}

MAKE_THUNK(libGL, glXWaitGL)

void glXWaitGL(){
struct {} args;

fexthunks_libGL_glXWaitGL(&args);
}

MAKE_THUNK(libGL, glXWaitX)

void glXWaitX(){
struct {} args;

fexthunks_libGL_glXWaitX(&args);
}

MAKE_THUNK(libGL, glXUseXFont)

void glXUseXFont(Font a_0,int a_1,int a_2,int a_3){
struct {Font a_0;int a_1;int a_2;int a_3;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;
fexthunks_libGL_glXUseXFont(&args);
}

MAKE_THUNK(libGL, glXQueryExtensionsString)

const char * glXQueryExtensionsString(Display * a_0,int a_1){
struct {Display * a_0;int a_1;const char * rv;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libGL_glXQueryExtensionsString(&args);
return args.rv;
}

MAKE_THUNK(libGL, glXQueryServerString)

const char * glXQueryServerString(Display * a_0,int a_1,int a_2){
struct {Display * a_0;int a_1;int a_2;const char * rv;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
fexthunks_libGL_glXQueryServerString(&args);
return args.rv;
}

MAKE_THUNK(libGL, glXGetClientString)

const char * glXGetClientString(Display * a_0,int a_1){
struct {Display * a_0;int a_1;const char * rv;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libGL_glXGetClientString(&args);
return args.rv;
}

MAKE_THUNK(libGL, glXCreateContextAttribsARB)

GLXContext glXCreateContextAttribsARB(Display * a_0,GLXFBConfig a_1,GLXContext a_2,Bool a_3,const int * a_4){
struct {Display * a_0;GLXFBConfig a_1;GLXContext a_2;Bool a_3;const int * a_4;GLXContext rv;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;args.a_4 = a_4;
fexthunks_libGL_glXCreateContextAttribsARB(&args);
return args.rv;
}

MAKE_THUNK(libGL, glXChooseFBConfig)

GLXFBConfig * glXChooseFBConfig(Display * a_0,int a_1,const int * a_2,int * a_3){
struct {Display * a_0;int a_1;const int * a_2;int * a_3;GLXFBConfig * rv;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;
fexthunks_libGL_glXChooseFBConfig(&args);
return args.rv;
}

MAKE_THUNK(libGL, glClearColor)

void glClearColor(GLclampf a_0,GLclampf a_1,GLclampf a_2,GLclampf a_3){
struct {GLclampf a_0;GLclampf a_1;GLclampf a_2;GLclampf a_3;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;
fexthunks_libGL_glClearColor(&args);
}

MAKE_THUNK(libGL, glClear)

void glClear(GLbitfield a_0){
struct {GLbitfield a_0;} args;
args.a_0 = a_0;
fexthunks_libGL_glClear(&args);
}

MAKE_THUNK(libX11, XOpenDisplay)

Display * XOpenDisplay(const char * a_0){
struct {const char * a_0;Display * rv;} args;
args.a_0 = a_0;
fexthunks_libX11_XOpenDisplay(&args);
return args.rv;
}

MAKE_THUNK(libX11, XCreateColormap)

Colormap XCreateColormap(Display * a_0,Window a_1,Visual * a_2,int a_3){
struct {Display * a_0;Window a_1;Visual * a_2;int a_3;Colormap rv;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;
fexthunks_libX11_XCreateColormap(&args);
return args.rv;
}

MAKE_THUNK(libX11, XCreateWindow)

Window XCreateWindow(Display * a_0,Window a_1,int a_2,int a_3,unsigned int a_4,unsigned int a_5,unsigned int a_6,int a_7,unsigned int a_8,Visual * a_9,unsigned long a_10,XSetWindowAttributes * a_11){
struct {Display * a_0;Window a_1;int a_2;int a_3;unsigned int a_4;unsigned int a_5;unsigned int a_6;int a_7;unsigned int a_8;Visual * a_9;unsigned long a_10;XSetWindowAttributes * a_11;Window rv;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;args.a_4 = a_4;args.a_5 = a_5;args.a_6 = a_6;args.a_7 = a_7;args.a_8 = a_8;args.a_9 = a_9;args.a_10 = a_10;args.a_11 = a_11;
fexthunks_libX11_XCreateWindow(&args);
return args.rv;
}

MAKE_THUNK(libX11, XMapWindow)

int XMapWindow(Display * a_0,Window a_1){
struct {Display * a_0;Window a_1;int rv;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libX11_XMapWindow(&args);
return args.rv;
}

}
