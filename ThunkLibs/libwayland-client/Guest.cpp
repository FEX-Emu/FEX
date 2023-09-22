/*
$info$
tags: thunklibs|wayland-client
$end_info$
*/

#include <wayland-util.h>
#include <wayland-client.h>

// These must be re-declared with an initializer here, since they don't get exported otherwise
// NOTE: The initializers for these must be fetched from the host Wayland library, however
//       we can't control how these symbols are loaded since they are global const objects.
//       LD puts them in the application rodata section and ignores any nontrivial library-provided
//       initializers. There is a workaround to enable late initialization anyway in OnInit.
// NOTE: We only need to do this for interfaces exported by libwayland-client itself. Interfaces
//       defined by external libraries work fine.
extern "C" const wl_interface wl_output_interface{};
extern "C" const wl_interface wl_shm_pool_interface{};
extern "C" const wl_interface wl_pointer_interface{};
extern "C" const wl_interface wl_compositor_interface{};
extern "C" const wl_interface wl_shm_interface{};
extern "C" const wl_interface wl_registry_interface{};
extern "C" const wl_interface wl_buffer_interface{};
extern "C" const wl_interface wl_seat_interface{};
extern "C" const wl_interface wl_surface_interface {};
extern "C" const wl_interface wl_keyboard_interface {};
extern "C" const wl_interface wl_callback_interface {};

#include <algorithm>
#include <array>
#include <charconv>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <string>
#include <unordered_map>

#include "common/Guest.h"

#include <map>

#include <stdarg.h>

#include <sys/mman.h>

#include "thunkgen_guest_libwayland-client.inl"

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
static uint64_t WaylandAllocateHostTrampolineForGuestListener(void (*callback)()) {
  using cb = void(void*, wl_proxy*, typename ArgType<Signature>::type...);
  return (uint64_t)(uintptr_t)(void*)AllocateHostTrampolineForGuestFunction((cb*)callback);
}

#define WL_CLOSURE_MAX_ARGS 20

