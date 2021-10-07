#!/usr/bin/python3
from ThunkHelpers import *

lib_with_filename("libxcb_dri3", "libxcb-dri3")

# FEX
fn("void FEX_xcb_dri3_init_extension(xcb_connection_t *, xcb_extension_t *)"); no_unpack()
fn("size_t FEX_usable_size(void*)"); no_unpack()
fn("void FEX_free_on_host(void*)"); no_unpack()

fn("xcb_dri3_query_version_cookie_t xcb_dri3_query_version(xcb_connection_t *, uint32_t, uint32_t)"); no_pack()
fn("xcb_dri3_query_version_cookie_t xcb_dri3_query_version_unchecked(xcb_connection_t *, uint32_t, uint32_t)"); no_pack()
fn("xcb_dri3_query_version_reply_t * xcb_dri3_query_version_reply(xcb_connection_t *, xcb_dri3_query_version_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("xcb_dri3_open_cookie_t xcb_dri3_open(xcb_connection_t *, xcb_drawable_t, uint32_t)")
fn("xcb_dri3_open_cookie_t xcb_dri3_open_unchecked(xcb_connection_t *, xcb_drawable_t, uint32_t)")
fn("xcb_dri3_open_reply_t * xcb_dri3_open_reply(xcb_connection_t *, xcb_dri3_open_cookie_t, xcb_generic_error_t **)"); no_pack()
# XXX: Who owns result?
fn("int * xcb_dri3_open_reply_fds(xcb_connection_t *, xcb_dri3_open_reply_t *)")
fn("xcb_void_cookie_t xcb_dri3_pixmap_from_buffer_checked(xcb_connection_t *, xcb_pixmap_t, xcb_drawable_t, uint32_t, uint16_t, uint16_t, uint16_t, uint8_t, uint8_t, int32_t)")
fn("xcb_void_cookie_t xcb_dri3_pixmap_from_buffer(xcb_connection_t *, xcb_pixmap_t, xcb_drawable_t, uint32_t, uint16_t, uint16_t, uint16_t, uint8_t, uint8_t, int32_t)")
fn("xcb_dri3_buffer_from_pixmap_cookie_t xcb_dri3_buffer_from_pixmap(xcb_connection_t *, xcb_pixmap_t)")
fn("xcb_dri3_buffer_from_pixmap_cookie_t xcb_dri3_buffer_from_pixmap_unchecked(xcb_connection_t *, xcb_pixmap_t)")
fn("xcb_dri3_buffer_from_pixmap_reply_t * xcb_dri3_buffer_from_pixmap_reply(xcb_connection_t *, xcb_dri3_buffer_from_pixmap_cookie_t, xcb_generic_error_t **)") ; no_pack()
# XXX: Who owns result?
fn("int * xcb_dri3_buffer_from_pixmap_reply_fds(xcb_connection_t *, xcb_dri3_buffer_from_pixmap_reply_t *)")
fn("xcb_void_cookie_t xcb_dri3_fence_from_fd_checked(xcb_connection_t *, xcb_drawable_t, uint32_t, uint8_t, int32_t)")
fn("xcb_void_cookie_t xcb_dri3_fence_from_fd(xcb_connection_t *, xcb_drawable_t, uint32_t, uint8_t, int32_t)")
fn("xcb_dri3_fd_from_fence_cookie_t xcb_dri3_fd_from_fence(xcb_connection_t *, xcb_drawable_t, uint32_t)")
fn("xcb_dri3_fd_from_fence_cookie_t xcb_dri3_fd_from_fence_unchecked(xcb_connection_t *, xcb_drawable_t, uint32_t)")
fn("xcb_dri3_fd_from_fence_reply_t * xcb_dri3_fd_from_fence_reply(xcb_connection_t *, xcb_dri3_fd_from_fence_cookie_t, xcb_generic_error_t **)"); no_pack()
# XXX: Who owns result?
fn("int * xcb_dri3_fd_from_fence_reply_fds(xcb_connection_t *, xcb_dri3_fd_from_fence_reply_t *)")
fn("int xcb_dri3_get_supported_modifiers_sizeof(const void *)")
fn("xcb_dri3_get_supported_modifiers_cookie_t xcb_dri3_get_supported_modifiers(xcb_connection_t *, uint32_t, uint8_t, uint8_t)")
fn("xcb_dri3_get_supported_modifiers_cookie_t xcb_dri3_get_supported_modifiers_unchecked(xcb_connection_t *, uint32_t, uint8_t, uint8_t)")
# ::Iterator::
fn("uint64_t * xcb_dri3_get_supported_modifiers_window_modifiers(const xcb_dri3_get_supported_modifiers_reply_t *)")
fn("int xcb_dri3_get_supported_modifiers_window_modifiers_length(const xcb_dri3_get_supported_modifiers_reply_t *)")
fn("xcb_generic_iterator_t xcb_dri3_get_supported_modifiers_window_modifiers_end(const xcb_dri3_get_supported_modifiers_reply_t *)")
# ::Iterator::
fn("uint64_t * xcb_dri3_get_supported_modifiers_screen_modifiers(const xcb_dri3_get_supported_modifiers_reply_t *)")
fn("int xcb_dri3_get_supported_modifiers_screen_modifiers_length(const xcb_dri3_get_supported_modifiers_reply_t *)")
fn("xcb_generic_iterator_t xcb_dri3_get_supported_modifiers_screen_modifiers_end(const xcb_dri3_get_supported_modifiers_reply_t *)")

fn("xcb_dri3_get_supported_modifiers_reply_t * xcb_dri3_get_supported_modifiers_reply(xcb_connection_t *, xcb_dri3_get_supported_modifiers_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("xcb_void_cookie_t xcb_dri3_pixmap_from_buffers_checked(xcb_connection_t *, xcb_pixmap_t, xcb_window_t, uint8_t, uint16_t, uint16_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint8_t, uint8_t, uint64_t, const int32_t *)")
fn("xcb_void_cookie_t xcb_dri3_pixmap_from_buffers(xcb_connection_t *, xcb_pixmap_t, xcb_window_t, uint8_t, uint16_t, uint16_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint8_t, uint8_t, uint64_t, const int32_t *)")
fn("int xcb_dri3_buffers_from_pixmap_sizeof(const void *, int32_t)")
fn("xcb_dri3_buffers_from_pixmap_cookie_t xcb_dri3_buffers_from_pixmap(xcb_connection_t *, xcb_pixmap_t)")
fn("xcb_dri3_buffers_from_pixmap_cookie_t xcb_dri3_buffers_from_pixmap_unchecked(xcb_connection_t *, xcb_pixmap_t)")
# ::Iterator::
fn("uint32_t * xcb_dri3_buffers_from_pixmap_strides(const xcb_dri3_buffers_from_pixmap_reply_t *)")
fn("int xcb_dri3_buffers_from_pixmap_strides_length(const xcb_dri3_buffers_from_pixmap_reply_t *)")
fn("xcb_generic_iterator_t xcb_dri3_buffers_from_pixmap_strides_end(const xcb_dri3_buffers_from_pixmap_reply_t *)")
# ::Iterator::
fn("uint32_t * xcb_dri3_buffers_from_pixmap_offsets(const xcb_dri3_buffers_from_pixmap_reply_t *)")
fn("int xcb_dri3_buffers_from_pixmap_offsets_length(const xcb_dri3_buffers_from_pixmap_reply_t *)")
fn("xcb_generic_iterator_t xcb_dri3_buffers_from_pixmap_offsets_end(const xcb_dri3_buffers_from_pixmap_reply_t *)")
# ::Iterator::
fn("int32_t * xcb_dri3_buffers_from_pixmap_buffers(const xcb_dri3_buffers_from_pixmap_reply_t *)")
fn("int xcb_dri3_buffers_from_pixmap_buffers_length(const xcb_dri3_buffers_from_pixmap_reply_t *)")
fn("xcb_generic_iterator_t xcb_dri3_buffers_from_pixmap_buffers_end(const xcb_dri3_buffers_from_pixmap_reply_t *)")

fn("xcb_dri3_buffers_from_pixmap_reply_t * xcb_dri3_buffers_from_pixmap_reply(xcb_connection_t *, xcb_dri3_buffers_from_pixmap_cookie_t, xcb_generic_error_t **)"); no_pack()
# XXX: Who owns result?
fn("int * xcb_dri3_buffers_from_pixmap_reply_fds(xcb_connection_t *, xcb_dri3_buffers_from_pixmap_reply_t *)")

Generate()
