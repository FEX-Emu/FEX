#!/usr/bin/python3
import sys
import re
from hashlib import sha256

Libs = { }
CurrentLib = None
CurrentFunction = None

def hash_lib_fn_asm(lib, fn):
    return "0x" + ", 0x".join(re.findall('..', sha256((lib + ":" + fn).encode('utf-8')).hexdigest()))

def hash_lib_fn_c(lib, fn):
    return "\\x" + "\\x".join(re.findall('..', sha256((lib + ":" + fn).encode('utf-8')).hexdigest()))

def lib(name):
    global Libs
    global CurrentLib
    global CurrentFunction

    Libs[name] = {
        "name": name,
        "functions": { },
        "callbacks": { }
    }
    CurrentLib = Libs[name]
    CurrentFunction = None

# format: "ret_type function_name(arg_type, arg_type)"
def fn(cdecl):
    global CurrentLib
    global CurrentFunction

    parts = re.findall("([^(]+)\(([^)]*)\)", cdecl)[0]
    name = parts[0].replace("*", " ").split(" ")[-1]
    rv = parts[0][0:-len(name)].strip()
    args = parts[1].strip()

    CurrentFunction = CurrentLib["functions"][name] = {
        "name" : name,
        "return": rv,
        "args": list(filter(None, map(str.strip, args.split(',')))),
        "thunk": True,
        "pack": True,
        "unpack": True,
        "ldr_ptr": True,
        "ldr": True,
        "tab_unpack": True,
        "tab_pack": True
    }

def no_thunk():
    global CurrentFunction
    CurrentFunction["thunk"] = False

def no_unpack():
    global CurrentFunction
    CurrentFunction["unpack"] = False

def no_pack():
    global CurrentFunction
    CurrentFunction["pack"] = False

def no_ldr_ptr():
    global CurrentFunction
    CurrentFunction["ldr_ptr"] = False

def no_ldr():
    global CurrentFunction
    CurrentFunction["ldr"] = False

def no_tab_unpack():
    global CurrentFunction
    CurrentFunction["tab_unpack"] = False

def no_tab_pack():
    global CurrentFunction
    CurrentFunction["tab_pack"] = False

# format: "ret_type function_name(arg_type, arg_type)"
def cb(cdecl):
    global CurrentLib

    parts = re.findall("([^(]+)\(([^)]*)\)", cdecl)[0]
    name = parts[0].replace("*", " ").split(" ")[-1]
    rv = parts[0][0:-len(name)].strip()
    args = parts[1].strip()

    CurrentLib["callbacks"][name] = {
        "name" : name,
        "return": rv,
        "args": list(filter(None, map(str.strip, args.split(','))))
    }

def IterateLibs(libs, field, filter, Fn):
    for lib in libs.values():
        for item in lib[field].values():
            if filter == False or item[filter] == True:
                Fn(lib, item)

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

def GenerateThunk_has_struct(returns, args):
    return returns != "void" or len(args) != 0

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

def GenerateFunctionThunk(lib, function):
    print("MAKE_THUNK(" + lib["name"] + ", " + function["name"] + ", \"" + hash_lib_fn_asm(lib["name"], function["name"]) + "\")")


###
### function_packs
### Used in Guest to export symbols to the loader
### TODO: Switch to symbol aliases
###

def GenerateFunctionPackPublic(lib, function):
    print(function["return"] + " " + function["name"] + GenerateThunk_args(function["args"]) + " __attribute__((alias(\"fexfn_pack_" + function["name"] + "\")));")


###
### function_packs
### Used in Guest to internally generate unpacks
### Introduced for libGL as we don't want to use
### the export symbol, as it may be overriden
###

def GenerateFunctionPack(lib, function):
    print("static " + function["return"] + " fexfn_pack_" + function["name"] + GenerateThunk_args(function["args"]) + "{")
    if GenerateThunk_has_struct(function["return"], function["args"]):
        print("struct " + GenerateThunk_struct(function["return"], function["args"]) + " args;")
        print(GenerateThunk_args_assignment(function["args"]))
        print("fexthunks_" + lib["name"] + "_" + function["name"] + "(&args);")

        if function["return"] != "void":
            print("return args.rv;")
    else:
        print("fexthunks_" + lib["name"] + "_" + function["name"] + "(nullptr);")
    
    print("}")
    
    print("")