extern "C" int wl_proxy_add_listener(wl_proxy *proxy,
      void (**callback)(void), void *data) {

  // NOTE: This pointer must remain valid past the return of this function.
  // TODO: What's a good way to manage the lifetime of this?
  auto& host_callbacks = *new std::array<uint64_t, WL_CLOSURE_MAX_ARGS>;


// TODO: Fixup for 32-bit...
  for (int i = 0; i < fex_wl_get_interface_event_count(proxy); ++i) {
    char event_signature[16];
    fex_wl_get_interface_event_signature(proxy, i, event_signature);
    auto signature2 = std::string_view { event_signature };

    // A leading number indicates the minimum protocol version
    uint32_t since_version = 0;
    auto [ptr, res] = std::from_chars(signature2.begin(), signature2.end(), since_version, 10);
    auto signature = std::string { signature2.substr(ptr - signature2.begin()) };

    // ? just indicates that the argument may be null, so it doesn't change the signature
    signature.erase(std::remove(signature.begin(), signature.end(), '?'), signature.end());

    if (signature == "") {
      // E.g. xdg_toplevel::close
      host_callbacks[i] = WaylandAllocateHostTrampolineForGuestListener<>(callback[i]);
    } else if (signature == "a") {
      // E.g. xdg_toplevel::wm_capabilities
      host_callbacks[i] = WaylandAllocateHostTrampolineForGuestListener<'a'>(callback[i]);
    } else if (signature == "hu") {
      // E.g. zwp_linux_dmabuf_feedback_v1::format_table
      host_callbacks[i] = WaylandAllocateHostTrampolineForGuestListener<'h', 'u'>(callback[i]);
    } else if (signature == "i") {
      // E.g. wl_output_listener::scale
      host_callbacks[i] = WaylandAllocateHostTrampolineForGuestListener<'i'>(callback[i]);
    } else if (signature == "if") {
      // E.g. wl_touch_listener::orientation
      host_callbacks[i] = WaylandAllocateHostTrampolineForGuestListener<'i', 'f'>(callback[i]);
    } else if (signature == "iff") {
      // E.g. wl_touch_listener::shape
      host_callbacks[i] = WaylandAllocateHostTrampolineForGuestListener<'i', 'f', 'f'>(callback[i]);
    } else if (signature == "ii") {
      // E.g. xdg_toplevel::configure_bounds
      host_callbacks[i] = WaylandAllocateHostTrampolineForGuestListener<'i', 'i'>(callback[i]);
    } else if (signature == "iia") {
      // E.g. xdg_toplevel::configure
      host_callbacks[i] = WaylandAllocateHostTrampolineForGuestListener<'i', 'i', 'a'>(callback[i]);
    } else if (signature == "iiiiissi") {
      // E.g. wl_output_listener::geometry
      host_callbacks[i] = WaylandAllocateHostTrampolineForGuestListener<'i', 'i', 'i', 'i', 'i', 's', 's', 'i'>(callback[i]);
    } else if (signature == "n") {
      // E.g. wl_data_device_listener::data_offer
      host_callbacks[i] = WaylandAllocateHostTrampolineForGuestListener<'n'>(callback[i]);
    } else if (signature == "o") {
      // E.g. wl_data_device_listener::selection
      host_callbacks[i] = WaylandAllocateHostTrampolineForGuestListener<'o'>(callback[i]);
    } else if (signature == "u") {
      // E.g. wl_registry::global_remove
      host_callbacks[i] = WaylandAllocateHostTrampolineForGuestListener<'u'>(callback[i]);
    } else if (signature == "uff") {
      // E.g. wl_pointer_listener::motion
      host_callbacks[i] = WaylandAllocateHostTrampolineForGuestListener<'u', 'f', 'f'>(callback[i]);
    } else if (signature == "uhu") {
      // E.g. wl_keyboard_listener::keymap
      host_callbacks[i] = WaylandAllocateHostTrampolineForGuestListener<'u', 'h', 'u'>(callback[i]);
    } else if (signature == "ui") {
      // E.g. wl_pointer_listener::axis_discrete
      host_callbacks[i] = WaylandAllocateHostTrampolineForGuestListener<'u', 'i'>(callback[i]);
    } else if (signature == "uiff") {
      // E.g. wl_touch_listener::motion
      host_callbacks[i] = WaylandAllocateHostTrampolineForGuestListener<'u', 'i', 'f', 'f'>(callback[i]);
    } else if (signature == "uiii") {
      // E.g. wl_output_listener::mode
      host_callbacks[i] = WaylandAllocateHostTrampolineForGuestListener<'u', 'i', 'i', 'i'>(callback[i]);
    } else if (signature == "uo") {
      // E.g. wl_pointer_listener::leave
      host_callbacks[i] = WaylandAllocateHostTrampolineForGuestListener<'u', 'o'>(callback[i]);
    } else if (signature == "uoa") {
      // E.g. wl_keyboard_listener::enter
      host_callbacks[i] = WaylandAllocateHostTrampolineForGuestListener<'u', 'o', 'a'>(callback[i]);
    } else if (signature == "uoff") {
      // E.g. wl_pointer_listener::enter
      host_callbacks[i] = WaylandAllocateHostTrampolineForGuestListener<'u', 'o', 'f', 'f'>(callback[i]);
    } else if (signature == "uoffo") {
      // E.g. wl_data_device_listener::enter
      host_callbacks[i] = WaylandAllocateHostTrampolineForGuestListener<'u', 'o', 'f', 'f', 'o'>(callback[i]);
    } else if (signature == "usu") {
      // E.g. wl_registry::global
      host_callbacks[i] = WaylandAllocateHostTrampolineForGuestListener<'u', 's', 'u'>(callback[i]);
    } else if (signature == "uu") {
      // E.g. wl_pointer_listener::axis_stop
      host_callbacks[i] = WaylandAllocateHostTrampolineForGuestListener<'u', 'u'>(callback[i]);
    } else if (signature == "uuf") {
      // E.g. wl_pointer_listener::axis
      host_callbacks[i] = WaylandAllocateHostTrampolineForGuestListener<'u', 'u', 'f'>(callback[i]);
    } else if (signature == "uui") {
      // E.g. wl_touch_listener::up
      host_callbacks[i] = WaylandAllocateHostTrampolineForGuestListener<'u', 'u', 'i'>(callback[i]);
    } else if (signature == "uuoiff") {
      // E.g. wl_touch_listener::down
      host_callbacks[i] = WaylandAllocateHostTrampolineForGuestListener<'u', 'u', 'o', 'i', 'f', 'f'>(callback[i]);
    } else if (signature == "uuu") {
      // E.g. zwp_linux_dmabuf_v1::modifier
      host_callbacks[i] = WaylandAllocateHostTrampolineForGuestListener<'u', 'u', 'u'>(callback[i]);
    } else if (signature == "uuuu") {
      // E.g. wl_pointer_listener::button
      host_callbacks[i] = WaylandAllocateHostTrampolineForGuestListener<'u', 'u', 'u', 'u'>(callback[i]);
    } else if (signature == "uuuuu") {
      // E.g. wl_keyboard_listener::modifiers
      host_callbacks[i] = WaylandAllocateHostTrampolineForGuestListener<'u', 'u', 'u', 'u', 'u'>(callback[i]);
    } else if (signature == "s") {
      // E.g. wl_seat::name
      host_callbacks[i] = WaylandAllocateHostTrampolineForGuestListener<'s'>(callback[i]);
    } else if (signature == "sii") {
      // E.g. zwp_text_input_v3::preedit_string
      host_callbacks[i] = WaylandAllocateHostTrampolineForGuestListener<'s', 'i', 'i'>(callback[i]);
    } else {
      fprintf(stderr, "TODO: Unknown wayland event signature descriptor %s\n", signature.data());
      std::abort();
    }
  }

  return fexfn_pack_wl_proxy_add_listener(proxy, (void(**)())host_callbacks.data(), data);
}

