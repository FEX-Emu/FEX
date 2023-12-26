/*
$info$
tags: thunklibs|wayland-client
$end_info$
*/

#include <string_view>
#include <wayland-client.h>

#include <stdio.h>

#include "common/Host.h"
#include <dlfcn.h>

#include <sys/mman.h>

#include <algorithm>
#include <array>
#include <charconv>
#include <cstring>
#include <map>
#include <string>

#include "thunkgen_host_libwayland-client.inl"

struct wl_proxy_private {
  wl_interface* interface;
  // Other data members omitted
};

// See wayland-util.h for documentation on protocol message signatures
template<char> struct ArgType;
template<> struct ArgType<'s'> { using type = const char*; };
template<> struct ArgType<'u'> { using type = uint32_t; };
template<> struct ArgType<'i'> { using type = int32_t; };
template<> struct ArgType<'o'> { using type = wl_proxy*; };
template<> struct ArgType<'n'> { using type = wl_proxy*; };
template<> struct ArgType<'a'> { using type = wl_array*; };
template<> struct ArgType<'f'> { using type = wl_fixed_t; };
template<> struct ArgType<'h'> { using type = int32_t; }; // fd?

template<char... Signature>
static void WaylandFinalizeHostTrampolineForGuestListener(void (*callback)()) {
  using cb = void(void*, wl_proxy*, typename ArgType<Signature>::type...);
  FinalizeHostTrampolineForGuestFunction((cb*)callback);
}

