/*
$info$
tags: thunklibs|wayland-client
$end_info$
*/

#include <string_view>
#include <unordered_map>
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

#include <ranges>

template<>
struct guest_layout<wl_argument> {
#ifdef IS_32BIT_THUNK
  using type = uint32_t;
#else
  using type = wl_argument;
#endif
  type data;

  // TODO: Make this conversion explicit
  guest_layout& operator=(const wl_argument from) {
#ifdef IS_32BIT_THUNK
    data = from.u;
#else
    data = from;
#endif
    return *this;
  }
};


#include "thunkgen_host_libwayland-client.inl"

// Maps guest interface to host_interfaces
static std::map<void*, wl_interface*> guest_to_host_interface;

struct wl_proxy_private {
  wl_interface* interface;
  // Other data members omitted
};

static wl_interface* lookup_wl_interface(const wl_interface* interface);

#ifdef IS_32BIT_THUNK
static void repack_guest_wl_interface_to_host(const wl_interface* guest_interface, wl_interface* host_interface) {
  struct guest_wl_message {
    guest_layout<char*> name;
    guest_layout<char*> signature;
    guest_layout<wl_interface**> types;
  };

  // Name pointer (offset 0) can safely be zero-extended
  host_interface->name = (const char*)(uintptr_t)(uint32_t)reinterpret_cast<uintptr_t>(guest_interface->name);
  memcpy(&host_interface->version, reinterpret_cast<const char*>(guest_interface) + 4, 4);
  memcpy(&host_interface->method_count, reinterpret_cast<const char*>(guest_interface) + 8, 4);
  // TODO: Manage lifetime of this memory?
  host_interface->methods = new wl_message[host_interface->method_count];
  memset((void*)host_interface->methods, 0, sizeof(wl_message) * host_interface->method_count);
  for (int i = 0; i < host_interface->method_count; ++i) {
    auto guest_method = &((*(guest_wl_message**)(reinterpret_cast<const char*>(guest_interface) + 12))[i]);
    guest_method = (guest_wl_message*)((uintptr_t)guest_method & 0xffffffff);
    // Sign-extend these pointers
    auto name_ptr = guest_method->name.get_pointer();
    memcpy((void*)&host_interface->methods[i].name, &name_ptr, sizeof(name_ptr));
    auto sig_ptr = guest_method->signature.get_pointer();
    memcpy((void*)&host_interface->methods[i].signature, &sig_ptr, sizeof(sig_ptr));

    // TODO: Handle zero-argument case... can't use new with 0 elements!
    auto num_types = std::ranges::count_if(std::string_view { host_interface->methods[i].signature }, [](char c) { return std::isalpha(c); });
    auto types = new wl_interface*[num_types];
    for (int type = 0; type < num_types; ++type) {
      uintptr_t guest_interface_addr = ((uint32_t*)(uintptr_t)guest_method->types.data)[type];
      types[type] = guest_interface_addr ? lookup_wl_interface(reinterpret_cast<const wl_interface*>(guest_interface_addr)) : nullptr;
      fprintf(stderr, "  METHOD %d type %d: %p\n", i, type, types[type]);
    }
    memcpy((void*)&host_interface->methods[i].types, &types, sizeof(types));
  }

  memcpy(&host_interface->event_count, reinterpret_cast<const char*>(guest_interface) + 16, sizeof(host_interface->event_count));
  // TODO: Manage lifetime of this memory?
  host_interface->events = new wl_message[host_interface->event_count];
  memset((void*)host_interface->events, 0, sizeof(wl_message) * host_interface->event_count);
  for (int i = 0; i < host_interface->event_count; ++i) {
    auto guest_event = &((*(guest_wl_message**)(reinterpret_cast<const char*>(guest_interface) + 20))[i]);
    guest_event = (guest_wl_message*)((uintptr_t)guest_event & 0xffffffff);
    // Sign-extend these pointers
    auto name_ptr = guest_event->name.get_pointer();
    memcpy((void*)&host_interface->events[i].name, &name_ptr, sizeof(name_ptr));
    auto sig_ptr = guest_event->signature.get_pointer();
    memcpy((void*)&host_interface->events[i].signature, &sig_ptr, sizeof(sig_ptr));

    auto num_types = std::ranges::count_if(std::string_view { host_interface->events[i].signature }, [](char c) { return std::isalpha(c); });
    auto types = new wl_interface*[num_types];
    for (int type = 0; type < num_types; ++type) {
      uintptr_t guest_interface_addr = ((uint32_t*)(uintptr_t)guest_event->types.data)[type];
      types[type] = guest_interface_addr ? lookup_wl_interface(reinterpret_cast<const wl_interface*>(guest_interface_addr)) : nullptr;
    }
    memcpy((void*)&host_interface->events[i].types, &types, sizeof(types));
  }
}
#endif

