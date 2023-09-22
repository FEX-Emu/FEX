#include <common/GeneratorInterface.h>

#include <xcb/dri3.h>
#include <xcb/xcbext.h>

template<auto>
struct fex_gen_config {
    unsigned version = 0;
};

template<typename>
struct fex_gen_type {};

template<> struct fex_gen_type<xcb_connection_t> : fexgen::opaque_type {};

void FEX_xcb_dri3_init_extension(xcb_connection_t*, xcb_extension_t*);
size_t FEX_usable_size(void*);
void FEX_free_on_host(void*);

template<> struct fex_gen_config<FEX_xcb_dri3_init_extension> : fexgen::custom_host_impl {};
template<> struct fex_gen_config<FEX_usable_size> : fexgen::custom_host_impl, fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<FEX_free_on_host> : fexgen::custom_host_impl, fexgen::custom_guest_entrypoint {};

template<> struct fex_gen_config<xcb_dri3_query_version> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_dri3_query_version_unchecked> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_dri3_query_version_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_dri3_open> {};
template<> struct fex_gen_config<xcb_dri3_open_unchecked> {};
template<> struct fex_gen_config<xcb_dri3_open_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_dri3_open_reply_fds> {};
template<> struct fex_gen_config<xcb_dri3_pixmap_from_buffer_checked> {};
template<> struct fex_gen_config<xcb_dri3_pixmap_from_buffer> {};
template<> struct fex_gen_config<xcb_dri3_buffer_from_pixmap> {};
template<> struct fex_gen_config<xcb_dri3_buffer_from_pixmap_unchecked> {};
template<> struct fex_gen_config<xcb_dri3_buffer_from_pixmap_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_dri3_buffer_from_pixmap_reply_fds> {};
template<> struct fex_gen_config<xcb_dri3_fence_from_fd_checked> {};
template<> struct fex_gen_config<xcb_dri3_fence_from_fd> {};
template<> struct fex_gen_config<xcb_dri3_fd_from_fence> {};
template<> struct fex_gen_config<xcb_dri3_fd_from_fence_unchecked> {};
template<> struct fex_gen_config<xcb_dri3_fd_from_fence_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_dri3_fd_from_fence_reply_fds> {};
template<> struct fex_gen_config<xcb_dri3_get_supported_modifiers_sizeof> {};
template<> struct fex_gen_config<xcb_dri3_get_supported_modifiers> {};
template<> struct fex_gen_config<xcb_dri3_get_supported_modifiers_unchecked> {};
template<> struct fex_gen_config<xcb_dri3_get_supported_modifiers_window_modifiers> {};
template<> struct fex_gen_config<xcb_dri3_get_supported_modifiers_window_modifiers_length> {};
template<> struct fex_gen_config<xcb_dri3_get_supported_modifiers_window_modifiers_end> {};
template<> struct fex_gen_config<xcb_dri3_get_supported_modifiers_screen_modifiers> {};
template<> struct fex_gen_config<xcb_dri3_get_supported_modifiers_screen_modifiers_length> {};
template<> struct fex_gen_config<xcb_dri3_get_supported_modifiers_screen_modifiers_end> {};

template<> struct fex_gen_config<xcb_dri3_get_supported_modifiers_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_dri3_pixmap_from_buffers_checked> {};
template<> struct fex_gen_config<xcb_dri3_pixmap_from_buffers> {};
template<> struct fex_gen_config<xcb_dri3_buffers_from_pixmap_sizeof> {};
template<> struct fex_gen_config<xcb_dri3_buffers_from_pixmap> {};
template<> struct fex_gen_config<xcb_dri3_buffers_from_pixmap_unchecked> {};
template<> struct fex_gen_config<xcb_dri3_buffers_from_pixmap_strides> {};
template<> struct fex_gen_config<xcb_dri3_buffers_from_pixmap_strides_length> {};
template<> struct fex_gen_config<xcb_dri3_buffers_from_pixmap_strides_end> {};
template<> struct fex_gen_config<xcb_dri3_buffers_from_pixmap_offsets> {};
template<> struct fex_gen_config<xcb_dri3_buffers_from_pixmap_offsets_length> {};
template<> struct fex_gen_config<xcb_dri3_buffers_from_pixmap_offsets_end> {};
template<> struct fex_gen_config<xcb_dri3_buffers_from_pixmap_buffers> {};
template<> struct fex_gen_config<xcb_dri3_buffers_from_pixmap_buffers_length> {};
template<> struct fex_gen_config<xcb_dri3_buffers_from_pixmap_buffers_end> {};

template<> struct fex_gen_config<xcb_dri3_buffers_from_pixmap_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_dri3_buffers_from_pixmap_reply_fds> {};
