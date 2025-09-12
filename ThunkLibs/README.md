# FEX Library Thunking (Thunklibs)
FEX supports special guest libraries that call out to host code for speed and compatibility.

We support both guest->host thunks, as well as host->guest callbacks

## Building and using
The thunked libraries can be built via the `guest-libs` and `host-libs` targets of the main FEX project. The outputs are in `$BUILDDIR/Guest` and `$BUILDDIR/Host`

After that, a guest rootfs is needed with the guest-libs installed. Typically this is done with symlinks that replace the native guest libraries. eg 
```
# Unlink original guest lib
unlink $ROOTFS/lib/x86_64-linux-gnu/libX11.so.6
# Make it point to thunked version
ln -s $BUILDDIR/Guest/libX11-guest.so $ROOTFS/lib/x86_64-linux-gnu/libX11.so.6
```

Finally, FEX needs to be told where to look for the matching host libraries with `-t /Host/Libs/Path`. eg
```FEX_THUNKHOSTLIBS= $BUILDDIR/Host FEX /PATH/TO/ELF```

We currently don't have any unit tests for the guest libraries, only for OP_THUNK.

## Implementation outline
There are several parts that make this possible. This is a rough outline.

In FEX
- Opcode 0xF 0x3F (IR::OP_THUNK) is used for the Guest -> Host transition. Register RSI (arg0 in guest) is passed as arg0 in host. Thunks are identified by a string in the form `library:function` that directly follows the Guest opcode.
- `Context::HandleCallback` does the Host -> Guest transition, and returns when the Guest function returns.
- A special thunk, `fex:loadlib` is used to load and initialize a matching host lib. For more details, look in `ThunkHandler_impl::LoadLib`
- `ThunkHandler_impl::CallCallback` is provided to the host libs, so they can call callbacks. It prepares guest arguments and uses `Context::HandleCallback` 

ThunkLibs, Library loading
- In Guest code, when a thunking library is loaded it has a constructor that calls the `fex:loadlib` thunk, with the library name and callback unpackers, if any.
- In FEX, a matching host library is loaded using dlopen, `fexthunks_exports_$libname(CallCallbackPtr, GuestUnpackers)` is called to initialize the host library.
- In Host code, the real host library is loaded using dlopen and dlsym (see ldr generation)

ThunkLibs, Guest -> Host
- In Guest code (guest packer), a packer takes care of packing the arguments & return value into a struct in Guest stack. The packer is usually exported as a symbol from the Guest library.
- In Guest code (guest thunk), a thunk does the Guest -> Host transition via OP_THUNK, and passes the struct pointer as an argument
- FEX handles OP_THUNK and looks up the Host function from the opcode argument
- In Host code (host unpacker), an unpacker takes the arguments from the struct, and calls a function pointer with the implementation of that function. It also stores the return value, if any, to the struct.
- In Host code (host unpacker), the unpacker returns, and we do an implicit Host -> Guest transition
- In Guest code (guest packer), the return value is loaded from the struct and returned, if needed

ThunkLibs, Host -> Guest. This is only possible while handling a Guest -> Host call (ie, callbacks). 
- In Host code (host packer), a packer packs the arguments & return value to a struct in Host stack.
- In Host code (host packer), `ThunkHandler_impl::CallCallback` is called with the Guest unpacker, and Guest function as arguments
- In Guest code (guest unpacker), the arguments are unpacked, the Guest function is called, and the return value is stored to the struct
- In Guest code (guest unpacker), the unpacker returns and we do an implicit Guest -> Host transition
- In host code (host packer), the return value is loaded from the struct and returned, if needed

Boilerplate code is automated using a dedicated code generator tool, which parses a C++ source file (`libX_interface.cpp`) that specializes
a templated `fex_gen_config` struct for each thunked function. The generator will pull all required function signatures from the original
library's header files and emit the appropriate boilerplate (guest->host thunks, argument packers/unpackers, host library loader, ...).

In most cases, an empty `fex_gen_config` specialization is sufficient, but if needed the generator behavior can be customized on a
function-by-function basis using an annotation-syntax: Binary properties are toggled by inheriting from a fixed set of tag types
(e.g. `fexgen::custom_host_impl`), whereas complicated properties are customized by defining struct members/aliases with a magic name
detected by the generator (e.g. `using uniform_va_type = char`).

For each thunked library, the generator outputs the following files:
- `thunks.inl`: Guest -> Host transition functions that use 0xF 0x3F
- `function_packs.inl`: Guest argument packers / rv handling, private to the SO. These are used to solve symbol resolution issues with glxGetProc*, etc.
- `function_packs_public.inl`: Guest argument packers / rv handling, exported from the SO. These are identical to the function_packs, but exported from the SO
- `function_unpacks.inl`: Host argument unpackers / rv handling
- `ldr.inl`: Host loader that dlopens/dlsyms the "real" host library for the implementation functions.
- `ldr_ptrs.inl`: Host loader pointer declarations, used by ldr and function_unpacks
- `tab_function_unpacks.inl`: Host function unpackers list, passed to FEX after Host library init so it can resolve the Guest Thunks to Host functions


## Adding a new library

There are two kinds of libs, simpler ones with no callbacks, and complex ones with callbacks. You can see how `libX11` is implemented for a callbacks example, and `libasound` for a non-callbacks example.

Getting started
- Create `libName/libName_interface.cpp` and customize the `fex_gen_config` template for each thunked function. See some existing lib for details.
- Create `libName/libName_Guest.cpp` and `libName/libName_Host.cpp`. Copy & rename from some existing lib is the way to go.
- Edit `GuestLibs/CMakeLists.txt` and `HostLibs/CMakeLists.txt` to add the new targets, similar to how other libs are done.

Now the host and the guest libs should be built as part of `guest-libs` and `host-libs`