// Maps guest interface pointers to host pointers
wl_interface* lookup_wl_interface(const wl_interface* interface) {
  // Used e.g. for wl_shm_pool_destroy
  if (interface == nullptr) {
    return nullptr;
  }

  if (!guest_to_host_interface.count((void*)interface)) {
  bool is_host = (uint32_t)interface->method_count < 0x1000 && (uint32_t)interface->event_count < 0x1000;
    fprintf(stderr, "Unknown wayland interface %p, adding to registry (as %s)\n", interface, is_host ? "host" : "guest");
    auto [host_interface_it, inserted] = guest_to_host_interface.emplace((void*)interface, new wl_interface);
#ifdef IS_32BIT_THUNK
if ((uint32_t)interface->method_count < 0x1000 && (uint32_t)interface->event_count < 0x1000) {
  // This is actually a host interface somehow reaching this function (otherwise the "method" pointer would bleed into method_count). This doesn't need repacking.
  // TODO: Investigate if this can be avoided
  memcpy(host_interface_it->second, interface, sizeof(wl_interface));
} else {
  wl_interface* host_interface = host_interface_it->second;
  repack_guest_wl_interface_to_host(interface, host_interface);
}
#else
    memcpy(host_interface_it->second, interface, sizeof(wl_interface));
#endif
  }

  return guest_to_host_interface.at((void*)interface);
}

// TODO: Reduce code duplication between this and its _flags variant
extern "C" void
fexfn_impl_libwayland_client_wl_proxy_marshal_array(
            wl_proxy *proxy, uint32_t opcode,
            guest_layout<wl_argument*> args) {
#define WL_CLOSURE_MAX_ARGS 20
  std::array<wl_argument, WL_CLOSURE_MAX_ARGS> host_args;
  for (int i = 0; i < host_args.size(); ++i) {
    // NOTE: wl_argument can store a pointer argument, so for 32-bit guests
    //       we need to make sure the upper 32-bits are explicitly zeroed
    std::memset(&host_args[i], 0, sizeof(host_args[i]));
    std::memcpy(&host_args[i], &args.get_pointer()[i], sizeof(args.get_pointer()[i]));
  }

  fexldr_ptr_libwayland_client_wl_proxy_marshal_array(proxy, opcode, host_args.data());
}

extern "C" wl_proxy*
fexfn_impl_libwayland_client_wl_proxy_marshal_array_constructor_versioned(
            wl_proxy *proxy, uint32_t opcode,
            guest_layout<wl_argument*> args,
            const wl_interface *interface,
            uint32_t version) {
  interface = lookup_wl_interface(interface);

  // NOTE: "interface" and "name" are the first members of their respective parent structs, so this is safe to read even on 32-bit
  if (((wl_proxy_private*)proxy)->interface->name == std::string_view { "wl_registry" }) {
    auto& host_interface = guest_to_host_interface[(void*)interface];
    if (!host_interface) {
      host_interface = new wl_interface;
#ifdef IS_32BIT_THUNK
      if ((uint32_t)interface->method_count < 0x1000 && (uint32_t)interface->event_count < 0x1000) {
        // This is actually a host interface somehow reaching this function (otherwise the "method" pointer would bleed into method_count). This doesn't need repacking.
        // TODO: Investigate if this can be avoided
        memcpy(host_interface, interface, sizeof(wl_interface));
      } else {
        repack_guest_wl_interface_to_host(interface, host_interface);
      }
#else
      memcpy(host_interface, interface, sizeof(wl_interface));
#endif
    }
  }

#define WL_CLOSURE_MAX_ARGS 20
  std::array<wl_argument, WL_CLOSURE_MAX_ARGS> host_args;
  for (int i = 0; i < host_args.size(); ++i) {
    // NOTE: wl_argument can store a pointer argument, so for 32-bit guests
    //       we need to make sure the upper 32-bits are explicitly zeroed
    std::memset(&host_args[i], 0, sizeof(host_args[i]));
    std::memcpy(&host_args[i], &args.get_pointer()[i], sizeof(args.get_pointer()[i]));
  }

  return fexldr_ptr_libwayland_client_wl_proxy_marshal_array_constructor_versioned(proxy, opcode, host_args.data(), interface, version);
}

