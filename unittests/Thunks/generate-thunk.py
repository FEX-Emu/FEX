#!/usr/bin/python3
import sys

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