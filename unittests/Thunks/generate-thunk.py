#!/usr/bin/python3
import sys
import re

Libs = { }
CurrentLib = None
CurrentFunction = None

def lib(name):
    global Libs
    global CurrentLib
    global CurrentFunction

    Libs[name] = {
        "name": name,
        "functions": { }
    }
    CurrentLib = Libs[name]
    CurrentFunction = None

def function(name):
    global Libs
    global CurrentLib
    global CurrentFunction

    CurrentLib["functions"][name] = {
        "name" : name,
        "return": "void",
        "args": []
    }
    CurrentFunction = CurrentLib["functions"][name]

def returns(type):
    global Libs
    global CurrentLib
    global CurrentFunction

    CurrentFunction["return"] = type

def arg(type):
    global Libs
    global CurrentLib
    global CurrentFunction

    CurrentFunction["args"].append(type)

def args(types):
    arr = types.split(",")
    for type in arr:
        type = type.strip()
        arg(type)

def fn(cdecl):
    parts = re.findall("([^(]+)\(([^)]*)\)", cdecl)[0]
    fn_name = parts[0].replace("*", " ").split(" ")[-1]
    fn_rv = parts[0][0:-len(fn_name)]
    fn_args = parts[1].strip()

    function(fn_name.strip())
    returns(fn_rv.strip())
    if (len(fn_args)):
        args(fn_args)


def GenerateThunk_args(args):
    rv = [ ]
    rv.append("(")
    for i in range(len(args)):
        if i != 0:
            rv.append(",")
        rv.append(args[i] + " a_" + str(i))
    rv.append(")")

    return "".join(rv)

def GenerateThunk_struct(returns, args):
    rv = [ ]
    rv.append("{")
    for i in range(len(args)):
        rv.append(args[i] + " a_" + str(i) + ";")
    if returns != "void":
        rv.append(returns + " rv;")
    rv.append("}")
    return "".join(rv)

def GenerateThunk_args_assignment(args):
    rv = [ ]
    for i in range(len(args)):
        rv.append("args.a_" + str(i) + " = a_" + str(i) + ";")
    return "".join(rv)

def GenerateThunk_function(lib, function):
    print("MAKE_THUNK(" + lib["name"] + ", " + function["name"] + ")")
    print("")
    print(function["return"] + " " + function["name"] + GenerateThunk_args(function["args"]) + "{")
    print("struct " + GenerateThunk_struct(function["return"], function["args"]) + " args;")
    print(GenerateThunk_args_assignment(function["args"]))
    print("fexthunks_" + lib["name"] + "_" + function["name"] + "(&args);")

    if function["return"] != "void":
        print("return args.rv;")
    
    print("}")
    
    print("")
    
def GenerateThunk_lib(lib):
    for function in lib["functions"].values():
        GenerateThunk_function(lib, function)

def GenerateThunks(libs):
    for lib in libs.values():
        GenerateThunk_lib(lib)

def GenerateForward_args(args):
    rv = [ ]
    rv.append("(")
    for i in range(len(args)):
        if i != 0:
            rv.append(",")
        rv.append("args->a_" + str(i))
    rv.append(")")
    
    return "".join(rv)

def GenerateForward_function(lib, function):
    print("void fexthunks_forward_" + lib["name"] + "_" + function["name"] + "(void *argsv){")
    print("struct arg_t "+ GenerateThunk_struct(function["return"], function["args"]) + ";")
    print("auto args = (arg_t*)argsv;")

    if function["return"] != "void":
        print("args->rv = ");
    print("fexthunks_impl_" + lib["name"] + "_" + function["name"])
    print(GenerateForward_args(function["args"]) + ";")
    print("}")

def GenerateForward_lib(lib):
    for function in lib["functions"].values():
        GenerateForward_function(lib, function)

def GenerateForwards(libs):
    for lib in libs.values():
        GenerateForward_lib(lib)

def GenerateInitializer_function(lib, function):
    print("typedef " + function["return"]  + " fexthunks_type_" + lib["name"] + "_" + function["name"] + GenerateThunk_args(function["args"]) + ";")
    print("fexthunks_type_" + lib["name"] + "_" + function["name"] + " *fexthunks_impl_" + lib["name"] + "_" + function["name"] + ";")

def GenerateInitializer_function_loader(lib, function, handle):
    print("(void*&)fexthunks_impl_" + lib["name"] + "_" + function["name"] + " = dlsym(" + handle + ', "' + function["name"] + '");')

def GenerateInitializer_lib(lib):
    handle =  "fexthunks_impl_" + lib["name"] + "_so"
    print("void* " + handle + ";")
    
    for function in lib["functions"].values():
        GenerateInitializer_function(lib, function)

    print("bool fexthunks_init_" + lib["name"] + "() {")
    print(handle + " = dlopen(\""+ lib["name"] +".so\", RTLD_LOCAL | RTLD_LAZY);");
    print("if (!" + handle + ") { return false; }");
    for function in lib["functions"].values():
        GenerateInitializer_function_loader(lib, function, handle)
    print("return true;")
    print("}")