struct argument_details {
  char type;
  int nullable;
};

static const char* get_next_argument(const char *signature, argument_details *details)
{
  details->nullable = 0;
  for(; *signature; ++signature) {
    switch(*signature) {
    case 'i':
    case 'u':
    case 'f':
    case 's':
    case 'o':
    case 'n':
    case 'a':
    case 'h':
      details->type = *signature;
      return signature + 1;
    case '?':
      details->nullable = 1;
    }
  }
  details->type = '\0';
  return signature;
}

static void wl_argument_from_va_list(const char *signature, wl_argument *args,
                                     int count, va_list ap) {
  int i;
  const char *sig_iter;
  argument_details arg;

  sig_iter = signature;
  for (i = 0; i < count; i++) {
    sig_iter = get_next_argument(sig_iter, &arg);

    switch (arg.type) {
    case 'i':
      args[i].i = va_arg(ap, int32_t);
      break;
    case 'u':
      args[i].u = va_arg(ap, uint32_t);
      break;
    case 'f':
      args[i].f = va_arg(ap, wl_fixed_t);
      break;
    case 's':
      args[i].s = va_arg(ap, const char *);
      break;
    case 'o':
      args[i].o = va_arg(ap, struct wl_object *);
      break;
    case 'n':
      args[i].o = va_arg(ap, struct wl_object *);
      break;
    case 'a':
      args[i].a = va_arg(ap, struct wl_array *);
      break;
    case 'h':
      args[i].h = va_arg(ap, int32_t);
      break;
    case '\0':
      return;
    }
  }
}

extern "C" void wl_proxy_marshal(wl_proxy *proxy, uint32_t opcode, ...) {
  wl_argument args[WL_CLOSURE_MAX_ARGS];
  va_list ap;

  va_start(ap, opcode);
  // This is equivalent to reading ((wl_proxy_private*)proxy)->interface->methods[opcode].signature on 64-bit.
  // On 32-bit, the data layout differs between host and guest however, so we let the host extract the data.
  char signature[64];
  fex_wl_get_method_signature(proxy, opcode, signature);
  wl_argument_from_va_list(signature, args, WL_CLOSURE_MAX_ARGS, ap);
  va_end(ap);

  wl_proxy_marshal_array(proxy, opcode, args);
}

