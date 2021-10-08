#!/usr/bin/python3
from ThunkHelpers import *

lib_with_filename("libxcb_present", "0", "libxcb-present")
# FEX
fn("void FEX_xcb_present_init_extension(xcb_connection_t *, xcb_extension_t *)"); no_unpack()
fn("size_t FEX_usable_size(void*)"); no_unpack()
fn("void FEX_free_on_host(void*)"); no_unpack()

fn("void xcb_present_notify_next(xcb_present_notify_iterator_t *)")
fn("xcb_generic_iterator_t xcb_present_notify_end(xcb_present_notify_iterator_t)")
fn("xcb_present_query_version_cookie_t xcb_present_query_version(xcb_connection_t *, uint32_t, uint32_t)"); no_pack()
fn("xcb_present_query_version_cookie_t xcb_present_query_version_unchecked(xcb_connection_t *, uint32_t, uint32_t)"); no_pack()
fn("xcb_present_query_version_reply_t * xcb_present_query_version_reply(xcb_connection_t *, xcb_present_query_version_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("int xcb_present_pixmap_sizeof(const void *, uint32_t)")
fn("xcb_void_cookie_t xcb_present_pixmap_checked(xcb_connection_t *, xcb_window_t, xcb_pixmap_t, uint32_t, xcb_xfixes_region_t, xcb_xfixes_region_t, int16_t, int16_t, xcb_randr_crtc_t, xcb_sync_fence_t, xcb_sync_fence_t, uint32_t, uint64_t, uint64_t, uint64_t, uint32_t, const xcb_present_notify_t *)")
fn("xcb_void_cookie_t xcb_present_pixmap(xcb_connection_t *, xcb_window_t, xcb_pixmap_t, uint32_t, xcb_xfixes_region_t, xcb_xfixes_region_t, int16_t, int16_t, xcb_randr_crtc_t, xcb_sync_fence_t, xcb_sync_fence_t, uint32_t, uint64_t, uint64_t, uint64_t, uint32_t, const xcb_present_notify_t *)")
# ::Iterator::
fn("xcb_present_notify_t * xcb_present_pixmap_notifies(const xcb_present_pixmap_request_t *)")
fn("int xcb_present_pixmap_notifies_length(const xcb_present_pixmap_request_t *)")
fn("xcb_present_notify_iterator_t xcb_present_pixmap_notifies_iterator(const xcb_present_pixmap_request_t *)")

fn("xcb_void_cookie_t xcb_present_notify_msc_checked(xcb_connection_t *, xcb_window_t, uint32_t, uint64_t, uint64_t, uint64_t)")
fn("xcb_void_cookie_t xcb_present_notify_msc(xcb_connection_t *, xcb_window_t, uint32_t, uint64_t, uint64_t, uint64_t)")
fn("void xcb_present_event_next(xcb_present_event_iterator_t *)")
fn("xcb_generic_iterator_t xcb_present_event_end(xcb_present_event_iterator_t)")
fn("xcb_void_cookie_t xcb_present_select_input_checked(xcb_connection_t *, xcb_present_event_t, xcb_window_t, uint32_t)")
fn("xcb_void_cookie_t xcb_present_select_input(xcb_connection_t *, xcb_present_event_t, xcb_window_t, uint32_t)")
fn("xcb_present_query_capabilities_cookie_t xcb_present_query_capabilities(xcb_connection_t *, uint32_t)")
fn("xcb_present_query_capabilities_cookie_t xcb_present_query_capabilities_unchecked(xcb_connection_t *, uint32_t)")
fn("xcb_present_query_capabilities_reply_t * xcb_present_query_capabilities_reply(xcb_connection_t *, xcb_present_query_capabilities_cookie_t, xcb_generic_error_t **)"); no_pack();
fn("int xcb_present_redirect_notify_sizeof(const void *, uint32_t)")
# ::Iterator::
fn("xcb_present_notify_t * xcb_present_redirect_notify_notifies(const xcb_present_redirect_notify_event_t *)")
fn("int xcb_present_redirect_notify_notifies_length(const xcb_present_redirect_notify_event_t *)")
fn("xcb_present_notify_iterator_t xcb_present_redirect_notify_notifies_iterator(const xcb_present_redirect_notify_event_t *)")

Generate()
