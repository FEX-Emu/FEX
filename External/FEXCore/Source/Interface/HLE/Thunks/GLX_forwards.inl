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
typedef void fexthunks_type_libGL_glXQueryDrawable(Display* a_0,GLXDrawable a_1,int a_2,unsigned int* a_3);
fexthunks_type_libGL_glXQueryDrawable *fexthunks_impl_libGL_glXQueryDrawable;
typedef int fexthunks_type_libGL_glXGetSwapIntervalMESA(unsigned int a_0);
fexthunks_type_libGL_glXGetSwapIntervalMESA *fexthunks_impl_libGL_glXGetSwapIntervalMESA;
typedef void fexthunks_type_libGL_glClearColor(GLclampf a_0,GLclampf a_1,GLclampf a_2,GLclampf a_3);
fexthunks_type_libGL_glClearColor *fexthunks_impl_libGL_glClearColor;
typedef void fexthunks_type_libGL_glClear(GLbitfield a_0);
fexthunks_type_libGL_glClear *fexthunks_impl_libGL_glClear;
typedef void fexthunks_type_libGL_glBegin(GLenum a_0);
fexthunks_type_libGL_glBegin *fexthunks_impl_libGL_glBegin;
typedef void fexthunks_type_libGL_glCallList(GLuint a_0);
fexthunks_type_libGL_glCallList *fexthunks_impl_libGL_glCallList;
typedef void fexthunks_type_libGL_glDeleteLists(GLuint a_0,GLsizei a_1);
fexthunks_type_libGL_glDeleteLists *fexthunks_impl_libGL_glDeleteLists;
typedef void fexthunks_type_libGL_glDrawBuffer(GLenum a_0);
fexthunks_type_libGL_glDrawBuffer *fexthunks_impl_libGL_glDrawBuffer;
typedef void fexthunks_type_libGL_glEnable(GLenum a_0);
fexthunks_type_libGL_glEnable *fexthunks_impl_libGL_glEnable;
typedef void fexthunks_type_libGL_glFrustum(GLdouble a_0,GLdouble a_1,GLdouble a_2,GLdouble a_3,GLdouble a_4,GLdouble a_5);
fexthunks_type_libGL_glFrustum *fexthunks_impl_libGL_glFrustum;
typedef GLuint fexthunks_type_libGL_glGenLists(GLsizei a_0);
fexthunks_type_libGL_glGenLists *fexthunks_impl_libGL_glGenLists;
typedef const GLubyte * fexthunks_type_libGL_glGetString(GLenum a_0);
fexthunks_type_libGL_glGetString *fexthunks_impl_libGL_glGetString;
typedef void fexthunks_type_libGL_glLightfv(GLenum a_0,GLenum a_1,const GLfloat* a_2);
fexthunks_type_libGL_glLightfv *fexthunks_impl_libGL_glLightfv;
typedef void fexthunks_type_libGL_glMaterialfv(GLenum a_0,GLenum a_1,const GLfloat* a_2);
fexthunks_type_libGL_glMaterialfv *fexthunks_impl_libGL_glMaterialfv;
typedef void fexthunks_type_libGL_glMatrixMode(GLenum a_0);
fexthunks_type_libGL_glMatrixMode *fexthunks_impl_libGL_glMatrixMode;
typedef void fexthunks_type_libGL_glNewList(GLuint a_0,GLenum a_1);
fexthunks_type_libGL_glNewList *fexthunks_impl_libGL_glNewList;
typedef void fexthunks_type_libGL_glNormal3f(GLfloat a_0,GLfloat a_1,GLfloat a_2);
fexthunks_type_libGL_glNormal3f *fexthunks_impl_libGL_glNormal3f;
typedef void fexthunks_type_libGL_glRotatef(GLfloat a_0,GLfloat a_1,GLfloat a_2,GLfloat a_3);
fexthunks_type_libGL_glRotatef *fexthunks_impl_libGL_glRotatef;
typedef void fexthunks_type_libGL_glShadeModel(GLenum a_0);
fexthunks_type_libGL_glShadeModel *fexthunks_impl_libGL_glShadeModel;
typedef void fexthunks_type_libGL_glTranslated(GLdouble a_0,GLdouble a_1,GLdouble a_2);
fexthunks_type_libGL_glTranslated *fexthunks_impl_libGL_glTranslated;
typedef void fexthunks_type_libGL_glTranslatef(GLfloat a_0,GLfloat a_1,GLfloat a_2);
fexthunks_type_libGL_glTranslatef *fexthunks_impl_libGL_glTranslatef;
typedef void fexthunks_type_libGL_glVertex3f(GLfloat a_0,GLfloat a_1,GLfloat a_2);
fexthunks_type_libGL_glVertex3f *fexthunks_impl_libGL_glVertex3f;
typedef void fexthunks_type_libGL_glViewport(GLint a_0,GLint a_1,GLsizei a_2,GLsizei a_3);
fexthunks_type_libGL_glViewport *fexthunks_impl_libGL_glViewport;
typedef void fexthunks_type_libGL_glEnd();
fexthunks_type_libGL_glEnd *fexthunks_impl_libGL_glEnd;
typedef void fexthunks_type_libGL_glEndList();
fexthunks_type_libGL_glEndList *fexthunks_impl_libGL_glEndList;
typedef void fexthunks_type_libGL_glLoadIdentity();
fexthunks_type_libGL_glLoadIdentity *fexthunks_impl_libGL_glLoadIdentity;
typedef void fexthunks_type_libGL_glPopMatrix();
fexthunks_type_libGL_glPopMatrix *fexthunks_impl_libGL_glPopMatrix;
typedef void fexthunks_type_libGL_glPushMatrix();
fexthunks_type_libGL_glPushMatrix *fexthunks_impl_libGL_glPushMatrix;
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
(void*&)fexthunks_impl_libGL_glXQueryDrawable = dlsym(fexthunks_impl_libGL_so, "glXQueryDrawable");
(void*&)fexthunks_impl_libGL_glXGetSwapIntervalMESA = dlsym(fexthunks_impl_libGL_so, "glXGetSwapIntervalMESA");
(void*&)fexthunks_impl_libGL_glClearColor = dlsym(fexthunks_impl_libGL_so, "glClearColor");
(void*&)fexthunks_impl_libGL_glClear = dlsym(fexthunks_impl_libGL_so, "glClear");
(void*&)fexthunks_impl_libGL_glBegin = dlsym(fexthunks_impl_libGL_so, "glBegin");
(void*&)fexthunks_impl_libGL_glCallList = dlsym(fexthunks_impl_libGL_so, "glCallList");
(void*&)fexthunks_impl_libGL_glDeleteLists = dlsym(fexthunks_impl_libGL_so, "glDeleteLists");
(void*&)fexthunks_impl_libGL_glDrawBuffer = dlsym(fexthunks_impl_libGL_so, "glDrawBuffer");
(void*&)fexthunks_impl_libGL_glEnable = dlsym(fexthunks_impl_libGL_so, "glEnable");
(void*&)fexthunks_impl_libGL_glFrustum = dlsym(fexthunks_impl_libGL_so, "glFrustum");
(void*&)fexthunks_impl_libGL_glGenLists = dlsym(fexthunks_impl_libGL_so, "glGenLists");
(void*&)fexthunks_impl_libGL_glGetString = dlsym(fexthunks_impl_libGL_so, "glGetString");
(void*&)fexthunks_impl_libGL_glLightfv = dlsym(fexthunks_impl_libGL_so, "glLightfv");
(void*&)fexthunks_impl_libGL_glMaterialfv = dlsym(fexthunks_impl_libGL_so, "glMaterialfv");
(void*&)fexthunks_impl_libGL_glMatrixMode = dlsym(fexthunks_impl_libGL_so, "glMatrixMode");
(void*&)fexthunks_impl_libGL_glNewList = dlsym(fexthunks_impl_libGL_so, "glNewList");
(void*&)fexthunks_impl_libGL_glNormal3f = dlsym(fexthunks_impl_libGL_so, "glNormal3f");
(void*&)fexthunks_impl_libGL_glRotatef = dlsym(fexthunks_impl_libGL_so, "glRotatef");
(void*&)fexthunks_impl_libGL_glShadeModel = dlsym(fexthunks_impl_libGL_so, "glShadeModel");
(void*&)fexthunks_impl_libGL_glTranslated = dlsym(fexthunks_impl_libGL_so, "glTranslated");
(void*&)fexthunks_impl_libGL_glTranslatef = dlsym(fexthunks_impl_libGL_so, "glTranslatef");
(void*&)fexthunks_impl_libGL_glVertex3f = dlsym(fexthunks_impl_libGL_so, "glVertex3f");
(void*&)fexthunks_impl_libGL_glViewport = dlsym(fexthunks_impl_libGL_so, "glViewport");
(void*&)fexthunks_impl_libGL_glEnd = dlsym(fexthunks_impl_libGL_so, "glEnd");
(void*&)fexthunks_impl_libGL_glEndList = dlsym(fexthunks_impl_libGL_so, "glEndList");
(void*&)fexthunks_impl_libGL_glLoadIdentity = dlsym(fexthunks_impl_libGL_so, "glLoadIdentity");
(void*&)fexthunks_impl_libGL_glPopMatrix = dlsym(fexthunks_impl_libGL_so, "glPopMatrix");
(void*&)fexthunks_impl_libGL_glPushMatrix = dlsym(fexthunks_impl_libGL_so, "glPushMatrix");
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
typedef int fexthunks_type_libX11_XChangeProperty(Display * a_0,Window a_1,Atom a_2,Atom a_3,int a_4,int a_5,const unsigned char * a_6,int a_7);
fexthunks_type_libX11_XChangeProperty *fexthunks_impl_libX11_XChangeProperty;
typedef int fexthunks_type_libX11_XCloseDisplay(Display * a_0);
fexthunks_type_libX11_XCloseDisplay *fexthunks_impl_libX11_XCloseDisplay;
typedef int fexthunks_type_libX11_XDestroyWindow(Display * a_0,Window a_1);
fexthunks_type_libX11_XDestroyWindow *fexthunks_impl_libX11_XDestroyWindow;
typedef int fexthunks_type_libX11_XFree(void* a_0);
fexthunks_type_libX11_XFree *fexthunks_impl_libX11_XFree;
typedef Atom fexthunks_type_libX11_XInternAtom(Display * a_0,const char * a_1,Bool a_2);
fexthunks_type_libX11_XInternAtom *fexthunks_impl_libX11_XInternAtom;
typedef KeySym fexthunks_type_libX11_XLookupKeysym(XKeyEvent * a_0,int a_1);
fexthunks_type_libX11_XLookupKeysym *fexthunks_impl_libX11_XLookupKeysym;
typedef int fexthunks_type_libX11_XLookupString(XKeyEvent * a_0,char * a_1,int a_2,KeySym * a_3,XComposeStatus* a_4);
fexthunks_type_libX11_XLookupString *fexthunks_impl_libX11_XLookupString;
typedef int fexthunks_type_libX11_XNextEvent(Display * a_0,XEvent * a_1);
fexthunks_type_libX11_XNextEvent *fexthunks_impl_libX11_XNextEvent;
typedef int fexthunks_type_libX11_XParseGeometry(const char * a_0,int * a_1,int * a_2,unsigned int * a_3,unsigned int * a_4);
fexthunks_type_libX11_XParseGeometry *fexthunks_impl_libX11_XParseGeometry;
typedef int fexthunks_type_libX11_XPending(Display * a_0);
fexthunks_type_libX11_XPending *fexthunks_impl_libX11_XPending;
typedef int fexthunks_type_libX11_XSetNormalHints(Display * a_0,Window a_1,XSizeHints * a_2);
fexthunks_type_libX11_XSetNormalHints *fexthunks_impl_libX11_XSetNormalHints;
typedef int fexthunks_type_libX11_XSetStandardProperties(Display * a_0,Window a_1,const char * a_2,const char * a_3,Pixmap a_4,char ** a_5,int a_6,XSizeHints * a_7);
fexthunks_type_libX11_XSetStandardProperties *fexthunks_impl_libX11_XSetStandardProperties;
bool fexthunks_init_libX11() {
fexthunks_impl_libX11_so = dlopen("libX11.so", RTLD_LOCAL | RTLD_LAZY);
if (!fexthunks_impl_libX11_so) { return false; }
(void*&)fexthunks_impl_libX11_XOpenDisplay = dlsym(fexthunks_impl_libX11_so, "XOpenDisplay");
(void*&)fexthunks_impl_libX11_XCreateColormap = dlsym(fexthunks_impl_libX11_so, "XCreateColormap");
(void*&)fexthunks_impl_libX11_XCreateWindow = dlsym(fexthunks_impl_libX11_so, "XCreateWindow");
(void*&)fexthunks_impl_libX11_XMapWindow = dlsym(fexthunks_impl_libX11_so, "XMapWindow");
(void*&)fexthunks_impl_libX11_XChangeProperty = dlsym(fexthunks_impl_libX11_so, "XChangeProperty");
(void*&)fexthunks_impl_libX11_XCloseDisplay = dlsym(fexthunks_impl_libX11_so, "XCloseDisplay");
(void*&)fexthunks_impl_libX11_XDestroyWindow = dlsym(fexthunks_impl_libX11_so, "XDestroyWindow");
(void*&)fexthunks_impl_libX11_XFree = dlsym(fexthunks_impl_libX11_so, "XFree");
(void*&)fexthunks_impl_libX11_XInternAtom = dlsym(fexthunks_impl_libX11_so, "XInternAtom");
(void*&)fexthunks_impl_libX11_XLookupKeysym = dlsym(fexthunks_impl_libX11_so, "XLookupKeysym");
(void*&)fexthunks_impl_libX11_XLookupString = dlsym(fexthunks_impl_libX11_so, "XLookupString");
(void*&)fexthunks_impl_libX11_XNextEvent = dlsym(fexthunks_impl_libX11_so, "XNextEvent");
(void*&)fexthunks_impl_libX11_XParseGeometry = dlsym(fexthunks_impl_libX11_so, "XParseGeometry");
(void*&)fexthunks_impl_libX11_XPending = dlsym(fexthunks_impl_libX11_so, "XPending");
(void*&)fexthunks_impl_libX11_XSetNormalHints = dlsym(fexthunks_impl_libX11_so, "XSetNormalHints");
(void*&)fexthunks_impl_libX11_XSetStandardProperties = dlsym(fexthunks_impl_libX11_so, "XSetStandardProperties");
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
void fexthunks_forward_libGL_glXQueryDrawable(void *argsv){
struct arg_t {Display* a_0;GLXDrawable a_1;int a_2;unsigned int* a_3;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glXQueryDrawable
(args->a_0,args->a_1,args->a_2,args->a_3);
}
void fexthunks_forward_libGL_glXGetSwapIntervalMESA(void *argsv){
struct arg_t {unsigned int a_0;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libGL_glXGetSwapIntervalMESA
(args->a_0);
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
void fexthunks_forward_libGL_glBegin(void *argsv){
struct arg_t {GLenum a_0;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glBegin
(args->a_0);
}
void fexthunks_forward_libGL_glCallList(void *argsv){
struct arg_t {GLuint a_0;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glCallList
(args->a_0);
}
void fexthunks_forward_libGL_glDeleteLists(void *argsv){
struct arg_t {GLuint a_0;GLsizei a_1;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glDeleteLists
(args->a_0,args->a_1);
}
void fexthunks_forward_libGL_glDrawBuffer(void *argsv){
struct arg_t {GLenum a_0;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glDrawBuffer
(args->a_0);
}
void fexthunks_forward_libGL_glEnable(void *argsv){
struct arg_t {GLenum a_0;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glEnable
(args->a_0);
}
void fexthunks_forward_libGL_glFrustum(void *argsv){
struct arg_t {GLdouble a_0;GLdouble a_1;GLdouble a_2;GLdouble a_3;GLdouble a_4;GLdouble a_5;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glFrustum
(args->a_0,args->a_1,args->a_2,args->a_3,args->a_4,args->a_5);
}
void fexthunks_forward_libGL_glGenLists(void *argsv){
struct arg_t {GLsizei a_0;GLuint rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libGL_glGenLists
(args->a_0);
}
void fexthunks_forward_libGL_glGetString(void *argsv){
struct arg_t {GLenum a_0;const GLubyte * rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libGL_glGetString
(args->a_0);
}
void fexthunks_forward_libGL_glLightfv(void *argsv){
struct arg_t {GLenum a_0;GLenum a_1;const GLfloat* a_2;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glLightfv
(args->a_0,args->a_1,args->a_2);
}
void fexthunks_forward_libGL_glMaterialfv(void *argsv){
struct arg_t {GLenum a_0;GLenum a_1;const GLfloat* a_2;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glMaterialfv
(args->a_0,args->a_1,args->a_2);
}
void fexthunks_forward_libGL_glMatrixMode(void *argsv){
struct arg_t {GLenum a_0;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glMatrixMode
(args->a_0);
}
void fexthunks_forward_libGL_glNewList(void *argsv){
struct arg_t {GLuint a_0;GLenum a_1;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glNewList
(args->a_0,args->a_1);
}
void fexthunks_forward_libGL_glNormal3f(void *argsv){
struct arg_t {GLfloat a_0;GLfloat a_1;GLfloat a_2;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glNormal3f
(args->a_0,args->a_1,args->a_2);
}
void fexthunks_forward_libGL_glRotatef(void *argsv){
struct arg_t {GLfloat a_0;GLfloat a_1;GLfloat a_2;GLfloat a_3;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glRotatef
(args->a_0,args->a_1,args->a_2,args->a_3);
}
void fexthunks_forward_libGL_glShadeModel(void *argsv){
struct arg_t {GLenum a_0;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glShadeModel
(args->a_0);
}
void fexthunks_forward_libGL_glTranslated(void *argsv){
struct arg_t {GLdouble a_0;GLdouble a_1;GLdouble a_2;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glTranslated
(args->a_0,args->a_1,args->a_2);
}
void fexthunks_forward_libGL_glTranslatef(void *argsv){
struct arg_t {GLfloat a_0;GLfloat a_1;GLfloat a_2;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glTranslatef
(args->a_0,args->a_1,args->a_2);
}
void fexthunks_forward_libGL_glVertex3f(void *argsv){
struct arg_t {GLfloat a_0;GLfloat a_1;GLfloat a_2;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glVertex3f
(args->a_0,args->a_1,args->a_2);
}
void fexthunks_forward_libGL_glViewport(void *argsv){
struct arg_t {GLint a_0;GLint a_1;GLsizei a_2;GLsizei a_3;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glViewport
(args->a_0,args->a_1,args->a_2,args->a_3);
}
void fexthunks_forward_libGL_glEnd(void *argsv){
struct arg_t {};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glEnd
();
}
void fexthunks_forward_libGL_glEndList(void *argsv){
struct arg_t {};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glEndList
();
}
void fexthunks_forward_libGL_glLoadIdentity(void *argsv){
struct arg_t {};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glLoadIdentity
();
}
void fexthunks_forward_libGL_glPopMatrix(void *argsv){
struct arg_t {};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glPopMatrix
();
}
void fexthunks_forward_libGL_glPushMatrix(void *argsv){
struct arg_t {};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glPushMatrix
();
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
void fexthunks_forward_libX11_XChangeProperty(void *argsv){
struct arg_t {Display * a_0;Window a_1;Atom a_2;Atom a_3;int a_4;int a_5;const unsigned char * a_6;int a_7;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XChangeProperty
(args->a_0,args->a_1,args->a_2,args->a_3,args->a_4,args->a_5,args->a_6,args->a_7);
}
void fexthunks_forward_libX11_XCloseDisplay(void *argsv){
struct arg_t {Display * a_0;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XCloseDisplay
(args->a_0);
}
void fexthunks_forward_libX11_XDestroyWindow(void *argsv){
struct arg_t {Display * a_0;Window a_1;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XDestroyWindow
(args->a_0,args->a_1);
}
void fexthunks_forward_libX11_XFree(void *argsv){
struct arg_t {void* a_0;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XFree
(args->a_0);
}
void fexthunks_forward_libX11_XInternAtom(void *argsv){
struct arg_t {Display * a_0;const char * a_1;Bool a_2;Atom rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XInternAtom
(args->a_0,args->a_1,args->a_2);
}
void fexthunks_forward_libX11_XLookupKeysym(void *argsv){
struct arg_t {XKeyEvent * a_0;int a_1;KeySym rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XLookupKeysym
(args->a_0,args->a_1);
}
void fexthunks_forward_libX11_XLookupString(void *argsv){
struct arg_t {XKeyEvent * a_0;char * a_1;int a_2;KeySym * a_3;XComposeStatus* a_4;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XLookupString
(args->a_0,args->a_1,args->a_2,args->a_3,args->a_4);
}
void fexthunks_forward_libX11_XNextEvent(void *argsv){
struct arg_t {Display * a_0;XEvent * a_1;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XNextEvent
(args->a_0,args->a_1);
}
void fexthunks_forward_libX11_XParseGeometry(void *argsv){
struct arg_t {const char * a_0;int * a_1;int * a_2;unsigned int * a_3;unsigned int * a_4;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XParseGeometry
(args->a_0,args->a_1,args->a_2,args->a_3,args->a_4);
}
void fexthunks_forward_libX11_XPending(void *argsv){
struct arg_t {Display * a_0;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XPending
(args->a_0);
}
void fexthunks_forward_libX11_XSetNormalHints(void *argsv){
struct arg_t {Display * a_0;Window a_1;XSizeHints * a_2;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XSetNormalHints
(args->a_0,args->a_1,args->a_2);
}
void fexthunks_forward_libX11_XSetStandardProperties(void *argsv){
struct arg_t {Display * a_0;Window a_1;const char * a_2;const char * a_3;Pixmap a_4;char ** a_5;int a_6;XSizeHints * a_7;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XSetStandardProperties
(args->a_0,args->a_1,args->a_2,args->a_3,args->a_4,args->a_5,args->a_6,args->a_7);
}
