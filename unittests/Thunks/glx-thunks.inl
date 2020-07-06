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

void glXDestroyContext(Display* a_0,GLXContext a_1){
struct {Display* a_0;GLXContext a_1;} args;
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

void glXSwapBuffers(Display* a_0,GLXDrawable a_1){
struct {Display* a_0;GLXDrawable a_1;} args;
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

int glXQueryExtension(Display* a_0,int* a_1,int* a_2){
struct {Display* a_0;int* a_1;int* a_2;int rv;} args;
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

__GLXcontextRec* glXGetCurrentContext(){
struct {__GLXcontextRec* rv;} args;

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

const char* glXQueryExtensionsString(Display* a_0,int a_1){
struct {Display* a_0;int a_1;const char* rv;} args;
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

__GLXFBConfigRec** glXChooseFBConfig(Display* a_0,int a_1,const int* a_2,int* a_3){
struct {Display* a_0;int a_1;const int* a_2;int* a_3;__GLXFBConfigRec** rv;} args;
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

const GLubyte* glGetString(GLenum a_0){
struct {GLenum a_0;const GLubyte* rv;} args;
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

MAKE_THUNK(libGL, glIsEnabled)

GLboolean glIsEnabled(GLenum a_0){
struct {GLenum a_0;GLboolean rv;} args;
args.a_0 = a_0;
fexthunks_libGL_glIsEnabled(&args);
return args.rv;
}

MAKE_THUNK(libGL, glGetError)

GLenum glGetError(){
struct {GLenum rv;} args;

fexthunks_libGL_glGetError(&args);
return args.rv;
}

MAKE_THUNK(libGL, glXCreateNewContext)

__GLXcontextRec* glXCreateNewContext(Display* a_0,GLXFBConfig a_1,int a_2,GLXContext a_3,int a_4){
struct {Display* a_0;GLXFBConfig a_1;int a_2;GLXContext a_3;int a_4;__GLXcontextRec* rv;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;args.a_4 = a_4;
fexthunks_libGL_glXCreateNewContext(&args);
return args.rv;
}

MAKE_THUNK(libGL, glXCreateWindow)

GLXWindow glXCreateWindow(Display* a_0,GLXFBConfig a_1,Window a_2,const int* a_3){
struct {Display* a_0;GLXFBConfig a_1;Window a_2;const int* a_3;GLXWindow rv;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;
fexthunks_libGL_glXCreateWindow(&args);
return args.rv;
}

MAKE_THUNK(libGL, glXMakeContextCurrent)

int glXMakeContextCurrent(Display* a_0,GLXDrawable a_1,GLXDrawable a_2,GLXContext a_3){
struct {Display* a_0;GLXDrawable a_1;GLXDrawable a_2;GLXContext a_3;int rv;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;
fexthunks_libGL_glXMakeContextCurrent(&args);
return args.rv;
}

MAKE_THUNK(libGL, glActiveTexture)

void glActiveTexture(GLenum a_0){
struct {GLenum a_0;} args;
args.a_0 = a_0;
fexthunks_libGL_glActiveTexture(&args);
}

MAKE_THUNK(libGL, glBindTexture)

void glBindTexture(GLenum a_0,GLuint a_1){
struct {GLenum a_0;GLuint a_1;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libGL_glBindTexture(&args);
}

MAKE_THUNK(libGL, glBlendColor)

void glBlendColor(GLclampf a_0,GLclampf a_1,GLclampf a_2,GLclampf a_3){
struct {GLclampf a_0;GLclampf a_1;GLclampf a_2;GLclampf a_3;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;
fexthunks_libGL_glBlendColor(&args);
}

MAKE_THUNK(libGL, glBlendEquation)

void glBlendEquation(GLenum a_0){
struct {GLenum a_0;} args;
args.a_0 = a_0;
fexthunks_libGL_glBlendEquation(&args);
}

MAKE_THUNK(libGL, glBlendFunc)

void glBlendFunc(GLenum a_0,GLenum a_1){
struct {GLenum a_0;GLenum a_1;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libGL_glBlendFunc(&args);
}

MAKE_THUNK(libGL, glClearDepth)

void glClearDepth(GLclampd a_0){
struct {GLclampd a_0;} args;
args.a_0 = a_0;
fexthunks_libGL_glClearDepth(&args);
}

MAKE_THUNK(libGL, glClearStencil)

void glClearStencil(GLint a_0){
struct {GLint a_0;} args;
args.a_0 = a_0;
fexthunks_libGL_glClearStencil(&args);
}

MAKE_THUNK(libGL, glColorMask)

void glColorMask(GLboolean a_0,GLboolean a_1,GLboolean a_2,GLboolean a_3){
struct {GLboolean a_0;GLboolean a_1;GLboolean a_2;GLboolean a_3;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;
fexthunks_libGL_glColorMask(&args);
}

MAKE_THUNK(libGL, glCompressedTexImage2D)

void glCompressedTexImage2D(GLenum a_0,GLint a_1,GLenum a_2,GLsizei a_3,GLsizei a_4,GLint a_5,GLsizei a_6,const GLvoid* a_7){
struct {GLenum a_0;GLint a_1;GLenum a_2;GLsizei a_3;GLsizei a_4;GLint a_5;GLsizei a_6;const GLvoid* a_7;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;args.a_4 = a_4;args.a_5 = a_5;args.a_6 = a_6;args.a_7 = a_7;
fexthunks_libGL_glCompressedTexImage2D(&args);
}

MAKE_THUNK(libGL, glCompressedTexSubImage2D)

void glCompressedTexSubImage2D(GLenum a_0,GLint a_1,GLint a_2,GLint a_3,GLsizei a_4,GLsizei a_5,GLenum a_6,GLsizei a_7,const GLvoid* a_8){
struct {GLenum a_0;GLint a_1;GLint a_2;GLint a_3;GLsizei a_4;GLsizei a_5;GLenum a_6;GLsizei a_7;const GLvoid* a_8;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;args.a_4 = a_4;args.a_5 = a_5;args.a_6 = a_6;args.a_7 = a_7;args.a_8 = a_8;
fexthunks_libGL_glCompressedTexSubImage2D(&args);
}

MAKE_THUNK(libGL, glCopyTexImage2D)

void glCopyTexImage2D(GLenum a_0,GLint a_1,GLenum a_2,GLint a_3,GLint a_4,GLsizei a_5,GLsizei a_6,GLint a_7){
struct {GLenum a_0;GLint a_1;GLenum a_2;GLint a_3;GLint a_4;GLsizei a_5;GLsizei a_6;GLint a_7;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;args.a_4 = a_4;args.a_5 = a_5;args.a_6 = a_6;args.a_7 = a_7;
fexthunks_libGL_glCopyTexImage2D(&args);
}

MAKE_THUNK(libGL, glCopyTexSubImage2D)

void glCopyTexSubImage2D(GLenum a_0,GLint a_1,GLint a_2,GLint a_3,GLint a_4,GLint a_5,GLsizei a_6,GLsizei a_7){
struct {GLenum a_0;GLint a_1;GLint a_2;GLint a_3;GLint a_4;GLint a_5;GLsizei a_6;GLsizei a_7;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;args.a_4 = a_4;args.a_5 = a_5;args.a_6 = a_6;args.a_7 = a_7;
fexthunks_libGL_glCopyTexSubImage2D(&args);
}

MAKE_THUNK(libGL, glCullFace)

void glCullFace(GLenum a_0){
struct {GLenum a_0;} args;
args.a_0 = a_0;
fexthunks_libGL_glCullFace(&args);
}

MAKE_THUNK(libGL, glDeleteTextures)

void glDeleteTextures(GLsizei a_0,const GLuint* a_1){
struct {GLsizei a_0;const GLuint* a_1;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libGL_glDeleteTextures(&args);
}

MAKE_THUNK(libGL, glDepthFunc)

void glDepthFunc(GLenum a_0){
struct {GLenum a_0;} args;
args.a_0 = a_0;
fexthunks_libGL_glDepthFunc(&args);
}

MAKE_THUNK(libGL, glDepthMask)

void glDepthMask(GLboolean a_0){
struct {GLboolean a_0;} args;
args.a_0 = a_0;
fexthunks_libGL_glDepthMask(&args);
}

MAKE_THUNK(libGL, glDepthRange)

void glDepthRange(GLclampd a_0,GLclampd a_1){
struct {GLclampd a_0;GLclampd a_1;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libGL_glDepthRange(&args);
}

MAKE_THUNK(libGL, glDisable)

void glDisable(GLenum a_0){
struct {GLenum a_0;} args;
args.a_0 = a_0;
fexthunks_libGL_glDisable(&args);
}

MAKE_THUNK(libGL, glDrawArrays)

void glDrawArrays(GLenum a_0,GLint a_1,GLsizei a_2){
struct {GLenum a_0;GLint a_1;GLsizei a_2;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
fexthunks_libGL_glDrawArrays(&args);
}

MAKE_THUNK(libGL, glDrawElements)

void glDrawElements(GLenum a_0,GLsizei a_1,GLenum a_2,const GLvoid* a_3){
struct {GLenum a_0;GLsizei a_1;GLenum a_2;const GLvoid* a_3;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;
fexthunks_libGL_glDrawElements(&args);
}

MAKE_THUNK(libGL, glFrontFace)

void glFrontFace(GLenum a_0){
struct {GLenum a_0;} args;
args.a_0 = a_0;
fexthunks_libGL_glFrontFace(&args);
}

MAKE_THUNK(libGL, glGenTextures)

void glGenTextures(GLsizei a_0,GLuint* a_1){
struct {GLsizei a_0;GLuint* a_1;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libGL_glGenTextures(&args);
}

MAKE_THUNK(libGL, glGetFloatv)

void glGetFloatv(GLenum a_0,GLfloat* a_1){
struct {GLenum a_0;GLfloat* a_1;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libGL_glGetFloatv(&args);
}

MAKE_THUNK(libGL, glGetIntegerv)

void glGetIntegerv(GLenum a_0,GLint* a_1){
struct {GLenum a_0;GLint* a_1;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libGL_glGetIntegerv(&args);
}

MAKE_THUNK(libGL, glGetTexImage)

void glGetTexImage(GLenum a_0,GLint a_1,GLenum a_2,GLenum a_3,GLvoid* a_4){
struct {GLenum a_0;GLint a_1;GLenum a_2;GLenum a_3;GLvoid* a_4;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;args.a_4 = a_4;
fexthunks_libGL_glGetTexImage(&args);
}

MAKE_THUNK(libGL, glGetTexLevelParameterfv)

void glGetTexLevelParameterfv(GLenum a_0,GLint a_1,GLenum a_2,GLfloat* a_3){
struct {GLenum a_0;GLint a_1;GLenum a_2;GLfloat* a_3;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;
fexthunks_libGL_glGetTexLevelParameterfv(&args);
}

MAKE_THUNK(libGL, glPixelStorei)

void glPixelStorei(GLenum a_0,GLint a_1){
struct {GLenum a_0;GLint a_1;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libGL_glPixelStorei(&args);
}

MAKE_THUNK(libGL, glRasterPos2i)

void glRasterPos2i(GLint a_0,GLint a_1){
struct {GLint a_0;GLint a_1;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libGL_glRasterPos2i(&args);
}

MAKE_THUNK(libGL, glReadPixels)

void glReadPixels(GLint a_0,GLint a_1,GLsizei a_2,GLsizei a_3,GLenum a_4,GLenum a_5,GLvoid* a_6){
struct {GLint a_0;GLint a_1;GLsizei a_2;GLsizei a_3;GLenum a_4;GLenum a_5;GLvoid* a_6;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;args.a_4 = a_4;args.a_5 = a_5;args.a_6 = a_6;
fexthunks_libGL_glReadPixels(&args);
}

MAKE_THUNK(libGL, glScissor)

void glScissor(GLint a_0,GLint a_1,GLsizei a_2,GLsizei a_3){
struct {GLint a_0;GLint a_1;GLsizei a_2;GLsizei a_3;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;
fexthunks_libGL_glScissor(&args);
}

MAKE_THUNK(libGL, glStencilFunc)

void glStencilFunc(GLenum a_0,GLint a_1,GLuint a_2){
struct {GLenum a_0;GLint a_1;GLuint a_2;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
fexthunks_libGL_glStencilFunc(&args);
}

MAKE_THUNK(libGL, glStencilOp)

void glStencilOp(GLenum a_0,GLenum a_1,GLenum a_2){
struct {GLenum a_0;GLenum a_1;GLenum a_2;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
fexthunks_libGL_glStencilOp(&args);
}

MAKE_THUNK(libGL, glTexImage2D)

void glTexImage2D(GLenum a_0,GLint a_1,GLint a_2,GLsizei a_3,GLsizei a_4,GLint a_5,GLenum a_6,GLenum a_7,const GLvoid* a_8){
struct {GLenum a_0;GLint a_1;GLint a_2;GLsizei a_3;GLsizei a_4;GLint a_5;GLenum a_6;GLenum a_7;const GLvoid* a_8;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;args.a_4 = a_4;args.a_5 = a_5;args.a_6 = a_6;args.a_7 = a_7;args.a_8 = a_8;
fexthunks_libGL_glTexImage2D(&args);
}

MAKE_THUNK(libGL, glTexParameteri)

void glTexParameteri(GLenum a_0,GLenum a_1,GLint a_2){
struct {GLenum a_0;GLenum a_1;GLint a_2;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
fexthunks_libGL_glTexParameteri(&args);
}

MAKE_THUNK(libGL, glTexSubImage2D)

void glTexSubImage2D(GLenum a_0,GLint a_1,GLint a_2,GLint a_3,GLsizei a_4,GLsizei a_5,GLenum a_6,GLenum a_7,const GLvoid* a_8){
struct {GLenum a_0;GLint a_1;GLint a_2;GLint a_3;GLsizei a_4;GLsizei a_5;GLenum a_6;GLenum a_7;const GLvoid* a_8;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;args.a_4 = a_4;args.a_5 = a_5;args.a_6 = a_6;args.a_7 = a_7;args.a_8 = a_8;
fexthunks_libGL_glTexSubImage2D(&args);
}

MAKE_THUNK(libGL, glXDestroyWindow)

void glXDestroyWindow(Display* a_0,GLXWindow a_1){
struct {Display* a_0;GLXWindow a_1;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libGL_glXDestroyWindow(&args);
}

MAKE_THUNK(libGL, glXGetVisualFromFBConfig)

XVisualInfo* glXGetVisualFromFBConfig(Display* a_0,GLXFBConfig a_1){
struct {Display* a_0;GLXFBConfig a_1;XVisualInfo* rv;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libGL_glXGetVisualFromFBConfig(&args);
return args.rv;
}

MAKE_THUNK(libGL, glFinish)

void glFinish(){
struct {} args;

fexthunks_libGL_glFinish(&args);
}

MAKE_THUNK(libGL, glFlush)

void glFlush(){
struct {} args;

fexthunks_libGL_glFlush(&args);
}

MAKE_THUNK(libGL, glGetStringi)

const GLubyte* glGetStringi(GLenum a_0,GLuint a_1){
struct {GLenum a_0;GLuint a_1;const GLubyte* rv;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libGL_glGetStringi(&args);
return args.rv;
}

MAKE_THUNK(libGL, glIsProgram)

GLboolean glIsProgram(GLuint a_0){
struct {GLuint a_0;GLboolean rv;} args;
args.a_0 = a_0;
fexthunks_libGL_glIsProgram(&args);
return args.rv;
}

MAKE_THUNK(libGL, glIsShader)

GLboolean glIsShader(GLuint a_0){
struct {GLuint a_0;GLboolean rv;} args;
args.a_0 = a_0;
fexthunks_libGL_glIsShader(&args);
return args.rv;
}

MAKE_THUNK(libGL, glCheckFramebufferStatus)

GLenum glCheckFramebufferStatus(GLenum a_0){
struct {GLenum a_0;GLenum rv;} args;
args.a_0 = a_0;
fexthunks_libGL_glCheckFramebufferStatus(&args);
return args.rv;
}

MAKE_THUNK(libGL, glCheckNamedFramebufferStatus)

GLenum glCheckNamedFramebufferStatus(GLuint a_0,GLenum a_1){
struct {GLuint a_0;GLenum a_1;GLenum rv;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libGL_glCheckNamedFramebufferStatus(&args);
return args.rv;
}

MAKE_THUNK(libGL, glGetUniformLocation)

GLint glGetUniformLocation(GLuint a_0,const GLchar* a_1){
struct {GLuint a_0;const GLchar* a_1;GLint rv;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libGL_glGetUniformLocation(&args);
return args.rv;
}

MAKE_THUNK(libGL, glCreateProgram)

GLuint glCreateProgram(){
struct {GLuint rv;} args;

fexthunks_libGL_glCreateProgram(&args);
return args.rv;
}

MAKE_THUNK(libGL, glCreateShader)

GLuint glCreateShader(GLenum a_0){
struct {GLenum a_0;GLuint rv;} args;
args.a_0 = a_0;
fexthunks_libGL_glCreateShader(&args);
return args.rv;
}

MAKE_THUNK(libGL, glAttachShader)

void glAttachShader(GLuint a_0,GLuint a_1){
struct {GLuint a_0;GLuint a_1;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libGL_glAttachShader(&args);
}

MAKE_THUNK(libGL, glBindAttribLocation)

void glBindAttribLocation(GLuint a_0,GLuint a_1,const GLchar* a_2){
struct {GLuint a_0;GLuint a_1;const GLchar* a_2;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
fexthunks_libGL_glBindAttribLocation(&args);
}

MAKE_THUNK(libGL, glBindBuffer)

void glBindBuffer(GLenum a_0,GLuint a_1){
struct {GLenum a_0;GLuint a_1;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libGL_glBindBuffer(&args);
}

MAKE_THUNK(libGL, glBindFragDataLocation)

void glBindFragDataLocation(GLuint a_0,GLuint a_1,const GLchar* a_2){
struct {GLuint a_0;GLuint a_1;const GLchar* a_2;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
fexthunks_libGL_glBindFragDataLocation(&args);
}

MAKE_THUNK(libGL, glBindFramebuffer)

void glBindFramebuffer(GLenum a_0,GLuint a_1){
struct {GLenum a_0;GLuint a_1;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libGL_glBindFramebuffer(&args);
}

MAKE_THUNK(libGL, glBindProgramPipeline)

void glBindProgramPipeline(GLuint a_0){
struct {GLuint a_0;} args;
args.a_0 = a_0;
fexthunks_libGL_glBindProgramPipeline(&args);
}

MAKE_THUNK(libGL, glBindRenderbuffer)

void glBindRenderbuffer(GLenum a_0,GLuint a_1){
struct {GLenum a_0;GLuint a_1;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libGL_glBindRenderbuffer(&args);
}

MAKE_THUNK(libGL, glBindTextureUnit)

void glBindTextureUnit(GLuint a_0,GLuint a_1){
struct {GLuint a_0;GLuint a_1;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libGL_glBindTextureUnit(&args);
}

MAKE_THUNK(libGL, glBindVertexArray)

void glBindVertexArray(GLuint a_0){
struct {GLuint a_0;} args;
args.a_0 = a_0;
fexthunks_libGL_glBindVertexArray(&args);
}

MAKE_THUNK(libGL, glBlendFuncSeparate)

void glBlendFuncSeparate(GLenum a_0,GLenum a_1,GLenum a_2,GLenum a_3){
struct {GLenum a_0;GLenum a_1;GLenum a_2;GLenum a_3;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;
fexthunks_libGL_glBlendFuncSeparate(&args);
}

MAKE_THUNK(libGL, glBufferData)

void glBufferData(GLenum a_0,GLsizeiptr a_1,const void* a_2,GLenum a_3){
struct {GLenum a_0;GLsizeiptr a_1;const void* a_2;GLenum a_3;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;
fexthunks_libGL_glBufferData(&args);
}

MAKE_THUNK(libGL, glBufferSubData)

void glBufferSubData(GLenum a_0,GLintptr a_1,GLsizeiptr a_2,const void* a_3){
struct {GLenum a_0;GLintptr a_1;GLsizeiptr a_2;const void* a_3;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;
fexthunks_libGL_glBufferSubData(&args);
}

MAKE_THUNK(libGL, glCompileShader)

void glCompileShader(GLuint a_0){
struct {GLuint a_0;} args;
args.a_0 = a_0;
fexthunks_libGL_glCompileShader(&args);
}

MAKE_THUNK(libGL, glCompressedTextureSubImage2D)

void glCompressedTextureSubImage2D(GLuint a_0,GLint a_1,GLint a_2,GLint a_3,GLsizei a_4,GLsizei a_5,GLenum a_6,GLsizei a_7,const void* a_8){
struct {GLuint a_0;GLint a_1;GLint a_2;GLint a_3;GLsizei a_4;GLsizei a_5;GLenum a_6;GLsizei a_7;const void* a_8;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;args.a_4 = a_4;args.a_5 = a_5;args.a_6 = a_6;args.a_7 = a_7;args.a_8 = a_8;
fexthunks_libGL_glCompressedTextureSubImage2D(&args);
}

MAKE_THUNK(libGL, glCopyTextureSubImage2D)

void glCopyTextureSubImage2D(GLuint a_0,GLint a_1,GLint a_2,GLint a_3,GLint a_4,GLint a_5,GLsizei a_6,GLsizei a_7){
struct {GLuint a_0;GLint a_1;GLint a_2;GLint a_3;GLint a_4;GLint a_5;GLsizei a_6;GLsizei a_7;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;args.a_4 = a_4;args.a_5 = a_5;args.a_6 = a_6;args.a_7 = a_7;
fexthunks_libGL_glCopyTextureSubImage2D(&args);
}

MAKE_THUNK(libGL, glCreateFramebuffers)

void glCreateFramebuffers(GLsizei a_0,GLuint* a_1){
struct {GLsizei a_0;GLuint* a_1;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libGL_glCreateFramebuffers(&args);
}

MAKE_THUNK(libGL, glCreateProgramPipelines)

void glCreateProgramPipelines(GLsizei a_0,GLuint* a_1){
struct {GLsizei a_0;GLuint* a_1;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libGL_glCreateProgramPipelines(&args);
}

MAKE_THUNK(libGL, glCreateRenderbuffers)

void glCreateRenderbuffers(GLsizei a_0,GLuint* a_1){
struct {GLsizei a_0;GLuint* a_1;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libGL_glCreateRenderbuffers(&args);
}

MAKE_THUNK(libGL, glCreateTextures)

void glCreateTextures(GLenum a_0,GLsizei a_1,GLuint* a_2){
struct {GLenum a_0;GLsizei a_1;GLuint* a_2;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
fexthunks_libGL_glCreateTextures(&args);
}

MAKE_THUNK(libGL, glDebugMessageCallback)

void glDebugMessageCallback(GLDEBUGPROC a_0,const void* a_1){
struct {GLDEBUGPROC a_0;const void* a_1;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libGL_glDebugMessageCallback(&args);
}

MAKE_THUNK(libGL, glDebugMessageControl)

void glDebugMessageControl(GLenum a_0,GLenum a_1,GLenum a_2,GLsizei a_3,const GLuint* a_4,GLboolean a_5){
struct {GLenum a_0;GLenum a_1;GLenum a_2;GLsizei a_3;const GLuint* a_4;GLboolean a_5;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;args.a_4 = a_4;args.a_5 = a_5;
fexthunks_libGL_glDebugMessageControl(&args);
}

MAKE_THUNK(libGL, glDebugMessageInsert)

void glDebugMessageInsert(GLenum a_0,GLenum a_1,GLuint a_2,GLenum a_3,GLsizei a_4,const GLchar* a_5){
struct {GLenum a_0;GLenum a_1;GLuint a_2;GLenum a_3;GLsizei a_4;const GLchar* a_5;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;args.a_4 = a_4;args.a_5 = a_5;
fexthunks_libGL_glDebugMessageInsert(&args);
}

MAKE_THUNK(libGL, glDeleteBuffers)

void glDeleteBuffers(GLsizei a_0,const GLuint* a_1){
struct {GLsizei a_0;const GLuint* a_1;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libGL_glDeleteBuffers(&args);
}

MAKE_THUNK(libGL, glDeleteFramebuffers)

void glDeleteFramebuffers(GLsizei a_0,const GLuint* a_1){
struct {GLsizei a_0;const GLuint* a_1;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libGL_glDeleteFramebuffers(&args);
}

MAKE_THUNK(libGL, glDeleteProgram)

void glDeleteProgram(GLuint a_0){
struct {GLuint a_0;} args;
args.a_0 = a_0;
fexthunks_libGL_glDeleteProgram(&args);
}

MAKE_THUNK(libGL, glDeleteProgramPipelines)

void glDeleteProgramPipelines(GLsizei a_0,const GLuint* a_1){
struct {GLsizei a_0;const GLuint* a_1;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libGL_glDeleteProgramPipelines(&args);
}

MAKE_THUNK(libGL, glDeleteRenderbuffers)

void glDeleteRenderbuffers(GLsizei a_0,const GLuint* a_1){
struct {GLsizei a_0;const GLuint* a_1;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libGL_glDeleteRenderbuffers(&args);
}

MAKE_THUNK(libGL, glDeleteShader)

void glDeleteShader(GLuint a_0){
struct {GLuint a_0;} args;
args.a_0 = a_0;
fexthunks_libGL_glDeleteShader(&args);
}

MAKE_THUNK(libGL, glDeleteVertexArrays)

void glDeleteVertexArrays(GLsizei a_0,const GLuint* a_1){
struct {GLsizei a_0;const GLuint* a_1;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libGL_glDeleteVertexArrays(&args);
}

MAKE_THUNK(libGL, glDetachShader)

void glDetachShader(GLuint a_0,GLuint a_1){
struct {GLuint a_0;GLuint a_1;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libGL_glDetachShader(&args);
}

MAKE_THUNK(libGL, glDisableVertexAttribArray)

void glDisableVertexAttribArray(GLuint a_0){
struct {GLuint a_0;} args;
args.a_0 = a_0;
fexthunks_libGL_glDisableVertexAttribArray(&args);
}

MAKE_THUNK(libGL, glEnableVertexAttribArray)

void glEnableVertexAttribArray(GLuint a_0){
struct {GLuint a_0;} args;
args.a_0 = a_0;
fexthunks_libGL_glEnableVertexAttribArray(&args);
}

MAKE_THUNK(libGL, glFramebufferRenderbuffer)

void glFramebufferRenderbuffer(GLenum a_0,GLenum a_1,GLenum a_2,GLuint a_3){
struct {GLenum a_0;GLenum a_1;GLenum a_2;GLuint a_3;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;
fexthunks_libGL_glFramebufferRenderbuffer(&args);
}

MAKE_THUNK(libGL, glFramebufferTexture2D)

void glFramebufferTexture2D(GLenum a_0,GLenum a_1,GLenum a_2,GLuint a_3,GLint a_4){
struct {GLenum a_0;GLenum a_1;GLenum a_2;GLuint a_3;GLint a_4;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;args.a_4 = a_4;
fexthunks_libGL_glFramebufferTexture2D(&args);
}

MAKE_THUNK(libGL, glGenBuffers)

void glGenBuffers(GLsizei a_0,GLuint* a_1){
struct {GLsizei a_0;GLuint* a_1;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libGL_glGenBuffers(&args);
}

MAKE_THUNK(libGL, glGenerateMipmap)

void glGenerateMipmap(GLenum a_0){
struct {GLenum a_0;} args;
args.a_0 = a_0;
fexthunks_libGL_glGenerateMipmap(&args);
}

MAKE_THUNK(libGL, glGenerateTextureMipmap)

void glGenerateTextureMipmap(GLuint a_0){
struct {GLuint a_0;} args;
args.a_0 = a_0;
fexthunks_libGL_glGenerateTextureMipmap(&args);
}

MAKE_THUNK(libGL, glGenFramebuffers)

void glGenFramebuffers(GLsizei a_0,GLuint* a_1){
struct {GLsizei a_0;GLuint* a_1;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libGL_glGenFramebuffers(&args);
}

MAKE_THUNK(libGL, glGenProgramPipelines)

void glGenProgramPipelines(GLsizei a_0,GLuint* a_1){
struct {GLsizei a_0;GLuint* a_1;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libGL_glGenProgramPipelines(&args);
}

MAKE_THUNK(libGL, glGenRenderbuffers)

void glGenRenderbuffers(GLsizei a_0,GLuint* a_1){
struct {GLsizei a_0;GLuint* a_1;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libGL_glGenRenderbuffers(&args);
}

MAKE_THUNK(libGL, glGenVertexArrays)

void glGenVertexArrays(GLsizei a_0,GLuint* a_1){
struct {GLsizei a_0;GLuint* a_1;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libGL_glGenVertexArrays(&args);
}

MAKE_THUNK(libGL, glGetFramebufferAttachmentParameteriv)

void glGetFramebufferAttachmentParameteriv(GLenum a_0,GLenum a_1,GLenum a_2,GLint* a_3){
struct {GLenum a_0;GLenum a_1;GLenum a_2;GLint* a_3;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;
fexthunks_libGL_glGetFramebufferAttachmentParameteriv(&args);
}

MAKE_THUNK(libGL, glGetNamedBufferSubData)

void glGetNamedBufferSubData(GLuint a_0,GLintptr a_1,GLsizeiptr a_2,void* a_3){
struct {GLuint a_0;GLintptr a_1;GLsizeiptr a_2;void* a_3;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;
fexthunks_libGL_glGetNamedBufferSubData(&args);
}

MAKE_THUNK(libGL, glGetProgramBinary)

void glGetProgramBinary(GLuint a_0,GLsizei a_1,GLsizei* a_2,GLenum* a_3,void* a_4){
struct {GLuint a_0;GLsizei a_1;GLsizei* a_2;GLenum* a_3;void* a_4;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;args.a_4 = a_4;
fexthunks_libGL_glGetProgramBinary(&args);
}

MAKE_THUNK(libGL, glGetProgramInfoLog)

void glGetProgramInfoLog(GLuint a_0,GLsizei a_1,GLsizei* a_2,GLchar* a_3){
struct {GLuint a_0;GLsizei a_1;GLsizei* a_2;GLchar* a_3;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;
fexthunks_libGL_glGetProgramInfoLog(&args);
}

MAKE_THUNK(libGL, glGetProgramiv)

void glGetProgramiv(GLuint a_0,GLenum a_1,GLint* a_2){
struct {GLuint a_0;GLenum a_1;GLint* a_2;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
fexthunks_libGL_glGetProgramiv(&args);
}

MAKE_THUNK(libGL, glGetProgramPipelineInfoLog)

void glGetProgramPipelineInfoLog(GLuint a_0,GLsizei a_1,GLsizei* a_2,GLchar* a_3){
struct {GLuint a_0;GLsizei a_1;GLsizei* a_2;GLchar* a_3;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;
fexthunks_libGL_glGetProgramPipelineInfoLog(&args);
}

MAKE_THUNK(libGL, glGetProgramPipelineiv)

void glGetProgramPipelineiv(GLuint a_0,GLenum a_1,GLint* a_2){
struct {GLuint a_0;GLenum a_1;GLint* a_2;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
fexthunks_libGL_glGetProgramPipelineiv(&args);
}

MAKE_THUNK(libGL, glGetShaderInfoLog)

void glGetShaderInfoLog(GLuint a_0,GLsizei a_1,GLsizei* a_2,GLchar* a_3){
struct {GLuint a_0;GLsizei a_1;GLsizei* a_2;GLchar* a_3;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;
fexthunks_libGL_glGetShaderInfoLog(&args);
}

MAKE_THUNK(libGL, glGetShaderiv)

void glGetShaderiv(GLuint a_0,GLenum a_1,GLint* a_2){
struct {GLuint a_0;GLenum a_1;GLint* a_2;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
fexthunks_libGL_glGetShaderiv(&args);
}

MAKE_THUNK(libGL, glGetShaderSource)

void glGetShaderSource(GLuint a_0,GLsizei a_1,GLsizei* a_2,GLchar* a_3){
struct {GLuint a_0;GLsizei a_1;GLsizei* a_2;GLchar* a_3;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;
fexthunks_libGL_glGetShaderSource(&args);
}

MAKE_THUNK(libGL, glGetTextureImage)

void glGetTextureImage(GLuint a_0,GLint a_1,GLenum a_2,GLenum a_3,GLsizei a_4,void* a_5){
struct {GLuint a_0;GLint a_1;GLenum a_2;GLenum a_3;GLsizei a_4;void* a_5;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;args.a_4 = a_4;args.a_5 = a_5;
fexthunks_libGL_glGetTextureImage(&args);
}

MAKE_THUNK(libGL, glInvalidateFramebuffer)

void glInvalidateFramebuffer(GLenum a_0,GLsizei a_1,const GLenum* a_2){
struct {GLenum a_0;GLsizei a_1;const GLenum* a_2;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
fexthunks_libGL_glInvalidateFramebuffer(&args);
}

MAKE_THUNK(libGL, glInvalidateNamedFramebufferData)

void glInvalidateNamedFramebufferData(GLuint a_0,GLsizei a_1,const GLenum* a_2){
struct {GLuint a_0;GLsizei a_1;const GLenum* a_2;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
fexthunks_libGL_glInvalidateNamedFramebufferData(&args);
}

MAKE_THUNK(libGL, glLinkProgram)

void glLinkProgram(GLuint a_0){
struct {GLuint a_0;} args;
args.a_0 = a_0;
fexthunks_libGL_glLinkProgram(&args);
}

MAKE_THUNK(libGL, glNamedFramebufferRenderbuffer)

void glNamedFramebufferRenderbuffer(GLuint a_0,GLenum a_1,GLenum a_2,GLuint a_3){
struct {GLuint a_0;GLenum a_1;GLenum a_2;GLuint a_3;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;
fexthunks_libGL_glNamedFramebufferRenderbuffer(&args);
}

MAKE_THUNK(libGL, glNamedFramebufferTexture)

void glNamedFramebufferTexture(GLuint a_0,GLenum a_1,GLuint a_2,GLint a_3){
struct {GLuint a_0;GLenum a_1;GLuint a_2;GLint a_3;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;
fexthunks_libGL_glNamedFramebufferTexture(&args);
}

MAKE_THUNK(libGL, glNamedRenderbufferStorage)

void glNamedRenderbufferStorage(GLuint a_0,GLenum a_1,GLsizei a_2,GLsizei a_3){
struct {GLuint a_0;GLenum a_1;GLsizei a_2;GLsizei a_3;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;
fexthunks_libGL_glNamedRenderbufferStorage(&args);
}

MAKE_THUNK(libGL, glProgramBinary)

void glProgramBinary(GLuint a_0,GLenum a_1,const void* a_2,GLsizei a_3){
struct {GLuint a_0;GLenum a_1;const void* a_2;GLsizei a_3;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;
fexthunks_libGL_glProgramBinary(&args);
}

MAKE_THUNK(libGL, glProgramParameteri)

void glProgramParameteri(GLuint a_0,GLenum a_1,GLint a_2){
struct {GLuint a_0;GLenum a_1;GLint a_2;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
fexthunks_libGL_glProgramParameteri(&args);
}

MAKE_THUNK(libGL, glProgramUniform1f)

void glProgramUniform1f(GLuint a_0,GLint a_1,GLfloat a_2){
struct {GLuint a_0;GLint a_1;GLfloat a_2;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
fexthunks_libGL_glProgramUniform1f(&args);
}

MAKE_THUNK(libGL, glProgramUniform1i)

void glProgramUniform1i(GLuint a_0,GLint a_1,GLint a_2){
struct {GLuint a_0;GLint a_1;GLint a_2;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
fexthunks_libGL_glProgramUniform1i(&args);
}

MAKE_THUNK(libGL, glProgramUniform2fv)

void glProgramUniform2fv(GLuint a_0,GLint a_1,GLsizei a_2,const GLfloat* a_3){
struct {GLuint a_0;GLint a_1;GLsizei a_2;const GLfloat* a_3;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;
fexthunks_libGL_glProgramUniform2fv(&args);
}

MAKE_THUNK(libGL, glProgramUniform3fv)

void glProgramUniform3fv(GLuint a_0,GLint a_1,GLsizei a_2,const GLfloat* a_3){
struct {GLuint a_0;GLint a_1;GLsizei a_2;const GLfloat* a_3;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;
fexthunks_libGL_glProgramUniform3fv(&args);
}

MAKE_THUNK(libGL, glProgramUniform4fv)

void glProgramUniform4fv(GLuint a_0,GLint a_1,GLsizei a_2,const GLfloat* a_3){
struct {GLuint a_0;GLint a_1;GLsizei a_2;const GLfloat* a_3;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;
fexthunks_libGL_glProgramUniform4fv(&args);
}

MAKE_THUNK(libGL, glProgramUniformMatrix4fv)

void glProgramUniformMatrix4fv(GLuint a_0,GLint a_1,GLsizei a_2,GLboolean a_3,const GLfloat* a_4){
struct {GLuint a_0;GLint a_1;GLsizei a_2;GLboolean a_3;const GLfloat* a_4;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;args.a_4 = a_4;
fexthunks_libGL_glProgramUniformMatrix4fv(&args);
}

MAKE_THUNK(libGL, glRenderbufferStorage)

void glRenderbufferStorage(GLenum a_0,GLenum a_1,GLsizei a_2,GLsizei a_3){
struct {GLenum a_0;GLenum a_1;GLsizei a_2;GLsizei a_3;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;
fexthunks_libGL_glRenderbufferStorage(&args);
}

MAKE_THUNK(libGL, glShaderSource)

void glShaderSource(GLuint a_0,GLsizei a_1,const GLchar* const* a_2,const GLint* a_3){
struct {GLuint a_0;GLsizei a_1;const GLchar* const* a_2;const GLint* a_3;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;
fexthunks_libGL_glShaderSource(&args);
}

MAKE_THUNK(libGL, glTexStorage2D)

void glTexStorage2D(GLenum a_0,GLsizei a_1,GLenum a_2,GLsizei a_3,GLsizei a_4){
struct {GLenum a_0;GLsizei a_1;GLenum a_2;GLsizei a_3;GLsizei a_4;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;args.a_4 = a_4;
fexthunks_libGL_glTexStorage2D(&args);
}

MAKE_THUNK(libGL, glTextureParameteri)

void glTextureParameteri(GLuint a_0,GLenum a_1,GLint a_2){
struct {GLuint a_0;GLenum a_1;GLint a_2;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
fexthunks_libGL_glTextureParameteri(&args);
}

MAKE_THUNK(libGL, glTextureStorage2D)

void glTextureStorage2D(GLuint a_0,GLsizei a_1,GLenum a_2,GLsizei a_3,GLsizei a_4){
struct {GLuint a_0;GLsizei a_1;GLenum a_2;GLsizei a_3;GLsizei a_4;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;args.a_4 = a_4;
fexthunks_libGL_glTextureStorage2D(&args);
}

MAKE_THUNK(libGL, glTextureSubImage2D)

void glTextureSubImage2D(GLuint a_0,GLint a_1,GLint a_2,GLint a_3,GLsizei a_4,GLsizei a_5,GLenum a_6,GLenum a_7,const void* a_8){
struct {GLuint a_0;GLint a_1;GLint a_2;GLint a_3;GLsizei a_4;GLsizei a_5;GLenum a_6;GLenum a_7;const void* a_8;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;args.a_4 = a_4;args.a_5 = a_5;args.a_6 = a_6;args.a_7 = a_7;args.a_8 = a_8;
fexthunks_libGL_glTextureSubImage2D(&args);
}

MAKE_THUNK(libGL, glUniform1f)

void glUniform1f(GLint a_0,GLfloat a_1){
struct {GLint a_0;GLfloat a_1;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libGL_glUniform1f(&args);
}

MAKE_THUNK(libGL, glUniform1fv)

void glUniform1fv(GLint a_0,GLsizei a_1,const GLfloat* a_2){
struct {GLint a_0;GLsizei a_1;const GLfloat* a_2;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
fexthunks_libGL_glUniform1fv(&args);
}

MAKE_THUNK(libGL, glUniform1i)

void glUniform1i(GLint a_0,GLint a_1){
struct {GLint a_0;GLint a_1;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libGL_glUniform1i(&args);
}

MAKE_THUNK(libGL, glUniform1iv)

void glUniform1iv(GLint a_0,GLsizei a_1,const GLint* a_2){
struct {GLint a_0;GLsizei a_1;const GLint* a_2;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
fexthunks_libGL_glUniform1iv(&args);
}

MAKE_THUNK(libGL, glUniform2f)

void glUniform2f(GLint a_0,GLfloat a_1,GLfloat a_2){
struct {GLint a_0;GLfloat a_1;GLfloat a_2;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
fexthunks_libGL_glUniform2f(&args);
}

MAKE_THUNK(libGL, glUniform2fv)

void glUniform2fv(GLint a_0,GLsizei a_1,const GLfloat* a_2){
struct {GLint a_0;GLsizei a_1;const GLfloat* a_2;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
fexthunks_libGL_glUniform2fv(&args);
}

MAKE_THUNK(libGL, glUniform2i)

void glUniform2i(GLint a_0,GLint a_1,GLint a_2){
struct {GLint a_0;GLint a_1;GLint a_2;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
fexthunks_libGL_glUniform2i(&args);
}

MAKE_THUNK(libGL, glUniform2iv)

void glUniform2iv(GLint a_0,GLsizei a_1,const GLint* a_2){
struct {GLint a_0;GLsizei a_1;const GLint* a_2;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
fexthunks_libGL_glUniform2iv(&args);
}

MAKE_THUNK(libGL, glUniform3f)

void glUniform3f(GLint a_0,GLfloat a_1,GLfloat a_2,GLfloat a_3){
struct {GLint a_0;GLfloat a_1;GLfloat a_2;GLfloat a_3;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;
fexthunks_libGL_glUniform3f(&args);
}

MAKE_THUNK(libGL, glUniform3fv)

void glUniform3fv(GLint a_0,GLsizei a_1,const GLfloat* a_2){
struct {GLint a_0;GLsizei a_1;const GLfloat* a_2;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
fexthunks_libGL_glUniform3fv(&args);
}

MAKE_THUNK(libGL, glUniform3i)

void glUniform3i(GLint a_0,GLint a_1,GLint a_2,GLint a_3){
struct {GLint a_0;GLint a_1;GLint a_2;GLint a_3;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;
fexthunks_libGL_glUniform3i(&args);
}

MAKE_THUNK(libGL, glUniform3iv)

void glUniform3iv(GLint a_0,GLsizei a_1,const GLint* a_2){
struct {GLint a_0;GLsizei a_1;const GLint* a_2;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
fexthunks_libGL_glUniform3iv(&args);
}

MAKE_THUNK(libGL, glUniform4f)

void glUniform4f(GLint a_0,GLfloat a_1,GLfloat a_2,GLfloat a_3,GLfloat a_4){
struct {GLint a_0;GLfloat a_1;GLfloat a_2;GLfloat a_3;GLfloat a_4;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;args.a_4 = a_4;
fexthunks_libGL_glUniform4f(&args);
}

MAKE_THUNK(libGL, glUniform4fv)

void glUniform4fv(GLint a_0,GLsizei a_1,const GLfloat* a_2){
struct {GLint a_0;GLsizei a_1;const GLfloat* a_2;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
fexthunks_libGL_glUniform4fv(&args);
}

MAKE_THUNK(libGL, glUniform4i)

void glUniform4i(GLint a_0,GLint a_1,GLint a_2,GLint a_3,GLint a_4){
struct {GLint a_0;GLint a_1;GLint a_2;GLint a_3;GLint a_4;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;args.a_4 = a_4;
fexthunks_libGL_glUniform4i(&args);
}

MAKE_THUNK(libGL, glUniform4iv)

void glUniform4iv(GLint a_0,GLsizei a_1,const GLint* a_2){
struct {GLint a_0;GLsizei a_1;const GLint* a_2;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
fexthunks_libGL_glUniform4iv(&args);
}

MAKE_THUNK(libGL, glUniformMatrix2fv)

void glUniformMatrix2fv(GLint a_0,GLsizei a_1,GLboolean a_2,const GLfloat* a_3){
struct {GLint a_0;GLsizei a_1;GLboolean a_2;const GLfloat* a_3;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;
fexthunks_libGL_glUniformMatrix2fv(&args);
}

MAKE_THUNK(libGL, glUniformMatrix3fv)

void glUniformMatrix3fv(GLint a_0,GLsizei a_1,GLboolean a_2,const GLfloat* a_3){
struct {GLint a_0;GLsizei a_1;GLboolean a_2;const GLfloat* a_3;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;
fexthunks_libGL_glUniformMatrix3fv(&args);
}

MAKE_THUNK(libGL, glUniformMatrix4fv)

void glUniformMatrix4fv(GLint a_0,GLsizei a_1,GLboolean a_2,const GLfloat* a_3){
struct {GLint a_0;GLsizei a_1;GLboolean a_2;const GLfloat* a_3;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;
fexthunks_libGL_glUniformMatrix4fv(&args);
}

MAKE_THUNK(libGL, glUseProgram)

void glUseProgram(GLuint a_0){
struct {GLuint a_0;} args;
args.a_0 = a_0;
fexthunks_libGL_glUseProgram(&args);
}

MAKE_THUNK(libGL, glUseProgramStages)

void glUseProgramStages(GLuint a_0,GLbitfield a_1,GLuint a_2){
struct {GLuint a_0;GLbitfield a_1;GLuint a_2;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
fexthunks_libGL_glUseProgramStages(&args);
}

MAKE_THUNK(libGL, glValidateProgram)

void glValidateProgram(GLuint a_0){
struct {GLuint a_0;} args;
args.a_0 = a_0;
fexthunks_libGL_glValidateProgram(&args);
}

MAKE_THUNK(libGL, glValidateProgramPipeline)

void glValidateProgramPipeline(GLuint a_0){
struct {GLuint a_0;} args;
args.a_0 = a_0;
fexthunks_libGL_glValidateProgramPipeline(&args);
}

MAKE_THUNK(libGL, glVertexAttribIPointer)

void glVertexAttribIPointer(GLuint a_0,GLint a_1,GLenum a_2,GLsizei a_3,const void* a_4){
struct {GLuint a_0;GLint a_1;GLenum a_2;GLsizei a_3;const void* a_4;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;args.a_4 = a_4;
fexthunks_libGL_glVertexAttribIPointer(&args);
}

MAKE_THUNK(libGL, glVertexAttribPointer)

void glVertexAttribPointer(GLuint a_0,GLint a_1,GLenum a_2,GLboolean a_3,GLsizei a_4,const void* a_5){
struct {GLuint a_0;GLint a_1;GLenum a_2;GLboolean a_3;GLsizei a_4;const void* a_5;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;args.a_4 = a_4;args.a_5 = a_5;
fexthunks_libGL_glVertexAttribPointer(&args);
}

MAKE_THUNK(libX11, XOpenDisplay)

Display* XOpenDisplay(const char* a_0){
struct {const char* a_0;Display* rv;} args;
args.a_0 = a_0;
fexthunks_libX11_XOpenDisplay(&args);
return args.rv;
}

MAKE_THUNK(libX11, XCreateColormap)

Colormap XCreateColormap(Display* a_0,Window a_1,Visual* a_2,int a_3){
struct {Display* a_0;Window a_1;Visual* a_2;int a_3;Colormap rv;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;
fexthunks_libX11_XCreateColormap(&args);
return args.rv;
}

MAKE_THUNK(libX11, XCreateWindow)

Window XCreateWindow(Display* a_0,Window a_1,int a_2,int a_3,unsigned int a_4,unsigned int a_5,unsigned int a_6,int a_7,unsigned int a_8,Visual* a_9,long unsigned int a_10,XSetWindowAttributes* a_11){
struct {Display* a_0;Window a_1;int a_2;int a_3;unsigned int a_4;unsigned int a_5;unsigned int a_6;int a_7;unsigned int a_8;Visual* a_9;long unsigned int a_10;XSetWindowAttributes* a_11;Window rv;} args;
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

int XChangeProperty(Display* a_0,Window a_1,Atom a_2,Atom a_3,int a_4,int a_5,const unsigned char* a_6,int a_7){
struct {Display* a_0;Window a_1;Atom a_2;Atom a_3;int a_4;int a_5;const unsigned char* a_6;int a_7;int rv;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;args.a_4 = a_4;args.a_5 = a_5;args.a_6 = a_6;args.a_7 = a_7;
fexthunks_libX11_XChangeProperty(&args);
return args.rv;
}

MAKE_THUNK(libX11, XCloseDisplay)

int XCloseDisplay(Display* a_0){
struct {Display* a_0;int rv;} args;
args.a_0 = a_0;
fexthunks_libX11_XCloseDisplay(&args);
return args.rv;
}

MAKE_THUNK(libX11, XDestroyWindow)

int XDestroyWindow(Display* a_0,Window a_1){
struct {Display* a_0;Window a_1;int rv;} args;
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

Atom XInternAtom(Display* a_0,const char* a_1,int a_2){
struct {Display* a_0;const char* a_1;int a_2;Atom rv;} args;
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

int XLookupString(XKeyEvent* a_0,char* a_1,int a_2,KeySym* a_3,XComposeStatus* a_4){
struct {XKeyEvent* a_0;char* a_1;int a_2;KeySym* a_3;XComposeStatus* a_4;int rv;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;args.a_4 = a_4;
fexthunks_libX11_XLookupString(&args);
return args.rv;
}

MAKE_THUNK(libX11, XNextEvent)

int XNextEvent(Display* a_0,XEvent* a_1){
struct {Display* a_0;XEvent* a_1;int rv;} args;
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

int XPending(Display* a_0){
struct {Display* a_0;int rv;} args;
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

MAKE_THUNK(libX11, XListExtensions)

char** XListExtensions(Display* a_0,int* a_1){
struct {Display* a_0;int* a_1;char** rv;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libX11_XListExtensions(&args);
return args.rv;
}

MAKE_THUNK(libX11, XSetLocaleModifiers)

char* XSetLocaleModifiers(const char* a_0){
struct {const char* a_0;char* rv;} args;
args.a_0 = a_0;
fexthunks_libX11_XSetLocaleModifiers(&args);
return args.rv;
}

MAKE_THUNK(libX11, XCreatePixmapCursor)

Cursor XCreatePixmapCursor(Display* a_0,Pixmap a_1,Pixmap a_2,XColor* a_3,XColor* a_4,unsigned int a_5,unsigned int a_6){
struct {Display* a_0;Pixmap a_1;Pixmap a_2;XColor* a_3;XColor* a_4;unsigned int a_5;unsigned int a_6;Cursor rv;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;args.a_4 = a_4;args.a_5 = a_5;args.a_6 = a_6;
fexthunks_libX11_XCreatePixmapCursor(&args);
return args.rv;
}

MAKE_THUNK(libX11, XCloseIM)

int XCloseIM(XIM a_0){
struct {XIM a_0;int rv;} args;
args.a_0 = a_0;
fexthunks_libX11_XCloseIM(&args);
return args.rv;
}

MAKE_THUNK(libX11, XDefineCursor)

int XDefineCursor(Display* a_0,Window a_1,Cursor a_2){
struct {Display* a_0;Window a_1;Cursor a_2;int rv;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
fexthunks_libX11_XDefineCursor(&args);
return args.rv;
}

MAKE_THUNK(libX11, XDrawString16)

int XDrawString16(Display* a_0,Drawable a_1,GC a_2,int a_3,int a_4,const XChar2b* a_5,int a_6){
struct {Display* a_0;Drawable a_1;GC a_2;int a_3;int a_4;const XChar2b* a_5;int a_6;int rv;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;args.a_4 = a_4;args.a_5 = a_5;args.a_6 = a_6;
fexthunks_libX11_XDrawString16(&args);
return args.rv;
}

MAKE_THUNK(libX11, XEventsQueued)

int XEventsQueued(Display* a_0,int a_1){
struct {Display* a_0;int a_1;int rv;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libX11_XEventsQueued(&args);
return args.rv;
}

MAKE_THUNK(libX11, XFillRectangle)

int XFillRectangle(Display* a_0,Drawable a_1,GC a_2,int a_3,int a_4,unsigned int a_5,unsigned int a_6){
struct {Display* a_0;Drawable a_1;GC a_2;int a_3;int a_4;unsigned int a_5;unsigned int a_6;int rv;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;args.a_4 = a_4;args.a_5 = a_5;args.a_6 = a_6;
fexthunks_libX11_XFillRectangle(&args);
return args.rv;
}

MAKE_THUNK(libX11, XFilterEvent)

int XFilterEvent(XEvent* a_0,Window a_1){
struct {XEvent* a_0;Window a_1;int rv;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libX11_XFilterEvent(&args);
return args.rv;
}

MAKE_THUNK(libX11, XFlush)

int XFlush(Display* a_0){
struct {Display* a_0;int rv;} args;
args.a_0 = a_0;
fexthunks_libX11_XFlush(&args);
return args.rv;
}

MAKE_THUNK(libX11, XFreeColormap)

int XFreeColormap(Display* a_0,Colormap a_1){
struct {Display* a_0;Colormap a_1;int rv;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libX11_XFreeColormap(&args);
return args.rv;
}

MAKE_THUNK(libX11, XFreeCursor)

int XFreeCursor(Display* a_0,Cursor a_1){
struct {Display* a_0;Cursor a_1;int rv;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libX11_XFreeCursor(&args);
return args.rv;
}

MAKE_THUNK(libX11, XFreeExtensionList)

int XFreeExtensionList(char** a_0){
struct {char** a_0;int rv;} args;
args.a_0 = a_0;
fexthunks_libX11_XFreeExtensionList(&args);
return args.rv;
}

MAKE_THUNK(libX11, XFreeFont)

int XFreeFont(Display* a_0,XFontStruct* a_1){
struct {Display* a_0;XFontStruct* a_1;int rv;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libX11_XFreeFont(&args);
return args.rv;
}

MAKE_THUNK(libX11, XFreeGC)

int XFreeGC(Display* a_0,GC a_1){
struct {Display* a_0;GC a_1;int rv;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libX11_XFreeGC(&args);
return args.rv;
}

MAKE_THUNK(libX11, XFreePixmap)

int XFreePixmap(Display* a_0,Pixmap a_1){
struct {Display* a_0;Pixmap a_1;int rv;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libX11_XFreePixmap(&args);
return args.rv;
}

MAKE_THUNK(libX11, XGetErrorDatabaseText)

int XGetErrorDatabaseText(Display* a_0,const char* a_1,const char* a_2,const char* a_3,char* a_4,int a_5){
struct {Display* a_0;const char* a_1;const char* a_2;const char* a_3;char* a_4;int a_5;int rv;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;args.a_4 = a_4;args.a_5 = a_5;
fexthunks_libX11_XGetErrorDatabaseText(&args);
return args.rv;
}

MAKE_THUNK(libX11, XGetErrorText)

int XGetErrorText(Display* a_0,int a_1,char* a_2,int a_3){
struct {Display* a_0;int a_1;char* a_2;int a_3;int rv;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;
fexthunks_libX11_XGetErrorText(&args);
return args.rv;
}

MAKE_THUNK(libX11, XGetEventData)

int XGetEventData(Display* a_0,XGenericEventCookie* a_1){
struct {Display* a_0;XGenericEventCookie* a_1;int rv;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libX11_XGetEventData(&args);
return args.rv;
}

MAKE_THUNK(libX11, XGetWindowProperty)

int XGetWindowProperty(Display* a_0,Window a_1,Atom a_2,long int a_3,long int a_4,int a_5,Atom a_6,Atom* a_7,int* a_8,long unsigned int* a_9,long unsigned int* a_10,unsigned char** a_11){
struct {Display* a_0;Window a_1;Atom a_2;long int a_3;long int a_4;int a_5;Atom a_6;Atom* a_7;int* a_8;long unsigned int* a_9;long unsigned int* a_10;unsigned char** a_11;int rv;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;args.a_4 = a_4;args.a_5 = a_5;args.a_6 = a_6;args.a_7 = a_7;args.a_8 = a_8;args.a_9 = a_9;args.a_10 = a_10;args.a_11 = a_11;
fexthunks_libX11_XGetWindowProperty(&args);
return args.rv;
}

MAKE_THUNK(libX11, XGrabPointer)

int XGrabPointer(Display* a_0,Window a_1,int a_2,unsigned int a_3,int a_4,int a_5,Window a_6,Cursor a_7,Time a_8){
struct {Display* a_0;Window a_1;int a_2;unsigned int a_3;int a_4;int a_5;Window a_6;Cursor a_7;Time a_8;int rv;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;args.a_4 = a_4;args.a_5 = a_5;args.a_6 = a_6;args.a_7 = a_7;args.a_8 = a_8;
fexthunks_libX11_XGrabPointer(&args);
return args.rv;
}

MAKE_THUNK(libX11, XGrabServer)

int XGrabServer(Display* a_0){
struct {Display* a_0;int rv;} args;
args.a_0 = a_0;
fexthunks_libX11_XGrabServer(&args);
return args.rv;
}

MAKE_THUNK(libX11, XIconifyWindow)

int XIconifyWindow(Display* a_0,Window a_1,int a_2){
struct {Display* a_0;Window a_1;int a_2;int rv;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
fexthunks_libX11_XIconifyWindow(&args);
return args.rv;
}

MAKE_THUNK(libX11, XIfEvent)

int XIfEvent(Display* a_0,XEvent* a_1,XIfEventFN* a_2,XPointer a_3){
struct {Display* a_0;XEvent* a_1;XIfEventFN* a_2;XPointer a_3;int rv;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;
fexthunks_libX11_XIfEvent(&args);
return args.rv;
}

MAKE_THUNK(libX11, XInitThreads)

int XInitThreads(){
struct {int rv;} args;

fexthunks_libX11_XInitThreads(&args);
return args.rv;
}

MAKE_THUNK(libX11, XMapRaised)

int XMapRaised(Display* a_0,Window a_1){
struct {Display* a_0;Window a_1;int rv;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libX11_XMapRaised(&args);
return args.rv;
}

MAKE_THUNK(libX11, XMoveResizeWindow)

int XMoveResizeWindow(Display* a_0,Window a_1,int a_2,int a_3,unsigned int a_4,unsigned int a_5){
struct {Display* a_0;Window a_1;int a_2;int a_3;unsigned int a_4;unsigned int a_5;int rv;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;args.a_4 = a_4;args.a_5 = a_5;
fexthunks_libX11_XMoveResizeWindow(&args);
return args.rv;
}

MAKE_THUNK(libX11, XMoveWindow)

int XMoveWindow(Display* a_0,Window a_1,int a_2,int a_3){
struct {Display* a_0;Window a_1;int a_2;int a_3;int rv;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;
fexthunks_libX11_XMoveWindow(&args);
return args.rv;
}

MAKE_THUNK(libX11, XPeekEvent)

int XPeekEvent(Display* a_0,XEvent* a_1){
struct {Display* a_0;XEvent* a_1;int rv;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libX11_XPeekEvent(&args);
return args.rv;
}

MAKE_THUNK(libX11, XQueryExtension)

int XQueryExtension(Display* a_0,const char* a_1,int* a_2,int* a_3,int* a_4){
struct {Display* a_0;const char* a_1;int* a_2;int* a_3;int* a_4;int rv;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;args.a_4 = a_4;
fexthunks_libX11_XQueryExtension(&args);
return args.rv;
}

MAKE_THUNK(libX11, XQueryPointer)

int XQueryPointer(Display* a_0,Window a_1,Window* a_2,Window* a_3,int* a_4,int* a_5,int* a_6,int* a_7,unsigned int* a_8){
struct {Display* a_0;Window a_1;Window* a_2;Window* a_3;int* a_4;int* a_5;int* a_6;int* a_7;unsigned int* a_8;int rv;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;args.a_4 = a_4;args.a_5 = a_5;args.a_6 = a_6;args.a_7 = a_7;args.a_8 = a_8;
fexthunks_libX11_XQueryPointer(&args);
return args.rv;
}

MAKE_THUNK(libX11, XQueryTree)

int XQueryTree(Display* a_0,Window a_1,Window* a_2,Window* a_3,Window** a_4,unsigned int* a_5){
struct {Display* a_0;Window a_1;Window* a_2;Window* a_3;Window** a_4;unsigned int* a_5;int rv;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;args.a_4 = a_4;args.a_5 = a_5;
fexthunks_libX11_XQueryTree(&args);
return args.rv;
}

MAKE_THUNK(libX11, XResetScreenSaver)

int XResetScreenSaver(Display* a_0){
struct {Display* a_0;int rv;} args;
args.a_0 = a_0;
fexthunks_libX11_XResetScreenSaver(&args);
return args.rv;
}

MAKE_THUNK(libX11, XResizeWindow)

int XResizeWindow(Display* a_0,Window a_1,unsigned int a_2,unsigned int a_3){
struct {Display* a_0;Window a_1;unsigned int a_2;unsigned int a_3;int rv;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;
fexthunks_libX11_XResizeWindow(&args);
return args.rv;
}

MAKE_THUNK(libX11, XSelectInput)

int XSelectInput(Display* a_0,Window a_1,long int a_2){
struct {Display* a_0;Window a_1;long int a_2;int rv;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
fexthunks_libX11_XSelectInput(&args);
return args.rv;
}

MAKE_THUNK(libX11, XSendEvent)

int XSendEvent(Display* a_0,Window a_1,int a_2,long int a_3,XEvent* a_4){
struct {Display* a_0;Window a_1;int a_2;long int a_3;XEvent* a_4;int rv;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;args.a_4 = a_4;
fexthunks_libX11_XSendEvent(&args);
return args.rv;
}

MAKE_THUNK(libX11, XSetErrorHandler)

XSetErrorHandlerFN* XSetErrorHandler(XErrorHandler a_0){
struct {XErrorHandler a_0;XSetErrorHandlerFN* rv;} args;
args.a_0 = a_0;
fexthunks_libX11_XSetErrorHandler(&args);
return args.rv;
}

MAKE_THUNK(libX11, XSetTransientForHint)

int XSetTransientForHint(Display* a_0,Window a_1,Window a_2){
struct {Display* a_0;Window a_1;Window a_2;int rv;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
fexthunks_libX11_XSetTransientForHint(&args);
return args.rv;
}

MAKE_THUNK(libX11, XSetWMProtocols)

int XSetWMProtocols(Display* a_0,Window a_1,Atom* a_2,int a_3){
struct {Display* a_0;Window a_1;Atom* a_2;int a_3;int rv;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;
fexthunks_libX11_XSetWMProtocols(&args);
return args.rv;
}

MAKE_THUNK(libX11, XSync)

int XSync(Display* a_0,int a_1){
struct {Display* a_0;int a_1;int rv;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libX11_XSync(&args);
return args.rv;
}

MAKE_THUNK(libX11, XTextExtents16)

int XTextExtents16(XFontStruct* a_0,const XChar2b* a_1,int a_2,int* a_3,int* a_4,int* a_5,XCharStruct* a_6){
struct {XFontStruct* a_0;const XChar2b* a_1;int a_2;int* a_3;int* a_4;int* a_5;XCharStruct* a_6;int rv;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;args.a_4 = a_4;args.a_5 = a_5;args.a_6 = a_6;
fexthunks_libX11_XTextExtents16(&args);
return args.rv;
}

MAKE_THUNK(libX11, XTranslateCoordinates)

int XTranslateCoordinates(Display* a_0,Window a_1,Window a_2,int a_3,int a_4,int* a_5,int* a_6,Window* a_7){
struct {Display* a_0;Window a_1;Window a_2;int a_3;int a_4;int* a_5;int* a_6;Window* a_7;int rv;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;args.a_4 = a_4;args.a_5 = a_5;args.a_6 = a_6;args.a_7 = a_7;
fexthunks_libX11_XTranslateCoordinates(&args);
return args.rv;
}

MAKE_THUNK(libX11, XUngrabPointer)

int XUngrabPointer(Display* a_0,Time a_1){
struct {Display* a_0;Time a_1;int rv;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libX11_XUngrabPointer(&args);
return args.rv;
}

MAKE_THUNK(libX11, XUngrabServer)

int XUngrabServer(Display* a_0){
struct {Display* a_0;int rv;} args;
args.a_0 = a_0;
fexthunks_libX11_XUngrabServer(&args);
return args.rv;
}

MAKE_THUNK(libX11, XUnmapWindow)

int XUnmapWindow(Display* a_0,Window a_1){
struct {Display* a_0;Window a_1;int rv;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libX11_XUnmapWindow(&args);
return args.rv;
}

MAKE_THUNK(libX11, Xutf8LookupString)

int Xutf8LookupString(XIC a_0,XKeyPressedEvent* a_1,char* a_2,int a_3,KeySym* a_4,int* a_5){
struct {XIC a_0;XKeyPressedEvent* a_1;char* a_2;int a_3;KeySym* a_4;int* a_5;int rv;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;args.a_4 = a_4;args.a_5 = a_5;
fexthunks_libX11_Xutf8LookupString(&args);
return args.rv;
}

MAKE_THUNK(libX11, XWarpPointer)

int XWarpPointer(Display* a_0,Window a_1,Window a_2,int a_3,int a_4,unsigned int a_5,unsigned int a_6,int a_7,int a_8){
struct {Display* a_0;Window a_1;Window a_2;int a_3;int a_4;unsigned int a_5;unsigned int a_6;int a_7;int a_8;int rv;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;args.a_4 = a_4;args.a_5 = a_5;args.a_6 = a_6;args.a_7 = a_7;args.a_8 = a_8;
fexthunks_libX11_XWarpPointer(&args);
return args.rv;
}

MAKE_THUNK(libX11, XWindowEvent)

int XWindowEvent(Display* a_0,Window a_1,long int a_2,XEvent* a_3){
struct {Display* a_0;Window a_1;long int a_2;XEvent* a_3;int rv;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;
fexthunks_libX11_XWindowEvent(&args);
return args.rv;
}

MAKE_THUNK(libX11, XCreateBitmapFromData)

Pixmap XCreateBitmapFromData(Display* a_0,Drawable a_1,const char* a_2,unsigned int a_3,unsigned int a_4){
struct {Display* a_0;Drawable a_1;const char* a_2;unsigned int a_3;unsigned int a_4;Pixmap rv;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;args.a_4 = a_4;
fexthunks_libX11_XCreateBitmapFromData(&args);
return args.rv;
}

MAKE_THUNK(libX11, XCreatePixmap)

Pixmap XCreatePixmap(Display* a_0,Drawable a_1,unsigned int a_2,unsigned int a_3,unsigned int a_4){
struct {Display* a_0;Drawable a_1;unsigned int a_2;unsigned int a_3;unsigned int a_4;Pixmap rv;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;args.a_4 = a_4;
fexthunks_libX11_XCreatePixmap(&args);
return args.rv;
}

MAKE_THUNK(libX11, XDestroyIC)

void XDestroyIC(XIC a_0){
struct {XIC a_0;} args;
args.a_0 = a_0;
fexthunks_libX11_XDestroyIC(&args);
}

MAKE_THUNK(libX11, XFreeEventData)

void XFreeEventData(Display* a_0,XGenericEventCookie* a_1){
struct {Display* a_0;XGenericEventCookie* a_1;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libX11_XFreeEventData(&args);
}

MAKE_THUNK(libX11, XLockDisplay)

void XLockDisplay(Display* a_0){
struct {Display* a_0;} args;
args.a_0 = a_0;
fexthunks_libX11_XLockDisplay(&args);
}

MAKE_THUNK(libX11, XSetICFocus)

void XSetICFocus(XIC a_0){
struct {XIC a_0;} args;
args.a_0 = a_0;
fexthunks_libX11_XSetICFocus(&args);
}

MAKE_THUNK(libX11, XSetWMNormalHints)

void XSetWMNormalHints(Display* a_0,Window a_1,XSizeHints* a_2){
struct {Display* a_0;Window a_1;XSizeHints* a_2;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;
fexthunks_libX11_XSetWMNormalHints(&args);
}

MAKE_THUNK(libX11, XUnlockDisplay)

void XUnlockDisplay(Display* a_0){
struct {Display* a_0;} args;
args.a_0 = a_0;
fexthunks_libX11_XUnlockDisplay(&args);
}

MAKE_THUNK(libX11, Xutf8SetWMProperties)

void Xutf8SetWMProperties(Display* a_0,Window a_1,const char* a_2,const char* a_3,char** a_4,int a_5,XSizeHints* a_6,XWMHints* a_7,XClassHint* a_8){
struct {Display* a_0;Window a_1;const char* a_2;const char* a_3;char** a_4;int a_5;XSizeHints* a_6;XWMHints* a_7;XClassHint* a_8;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;args.a_4 = a_4;args.a_5 = a_5;args.a_6 = a_6;args.a_7 = a_7;args.a_8 = a_8;
fexthunks_libX11_Xutf8SetWMProperties(&args);
}

MAKE_THUNK(libX11, XLoadQueryFont)

XFontStruct* XLoadQueryFont(Display* a_0,const char* a_1){
struct {Display* a_0;const char* a_1;XFontStruct* rv;} args;
args.a_0 = a_0;args.a_1 = a_1;
fexthunks_libX11_XLoadQueryFont(&args);
return args.rv;
}

MAKE_THUNK(libX11, XCreateGC)

_XGC* XCreateGC(Display* a_0,Drawable a_1,long unsigned int a_2,XGCValues* a_3){
struct {Display* a_0;Drawable a_1;long unsigned int a_2;XGCValues* a_3;_XGC* rv;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;
fexthunks_libX11_XCreateGC(&args);
return args.rv;
}

MAKE_THUNK(libX11, XGetImage)

XImage* XGetImage(Display* a_0,Drawable a_1,int a_2,int a_3,unsigned int a_4,unsigned int a_5,long unsigned int a_6,int a_7){
struct {Display* a_0;Drawable a_1;int a_2;int a_3;unsigned int a_4;unsigned int a_5;long unsigned int a_6;int a_7;XImage* rv;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;args.a_4 = a_4;args.a_5 = a_5;args.a_6 = a_6;args.a_7 = a_7;
fexthunks_libX11_XGetImage(&args);
return args.rv;
}

MAKE_THUNK(libX11, XOpenIM)

_XIM* XOpenIM(Display* a_0,_XrmHashBucketRec* a_1,char* a_2,char* a_3){
struct {Display* a_0;_XrmHashBucketRec* a_1;char* a_2;char* a_3;_XIM* rv;} args;
args.a_0 = a_0;args.a_1 = a_1;args.a_2 = a_2;args.a_3 = a_3;
fexthunks_libX11_XOpenIM(&args);
return args.rv;
}

}
