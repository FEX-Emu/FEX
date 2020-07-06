// initializers
void* fexthunks_impl_libGL_so;
typedef XVisualInfo * fexthunks_type_libGL_glXChooseVisual(Display * a_0,int a_1,int * a_2);
fexthunks_type_libGL_glXChooseVisual *fexthunks_impl_libGL_glXChooseVisual;
typedef GLXContext fexthunks_type_libGL_glXCreateContext(Display * a_0,XVisualInfo * a_1,GLXContext a_2,Bool a_3);
fexthunks_type_libGL_glXCreateContext *fexthunks_impl_libGL_glXCreateContext;
typedef void fexthunks_type_libGL_glXDestroyContext(Display* a_0,GLXContext a_1);
fexthunks_type_libGL_glXDestroyContext *fexthunks_impl_libGL_glXDestroyContext;
typedef Bool fexthunks_type_libGL_glXMakeCurrent(Display * a_0,GLXDrawable a_1,GLXContext a_2);
fexthunks_type_libGL_glXMakeCurrent *fexthunks_impl_libGL_glXMakeCurrent;
typedef void fexthunks_type_libGL_glXCopyContext(Display * a_0,GLXContext a_1,GLXContext a_2,long unsigned int a_3);
fexthunks_type_libGL_glXCopyContext *fexthunks_impl_libGL_glXCopyContext;
typedef void fexthunks_type_libGL_glXSwapBuffers(Display* a_0,GLXDrawable a_1);
fexthunks_type_libGL_glXSwapBuffers *fexthunks_impl_libGL_glXSwapBuffers;
typedef GLXPixmap fexthunks_type_libGL_glXCreateGLXPixmap(Display * a_0,XVisualInfo * a_1,Pixmap a_2);
fexthunks_type_libGL_glXCreateGLXPixmap *fexthunks_impl_libGL_glXCreateGLXPixmap;
typedef void fexthunks_type_libGL_glXDestroyGLXPixmap(Display * a_0,GLXPixmap a_1);
fexthunks_type_libGL_glXDestroyGLXPixmap *fexthunks_impl_libGL_glXDestroyGLXPixmap;
typedef int fexthunks_type_libGL_glXQueryExtension(Display* a_0,int* a_1,int* a_2);
fexthunks_type_libGL_glXQueryExtension *fexthunks_impl_libGL_glXQueryExtension;
typedef Bool fexthunks_type_libGL_glXQueryVersion(Display * a_0,int * a_1,int * a_2);
fexthunks_type_libGL_glXQueryVersion *fexthunks_impl_libGL_glXQueryVersion;
typedef Bool fexthunks_type_libGL_glXIsDirect(Display * a_0,GLXContext a_1);
fexthunks_type_libGL_glXIsDirect *fexthunks_impl_libGL_glXIsDirect;
typedef int fexthunks_type_libGL_glXGetConfig(Display * a_0,XVisualInfo * a_1,int a_2,int * a_3);
fexthunks_type_libGL_glXGetConfig *fexthunks_impl_libGL_glXGetConfig;
typedef __GLXcontextRec* fexthunks_type_libGL_glXGetCurrentContext();
fexthunks_type_libGL_glXGetCurrentContext *fexthunks_impl_libGL_glXGetCurrentContext;
typedef GLXDrawable fexthunks_type_libGL_glXGetCurrentDrawable();
fexthunks_type_libGL_glXGetCurrentDrawable *fexthunks_impl_libGL_glXGetCurrentDrawable;
typedef void fexthunks_type_libGL_glXWaitGL();
fexthunks_type_libGL_glXWaitGL *fexthunks_impl_libGL_glXWaitGL;
typedef void fexthunks_type_libGL_glXWaitX();
fexthunks_type_libGL_glXWaitX *fexthunks_impl_libGL_glXWaitX;
typedef void fexthunks_type_libGL_glXUseXFont(Font a_0,int a_1,int a_2,int a_3);
fexthunks_type_libGL_glXUseXFont *fexthunks_impl_libGL_glXUseXFont;
typedef const char* fexthunks_type_libGL_glXQueryExtensionsString(Display* a_0,int a_1);
fexthunks_type_libGL_glXQueryExtensionsString *fexthunks_impl_libGL_glXQueryExtensionsString;
typedef const char * fexthunks_type_libGL_glXQueryServerString(Display * a_0,int a_1,int a_2);
fexthunks_type_libGL_glXQueryServerString *fexthunks_impl_libGL_glXQueryServerString;
typedef const char * fexthunks_type_libGL_glXGetClientString(Display * a_0,int a_1);
fexthunks_type_libGL_glXGetClientString *fexthunks_impl_libGL_glXGetClientString;
typedef GLXContext fexthunks_type_libGL_glXCreateContextAttribsARB(Display * a_0,GLXFBConfig a_1,GLXContext a_2,Bool a_3,const int * a_4);
fexthunks_type_libGL_glXCreateContextAttribsARB *fexthunks_impl_libGL_glXCreateContextAttribsARB;
typedef __GLXFBConfigRec** fexthunks_type_libGL_glXChooseFBConfig(Display* a_0,int a_1,const int* a_2,int* a_3);
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
typedef const GLubyte* fexthunks_type_libGL_glGetString(GLenum a_0);
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
typedef GLboolean fexthunks_type_libGL_glIsEnabled(GLenum a_0);
fexthunks_type_libGL_glIsEnabled *fexthunks_impl_libGL_glIsEnabled;
typedef GLenum fexthunks_type_libGL_glGetError();
fexthunks_type_libGL_glGetError *fexthunks_impl_libGL_glGetError;
typedef __GLXcontextRec* fexthunks_type_libGL_glXCreateNewContext(Display* a_0,GLXFBConfig a_1,int a_2,GLXContext a_3,int a_4);
fexthunks_type_libGL_glXCreateNewContext *fexthunks_impl_libGL_glXCreateNewContext;
typedef GLXWindow fexthunks_type_libGL_glXCreateWindow(Display* a_0,GLXFBConfig a_1,Window a_2,const int* a_3);
fexthunks_type_libGL_glXCreateWindow *fexthunks_impl_libGL_glXCreateWindow;
typedef int fexthunks_type_libGL_glXMakeContextCurrent(Display* a_0,GLXDrawable a_1,GLXDrawable a_2,GLXContext a_3);
fexthunks_type_libGL_glXMakeContextCurrent *fexthunks_impl_libGL_glXMakeContextCurrent;
typedef void fexthunks_type_libGL_glActiveTexture(GLenum a_0);
fexthunks_type_libGL_glActiveTexture *fexthunks_impl_libGL_glActiveTexture;
typedef void fexthunks_type_libGL_glBindTexture(GLenum a_0,GLuint a_1);
fexthunks_type_libGL_glBindTexture *fexthunks_impl_libGL_glBindTexture;
typedef void fexthunks_type_libGL_glBlendColor(GLclampf a_0,GLclampf a_1,GLclampf a_2,GLclampf a_3);
fexthunks_type_libGL_glBlendColor *fexthunks_impl_libGL_glBlendColor;
typedef void fexthunks_type_libGL_glBlendEquation(GLenum a_0);
fexthunks_type_libGL_glBlendEquation *fexthunks_impl_libGL_glBlendEquation;
typedef void fexthunks_type_libGL_glBlendFunc(GLenum a_0,GLenum a_1);
fexthunks_type_libGL_glBlendFunc *fexthunks_impl_libGL_glBlendFunc;
typedef void fexthunks_type_libGL_glClearDepth(GLclampd a_0);
fexthunks_type_libGL_glClearDepth *fexthunks_impl_libGL_glClearDepth;
typedef void fexthunks_type_libGL_glClearStencil(GLint a_0);
fexthunks_type_libGL_glClearStencil *fexthunks_impl_libGL_glClearStencil;
typedef void fexthunks_type_libGL_glColorMask(GLboolean a_0,GLboolean a_1,GLboolean a_2,GLboolean a_3);
fexthunks_type_libGL_glColorMask *fexthunks_impl_libGL_glColorMask;
typedef void fexthunks_type_libGL_glCompressedTexImage2D(GLenum a_0,GLint a_1,GLenum a_2,GLsizei a_3,GLsizei a_4,GLint a_5,GLsizei a_6,const GLvoid* a_7);
fexthunks_type_libGL_glCompressedTexImage2D *fexthunks_impl_libGL_glCompressedTexImage2D;
typedef void fexthunks_type_libGL_glCompressedTexSubImage2D(GLenum a_0,GLint a_1,GLint a_2,GLint a_3,GLsizei a_4,GLsizei a_5,GLenum a_6,GLsizei a_7,const GLvoid* a_8);
fexthunks_type_libGL_glCompressedTexSubImage2D *fexthunks_impl_libGL_glCompressedTexSubImage2D;
typedef void fexthunks_type_libGL_glCopyTexImage2D(GLenum a_0,GLint a_1,GLenum a_2,GLint a_3,GLint a_4,GLsizei a_5,GLsizei a_6,GLint a_7);
fexthunks_type_libGL_glCopyTexImage2D *fexthunks_impl_libGL_glCopyTexImage2D;
typedef void fexthunks_type_libGL_glCopyTexSubImage2D(GLenum a_0,GLint a_1,GLint a_2,GLint a_3,GLint a_4,GLint a_5,GLsizei a_6,GLsizei a_7);
fexthunks_type_libGL_glCopyTexSubImage2D *fexthunks_impl_libGL_glCopyTexSubImage2D;
typedef void fexthunks_type_libGL_glCullFace(GLenum a_0);
fexthunks_type_libGL_glCullFace *fexthunks_impl_libGL_glCullFace;
typedef void fexthunks_type_libGL_glDeleteTextures(GLsizei a_0,const GLuint* a_1);
fexthunks_type_libGL_glDeleteTextures *fexthunks_impl_libGL_glDeleteTextures;
typedef void fexthunks_type_libGL_glDepthFunc(GLenum a_0);
fexthunks_type_libGL_glDepthFunc *fexthunks_impl_libGL_glDepthFunc;
typedef void fexthunks_type_libGL_glDepthMask(GLboolean a_0);
fexthunks_type_libGL_glDepthMask *fexthunks_impl_libGL_glDepthMask;
typedef void fexthunks_type_libGL_glDepthRange(GLclampd a_0,GLclampd a_1);
fexthunks_type_libGL_glDepthRange *fexthunks_impl_libGL_glDepthRange;
typedef void fexthunks_type_libGL_glDisable(GLenum a_0);
fexthunks_type_libGL_glDisable *fexthunks_impl_libGL_glDisable;
typedef void fexthunks_type_libGL_glDrawArrays(GLenum a_0,GLint a_1,GLsizei a_2);
fexthunks_type_libGL_glDrawArrays *fexthunks_impl_libGL_glDrawArrays;
typedef void fexthunks_type_libGL_glDrawElements(GLenum a_0,GLsizei a_1,GLenum a_2,const GLvoid* a_3);
fexthunks_type_libGL_glDrawElements *fexthunks_impl_libGL_glDrawElements;
typedef void fexthunks_type_libGL_glFrontFace(GLenum a_0);
fexthunks_type_libGL_glFrontFace *fexthunks_impl_libGL_glFrontFace;
typedef void fexthunks_type_libGL_glGenTextures(GLsizei a_0,GLuint* a_1);
fexthunks_type_libGL_glGenTextures *fexthunks_impl_libGL_glGenTextures;
typedef void fexthunks_type_libGL_glGetFloatv(GLenum a_0,GLfloat* a_1);
fexthunks_type_libGL_glGetFloatv *fexthunks_impl_libGL_glGetFloatv;
typedef void fexthunks_type_libGL_glGetIntegerv(GLenum a_0,GLint* a_1);
fexthunks_type_libGL_glGetIntegerv *fexthunks_impl_libGL_glGetIntegerv;
typedef void fexthunks_type_libGL_glGetTexImage(GLenum a_0,GLint a_1,GLenum a_2,GLenum a_3,GLvoid* a_4);
fexthunks_type_libGL_glGetTexImage *fexthunks_impl_libGL_glGetTexImage;
typedef void fexthunks_type_libGL_glGetTexLevelParameterfv(GLenum a_0,GLint a_1,GLenum a_2,GLfloat* a_3);
fexthunks_type_libGL_glGetTexLevelParameterfv *fexthunks_impl_libGL_glGetTexLevelParameterfv;
typedef void fexthunks_type_libGL_glPixelStorei(GLenum a_0,GLint a_1);
fexthunks_type_libGL_glPixelStorei *fexthunks_impl_libGL_glPixelStorei;
typedef void fexthunks_type_libGL_glRasterPos2i(GLint a_0,GLint a_1);
fexthunks_type_libGL_glRasterPos2i *fexthunks_impl_libGL_glRasterPos2i;
typedef void fexthunks_type_libGL_glReadPixels(GLint a_0,GLint a_1,GLsizei a_2,GLsizei a_3,GLenum a_4,GLenum a_5,GLvoid* a_6);
fexthunks_type_libGL_glReadPixels *fexthunks_impl_libGL_glReadPixels;
typedef void fexthunks_type_libGL_glScissor(GLint a_0,GLint a_1,GLsizei a_2,GLsizei a_3);
fexthunks_type_libGL_glScissor *fexthunks_impl_libGL_glScissor;
typedef void fexthunks_type_libGL_glStencilFunc(GLenum a_0,GLint a_1,GLuint a_2);
fexthunks_type_libGL_glStencilFunc *fexthunks_impl_libGL_glStencilFunc;
typedef void fexthunks_type_libGL_glStencilOp(GLenum a_0,GLenum a_1,GLenum a_2);
fexthunks_type_libGL_glStencilOp *fexthunks_impl_libGL_glStencilOp;
typedef void fexthunks_type_libGL_glTexImage2D(GLenum a_0,GLint a_1,GLint a_2,GLsizei a_3,GLsizei a_4,GLint a_5,GLenum a_6,GLenum a_7,const GLvoid* a_8);
fexthunks_type_libGL_glTexImage2D *fexthunks_impl_libGL_glTexImage2D;
typedef void fexthunks_type_libGL_glTexParameteri(GLenum a_0,GLenum a_1,GLint a_2);
fexthunks_type_libGL_glTexParameteri *fexthunks_impl_libGL_glTexParameteri;
typedef void fexthunks_type_libGL_glTexSubImage2D(GLenum a_0,GLint a_1,GLint a_2,GLint a_3,GLsizei a_4,GLsizei a_5,GLenum a_6,GLenum a_7,const GLvoid* a_8);
fexthunks_type_libGL_glTexSubImage2D *fexthunks_impl_libGL_glTexSubImage2D;
typedef void fexthunks_type_libGL_glXDestroyWindow(Display* a_0,GLXWindow a_1);
fexthunks_type_libGL_glXDestroyWindow *fexthunks_impl_libGL_glXDestroyWindow;
typedef XVisualInfo* fexthunks_type_libGL_glXGetVisualFromFBConfig(Display* a_0,GLXFBConfig a_1);
fexthunks_type_libGL_glXGetVisualFromFBConfig *fexthunks_impl_libGL_glXGetVisualFromFBConfig;
typedef void fexthunks_type_libGL_glFinish();
fexthunks_type_libGL_glFinish *fexthunks_impl_libGL_glFinish;
typedef void fexthunks_type_libGL_glFlush();
fexthunks_type_libGL_glFlush *fexthunks_impl_libGL_glFlush;
typedef const GLubyte* fexthunks_type_libGL_glGetStringi(GLenum a_0,GLuint a_1);
fexthunks_type_libGL_glGetStringi *fexthunks_impl_libGL_glGetStringi;
typedef GLboolean fexthunks_type_libGL_glIsProgram(GLuint a_0);
fexthunks_type_libGL_glIsProgram *fexthunks_impl_libGL_glIsProgram;
typedef GLboolean fexthunks_type_libGL_glIsShader(GLuint a_0);
fexthunks_type_libGL_glIsShader *fexthunks_impl_libGL_glIsShader;
typedef GLenum fexthunks_type_libGL_glCheckFramebufferStatus(GLenum a_0);
fexthunks_type_libGL_glCheckFramebufferStatus *fexthunks_impl_libGL_glCheckFramebufferStatus;
typedef GLenum fexthunks_type_libGL_glCheckNamedFramebufferStatus(GLuint a_0,GLenum a_1);
fexthunks_type_libGL_glCheckNamedFramebufferStatus *fexthunks_impl_libGL_glCheckNamedFramebufferStatus;
typedef GLint fexthunks_type_libGL_glGetUniformLocation(GLuint a_0,const GLchar* a_1);
fexthunks_type_libGL_glGetUniformLocation *fexthunks_impl_libGL_glGetUniformLocation;
typedef GLuint fexthunks_type_libGL_glCreateProgram();
fexthunks_type_libGL_glCreateProgram *fexthunks_impl_libGL_glCreateProgram;
typedef GLuint fexthunks_type_libGL_glCreateShader(GLenum a_0);
fexthunks_type_libGL_glCreateShader *fexthunks_impl_libGL_glCreateShader;
typedef void fexthunks_type_libGL_glAttachShader(GLuint a_0,GLuint a_1);
fexthunks_type_libGL_glAttachShader *fexthunks_impl_libGL_glAttachShader;
typedef void fexthunks_type_libGL_glBindAttribLocation(GLuint a_0,GLuint a_1,const GLchar* a_2);
fexthunks_type_libGL_glBindAttribLocation *fexthunks_impl_libGL_glBindAttribLocation;
typedef void fexthunks_type_libGL_glBindBuffer(GLenum a_0,GLuint a_1);
fexthunks_type_libGL_glBindBuffer *fexthunks_impl_libGL_glBindBuffer;
typedef void fexthunks_type_libGL_glBindFragDataLocation(GLuint a_0,GLuint a_1,const GLchar* a_2);
fexthunks_type_libGL_glBindFragDataLocation *fexthunks_impl_libGL_glBindFragDataLocation;
typedef void fexthunks_type_libGL_glBindFramebuffer(GLenum a_0,GLuint a_1);
fexthunks_type_libGL_glBindFramebuffer *fexthunks_impl_libGL_glBindFramebuffer;
typedef void fexthunks_type_libGL_glBindProgramPipeline(GLuint a_0);
fexthunks_type_libGL_glBindProgramPipeline *fexthunks_impl_libGL_glBindProgramPipeline;
typedef void fexthunks_type_libGL_glBindRenderbuffer(GLenum a_0,GLuint a_1);
fexthunks_type_libGL_glBindRenderbuffer *fexthunks_impl_libGL_glBindRenderbuffer;
typedef void fexthunks_type_libGL_glBindTextureUnit(GLuint a_0,GLuint a_1);
fexthunks_type_libGL_glBindTextureUnit *fexthunks_impl_libGL_glBindTextureUnit;
typedef void fexthunks_type_libGL_glBindVertexArray(GLuint a_0);
fexthunks_type_libGL_glBindVertexArray *fexthunks_impl_libGL_glBindVertexArray;
typedef void fexthunks_type_libGL_glBlendFuncSeparate(GLenum a_0,GLenum a_1,GLenum a_2,GLenum a_3);
fexthunks_type_libGL_glBlendFuncSeparate *fexthunks_impl_libGL_glBlendFuncSeparate;
typedef void fexthunks_type_libGL_glBufferData(GLenum a_0,GLsizeiptr a_1,const void* a_2,GLenum a_3);
fexthunks_type_libGL_glBufferData *fexthunks_impl_libGL_glBufferData;
typedef void fexthunks_type_libGL_glBufferSubData(GLenum a_0,GLintptr a_1,GLsizeiptr a_2,const void* a_3);
fexthunks_type_libGL_glBufferSubData *fexthunks_impl_libGL_glBufferSubData;
typedef void fexthunks_type_libGL_glCompileShader(GLuint a_0);
fexthunks_type_libGL_glCompileShader *fexthunks_impl_libGL_glCompileShader;
typedef void fexthunks_type_libGL_glCompressedTextureSubImage2D(GLuint a_0,GLint a_1,GLint a_2,GLint a_3,GLsizei a_4,GLsizei a_5,GLenum a_6,GLsizei a_7,const void* a_8);
fexthunks_type_libGL_glCompressedTextureSubImage2D *fexthunks_impl_libGL_glCompressedTextureSubImage2D;
typedef void fexthunks_type_libGL_glCopyTextureSubImage2D(GLuint a_0,GLint a_1,GLint a_2,GLint a_3,GLint a_4,GLint a_5,GLsizei a_6,GLsizei a_7);
fexthunks_type_libGL_glCopyTextureSubImage2D *fexthunks_impl_libGL_glCopyTextureSubImage2D;
typedef void fexthunks_type_libGL_glCreateFramebuffers(GLsizei a_0,GLuint* a_1);
fexthunks_type_libGL_glCreateFramebuffers *fexthunks_impl_libGL_glCreateFramebuffers;
typedef void fexthunks_type_libGL_glCreateProgramPipelines(GLsizei a_0,GLuint* a_1);
fexthunks_type_libGL_glCreateProgramPipelines *fexthunks_impl_libGL_glCreateProgramPipelines;
typedef void fexthunks_type_libGL_glCreateRenderbuffers(GLsizei a_0,GLuint* a_1);
fexthunks_type_libGL_glCreateRenderbuffers *fexthunks_impl_libGL_glCreateRenderbuffers;
typedef void fexthunks_type_libGL_glCreateTextures(GLenum a_0,GLsizei a_1,GLuint* a_2);
fexthunks_type_libGL_glCreateTextures *fexthunks_impl_libGL_glCreateTextures;
typedef void fexthunks_type_libGL_glDebugMessageCallback(GLDEBUGPROC a_0,const void* a_1);
fexthunks_type_libGL_glDebugMessageCallback *fexthunks_impl_libGL_glDebugMessageCallback;
typedef void fexthunks_type_libGL_glDebugMessageControl(GLenum a_0,GLenum a_1,GLenum a_2,GLsizei a_3,const GLuint* a_4,GLboolean a_5);
fexthunks_type_libGL_glDebugMessageControl *fexthunks_impl_libGL_glDebugMessageControl;
typedef void fexthunks_type_libGL_glDebugMessageInsert(GLenum a_0,GLenum a_1,GLuint a_2,GLenum a_3,GLsizei a_4,const GLchar* a_5);
fexthunks_type_libGL_glDebugMessageInsert *fexthunks_impl_libGL_glDebugMessageInsert;
typedef void fexthunks_type_libGL_glDeleteBuffers(GLsizei a_0,const GLuint* a_1);
fexthunks_type_libGL_glDeleteBuffers *fexthunks_impl_libGL_glDeleteBuffers;
typedef void fexthunks_type_libGL_glDeleteFramebuffers(GLsizei a_0,const GLuint* a_1);
fexthunks_type_libGL_glDeleteFramebuffers *fexthunks_impl_libGL_glDeleteFramebuffers;
typedef void fexthunks_type_libGL_glDeleteProgram(GLuint a_0);
fexthunks_type_libGL_glDeleteProgram *fexthunks_impl_libGL_glDeleteProgram;
typedef void fexthunks_type_libGL_glDeleteProgramPipelines(GLsizei a_0,const GLuint* a_1);
fexthunks_type_libGL_glDeleteProgramPipelines *fexthunks_impl_libGL_glDeleteProgramPipelines;
typedef void fexthunks_type_libGL_glDeleteRenderbuffers(GLsizei a_0,const GLuint* a_1);
fexthunks_type_libGL_glDeleteRenderbuffers *fexthunks_impl_libGL_glDeleteRenderbuffers;
typedef void fexthunks_type_libGL_glDeleteShader(GLuint a_0);
fexthunks_type_libGL_glDeleteShader *fexthunks_impl_libGL_glDeleteShader;
typedef void fexthunks_type_libGL_glDeleteVertexArrays(GLsizei a_0,const GLuint* a_1);
fexthunks_type_libGL_glDeleteVertexArrays *fexthunks_impl_libGL_glDeleteVertexArrays;
typedef void fexthunks_type_libGL_glDetachShader(GLuint a_0,GLuint a_1);
fexthunks_type_libGL_glDetachShader *fexthunks_impl_libGL_glDetachShader;
typedef void fexthunks_type_libGL_glDisableVertexAttribArray(GLuint a_0);
fexthunks_type_libGL_glDisableVertexAttribArray *fexthunks_impl_libGL_glDisableVertexAttribArray;
typedef void fexthunks_type_libGL_glEnableVertexAttribArray(GLuint a_0);
fexthunks_type_libGL_glEnableVertexAttribArray *fexthunks_impl_libGL_glEnableVertexAttribArray;
typedef void fexthunks_type_libGL_glFramebufferRenderbuffer(GLenum a_0,GLenum a_1,GLenum a_2,GLuint a_3);
fexthunks_type_libGL_glFramebufferRenderbuffer *fexthunks_impl_libGL_glFramebufferRenderbuffer;
typedef void fexthunks_type_libGL_glFramebufferTexture2D(GLenum a_0,GLenum a_1,GLenum a_2,GLuint a_3,GLint a_4);
fexthunks_type_libGL_glFramebufferTexture2D *fexthunks_impl_libGL_glFramebufferTexture2D;
typedef void fexthunks_type_libGL_glGenBuffers(GLsizei a_0,GLuint* a_1);
fexthunks_type_libGL_glGenBuffers *fexthunks_impl_libGL_glGenBuffers;
typedef void fexthunks_type_libGL_glGenerateMipmap(GLenum a_0);
fexthunks_type_libGL_glGenerateMipmap *fexthunks_impl_libGL_glGenerateMipmap;
typedef void fexthunks_type_libGL_glGenerateTextureMipmap(GLuint a_0);
fexthunks_type_libGL_glGenerateTextureMipmap *fexthunks_impl_libGL_glGenerateTextureMipmap;
typedef void fexthunks_type_libGL_glGenFramebuffers(GLsizei a_0,GLuint* a_1);
fexthunks_type_libGL_glGenFramebuffers *fexthunks_impl_libGL_glGenFramebuffers;
typedef void fexthunks_type_libGL_glGenProgramPipelines(GLsizei a_0,GLuint* a_1);
fexthunks_type_libGL_glGenProgramPipelines *fexthunks_impl_libGL_glGenProgramPipelines;
typedef void fexthunks_type_libGL_glGenRenderbuffers(GLsizei a_0,GLuint* a_1);
fexthunks_type_libGL_glGenRenderbuffers *fexthunks_impl_libGL_glGenRenderbuffers;
typedef void fexthunks_type_libGL_glGenVertexArrays(GLsizei a_0,GLuint* a_1);
fexthunks_type_libGL_glGenVertexArrays *fexthunks_impl_libGL_glGenVertexArrays;
typedef void fexthunks_type_libGL_glGetFramebufferAttachmentParameteriv(GLenum a_0,GLenum a_1,GLenum a_2,GLint* a_3);
fexthunks_type_libGL_glGetFramebufferAttachmentParameteriv *fexthunks_impl_libGL_glGetFramebufferAttachmentParameteriv;
typedef void fexthunks_type_libGL_glGetNamedBufferSubData(GLuint a_0,GLintptr a_1,GLsizeiptr a_2,void* a_3);
fexthunks_type_libGL_glGetNamedBufferSubData *fexthunks_impl_libGL_glGetNamedBufferSubData;
typedef void fexthunks_type_libGL_glGetProgramBinary(GLuint a_0,GLsizei a_1,GLsizei* a_2,GLenum* a_3,void* a_4);
fexthunks_type_libGL_glGetProgramBinary *fexthunks_impl_libGL_glGetProgramBinary;
typedef void fexthunks_type_libGL_glGetProgramInfoLog(GLuint a_0,GLsizei a_1,GLsizei* a_2,GLchar* a_3);
fexthunks_type_libGL_glGetProgramInfoLog *fexthunks_impl_libGL_glGetProgramInfoLog;
typedef void fexthunks_type_libGL_glGetProgramiv(GLuint a_0,GLenum a_1,GLint* a_2);
fexthunks_type_libGL_glGetProgramiv *fexthunks_impl_libGL_glGetProgramiv;
typedef void fexthunks_type_libGL_glGetProgramPipelineInfoLog(GLuint a_0,GLsizei a_1,GLsizei* a_2,GLchar* a_3);
fexthunks_type_libGL_glGetProgramPipelineInfoLog *fexthunks_impl_libGL_glGetProgramPipelineInfoLog;
typedef void fexthunks_type_libGL_glGetProgramPipelineiv(GLuint a_0,GLenum a_1,GLint* a_2);
fexthunks_type_libGL_glGetProgramPipelineiv *fexthunks_impl_libGL_glGetProgramPipelineiv;
typedef void fexthunks_type_libGL_glGetShaderInfoLog(GLuint a_0,GLsizei a_1,GLsizei* a_2,GLchar* a_3);
fexthunks_type_libGL_glGetShaderInfoLog *fexthunks_impl_libGL_glGetShaderInfoLog;
typedef void fexthunks_type_libGL_glGetShaderiv(GLuint a_0,GLenum a_1,GLint* a_2);
fexthunks_type_libGL_glGetShaderiv *fexthunks_impl_libGL_glGetShaderiv;
typedef void fexthunks_type_libGL_glGetShaderSource(GLuint a_0,GLsizei a_1,GLsizei* a_2,GLchar* a_3);
fexthunks_type_libGL_glGetShaderSource *fexthunks_impl_libGL_glGetShaderSource;
typedef void fexthunks_type_libGL_glGetTextureImage(GLuint a_0,GLint a_1,GLenum a_2,GLenum a_3,GLsizei a_4,void* a_5);
fexthunks_type_libGL_glGetTextureImage *fexthunks_impl_libGL_glGetTextureImage;
typedef void fexthunks_type_libGL_glInvalidateFramebuffer(GLenum a_0,GLsizei a_1,const GLenum* a_2);
fexthunks_type_libGL_glInvalidateFramebuffer *fexthunks_impl_libGL_glInvalidateFramebuffer;
typedef void fexthunks_type_libGL_glInvalidateNamedFramebufferData(GLuint a_0,GLsizei a_1,const GLenum* a_2);
fexthunks_type_libGL_glInvalidateNamedFramebufferData *fexthunks_impl_libGL_glInvalidateNamedFramebufferData;
typedef void fexthunks_type_libGL_glLinkProgram(GLuint a_0);
fexthunks_type_libGL_glLinkProgram *fexthunks_impl_libGL_glLinkProgram;
typedef void fexthunks_type_libGL_glNamedFramebufferRenderbuffer(GLuint a_0,GLenum a_1,GLenum a_2,GLuint a_3);
fexthunks_type_libGL_glNamedFramebufferRenderbuffer *fexthunks_impl_libGL_glNamedFramebufferRenderbuffer;
typedef void fexthunks_type_libGL_glNamedFramebufferTexture(GLuint a_0,GLenum a_1,GLuint a_2,GLint a_3);
fexthunks_type_libGL_glNamedFramebufferTexture *fexthunks_impl_libGL_glNamedFramebufferTexture;
typedef void fexthunks_type_libGL_glNamedRenderbufferStorage(GLuint a_0,GLenum a_1,GLsizei a_2,GLsizei a_3);
fexthunks_type_libGL_glNamedRenderbufferStorage *fexthunks_impl_libGL_glNamedRenderbufferStorage;
typedef void fexthunks_type_libGL_glProgramBinary(GLuint a_0,GLenum a_1,const void* a_2,GLsizei a_3);
fexthunks_type_libGL_glProgramBinary *fexthunks_impl_libGL_glProgramBinary;
typedef void fexthunks_type_libGL_glProgramParameteri(GLuint a_0,GLenum a_1,GLint a_2);
fexthunks_type_libGL_glProgramParameteri *fexthunks_impl_libGL_glProgramParameteri;
typedef void fexthunks_type_libGL_glProgramUniform1f(GLuint a_0,GLint a_1,GLfloat a_2);
fexthunks_type_libGL_glProgramUniform1f *fexthunks_impl_libGL_glProgramUniform1f;
typedef void fexthunks_type_libGL_glProgramUniform1i(GLuint a_0,GLint a_1,GLint a_2);
fexthunks_type_libGL_glProgramUniform1i *fexthunks_impl_libGL_glProgramUniform1i;
typedef void fexthunks_type_libGL_glProgramUniform2fv(GLuint a_0,GLint a_1,GLsizei a_2,const GLfloat* a_3);
fexthunks_type_libGL_glProgramUniform2fv *fexthunks_impl_libGL_glProgramUniform2fv;
typedef void fexthunks_type_libGL_glProgramUniform3fv(GLuint a_0,GLint a_1,GLsizei a_2,const GLfloat* a_3);
fexthunks_type_libGL_glProgramUniform3fv *fexthunks_impl_libGL_glProgramUniform3fv;
typedef void fexthunks_type_libGL_glProgramUniform4fv(GLuint a_0,GLint a_1,GLsizei a_2,const GLfloat* a_3);
fexthunks_type_libGL_glProgramUniform4fv *fexthunks_impl_libGL_glProgramUniform4fv;
typedef void fexthunks_type_libGL_glProgramUniformMatrix4fv(GLuint a_0,GLint a_1,GLsizei a_2,GLboolean a_3,const GLfloat* a_4);
fexthunks_type_libGL_glProgramUniformMatrix4fv *fexthunks_impl_libGL_glProgramUniformMatrix4fv;
typedef void fexthunks_type_libGL_glRenderbufferStorage(GLenum a_0,GLenum a_1,GLsizei a_2,GLsizei a_3);
fexthunks_type_libGL_glRenderbufferStorage *fexthunks_impl_libGL_glRenderbufferStorage;
typedef void fexthunks_type_libGL_glShaderSource(GLuint a_0,GLsizei a_1,const GLchar* const* a_2,const GLint* a_3);
fexthunks_type_libGL_glShaderSource *fexthunks_impl_libGL_glShaderSource;
typedef void fexthunks_type_libGL_glTexStorage2D(GLenum a_0,GLsizei a_1,GLenum a_2,GLsizei a_3,GLsizei a_4);
fexthunks_type_libGL_glTexStorage2D *fexthunks_impl_libGL_glTexStorage2D;
typedef void fexthunks_type_libGL_glTextureParameteri(GLuint a_0,GLenum a_1,GLint a_2);
fexthunks_type_libGL_glTextureParameteri *fexthunks_impl_libGL_glTextureParameteri;
typedef void fexthunks_type_libGL_glTextureStorage2D(GLuint a_0,GLsizei a_1,GLenum a_2,GLsizei a_3,GLsizei a_4);
fexthunks_type_libGL_glTextureStorage2D *fexthunks_impl_libGL_glTextureStorage2D;
typedef void fexthunks_type_libGL_glTextureSubImage2D(GLuint a_0,GLint a_1,GLint a_2,GLint a_3,GLsizei a_4,GLsizei a_5,GLenum a_6,GLenum a_7,const void* a_8);
fexthunks_type_libGL_glTextureSubImage2D *fexthunks_impl_libGL_glTextureSubImage2D;
typedef void fexthunks_type_libGL_glUniform1f(GLint a_0,GLfloat a_1);
fexthunks_type_libGL_glUniform1f *fexthunks_impl_libGL_glUniform1f;
typedef void fexthunks_type_libGL_glUniform1fv(GLint a_0,GLsizei a_1,const GLfloat* a_2);
fexthunks_type_libGL_glUniform1fv *fexthunks_impl_libGL_glUniform1fv;
typedef void fexthunks_type_libGL_glUniform1i(GLint a_0,GLint a_1);
fexthunks_type_libGL_glUniform1i *fexthunks_impl_libGL_glUniform1i;
typedef void fexthunks_type_libGL_glUniform1iv(GLint a_0,GLsizei a_1,const GLint* a_2);
fexthunks_type_libGL_glUniform1iv *fexthunks_impl_libGL_glUniform1iv;
typedef void fexthunks_type_libGL_glUniform2f(GLint a_0,GLfloat a_1,GLfloat a_2);
fexthunks_type_libGL_glUniform2f *fexthunks_impl_libGL_glUniform2f;
typedef void fexthunks_type_libGL_glUniform2fv(GLint a_0,GLsizei a_1,const GLfloat* a_2);
fexthunks_type_libGL_glUniform2fv *fexthunks_impl_libGL_glUniform2fv;
typedef void fexthunks_type_libGL_glUniform2i(GLint a_0,GLint a_1,GLint a_2);
fexthunks_type_libGL_glUniform2i *fexthunks_impl_libGL_glUniform2i;
typedef void fexthunks_type_libGL_glUniform2iv(GLint a_0,GLsizei a_1,const GLint* a_2);
fexthunks_type_libGL_glUniform2iv *fexthunks_impl_libGL_glUniform2iv;
typedef void fexthunks_type_libGL_glUniform3f(GLint a_0,GLfloat a_1,GLfloat a_2,GLfloat a_3);
fexthunks_type_libGL_glUniform3f *fexthunks_impl_libGL_glUniform3f;
typedef void fexthunks_type_libGL_glUniform3fv(GLint a_0,GLsizei a_1,const GLfloat* a_2);
fexthunks_type_libGL_glUniform3fv *fexthunks_impl_libGL_glUniform3fv;
typedef void fexthunks_type_libGL_glUniform3i(GLint a_0,GLint a_1,GLint a_2,GLint a_3);
fexthunks_type_libGL_glUniform3i *fexthunks_impl_libGL_glUniform3i;
typedef void fexthunks_type_libGL_glUniform3iv(GLint a_0,GLsizei a_1,const GLint* a_2);
fexthunks_type_libGL_glUniform3iv *fexthunks_impl_libGL_glUniform3iv;
typedef void fexthunks_type_libGL_glUniform4f(GLint a_0,GLfloat a_1,GLfloat a_2,GLfloat a_3,GLfloat a_4);
fexthunks_type_libGL_glUniform4f *fexthunks_impl_libGL_glUniform4f;
typedef void fexthunks_type_libGL_glUniform4fv(GLint a_0,GLsizei a_1,const GLfloat* a_2);
fexthunks_type_libGL_glUniform4fv *fexthunks_impl_libGL_glUniform4fv;
typedef void fexthunks_type_libGL_glUniform4i(GLint a_0,GLint a_1,GLint a_2,GLint a_3,GLint a_4);
fexthunks_type_libGL_glUniform4i *fexthunks_impl_libGL_glUniform4i;
typedef void fexthunks_type_libGL_glUniform4iv(GLint a_0,GLsizei a_1,const GLint* a_2);
fexthunks_type_libGL_glUniform4iv *fexthunks_impl_libGL_glUniform4iv;
typedef void fexthunks_type_libGL_glUniformMatrix2fv(GLint a_0,GLsizei a_1,GLboolean a_2,const GLfloat* a_3);
fexthunks_type_libGL_glUniformMatrix2fv *fexthunks_impl_libGL_glUniformMatrix2fv;
typedef void fexthunks_type_libGL_glUniformMatrix3fv(GLint a_0,GLsizei a_1,GLboolean a_2,const GLfloat* a_3);
fexthunks_type_libGL_glUniformMatrix3fv *fexthunks_impl_libGL_glUniformMatrix3fv;
typedef void fexthunks_type_libGL_glUniformMatrix4fv(GLint a_0,GLsizei a_1,GLboolean a_2,const GLfloat* a_3);
fexthunks_type_libGL_glUniformMatrix4fv *fexthunks_impl_libGL_glUniformMatrix4fv;
typedef void fexthunks_type_libGL_glUseProgram(GLuint a_0);
fexthunks_type_libGL_glUseProgram *fexthunks_impl_libGL_glUseProgram;
typedef void fexthunks_type_libGL_glUseProgramStages(GLuint a_0,GLbitfield a_1,GLuint a_2);
fexthunks_type_libGL_glUseProgramStages *fexthunks_impl_libGL_glUseProgramStages;
typedef void fexthunks_type_libGL_glValidateProgram(GLuint a_0);
fexthunks_type_libGL_glValidateProgram *fexthunks_impl_libGL_glValidateProgram;
typedef void fexthunks_type_libGL_glValidateProgramPipeline(GLuint a_0);
fexthunks_type_libGL_glValidateProgramPipeline *fexthunks_impl_libGL_glValidateProgramPipeline;
typedef void fexthunks_type_libGL_glVertexAttribIPointer(GLuint a_0,GLint a_1,GLenum a_2,GLsizei a_3,const void* a_4);
fexthunks_type_libGL_glVertexAttribIPointer *fexthunks_impl_libGL_glVertexAttribIPointer;
typedef void fexthunks_type_libGL_glVertexAttribPointer(GLuint a_0,GLint a_1,GLenum a_2,GLboolean a_3,GLsizei a_4,const void* a_5);
fexthunks_type_libGL_glVertexAttribPointer *fexthunks_impl_libGL_glVertexAttribPointer;
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
(void*&)fexthunks_impl_libGL_glIsEnabled = dlsym(fexthunks_impl_libGL_so, "glIsEnabled");
(void*&)fexthunks_impl_libGL_glGetError = dlsym(fexthunks_impl_libGL_so, "glGetError");
(void*&)fexthunks_impl_libGL_glXCreateNewContext = dlsym(fexthunks_impl_libGL_so, "glXCreateNewContext");
(void*&)fexthunks_impl_libGL_glXCreateWindow = dlsym(fexthunks_impl_libGL_so, "glXCreateWindow");
(void*&)fexthunks_impl_libGL_glXMakeContextCurrent = dlsym(fexthunks_impl_libGL_so, "glXMakeContextCurrent");
(void*&)fexthunks_impl_libGL_glActiveTexture = dlsym(fexthunks_impl_libGL_so, "glActiveTexture");
(void*&)fexthunks_impl_libGL_glBindTexture = dlsym(fexthunks_impl_libGL_so, "glBindTexture");
(void*&)fexthunks_impl_libGL_glBlendColor = dlsym(fexthunks_impl_libGL_so, "glBlendColor");
(void*&)fexthunks_impl_libGL_glBlendEquation = dlsym(fexthunks_impl_libGL_so, "glBlendEquation");
(void*&)fexthunks_impl_libGL_glBlendFunc = dlsym(fexthunks_impl_libGL_so, "glBlendFunc");
(void*&)fexthunks_impl_libGL_glClearDepth = dlsym(fexthunks_impl_libGL_so, "glClearDepth");
(void*&)fexthunks_impl_libGL_glClearStencil = dlsym(fexthunks_impl_libGL_so, "glClearStencil");
(void*&)fexthunks_impl_libGL_glColorMask = dlsym(fexthunks_impl_libGL_so, "glColorMask");
(void*&)fexthunks_impl_libGL_glCompressedTexImage2D = dlsym(fexthunks_impl_libGL_so, "glCompressedTexImage2D");
(void*&)fexthunks_impl_libGL_glCompressedTexSubImage2D = dlsym(fexthunks_impl_libGL_so, "glCompressedTexSubImage2D");
(void*&)fexthunks_impl_libGL_glCopyTexImage2D = dlsym(fexthunks_impl_libGL_so, "glCopyTexImage2D");
(void*&)fexthunks_impl_libGL_glCopyTexSubImage2D = dlsym(fexthunks_impl_libGL_so, "glCopyTexSubImage2D");
(void*&)fexthunks_impl_libGL_glCullFace = dlsym(fexthunks_impl_libGL_so, "glCullFace");
(void*&)fexthunks_impl_libGL_glDeleteTextures = dlsym(fexthunks_impl_libGL_so, "glDeleteTextures");
(void*&)fexthunks_impl_libGL_glDepthFunc = dlsym(fexthunks_impl_libGL_so, "glDepthFunc");
(void*&)fexthunks_impl_libGL_glDepthMask = dlsym(fexthunks_impl_libGL_so, "glDepthMask");
(void*&)fexthunks_impl_libGL_glDepthRange = dlsym(fexthunks_impl_libGL_so, "glDepthRange");
(void*&)fexthunks_impl_libGL_glDisable = dlsym(fexthunks_impl_libGL_so, "glDisable");
(void*&)fexthunks_impl_libGL_glDrawArrays = dlsym(fexthunks_impl_libGL_so, "glDrawArrays");
(void*&)fexthunks_impl_libGL_glDrawElements = dlsym(fexthunks_impl_libGL_so, "glDrawElements");
(void*&)fexthunks_impl_libGL_glFrontFace = dlsym(fexthunks_impl_libGL_so, "glFrontFace");
(void*&)fexthunks_impl_libGL_glGenTextures = dlsym(fexthunks_impl_libGL_so, "glGenTextures");
(void*&)fexthunks_impl_libGL_glGetFloatv = dlsym(fexthunks_impl_libGL_so, "glGetFloatv");
(void*&)fexthunks_impl_libGL_glGetIntegerv = dlsym(fexthunks_impl_libGL_so, "glGetIntegerv");
(void*&)fexthunks_impl_libGL_glGetTexImage = dlsym(fexthunks_impl_libGL_so, "glGetTexImage");
(void*&)fexthunks_impl_libGL_glGetTexLevelParameterfv = dlsym(fexthunks_impl_libGL_so, "glGetTexLevelParameterfv");
(void*&)fexthunks_impl_libGL_glPixelStorei = dlsym(fexthunks_impl_libGL_so, "glPixelStorei");
(void*&)fexthunks_impl_libGL_glRasterPos2i = dlsym(fexthunks_impl_libGL_so, "glRasterPos2i");
(void*&)fexthunks_impl_libGL_glReadPixels = dlsym(fexthunks_impl_libGL_so, "glReadPixels");
(void*&)fexthunks_impl_libGL_glScissor = dlsym(fexthunks_impl_libGL_so, "glScissor");
(void*&)fexthunks_impl_libGL_glStencilFunc = dlsym(fexthunks_impl_libGL_so, "glStencilFunc");
(void*&)fexthunks_impl_libGL_glStencilOp = dlsym(fexthunks_impl_libGL_so, "glStencilOp");
(void*&)fexthunks_impl_libGL_glTexImage2D = dlsym(fexthunks_impl_libGL_so, "glTexImage2D");
(void*&)fexthunks_impl_libGL_glTexParameteri = dlsym(fexthunks_impl_libGL_so, "glTexParameteri");
(void*&)fexthunks_impl_libGL_glTexSubImage2D = dlsym(fexthunks_impl_libGL_so, "glTexSubImage2D");
(void*&)fexthunks_impl_libGL_glXDestroyWindow = dlsym(fexthunks_impl_libGL_so, "glXDestroyWindow");
(void*&)fexthunks_impl_libGL_glXGetVisualFromFBConfig = dlsym(fexthunks_impl_libGL_so, "glXGetVisualFromFBConfig");
(void*&)fexthunks_impl_libGL_glFinish = dlsym(fexthunks_impl_libGL_so, "glFinish");
(void*&)fexthunks_impl_libGL_glFlush = dlsym(fexthunks_impl_libGL_so, "glFlush");
(void*&)fexthunks_impl_libGL_glGetStringi = dlsym(fexthunks_impl_libGL_so, "glGetStringi");
(void*&)fexthunks_impl_libGL_glIsProgram = dlsym(fexthunks_impl_libGL_so, "glIsProgram");
(void*&)fexthunks_impl_libGL_glIsShader = dlsym(fexthunks_impl_libGL_so, "glIsShader");
(void*&)fexthunks_impl_libGL_glCheckFramebufferStatus = dlsym(fexthunks_impl_libGL_so, "glCheckFramebufferStatus");
(void*&)fexthunks_impl_libGL_glCheckNamedFramebufferStatus = dlsym(fexthunks_impl_libGL_so, "glCheckNamedFramebufferStatus");
(void*&)fexthunks_impl_libGL_glGetUniformLocation = dlsym(fexthunks_impl_libGL_so, "glGetUniformLocation");
(void*&)fexthunks_impl_libGL_glCreateProgram = dlsym(fexthunks_impl_libGL_so, "glCreateProgram");
(void*&)fexthunks_impl_libGL_glCreateShader = dlsym(fexthunks_impl_libGL_so, "glCreateShader");
(void*&)fexthunks_impl_libGL_glAttachShader = dlsym(fexthunks_impl_libGL_so, "glAttachShader");
(void*&)fexthunks_impl_libGL_glBindAttribLocation = dlsym(fexthunks_impl_libGL_so, "glBindAttribLocation");
(void*&)fexthunks_impl_libGL_glBindBuffer = dlsym(fexthunks_impl_libGL_so, "glBindBuffer");
(void*&)fexthunks_impl_libGL_glBindFragDataLocation = dlsym(fexthunks_impl_libGL_so, "glBindFragDataLocation");
(void*&)fexthunks_impl_libGL_glBindFramebuffer = dlsym(fexthunks_impl_libGL_so, "glBindFramebuffer");
(void*&)fexthunks_impl_libGL_glBindProgramPipeline = dlsym(fexthunks_impl_libGL_so, "glBindProgramPipeline");
(void*&)fexthunks_impl_libGL_glBindRenderbuffer = dlsym(fexthunks_impl_libGL_so, "glBindRenderbuffer");
(void*&)fexthunks_impl_libGL_glBindTextureUnit = dlsym(fexthunks_impl_libGL_so, "glBindTextureUnit");
(void*&)fexthunks_impl_libGL_glBindVertexArray = dlsym(fexthunks_impl_libGL_so, "glBindVertexArray");
(void*&)fexthunks_impl_libGL_glBlendFuncSeparate = dlsym(fexthunks_impl_libGL_so, "glBlendFuncSeparate");
(void*&)fexthunks_impl_libGL_glBufferData = dlsym(fexthunks_impl_libGL_so, "glBufferData");
(void*&)fexthunks_impl_libGL_glBufferSubData = dlsym(fexthunks_impl_libGL_so, "glBufferSubData");
(void*&)fexthunks_impl_libGL_glCompileShader = dlsym(fexthunks_impl_libGL_so, "glCompileShader");
(void*&)fexthunks_impl_libGL_glCompressedTextureSubImage2D = dlsym(fexthunks_impl_libGL_so, "glCompressedTextureSubImage2D");
(void*&)fexthunks_impl_libGL_glCopyTextureSubImage2D = dlsym(fexthunks_impl_libGL_so, "glCopyTextureSubImage2D");
(void*&)fexthunks_impl_libGL_glCreateFramebuffers = dlsym(fexthunks_impl_libGL_so, "glCreateFramebuffers");
(void*&)fexthunks_impl_libGL_glCreateProgramPipelines = dlsym(fexthunks_impl_libGL_so, "glCreateProgramPipelines");
(void*&)fexthunks_impl_libGL_glCreateRenderbuffers = dlsym(fexthunks_impl_libGL_so, "glCreateRenderbuffers");
(void*&)fexthunks_impl_libGL_glCreateTextures = dlsym(fexthunks_impl_libGL_so, "glCreateTextures");
(void*&)fexthunks_impl_libGL_glDebugMessageCallback = dlsym(fexthunks_impl_libGL_so, "glDebugMessageCallback");
(void*&)fexthunks_impl_libGL_glDebugMessageControl = dlsym(fexthunks_impl_libGL_so, "glDebugMessageControl");
(void*&)fexthunks_impl_libGL_glDebugMessageInsert = dlsym(fexthunks_impl_libGL_so, "glDebugMessageInsert");
(void*&)fexthunks_impl_libGL_glDeleteBuffers = dlsym(fexthunks_impl_libGL_so, "glDeleteBuffers");
(void*&)fexthunks_impl_libGL_glDeleteFramebuffers = dlsym(fexthunks_impl_libGL_so, "glDeleteFramebuffers");
(void*&)fexthunks_impl_libGL_glDeleteProgram = dlsym(fexthunks_impl_libGL_so, "glDeleteProgram");
(void*&)fexthunks_impl_libGL_glDeleteProgramPipelines = dlsym(fexthunks_impl_libGL_so, "glDeleteProgramPipelines");
(void*&)fexthunks_impl_libGL_glDeleteRenderbuffers = dlsym(fexthunks_impl_libGL_so, "glDeleteRenderbuffers");
(void*&)fexthunks_impl_libGL_glDeleteShader = dlsym(fexthunks_impl_libGL_so, "glDeleteShader");
(void*&)fexthunks_impl_libGL_glDeleteVertexArrays = dlsym(fexthunks_impl_libGL_so, "glDeleteVertexArrays");
(void*&)fexthunks_impl_libGL_glDetachShader = dlsym(fexthunks_impl_libGL_so, "glDetachShader");
(void*&)fexthunks_impl_libGL_glDisableVertexAttribArray = dlsym(fexthunks_impl_libGL_so, "glDisableVertexAttribArray");
(void*&)fexthunks_impl_libGL_glEnableVertexAttribArray = dlsym(fexthunks_impl_libGL_so, "glEnableVertexAttribArray");
(void*&)fexthunks_impl_libGL_glFramebufferRenderbuffer = dlsym(fexthunks_impl_libGL_so, "glFramebufferRenderbuffer");
(void*&)fexthunks_impl_libGL_glFramebufferTexture2D = dlsym(fexthunks_impl_libGL_so, "glFramebufferTexture2D");
(void*&)fexthunks_impl_libGL_glGenBuffers = dlsym(fexthunks_impl_libGL_so, "glGenBuffers");
(void*&)fexthunks_impl_libGL_glGenerateMipmap = dlsym(fexthunks_impl_libGL_so, "glGenerateMipmap");
(void*&)fexthunks_impl_libGL_glGenerateTextureMipmap = dlsym(fexthunks_impl_libGL_so, "glGenerateTextureMipmap");
(void*&)fexthunks_impl_libGL_glGenFramebuffers = dlsym(fexthunks_impl_libGL_so, "glGenFramebuffers");
(void*&)fexthunks_impl_libGL_glGenProgramPipelines = dlsym(fexthunks_impl_libGL_so, "glGenProgramPipelines");
(void*&)fexthunks_impl_libGL_glGenRenderbuffers = dlsym(fexthunks_impl_libGL_so, "glGenRenderbuffers");
(void*&)fexthunks_impl_libGL_glGenVertexArrays = dlsym(fexthunks_impl_libGL_so, "glGenVertexArrays");
(void*&)fexthunks_impl_libGL_glGetFramebufferAttachmentParameteriv = dlsym(fexthunks_impl_libGL_so, "glGetFramebufferAttachmentParameteriv");
(void*&)fexthunks_impl_libGL_glGetNamedBufferSubData = dlsym(fexthunks_impl_libGL_so, "glGetNamedBufferSubData");
(void*&)fexthunks_impl_libGL_glGetProgramBinary = dlsym(fexthunks_impl_libGL_so, "glGetProgramBinary");
(void*&)fexthunks_impl_libGL_glGetProgramInfoLog = dlsym(fexthunks_impl_libGL_so, "glGetProgramInfoLog");
(void*&)fexthunks_impl_libGL_glGetProgramiv = dlsym(fexthunks_impl_libGL_so, "glGetProgramiv");
(void*&)fexthunks_impl_libGL_glGetProgramPipelineInfoLog = dlsym(fexthunks_impl_libGL_so, "glGetProgramPipelineInfoLog");
(void*&)fexthunks_impl_libGL_glGetProgramPipelineiv = dlsym(fexthunks_impl_libGL_so, "glGetProgramPipelineiv");
(void*&)fexthunks_impl_libGL_glGetShaderInfoLog = dlsym(fexthunks_impl_libGL_so, "glGetShaderInfoLog");
(void*&)fexthunks_impl_libGL_glGetShaderiv = dlsym(fexthunks_impl_libGL_so, "glGetShaderiv");
(void*&)fexthunks_impl_libGL_glGetShaderSource = dlsym(fexthunks_impl_libGL_so, "glGetShaderSource");
(void*&)fexthunks_impl_libGL_glGetTextureImage = dlsym(fexthunks_impl_libGL_so, "glGetTextureImage");
(void*&)fexthunks_impl_libGL_glInvalidateFramebuffer = dlsym(fexthunks_impl_libGL_so, "glInvalidateFramebuffer");
(void*&)fexthunks_impl_libGL_glInvalidateNamedFramebufferData = dlsym(fexthunks_impl_libGL_so, "glInvalidateNamedFramebufferData");
(void*&)fexthunks_impl_libGL_glLinkProgram = dlsym(fexthunks_impl_libGL_so, "glLinkProgram");
(void*&)fexthunks_impl_libGL_glNamedFramebufferRenderbuffer = dlsym(fexthunks_impl_libGL_so, "glNamedFramebufferRenderbuffer");
(void*&)fexthunks_impl_libGL_glNamedFramebufferTexture = dlsym(fexthunks_impl_libGL_so, "glNamedFramebufferTexture");
(void*&)fexthunks_impl_libGL_glNamedRenderbufferStorage = dlsym(fexthunks_impl_libGL_so, "glNamedRenderbufferStorage");
(void*&)fexthunks_impl_libGL_glProgramBinary = dlsym(fexthunks_impl_libGL_so, "glProgramBinary");
(void*&)fexthunks_impl_libGL_glProgramParameteri = dlsym(fexthunks_impl_libGL_so, "glProgramParameteri");
(void*&)fexthunks_impl_libGL_glProgramUniform1f = dlsym(fexthunks_impl_libGL_so, "glProgramUniform1f");
(void*&)fexthunks_impl_libGL_glProgramUniform1i = dlsym(fexthunks_impl_libGL_so, "glProgramUniform1i");
(void*&)fexthunks_impl_libGL_glProgramUniform2fv = dlsym(fexthunks_impl_libGL_so, "glProgramUniform2fv");
(void*&)fexthunks_impl_libGL_glProgramUniform3fv = dlsym(fexthunks_impl_libGL_so, "glProgramUniform3fv");
(void*&)fexthunks_impl_libGL_glProgramUniform4fv = dlsym(fexthunks_impl_libGL_so, "glProgramUniform4fv");
(void*&)fexthunks_impl_libGL_glProgramUniformMatrix4fv = dlsym(fexthunks_impl_libGL_so, "glProgramUniformMatrix4fv");
(void*&)fexthunks_impl_libGL_glRenderbufferStorage = dlsym(fexthunks_impl_libGL_so, "glRenderbufferStorage");
(void*&)fexthunks_impl_libGL_glShaderSource = dlsym(fexthunks_impl_libGL_so, "glShaderSource");
(void*&)fexthunks_impl_libGL_glTexStorage2D = dlsym(fexthunks_impl_libGL_so, "glTexStorage2D");
(void*&)fexthunks_impl_libGL_glTextureParameteri = dlsym(fexthunks_impl_libGL_so, "glTextureParameteri");
(void*&)fexthunks_impl_libGL_glTextureStorage2D = dlsym(fexthunks_impl_libGL_so, "glTextureStorage2D");
(void*&)fexthunks_impl_libGL_glTextureSubImage2D = dlsym(fexthunks_impl_libGL_so, "glTextureSubImage2D");
(void*&)fexthunks_impl_libGL_glUniform1f = dlsym(fexthunks_impl_libGL_so, "glUniform1f");
(void*&)fexthunks_impl_libGL_glUniform1fv = dlsym(fexthunks_impl_libGL_so, "glUniform1fv");
(void*&)fexthunks_impl_libGL_glUniform1i = dlsym(fexthunks_impl_libGL_so, "glUniform1i");
(void*&)fexthunks_impl_libGL_glUniform1iv = dlsym(fexthunks_impl_libGL_so, "glUniform1iv");
(void*&)fexthunks_impl_libGL_glUniform2f = dlsym(fexthunks_impl_libGL_so, "glUniform2f");
(void*&)fexthunks_impl_libGL_glUniform2fv = dlsym(fexthunks_impl_libGL_so, "glUniform2fv");
(void*&)fexthunks_impl_libGL_glUniform2i = dlsym(fexthunks_impl_libGL_so, "glUniform2i");
(void*&)fexthunks_impl_libGL_glUniform2iv = dlsym(fexthunks_impl_libGL_so, "glUniform2iv");
(void*&)fexthunks_impl_libGL_glUniform3f = dlsym(fexthunks_impl_libGL_so, "glUniform3f");
(void*&)fexthunks_impl_libGL_glUniform3fv = dlsym(fexthunks_impl_libGL_so, "glUniform3fv");
(void*&)fexthunks_impl_libGL_glUniform3i = dlsym(fexthunks_impl_libGL_so, "glUniform3i");
(void*&)fexthunks_impl_libGL_glUniform3iv = dlsym(fexthunks_impl_libGL_so, "glUniform3iv");
(void*&)fexthunks_impl_libGL_glUniform4f = dlsym(fexthunks_impl_libGL_so, "glUniform4f");
(void*&)fexthunks_impl_libGL_glUniform4fv = dlsym(fexthunks_impl_libGL_so, "glUniform4fv");
(void*&)fexthunks_impl_libGL_glUniform4i = dlsym(fexthunks_impl_libGL_so, "glUniform4i");
(void*&)fexthunks_impl_libGL_glUniform4iv = dlsym(fexthunks_impl_libGL_so, "glUniform4iv");
(void*&)fexthunks_impl_libGL_glUniformMatrix2fv = dlsym(fexthunks_impl_libGL_so, "glUniformMatrix2fv");
(void*&)fexthunks_impl_libGL_glUniformMatrix3fv = dlsym(fexthunks_impl_libGL_so, "glUniformMatrix3fv");
(void*&)fexthunks_impl_libGL_glUniformMatrix4fv = dlsym(fexthunks_impl_libGL_so, "glUniformMatrix4fv");
(void*&)fexthunks_impl_libGL_glUseProgram = dlsym(fexthunks_impl_libGL_so, "glUseProgram");
(void*&)fexthunks_impl_libGL_glUseProgramStages = dlsym(fexthunks_impl_libGL_so, "glUseProgramStages");
(void*&)fexthunks_impl_libGL_glValidateProgram = dlsym(fexthunks_impl_libGL_so, "glValidateProgram");
(void*&)fexthunks_impl_libGL_glValidateProgramPipeline = dlsym(fexthunks_impl_libGL_so, "glValidateProgramPipeline");
(void*&)fexthunks_impl_libGL_glVertexAttribIPointer = dlsym(fexthunks_impl_libGL_so, "glVertexAttribIPointer");
(void*&)fexthunks_impl_libGL_glVertexAttribPointer = dlsym(fexthunks_impl_libGL_so, "glVertexAttribPointer");
return true;
}
void* fexthunks_impl_libX11_so;
typedef Display* fexthunks_type_libX11_XOpenDisplay(const char* a_0);
fexthunks_type_libX11_XOpenDisplay *fexthunks_impl_libX11_XOpenDisplay;
typedef Colormap fexthunks_type_libX11_XCreateColormap(Display* a_0,Window a_1,Visual* a_2,int a_3);
fexthunks_type_libX11_XCreateColormap *fexthunks_impl_libX11_XCreateColormap;
typedef Window fexthunks_type_libX11_XCreateWindow(Display* a_0,Window a_1,int a_2,int a_3,unsigned int a_4,unsigned int a_5,unsigned int a_6,int a_7,unsigned int a_8,Visual* a_9,long unsigned int a_10,XSetWindowAttributes* a_11);
fexthunks_type_libX11_XCreateWindow *fexthunks_impl_libX11_XCreateWindow;
typedef int fexthunks_type_libX11_XMapWindow(Display * a_0,Window a_1);
fexthunks_type_libX11_XMapWindow *fexthunks_impl_libX11_XMapWindow;
typedef int fexthunks_type_libX11_XChangeProperty(Display* a_0,Window a_1,Atom a_2,Atom a_3,int a_4,int a_5,const unsigned char* a_6,int a_7);
fexthunks_type_libX11_XChangeProperty *fexthunks_impl_libX11_XChangeProperty;
typedef int fexthunks_type_libX11_XCloseDisplay(Display* a_0);
fexthunks_type_libX11_XCloseDisplay *fexthunks_impl_libX11_XCloseDisplay;
typedef int fexthunks_type_libX11_XDestroyWindow(Display* a_0,Window a_1);
fexthunks_type_libX11_XDestroyWindow *fexthunks_impl_libX11_XDestroyWindow;
typedef int fexthunks_type_libX11_XFree(void* a_0);
fexthunks_type_libX11_XFree *fexthunks_impl_libX11_XFree;
typedef Atom fexthunks_type_libX11_XInternAtom(Display* a_0,const char* a_1,int a_2);
fexthunks_type_libX11_XInternAtom *fexthunks_impl_libX11_XInternAtom;
typedef KeySym fexthunks_type_libX11_XLookupKeysym(XKeyEvent * a_0,int a_1);
fexthunks_type_libX11_XLookupKeysym *fexthunks_impl_libX11_XLookupKeysym;
typedef int fexthunks_type_libX11_XLookupString(XKeyEvent* a_0,char* a_1,int a_2,KeySym* a_3,XComposeStatus* a_4);
fexthunks_type_libX11_XLookupString *fexthunks_impl_libX11_XLookupString;
typedef int fexthunks_type_libX11_XNextEvent(Display* a_0,XEvent* a_1);
fexthunks_type_libX11_XNextEvent *fexthunks_impl_libX11_XNextEvent;
typedef int fexthunks_type_libX11_XParseGeometry(const char * a_0,int * a_1,int * a_2,unsigned int * a_3,unsigned int * a_4);
fexthunks_type_libX11_XParseGeometry *fexthunks_impl_libX11_XParseGeometry;
typedef int fexthunks_type_libX11_XPending(Display* a_0);
fexthunks_type_libX11_XPending *fexthunks_impl_libX11_XPending;
typedef int fexthunks_type_libX11_XSetNormalHints(Display * a_0,Window a_1,XSizeHints * a_2);
fexthunks_type_libX11_XSetNormalHints *fexthunks_impl_libX11_XSetNormalHints;
typedef int fexthunks_type_libX11_XSetStandardProperties(Display * a_0,Window a_1,const char * a_2,const char * a_3,Pixmap a_4,char ** a_5,int a_6,XSizeHints * a_7);
fexthunks_type_libX11_XSetStandardProperties *fexthunks_impl_libX11_XSetStandardProperties;
typedef char** fexthunks_type_libX11_XListExtensions(Display* a_0,int* a_1);
fexthunks_type_libX11_XListExtensions *fexthunks_impl_libX11_XListExtensions;
typedef char* fexthunks_type_libX11_XSetLocaleModifiers(const char* a_0);
fexthunks_type_libX11_XSetLocaleModifiers *fexthunks_impl_libX11_XSetLocaleModifiers;
typedef Cursor fexthunks_type_libX11_XCreatePixmapCursor(Display* a_0,Pixmap a_1,Pixmap a_2,XColor* a_3,XColor* a_4,unsigned int a_5,unsigned int a_6);
fexthunks_type_libX11_XCreatePixmapCursor *fexthunks_impl_libX11_XCreatePixmapCursor;
typedef int fexthunks_type_libX11_XCloseIM(XIM a_0);
fexthunks_type_libX11_XCloseIM *fexthunks_impl_libX11_XCloseIM;
typedef int fexthunks_type_libX11_XDefineCursor(Display* a_0,Window a_1,Cursor a_2);
fexthunks_type_libX11_XDefineCursor *fexthunks_impl_libX11_XDefineCursor;
typedef int fexthunks_type_libX11_XDrawString16(Display* a_0,Drawable a_1,GC a_2,int a_3,int a_4,const XChar2b* a_5,int a_6);
fexthunks_type_libX11_XDrawString16 *fexthunks_impl_libX11_XDrawString16;
typedef int fexthunks_type_libX11_XEventsQueued(Display* a_0,int a_1);
fexthunks_type_libX11_XEventsQueued *fexthunks_impl_libX11_XEventsQueued;
typedef int fexthunks_type_libX11_XFillRectangle(Display* a_0,Drawable a_1,GC a_2,int a_3,int a_4,unsigned int a_5,unsigned int a_6);
fexthunks_type_libX11_XFillRectangle *fexthunks_impl_libX11_XFillRectangle;
typedef int fexthunks_type_libX11_XFilterEvent(XEvent* a_0,Window a_1);
fexthunks_type_libX11_XFilterEvent *fexthunks_impl_libX11_XFilterEvent;
typedef int fexthunks_type_libX11_XFlush(Display* a_0);
fexthunks_type_libX11_XFlush *fexthunks_impl_libX11_XFlush;
typedef int fexthunks_type_libX11_XFreeColormap(Display* a_0,Colormap a_1);
fexthunks_type_libX11_XFreeColormap *fexthunks_impl_libX11_XFreeColormap;
typedef int fexthunks_type_libX11_XFreeCursor(Display* a_0,Cursor a_1);
fexthunks_type_libX11_XFreeCursor *fexthunks_impl_libX11_XFreeCursor;
typedef int fexthunks_type_libX11_XFreeExtensionList(char** a_0);
fexthunks_type_libX11_XFreeExtensionList *fexthunks_impl_libX11_XFreeExtensionList;
typedef int fexthunks_type_libX11_XFreeFont(Display* a_0,XFontStruct* a_1);
fexthunks_type_libX11_XFreeFont *fexthunks_impl_libX11_XFreeFont;
typedef int fexthunks_type_libX11_XFreeGC(Display* a_0,GC a_1);
fexthunks_type_libX11_XFreeGC *fexthunks_impl_libX11_XFreeGC;
typedef int fexthunks_type_libX11_XFreePixmap(Display* a_0,Pixmap a_1);
fexthunks_type_libX11_XFreePixmap *fexthunks_impl_libX11_XFreePixmap;
typedef int fexthunks_type_libX11_XGetErrorDatabaseText(Display* a_0,const char* a_1,const char* a_2,const char* a_3,char* a_4,int a_5);
fexthunks_type_libX11_XGetErrorDatabaseText *fexthunks_impl_libX11_XGetErrorDatabaseText;
typedef int fexthunks_type_libX11_XGetErrorText(Display* a_0,int a_1,char* a_2,int a_3);
fexthunks_type_libX11_XGetErrorText *fexthunks_impl_libX11_XGetErrorText;
typedef int fexthunks_type_libX11_XGetEventData(Display* a_0,XGenericEventCookie* a_1);
fexthunks_type_libX11_XGetEventData *fexthunks_impl_libX11_XGetEventData;
typedef int fexthunks_type_libX11_XGetWindowProperty(Display* a_0,Window a_1,Atom a_2,long int a_3,long int a_4,int a_5,Atom a_6,Atom* a_7,int* a_8,long unsigned int* a_9,long unsigned int* a_10,unsigned char** a_11);
fexthunks_type_libX11_XGetWindowProperty *fexthunks_impl_libX11_XGetWindowProperty;
typedef int fexthunks_type_libX11_XGrabPointer(Display* a_0,Window a_1,int a_2,unsigned int a_3,int a_4,int a_5,Window a_6,Cursor a_7,Time a_8);
fexthunks_type_libX11_XGrabPointer *fexthunks_impl_libX11_XGrabPointer;
typedef int fexthunks_type_libX11_XGrabServer(Display* a_0);
fexthunks_type_libX11_XGrabServer *fexthunks_impl_libX11_XGrabServer;
typedef int fexthunks_type_libX11_XIconifyWindow(Display* a_0,Window a_1,int a_2);
fexthunks_type_libX11_XIconifyWindow *fexthunks_impl_libX11_XIconifyWindow;
typedef int fexthunks_type_libX11_XIfEvent(Display* a_0,XEvent* a_1,XIfEventFN* a_2,XPointer a_3);
fexthunks_type_libX11_XIfEvent *fexthunks_impl_libX11_XIfEvent;
typedef int fexthunks_type_libX11_XInitThreads();
fexthunks_type_libX11_XInitThreads *fexthunks_impl_libX11_XInitThreads;
typedef int fexthunks_type_libX11_XMapRaised(Display* a_0,Window a_1);
fexthunks_type_libX11_XMapRaised *fexthunks_impl_libX11_XMapRaised;
typedef int fexthunks_type_libX11_XMoveResizeWindow(Display* a_0,Window a_1,int a_2,int a_3,unsigned int a_4,unsigned int a_5);
fexthunks_type_libX11_XMoveResizeWindow *fexthunks_impl_libX11_XMoveResizeWindow;
typedef int fexthunks_type_libX11_XMoveWindow(Display* a_0,Window a_1,int a_2,int a_3);
fexthunks_type_libX11_XMoveWindow *fexthunks_impl_libX11_XMoveWindow;
typedef int fexthunks_type_libX11_XPeekEvent(Display* a_0,XEvent* a_1);
fexthunks_type_libX11_XPeekEvent *fexthunks_impl_libX11_XPeekEvent;
typedef int fexthunks_type_libX11_XQueryExtension(Display* a_0,const char* a_1,int* a_2,int* a_3,int* a_4);
fexthunks_type_libX11_XQueryExtension *fexthunks_impl_libX11_XQueryExtension;
typedef int fexthunks_type_libX11_XQueryPointer(Display* a_0,Window a_1,Window* a_2,Window* a_3,int* a_4,int* a_5,int* a_6,int* a_7,unsigned int* a_8);
fexthunks_type_libX11_XQueryPointer *fexthunks_impl_libX11_XQueryPointer;
typedef int fexthunks_type_libX11_XQueryTree(Display* a_0,Window a_1,Window* a_2,Window* a_3,Window** a_4,unsigned int* a_5);
fexthunks_type_libX11_XQueryTree *fexthunks_impl_libX11_XQueryTree;
typedef int fexthunks_type_libX11_XResetScreenSaver(Display* a_0);
fexthunks_type_libX11_XResetScreenSaver *fexthunks_impl_libX11_XResetScreenSaver;
typedef int fexthunks_type_libX11_XResizeWindow(Display* a_0,Window a_1,unsigned int a_2,unsigned int a_3);
fexthunks_type_libX11_XResizeWindow *fexthunks_impl_libX11_XResizeWindow;
typedef int fexthunks_type_libX11_XSelectInput(Display* a_0,Window a_1,long int a_2);
fexthunks_type_libX11_XSelectInput *fexthunks_impl_libX11_XSelectInput;
typedef int fexthunks_type_libX11_XSendEvent(Display* a_0,Window a_1,int a_2,long int a_3,XEvent* a_4);
fexthunks_type_libX11_XSendEvent *fexthunks_impl_libX11_XSendEvent;
typedef XSetErrorHandlerFN* fexthunks_type_libX11_XSetErrorHandler(XErrorHandler a_0);
fexthunks_type_libX11_XSetErrorHandler *fexthunks_impl_libX11_XSetErrorHandler;
typedef int fexthunks_type_libX11_XSetTransientForHint(Display* a_0,Window a_1,Window a_2);
fexthunks_type_libX11_XSetTransientForHint *fexthunks_impl_libX11_XSetTransientForHint;
typedef int fexthunks_type_libX11_XSetWMProtocols(Display* a_0,Window a_1,Atom* a_2,int a_3);
fexthunks_type_libX11_XSetWMProtocols *fexthunks_impl_libX11_XSetWMProtocols;
typedef int fexthunks_type_libX11_XSync(Display* a_0,int a_1);
fexthunks_type_libX11_XSync *fexthunks_impl_libX11_XSync;
typedef int fexthunks_type_libX11_XTextExtents16(XFontStruct* a_0,const XChar2b* a_1,int a_2,int* a_3,int* a_4,int* a_5,XCharStruct* a_6);
fexthunks_type_libX11_XTextExtents16 *fexthunks_impl_libX11_XTextExtents16;
typedef int fexthunks_type_libX11_XTranslateCoordinates(Display* a_0,Window a_1,Window a_2,int a_3,int a_4,int* a_5,int* a_6,Window* a_7);
fexthunks_type_libX11_XTranslateCoordinates *fexthunks_impl_libX11_XTranslateCoordinates;
typedef int fexthunks_type_libX11_XUngrabPointer(Display* a_0,Time a_1);
fexthunks_type_libX11_XUngrabPointer *fexthunks_impl_libX11_XUngrabPointer;
typedef int fexthunks_type_libX11_XUngrabServer(Display* a_0);
fexthunks_type_libX11_XUngrabServer *fexthunks_impl_libX11_XUngrabServer;
typedef int fexthunks_type_libX11_XUnmapWindow(Display* a_0,Window a_1);
fexthunks_type_libX11_XUnmapWindow *fexthunks_impl_libX11_XUnmapWindow;
typedef int fexthunks_type_libX11_Xutf8LookupString(XIC a_0,XKeyPressedEvent* a_1,char* a_2,int a_3,KeySym* a_4,int* a_5);
fexthunks_type_libX11_Xutf8LookupString *fexthunks_impl_libX11_Xutf8LookupString;
typedef int fexthunks_type_libX11_XWarpPointer(Display* a_0,Window a_1,Window a_2,int a_3,int a_4,unsigned int a_5,unsigned int a_6,int a_7,int a_8);
fexthunks_type_libX11_XWarpPointer *fexthunks_impl_libX11_XWarpPointer;
typedef int fexthunks_type_libX11_XWindowEvent(Display* a_0,Window a_1,long int a_2,XEvent* a_3);
fexthunks_type_libX11_XWindowEvent *fexthunks_impl_libX11_XWindowEvent;
typedef Pixmap fexthunks_type_libX11_XCreateBitmapFromData(Display* a_0,Drawable a_1,const char* a_2,unsigned int a_3,unsigned int a_4);
fexthunks_type_libX11_XCreateBitmapFromData *fexthunks_impl_libX11_XCreateBitmapFromData;
typedef Pixmap fexthunks_type_libX11_XCreatePixmap(Display* a_0,Drawable a_1,unsigned int a_2,unsigned int a_3,unsigned int a_4);
fexthunks_type_libX11_XCreatePixmap *fexthunks_impl_libX11_XCreatePixmap;
typedef void fexthunks_type_libX11_XDestroyIC(XIC a_0);
fexthunks_type_libX11_XDestroyIC *fexthunks_impl_libX11_XDestroyIC;
typedef void fexthunks_type_libX11_XFreeEventData(Display* a_0,XGenericEventCookie* a_1);
fexthunks_type_libX11_XFreeEventData *fexthunks_impl_libX11_XFreeEventData;
typedef void fexthunks_type_libX11_XLockDisplay(Display* a_0);
fexthunks_type_libX11_XLockDisplay *fexthunks_impl_libX11_XLockDisplay;
typedef void fexthunks_type_libX11_XSetICFocus(XIC a_0);
fexthunks_type_libX11_XSetICFocus *fexthunks_impl_libX11_XSetICFocus;
typedef void fexthunks_type_libX11_XSetWMNormalHints(Display* a_0,Window a_1,XSizeHints* a_2);
fexthunks_type_libX11_XSetWMNormalHints *fexthunks_impl_libX11_XSetWMNormalHints;
typedef void fexthunks_type_libX11_XUnlockDisplay(Display* a_0);
fexthunks_type_libX11_XUnlockDisplay *fexthunks_impl_libX11_XUnlockDisplay;
typedef void fexthunks_type_libX11_Xutf8SetWMProperties(Display* a_0,Window a_1,const char* a_2,const char* a_3,char** a_4,int a_5,XSizeHints* a_6,XWMHints* a_7,XClassHint* a_8);
fexthunks_type_libX11_Xutf8SetWMProperties *fexthunks_impl_libX11_Xutf8SetWMProperties;
typedef XFontStruct* fexthunks_type_libX11_XLoadQueryFont(Display* a_0,const char* a_1);
fexthunks_type_libX11_XLoadQueryFont *fexthunks_impl_libX11_XLoadQueryFont;
typedef _XGC* fexthunks_type_libX11_XCreateGC(Display* a_0,Drawable a_1,long unsigned int a_2,XGCValues* a_3);
fexthunks_type_libX11_XCreateGC *fexthunks_impl_libX11_XCreateGC;
typedef XImage* fexthunks_type_libX11_XGetImage(Display* a_0,Drawable a_1,int a_2,int a_3,unsigned int a_4,unsigned int a_5,long unsigned int a_6,int a_7);
fexthunks_type_libX11_XGetImage *fexthunks_impl_libX11_XGetImage;
typedef _XIM* fexthunks_type_libX11_XOpenIM(Display* a_0,_XrmHashBucketRec* a_1,char* a_2,char* a_3);
fexthunks_type_libX11_XOpenIM *fexthunks_impl_libX11_XOpenIM;
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
(void*&)fexthunks_impl_libX11_XListExtensions = dlsym(fexthunks_impl_libX11_so, "XListExtensions");
(void*&)fexthunks_impl_libX11_XSetLocaleModifiers = dlsym(fexthunks_impl_libX11_so, "XSetLocaleModifiers");
(void*&)fexthunks_impl_libX11_XCreatePixmapCursor = dlsym(fexthunks_impl_libX11_so, "XCreatePixmapCursor");
(void*&)fexthunks_impl_libX11_XCloseIM = dlsym(fexthunks_impl_libX11_so, "XCloseIM");
(void*&)fexthunks_impl_libX11_XDefineCursor = dlsym(fexthunks_impl_libX11_so, "XDefineCursor");
(void*&)fexthunks_impl_libX11_XDrawString16 = dlsym(fexthunks_impl_libX11_so, "XDrawString16");
(void*&)fexthunks_impl_libX11_XEventsQueued = dlsym(fexthunks_impl_libX11_so, "XEventsQueued");
(void*&)fexthunks_impl_libX11_XFillRectangle = dlsym(fexthunks_impl_libX11_so, "XFillRectangle");
(void*&)fexthunks_impl_libX11_XFilterEvent = dlsym(fexthunks_impl_libX11_so, "XFilterEvent");
(void*&)fexthunks_impl_libX11_XFlush = dlsym(fexthunks_impl_libX11_so, "XFlush");
(void*&)fexthunks_impl_libX11_XFreeColormap = dlsym(fexthunks_impl_libX11_so, "XFreeColormap");
(void*&)fexthunks_impl_libX11_XFreeCursor = dlsym(fexthunks_impl_libX11_so, "XFreeCursor");
(void*&)fexthunks_impl_libX11_XFreeExtensionList = dlsym(fexthunks_impl_libX11_so, "XFreeExtensionList");
(void*&)fexthunks_impl_libX11_XFreeFont = dlsym(fexthunks_impl_libX11_so, "XFreeFont");
(void*&)fexthunks_impl_libX11_XFreeGC = dlsym(fexthunks_impl_libX11_so, "XFreeGC");
(void*&)fexthunks_impl_libX11_XFreePixmap = dlsym(fexthunks_impl_libX11_so, "XFreePixmap");
(void*&)fexthunks_impl_libX11_XGetErrorDatabaseText = dlsym(fexthunks_impl_libX11_so, "XGetErrorDatabaseText");
(void*&)fexthunks_impl_libX11_XGetErrorText = dlsym(fexthunks_impl_libX11_so, "XGetErrorText");
(void*&)fexthunks_impl_libX11_XGetEventData = dlsym(fexthunks_impl_libX11_so, "XGetEventData");
(void*&)fexthunks_impl_libX11_XGetWindowProperty = dlsym(fexthunks_impl_libX11_so, "XGetWindowProperty");
(void*&)fexthunks_impl_libX11_XGrabPointer = dlsym(fexthunks_impl_libX11_so, "XGrabPointer");
(void*&)fexthunks_impl_libX11_XGrabServer = dlsym(fexthunks_impl_libX11_so, "XGrabServer");
(void*&)fexthunks_impl_libX11_XIconifyWindow = dlsym(fexthunks_impl_libX11_so, "XIconifyWindow");
(void*&)fexthunks_impl_libX11_XIfEvent = dlsym(fexthunks_impl_libX11_so, "XIfEvent");
(void*&)fexthunks_impl_libX11_XInitThreads = dlsym(fexthunks_impl_libX11_so, "XInitThreads");
(void*&)fexthunks_impl_libX11_XMapRaised = dlsym(fexthunks_impl_libX11_so, "XMapRaised");
(void*&)fexthunks_impl_libX11_XMoveResizeWindow = dlsym(fexthunks_impl_libX11_so, "XMoveResizeWindow");
(void*&)fexthunks_impl_libX11_XMoveWindow = dlsym(fexthunks_impl_libX11_so, "XMoveWindow");
(void*&)fexthunks_impl_libX11_XPeekEvent = dlsym(fexthunks_impl_libX11_so, "XPeekEvent");
(void*&)fexthunks_impl_libX11_XQueryExtension = dlsym(fexthunks_impl_libX11_so, "XQueryExtension");
(void*&)fexthunks_impl_libX11_XQueryPointer = dlsym(fexthunks_impl_libX11_so, "XQueryPointer");
(void*&)fexthunks_impl_libX11_XQueryTree = dlsym(fexthunks_impl_libX11_so, "XQueryTree");
(void*&)fexthunks_impl_libX11_XResetScreenSaver = dlsym(fexthunks_impl_libX11_so, "XResetScreenSaver");
(void*&)fexthunks_impl_libX11_XResizeWindow = dlsym(fexthunks_impl_libX11_so, "XResizeWindow");
(void*&)fexthunks_impl_libX11_XSelectInput = dlsym(fexthunks_impl_libX11_so, "XSelectInput");
(void*&)fexthunks_impl_libX11_XSendEvent = dlsym(fexthunks_impl_libX11_so, "XSendEvent");
(void*&)fexthunks_impl_libX11_XSetErrorHandler = dlsym(fexthunks_impl_libX11_so, "XSetErrorHandler");
(void*&)fexthunks_impl_libX11_XSetTransientForHint = dlsym(fexthunks_impl_libX11_so, "XSetTransientForHint");
(void*&)fexthunks_impl_libX11_XSetWMProtocols = dlsym(fexthunks_impl_libX11_so, "XSetWMProtocols");
(void*&)fexthunks_impl_libX11_XSync = dlsym(fexthunks_impl_libX11_so, "XSync");
(void*&)fexthunks_impl_libX11_XTextExtents16 = dlsym(fexthunks_impl_libX11_so, "XTextExtents16");
(void*&)fexthunks_impl_libX11_XTranslateCoordinates = dlsym(fexthunks_impl_libX11_so, "XTranslateCoordinates");
(void*&)fexthunks_impl_libX11_XUngrabPointer = dlsym(fexthunks_impl_libX11_so, "XUngrabPointer");
(void*&)fexthunks_impl_libX11_XUngrabServer = dlsym(fexthunks_impl_libX11_so, "XUngrabServer");
(void*&)fexthunks_impl_libX11_XUnmapWindow = dlsym(fexthunks_impl_libX11_so, "XUnmapWindow");
(void*&)fexthunks_impl_libX11_Xutf8LookupString = dlsym(fexthunks_impl_libX11_so, "Xutf8LookupString");
(void*&)fexthunks_impl_libX11_XWarpPointer = dlsym(fexthunks_impl_libX11_so, "XWarpPointer");
(void*&)fexthunks_impl_libX11_XWindowEvent = dlsym(fexthunks_impl_libX11_so, "XWindowEvent");
(void*&)fexthunks_impl_libX11_XCreateBitmapFromData = dlsym(fexthunks_impl_libX11_so, "XCreateBitmapFromData");
(void*&)fexthunks_impl_libX11_XCreatePixmap = dlsym(fexthunks_impl_libX11_so, "XCreatePixmap");
(void*&)fexthunks_impl_libX11_XDestroyIC = dlsym(fexthunks_impl_libX11_so, "XDestroyIC");
(void*&)fexthunks_impl_libX11_XFreeEventData = dlsym(fexthunks_impl_libX11_so, "XFreeEventData");
(void*&)fexthunks_impl_libX11_XLockDisplay = dlsym(fexthunks_impl_libX11_so, "XLockDisplay");
(void*&)fexthunks_impl_libX11_XSetICFocus = dlsym(fexthunks_impl_libX11_so, "XSetICFocus");
(void*&)fexthunks_impl_libX11_XSetWMNormalHints = dlsym(fexthunks_impl_libX11_so, "XSetWMNormalHints");
(void*&)fexthunks_impl_libX11_XUnlockDisplay = dlsym(fexthunks_impl_libX11_so, "XUnlockDisplay");
(void*&)fexthunks_impl_libX11_Xutf8SetWMProperties = dlsym(fexthunks_impl_libX11_so, "Xutf8SetWMProperties");
(void*&)fexthunks_impl_libX11_XLoadQueryFont = dlsym(fexthunks_impl_libX11_so, "XLoadQueryFont");
(void*&)fexthunks_impl_libX11_XCreateGC = dlsym(fexthunks_impl_libX11_so, "XCreateGC");
(void*&)fexthunks_impl_libX11_XGetImage = dlsym(fexthunks_impl_libX11_so, "XGetImage");
(void*&)fexthunks_impl_libX11_XOpenIM = dlsym(fexthunks_impl_libX11_so, "XOpenIM");
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
struct arg_t {Display* a_0;GLXContext a_1;};
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
struct arg_t {Display* a_0;GLXDrawable a_1;};
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
struct arg_t {Display* a_0;int* a_1;int* a_2;int rv;};
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
struct arg_t {__GLXcontextRec* rv;};
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
struct arg_t {Display* a_0;int a_1;const char* rv;};
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
struct arg_t {Display* a_0;int a_1;const int* a_2;int* a_3;__GLXFBConfigRec** rv;};
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
struct arg_t {GLenum a_0;const GLubyte* rv;};
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
void fexthunks_forward_libGL_glIsEnabled(void *argsv){
struct arg_t {GLenum a_0;GLboolean rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libGL_glIsEnabled
(args->a_0);
}
void fexthunks_forward_libGL_glGetError(void *argsv){
struct arg_t {GLenum rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libGL_glGetError
();
}
void fexthunks_forward_libGL_glXCreateNewContext(void *argsv){
struct arg_t {Display* a_0;GLXFBConfig a_1;int a_2;GLXContext a_3;int a_4;__GLXcontextRec* rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libGL_glXCreateNewContext
(args->a_0,args->a_1,args->a_2,args->a_3,args->a_4);
}
void fexthunks_forward_libGL_glXCreateWindow(void *argsv){
struct arg_t {Display* a_0;GLXFBConfig a_1;Window a_2;const int* a_3;GLXWindow rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libGL_glXCreateWindow
(args->a_0,args->a_1,args->a_2,args->a_3);
}
void fexthunks_forward_libGL_glXMakeContextCurrent(void *argsv){
struct arg_t {Display* a_0;GLXDrawable a_1;GLXDrawable a_2;GLXContext a_3;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libGL_glXMakeContextCurrent
(args->a_0,args->a_1,args->a_2,args->a_3);
}
void fexthunks_forward_libGL_glActiveTexture(void *argsv){
struct arg_t {GLenum a_0;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glActiveTexture
(args->a_0);
}
void fexthunks_forward_libGL_glBindTexture(void *argsv){
struct arg_t {GLenum a_0;GLuint a_1;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glBindTexture
(args->a_0,args->a_1);
}
void fexthunks_forward_libGL_glBlendColor(void *argsv){
struct arg_t {GLclampf a_0;GLclampf a_1;GLclampf a_2;GLclampf a_3;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glBlendColor
(args->a_0,args->a_1,args->a_2,args->a_3);
}
void fexthunks_forward_libGL_glBlendEquation(void *argsv){
struct arg_t {GLenum a_0;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glBlendEquation
(args->a_0);
}
void fexthunks_forward_libGL_glBlendFunc(void *argsv){
struct arg_t {GLenum a_0;GLenum a_1;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glBlendFunc
(args->a_0,args->a_1);
}
void fexthunks_forward_libGL_glClearDepth(void *argsv){
struct arg_t {GLclampd a_0;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glClearDepth
(args->a_0);
}
void fexthunks_forward_libGL_glClearStencil(void *argsv){
struct arg_t {GLint a_0;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glClearStencil
(args->a_0);
}
void fexthunks_forward_libGL_glColorMask(void *argsv){
struct arg_t {GLboolean a_0;GLboolean a_1;GLboolean a_2;GLboolean a_3;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glColorMask
(args->a_0,args->a_1,args->a_2,args->a_3);
}
void fexthunks_forward_libGL_glCompressedTexImage2D(void *argsv){
struct arg_t {GLenum a_0;GLint a_1;GLenum a_2;GLsizei a_3;GLsizei a_4;GLint a_5;GLsizei a_6;const GLvoid* a_7;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glCompressedTexImage2D
(args->a_0,args->a_1,args->a_2,args->a_3,args->a_4,args->a_5,args->a_6,args->a_7);
}
void fexthunks_forward_libGL_glCompressedTexSubImage2D(void *argsv){
struct arg_t {GLenum a_0;GLint a_1;GLint a_2;GLint a_3;GLsizei a_4;GLsizei a_5;GLenum a_6;GLsizei a_7;const GLvoid* a_8;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glCompressedTexSubImage2D
(args->a_0,args->a_1,args->a_2,args->a_3,args->a_4,args->a_5,args->a_6,args->a_7,args->a_8);
}
void fexthunks_forward_libGL_glCopyTexImage2D(void *argsv){
struct arg_t {GLenum a_0;GLint a_1;GLenum a_2;GLint a_3;GLint a_4;GLsizei a_5;GLsizei a_6;GLint a_7;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glCopyTexImage2D
(args->a_0,args->a_1,args->a_2,args->a_3,args->a_4,args->a_5,args->a_6,args->a_7);
}
void fexthunks_forward_libGL_glCopyTexSubImage2D(void *argsv){
struct arg_t {GLenum a_0;GLint a_1;GLint a_2;GLint a_3;GLint a_4;GLint a_5;GLsizei a_6;GLsizei a_7;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glCopyTexSubImage2D
(args->a_0,args->a_1,args->a_2,args->a_3,args->a_4,args->a_5,args->a_6,args->a_7);
}
void fexthunks_forward_libGL_glCullFace(void *argsv){
struct arg_t {GLenum a_0;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glCullFace
(args->a_0);
}
void fexthunks_forward_libGL_glDeleteTextures(void *argsv){
struct arg_t {GLsizei a_0;const GLuint* a_1;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glDeleteTextures
(args->a_0,args->a_1);
}
void fexthunks_forward_libGL_glDepthFunc(void *argsv){
struct arg_t {GLenum a_0;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glDepthFunc
(args->a_0);
}
void fexthunks_forward_libGL_glDepthMask(void *argsv){
struct arg_t {GLboolean a_0;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glDepthMask
(args->a_0);
}
void fexthunks_forward_libGL_glDepthRange(void *argsv){
struct arg_t {GLclampd a_0;GLclampd a_1;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glDepthRange
(args->a_0,args->a_1);
}
void fexthunks_forward_libGL_glDisable(void *argsv){
struct arg_t {GLenum a_0;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glDisable
(args->a_0);
}
void fexthunks_forward_libGL_glDrawArrays(void *argsv){
struct arg_t {GLenum a_0;GLint a_1;GLsizei a_2;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glDrawArrays
(args->a_0,args->a_1,args->a_2);
}
void fexthunks_forward_libGL_glDrawElements(void *argsv){
struct arg_t {GLenum a_0;GLsizei a_1;GLenum a_2;const GLvoid* a_3;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glDrawElements
(args->a_0,args->a_1,args->a_2,args->a_3);
}
void fexthunks_forward_libGL_glFrontFace(void *argsv){
struct arg_t {GLenum a_0;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glFrontFace
(args->a_0);
}
void fexthunks_forward_libGL_glGenTextures(void *argsv){
struct arg_t {GLsizei a_0;GLuint* a_1;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glGenTextures
(args->a_0,args->a_1);
}
void fexthunks_forward_libGL_glGetFloatv(void *argsv){
struct arg_t {GLenum a_0;GLfloat* a_1;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glGetFloatv
(args->a_0,args->a_1);
}
void fexthunks_forward_libGL_glGetIntegerv(void *argsv){
struct arg_t {GLenum a_0;GLint* a_1;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glGetIntegerv
(args->a_0,args->a_1);
}
void fexthunks_forward_libGL_glGetTexImage(void *argsv){
struct arg_t {GLenum a_0;GLint a_1;GLenum a_2;GLenum a_3;GLvoid* a_4;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glGetTexImage
(args->a_0,args->a_1,args->a_2,args->a_3,args->a_4);
}
void fexthunks_forward_libGL_glGetTexLevelParameterfv(void *argsv){
struct arg_t {GLenum a_0;GLint a_1;GLenum a_2;GLfloat* a_3;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glGetTexLevelParameterfv
(args->a_0,args->a_1,args->a_2,args->a_3);
}
void fexthunks_forward_libGL_glPixelStorei(void *argsv){
struct arg_t {GLenum a_0;GLint a_1;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glPixelStorei
(args->a_0,args->a_1);
}
void fexthunks_forward_libGL_glRasterPos2i(void *argsv){
struct arg_t {GLint a_0;GLint a_1;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glRasterPos2i
(args->a_0,args->a_1);
}
void fexthunks_forward_libGL_glReadPixels(void *argsv){
struct arg_t {GLint a_0;GLint a_1;GLsizei a_2;GLsizei a_3;GLenum a_4;GLenum a_5;GLvoid* a_6;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glReadPixels
(args->a_0,args->a_1,args->a_2,args->a_3,args->a_4,args->a_5,args->a_6);
}
void fexthunks_forward_libGL_glScissor(void *argsv){
struct arg_t {GLint a_0;GLint a_1;GLsizei a_2;GLsizei a_3;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glScissor
(args->a_0,args->a_1,args->a_2,args->a_3);
}
void fexthunks_forward_libGL_glStencilFunc(void *argsv){
struct arg_t {GLenum a_0;GLint a_1;GLuint a_2;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glStencilFunc
(args->a_0,args->a_1,args->a_2);
}
void fexthunks_forward_libGL_glStencilOp(void *argsv){
struct arg_t {GLenum a_0;GLenum a_1;GLenum a_2;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glStencilOp
(args->a_0,args->a_1,args->a_2);
}
void fexthunks_forward_libGL_glTexImage2D(void *argsv){
struct arg_t {GLenum a_0;GLint a_1;GLint a_2;GLsizei a_3;GLsizei a_4;GLint a_5;GLenum a_6;GLenum a_7;const GLvoid* a_8;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glTexImage2D
(args->a_0,args->a_1,args->a_2,args->a_3,args->a_4,args->a_5,args->a_6,args->a_7,args->a_8);
}
void fexthunks_forward_libGL_glTexParameteri(void *argsv){
struct arg_t {GLenum a_0;GLenum a_1;GLint a_2;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glTexParameteri
(args->a_0,args->a_1,args->a_2);
}
void fexthunks_forward_libGL_glTexSubImage2D(void *argsv){
struct arg_t {GLenum a_0;GLint a_1;GLint a_2;GLint a_3;GLsizei a_4;GLsizei a_5;GLenum a_6;GLenum a_7;const GLvoid* a_8;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glTexSubImage2D
(args->a_0,args->a_1,args->a_2,args->a_3,args->a_4,args->a_5,args->a_6,args->a_7,args->a_8);
}
void fexthunks_forward_libGL_glXDestroyWindow(void *argsv){
struct arg_t {Display* a_0;GLXWindow a_1;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glXDestroyWindow
(args->a_0,args->a_1);
}
void fexthunks_forward_libGL_glXGetVisualFromFBConfig(void *argsv){
struct arg_t {Display* a_0;GLXFBConfig a_1;XVisualInfo* rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libGL_glXGetVisualFromFBConfig
(args->a_0,args->a_1);
}
void fexthunks_forward_libGL_glFinish(void *argsv){
struct arg_t {};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glFinish
();
}
void fexthunks_forward_libGL_glFlush(void *argsv){
struct arg_t {};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glFlush
();
}
void fexthunks_forward_libGL_glGetStringi(void *argsv){
struct arg_t {GLenum a_0;GLuint a_1;const GLubyte* rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libGL_glGetStringi
(args->a_0,args->a_1);
}
void fexthunks_forward_libGL_glIsProgram(void *argsv){
struct arg_t {GLuint a_0;GLboolean rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libGL_glIsProgram
(args->a_0);
}
void fexthunks_forward_libGL_glIsShader(void *argsv){
struct arg_t {GLuint a_0;GLboolean rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libGL_glIsShader
(args->a_0);
}
void fexthunks_forward_libGL_glCheckFramebufferStatus(void *argsv){
struct arg_t {GLenum a_0;GLenum rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libGL_glCheckFramebufferStatus
(args->a_0);
}
void fexthunks_forward_libGL_glCheckNamedFramebufferStatus(void *argsv){
struct arg_t {GLuint a_0;GLenum a_1;GLenum rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libGL_glCheckNamedFramebufferStatus
(args->a_0,args->a_1);
}
void fexthunks_forward_libGL_glGetUniformLocation(void *argsv){
struct arg_t {GLuint a_0;const GLchar* a_1;GLint rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libGL_glGetUniformLocation
(args->a_0,args->a_1);
}
void fexthunks_forward_libGL_glCreateProgram(void *argsv){
struct arg_t {GLuint rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libGL_glCreateProgram
();
}
void fexthunks_forward_libGL_glCreateShader(void *argsv){
struct arg_t {GLenum a_0;GLuint rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libGL_glCreateShader
(args->a_0);
}
void fexthunks_forward_libGL_glAttachShader(void *argsv){
struct arg_t {GLuint a_0;GLuint a_1;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glAttachShader
(args->a_0,args->a_1);
}
void fexthunks_forward_libGL_glBindAttribLocation(void *argsv){
struct arg_t {GLuint a_0;GLuint a_1;const GLchar* a_2;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glBindAttribLocation
(args->a_0,args->a_1,args->a_2);
}
void fexthunks_forward_libGL_glBindBuffer(void *argsv){
struct arg_t {GLenum a_0;GLuint a_1;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glBindBuffer
(args->a_0,args->a_1);
}
void fexthunks_forward_libGL_glBindFragDataLocation(void *argsv){
struct arg_t {GLuint a_0;GLuint a_1;const GLchar* a_2;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glBindFragDataLocation
(args->a_0,args->a_1,args->a_2);
}
void fexthunks_forward_libGL_glBindFramebuffer(void *argsv){
struct arg_t {GLenum a_0;GLuint a_1;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glBindFramebuffer
(args->a_0,args->a_1);
}
void fexthunks_forward_libGL_glBindProgramPipeline(void *argsv){
struct arg_t {GLuint a_0;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glBindProgramPipeline
(args->a_0);
}
void fexthunks_forward_libGL_glBindRenderbuffer(void *argsv){
struct arg_t {GLenum a_0;GLuint a_1;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glBindRenderbuffer
(args->a_0,args->a_1);
}
void fexthunks_forward_libGL_glBindTextureUnit(void *argsv){
struct arg_t {GLuint a_0;GLuint a_1;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glBindTextureUnit
(args->a_0,args->a_1);
}
void fexthunks_forward_libGL_glBindVertexArray(void *argsv){
struct arg_t {GLuint a_0;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glBindVertexArray
(args->a_0);
}
void fexthunks_forward_libGL_glBlendFuncSeparate(void *argsv){
struct arg_t {GLenum a_0;GLenum a_1;GLenum a_2;GLenum a_3;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glBlendFuncSeparate
(args->a_0,args->a_1,args->a_2,args->a_3);
}
void fexthunks_forward_libGL_glBufferData(void *argsv){
struct arg_t {GLenum a_0;GLsizeiptr a_1;const void* a_2;GLenum a_3;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glBufferData
(args->a_0,args->a_1,args->a_2,args->a_3);
}
void fexthunks_forward_libGL_glBufferSubData(void *argsv){
struct arg_t {GLenum a_0;GLintptr a_1;GLsizeiptr a_2;const void* a_3;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glBufferSubData
(args->a_0,args->a_1,args->a_2,args->a_3);
}
void fexthunks_forward_libGL_glCompileShader(void *argsv){
struct arg_t {GLuint a_0;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glCompileShader
(args->a_0);
}
void fexthunks_forward_libGL_glCompressedTextureSubImage2D(void *argsv){
struct arg_t {GLuint a_0;GLint a_1;GLint a_2;GLint a_3;GLsizei a_4;GLsizei a_5;GLenum a_6;GLsizei a_7;const void* a_8;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glCompressedTextureSubImage2D
(args->a_0,args->a_1,args->a_2,args->a_3,args->a_4,args->a_5,args->a_6,args->a_7,args->a_8);
}
void fexthunks_forward_libGL_glCopyTextureSubImage2D(void *argsv){
struct arg_t {GLuint a_0;GLint a_1;GLint a_2;GLint a_3;GLint a_4;GLint a_5;GLsizei a_6;GLsizei a_7;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glCopyTextureSubImage2D
(args->a_0,args->a_1,args->a_2,args->a_3,args->a_4,args->a_5,args->a_6,args->a_7);
}
void fexthunks_forward_libGL_glCreateFramebuffers(void *argsv){
struct arg_t {GLsizei a_0;GLuint* a_1;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glCreateFramebuffers
(args->a_0,args->a_1);
}
void fexthunks_forward_libGL_glCreateProgramPipelines(void *argsv){
struct arg_t {GLsizei a_0;GLuint* a_1;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glCreateProgramPipelines
(args->a_0,args->a_1);
}
void fexthunks_forward_libGL_glCreateRenderbuffers(void *argsv){
struct arg_t {GLsizei a_0;GLuint* a_1;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glCreateRenderbuffers
(args->a_0,args->a_1);
}
void fexthunks_forward_libGL_glCreateTextures(void *argsv){
struct arg_t {GLenum a_0;GLsizei a_1;GLuint* a_2;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glCreateTextures
(args->a_0,args->a_1,args->a_2);
}
void fexthunks_forward_libGL_glDebugMessageCallback(void *argsv){
struct arg_t {GLDEBUGPROC a_0;const void* a_1;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glDebugMessageCallback
(args->a_0,args->a_1);
}
void fexthunks_forward_libGL_glDebugMessageControl(void *argsv){
struct arg_t {GLenum a_0;GLenum a_1;GLenum a_2;GLsizei a_3;const GLuint* a_4;GLboolean a_5;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glDebugMessageControl
(args->a_0,args->a_1,args->a_2,args->a_3,args->a_4,args->a_5);
}
void fexthunks_forward_libGL_glDebugMessageInsert(void *argsv){
struct arg_t {GLenum a_0;GLenum a_1;GLuint a_2;GLenum a_3;GLsizei a_4;const GLchar* a_5;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glDebugMessageInsert
(args->a_0,args->a_1,args->a_2,args->a_3,args->a_4,args->a_5);
}
void fexthunks_forward_libGL_glDeleteBuffers(void *argsv){
struct arg_t {GLsizei a_0;const GLuint* a_1;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glDeleteBuffers
(args->a_0,args->a_1);
}
void fexthunks_forward_libGL_glDeleteFramebuffers(void *argsv){
struct arg_t {GLsizei a_0;const GLuint* a_1;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glDeleteFramebuffers
(args->a_0,args->a_1);
}
void fexthunks_forward_libGL_glDeleteProgram(void *argsv){
struct arg_t {GLuint a_0;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glDeleteProgram
(args->a_0);
}
void fexthunks_forward_libGL_glDeleteProgramPipelines(void *argsv){
struct arg_t {GLsizei a_0;const GLuint* a_1;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glDeleteProgramPipelines
(args->a_0,args->a_1);
}
void fexthunks_forward_libGL_glDeleteRenderbuffers(void *argsv){
struct arg_t {GLsizei a_0;const GLuint* a_1;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glDeleteRenderbuffers
(args->a_0,args->a_1);
}
void fexthunks_forward_libGL_glDeleteShader(void *argsv){
struct arg_t {GLuint a_0;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glDeleteShader
(args->a_0);
}
void fexthunks_forward_libGL_glDeleteVertexArrays(void *argsv){
struct arg_t {GLsizei a_0;const GLuint* a_1;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glDeleteVertexArrays
(args->a_0,args->a_1);
}
void fexthunks_forward_libGL_glDetachShader(void *argsv){
struct arg_t {GLuint a_0;GLuint a_1;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glDetachShader
(args->a_0,args->a_1);
}
void fexthunks_forward_libGL_glDisableVertexAttribArray(void *argsv){
struct arg_t {GLuint a_0;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glDisableVertexAttribArray
(args->a_0);
}
void fexthunks_forward_libGL_glEnableVertexAttribArray(void *argsv){
struct arg_t {GLuint a_0;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glEnableVertexAttribArray
(args->a_0);
}
void fexthunks_forward_libGL_glFramebufferRenderbuffer(void *argsv){
struct arg_t {GLenum a_0;GLenum a_1;GLenum a_2;GLuint a_3;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glFramebufferRenderbuffer
(args->a_0,args->a_1,args->a_2,args->a_3);
}
void fexthunks_forward_libGL_glFramebufferTexture2D(void *argsv){
struct arg_t {GLenum a_0;GLenum a_1;GLenum a_2;GLuint a_3;GLint a_4;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glFramebufferTexture2D
(args->a_0,args->a_1,args->a_2,args->a_3,args->a_4);
}
void fexthunks_forward_libGL_glGenBuffers(void *argsv){
struct arg_t {GLsizei a_0;GLuint* a_1;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glGenBuffers
(args->a_0,args->a_1);
}
void fexthunks_forward_libGL_glGenerateMipmap(void *argsv){
struct arg_t {GLenum a_0;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glGenerateMipmap
(args->a_0);
}
void fexthunks_forward_libGL_glGenerateTextureMipmap(void *argsv){
struct arg_t {GLuint a_0;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glGenerateTextureMipmap
(args->a_0);
}
void fexthunks_forward_libGL_glGenFramebuffers(void *argsv){
struct arg_t {GLsizei a_0;GLuint* a_1;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glGenFramebuffers
(args->a_0,args->a_1);
}
void fexthunks_forward_libGL_glGenProgramPipelines(void *argsv){
struct arg_t {GLsizei a_0;GLuint* a_1;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glGenProgramPipelines
(args->a_0,args->a_1);
}
void fexthunks_forward_libGL_glGenRenderbuffers(void *argsv){
struct arg_t {GLsizei a_0;GLuint* a_1;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glGenRenderbuffers
(args->a_0,args->a_1);
}
void fexthunks_forward_libGL_glGenVertexArrays(void *argsv){
struct arg_t {GLsizei a_0;GLuint* a_1;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glGenVertexArrays
(args->a_0,args->a_1);
}
void fexthunks_forward_libGL_glGetFramebufferAttachmentParameteriv(void *argsv){
struct arg_t {GLenum a_0;GLenum a_1;GLenum a_2;GLint* a_3;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glGetFramebufferAttachmentParameteriv
(args->a_0,args->a_1,args->a_2,args->a_3);
}
void fexthunks_forward_libGL_glGetNamedBufferSubData(void *argsv){
struct arg_t {GLuint a_0;GLintptr a_1;GLsizeiptr a_2;void* a_3;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glGetNamedBufferSubData
(args->a_0,args->a_1,args->a_2,args->a_3);
}
void fexthunks_forward_libGL_glGetProgramBinary(void *argsv){
struct arg_t {GLuint a_0;GLsizei a_1;GLsizei* a_2;GLenum* a_3;void* a_4;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glGetProgramBinary
(args->a_0,args->a_1,args->a_2,args->a_3,args->a_4);
}
void fexthunks_forward_libGL_glGetProgramInfoLog(void *argsv){
struct arg_t {GLuint a_0;GLsizei a_1;GLsizei* a_2;GLchar* a_3;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glGetProgramInfoLog
(args->a_0,args->a_1,args->a_2,args->a_3);
}
void fexthunks_forward_libGL_glGetProgramiv(void *argsv){
struct arg_t {GLuint a_0;GLenum a_1;GLint* a_2;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glGetProgramiv
(args->a_0,args->a_1,args->a_2);
}
void fexthunks_forward_libGL_glGetProgramPipelineInfoLog(void *argsv){
struct arg_t {GLuint a_0;GLsizei a_1;GLsizei* a_2;GLchar* a_3;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glGetProgramPipelineInfoLog
(args->a_0,args->a_1,args->a_2,args->a_3);
}
void fexthunks_forward_libGL_glGetProgramPipelineiv(void *argsv){
struct arg_t {GLuint a_0;GLenum a_1;GLint* a_2;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glGetProgramPipelineiv
(args->a_0,args->a_1,args->a_2);
}
void fexthunks_forward_libGL_glGetShaderInfoLog(void *argsv){
struct arg_t {GLuint a_0;GLsizei a_1;GLsizei* a_2;GLchar* a_3;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glGetShaderInfoLog
(args->a_0,args->a_1,args->a_2,args->a_3);
}
void fexthunks_forward_libGL_glGetShaderiv(void *argsv){
struct arg_t {GLuint a_0;GLenum a_1;GLint* a_2;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glGetShaderiv
(args->a_0,args->a_1,args->a_2);
}
void fexthunks_forward_libGL_glGetShaderSource(void *argsv){
struct arg_t {GLuint a_0;GLsizei a_1;GLsizei* a_2;GLchar* a_3;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glGetShaderSource
(args->a_0,args->a_1,args->a_2,args->a_3);
}
void fexthunks_forward_libGL_glGetTextureImage(void *argsv){
struct arg_t {GLuint a_0;GLint a_1;GLenum a_2;GLenum a_3;GLsizei a_4;void* a_5;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glGetTextureImage
(args->a_0,args->a_1,args->a_2,args->a_3,args->a_4,args->a_5);
}
void fexthunks_forward_libGL_glInvalidateFramebuffer(void *argsv){
struct arg_t {GLenum a_0;GLsizei a_1;const GLenum* a_2;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glInvalidateFramebuffer
(args->a_0,args->a_1,args->a_2);
}
void fexthunks_forward_libGL_glInvalidateNamedFramebufferData(void *argsv){
struct arg_t {GLuint a_0;GLsizei a_1;const GLenum* a_2;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glInvalidateNamedFramebufferData
(args->a_0,args->a_1,args->a_2);
}
void fexthunks_forward_libGL_glLinkProgram(void *argsv){
struct arg_t {GLuint a_0;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glLinkProgram
(args->a_0);
}
void fexthunks_forward_libGL_glNamedFramebufferRenderbuffer(void *argsv){
struct arg_t {GLuint a_0;GLenum a_1;GLenum a_2;GLuint a_3;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glNamedFramebufferRenderbuffer
(args->a_0,args->a_1,args->a_2,args->a_3);
}
void fexthunks_forward_libGL_glNamedFramebufferTexture(void *argsv){
struct arg_t {GLuint a_0;GLenum a_1;GLuint a_2;GLint a_3;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glNamedFramebufferTexture
(args->a_0,args->a_1,args->a_2,args->a_3);
}
void fexthunks_forward_libGL_glNamedRenderbufferStorage(void *argsv){
struct arg_t {GLuint a_0;GLenum a_1;GLsizei a_2;GLsizei a_3;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glNamedRenderbufferStorage
(args->a_0,args->a_1,args->a_2,args->a_3);
}
void fexthunks_forward_libGL_glProgramBinary(void *argsv){
struct arg_t {GLuint a_0;GLenum a_1;const void* a_2;GLsizei a_3;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glProgramBinary
(args->a_0,args->a_1,args->a_2,args->a_3);
}
void fexthunks_forward_libGL_glProgramParameteri(void *argsv){
struct arg_t {GLuint a_0;GLenum a_1;GLint a_2;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glProgramParameteri
(args->a_0,args->a_1,args->a_2);
}
void fexthunks_forward_libGL_glProgramUniform1f(void *argsv){
struct arg_t {GLuint a_0;GLint a_1;GLfloat a_2;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glProgramUniform1f
(args->a_0,args->a_1,args->a_2);
}
void fexthunks_forward_libGL_glProgramUniform1i(void *argsv){
struct arg_t {GLuint a_0;GLint a_1;GLint a_2;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glProgramUniform1i
(args->a_0,args->a_1,args->a_2);
}
void fexthunks_forward_libGL_glProgramUniform2fv(void *argsv){
struct arg_t {GLuint a_0;GLint a_1;GLsizei a_2;const GLfloat* a_3;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glProgramUniform2fv
(args->a_0,args->a_1,args->a_2,args->a_3);
}
void fexthunks_forward_libGL_glProgramUniform3fv(void *argsv){
struct arg_t {GLuint a_0;GLint a_1;GLsizei a_2;const GLfloat* a_3;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glProgramUniform3fv
(args->a_0,args->a_1,args->a_2,args->a_3);
}
void fexthunks_forward_libGL_glProgramUniform4fv(void *argsv){
struct arg_t {GLuint a_0;GLint a_1;GLsizei a_2;const GLfloat* a_3;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glProgramUniform4fv
(args->a_0,args->a_1,args->a_2,args->a_3);
}
void fexthunks_forward_libGL_glProgramUniformMatrix4fv(void *argsv){
struct arg_t {GLuint a_0;GLint a_1;GLsizei a_2;GLboolean a_3;const GLfloat* a_4;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glProgramUniformMatrix4fv
(args->a_0,args->a_1,args->a_2,args->a_3,args->a_4);
}
void fexthunks_forward_libGL_glRenderbufferStorage(void *argsv){
struct arg_t {GLenum a_0;GLenum a_1;GLsizei a_2;GLsizei a_3;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glRenderbufferStorage
(args->a_0,args->a_1,args->a_2,args->a_3);
}
void fexthunks_forward_libGL_glShaderSource(void *argsv){
struct arg_t {GLuint a_0;GLsizei a_1;const GLchar* const* a_2;const GLint* a_3;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glShaderSource
(args->a_0,args->a_1,args->a_2,args->a_3);
}
void fexthunks_forward_libGL_glTexStorage2D(void *argsv){
struct arg_t {GLenum a_0;GLsizei a_1;GLenum a_2;GLsizei a_3;GLsizei a_4;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glTexStorage2D
(args->a_0,args->a_1,args->a_2,args->a_3,args->a_4);
}
void fexthunks_forward_libGL_glTextureParameteri(void *argsv){
struct arg_t {GLuint a_0;GLenum a_1;GLint a_2;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glTextureParameteri
(args->a_0,args->a_1,args->a_2);
}
void fexthunks_forward_libGL_glTextureStorage2D(void *argsv){
struct arg_t {GLuint a_0;GLsizei a_1;GLenum a_2;GLsizei a_3;GLsizei a_4;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glTextureStorage2D
(args->a_0,args->a_1,args->a_2,args->a_3,args->a_4);
}
void fexthunks_forward_libGL_glTextureSubImage2D(void *argsv){
struct arg_t {GLuint a_0;GLint a_1;GLint a_2;GLint a_3;GLsizei a_4;GLsizei a_5;GLenum a_6;GLenum a_7;const void* a_8;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glTextureSubImage2D
(args->a_0,args->a_1,args->a_2,args->a_3,args->a_4,args->a_5,args->a_6,args->a_7,args->a_8);
}
void fexthunks_forward_libGL_glUniform1f(void *argsv){
struct arg_t {GLint a_0;GLfloat a_1;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glUniform1f
(args->a_0,args->a_1);
}
void fexthunks_forward_libGL_glUniform1fv(void *argsv){
struct arg_t {GLint a_0;GLsizei a_1;const GLfloat* a_2;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glUniform1fv
(args->a_0,args->a_1,args->a_2);
}
void fexthunks_forward_libGL_glUniform1i(void *argsv){
struct arg_t {GLint a_0;GLint a_1;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glUniform1i
(args->a_0,args->a_1);
}
void fexthunks_forward_libGL_glUniform1iv(void *argsv){
struct arg_t {GLint a_0;GLsizei a_1;const GLint* a_2;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glUniform1iv
(args->a_0,args->a_1,args->a_2);
}
void fexthunks_forward_libGL_glUniform2f(void *argsv){
struct arg_t {GLint a_0;GLfloat a_1;GLfloat a_2;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glUniform2f
(args->a_0,args->a_1,args->a_2);
}
void fexthunks_forward_libGL_glUniform2fv(void *argsv){
struct arg_t {GLint a_0;GLsizei a_1;const GLfloat* a_2;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glUniform2fv
(args->a_0,args->a_1,args->a_2);
}
void fexthunks_forward_libGL_glUniform2i(void *argsv){
struct arg_t {GLint a_0;GLint a_1;GLint a_2;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glUniform2i
(args->a_0,args->a_1,args->a_2);
}
void fexthunks_forward_libGL_glUniform2iv(void *argsv){
struct arg_t {GLint a_0;GLsizei a_1;const GLint* a_2;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glUniform2iv
(args->a_0,args->a_1,args->a_2);
}
void fexthunks_forward_libGL_glUniform3f(void *argsv){
struct arg_t {GLint a_0;GLfloat a_1;GLfloat a_2;GLfloat a_3;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glUniform3f
(args->a_0,args->a_1,args->a_2,args->a_3);
}
void fexthunks_forward_libGL_glUniform3fv(void *argsv){
struct arg_t {GLint a_0;GLsizei a_1;const GLfloat* a_2;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glUniform3fv
(args->a_0,args->a_1,args->a_2);
}
void fexthunks_forward_libGL_glUniform3i(void *argsv){
struct arg_t {GLint a_0;GLint a_1;GLint a_2;GLint a_3;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glUniform3i
(args->a_0,args->a_1,args->a_2,args->a_3);
}
void fexthunks_forward_libGL_glUniform3iv(void *argsv){
struct arg_t {GLint a_0;GLsizei a_1;const GLint* a_2;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glUniform3iv
(args->a_0,args->a_1,args->a_2);
}
void fexthunks_forward_libGL_glUniform4f(void *argsv){
struct arg_t {GLint a_0;GLfloat a_1;GLfloat a_2;GLfloat a_3;GLfloat a_4;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glUniform4f
(args->a_0,args->a_1,args->a_2,args->a_3,args->a_4);
}
void fexthunks_forward_libGL_glUniform4fv(void *argsv){
struct arg_t {GLint a_0;GLsizei a_1;const GLfloat* a_2;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glUniform4fv
(args->a_0,args->a_1,args->a_2);
}
void fexthunks_forward_libGL_glUniform4i(void *argsv){
struct arg_t {GLint a_0;GLint a_1;GLint a_2;GLint a_3;GLint a_4;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glUniform4i
(args->a_0,args->a_1,args->a_2,args->a_3,args->a_4);
}
void fexthunks_forward_libGL_glUniform4iv(void *argsv){
struct arg_t {GLint a_0;GLsizei a_1;const GLint* a_2;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glUniform4iv
(args->a_0,args->a_1,args->a_2);
}
void fexthunks_forward_libGL_glUniformMatrix2fv(void *argsv){
struct arg_t {GLint a_0;GLsizei a_1;GLboolean a_2;const GLfloat* a_3;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glUniformMatrix2fv
(args->a_0,args->a_1,args->a_2,args->a_3);
}
void fexthunks_forward_libGL_glUniformMatrix3fv(void *argsv){
struct arg_t {GLint a_0;GLsizei a_1;GLboolean a_2;const GLfloat* a_3;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glUniformMatrix3fv
(args->a_0,args->a_1,args->a_2,args->a_3);
}
void fexthunks_forward_libGL_glUniformMatrix4fv(void *argsv){
struct arg_t {GLint a_0;GLsizei a_1;GLboolean a_2;const GLfloat* a_3;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glUniformMatrix4fv
(args->a_0,args->a_1,args->a_2,args->a_3);
}
void fexthunks_forward_libGL_glUseProgram(void *argsv){
struct arg_t {GLuint a_0;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glUseProgram
(args->a_0);
}
void fexthunks_forward_libGL_glUseProgramStages(void *argsv){
struct arg_t {GLuint a_0;GLbitfield a_1;GLuint a_2;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glUseProgramStages
(args->a_0,args->a_1,args->a_2);
}
void fexthunks_forward_libGL_glValidateProgram(void *argsv){
struct arg_t {GLuint a_0;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glValidateProgram
(args->a_0);
}
void fexthunks_forward_libGL_glValidateProgramPipeline(void *argsv){
struct arg_t {GLuint a_0;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glValidateProgramPipeline
(args->a_0);
}
void fexthunks_forward_libGL_glVertexAttribIPointer(void *argsv){
struct arg_t {GLuint a_0;GLint a_1;GLenum a_2;GLsizei a_3;const void* a_4;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glVertexAttribIPointer
(args->a_0,args->a_1,args->a_2,args->a_3,args->a_4);
}
void fexthunks_forward_libGL_glVertexAttribPointer(void *argsv){
struct arg_t {GLuint a_0;GLint a_1;GLenum a_2;GLboolean a_3;GLsizei a_4;const void* a_5;};
auto args = (arg_t*)argsv;
fexthunks_impl_libGL_glVertexAttribPointer
(args->a_0,args->a_1,args->a_2,args->a_3,args->a_4,args->a_5);
}
void fexthunks_forward_libX11_XOpenDisplay(void *argsv){
struct arg_t {const char* a_0;Display* rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XOpenDisplay
(args->a_0);
}
void fexthunks_forward_libX11_XCreateColormap(void *argsv){
struct arg_t {Display* a_0;Window a_1;Visual* a_2;int a_3;Colormap rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XCreateColormap
(args->a_0,args->a_1,args->a_2,args->a_3);
}
void fexthunks_forward_libX11_XCreateWindow(void *argsv){
struct arg_t {Display* a_0;Window a_1;int a_2;int a_3;unsigned int a_4;unsigned int a_5;unsigned int a_6;int a_7;unsigned int a_8;Visual* a_9;long unsigned int a_10;XSetWindowAttributes* a_11;Window rv;};
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
struct arg_t {Display* a_0;Window a_1;Atom a_2;Atom a_3;int a_4;int a_5;const unsigned char* a_6;int a_7;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XChangeProperty
(args->a_0,args->a_1,args->a_2,args->a_3,args->a_4,args->a_5,args->a_6,args->a_7);
}
void fexthunks_forward_libX11_XCloseDisplay(void *argsv){
struct arg_t {Display* a_0;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XCloseDisplay
(args->a_0);
}
void fexthunks_forward_libX11_XDestroyWindow(void *argsv){
struct arg_t {Display* a_0;Window a_1;int rv;};
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
struct arg_t {Display* a_0;const char* a_1;int a_2;Atom rv;};
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
struct arg_t {XKeyEvent* a_0;char* a_1;int a_2;KeySym* a_3;XComposeStatus* a_4;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XLookupString
(args->a_0,args->a_1,args->a_2,args->a_3,args->a_4);
}
void fexthunks_forward_libX11_XNextEvent(void *argsv){
struct arg_t {Display* a_0;XEvent* a_1;int rv;};
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
struct arg_t {Display* a_0;int rv;};
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
void fexthunks_forward_libX11_XListExtensions(void *argsv){
struct arg_t {Display* a_0;int* a_1;char** rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XListExtensions
(args->a_0,args->a_1);
}
void fexthunks_forward_libX11_XSetLocaleModifiers(void *argsv){
struct arg_t {const char* a_0;char* rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XSetLocaleModifiers
(args->a_0);
}
void fexthunks_forward_libX11_XCreatePixmapCursor(void *argsv){
struct arg_t {Display* a_0;Pixmap a_1;Pixmap a_2;XColor* a_3;XColor* a_4;unsigned int a_5;unsigned int a_6;Cursor rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XCreatePixmapCursor
(args->a_0,args->a_1,args->a_2,args->a_3,args->a_4,args->a_5,args->a_6);
}
void fexthunks_forward_libX11_XCloseIM(void *argsv){
struct arg_t {XIM a_0;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XCloseIM
(args->a_0);
}
void fexthunks_forward_libX11_XDefineCursor(void *argsv){
struct arg_t {Display* a_0;Window a_1;Cursor a_2;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XDefineCursor
(args->a_0,args->a_1,args->a_2);
}
void fexthunks_forward_libX11_XDrawString16(void *argsv){
struct arg_t {Display* a_0;Drawable a_1;GC a_2;int a_3;int a_4;const XChar2b* a_5;int a_6;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XDrawString16
(args->a_0,args->a_1,args->a_2,args->a_3,args->a_4,args->a_5,args->a_6);
}
void fexthunks_forward_libX11_XEventsQueued(void *argsv){
struct arg_t {Display* a_0;int a_1;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XEventsQueued
(args->a_0,args->a_1);
}
void fexthunks_forward_libX11_XFillRectangle(void *argsv){
struct arg_t {Display* a_0;Drawable a_1;GC a_2;int a_3;int a_4;unsigned int a_5;unsigned int a_6;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XFillRectangle
(args->a_0,args->a_1,args->a_2,args->a_3,args->a_4,args->a_5,args->a_6);
}
void fexthunks_forward_libX11_XFilterEvent(void *argsv){
struct arg_t {XEvent* a_0;Window a_1;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XFilterEvent
(args->a_0,args->a_1);
}
void fexthunks_forward_libX11_XFlush(void *argsv){
struct arg_t {Display* a_0;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XFlush
(args->a_0);
}
void fexthunks_forward_libX11_XFreeColormap(void *argsv){
struct arg_t {Display* a_0;Colormap a_1;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XFreeColormap
(args->a_0,args->a_1);
}
void fexthunks_forward_libX11_XFreeCursor(void *argsv){
struct arg_t {Display* a_0;Cursor a_1;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XFreeCursor
(args->a_0,args->a_1);
}
void fexthunks_forward_libX11_XFreeExtensionList(void *argsv){
struct arg_t {char** a_0;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XFreeExtensionList
(args->a_0);
}
void fexthunks_forward_libX11_XFreeFont(void *argsv){
struct arg_t {Display* a_0;XFontStruct* a_1;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XFreeFont
(args->a_0,args->a_1);
}
void fexthunks_forward_libX11_XFreeGC(void *argsv){
struct arg_t {Display* a_0;GC a_1;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XFreeGC
(args->a_0,args->a_1);
}
void fexthunks_forward_libX11_XFreePixmap(void *argsv){
struct arg_t {Display* a_0;Pixmap a_1;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XFreePixmap
(args->a_0,args->a_1);
}
void fexthunks_forward_libX11_XGetErrorDatabaseText(void *argsv){
struct arg_t {Display* a_0;const char* a_1;const char* a_2;const char* a_3;char* a_4;int a_5;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XGetErrorDatabaseText
(args->a_0,args->a_1,args->a_2,args->a_3,args->a_4,args->a_5);
}
void fexthunks_forward_libX11_XGetErrorText(void *argsv){
struct arg_t {Display* a_0;int a_1;char* a_2;int a_3;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XGetErrorText
(args->a_0,args->a_1,args->a_2,args->a_3);
}
void fexthunks_forward_libX11_XGetEventData(void *argsv){
struct arg_t {Display* a_0;XGenericEventCookie* a_1;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XGetEventData
(args->a_0,args->a_1);
}
void fexthunks_forward_libX11_XGetWindowProperty(void *argsv){
struct arg_t {Display* a_0;Window a_1;Atom a_2;long int a_3;long int a_4;int a_5;Atom a_6;Atom* a_7;int* a_8;long unsigned int* a_9;long unsigned int* a_10;unsigned char** a_11;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XGetWindowProperty
(args->a_0,args->a_1,args->a_2,args->a_3,args->a_4,args->a_5,args->a_6,args->a_7,args->a_8,args->a_9,args->a_10,args->a_11);
}
void fexthunks_forward_libX11_XGrabPointer(void *argsv){
struct arg_t {Display* a_0;Window a_1;int a_2;unsigned int a_3;int a_4;int a_5;Window a_6;Cursor a_7;Time a_8;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XGrabPointer
(args->a_0,args->a_1,args->a_2,args->a_3,args->a_4,args->a_5,args->a_6,args->a_7,args->a_8);
}
void fexthunks_forward_libX11_XGrabServer(void *argsv){
struct arg_t {Display* a_0;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XGrabServer
(args->a_0);
}
void fexthunks_forward_libX11_XIconifyWindow(void *argsv){
struct arg_t {Display* a_0;Window a_1;int a_2;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XIconifyWindow
(args->a_0,args->a_1,args->a_2);
}
void fexthunks_forward_libX11_XIfEvent(void *argsv){
struct arg_t {Display* a_0;XEvent* a_1;XIfEventFN* a_2;XPointer a_3;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XIfEvent
(args->a_0,args->a_1,args->a_2,args->a_3);
}
void fexthunks_forward_libX11_XInitThreads(void *argsv){
struct arg_t {int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XInitThreads
();
}
void fexthunks_forward_libX11_XMapRaised(void *argsv){
struct arg_t {Display* a_0;Window a_1;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XMapRaised
(args->a_0,args->a_1);
}
void fexthunks_forward_libX11_XMoveResizeWindow(void *argsv){
struct arg_t {Display* a_0;Window a_1;int a_2;int a_3;unsigned int a_4;unsigned int a_5;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XMoveResizeWindow
(args->a_0,args->a_1,args->a_2,args->a_3,args->a_4,args->a_5);
}
void fexthunks_forward_libX11_XMoveWindow(void *argsv){
struct arg_t {Display* a_0;Window a_1;int a_2;int a_3;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XMoveWindow
(args->a_0,args->a_1,args->a_2,args->a_3);
}
void fexthunks_forward_libX11_XPeekEvent(void *argsv){
struct arg_t {Display* a_0;XEvent* a_1;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XPeekEvent
(args->a_0,args->a_1);
}
void fexthunks_forward_libX11_XQueryExtension(void *argsv){
struct arg_t {Display* a_0;const char* a_1;int* a_2;int* a_3;int* a_4;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XQueryExtension
(args->a_0,args->a_1,args->a_2,args->a_3,args->a_4);
}
void fexthunks_forward_libX11_XQueryPointer(void *argsv){
struct arg_t {Display* a_0;Window a_1;Window* a_2;Window* a_3;int* a_4;int* a_5;int* a_6;int* a_7;unsigned int* a_8;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XQueryPointer
(args->a_0,args->a_1,args->a_2,args->a_3,args->a_4,args->a_5,args->a_6,args->a_7,args->a_8);
}
void fexthunks_forward_libX11_XQueryTree(void *argsv){
struct arg_t {Display* a_0;Window a_1;Window* a_2;Window* a_3;Window** a_4;unsigned int* a_5;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XQueryTree
(args->a_0,args->a_1,args->a_2,args->a_3,args->a_4,args->a_5);
}
void fexthunks_forward_libX11_XResetScreenSaver(void *argsv){
struct arg_t {Display* a_0;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XResetScreenSaver
(args->a_0);
}
void fexthunks_forward_libX11_XResizeWindow(void *argsv){
struct arg_t {Display* a_0;Window a_1;unsigned int a_2;unsigned int a_3;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XResizeWindow
(args->a_0,args->a_1,args->a_2,args->a_3);
}
void fexthunks_forward_libX11_XSelectInput(void *argsv){
struct arg_t {Display* a_0;Window a_1;long int a_2;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XSelectInput
(args->a_0,args->a_1,args->a_2);
}
void fexthunks_forward_libX11_XSendEvent(void *argsv){
struct arg_t {Display* a_0;Window a_1;int a_2;long int a_3;XEvent* a_4;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XSendEvent
(args->a_0,args->a_1,args->a_2,args->a_3,args->a_4);
}
void fexthunks_forward_libX11_XSetErrorHandler(void *argsv){
struct arg_t {XErrorHandler a_0;XSetErrorHandlerFN* rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XSetErrorHandler
(args->a_0);
}
void fexthunks_forward_libX11_XSetTransientForHint(void *argsv){
struct arg_t {Display* a_0;Window a_1;Window a_2;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XSetTransientForHint
(args->a_0,args->a_1,args->a_2);
}
void fexthunks_forward_libX11_XSetWMProtocols(void *argsv){
struct arg_t {Display* a_0;Window a_1;Atom* a_2;int a_3;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XSetWMProtocols
(args->a_0,args->a_1,args->a_2,args->a_3);
}
void fexthunks_forward_libX11_XSync(void *argsv){
struct arg_t {Display* a_0;int a_1;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XSync
(args->a_0,args->a_1);
}
void fexthunks_forward_libX11_XTextExtents16(void *argsv){
struct arg_t {XFontStruct* a_0;const XChar2b* a_1;int a_2;int* a_3;int* a_4;int* a_5;XCharStruct* a_6;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XTextExtents16
(args->a_0,args->a_1,args->a_2,args->a_3,args->a_4,args->a_5,args->a_6);
}
void fexthunks_forward_libX11_XTranslateCoordinates(void *argsv){
struct arg_t {Display* a_0;Window a_1;Window a_2;int a_3;int a_4;int* a_5;int* a_6;Window* a_7;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XTranslateCoordinates
(args->a_0,args->a_1,args->a_2,args->a_3,args->a_4,args->a_5,args->a_6,args->a_7);
}
void fexthunks_forward_libX11_XUngrabPointer(void *argsv){
struct arg_t {Display* a_0;Time a_1;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XUngrabPointer
(args->a_0,args->a_1);
}
void fexthunks_forward_libX11_XUngrabServer(void *argsv){
struct arg_t {Display* a_0;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XUngrabServer
(args->a_0);
}
void fexthunks_forward_libX11_XUnmapWindow(void *argsv){
struct arg_t {Display* a_0;Window a_1;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XUnmapWindow
(args->a_0,args->a_1);
}
void fexthunks_forward_libX11_Xutf8LookupString(void *argsv){
struct arg_t {XIC a_0;XKeyPressedEvent* a_1;char* a_2;int a_3;KeySym* a_4;int* a_5;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_Xutf8LookupString
(args->a_0,args->a_1,args->a_2,args->a_3,args->a_4,args->a_5);
}
void fexthunks_forward_libX11_XWarpPointer(void *argsv){
struct arg_t {Display* a_0;Window a_1;Window a_2;int a_3;int a_4;unsigned int a_5;unsigned int a_6;int a_7;int a_8;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XWarpPointer
(args->a_0,args->a_1,args->a_2,args->a_3,args->a_4,args->a_5,args->a_6,args->a_7,args->a_8);
}
void fexthunks_forward_libX11_XWindowEvent(void *argsv){
struct arg_t {Display* a_0;Window a_1;long int a_2;XEvent* a_3;int rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XWindowEvent
(args->a_0,args->a_1,args->a_2,args->a_3);
}
void fexthunks_forward_libX11_XCreateBitmapFromData(void *argsv){
struct arg_t {Display* a_0;Drawable a_1;const char* a_2;unsigned int a_3;unsigned int a_4;Pixmap rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XCreateBitmapFromData
(args->a_0,args->a_1,args->a_2,args->a_3,args->a_4);
}
void fexthunks_forward_libX11_XCreatePixmap(void *argsv){
struct arg_t {Display* a_0;Drawable a_1;unsigned int a_2;unsigned int a_3;unsigned int a_4;Pixmap rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XCreatePixmap
(args->a_0,args->a_1,args->a_2,args->a_3,args->a_4);
}
void fexthunks_forward_libX11_XDestroyIC(void *argsv){
struct arg_t {XIC a_0;};
auto args = (arg_t*)argsv;
fexthunks_impl_libX11_XDestroyIC
(args->a_0);
}
void fexthunks_forward_libX11_XFreeEventData(void *argsv){
struct arg_t {Display* a_0;XGenericEventCookie* a_1;};
auto args = (arg_t*)argsv;
fexthunks_impl_libX11_XFreeEventData
(args->a_0,args->a_1);
}
void fexthunks_forward_libX11_XLockDisplay(void *argsv){
struct arg_t {Display* a_0;};
auto args = (arg_t*)argsv;
fexthunks_impl_libX11_XLockDisplay
(args->a_0);
}
void fexthunks_forward_libX11_XSetICFocus(void *argsv){
struct arg_t {XIC a_0;};
auto args = (arg_t*)argsv;
fexthunks_impl_libX11_XSetICFocus
(args->a_0);
}
void fexthunks_forward_libX11_XSetWMNormalHints(void *argsv){
struct arg_t {Display* a_0;Window a_1;XSizeHints* a_2;};
auto args = (arg_t*)argsv;
fexthunks_impl_libX11_XSetWMNormalHints
(args->a_0,args->a_1,args->a_2);
}
void fexthunks_forward_libX11_XUnlockDisplay(void *argsv){
struct arg_t {Display* a_0;};
auto args = (arg_t*)argsv;
fexthunks_impl_libX11_XUnlockDisplay
(args->a_0);
}
void fexthunks_forward_libX11_Xutf8SetWMProperties(void *argsv){
struct arg_t {Display* a_0;Window a_1;const char* a_2;const char* a_3;char** a_4;int a_5;XSizeHints* a_6;XWMHints* a_7;XClassHint* a_8;};
auto args = (arg_t*)argsv;
fexthunks_impl_libX11_Xutf8SetWMProperties
(args->a_0,args->a_1,args->a_2,args->a_3,args->a_4,args->a_5,args->a_6,args->a_7,args->a_8);
}
void fexthunks_forward_libX11_XLoadQueryFont(void *argsv){
struct arg_t {Display* a_0;const char* a_1;XFontStruct* rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XLoadQueryFont
(args->a_0,args->a_1);
}
void fexthunks_forward_libX11_XCreateGC(void *argsv){
struct arg_t {Display* a_0;Drawable a_1;long unsigned int a_2;XGCValues* a_3;_XGC* rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XCreateGC
(args->a_0,args->a_1,args->a_2,args->a_3);
}
void fexthunks_forward_libX11_XGetImage(void *argsv){
struct arg_t {Display* a_0;Drawable a_1;int a_2;int a_3;unsigned int a_4;unsigned int a_5;long unsigned int a_6;int a_7;XImage* rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XGetImage
(args->a_0,args->a_1,args->a_2,args->a_3,args->a_4,args->a_5,args->a_6,args->a_7);
}
void fexthunks_forward_libX11_XOpenIM(void *argsv){
struct arg_t {Display* a_0;_XrmHashBucketRec* a_1;char* a_2;char* a_3;_XIM* rv;};
auto args = (arg_t*)argsv;
args->rv = 
fexthunks_impl_libX11_XOpenIM
(args->a_0,args->a_1,args->a_2,args->a_3);
}