def GenerateInitializers(libs):
    for lib in libs.values():
        GenerateInitializer_lib(lib)

def GenerateThunkmap_function(lib, function):
    print("thunks[\"" + lib["name"]  + ":" + function["name"] + "\"] = &fexthunks_forward_" + lib["name"]  + "_" + function["name"] + ";")

def GenerateThunkmap_lib(lib):
    for function in lib["functions"].values():
        GenerateThunkmap_function(lib, function)

def GenerateThunkmap(libs):
    for lib in libs.values():
        GenerateThunkmap_lib(lib)

def GenerateSymtab_function(lib, function):
    print("{\"" + lib["name"]  + ":" + function["name"] + "\", (voidFunc*)&" + function["name"] + "},")

def GenerateSymtab_lib(lib):
    for function in lib["functions"].values():
        GenerateSymtab_function(lib, function)

def GenerateSymtab(libs):
    for lib in libs.values():
        GenerateSymtab_lib(lib)

# define libs here
lib("libGL")

#XVisualInfo* glXChooseVisual( Display *dpy, int screen, int *attribList );
function("glXChooseVisual")
returns("XVisualInfo *")
arg("Display *")
arg("int")
arg("int *")

#GLXContext glXCreateContext( Display *dpy, XVisualInfo *vis, GLXContext shareList, Bool direct );
function("glXCreateContext")
returns("GLXContext")
arg("Display *")
arg("XVisualInfo *")
arg("GLXContext")
arg("Bool")

#void glXDestroyContext( Display *dpy, GLXContext ctx );
function("glXDestroyContext")
returns("void")
arg("Display *")
arg("GLXContext")

#Bool glXMakeCurrent( Display *dpy, GLXDrawable drawable, GLXContext ctx);
function("glXMakeCurrent")
returns("Bool")
arg("Display *")
arg("GLXDrawable")
arg("GLXContext")

#void glXCopyContext( Display *dpy, GLXContext src, GLXContext dst, GLuint mask );
function("glXCopyContext")
returns("void")
arg("Display *")
arg("GLXContext")
arg("GLXContext")
arg("long unsigned int")

#void glXSwapBuffers( Display *dpy, GLXDrawable drawable );
function("glXSwapBuffers")
returns("void")
arg("Display *")
arg("GLXDrawable")

#GLXPixmap glXCreateGLXPixmap( Display *dpy, XVisualInfo *visual, Pixmap pixmap );
function("glXCreateGLXPixmap")
returns("GLXPixmap")
arg("Display *")
arg("XVisualInfo *")
arg("Pixmap")

#void glXDestroyGLXPixmap( Display *dpy, GLXPixmap pixmap );
function("glXDestroyGLXPixmap")
returns("void")
arg("Display *")
arg("GLXPixmap")

#Bool glXQueryExtension( Display *dpy, int *errorb, int *event );
function("glXQueryExtension")
returns("Bool")
arg("Display *")
arg("int *")
arg("int *")

#Bool glXQueryVersion( Display *dpy, int *maj, int *min );
function("glXQueryVersion")
returns("Bool")
arg("Display *")
arg("int *")
arg("int *")

#Bool glXIsDirect( Display *dpy, GLXContext ctx );
function("glXIsDirect")
returns("Bool")
arg("Display *")
arg("GLXContext")

#int glXGetConfig( Display *dpy, XVisualInfo *visual, int attrib, int *value );
function("glXGetConfig")
returns("int")
arg("Display *")
arg("XVisualInfo *")
arg("int")
arg("int *")

#GLXContext glXGetCurrentContext( void );
function("glXGetCurrentContext")
returns("GLXContext")

#GLXDrawable glXGetCurrentDrawable( void );
function("glXGetCurrentDrawable")
returns("GLXDrawable")

#void glXWaitGL( void );
function("glXWaitGL")
returns("void")

#void glXWaitX( void );
function("glXWaitX")
returns("void")

#void glXUseXFont( Font font, int first, int count, int list );
function("glXUseXFont")
returns("void")
arg("Font")
arg("int")
arg("int")
arg("int")

#const char *glXQueryExtensionsString( Display *dpy, int screen );
function("glXQueryExtensionsString")
returns("const char *")
arg("Display *")
arg("int")

#const char *glXQueryServerString( Display *dpy, int screen, int name );
function("glXQueryServerString")
returns("const char *")
arg("Display *")
arg("int")
arg("int")

#const char *glXGetClientString( Display *dpy, int name );
function("glXGetClientString")
returns("const char *")
arg("Display *")
arg("int")

#GLXContext (*GLXCREATECONTEXTATTRIBSARBPROC)(Display*, GLXFBConfig, GLXContext, Bool, const int*);
function("glXCreateContextAttribsARB")
returns("GLXContext")
arg("Display *")
arg("GLXFBConfig")
arg("GLXContext");
arg("Bool");
arg("const int *")