###
### function_unpacks
### Used in the Host to unpack and call the ldr_ptr
### for that function
###
def GenerateFunctionUnpack_args(args):
    rv = [ ]
    rv.append("(")
    for i in range(len(args)):
        if i != 0:
            rv.append(",")
        rv.append("args->a_" + str(i))
    rv.append(")")
    
    return "".join(rv)

def GenerateFunctionUnpack(lib, function):
    print("static void fexfn_unpack_" + lib["name"] + "_" + function["name"] + "(void *argsv){")

    if GenerateThunk_has_struct(function["return"], function["args"]):
        print("struct arg_t "+ GenerateThunk_struct(function["return"], function["args"]) + ";")
        print("auto args = (arg_t*)argsv;")

        if function["return"] != "void":
            print("args->rv = ");

    print("fexldr_ptr_" + lib["name"] + "_" + function["name"])
    print(GenerateFunctionUnpack_args(function["args"]) + ";")
    print("}")

###
### ldr_ptrs
### Used in Host
### Loader ptrs for function resolution from dlsym/internal implementation
###
def GenerateFunctionLdrPtr(lib, function):
    print("typedef " + function["return"]  + " fexldr_type_" + lib["name"] + "_" + function["name"] + GenerateThunk_args(function["args"]) + ";")
    print("static fexldr_type_" + lib["name"] + "_" + function["name"] + " *fexldr_ptr_" + lib["name"] + "_" + function["name"] + ";")

###
### ldr
### Used in Host
### so handle, loader function that dlsyms the host symbols to the loader ptrs
###
def GenerateLdr_function_loader(lib, function, handle, dlsym_impl):
    if dlsym_impl:
        print("(void*&)fexldr_ptr_" + lib["name"] + "_" + function["name"] + " = dlsym(" + handle + ', "' + function["name"] + '");')
    else:
        print("fexldr_ptr_" + lib["name"] + "_" + function["name"] + " = &fexfn_impl_" + lib["name"] + "_" + function["name"] + ";")

def GenerateLdr_lib(lib):
    handle =  "fexldr_ptr_" + lib["name"] + "_so"
    print("static void* " + handle + ";")

    print("extern \"C\" bool fexldr_init_" + lib["name"] + "() {")
    print(handle + " = dlopen(\""+ lib["name"] +".so\", RTLD_LOCAL | RTLD_LAZY);");
    print("if (!" + handle + ") { return false; }");
    for function in lib["functions"].values():
        GenerateLdr_function_loader(lib, function, handle, function["ldr"])
    print("return true;")
    print("}")

def GenerateLdr(libs):
    for lib in libs.values():
        GenerateLdr_lib(lib)

# Used to initialize host thunk list
def GenerateTabFunctionUnpack(lib, function):
    print("{(uint8_t*)\"" + hash_lib_fn_c(lib["name"], function["name"]) + "\", &fexfn_unpack_" + lib["name"]  + "_" + function["name"] + "}, //", lib["name"]  + ":" + function["name"])

# Symtab, used for glxGetProc
def GenerateTabFunctionPack(lib, function):
    print("{\"" + function["name"] + "\", (voidFunc*)&fexfn_pack_" + function["name"] + "},")

def GenerateCallbackStruct(lib, callback):
    print("struct " + callback["name"] + "_Args " + GenerateThunk_struct(callback["return"], callback["args"]) + ";")

def GenerateCallbackUnpackHeader(lib, callback):
    print("uintptr_t " + lib["name"] + "_" + callback["name"] + ";")

def GenerateCallbackUnpackHeaderInit(lib, callback):
    print("(uintptr_t)&fexfn_unpack_" + lib["name"] + "_" + callback["name"] + ",")

