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
```FEXLoader -c irjit -n 500 -R $ROOTFS -t $BUILDDIR/Host -- /PATH/TO/ELF```

We currently don't have any unit tests for the guest libraries, only for OP_THUNK.

## Implementation outline
There are several parts that make this possible. This is a rough outline.

In FEX
- Opcode 0xF 0x3D (IR:OP_THUNK) is used for the Guest -> Host transition. Register RSI (arg0 in guest) is passed as arg0 in host. Thunks are identified by a string in the form `library:function` that directly follows the Guest opcode.
- `Context::HandleCallback` does the Host -> Guest transition, and returns when the Guest function returns.
- A special thunk, `fex:loadlib` is used to load and initialize a matching host lib. For more details, look in `ThunkHandler_impl::LoadLib`
- `ThunkHandler_impl::CallCallback` is provided to the host libs, so they can call callbacks. It prepares guest arguments and uses `Context::HandleCallback` 

ThunkLibs, Library loading
- In Guest code, when a thunking library is loaded it has a constructor that calls the `fex:loadlib` thunk, with the library name and callback unpackers, if any.
- In FEX, a matching host library is loaded using dlopen, `fexthunks_exports_$libname(CallCallbackPtr, GuestUnpackers)` is called to initialize the host library.
- In Host code, the real host library is loaded using dlopen and dlsym (see ldr generation)

ThunkLibs, Guest -> Host
- In Guest code (guest packer), a packer takes care of packing tha arguments & return value into a struct in Guest stack. The packer is usually exported as a symbol from the Guest library.
- in Guest code (guest thunk), a thunk does the Guest -> Host transition, and passes the struct ptr as an argument
- In Host code (host unpacker), an unpacker takes the arguments from the struct, and calls a function pointer with the implementation of that function. It also stores the return value, if any, to the struct.
- In Host code (host unpacker), the the unpacker returns, and we do an implicit Host -> Guest transition
- In Guest code (guest packer), the return value is loaded from the struct and returned, if needed

ThunkLibs, Host -> Guest. This is only possible while handling a Guest -> Host call (ie, callbacks). 
- In Host code (host packer), a packer packs the arguments & return value to a struct in Host stack.
- In Host code (host packer), `ThunkHandler_impl::CallCallback` is called with the Guest unpacker, and Guest function as arguments
- In Guest code (guest unpacker), the arguments are unpacked, the Guest function is called, and the return value is stored to the struct
- In Guest code (guest unpacker), the unpacker returns and we do an implicit Guest -> Host transition
- In host code (host packer), the return value is loaded from the struct and returned, if needed

A python generator script, `Generators/ThunkHelpers.py` can be used to auto-generate various parts of the code. It takes in a
C-like function descriptor, and can generate guest->host thunks, argument packers, argument unpackers, a host library loader
and some other helpers.

Components that can be generated with the python script
- `thunks`: Guest -> Host transition functions that use 0xF 0x3D
- `function_packs`: Guest argument packers / rv handling, private to the SO. These are used to solve symbol resolution issues with glxGetProc*, etc.
- `function_packs_public`: Guest argument packers / rv handling, exported from the SO. These are identical to the function_packs, but exported from the SO
- `function_unpacks`: Host argument unpackers / rv handling
- `ldr`: Host loader that dlopens/dlsyms the "real" host library for the implementation functions.
- `ldr_ptrs`: Host loader pointer declarations, used by ldr and function_unpacks
- `tab_function_unpacks`: Host function unpackers list, passed to FEX after Host library init so it can resolve the Guest Thunks to Host functions
- `tab_function_packs`: Guest private function packers list, used for glxGetProc*
- `callback_structs`: Guest/Host callback un/packer struct declarations
- `callback_unpacks_header`: Guest callback unpacker handler declarations (Used both in Guest and Host). This is how the Guest passes the callback unpackers to host
- `callback_unpacks_header_init`: Guest callback unpacker handler initializer (Used in Guest).
- `callback_unpacks`: Guest callback unpackers
- `callback_typedefs`: Guest callback unpacker typedefs


## Adding a new library

There are two kinds of libs, simpler ones with no callbacks, and complex ones with callbacks. You can see how `libX11` is implemented for a callbacks example, and `libasound` for a non-callbacks example.

Getting started
- In `Generators/` make a new script named `libName.py`. See some existing lib on how to populate that one.
- Create `libName/libName_Guest.cpp` and `libName/libName_Host.cpp`. Copy & rename from some existing lib is the way to go.
- Edit `GuestLibs/CMakeLists.txt` and `HostLibs/CMakeLists.txt` to add the new targets, similar to how other libs are done.

Now the host and the guest libs should be built as part of `guest-libs` and `host-libs`