extern "C" wl_proxy*
fexfn_impl_libwayland_client_wl_proxy_marshal_array_constructor(
            wl_proxy *proxy, uint32_t opcode,
            guest_layout<wl_argument*> args,
            const wl_interface *interface) {
  interface = lookup_wl_interface(interface);

  // NOTE: "interface" and "name" are the first members of their respective parent structs, so this is safe to read even on 32-bit
  if (((wl_proxy_private*)proxy)->interface->name == std::string_view { "wl_registry" }) {
    auto& host_interface = guest_to_host_interface[(void*)interface];
    if (!host_interface) {
      host_interface = new wl_interface;
#ifdef IS_32BIT_THUNK
      if ((uint32_t)interface->method_count < 0x1000 && (uint32_t)interface->event_count < 0x1000) {
        // This is actually a host interface somehow reaching this function (otherwise the "method" pointer would bleed into method_count). This doesn't need repacking.
        // TODO: Investigate if this can be avoided
        memcpy(host_interface, interface, sizeof(wl_interface));
      } else {
        repack_guest_wl_interface_to_host(interface, host_interface);
      }
#else
      memcpy(host_interface, interface, sizeof(wl_interface));
#endif
    }
  }

#define WL_CLOSURE_MAX_ARGS 20
  std::array<wl_argument, WL_CLOSURE_MAX_ARGS> host_args;
  for (int i = 0; i < host_args.size(); ++i) {
    // NOTE: wl_argument can store a pointer argument, so for 32-bit guests
    //       we need to make sure the upper 32-bits are explicitly zeroed
    std::memset(&host_args[i], 0, sizeof(host_args[i]));
    std::memcpy(&host_args[i], &args.get_pointer()[i], sizeof(args.get_pointer()[i]));
  }

  return fexldr_ptr_libwayland_client_wl_proxy_marshal_array_constructor(proxy, opcode, host_args.data(), interface);
}

extern "C" wl_proxy*
fexfn_impl_libwayland_client_wl_proxy_marshal_array_flags(
            wl_proxy *proxy, uint32_t opcode,
            const wl_interface *interface,
            uint32_t version, uint32_t flags,
            guest_layout<wl_argument*> args) {
  interface = lookup_wl_interface(interface);

  // NOTE: "interface" and "name" are the first members of their respective parent structs, so this is safe to read even on 32-bit
  if (((wl_proxy_private*)proxy)->interface->name == std::string_view { "wl_registry" }) {
    auto& host_interface = guest_to_host_interface[(void*)interface];
    if (!host_interface) {
      host_interface = new wl_interface;
#ifdef IS_32BIT_THUNK
      if ((uint32_t)interface->method_count < 0x1000 && (uint32_t)interface->event_count < 0x1000) {
        // This is actually a host interface somehow reaching this function (otherwise the "method" pointer would bleed into method_count). This doesn't need repacking.
        // TODO: Investigate if this can be avoided
        memcpy(host_interface, interface, sizeof(wl_interface));
      } else {
        repack_guest_wl_interface_to_host(interface, host_interface);
      }
#else
      memcpy(host_interface, interface, sizeof(wl_interface));
#endif
    }
  }

#define WL_CLOSURE_MAX_ARGS 20
  std::array<wl_argument, WL_CLOSURE_MAX_ARGS> host_args;
  for (int i = 0; i < host_args.size(); ++i) {
    // NOTE: wl_argument can store a pointer argument, so for 32-bit guests
    //       we need to make sure the upper 32-bits are explicitly zeroed
    std::memset(&host_args[i], 0, sizeof(host_args[i]));
    std::memcpy(&host_args[i], &args.get_pointer()[i], sizeof(args.get_pointer()[i]));
  }

  return fexldr_ptr_libwayland_client_wl_proxy_marshal_array_flags(proxy, opcode, interface, version, flags, host_args.data());
}

