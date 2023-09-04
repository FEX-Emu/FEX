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
template<> struct fex_gen_type<wl_interface> : fexgen::opaque_type {};

template<> struct fex_gen_type<wl_event_queue> : fexgen::opaque_type {};

// Passed over Wayland's wire protocol for some functions
template<> struct fex_gen_type<wl_array> {};


template<> struct fex_gen_config<wl_proxy_destroy> {};

template<> struct fex_gen_config<wl_display_connect> {};
template<> struct fex_gen_config<wl_display_flush> {};
template<> struct fex_gen_config<wl_display_cancel_read> {};
template<> struct fex_gen_config<wl_display_create_queue> {};
template<> struct fex_gen_config<wl_display_disconnect> {};
template<> struct fex_gen_config<wl_display_dispatch> {};
template<> struct fex_gen_config<wl_display_dispatch_pending> {};
template<> struct fex_gen_config<wl_display_dispatch_queue> {};
template<> struct fex_gen_config<wl_display_dispatch_queue_pending> {};
template<> struct fex_gen_config<wl_display_get_error> {};
template<> struct fex_gen_config<wl_display_prepare_read> {};
template<> struct fex_gen_config<wl_display_prepare_read_queue> {};
template<> struct fex_gen_config<wl_display_read_events> {};
template<> struct fex_gen_config<wl_display_roundtrip> {};
template<> struct fex_gen_config<wl_display_roundtrip_queue> {};
template<> struct fex_gen_config<wl_display_connect_to_fd> {};
template<> struct fex_gen_config<wl_display_get_fd> {};

template<> struct fex_gen_config<wl_event_queue_destroy> {};

template<> struct fex_gen_config<wl_proxy_add_listener> : fexgen::custom_host_impl, fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_param<wl_proxy_add_listener, 1, void(**)()> : fexgen::ptr_passthrough {};
// TODO: Assume compatible
template<> struct fex_gen_param<wl_proxy_add_listener, 2, void*> : fexgen::ptr_passthrough {};
template<> struct fex_gen_config<wl_proxy_create> {};
template<> struct fex_gen_config<wl_proxy_create_wrapper> : fexgen::custom_host_impl {};
template<> struct fex_gen_param<wl_proxy_create_wrapper, 0, void*> : fexgen::ptr_passthrough {}; // Actually a wl_proxy* => convert manually
template<> struct fex_gen_config<wl_proxy_get_class> {};
template<> struct fex_gen_config<wl_proxy_get_id> {};
template<> struct fex_gen_config<wl_proxy_get_tag> {};
template<> struct fex_gen_config<wl_proxy_get_user_data> {};
template<> struct fex_gen_config<wl_proxy_set_tag> {};
template<> struct fex_gen_config<wl_proxy_get_version> {};
template<> struct fex_gen_config<wl_proxy_set_queue> {};
//template<> struct fex_gen_param<wl_proxy_set_queue, 1, wl_event_queue *> : fexgen::ptr_passthrough {};
template<> struct fex_gen_config<wl_proxy_set_tag> {};
// TODO: This has a void* parameter. Why does 32-bit accept this without annotations?
template<> struct fex_gen_config<wl_proxy_set_user_data> {};
template<> struct fex_gen_config<wl_proxy_wrapper_destroy> : fexgen::custom_host_impl {};
template<> struct fex_gen_param<wl_proxy_wrapper_destroy, 0, void*> : fexgen::ptr_passthrough {}; // Actually a wl_proxy* => convert manually

template<> struct fex_gen_config<wl_proxy_marshal_array> : fexgen::custom_host_impl {};
template<> struct fex_gen_param<wl_proxy_marshal_array, 2, wl_argument*> : fexgen::ptr_passthrough {};
template<> struct fex_gen_config<wl_proxy_marshal_array_constructor> : fexgen::custom_host_impl {};
template<> struct fex_gen_param<wl_proxy_marshal_array_constructor, 2, wl_argument*> : fexgen::ptr_passthrough {};
template<> struct fex_gen_config<wl_proxy_marshal_array_constructor_versioned> : fexgen::custom_host_impl {};
template<> struct fex_gen_param<wl_proxy_marshal_array_constructor_versioned, 2, wl_argument*> : fexgen::ptr_passthrough {};
template<> struct fex_gen_config<wl_proxy_marshal_array_flags> : fexgen::custom_host_impl {};
template<> struct fex_gen_param<wl_proxy_marshal_array_flags, 5, wl_argument*> : fexgen::ptr_passthrough {};

// Guest notifies host about its interface. Host returns its corresponding interface pointer
wl_interface* fex_wl_exchange_interface_pointer(wl_interface*, const char* name);
template<> struct fex_gen_config<fex_wl_exchange_interface_pointer> : fexgen::custom_host_impl/*, fexgen::custom_guest_entrypoint*/ {};
//template<> struct fex_gen_param<fex_wl_exchange_interface_pointer, 0, wl_interface*> : fexgen::ptr_passthrough {};
template<> struct fex_gen_param<fex_wl_exchange_interface_pointer, 1, const char*> : fexgen::ptr_passthrough {};

// This is equivalent to reading ((wl_proxy_private*)proxy)->interface->methods[opcode].signature on 64-bit.
// On 32-bit, the data layout differs between host and guest however, so we let the host extract the data.
void fex_wl_get_method_signature(wl_proxy*, uint32_t opcode, char*);
template<> struct fex_gen_config<fex_wl_get_method_signature> : fexgen::custom_host_impl {};

int fex_wl_get_interface_event_count(wl_proxy*);
template<> struct fex_gen_config<fex_wl_get_interface_event_count> : fexgen::custom_host_impl {};
void fex_wl_get_interface_event_name(wl_proxy*, int, char*);
template<> struct fex_gen_config<fex_wl_get_interface_event_name> : fexgen::custom_host_impl {};
void fex_wl_get_interface_event_signature(wl_proxy*, int, char*);
template<> struct fex_gen_config<fex_wl_get_interface_event_signature> : fexgen::custom_host_impl {};
