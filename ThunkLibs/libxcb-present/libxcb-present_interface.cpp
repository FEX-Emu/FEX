#include <common/GeneratorInterface.h>

#include <xcb/present.h>

template<auto>
struct fex_gen_config;

void FEX_xcb_present_init_extension(xcb_connection_t*, xcb_extension_t*);
size_t FEX_usable_size(void*);
void FEX_free_on_host(void*);

template<> struct fex_gen_type<xcb_connection_t> : fexgen::opaque_to_guest {};
template<> struct fex_gen_type<xcb_extension_t> : fexgen::opaque_to_guest {};

template<> struct fex_gen_config<FEX_xcb_present_init_extension> : fexgen::custom_host_impl {};
template<> struct fex_gen_config<FEX_usable_size> : fexgen::custom_host_impl, fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_param<FEX_usable_size, 0, void*> : fexgen::ptr_is_untyped_address {};
template<> struct fex_gen_config<FEX_free_on_host> : fexgen::custom_host_impl, fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_param<FEX_free_on_host, 0, void*> : fexgen::ptr_is_untyped_address {};

template<> struct fex_gen_config<xcb_present_notify_next> {};
template<> struct fex_gen_config<xcb_present_notify_end> {};
template<> struct fex_gen_config<xcb_present_query_version> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_present_query_version_unchecked> : fexgen::custom_guest_entrypoint {};

template<> struct fex_gen_config<xcb_present_query_version_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_param<xcb_present_query_version_reply, 2, xcb_generic_error_t**> : fexgen::ptr_todo_only64 {};

template<> struct fex_gen_config<xcb_present_pixmap_sizeof> {};
template<> struct fex_gen_param<xcb_present_pixmap_sizeof, 0, const void*> : fexgen::ptr_todo_only64 {};

template<> struct fex_gen_config<xcb_present_pixmap_checked> {};
template<> struct fex_gen_config<xcb_present_pixmap> {};
template<> struct fex_gen_config<xcb_present_pixmap_notifies> {};
template<> struct fex_gen_config<xcb_present_pixmap_notifies_length> {};
template<> struct fex_gen_config<xcb_present_pixmap_notifies_iterator> {};

template<> struct fex_gen_config<xcb_present_notify_msc_checked> {};
template<> struct fex_gen_config<xcb_present_notify_msc> {};
template<> struct fex_gen_config<xcb_present_event_next> {};
template<> struct fex_gen_config<xcb_present_event_end> {};
template<> struct fex_gen_config<xcb_present_select_input_checked> {};
template<> struct fex_gen_config<xcb_present_select_input> {};
template<> struct fex_gen_config<xcb_present_query_capabilities> {};
template<> struct fex_gen_config<xcb_present_query_capabilities_unchecked> {};

template<> struct fex_gen_config<xcb_present_query_capabilities_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_param<xcb_present_query_capabilities_reply, 2, xcb_generic_error_t**> : fexgen::ptr_todo_only64 {};

// TODO: _sizeof functions compute the size of serialized objects, so this parameter can be treated as an opaque object of unspecified type
template<> struct fex_gen_config<xcb_present_redirect_notify_sizeof> {};
template<> struct fex_gen_param<xcb_present_redirect_notify_sizeof, 0, const void*> : fexgen::ptr_todo_only64 {};

template<> struct fex_gen_config<xcb_present_redirect_notify_notifies> {};
template<> struct fex_gen_config<xcb_present_redirect_notify_notifies_length> {};
template<> struct fex_gen_config<xcb_present_redirect_notify_notifies_iterator> {};