// Generalization of CallbackUnpack::CallGuestPtr that repacks an wl_array parameter
// TODO: Provide a generic solution for this...
template<typename Result, typename... Args>
static auto CallGuestPtrWithWaylandArray(Args... args, struct wl_array *array) -> Result {
  GuestcallInfo *guestcall;
  LOAD_INTERNAL_GUESTPTR_VIA_CUSTOM_ABI(guestcall);

  // TODO: What lifetime does this have? Can it be stack-allocated instead?
  // TODO: Cleanup memory if heap allocation is indeed necessary
  guest_layout<wl_array>* guest_array = new guest_layout<wl_array>;
  guest_array->data.size = array->size;
  guest_array->data.alloc = array->alloc;
  guest_array->data.data = array->data;
  guest_layout<wl_array*> guest_array_ptr = { .data = static_cast<guest_layout<wl_array*>::type>(reinterpret_cast<uintptr_t>(guest_array)) };

  PackedArguments<Result, guest_layout<Args>..., guest_layout<wl_array*>> packed_args = {
    to_guest(to_host_layout(args))..., guest_array_ptr
  };
  guestcall->CallCallback(guestcall->GuestUnpacker, guestcall->GuestTarget, &packed_args);

  if constexpr (!std::is_void_v<Result>) {
    return packed_args.rv;
  }
}

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
      guest_layout<void (**)(void)> callback, guest_layout<void *> data) {

  auto guest_interface = lookup_wl_interface(((wl_proxy_private*)proxy)->interface);

  // Drop upper 32-bits, since they are likely garbage... TODO: Let the repacker do this, or check if it's already done even
//  auto callback2 = (uint32_t*)(uintptr_t)(uint32_t)(uintptr_t)callback;
  auto callback2 = (void(**)(void))callback.get_pointer();

  for (int i = 0; i < guest_interface->event_count; ++i) {
    auto signature_view = std::string_view { guest_interface->events[i].signature };

    // A leading number indicates the minimum protocol version
    uint32_t since_version = 0;
    auto [ptr, res] = std::from_chars(signature_view.begin(), signature_view.end(), since_version, 10);
    auto signature = std::string { signature_view.substr(ptr - signature_view.begin()) };

    // ? just indicates that the argument may be null, so it doesn't change the signature
    signature.erase(std::remove(signature.begin(), signature.end(), '?'), signature.end());

    auto callback = callback2[i];

    if (signature == "") {
      // E.g. xdg_toplevel::close
      WaylandFinalizeHostTrampolineForGuestListener<>(callback);
    } else if (signature == "a") {
      // E.g. xdg_toplevel::wm_capabilities
      FEXCore::FinalizeHostTrampolineForGuestFunction(
          (FEXCore::HostToGuestTrampolinePtr*)callback,
          (void*)CallGuestPtrWithWaylandArray<void, void*, wl_proxy*>);
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
      FEXCore::FinalizeHostTrampolineForGuestFunction(
          (FEXCore::HostToGuestTrampolinePtr*)callback,
          (void*)CallGuestPtrWithWaylandArray<void, void*, wl_proxy*, int32_t, int32_t>);
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
      FEXCore::FinalizeHostTrampolineForGuestFunction(
          (FEXCore::HostToGuestTrampolinePtr*)callback,
          (void*)CallGuestPtrWithWaylandArray<void, void*, wl_proxy*, uint32_t, wl_surface*>);
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

// TODO: Is this needed?
//    MakeHostTrampolineForGuestFunctionAsyncCallable(registry_listener.global_remove, 1);
  }

  // Pass the original function pointer table to the host wayland library. This ensures the table is valid until the listener is unregistered.
  return fexldr_ptr_libwayland_client_wl_proxy_add_listener(proxy, callback2, (void*)(uintptr_t)data.data);
}

void* fexfn_impl_libwayland_client_wl_proxy_create_wrapper(guest_layout<void*> void_proxy) {
  guest_layout<wl_proxy*> proxy;
  memcpy(&proxy, &void_proxy, sizeof(void_proxy));
  return fexldr_ptr_libwayland_client_wl_proxy_create_wrapper(host_layout<wl_proxy*> { proxy }.data);
}

void fexfn_impl_libwayland_client_wl_proxy_wrapper_destroy(guest_layout<void*> void_proxy) {
  guest_layout<wl_proxy*> proxy;
  memcpy(&proxy, &void_proxy, sizeof(void_proxy));
  return fexldr_ptr_libwayland_client_wl_proxy_wrapper_destroy(host_layout<wl_proxy*> { proxy }.data);
}

wl_interface* fexfn_impl_libwayland_client_fex_wl_exchange_interface_pointer(wl_interface* guest_interface, guest_layout<char const*> name) {
  auto& host_interface = guest_to_host_interface[(void*)guest_interface];
  host_interface = reinterpret_cast<wl_interface*>(dlsym(fexldr_ptr_libwayland_client_so, (std::string { (const char*)name.get_pointer() } + "_interface").c_str()));
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
  // TODO: Do wl_interface symbols from host get allocated in 32-bit memory space? At least add an assert for this!
  memcpy(guest_interface, host_interface, 4); // name
  memcpy(reinterpret_cast<char*>(guest_interface) + 4, &host_interface->version, 4);
  memcpy(reinterpret_cast<char*>(guest_interface) + 8, &host_interface->method_count, 4);

  struct guest_wl_message {
    guest_layout<char*> name;
    guest_layout<char*> signature;
    guest_layout<wl_interface**> types;
  };

  // TODO: Manage lifetime of this memory?
  auto guest_methods = new guest_wl_message[host_interface->method_count];
  for (int i = 0; i < host_interface->method_count; ++i) {
    guest_methods[i].name.data = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(host_interface->methods[i].name));
    guest_methods[i].signature.data = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(host_interface->methods[i].signature));
    // TODO: Ugh, this will require more sophisticated fixups...
    guest_methods[i].types.data = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(host_interface->methods[i].types));
  }
  memcpy(reinterpret_cast<char*>(guest_interface) + 12, &guest_methods, 4);

  auto guest_events = new guest_wl_message[host_interface->event_count];
  for (int i = 0; i < host_interface->event_count; ++i) {
    guest_events[i].name.data = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(host_interface->events[i].name));
    guest_events[i].signature.data = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(host_interface->events[i].signature));
    // TODO: Ugh, this will require more sophisticated fixups...
    guest_events[i].types.data = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(host_interface->events[i].types));
  }
  memcpy(reinterpret_cast<char*>(guest_interface) + 16, &host_interface->event_count, sizeof(host_interface->event_count));
  memcpy(reinterpret_cast<char*>(guest_interface) + 20, &guest_events, 4);
