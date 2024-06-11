#include "Host.h"

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <dlfcn.h>
#include <mutex>
#include <unordered_map>

#include <X11/Xlib.h>
#include <xcb/xcb.h>

#ifdef IS_32BIT_THUNK
using guest_long = int32_t;
using guest_size_t = int32_t;
#else
using guest_long = long;
using guest_size_t = size_t;
#endif

/**
 * Guest X11 displays and xcb connections can't be used by the host, so
 * instead an intermediary object is created and mapped to the original
 * guest display/connection.
 */
struct X11Manager {
  std::mutex mutex;

  // Maps guest connection to intermediary host connection
  std::unordered_map<xcb_connection_t*, xcb_connection_t*> connections;

  xcb_connection_t* GuestToHostConnection(xcb_connection_t* GuestConnection) {
    std::unique_lock lock(mutex);
    auto [it, inserted] = connections.emplace(GuestConnection, nullptr);
    if (inserted) {
      // NOTE: There's no easy way to query the display name from the guest, so just connect to the default display.
      static void* libxcb = dlopen("libxcb.so.1", RTLD_LAZY);
      static auto ptr_xcb_connect = (decltype(&xcb_connect))dlsym(libxcb, "xcb_connect");
      static auto ptr_xcb_connection_has_error = (decltype(&xcb_connection_has_error))dlsym(libxcb, "xcb_connection_has_error");
      it->second = ptr_xcb_connect(nullptr, nullptr);
      if (ptr_xcb_connection_has_error(it->second)) {
        fprintf(stderr, "ERROR: Could not open xcb connection\n");
        std::abort();
      }
    }
    return it->second;
  }

  // Maps guest display to intermediary host display
  std::unordered_map<_XDisplay*, _XDisplay*> displays;

  _XDisplay* GuestToHostDisplay(_XDisplay* GuestDisplay) {
    // Flush event queue to make effects of the guest-side connection visible
    GuestXSync(GuestDisplay, 0);

    std::unique_lock lock(mutex);
    auto [it, inserted] = displays.emplace(GuestDisplay, nullptr);
    if (inserted) {
      auto host_display = HostXOpenDisplay(GuestXDisplayString(GuestDisplay));
      fprintf(stderr, "Opening host-side X11 display: %p -> %p\n", GuestDisplay, host_display);
      if (!host_display) {
        fprintf(stderr, "ERROR: Could not open X display\n");
        std::abort();
      } else {
        it->second = host_display;
      }
    }
    return it->second;
  }

  guest_layout<_XDisplay*> HostToGuestDisplay(const _XDisplay* from) {
    if (from == nullptr) {
      return {.data = 0};
    }

    std::unique_lock lock(mutex);
    for (auto& [guest, host] : displays) {
      if (host == from) {
        guest_layout<_XDisplay*> ret;
        ret.data = reinterpret_cast<uintptr_t>(guest);
        return ret;
      }
    }

    fprintf(stderr, "ERROR: Could not map host display %p back to guest\n", from);
    std::abort();
  }

  static void* GetLibX11() {
    static void* libx11 = dlopen("libX11.so.6", RTLD_LAZY);
    return libx11;
  }

  static int HostXFree(void* Ptr) {
    static auto func = reinterpret_cast<decltype(&XFree)>(dlsym(GetLibX11(), "XFree"));
    return func(Ptr);
  }

  static int HostXFlush(Display* Dis) {
    static auto func = reinterpret_cast<decltype(&XFlush)>(dlsym(GetLibX11(), "XFlush"));
    return func(Dis);
  }

  static Display* HostXOpenDisplay(const char* Name) {
    static auto func = reinterpret_cast<decltype(&XOpenDisplay)>(dlsym(GetLibX11(), "XOpenDisplay"));
    return func(Name);
  }

  static XVisualInfo* HostXGetVisualInfo(Display* a, long b, XVisualInfo* c, int* d) {
    static auto func = reinterpret_cast<decltype(&XGetVisualInfo)>(dlsym(GetLibX11(), "XGetVisualInfo"));
    return func(a, b, c, d);
  }

  // NOTE: Struct pointers are replaced by void* to avoid involving data layout conversion here.
  int (*GuestXSync)(void*, int) = nullptr;
  void* (*GuestXGetVisualInfo)(void*, guest_long, void*, int*) = nullptr;

  // XDisplayString internally just reads data from _XDisplay's internal struct definition.
  // This breaks when data layout is different, so allow reading from a guest context instead.
  char* (*GuestXDisplayString)(void*) = nullptr;
};