#GLXFBConfig * glXChooseFBConfig(	Display * dpy, int screen, const int * attrib_list, int * nelements);
function("glXChooseFBConfig")
returns("GLXFBConfig *")
arg("Display *")
arg("int")
arg("const int *")
arg("int *")

function("glXQueryDrawable")
args("Display*, GLXDrawable, int, unsigned int*")

function("glXGetSwapIntervalMESA")
returns("int")
arg("unsigned int")

# some GL ones

#void glClearColor(	GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
function("glClearColor")
returns("void")
arg("GLclampf")
arg("GLclampf")
arg("GLclampf")
arg("GLclampf")

#void glClear(	GLbitfield mask);
function("glClear")
returns("void")
arg("GLbitfield")

function("glBegin")
returns("void")
args("GLenum")

function("glCallList")
returns("void")
args("GLuint")

function("glDeleteLists")
returns("void")
args("GLuint")
args("GLsizei")

function("glDrawBuffer")
returns("void")
args("GLenum")

function("glEnable")
returns("void")
args("GLenum")

function("glFrustum")
returns("void")
args("GLdouble")
args("GLdouble")
args("GLdouble")
args("GLdouble")
args("GLdouble")
args("GLdouble")

function("glGenLists")
returns("GLuint")
args("GLsizei")

function("glGetString")
returns("const GLubyte *")
args("GLenum")

function("glLightfv")
returns("void")
args("GLenum")
args("GLenum")
args("const GLfloat*")

function("glMaterialfv")
returns("void")
args("GLenum, GLenum, const GLfloat*")

function("glMatrixMode")
returns("void")
args("GLenum")

function("glNewList")
returns("void")
args("GLuint, GLenum")

function("glNormal3f")
returns("void")
args("GLfloat, GLfloat, GLfloat")

function("glRotatef")
returns("void")
args("GLfloat, GLfloat, GLfloat, GLfloat")

function("glShadeModel")
returns("void")
args("GLenum")

function("glTranslated")
returns("void")
args("GLdouble, GLdouble, GLdouble")

function("glTranslatef")
returns("void")
args("GLfloat, GLfloat, GLfloat")

function("glVertex3f")
returns("void")
args("GLfloat, GLfloat, GLfloat")

function("glViewport")
returns("void")
args("GLint, GLint, GLsizei, GLsizei")

function("glEnd")
function("glEndList")
function("glLoadIdentity")
function("glPopMatrix")
function("glPushMatrix")

fn("const char* glXQueryExtensionsString(Display*, int)")
fn("const GLubyte* glGetString(GLenum)")
fn("GLboolean glIsEnabled(GLenum)")
fn("GLenum glGetError()")
fn("__GLXcontextRec* glXCreateNewContext(Display*, GLXFBConfig, int, GLXContext, int)")
fn("__GLXcontextRec* glXGetCurrentContext()")
fn("__GLXFBConfigRec** glXChooseFBConfig(Display*, int, const int*, int*)")
fn("GLXWindow glXCreateWindow(Display*, GLXFBConfig, Window, const int*)")
fn("int glXMakeContextCurrent(Display*, GLXDrawable, GLXDrawable, GLXContext)")
fn("int glXQueryExtension(Display*, int*, int*)")
fn("void glActiveTexture(GLenum)")
fn("void glBindTexture(GLenum, GLuint)")
fn("void glBlendColor(GLclampf, GLclampf, GLclampf, GLclampf)")
fn("void glBlendEquation(GLenum)")
fn("void glBlendFunc(GLenum, GLenum)")
fn("void glClearColor(GLclampf, GLclampf, GLclampf, GLclampf)")
fn("void glClearDepth(GLclampd)")
fn("void glClear(GLbitfield)")
fn("void glClearStencil(GLint)")
fn("void glColorMask(GLboolean, GLboolean, GLboolean, GLboolean)")
fn("void glCompressedTexImage2D(GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, const GLvoid*)")
fn("void glCompressedTexSubImage2D(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLsizei, const GLvoid*)")
fn("void glCopyTexImage2D(GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLsizei, GLint)")
fn("void glCopyTexSubImage2D(GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei)")
fn("void glCullFace(GLenum)")
fn("void glDeleteTextures(GLsizei, const GLuint*)")
fn("void glDepthFunc(GLenum)")
fn("void glDepthMask(GLboolean)")
fn("void glDepthRange(GLclampd, GLclampd)")
fn("void glDisable(GLenum)")
fn("void glDrawArrays(GLenum, GLint, GLsizei)")
fn("void glDrawElements(GLenum, GLsizei, GLenum, const GLvoid*)")
fn("void glEnable(GLenum)")
fn("void glFrontFace(GLenum)")
fn("void glGenTextures(GLsizei, GLuint*)")
fn("void glGetFloatv(GLenum, GLfloat*)")
fn("void glGetIntegerv(GLenum, GLint*)")
fn("void glGetTexImage(GLenum, GLint, GLenum, GLenum, GLvoid*)")
fn("void glGetTexLevelParameterfv(GLenum, GLint, GLenum, GLfloat*)")
fn("void glPixelStorei(GLenum, GLint)")
fn("void glRasterPos2i(GLint, GLint)")
fn("void glReadPixels(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLvoid*)")
fn("void glScissor(GLint, GLint, GLsizei, GLsizei)")
fn("void glStencilFunc(GLenum, GLint, GLuint)")
fn("void glStencilOp(GLenum, GLenum, GLenum)")
fn("void glTexImage2D(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid*)")
fn("void glTexParameteri(GLenum, GLenum, GLint)")
fn("void glTexSubImage2D(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const GLvoid*)")
fn("void glViewport(GLint, GLint, GLsizei, GLsizei)")
fn("void glXDestroyContext(Display*, GLXContext)")
fn("void glXDestroyWindow(Display*, GLXWindow)")
fn("void glXSwapBuffers(Display*, GLXDrawable)")
fn("void glXWaitGL()")
fn("void glXWaitX()")
fn("XVisualInfo* glXGetVisualFromFBConfig(Display*, GLXFBConfig)")