extern "C" wl_proxy *wl_proxy_marshal_constructor(wl_proxy *proxy, uint32_t opcode,
           const wl_interface *interface, ...) {
  wl_argument args[WL_CLOSURE_MAX_ARGS];
  va_list ap;

  va_start(ap, interface);
  // This is equivalent to reading ((wl_proxy_private*)proxy)->interface->methods[opcode].signature on 64-bit.
  // On 32-bit, the data layout differs between host and guest however, so we let the host extract the data.
  char signature[64];
  fex_wl_get_method_signature(proxy, opcode, signature);
  wl_argument_from_va_list(signature, args, WL_CLOSURE_MAX_ARGS, ap);
  va_end(ap);

  return wl_proxy_marshal_array_constructor(proxy, opcode, args, interface);
}

extern "C" wl_proxy *wl_proxy_marshal_constructor_versioned(wl_proxy *proxy, uint32_t opcode,
           const wl_interface *interface, uint32_t version, ...) {
  wl_argument args[WL_CLOSURE_MAX_ARGS];
  va_list ap;

  va_start(ap, version);
  // This is equivalent to reading ((wl_proxy_private*)proxy)->interface->methods[opcode].signature on 64-bit.
  // On 32-bit, the data layout differs between host and guest however, so we let the host extract the data.
  char signature[64];
  fex_wl_get_method_signature(proxy, opcode, signature);
  wl_argument_from_va_list(signature, args, WL_CLOSURE_MAX_ARGS, ap);
  va_end(ap);

  return wl_proxy_marshal_array_constructor_versioned(proxy, opcode, args, interface, version);
}

extern "C" wl_proxy *wl_proxy_marshal_flags(wl_proxy *proxy, uint32_t opcode,
           const wl_interface *interface,
           uint32_t version,
           uint32_t flags, ...) {
  wl_argument args[WL_CLOSURE_MAX_ARGS];
  va_list ap;

  va_start(ap, flags);
  // This is equivalent to reading ((wl_proxy_private*)proxy)->interface->methods[opcode].signature on 64-bit.
  // On 32-bit, the data layout differs between host and guest however, so we let the host extract the data.
  char signature[64];
  fex_wl_get_method_signature(proxy, opcode, signature);
  wl_argument_from_va_list(signature, args, WL_CLOSURE_MAX_ARGS, ap);
  va_end(ap);

  return wl_proxy_marshal_array_flags(proxy, opcode, interface, version, flags, args);
}

extern "C" void wl_log_set_handler_client(wl_log_func_t handler) {
  // Ignore
}


void OnInit() {
  fex_wl_exchange_interface_pointer(const_cast<wl_interface*>(&wl_output_interface), "wl_output");
  fex_wl_exchange_interface_pointer(const_cast<wl_interface*>(&wl_shm_pool_interface), "wl_shm_pool");
  fex_wl_exchange_interface_pointer(const_cast<wl_interface*>(&wl_pointer_interface), "wl_pointer");
  fex_wl_exchange_interface_pointer(const_cast<wl_interface*>(&wl_compositor_interface), "wl_compositor");
  fex_wl_exchange_interface_pointer(const_cast<wl_interface*>(&wl_shm_interface), "wl_shm");
  fex_wl_exchange_interface_pointer(const_cast<wl_interface*>(&wl_registry_interface), "wl_registry");
  fex_wl_exchange_interface_pointer(const_cast<wl_interface*>(&wl_buffer_interface), "wl_buffer");
  fex_wl_exchange_interface_pointer(const_cast<wl_interface*>(&wl_seat_interface), "wl_seat");
  fex_wl_exchange_interface_pointer(const_cast<wl_interface*>(&wl_surface_interface), "wl_surface");
  fex_wl_exchange_interface_pointer(const_cast<wl_interface*>(&wl_keyboard_interface), "wl_keyboard");
  fex_wl_exchange_interface_pointer(const_cast<wl_interface*>(&wl_callback_interface), "wl_callback");
}

LOAD_LIB_INIT(libwayland-client, OnInit)