#else
  memcpy(guest_interface, host_interface, sizeof(wl_interface));
#endif

// TODO: Disabled until we ensure the interface data is indeed stored in rodata
//  mprotect((void*)page_begin, 0x1000, PROT_READ);

  return host_interface;
}

static wl_interface* get_proxy_interface(wl_proxy* proxy) {
  // TODO: Use guest_layout/host_layout to do this dance instead
  uintptr_t interface_addr = reinterpret_cast<uintptr_t>(((wl_proxy_private*)proxy)->interface);
#ifdef IS_32BIT_THUNK
  // The pointer has size 4 Zero out upper 4 bytes that might leak in
  interface_addr &= 0xffffffff;
#endif
  return reinterpret_cast<wl_interface*>(interface_addr);
}

void fexfn_impl_libwayland_client_fex_wl_get_method_signature(wl_proxy* proxy, uint32_t opcode, char* out) {
  // TODO: Assert proxy comes from the host library
  strcpy(out, get_proxy_interface(proxy)->methods[opcode].signature);
}

int fexfn_impl_libwayland_client_fex_wl_get_interface_event_count(wl_proxy* proxy) {
  return get_proxy_interface(proxy)->event_count;
}

void fexfn_impl_libwayland_client_fex_wl_get_interface_event_name(wl_proxy* proxy, int i, char* out) {
  strcpy(out, get_proxy_interface(proxy)->events[i].name);
}

void fexfn_impl_libwayland_client_fex_wl_get_interface_event_signature(wl_proxy* proxy, int i, char* out) {
  strcpy(out, ((wl_proxy_private*)proxy)->interface->events[i].signature);
}

EXPORTS(libwayland_client)
