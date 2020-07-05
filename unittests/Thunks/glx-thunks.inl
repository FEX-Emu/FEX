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

MAKE_THUNK(libGL, glXQueryDrawable)

void glXQueryDrawable(Display* a_0,GLXDrawable a_1,int a_2,unsigned int* a_3){
struct {Display* a_0;GLXDrawable a_1;int a_2;unsigned int* a_3;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;
fexthunks_libGL_glXQueryDrawable(&args);
}

MAKE_THUNK(libGL, glXGetSwapIntervalMESA)

int glXGetSwapIntervalMESA(unsigned int a_0){
struct {unsigned int a_0;int rv;} args;
args.a_0 = a_0;
fexthunks_libGL_glXGetSwapIntervalMESA(&args);
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

MAKE_THUNK(libGL, glBegin)

void glBegin(GLenum a_0){
struct {GLenum a_0;} args;
args.a_0 = a_0;
fexthunks_libGL_glBegin(&args);
}

MAKE_THUNK(libGL, glCallList)

void glCallList(GLuint a_0){
struct {GLuint a_0;} args;
args.a_0 = a_0;
fexthunks_libGL_glCallList(&args);
}

MAKE_THUNK(libGL, glDeleteLists)

void glDeleteLists(GLuint a_0,GLsizei a_1){
struct {GLuint a_0;GLsizei a_1;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libGL_glDeleteLists(&args);
}

MAKE_THUNK(libGL, glDrawBuffer)

void glDrawBuffer(GLenum a_0){
struct {GLenum a_0;} args;
args.a_0 = a_0;
fexthunks_libGL_glDrawBuffer(&args);
}

MAKE_THUNK(libGL, glEnable)

void glEnable(GLenum a_0){
struct {GLenum a_0;} args;
args.a_0 = a_0;
fexthunks_libGL_glEnable(&args);
}

MAKE_THUNK(libGL, glFrustum)

void glFrustum(GLdouble a_0,GLdouble a_1,GLdouble a_2,GLdouble a_3,GLdouble a_4,GLdouble a_5){
struct {GLdouble a_0;GLdouble a_1;GLdouble a_2;GLdouble a_3;GLdouble a_4;GLdouble a_5;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;args.a_4 = a_4;args.a_5 = a_5;
fexthunks_libGL_glFrustum(&args);
}

MAKE_THUNK(libGL, glGenLists)

GLuint glGenLists(GLsizei a_0){
struct {GLsizei a_0;GLuint rv;} args;
args.a_0 = a_0;
fexthunks_libGL_glGenLists(&args);
return args.rv;
}

MAKE_THUNK(libGL, glGetString)

const GLubyte * glGetString(GLenum a_0){
struct {GLenum a_0;const GLubyte * rv;} args;
args.a_0 = a_0;
fexthunks_libGL_glGetString(&args);
return args.rv;
}

MAKE_THUNK(libGL, glLightfv)

void glLightfv(GLenum a_0,GLenum a_1,const GLfloat* a_2){
struct {GLenum a_0;GLenum a_1;const GLfloat* a_2;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
fexthunks_libGL_glLightfv(&args);
}

MAKE_THUNK(libGL, glMaterialfv)

void glMaterialfv(GLenum a_0,GLenum a_1,const GLfloat* a_2){
struct {GLenum a_0;GLenum a_1;const GLfloat* a_2;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
fexthunks_libGL_glMaterialfv(&args);
}

MAKE_THUNK(libGL, glMatrixMode)

void glMatrixMode(GLenum a_0){
struct {GLenum a_0;} args;
args.a_0 = a_0;
fexthunks_libGL_glMatrixMode(&args);
}

MAKE_THUNK(libGL, glNewList)

void glNewList(GLuint a_0,GLenum a_1){
struct {GLuint a_0;GLenum a_1;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libGL_glNewList(&args);
}

MAKE_THUNK(libGL, glNormal3f)

void glNormal3f(GLfloat a_0,GLfloat a_1,GLfloat a_2){
struct {GLfloat a_0;GLfloat a_1;GLfloat a_2;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
fexthunks_libGL_glNormal3f(&args);
}

MAKE_THUNK(libGL, glRotatef)

void glRotatef(GLfloat a_0,GLfloat a_1,GLfloat a_2,GLfloat a_3){
struct {GLfloat a_0;GLfloat a_1;GLfloat a_2;GLfloat a_3;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;
fexthunks_libGL_glRotatef(&args);
}

MAKE_THUNK(libGL, glShadeModel)

void glShadeModel(GLenum a_0){
struct {GLenum a_0;} args;
args.a_0 = a_0;
fexthunks_libGL_glShadeModel(&args);
}

MAKE_THUNK(libGL, glTranslated)

void glTranslated(GLdouble a_0,GLdouble a_1,GLdouble a_2){
struct {GLdouble a_0;GLdouble a_1;GLdouble a_2;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
fexthunks_libGL_glTranslated(&args);
}

MAKE_THUNK(libGL, glTranslatef)

void glTranslatef(GLfloat a_0,GLfloat a_1,GLfloat a_2){
struct {GLfloat a_0;GLfloat a_1;GLfloat a_2;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
fexthunks_libGL_glTranslatef(&args);
}

MAKE_THUNK(libGL, glVertex3f)

void glVertex3f(GLfloat a_0,GLfloat a_1,GLfloat a_2){
struct {GLfloat a_0;GLfloat a_1;GLfloat a_2;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
fexthunks_libGL_glVertex3f(&args);
}

MAKE_THUNK(libGL, glViewport)

void glViewport(GLint a_0,GLint a_1,GLsizei a_2,GLsizei a_3){
struct {GLint a_0;GLint a_1;GLsizei a_2;GLsizei a_3;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;
fexthunks_libGL_glViewport(&args);
}

MAKE_THUNK(libGL, glEnd)

void glEnd(){
struct {} args;

fexthunks_libGL_glEnd(&args);
}

MAKE_THUNK(libGL, glEndList)

void glEndList(){
struct {} args;

fexthunks_libGL_glEndList(&args);
}

MAKE_THUNK(libGL, glLoadIdentity)

void glLoadIdentity(){
struct {} args;

fexthunks_libGL_glLoadIdentity(&args);
}

MAKE_THUNK(libGL, glPopMatrix)

void glPopMatrix(){
struct {} args;

fexthunks_libGL_glPopMatrix(&args);
}

MAKE_THUNK(libGL, glPushMatrix)

void glPushMatrix(){
struct {} args;

fexthunks_libGL_glPushMatrix(&args);
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

MAKE_THUNK(libX11, XChangeProperty)

int XChangeProperty(Display * a_0,Window a_1,Atom a_2,Atom a_3,int a_4,int a_5,const unsigned char * a_6,int a_7){
struct {Display * a_0;Window a_1;Atom a_2;Atom a_3;int a_4;int a_5;const unsigned char * a_6;int a_7;int rv;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;args.a_4 = a_4;args.a_5 = a_5;args.a_6 = a_6;args.a_7 = a_7;
fexthunks_libX11_XChangeProperty(&args);
return args.rv;
}

MAKE_THUNK(libX11, XCloseDisplay)

int XCloseDisplay(Display * a_0){
struct {Display * a_0;int rv;} args;
args.a_0 = a_0;
fexthunks_libX11_XCloseDisplay(&args);
return args.rv;
}

MAKE_THUNK(libX11, XDestroyWindow)

int XDestroyWindow(Display * a_0,Window a_1){
struct {Display * a_0;Window a_1;int rv;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libX11_XDestroyWindow(&args);
return args.rv;
}

MAKE_THUNK(libX11, XFree)

int XFree(void* a_0){
struct {void* a_0;int rv;} args;
args.a_0 = a_0;
fexthunks_libX11_XFree(&args);
return args.rv;
}

MAKE_THUNK(libX11, XInternAtom)

Atom XInternAtom(Display * a_0,const char * a_1,Bool a_2){
struct {Display * a_0;const char * a_1;Bool a_2;Atom rv;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
fexthunks_libX11_XInternAtom(&args);
return args.rv;
}

MAKE_THUNK(libX11, XLookupKeysym)

KeySym XLookupKeysym(XKeyEvent * a_0,int a_1){
struct {XKeyEvent * a_0;int a_1;KeySym rv;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libX11_XLookupKeysym(&args);
return args.rv;
}

MAKE_THUNK(libX11, XLookupString)

int XLookupString(XKeyEvent * a_0,char * a_1,int a_2,KeySym * a_3,XComposeStatus* a_4){
struct {XKeyEvent * a_0;char * a_1;int a_2;KeySym * a_3;XComposeStatus* a_4;int rv;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;args.a_4 = a_4;
fexthunks_libX11_XLookupString(&args);
return args.rv;
}

MAKE_THUNK(libX11, XNextEvent)

int XNextEvent(Display * a_0,XEvent * a_1){
struct {Display * a_0;XEvent * a_1;int rv;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libX11_XNextEvent(&args);
return args.rv;
}

MAKE_THUNK(libX11, XParseGeometry)

int XParseGeometry(const char * a_0,int * a_1,int * a_2,unsigned int * a_3,unsigned int * a_4){
struct {const char * a_0;int * a_1;int * a_2;unsigned int * a_3;unsigned int * a_4;int rv;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;args.a_4 = a_4;
fexthunks_libX11_XParseGeometry(&args);
return args.rv;
}

MAKE_THUNK(libX11, XPending)

int XPending(Display * a_0){
struct {Display * a_0;int rv;} args;
args.a_0 = a_0;
fexthunks_libX11_XPending(&args);
return args.rv;
}

MAKE_THUNK(libX11, XSetNormalHints)

int XSetNormalHints(Display * a_0,Window a_1,XSizeHints * a_2){
struct {Display * a_0;Window a_1;XSizeHints * a_2;int rv;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
fexthunks_libX11_XSetNormalHints(&args);
return args.rv;
}

MAKE_THUNK(libX11, XSetStandardProperties)

int XSetStandardProperties(Display * a_0,Window a_1,const char * a_2,const char * a_3,Pixmap a_4,char ** a_5,int a_6,XSizeHints * a_7){
struct {Display * a_0;Window a_1;const char * a_2;const char * a_3;Pixmap a_4;char ** a_5;int a_6;XSizeHints * a_7;int rv;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;args.a_4 = a_4;args.a_5 = a_5;args.a_6 = a_6;args.a_7 = a_7;
fexthunks_libX11_XSetStandardProperties(&args);
return args.rv;
}

}