def GenerateCallbackUnpack(lib, function):
    print("static void fexfn_unpack_" + lib["name"] + "_" + function["name"] + "(uintptr_t cb, void *argsv){")

    print("typedef " + function["return"] + " fn_t " + GenerateThunk_args(function["args"]) + ";")
    print("auto callback = reinterpret_cast<fn_t*>(cb);")
    
    if GenerateThunk_has_struct(function["return"], function["args"]):
        print("struct arg_t "+ GenerateThunk_struct(function["return"], function["args"]) + ";")
        print("auto args = (arg_t*)argsv;")

        if function["return"] != "void":
            print("args->rv = ");

    print("callback")
    print(GenerateFunctionUnpack_args(function["args"]) + ";")
    print("}")

def GenerateCallbackTypedefs(lib, function):
    print("typedef " + function["return"] + " " +  function["name"] + "FN " + GenerateThunk_args(function["args"]) + ";")

def Generate():
    # generate
    WhatToGenerate = "all"

    if len(sys.argv) == 2:
        WhatToGenerate = sys.argv[1]

    if WhatToGenerate == "all" or  WhatToGenerate == "thunks":
        print("// Generated: thunks")
        print('extern "C" {')
        IterateLibs(Libs, "functions", "thunk", GenerateFunctionThunk)
        print('}')

    if WhatToGenerate == "all" or  WhatToGenerate == "function_packs":
        print("// Generated: function_packs")
        print('extern "C" {')
        IterateLibs(Libs, "functions", "pack", GenerateFunctionPack)
        print('}')

    if WhatToGenerate == "all" or  WhatToGenerate == "function_packs_public":
        print("// Generated: function_packs_public")
        print('extern "C" {')
        IterateLibs(Libs, "functions", "pack", GenerateFunctionPackPublic)
        print('}')
    
    if WhatToGenerate == "all" or  WhatToGenerate == "function_unpacks":
        print("// Generated: function_unpacks")
        print('extern "C" {')
        IterateLibs(Libs, "functions", "unpack", GenerateFunctionUnpack)
        print('}')

    if WhatToGenerate == "all" or  WhatToGenerate == "ldr":
        print("// Generated: ldr")
        GenerateLdr(Libs)

    if WhatToGenerate == "all" or  WhatToGenerate == "ldr_ptrs":
        print("// Generated: ldr_ptrs")
        IterateLibs(Libs, "functions", "ldr_ptr", GenerateFunctionLdrPtr)

    if WhatToGenerate == "all" or  WhatToGenerate == "tab_function_unpacks":
        print("// Generated: tab_function_unpacks")
        IterateLibs(Libs, "functions", "tab_unpack", GenerateTabFunctionUnpack)

    if WhatToGenerate == "all" or  WhatToGenerate == "tab_function_packs":
        print("// Generated: tab_function_packs")
        IterateLibs(Libs, "functions", "tab_pack", GenerateTabFunctionPack)

    if WhatToGenerate == "all" or  WhatToGenerate == "callback_structs":
        print("// Generated: callback_structs")
        IterateLibs(Libs, "callbacks", False, GenerateCallbackStruct)

    if WhatToGenerate == "all" or  WhatToGenerate == "callback_unpacks_header":
        print("// Generated: callback_unpacks_header")
        IterateLibs(Libs, "callbacks", False, GenerateCallbackUnpackHeader)
    
    if WhatToGenerate == "all" or  WhatToGenerate == "callback_unpacks_header_init":
        print("// Generated: callback_unpacks_header_init")
        IterateLibs(Libs, "callbacks", False, GenerateCallbackUnpackHeaderInit)

    if WhatToGenerate == "all" or  WhatToGenerate == "callback_unpacks":
        print("// Generated: callback_unpacks")
        IterateLibs(Libs, "callbacks", False, GenerateCallbackUnpack)
    
    if WhatToGenerate == "all" or  WhatToGenerate == "callback_typedefs":
        print("// Generated: callback_unpacks")
        IterateLibs(Libs, "callbacks", False, GenerateCallbackTypedefs)

        