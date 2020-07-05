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
        "args": [],
        "noforward": False,
        "noinit": False,
        "noload": False,
        "nothunkmap": False,
        "nosymtab": False,
        "nothunk": False
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

# format: "ret_type function_name(arg_type, arg_type)"
def fn(cdecl):
    parts = re.findall("([^(]+)\(([^)]*)\)", cdecl)[0]
    fn_name = parts[0].replace("*", " ").split(" ")[-1]
    fn_rv = parts[0][0:-len(fn_name)]
    fn_args = parts[1].strip()

    function(fn_name.strip())
    returns(fn_rv.strip())
    if (len(fn_args)):
        args(fn_args)

def noforward():
    global CurrentFunction
    CurrentFunction["noforward"] = True

def noinit():
    global CurrentFunction
    CurrentFunction["noinit"] = True

def noload():
    global CurrentFunction
    CurrentFunction["noload"] = True

def nothunkmap():
    global CurrentFunction
    CurrentFunction["nothunkmap"] = True

def nosymtab():
    global CurrentFunction
    CurrentFunction["nosymtab"] = True

def nothunk():
    global CurrentFunction
    CurrentFunction["nothunk"] = True

def GenerateThunk_args(args):
    rv = [ ]
    rv.append("(")
    for i in range(len(args)):
        if i != 0:
            rv.append(",")

        if (args[i] == "..."):
            rv.append("...")
        else:
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

    print("static " + function["return"] + " fex_internal_sym_" + function["name"] + GenerateThunk_args(function["args"]) + "{")
    print("struct " + GenerateThunk_struct(function["return"], function["args"]) + " args;")
    print(GenerateThunk_args_assignment(function["args"]))
    print("fexthunks_" + lib["name"] + "_" + function["name"] + "(&args);")

    if function["return"] != "void":
        print("return args.rv;")
    
    print("}")
    
    print("")
    
def GenerateThunk_lib(lib):
    for function in lib["functions"].values():
        if function["nothunk"] == False:
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
    print("static void fexthunks_forward_" + lib["name"] + "_" + function["name"] + "(void *argsv){")
    print("struct arg_t "+ GenerateThunk_struct(function["return"], function["args"]) + ";")
    print("auto args = (arg_t*)argsv;")

    if function["return"] != "void":
        print("args->rv = ");
    print("fexthunks_impl_" + lib["name"] + "_" + function["name"])
    print(GenerateForward_args(function["args"]) + ";")
    print("}")

def GenerateForward_lib(lib):
    for function in lib["functions"].values():
        if function["noforward"] == False:
            GenerateForward_function(lib, function)

def GenerateForwards(libs):
    for lib in libs.values():
        GenerateForward_lib(lib)

def GenerateInitializer_function(lib, function):
    print("typedef " + function["return"]  + " fexthunks_type_" + lib["name"] + "_" + function["name"] + GenerateThunk_args(function["args"]) + ";")
    print("static fexthunks_type_" + lib["name"] + "_" + function["name"] + " *fexthunks_impl_" + lib["name"] + "_" + function["name"] + ";")

def GenerateInitializer_function_loader(lib, function, handle):
    print("(void*&)fexthunks_impl_" + lib["name"] + "_" + function["name"] + " = dlsym(" + handle + ', "' + function["name"] + '");')

def GenerateInitializer_lib(lib):
    handle =  "fexthunks_impl_" + lib["name"] + "_so"
    print("static void* " + handle + ";")
    
    for function in lib["functions"].values():
        if function["noinit"] == False:
            GenerateInitializer_function(lib, function)

    print("extern \"C\" bool fexthunks_init_" + lib["name"] + "() {")
    print(handle + " = dlopen(\""+ lib["name"] +".so\", RTLD_LOCAL | RTLD_LAZY);");
    print("if (!" + handle + ") { return false; }");
    for function in lib["functions"].values():
        if function["noload"] == False:
            GenerateInitializer_function_loader(lib, function, handle)
    print("return true;")
    print("}")

def GenerateInitializers(libs):
    for lib in libs.values():
        GenerateInitializer_lib(lib)

# Used to initialize host thunk list
def GenerateThunkmap_function(lib, function):
    print("{\"" + lib["name"]  + ":" + function["name"] + "\", &fexthunks_forward_" + lib["name"]  + "_" + function["name"] + "},")

def GenerateThunkmap_lib(lib):
    for function in lib["functions"].values():
        if function["nothunkmap"] == False:
            GenerateThunkmap_function(lib, function)

def GenerateThunkmap(libs):
    for lib in libs.values():
        GenerateThunkmap_lib(lib)

# Symtab, used for glxGetProc
def GenerateSymtab_function(lib, function):
    print("{\"" + function["name"] + "\", (voidFunc*)&fex_internal_sym_" + function["name"] + "},")

def GenerateSymtab_lib(lib):
    for function in lib["functions"].values():
        if function["nosymtab"] == False:
            GenerateSymtab_function(lib, function)

def GenerateSymtab(libs):
    for lib in libs.values():
        GenerateSymtab_lib(lib)

def Generate():
    # generate
    WhatToGenerate = "all"

    if len(sys.argv) == 2:
        WhatToGenerate = sys.argv[1]

    if WhatToGenerate == "all" or  WhatToGenerate == "thunks":
        print("// thunks")
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