fn("void glActiveTexture(GLenum)")
fn("void glBindTexture(GLenum, GLuint)")
fn("void glBlendColor(GLclampf, GLclampf, GLclampf, GLclampf)")
fn("void glBlendEquation(GLenum)")
fn("void glBlendFunc(GLenum, GLenum)")
fn("void glClearColor(GLfloat, GLfloat, GLfloat, GLfloat)")
fn("void glClearDepth(GLdouble)")
fn("void glClear(GLbitfield)")
fn("void glClearStencil(GLint)")
fn("void glColorMask(GLboolean, GLboolean, GLboolean, GLboolean)")
fn("void glCompressedTexImage2D(GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, const GLvoid*)")
fn("void glCompressedTexSubImage2D(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLsizei, const GLvoid*)")
fn("void glCopyTexImage2D(GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLsizei, GLint)")
fn("void glCopyTexSubImage2D(GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei)")
fn("void glCullFace(GLenum)")
fn("void glDeleteTextures(GLsizei, const GLuint*)")
fn("void glDepthFunc(GLenum)")
fn("void glDepthMask(GLboolean)")
fn("void glDepthRange(GLdouble, GLdouble)")
fn("void glDisable(GLenum)")
fn("void glDrawArrays(GLenum, GLint, GLsizei)")
fn("void glDrawElements(GLenum, GLsizei, GLenum, const GLvoid*)")
fn("void glEnable(GLenum)")
fn("void glFinish()")
fn("void glFlush()")
fn("void glFrontFace(GLenum)")
fn("void glGenTextures(GLsizei, GLuint*)")
fn("void glGetFloatv(GLenum, GLfloat*)")
fn("void glGetIntegerv(GLenum, GLint*)")
fn("void glGetTexImage(GLenum, GLint, GLenum, GLenum, void*)")
fn("void glGetTexLevelParameterfv(GLenum, GLint, GLenum, GLfloat*)")
fn("void glPixelStorei(GLenum, GLint)")
fn("void glRasterPos2i(GLint, GLint)")
fn("void glReadPixels(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, void*)")
fn("void glScissor(GLint, GLint, GLsizei, GLsizei)")
fn("void glStencilFunc(GLenum, GLint, GLuint)")
fn("void glStencilOp(GLenum, GLenum, GLenum)")
fn("void glTexImage2D(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*)")
fn("void glTexParameteri(GLenum, GLenum, GLint)")
fn("void glTexSubImage2D(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const GLvoid*)")
fn("void glViewport(GLint, GLint, GLsizei, GLsizei)")

