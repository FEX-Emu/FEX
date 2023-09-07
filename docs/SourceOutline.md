# FEX-2309.1

## External/FEXCore

## ThunkLibs
See [ThunkLibs/README.md](../ThunkLibs/README.md) for more details

### thunklibs
These are generated + glue logic 1:1 thunks unless noted otherwise

#### EGL
- [libEGL_Guest.cpp](../ThunkLibs/libEGL/libEGL_Guest.cpp): Depends on glXGetProcAddress thunk
- [libEGL_Host.cpp](../ThunkLibs/libEGL/libEGL_Host.cpp)

#### GL
- [libGL_Guest.cpp](../ThunkLibs/libGL/libGL_Guest.cpp): Handles glXGetProcAddress
- [libGL_Host.cpp](../ThunkLibs/libGL/libGL_Host.cpp): Uses glXGetProcAddress instead of dlsym

#### SDL2
- [libSDL2_Guest.cpp](../ThunkLibs/libSDL2/libSDL2_Guest.cpp): Handles sdlglproc, dload, stubs a few log fns
- [libSDL2_Host.cpp](../ThunkLibs/libSDL2/libSDL2_Host.cpp)

#### VDSO
- [libVDSO_Guest.cpp](../ThunkLibs/libVDSO/libVDSO_Guest.cpp): Linux VDSO thunking

#### Vulkan
- [Guest.cpp](../ThunkLibs/libvulkan/Guest.cpp)
- [Host.cpp](../ThunkLibs/libvulkan/Host.cpp)

#### X11
- [libX11_Guest.cpp](../ThunkLibs/libX11/libX11_Guest.cpp): Handles callbacks and varargs
- [libX11_Host.cpp](../ThunkLibs/libX11/libX11_Host.cpp): Handles callbacks and varargs
- [libXext_Guest.cpp](../ThunkLibs/libXext/libXext_Guest.cpp)
- [libXext_Host.cpp](../ThunkLibs/libXext/libXext_Host.cpp)
- [libXfixes_Guest.cpp](../ThunkLibs/libXfixes/libXfixes_Guest.cpp)
- [libXfixes_Host.cpp](../ThunkLibs/libXfixes/libXfixes_Host.cpp)
- [libXrender_Guest.cpp](../ThunkLibs/libXrender/libXrender_Guest.cpp)
- [libXrender_Host.cpp](../ThunkLibs/libXrender/libXrender_Host.cpp)

#### asound
- [libasound_Guest.cpp](../ThunkLibs/libasound/libasound_Guest.cpp)
- [libasound_Host.cpp](../ThunkLibs/libasound/libasound_Host.cpp)

#### cef
- [libcef_Guest.cpp](../ThunkLibs/libcef/libcef_Guest.cpp)
- [libcef_Host.cpp](../ThunkLibs/libcef/libcef_Host.cpp)

#### drm
- [Guest.cpp](../ThunkLibs/libdrm/Guest.cpp)
- [Host.cpp](../ThunkLibs/libdrm/Host.cpp)

#### fex_malloc
- [Guest.cpp](../ThunkLibs/libfex_malloc/Guest.cpp): Handles allocations between guest and host thunks
- [Host.cpp](../ThunkLibs/libfex_malloc/Host.cpp): Handles allocations between guest and host thunks

#### fex_malloc_loader
- [Guest.cpp](../ThunkLibs/libfex_malloc_loader/Guest.cpp): Delays malloc symbol replacement until it is safe to run constructors

#### fex_malloc_symbols
- [Host.cpp](../ThunkLibs/libfex_malloc_symbols/Host.cpp): Allows FEX to export allocation symbols

#### wayland-client
- [Guest.cpp](../ThunkLibs/libwayland-client/Guest.cpp)
- [Host.cpp](../ThunkLibs/libwayland-client/Host.cpp)

#### xcb
- [libxcb_Guest.cpp](../ThunkLibs/libxcb/libxcb_Guest.cpp)
- [libxcb_Host.cpp](../ThunkLibs/libxcb/libxcb_Host.cpp)

#### xcb-dri2
- [libxcb-dri2_Guest.cpp](../ThunkLibs/libxcb-dri2/libxcb-dri2_Guest.cpp)
- [libxcb-dri2_Host.cpp](../ThunkLibs/libxcb-dri2/libxcb-dri2_Host.cpp)

#### xcb-dri3
- [libxcb-dri3_Guest.cpp](../ThunkLibs/libxcb-dri3/libxcb-dri3_Guest.cpp)
- [libxcb-dri3_Host.cpp](../ThunkLibs/libxcb-dri3/libxcb-dri3_Host.cpp)

#### xcb-glx
- [Guest.cpp](../ThunkLibs/libxcb-glx/Guest.cpp)
- [Host.cpp](../ThunkLibs/libxcb-glx/Host.cpp)

#### xcb-present
- [Guest.cpp](../ThunkLibs/libxcb-present/Guest.cpp)
- [Host.cpp](../ThunkLibs/libxcb-present/Host.cpp)

#### xcb-randr
- [Guest.cpp](../ThunkLibs/libxcb-randr/Guest.cpp)
- [Host.cpp](../ThunkLibs/libxcb-randr/Host.cpp)

#### xcb-shm
- [libxcb-shm_Guest.cpp](../ThunkLibs/libxcb-shm/libxcb-shm_Guest.cpp)
- [libxcb-shm_Host.cpp](../ThunkLibs/libxcb-shm/libxcb-shm_Host.cpp)

#### xcb-sync
- [Guest.cpp](../ThunkLibs/libxcb-sync/Guest.cpp)
- [Host.cpp](../ThunkLibs/libxcb-sync/Host.cpp)

#### xcb-xfixes
- [libxcb-xfixes_Guest.cpp](../ThunkLibs/libxcb-xfixes/libxcb-xfixes_Guest.cpp)
- [libxcb-xfixes_Host.cpp](../ThunkLibs/libxcb-xfixes/libxcb-xfixes_Host.cpp)

#### xshmfence
- [Guest.cpp](../ThunkLibs/libxshmfence/Guest.cpp)
- [Host.cpp](../ThunkLibs/libxshmfence/Host.cpp)

## Source/Tests

## unittests
See [unittests/Readme.md](../unittests/Readme.md) for more details

