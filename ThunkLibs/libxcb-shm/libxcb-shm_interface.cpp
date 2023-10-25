#include <common/GeneratorInterface.h>

#include <xcb/shm.h>
#include <xcb/xcbext.h>

template<auto>
struct fex_gen_config {
    unsigned version = 0;
};

template<typename>
struct fex_gen_type {};

template<> struct fex_gen_type<xcb_connection_t> : fexgen::opaque_type {};

void FEX_xcb_shm_init_extension(xcb_connection_t*, xcb_extension_t*);
size_t FEX_usable_size(void*);
void FEX_free_on_host(void*);

template<> struct fex_gen_config<FEX_xcb_shm_init_extension> : fexgen::custom_host_impl {};
template<> struct fex_gen_config<FEX_usable_size> : fexgen::custom_host_impl, fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<FEX_free_on_host> : fexgen::custom_host_impl, fexgen::custom_guest_entrypoint {};

template<> struct fex_gen_config<xcb_shm_seg_next> {};
template<> struct fex_gen_config<xcb_shm_seg_end> {};
template<> struct fex_gen_config<xcb_shm_query_version> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_shm_query_version_unchecked> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_shm_query_version_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_shm_attach_checked> {};
template<> struct fex_gen_config<xcb_shm_attach> {};
template<> struct fex_gen_config<xcb_shm_detach_checked> {};
template<> struct fex_gen_config<xcb_shm_detach> {};
template<> struct fex_gen_config<xcb_shm_put_image_checked> {};
template<> struct fex_gen_config<xcb_shm_put_image> {};
template<> struct fex_gen_config<xcb_shm_get_image> {};
template<> struct fex_gen_config<xcb_shm_get_image_unchecked> {};
template<> struct fex_gen_config<xcb_shm_get_image_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_shm_create_pixmap_checked> {};
template<> struct fex_gen_config<xcb_shm_create_pixmap> {};
template<> struct fex_gen_config<xcb_shm_attach_fd_checked> {};
template<> struct fex_gen_config<xcb_shm_attach_fd> {};
template<> struct fex_gen_config<xcb_shm_create_segment> {};
template<> struct fex_gen_config<xcb_shm_create_segment_unchecked> {};
template<> struct fex_gen_config<xcb_shm_create_segment_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_shm_create_segment_reply_fds> {};