fn("const GLubyte* glGetString(GLenum)")
fn("const GLubyte* glGetStringi(GLenum, GLuint)")
fn("GLboolean glIsEnabled(GLenum)")
fn("GLboolean glIsProgram(GLuint)")
fn("GLboolean glIsShader(GLuint)")
fn("GLenum glCheckFramebufferStatus(GLenum)")
fn("GLenum glCheckNamedFramebufferStatus(GLuint, GLenum)")
fn("GLenum glGetError()")
fn("GLint glGetUniformLocation(GLuint, const GLchar*)")
fn("GLuint glCreateProgram()")
fn("GLuint glCreateShader(GLenum)")
fn("void glActiveTexture(GLenum)")
fn("void glAttachShader(GLuint, GLuint)")
fn("void glBindAttribLocation(GLuint, GLuint, const GLchar*)")
fn("void glBindBuffer(GLenum, GLuint)")
fn("void glBindFragDataLocation(GLuint, GLuint, const GLchar*)")
fn("void glBindFramebuffer(GLenum, GLuint)")
fn("void glBindProgramPipeline(GLuint)")
fn("void glBindRenderbuffer(GLenum, GLuint)")
fn("void glBindTexture(GLenum, GLuint)")
fn("void glBindTextureUnit(GLuint, GLuint)")
fn("void glBindVertexArray(GLuint)")
fn("void glBlendColor(GLclampf, GLclampf, GLclampf, GLclampf)")
fn("void glBlendEquation(GLenum)")
fn("void glBlendFunc(GLenum, GLenum)")
fn("void glBlendFuncSeparate(GLenum, GLenum, GLenum, GLenum)")
fn("void glBufferData(GLenum, GLsizeiptr, const void*, GLenum)")
fn("void glBufferSubData(GLenum, GLintptr, GLsizeiptr, const void*)")
fn("void glClearColor(GLclampf, GLclampf, GLclampf, GLclampf)")
fn("void glClearDepth(GLclampd)")
fn("void glClear(GLbitfield)")
fn("void glClearStencil(GLint)")
fn("void glColorMask(GLboolean, GLboolean, GLboolean, GLboolean)")
fn("void glCompileShader(GLuint)")
fn("void glCompressedTexImage2D(GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, const GLvoid*)")
fn("void glCompressedTexSubImage2D(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLsizei, const GLvoid*)")
fn("void glCompressedTextureSubImage2D(GLuint, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLsizei, const void*)")
fn("void glCopyTexImage2D(GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLsizei, GLint)")
fn("void glCopyTexSubImage2D(GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei)")
fn("void glCopyTextureSubImage2D(GLuint, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei)")
fn("void glCreateFramebuffers(GLsizei, GLuint*)")
fn("void glCreateProgramPipelines(GLsizei, GLuint*)")
fn("void glCreateRenderbuffers(GLsizei, GLuint*)")
fn("void glCreateTextures(GLenum, GLsizei, GLuint*)")
fn("void glCullFace(GLenum)")
fn("void glDebugMessageCallback(GLDEBUGPROC, const void*)")
fn("void glDebugMessageControl(GLenum, GLenum, GLenum, GLsizei, const GLuint*, GLboolean)")
fn("void glDebugMessageInsert(GLenum, GLenum, GLuint, GLenum, GLsizei, const GLchar*)")
fn("void glDeleteBuffers(GLsizei, const GLuint*)")
fn("void glDeleteFramebuffers(GLsizei, const GLuint*)")
fn("void glDeleteProgram(GLuint)")
fn("void glDeleteProgramPipelines(GLsizei, const GLuint*)")
fn("void glDeleteRenderbuffers(GLsizei, const GLuint*)")
fn("void glDeleteShader(GLuint)")
fn("void glDeleteTextures(GLsizei, const GLuint*)")
fn("void glDeleteVertexArrays(GLsizei, const GLuint*)")
fn("void glDepthFunc(GLenum)")
fn("void glDepthMask(GLboolean)")
fn("void glDepthRange(GLclampd, GLclampd)")
fn("void glDetachShader(GLuint, GLuint)")
fn("void glDisable(GLenum)")
fn("void glDisableVertexAttribArray(GLuint)")
fn("void glDrawArrays(GLenum, GLint, GLsizei)")
fn("void glDrawElements(GLenum, GLsizei, GLenum, const GLvoid*)")
fn("void glEnable(GLenum)")
fn("void glEnableVertexAttribArray(GLuint)")
fn("void glFinish()")
fn("void glFlush()")
fn("void glFramebufferRenderbuffer(GLenum, GLenum, GLenum, GLuint)")
fn("void glFramebufferTexture2D(GLenum, GLenum, GLenum, GLuint, GLint)")
fn("void glFrontFace(GLenum)")
fn("void glGenBuffers(GLsizei, GLuint*)")
fn("void glGenerateMipmap(GLenum)")
fn("void glGenerateTextureMipmap(GLuint)")
fn("void glGenFramebuffers(GLsizei, GLuint*)")
fn("void glGenProgramPipelines(GLsizei, GLuint*)")
fn("void glGenRenderbuffers(GLsizei, GLuint*)")
fn("void glGenTextures(GLsizei, GLuint*)")
fn("void glGenVertexArrays(GLsizei, GLuint*)")
fn("void glGetFloatv(GLenum, GLfloat*)")
fn("void glGetFramebufferAttachmentParameteriv(GLenum, GLenum, GLenum, GLint*)")
fn("void glGetIntegerv(GLenum, GLint*)")
fn("void glGetNamedBufferSubData(GLuint, GLintptr, GLsizeiptr, void*)")
fn("void glGetProgramBinary(GLuint, GLsizei, GLsizei*, GLenum*, void*)")
fn("void glGetProgramInfoLog(GLuint, GLsizei, GLsizei*, GLchar*)")
fn("void glGetProgramiv(GLuint, GLenum, GLint*)")
fn("void glGetProgramPipelineInfoLog(GLuint, GLsizei, GLsizei*, GLchar*)")
fn("void glGetProgramPipelineiv(GLuint, GLenum, GLint*)")
fn("void glGetShaderInfoLog(GLuint, GLsizei, GLsizei*, GLchar*)")
fn("void glGetShaderiv(GLuint, GLenum, GLint*)")
fn("void glGetShaderSource(GLuint, GLsizei, GLsizei*, GLchar*)")
fn("void glGetTexImage(GLenum, GLint, GLenum, GLenum, GLvoid*)")
fn("void glGetTexLevelParameterfv(GLenum, GLint, GLenum, GLfloat*)")
fn("void glGetTextureImage(GLuint, GLint, GLenum, GLenum, GLsizei, void*)")
fn("void glInvalidateFramebuffer(GLenum, GLsizei, const GLenum*)")
fn("void glInvalidateNamedFramebufferData(GLuint, GLsizei, const GLenum*)")
fn("void glLinkProgram(GLuint)")
fn("void glNamedFramebufferRenderbuffer(GLuint, GLenum, GLenum, GLuint)")
fn("void glNamedFramebufferTexture(GLuint, GLenum, GLuint, GLint)")
fn("void glNamedRenderbufferStorage(GLuint, GLenum, GLsizei, GLsizei)")
fn("void glPixelStorei(GLenum, GLint)")
fn("void glProgramBinary(GLuint, GLenum, const void*, GLsizei)")
fn("void glProgramParameteri(GLuint, GLenum, GLint)")
fn("void glProgramUniform1f(GLuint, GLint, GLfloat)")
fn("void glProgramUniform1i(GLuint, GLint, GLint)")
fn("void glProgramUniform2fv(GLuint, GLint, GLsizei, const GLfloat*)")
fn("void glProgramUniform3fv(GLuint, GLint, GLsizei, const GLfloat*)")
fn("void glProgramUniform4fv(GLuint, GLint, GLsizei, const GLfloat*)")
fn("void glProgramUniformMatrix4fv(GLuint, GLint, GLsizei, GLboolean, const GLfloat*)")
fn("void glRasterPos2i(GLint, GLint)")
fn("void glReadPixels(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLvoid*)")
fn("void glRenderbufferStorage(GLenum, GLenum, GLsizei, GLsizei)")
fn("void glScissor(GLint, GLint, GLsizei, GLsizei)")
fn("void glShaderSource(GLuint, GLsizei, const GLchar* const*, const GLint*)")
fn("void glStencilFunc(GLenum, GLint, GLuint)")
fn("void glStencilOp(GLenum, GLenum, GLenum)")
fn("void glTexImage2D(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid*)")
fn("void glTexParameteri(GLenum, GLenum, GLint)")
fn("void glTexStorage2D(GLenum, GLsizei, GLenum, GLsizei, GLsizei)")
fn("void glTexSubImage2D(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const GLvoid*)")
fn("void glTextureParameteri(GLuint, GLenum, GLint)")
fn("void glTextureStorage2D(GLuint, GLsizei, GLenum, GLsizei, GLsizei)")
fn("void glTextureSubImage2D(GLuint, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const void*)")
fn("void glUniform1f(GLint, GLfloat)")
fn("void glUniform1fv(GLint, GLsizei, const GLfloat*)")
fn("void glUniform1i(GLint, GLint)")
fn("void glUniform1iv(GLint, GLsizei, const GLint*)")
fn("void glUniform2f(GLint, GLfloat, GLfloat)")
fn("void glUniform2fv(GLint, GLsizei, const GLfloat*)")
fn("void glUniform2i(GLint, GLint, GLint)")
fn("void glUniform2iv(GLint, GLsizei, const GLint*)")
fn("void glUniform3f(GLint, GLfloat, GLfloat, GLfloat)")
fn("void glUniform3fv(GLint, GLsizei, const GLfloat*)")
fn("void glUniform3i(GLint, GLint, GLint, GLint)")
fn("void glUniform3iv(GLint, GLsizei, const GLint*)")
fn("void glUniform4f(GLint, GLfloat, GLfloat, GLfloat, GLfloat)")
fn("void glUniform4fv(GLint, GLsizei, const GLfloat*)")
fn("void glUniform4i(GLint, GLint, GLint, GLint, GLint)")
fn("void glUniform4iv(GLint, GLsizei, const GLint*)")
fn("void glUniformMatrix2fv(GLint, GLsizei, GLboolean, const GLfloat*)")
fn("void glUniformMatrix3fv(GLint, GLsizei, GLboolean, const GLfloat*)")
fn("void glUniformMatrix4fv(GLint, GLsizei, GLboolean, const GLfloat*)")
fn("void glUseProgram(GLuint)")
fn("void glUseProgramStages(GLuint, GLbitfield, GLuint)")
fn("void glValidateProgram(GLuint)")
fn("void glValidateProgramPipeline(GLuint)")
fn("void glVertexAttribIPointer(GLuint, GLint, GLenum, GLsizei, const void*)")
fn("void glVertexAttribPointer(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*)")
fn("void glViewport(GLint, GLint, GLsizei, GLsizei)")


