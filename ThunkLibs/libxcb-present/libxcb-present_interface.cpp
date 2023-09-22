#include <common/GeneratorInterface.h>

#include <xcb/present.h>
#include <xcb/xcbext.h>

template<auto>
struct fex_gen_config {
    unsigned version = 0;
};

template<typename>
struct fex_gen_type {};

template<> struct fex_gen_type<xcb_connection_t> : fexgen::opaque_type {};

void FEX_xcb_present_init_extension(xcb_connection_t*, xcb_extension_t*);
size_t FEX_usable_size(void*);
void FEX_free_on_host(void*);

template<> struct fex_gen_config<FEX_xcb_present_init_extension> : fexgen::custom_host_impl {};
template<> struct fex_gen_config<FEX_usable_size> : fexgen::custom_host_impl, fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<FEX_free_on_host> : fexgen::custom_host_impl, fexgen::custom_guest_entrypoint {};

template<> struct fex_gen_config<xcb_present_notify_next> {};
template<> struct fex_gen_config<xcb_present_notify_end> {};
template<> struct fex_gen_config<xcb_present_query_version> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_present_query_version_unchecked> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_present_query_version_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_present_pixmap_sizeof> {};
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
template<> struct fex_gen_config<xcb_present_redirect_notify_sizeof> {};
template<> struct fex_gen_config<xcb_present_redirect_notify_notifies> {};
template<> struct fex_gen_config<xcb_present_redirect_notify_notifies_length> {};
template<> struct fex_gen_config<xcb_present_redirect_notify_notifies_iterator> {};
