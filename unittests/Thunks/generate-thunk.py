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
    print("void* fexthunks_init_" + lib["name"] + "_" + function["name"] + ";")

def GenerateInitializer_lib(lib):
    handle =  "fexthunks_init_" + lib["name"] + "_so"
    print("void* " + handle + ";")
    
    for function in lib["functions"].values():
        GenerateInitializer_function(lib, function)

    print("bool fexthunks_init_" + lib["name"] + "() {")
    print(handle + " = dlopen(\""+ lib["name"] +"\", RTLD_LOCAL | RTLD_LAZY);");
    print("if (!" + handle + ") { return false; }");
    print("}")

def GenerateInitializers(libs):
    for lib in libs.values():
        GenerateInitializer_lib(lib)

# define libs here
### lib("libGL")
### 
### #XVisualInfo* glXChooseVisual( Display *dpy, int screen, int *attribList );
### function("glXChooseVisual")
### returns("extern XVisualInfo *")
### arg("Display *")
### arg("int")
### arg("int *")
### 
### #GLXContext glXCreateContext( Display *dpy, XVisualInfo *vis, GLXContext shareList, Bool direct );
### function("glXCreateContext")
### returns("GLXContext")
### arg("Display *")
### arg("XVisualInfo *")
### arg("GLXContext *")
### arg("Bool *")
### 
### #void glXDestroyContext( Display *dpy, GLXContext ctx );
### function("glXDestroyContext")
### returns("void")
### arg("Display *")
### arg("GLXContext")
### 
### #Bool glXMakeCurrent( Display *dpy, GLXDrawable drawable, GLXContext ctx);
### function("glXMakeCurrent")
### returns("Bool")
### arg("Display *")
### arg("GLXDrawable")
### arg("GLXContext")
### 
### #void glXCopyContext( Display *dpy, GLXContext src, GLXContext dst, GLuint mask );
### function("glXCopyContext")
### returns("void")
### arg("Display *")
### arg("GLXContext")
### arg("GLXContext")
### arg("GLuint")
### 
### #void glXSwapBuffers( Display *dpy, GLXDrawable drawable );
### function("glXSwapBuffers")
### returns("void")
### arg("Display *")
### arg("GLXDrawable")
### 
### #GLXPixmap glXCreateGLXPixmap( Display *dpy, XVisualInfo *visual, Pixmap pixmap );
### function("glXCreateGLXPixmap")
### returns("GLXPixmap")
### arg("Display *")
### arg("XVisualInfo *")
### arg("Pixmap")
### 
### #void glXDestroyGLXPixmap( Display *dpy, GLXPixmap pixmap );
### function("glXDestroyGLXPixmap")
### returns("void")
### arg("Display *")
### arg("GLXPixmap")
### 
### #Bool glXQueryExtension( Display *dpy, int *errorb, int *event );
### function("glXQueryExtension")
### returns("Bool")
### arg("Display *")
### arg("int *")
### arg("int *")
### 
### #Bool glXQueryVersion( Display *dpy, int *maj, int *min );
### function("glXQueryVersion")
### returns("Bool")
### arg("Display *")
### arg("int *")
### arg("int *")
### 
### #Bool glXIsDirect( Display *dpy, GLXContext ctx );
### function("glXIsDirect")
### returns("Bool")
### arg("Display *")
### arg("GLXContext")
### 
### #int glXGetConfig( Display *dpy, XVisualInfo *visual, int attrib, int *value );
### function("glXGetConfig")
### returns("int")
### arg("Display *")
### arg("XVisualInfo *")
### arg("int")
### arg("int *")
### 
### #GLXContext glXGetCurrentContext( void );
### function("glXGetCurrentContext")
### returns("GLXContext")
### 
### #GLXDrawable glXGetCurrentDrawable( void );
### function("glXGetCurrentDrawable")
### returns("GLXDrawable")
### 
### #void glXWaitGL( void );
### function("glXWaitGL")
### returns("void")
### 
### #void glXWaitX( void );
### function("glXWaitX")
### returns("void")
### 
### #void glXUseXFont( Font font, int first, int count, int list );
### function("glXUseXFont")
### returns("void")
### arg("Font")
### arg("int")
### arg("int")
### arg("int")
### 
### #const char *glXQueryExtensionsString( Display *dpy, int screen );
### function("glXQueryExtensionsString")
### returns("const char *")
### arg("Display *")
### arg("int")
### 
### #const char *glXQueryServerString( Display *dpy, int screen, int name );
### function("glXQueryServerString")
### returns("const char *")
### arg("Display *")
### arg("int")
### arg("int")
### 
### #const char *glXGetClientString( Display *dpy, int name );
### function("glXGetClientString")
### returns("const char *")
### arg("Display *")
### arg("int")

## fexthunk
lib("fexthunk")

function("test"); returns("int"); arg("int")
function("test_void"); arg("int")
function("add"); returns("int"); arg("int"); arg("int")

# generate
WhatToGenerate = "all"

if len(sys.argv) == 2:
    WhatToGenerate = sys.argv[1]

if WhatToGenerate == "all" or  WhatToGenerate == "thunks":
    print("// thunks")
    GenerateThunks(Libs)

if WhatToGenerate == "all" or  WhatToGenerate == "forwards":
    print("// forwards")
    GenerateForwards(Libs)

#if WhatToGenerate == "all" or  WhatToGenerate == "initializers":
#    GenerateInitializers(Libs)