## fexthunk
### lib("fexthunk")
### 
### function("test"); returns("int"); arg("int")
### function("test_void"); arg("int")
### function("add"); returns("int"); arg("int"); arg("int")

lib("libX11")

#Display *XOpenDisplay(char *display_name);
function("XOpenDisplay");
returns("Display *");
arg("const char *");

#Colormap XCreateColormap(Display *display, Window w, Visual *visual, int alloc);
function("XCreateColormap");
returns("Colormap");
arg("Display *");
arg("Window");
arg("Visual *");
arg("int");

#Window XCreateWindow(Display *display, Window parent, int x, int y, unsigned int width, unsigned int height, unsigned int border_width, int depth, 
#unsigned int class, Visual *visual, unsigned long valuemask, XSetWindowAttributes *attributes);
function("XCreateWindow");
returns("Window");
arg("Display *");
arg("Window");
arg("int")
arg("int")
arg("unsigned int")
arg("unsigned int")
arg("unsigned int")
arg("int")
arg("unsigned int")
arg("Visual *")
arg("unsigned long")
arg("XSetWindowAttributes *")

#int XMapWindow(Display *display, Window w);
function("XMapWindow");
returns("int");
arg("Display *");
arg("Window")


#
function("XChangeProperty")
returns("int")
arg("Display *")
arg("Window")
arg("Atom")
arg("Atom")
arg("int")
arg("int")
arg("const unsigned char *")
arg("int")

