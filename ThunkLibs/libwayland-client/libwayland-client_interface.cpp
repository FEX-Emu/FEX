#include <common/GeneratorInterface.h>

#include <wayland-client.h>

template<auto>
struct fex_gen_config {
    unsigned version = 0;
};

template<typename>
struct fex_gen_type {};

// Function, parameter index, parameter type [optional]
template<auto, int, typename = void>
struct fex_gen_param {};

template<> struct fex_gen_type<wl_display> : fexgen::opaque_type {};
template<> struct fex_gen_type<wl_proxy> : fexgen::opaque_type {};

template<> struct fex_gen_type<wl_event_queue> : fexgen::opaque_type {};

// Passed over Wayland's wire protocol for some functions
template<> struct fex_gen_type<wl_array> : fexgen::emit_layout_wrappers {};

#ifdef IS_32BIT_THUNK
// wl_interface and wl_message reference each other through pointers
template<> struct fex_gen_type<wl_interface> : fexgen::emit_layout_wrappers {};
template<> struct fex_gen_config<&wl_interface::methods> : fexgen::custom_repack {};
template<> struct fex_gen_config<&wl_interface::events> : fexgen::custom_repack {};
template<> struct fex_gen_type<wl_message> : fexgen::emit_layout_wrappers {};
template<> struct fex_gen_config<&wl_message::types> : fexgen::custom_repack {};
#else
template<> struct fex_gen_type<wl_interface> : fexgen::assume_compatible_data_layout {};
#endif

template<> struct fex_gen_config<wl_proxy_destroy> : fexgen::custom_guest_entrypoint {};

template<> struct fex_gen_config<wl_display_connect> {};
template<> struct fex_gen_config<wl_display_flush> {};
template<> struct fex_gen_config<wl_display_cancel_read> {};
template<> struct fex_gen_config<wl_display_create_queue> {};
template<> struct fex_gen_config<wl_display_disconnect> {};
template<> struct fex_gen_config<wl_display_dispatch> {};
template<> struct fex_gen_config<wl_display_dispatch_pending> {};
template<> struct fex_gen_config<wl_display_dispatch_queue> {};
template<> struct fex_gen_config<wl_display_dispatch_queue_pending> {};
template<> struct fex_gen_config<wl_display_prepare_read> {};
template<> struct fex_gen_config<wl_display_prepare_read_queue> {};
template<> struct fex_gen_config<wl_display_read_events> {};
template<> struct fex_gen_config<wl_display_roundtrip> {};
template<> struct fex_gen_config<wl_display_roundtrip_queue> {};
template<> struct fex_gen_config<wl_display_get_fd> {};

template<> struct fex_gen_config<wl_event_queue_destroy> {};

template<> struct fex_gen_config<wl_proxy_add_listener> : fexgen::custom_host_impl, fexgen::custom_guest_entrypoint {};
// Callback table
template<> struct fex_gen_param<wl_proxy_add_listener, 1, void(**)()> : fexgen::ptr_passthrough {};
// User-provided data pointer (not used in caller-provided callback)
template<> struct fex_gen_param<wl_proxy_add_listener, 2, void*> : fexgen::assume_compatible_data_layout {};
template<> struct fex_gen_config<wl_proxy_create> : fexgen::custom_host_impl {};
template<> struct fex_gen_param<wl_proxy_create, 1, const wl_interface*> : fexgen::ptr_passthrough {};
template<> struct fex_gen_config<wl_proxy_create_wrapper> {};
template<> struct fex_gen_config<wl_proxy_get_listener> {};
template<> struct fex_gen_config<wl_proxy_get_tag> {};
template<> struct fex_gen_config<wl_proxy_get_user_data> {};
template<> struct fex_gen_config<wl_proxy_get_version> {};
template<> struct fex_gen_config<wl_proxy_set_queue> {};
template<> struct fex_gen_config<wl_proxy_set_tag> {};
// TODO: This has a void* parameter. Why does 32-bit accept this without annotations?
template<> struct fex_gen_config<wl_proxy_set_user_data> {};
template<> struct fex_gen_config<wl_proxy_wrapper_destroy> {};

template<> struct fex_gen_config<wl_proxy_marshal_array> : fexgen::custom_host_impl {};
template<> struct fex_gen_param<wl_proxy_marshal_array, 2, wl_argument*> : fexgen::ptr_passthrough {};
// wl_proxy_marshal_array_flags is only available starting from Wayland 1.19.91
#if WAYLAND_VERSION_MAJOR * 10000 + WAYLAND_VERSION_MINOR * 100 + WAYLAND_VERSION_MICRO >= 11991
template<> struct fex_gen_config<wl_proxy_marshal_array_flags> : fexgen::custom_host_impl {};
template<> struct fex_gen_param<wl_proxy_marshal_array_flags, 2, const wl_interface*> : fexgen::ptr_passthrough {};
template<> struct fex_gen_param<wl_proxy_marshal_array_flags, 5, wl_argument*> : fexgen::ptr_passthrough {};
#endif

// Guest notifies host about its interface. Host returns its corresponding interface pointer
void fex_wl_exchange_interface_pointer(wl_interface*, const char* name);
template<> struct fex_gen_config<fex_wl_exchange_interface_pointer> : fexgen::custom_host_impl {};
template<> struct fex_gen_param<fex_wl_exchange_interface_pointer, 0, wl_interface*> : fexgen::ptr_passthrough {};

// This is equivalent to reading proxy->interface->methods[opcode].signature on 64-bit.
// On 32-bit, the data layout differs between host and guest however, so we let the host extract the data.
void fex_wl_get_method_signature(wl_proxy*, uint32_t opcode, char*);
template<> struct fex_gen_config<fex_wl_get_method_signature> : fexgen::custom_host_impl {};
int fex_wl_get_interface_event_count(wl_proxy*);
template<> struct fex_gen_config<fex_wl_get_interface_event_count> : fexgen::custom_host_impl {};
void fex_wl_get_interface_event_name(wl_proxy*, int, char*);
template<> struct fex_gen_config<fex_wl_get_interface_event_name> : fexgen::custom_host_impl {};
void fex_wl_get_interface_event_signature(wl_proxy*, int, char*);
template<> struct fex_gen_config<fex_wl_get_interface_event_signature> : fexgen::custom_host_impl {};