extern "C" int fexfn_impl_libwayland_client_wl_proxy_add_listener(struct wl_proxy *proxy,
      guest_layout<void (**)(void)> callback_raw, void* data) {
  auto guest_interface = ((wl_proxy_private*)proxy)->interface;

  for (int i = 0; i < guest_interface->event_count; ++i) {
    auto signature_view = std::string_view { guest_interface->events[i].signature };

    // A leading number indicates the minimum protocol version
    uint32_t since_version = 0;
    auto [ptr, res] = std::from_chars(signature_view.begin(), signature_view.end(), since_version, 10);
    std::string signature { ptr, &*signature_view.end() };

    // ? just indicates that the argument may be null, so it doesn't change the signature
    signature.erase(std::remove(signature.begin(), signature.end(), '?'), signature.end());

    auto callback = reinterpret_cast<void(*)()>(uintptr_t { callback_raw.get_pointer()[i].data });

    if (signature == "") {
      // E.g. xdg_toplevel::close
      WaylandFinalizeHostTrampolineForGuestListener<>(callback);
    } else if (signature == "a") {
      // E.g. xdg_toplevel::wm_capabilities
      WaylandFinalizeHostTrampolineForGuestListener<'a'>(callback);
    } else if (signature == "hu") {
      // E.g. zwp_linux_dmabuf_feedback_v1::format_table
      WaylandFinalizeHostTrampolineForGuestListener<'h', 'u'>(callback);
    } else if (signature == "i") {
      // E.g. wl_output_listener::scale
      WaylandFinalizeHostTrampolineForGuestListener<'i'>(callback);
    } else if (signature == "if") {
      // E.g. wl_touch_listener::orientation
      WaylandFinalizeHostTrampolineForGuestListener<'i', 'f'>(callback);
    } else if (signature == "iff") {
      // E.g. wl_touch_listener::shape
      WaylandFinalizeHostTrampolineForGuestListener<'i', 'f', 'f'>(callback);
    } else if (signature == "ii") {
      // E.g. xdg_toplevel::configure_bounds
      WaylandFinalizeHostTrampolineForGuestListener<'i', 'i'>(callback);
    } else if (signature == "iia") {
      // E.g. xdg_toplevel::configure
      WaylandFinalizeHostTrampolineForGuestListener<'i', 'i', 'a'>(callback);
    } else if (signature == "iiiiissi") {
      // E.g. wl_output_listener::geometry
      WaylandFinalizeHostTrampolineForGuestListener<'i', 'i', 'i', 'i', 'i', 's', 's', 'i'>(callback);
    } else if (signature == "n") {
      // E.g. wl_data_device_listener::data_offer
      WaylandFinalizeHostTrampolineForGuestListener<'n'>(callback);
    } else if (signature == "o") {
      // E.g. wl_data_device_listener::selection
      WaylandFinalizeHostTrampolineForGuestListener<'o'>(callback);
    } else if (signature == "u") {
      // E.g. wl_registry::global_remove
      WaylandFinalizeHostTrampolineForGuestListener<'u'>(callback);
    } else if (signature == "uff") {
      // E.g. wl_pointer_listener::motion
      WaylandFinalizeHostTrampolineForGuestListener<'u', 'f', 'f'>(callback);
    } else if (signature == "uhu") {
      // E.g. wl_keyboard_listener::keymap
      WaylandFinalizeHostTrampolineForGuestListener<'u', 'h', 'u'>(callback);
    } else if (signature == "ui") {
      // E.g. wl_pointer_listener::axis_discrete
      WaylandFinalizeHostTrampolineForGuestListener<'u', 'i'>(callback);
    } else if (signature == "uiff") {
      // E.g. wl_touch_listener::motion
      WaylandFinalizeHostTrampolineForGuestListener<'u', 'i', 'f', 'f'>(callback);
    } else if (signature == "uiii") {
      // E.g. wl_output_listener::mode
      WaylandFinalizeHostTrampolineForGuestListener<'u', 'i', 'i', 'i'>(callback);
    } else if (signature == "uo") {
      // E.g. wl_pointer_listener::leave
      WaylandFinalizeHostTrampolineForGuestListener<'u', 'o'>(callback);
    } else if (signature == "uoa") {
      // E.g. wl_keyboard_listener::enter
      WaylandFinalizeHostTrampolineForGuestListener<'u', 'o', 'a'>(callback);
    } else if (signature == "uoff") {
      // E.g. wl_pointer_listener::enter
      WaylandFinalizeHostTrampolineForGuestListener<'u', 'o', 'f', 'f'>(callback);
    } else if (signature == "uoffo") {
      // E.g. wl_data_device_listener::enter
      WaylandFinalizeHostTrampolineForGuestListener<'u', 'o', 'f', 'f', 'o'>(callback);
    } else if (signature == "usu") {
      // E.g. wl_registry::global
      WaylandFinalizeHostTrampolineForGuestListener<'u', 's', 'u'>(callback);
    } else if (signature == "uu") {
      // E.g. wl_pointer_listener::axis_stop
      WaylandFinalizeHostTrampolineForGuestListener<'u', 'u'>(callback);
    } else if (signature == "uuf") {
      // E.g. wl_pointer_listener::axis
      WaylandFinalizeHostTrampolineForGuestListener<'u', 'u', 'f'>(callback);
    } else if (signature == "uui") {
      // E.g. wl_touch_listener::up
      WaylandFinalizeHostTrampolineForGuestListener<'u', 'u', 'i'>(callback);
    } else if (signature == "uuoiff") {
      // E.g. wl_touch_listener::down
      WaylandFinalizeHostTrampolineForGuestListener<'u', 'u', 'o', 'i', 'f', 'f'>(callback);
    } else if (signature == "uuu") {
      // E.g. zwp_linux_dmabuf_v1::modifier
      WaylandFinalizeHostTrampolineForGuestListener<'u', 'u', 'u'>(callback);
    } else if (signature == "uuuu") {
      // E.g. wl_pointer_listener::button
      WaylandFinalizeHostTrampolineForGuestListener<'u', 'u', 'u', 'u'>(callback);
    } else if (signature == "uuuuu") {
      // E.g. wl_keyboard_listener::modifiers
      WaylandFinalizeHostTrampolineForGuestListener<'u', 'u', 'u', 'u', 'u'>(callback);
    } else if (signature == "s") {
      // E.g. wl_seat::name
      WaylandFinalizeHostTrampolineForGuestListener<'s'>(callback);
    } else if (signature == "sii") {
      // E.g. zwp_text_input_v3::preedit_string
      WaylandFinalizeHostTrampolineForGuestListener<'s', 'i', 'i'>(callback);
    } else {
      fprintf(stderr, "TODO: Unknown wayland event signature descriptor %s\n", signature.data());
      std::abort();
    }
  }

  // Pass the original function pointer table to the host wayland library. This ensures the table is valid until the listener is unregistered.
  return fexldr_ptr_libwayland_client_wl_proxy_add_listener(proxy,
                                                            reinterpret_cast<void(**)(void)>(callback_raw.get_pointer()),
                                                            data);
}

wl_interface* fexfn_impl_libwayland_client_fex_wl_exchange_interface_pointer(wl_interface* guest_interface, char const* name) {
  auto host_interface = reinterpret_cast<wl_interface*>(dlsym(fexldr_ptr_libwayland_client_so, (std::string { name } + "_interface").c_str()));
  if (!host_interface) {
    fprintf(stderr, "Could not find host interface corresponding to %p (%s)\n", guest_interface, name);
    std::abort();
  }

  // Wayland-client declares interface pointers as `const`, which makes LD put
  // them into the rodata section of the application itself instead of the
  // library. To copy the host information to them on startup, we must
  // temporarily disable write-protection on this data hence.
  auto page_begin = reinterpret_cast<uintptr_t>(guest_interface) & ~uintptr_t { 0xfff };
  if (0 != mprotect((void*)page_begin, 0x1000, PROT_READ | PROT_WRITE)) {
    fprintf(stderr, "ERROR: %s\n", strerror(errno));
    std::abort();
  }

#ifdef IS_32BIT_THUNK
// Requires struct repacking for wl_interface
#error Not implemented
#else
  memcpy(guest_interface, host_interface, sizeof(wl_interface));
#endif

// TODO: Disabled until we ensure the interface data is indeed stored in rodata
//  mprotect((void*)page_begin, 0x1000, PROT_READ);

  return host_interface;
}

EXPORTS(libwayland_client)