#
function("XCloseDisplay")
returns("int")
arg("Display *")

#
function("XDestroyWindow")
returns("int")
arg("Display *")
arg("Window")

#
function("XFree")
returns("int")
arg("void*")

#
function("XInternAtom")
returns("Atom")
arg("Display *")
arg("const char *")
arg("Bool")

#
function("XLookupKeysym")
returns("KeySym")
arg("XKeyEvent *")
arg("int")

#
function("XLookupString")
returns("int")
arg("XKeyEvent *")
arg("char *")
arg("int")
arg("KeySym *")
arg("XComposeStatus*")

#
function("XNextEvent")
returns("int")
arg("Display *")
arg("XEvent *")

#
function("XParseGeometry")
returns("int")
arg("const char *")
arg("int *")
arg("int *")
arg("unsigned int *")
arg("unsigned int *")

#
function("XPending")
returns("int")
arg("Display *")

#
function("XSetNormalHints")
returns("int")
arg("Display *")
arg("Window")
arg("XSizeHints *")

#
function("XSetStandardProperties")
returns("int")
arg("Display *")
arg("Window")
arg("const char *")
arg("const char *")
arg("Pixmap")
arg("char **")
arg("int")
arg("XSizeHints *")

########
########
########

#fn("char* XGetICValues(XIC, ...)")
#fn("_XIC* XCreateIC(XIM, ...)")

