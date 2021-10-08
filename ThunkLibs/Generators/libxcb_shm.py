#!/usr/bin/python3
from ThunkHelpers import *

lib_with_filename("libxcb_shm", "0", "libxcb-shm")

# FEX
fn("void FEX_xcb_shm_init_extension(xcb_connection_t *, xcb_extension_t *)"); no_unpack()
fn("size_t FEX_usable_size(void*)"); no_unpack()
fn("void FEX_free_on_host(void*)"); no_unpack()

fn("void xcb_shm_seg_next(xcb_shm_seg_iterator_t *)")
fn("xcb_generic_iterator_t xcb_shm_seg_end(xcb_shm_seg_iterator_t)")
fn("xcb_shm_query_version_cookie_t xcb_shm_query_version(xcb_connection_t *)"); no_pack()
fn("xcb_shm_query_version_cookie_t xcb_shm_query_version_unchecked(xcb_connection_t *)"); no_pack()
fn("xcb_shm_query_version_reply_t * xcb_shm_query_version_reply(xcb_connection_t *, xcb_shm_query_version_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("xcb_void_cookie_t xcb_shm_attach_checked(xcb_connection_t *, xcb_shm_seg_t, uint32_t, uint8_t)")
fn("xcb_void_cookie_t xcb_shm_attach(xcb_connection_t *, xcb_shm_seg_t, uint32_t, uint8_t)")
fn("xcb_void_cookie_t xcb_shm_detach_checked(xcb_connection_t *, xcb_shm_seg_t)")
fn("xcb_void_cookie_t xcb_shm_detach(xcb_connection_t *, xcb_shm_seg_t)")
fn("xcb_void_cookie_t xcb_shm_put_image_checked(xcb_connection_t *, xcb_drawable_t, xcb_gcontext_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, int16_t, int16_t, uint8_t, uint8_t, uint8_t, xcb_shm_seg_t, uint32_t)")
fn("xcb_void_cookie_t xcb_shm_put_image(xcb_connection_t *, xcb_drawable_t, xcb_gcontext_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t, int16_t, int16_t, uint8_t, uint8_t, uint8_t, xcb_shm_seg_t, uint32_t)")
fn("xcb_shm_get_image_cookie_t xcb_shm_get_image(xcb_connection_t *, xcb_drawable_t, int16_t, int16_t, uint16_t, uint16_t, uint32_t, uint8_t, xcb_shm_seg_t, uint32_t)")
fn("xcb_shm_get_image_cookie_t xcb_shm_get_image_unchecked(xcb_connection_t *, xcb_drawable_t, int16_t, int16_t, uint16_t, uint16_t, uint32_t, uint8_t, xcb_shm_seg_t, uint32_t)")
fn("xcb_shm_get_image_reply_t * xcb_shm_get_image_reply(xcb_connection_t *, xcb_shm_get_image_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("xcb_void_cookie_t xcb_shm_create_pixmap_checked(xcb_connection_t *, xcb_pixmap_t, xcb_drawable_t, uint16_t, uint16_t, uint8_t, xcb_shm_seg_t, uint32_t)")
fn("xcb_void_cookie_t xcb_shm_create_pixmap(xcb_connection_t *, xcb_pixmap_t, xcb_drawable_t, uint16_t, uint16_t, uint8_t, xcb_shm_seg_t, uint32_t)")
fn("xcb_void_cookie_t xcb_shm_attach_fd_checked(xcb_connection_t *, xcb_shm_seg_t, int32_t, uint8_t)")
fn("xcb_void_cookie_t xcb_shm_attach_fd(xcb_connection_t *, xcb_shm_seg_t, int32_t, uint8_t)")
fn("xcb_shm_create_segment_cookie_t xcb_shm_create_segment(xcb_connection_t *, xcb_shm_seg_t, uint32_t, uint8_t)")
fn("xcb_shm_create_segment_cookie_t xcb_shm_create_segment_unchecked(xcb_connection_t *, xcb_shm_seg_t, uint32_t, uint8_t)")
fn("xcb_shm_create_segment_reply_t * xcb_shm_create_segment_reply(xcb_connection_t *, xcb_shm_create_segment_cookie_t, xcb_generic_error_t **)"); no_pack()
# XXX: Who owns result?
fn("int * xcb_shm_create_segment_reply_fds(xcb_connection_t *, xcb_shm_create_segment_reply_t *)")
Generate()