fn("Atom XInternAtom(Display*, const char*, int)")
fn("char** XListExtensions(Display*, int*)")
fn("char* XSetLocaleModifiers(const char*)")
fn("Colormap XCreateColormap(Display*, Window, Visual*, int)")
fn("Cursor XCreatePixmapCursor(Display*, Pixmap, Pixmap, XColor*, XColor*, unsigned int, unsigned int)")
fn("Display* XOpenDisplay(const char*)")
fn("int XChangeProperty(Display*, Window, Atom, Atom, int, int, const unsigned char*, int)")
fn("int XCloseDisplay(Display*)")
fn("int XCloseIM(XIM)")
fn("int XDefineCursor(Display*, Window, Cursor)")
fn("int XDestroyWindow(Display*, Window)")
fn("int XDrawString16(Display*, Drawable, GC, int, int, const XChar2b*, int)")
fn("int XEventsQueued(Display*, int)")
fn("int XFillRectangle(Display*, Drawable, GC, int, int, unsigned int, unsigned int)")
fn("int XFilterEvent(XEvent*, Window)")
fn("int XFlush(Display*)")
fn("int XFreeColormap(Display*, Colormap)")
fn("int XFreeCursor(Display*, Cursor)")
fn("int XFreeExtensionList(char**)")
fn("int XFreeFont(Display*, XFontStruct*)")
fn("int XFreeGC(Display*, GC)")
fn("int XFreePixmap(Display*, Pixmap)")
fn("int XFree(void*)")
fn("int XGetErrorDatabaseText(Display*, const char*, const char*, const char*, char*, int)")
fn("int XGetErrorText(Display*, int, char*, int)")
fn("int XGetEventData(Display*, XGenericEventCookie*)")
fn("int XGetWindowProperty(Display*, Window, Atom, long int, long int, int, Atom, Atom*, int*, long unsigned int*, long unsigned int*, unsigned char**)")
fn("int XGrabPointer(Display*, Window, int, unsigned int, int, int, Window, Cursor, Time)")
fn("int XGrabServer(Display*)")
fn("int XIconifyWindow(Display*, Window, int)")
fn("int XIfEvent(Display*, XEvent*, XIfEventFN*, XPointer)")
fn("int XInitThreads()")
fn("int XLookupString(XKeyEvent*, char*, int, KeySym*, XComposeStatus*)")
fn("int XMapRaised(Display*, Window)")
fn("int XMoveResizeWindow(Display*, Window, int, int, unsigned int, unsigned int)")
fn("int XMoveWindow(Display*, Window, int, int)")
fn("int XNextEvent(Display*, XEvent*)")
fn("int XPeekEvent(Display*, XEvent*)")
fn("int XPending(Display*)")
fn("int XQueryExtension(Display*, const char*, int*, int*, int*)")
fn("int XQueryPointer(Display*, Window, Window*, Window*, int*, int*, int*, int*, unsigned int*)")
fn("int XQueryTree(Display*, Window, Window*, Window*, Window**, unsigned int*)")
fn("int XResetScreenSaver(Display*)")
fn("int XResizeWindow(Display*, Window, unsigned int, unsigned int)")
fn("int XSelectInput(Display*, Window, long int)")
fn("int XSendEvent(Display*, Window, int, long int, XEvent*)")
fn("XSetErrorHandlerFN* XSetErrorHandler(XErrorHandler)")
fn("int XSetTransientForHint(Display*, Window, Window)")
fn("int XSetWMProtocols(Display*, Window, Atom*, int)")
fn("int XSync(Display*, int)")
fn("int XTextExtents16(XFontStruct*, const XChar2b*, int, int*, int*, int*, XCharStruct*)")
fn("int XTranslateCoordinates(Display*, Window, Window, int, int, int*, int*, Window*)")
fn("int XUngrabPointer(Display*, Time)")
fn("int XUngrabServer(Display*)")
fn("int XUnmapWindow(Display*, Window)")
fn("int Xutf8LookupString(XIC, XKeyPressedEvent*, char*, int, KeySym*, int*)")
fn("int XWarpPointer(Display*, Window, Window, int, int, unsigned int, unsigned int, int, int)")
fn("int XWindowEvent(Display*, Window, long int, XEvent*)")
fn("Pixmap XCreateBitmapFromData(Display*, Drawable, const char*, unsigned int, unsigned int)")
fn("Pixmap XCreatePixmap(Display*, Drawable, unsigned int, unsigned int, unsigned int)")
fn("void XDestroyIC(XIC)")
fn("void XFreeEventData(Display*, XGenericEventCookie*)")
fn("void XLockDisplay(Display*)")
fn("void XSetICFocus(XIC)")
fn("void XSetWMNormalHints(Display*, Window, XSizeHints*)")
fn("void XUnlockDisplay(Display*)")
fn("void Xutf8SetWMProperties(Display*, Window, const char*, const char*, char**, int, XSizeHints*, XWMHints*, XClassHint*)")
fn("Window XCreateWindow(Display*, Window, int, int, unsigned int, unsigned int, unsigned int, int, unsigned int, Visual*, long unsigned int, XSetWindowAttributes*)")
fn("XFontStruct* XLoadQueryFont(Display*, const char*)")
fn("_XGC* XCreateGC(Display*, Drawable, long unsigned int, XGCValues*)")
fn("XImage* XGetImage(Display*, Drawable, int, int, unsigned int, unsigned int, long unsigned int, int)")
fn("_XIM* XOpenIM(Display*, _XrmHashBucketRec*, char*, char*)")


# generate
WhatToGenerate = "all"

if len(sys.argv) == 2:
    WhatToGenerate = sys.argv[1]

if WhatToGenerate == "all" or  WhatToGenerate == "thunks":
    print("// thunks")
    #print('#define MAKE_THUNK(lib, name) __attribute__((naked)) int fexthunks_##lib##_##name(void* args) { asm("int $0x7F\\n"); asm(".asciz " #lib ":" #name "\\n"); }')
    print('extern "C" {')
    GenerateThunks(Libs)
    print('}')

if WhatToGenerate == "all" or  WhatToGenerate == "forwards":
    print("// forwards")
    GenerateForwards(Libs)

if WhatToGenerate == "all" or  WhatToGenerate == "initializers":
    print("// initializers")
    GenerateInitializers(Libs)

if WhatToGenerate == "all" or  WhatToGenerate == "thunkmap":
    print("// thunkmap")
    GenerateThunkmap(Libs)

if WhatToGenerate == "all" or  WhatToGenerate == "symtab":
    print("// symtab")
    GenerateSymtab(Libs)