#!/usr/bin/python3
from ThunkHelpers import *

lib_with_filename("libxcb_glx", "0", "libxcb-glx")
# FEX
fn("void FEX_xcb_glx_init_extension(xcb_connection_t *, xcb_extension_t *)"); no_unpack()
fn("size_t FEX_usable_size(void*)"); no_unpack()
fn("void FEX_free_on_host(void*)"); no_unpack()

fn("void xcb_glx_pixmap_next(xcb_glx_pixmap_iterator_t *)")
fn("xcb_generic_iterator_t xcb_glx_pixmap_end(xcb_glx_pixmap_iterator_t)")
fn("void xcb_glx_context_next(xcb_glx_context_iterator_t *)")
fn("xcb_generic_iterator_t xcb_glx_context_end(xcb_glx_context_iterator_t)")
fn("void xcb_glx_pbuffer_next(xcb_glx_pbuffer_iterator_t *)")
fn("xcb_generic_iterator_t xcb_glx_pbuffer_end(xcb_glx_pbuffer_iterator_t)")
fn("void xcb_glx_window_next(xcb_glx_window_iterator_t *)")
fn("xcb_generic_iterator_t xcb_glx_window_end(xcb_glx_window_iterator_t)")
fn("void xcb_glx_fbconfig_next(xcb_glx_fbconfig_iterator_t *)")
fn("xcb_generic_iterator_t xcb_glx_fbconfig_end(xcb_glx_fbconfig_iterator_t)")
fn("void xcb_glx_drawable_next(xcb_glx_drawable_iterator_t *)")
fn("xcb_generic_iterator_t xcb_glx_drawable_end(xcb_glx_drawable_iterator_t)")
fn("void xcb_glx_float32_next(xcb_glx_float32_iterator_t *)")
fn("xcb_generic_iterator_t xcb_glx_float32_end(xcb_glx_float32_iterator_t)")
fn("void xcb_glx_float64_next(xcb_glx_float64_iterator_t *)")
fn("xcb_generic_iterator_t xcb_glx_float64_end(xcb_glx_float64_iterator_t)")
fn("void xcb_glx_bool32_next(xcb_glx_bool32_iterator_t *)")
fn("xcb_generic_iterator_t xcb_glx_bool32_end(xcb_glx_bool32_iterator_t)")
fn("void xcb_glx_context_tag_next(xcb_glx_context_tag_iterator_t *)")
fn("xcb_generic_iterator_t xcb_glx_context_tag_end(xcb_glx_context_tag_iterator_t)")
fn("int xcb_glx_render_sizeof(const void *, uint32_t)")
fn("xcb_void_cookie_t xcb_glx_render_checked(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, const uint8_t *)")
fn("xcb_void_cookie_t xcb_glx_render(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, const uint8_t *)")
# ::Iterator::
fn("uint8_t * xcb_glx_render_data(const xcb_glx_render_request_t *)")
fn("int xcb_glx_render_data_length(const xcb_glx_render_request_t *)")
fn("xcb_generic_iterator_t xcb_glx_render_data_end(const xcb_glx_render_request_t *)")

fn("int xcb_glx_render_large_sizeof(const void *)")
fn("xcb_void_cookie_t xcb_glx_render_large_checked(xcb_connection_t *, xcb_glx_context_tag_t, uint16_t, uint16_t, uint32_t, const uint8_t *)")
fn("xcb_void_cookie_t xcb_glx_render_large(xcb_connection_t *, xcb_glx_context_tag_t, uint16_t, uint16_t, uint32_t, const uint8_t *)")
fn("uint8_t * xcb_glx_render_large_data(const xcb_glx_render_large_request_t *)")
fn("int xcb_glx_render_large_data_length(const xcb_glx_render_large_request_t *)")
fn("xcb_generic_iterator_t xcb_glx_render_large_data_end(const xcb_glx_render_large_request_t *)")
fn("xcb_void_cookie_t xcb_glx_create_context_checked(xcb_connection_t *, xcb_glx_context_t, xcb_visualid_t, uint32_t, xcb_glx_context_t, uint8_t)"); no_pack()
fn("xcb_void_cookie_t xcb_glx_create_context(xcb_connection_t *, xcb_glx_context_t, xcb_visualid_t, uint32_t, xcb_glx_context_t, uint8_t)"); no_pack()
fn("xcb_void_cookie_t xcb_glx_destroy_context_checked(xcb_connection_t *, xcb_glx_context_t)")
fn("xcb_void_cookie_t xcb_glx_destroy_context(xcb_connection_t *, xcb_glx_context_t)")
fn("xcb_glx_make_current_cookie_t xcb_glx_make_current(xcb_connection_t *, xcb_glx_drawable_t, xcb_glx_context_t, xcb_glx_context_tag_t)")
fn("xcb_glx_make_current_cookie_t xcb_glx_make_current_unchecked(xcb_connection_t *, xcb_glx_drawable_t, xcb_glx_context_t, xcb_glx_context_tag_t)")
fn("xcb_glx_make_current_reply_t * xcb_glx_make_current_reply(xcb_connection_t *, xcb_glx_make_current_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("xcb_glx_is_direct_cookie_t xcb_glx_is_direct(xcb_connection_t *, xcb_glx_context_t)")
fn("xcb_glx_is_direct_cookie_t xcb_glx_is_direct_unchecked(xcb_connection_t *, xcb_glx_context_t)")
fn("xcb_glx_is_direct_reply_t * xcb_glx_is_direct_reply(xcb_connection_t *, xcb_glx_is_direct_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("xcb_glx_query_version_cookie_t xcb_glx_query_version(xcb_connection_t *, uint32_t, uint32_t)")
fn("xcb_glx_query_version_cookie_t xcb_glx_query_version_unchecked(xcb_connection_t *, uint32_t, uint32_t)")
fn("xcb_glx_query_version_reply_t * xcb_glx_query_version_reply(xcb_connection_t *, xcb_glx_query_version_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("xcb_void_cookie_t xcb_glx_wait_gl_checked(xcb_connection_t *, xcb_glx_context_tag_t)")
fn("xcb_void_cookie_t xcb_glx_wait_gl(xcb_connection_t *, xcb_glx_context_tag_t)")
fn("xcb_void_cookie_t xcb_glx_wait_x_checked(xcb_connection_t *, xcb_glx_context_tag_t)")
fn("xcb_void_cookie_t xcb_glx_wait_x(xcb_connection_t *, xcb_glx_context_tag_t)")
fn("xcb_void_cookie_t xcb_glx_copy_context_checked(xcb_connection_t *, xcb_glx_context_t, xcb_glx_context_t, uint32_t, xcb_glx_context_tag_t)")
fn("xcb_void_cookie_t xcb_glx_copy_context(xcb_connection_t *, xcb_glx_context_t, xcb_glx_context_t, uint32_t, xcb_glx_context_tag_t)")
fn("xcb_void_cookie_t xcb_glx_swap_buffers_checked(xcb_connection_t *, xcb_glx_context_tag_t, xcb_glx_drawable_t)")
fn("xcb_void_cookie_t xcb_glx_swap_buffers(xcb_connection_t *, xcb_glx_context_tag_t, xcb_glx_drawable_t)")
fn("xcb_void_cookie_t xcb_glx_use_x_font_checked(xcb_connection_t *, xcb_glx_context_tag_t, xcb_font_t, uint32_t, uint32_t, uint32_t)")
fn("xcb_void_cookie_t xcb_glx_use_x_font(xcb_connection_t *, xcb_glx_context_tag_t, xcb_font_t, uint32_t, uint32_t, uint32_t)")
fn("xcb_void_cookie_t xcb_glx_create_glx_pixmap_checked(xcb_connection_t *, uint32_t, xcb_visualid_t, xcb_pixmap_t, xcb_glx_pixmap_t)")
fn("xcb_void_cookie_t xcb_glx_create_glx_pixmap(xcb_connection_t *, uint32_t, xcb_visualid_t, xcb_pixmap_t, xcb_glx_pixmap_t)")
fn("int xcb_glx_get_visual_configs_sizeof(const void *)")
fn("xcb_glx_get_visual_configs_cookie_t xcb_glx_get_visual_configs(xcb_connection_t *, uint32_t)")
fn("xcb_glx_get_visual_configs_cookie_t xcb_glx_get_visual_configs_unchecked(xcb_connection_t *, uint32_t)")
# ::Iterator::
fn("uint32_t * xcb_glx_get_visual_configs_property_list(const xcb_glx_get_visual_configs_reply_t *)")
fn("int xcb_glx_get_visual_configs_property_list_length(const xcb_glx_get_visual_configs_reply_t *)")
fn("xcb_generic_iterator_t xcb_glx_get_visual_configs_property_list_end(const xcb_glx_get_visual_configs_reply_t *)")

fn("xcb_glx_get_visual_configs_reply_t * xcb_glx_get_visual_configs_reply(xcb_connection_t *, xcb_glx_get_visual_configs_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("xcb_void_cookie_t xcb_glx_destroy_glx_pixmap_checked(xcb_connection_t *, xcb_glx_pixmap_t)")
fn("xcb_void_cookie_t xcb_glx_destroy_glx_pixmap(xcb_connection_t *, xcb_glx_pixmap_t)")
fn("int xcb_glx_vendor_private_sizeof(const void *, uint32_t)")
fn("xcb_void_cookie_t xcb_glx_vendor_private_checked(xcb_connection_t *, uint32_t, xcb_glx_context_tag_t, uint32_t, const uint8_t *)")
fn("xcb_void_cookie_t xcb_glx_vendor_private(xcb_connection_t *, uint32_t, xcb_glx_context_tag_t, uint32_t, const uint8_t *)")
# ::Iterator::
fn("uint8_t * xcb_glx_vendor_private_data(const xcb_glx_vendor_private_request_t *)")
fn("int xcb_glx_vendor_private_data_length(const xcb_glx_vendor_private_request_t *)")
fn("xcb_generic_iterator_t xcb_glx_vendor_private_data_end(const xcb_glx_vendor_private_request_t *)")

fn("int xcb_glx_vendor_private_with_reply_sizeof(const void *, uint32_t)")
fn("xcb_glx_vendor_private_with_reply_cookie_t xcb_glx_vendor_private_with_reply(xcb_connection_t *, uint32_t, xcb_glx_context_tag_t, uint32_t, const uint8_t *)")
fn("xcb_glx_vendor_private_with_reply_cookie_t xcb_glx_vendor_private_with_reply_unchecked(xcb_connection_t *, uint32_t, xcb_glx_context_tag_t, uint32_t, const uint8_t *)")
# ::Iterator::
fn("uint8_t * xcb_glx_vendor_private_with_reply_data_2(const xcb_glx_vendor_private_with_reply_reply_t *)")
fn("int xcb_glx_vendor_private_with_reply_data_2_length(const xcb_glx_vendor_private_with_reply_reply_t *)")
fn("xcb_generic_iterator_t xcb_glx_vendor_private_with_reply_data_2_end(const xcb_glx_vendor_private_with_reply_reply_t *)")

fn("xcb_glx_vendor_private_with_reply_reply_t * xcb_glx_vendor_private_with_reply_reply(xcb_connection_t *, xcb_glx_vendor_private_with_reply_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("xcb_glx_query_extensions_string_cookie_t xcb_glx_query_extensions_string(xcb_connection_t *, uint32_t)")
fn("xcb_glx_query_extensions_string_cookie_t xcb_glx_query_extensions_string_unchecked(xcb_connection_t *, uint32_t)")
fn("xcb_glx_query_extensions_string_reply_t * xcb_glx_query_extensions_string_reply(xcb_connection_t *, xcb_glx_query_extensions_string_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("int xcb_glx_query_server_string_sizeof(const void *)")
fn("xcb_glx_query_server_string_cookie_t xcb_glx_query_server_string(xcb_connection_t *, uint32_t, uint32_t)")
fn("xcb_glx_query_server_string_cookie_t xcb_glx_query_server_string_unchecked(xcb_connection_t *, uint32_t, uint32_t)")
# ::Iterator::
fn("char * xcb_glx_query_server_string_string(const xcb_glx_query_server_string_reply_t *)")
fn("int xcb_glx_query_server_string_string_length(const xcb_glx_query_server_string_reply_t *)")
fn("xcb_generic_iterator_t xcb_glx_query_server_string_string_end(const xcb_glx_query_server_string_reply_t *)")

fn("xcb_glx_query_server_string_reply_t * xcb_glx_query_server_string_reply(xcb_connection_t *, xcb_glx_query_server_string_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("int xcb_glx_client_info_sizeof(const void *)")
fn("xcb_void_cookie_t xcb_glx_client_info_checked(xcb_connection_t *, uint32_t, uint32_t, uint32_t, const char *)")
fn("xcb_void_cookie_t xcb_glx_client_info(xcb_connection_t *, uint32_t, uint32_t, uint32_t, const char *)")
# ::Iterator::
fn("char * xcb_glx_client_info_string(const xcb_glx_client_info_request_t *)")
fn("int xcb_glx_client_info_string_length(const xcb_glx_client_info_request_t *)")
fn("xcb_generic_iterator_t xcb_glx_client_info_string_end(const xcb_glx_client_info_request_t *)")

fn("int xcb_glx_get_fb_configs_sizeof(const void *)")
fn("xcb_glx_get_fb_configs_cookie_t xcb_glx_get_fb_configs(xcb_connection_t *, uint32_t)")
fn("xcb_glx_get_fb_configs_cookie_t xcb_glx_get_fb_configs_unchecked(xcb_connection_t *, uint32_t)")
# ::Iterator::
fn("uint32_t * xcb_glx_get_fb_configs_property_list(const xcb_glx_get_fb_configs_reply_t *)")
fn("int xcb_glx_get_fb_configs_property_list_length(const xcb_glx_get_fb_configs_reply_t *)")
fn("xcb_generic_iterator_t xcb_glx_get_fb_configs_property_list_end(const xcb_glx_get_fb_configs_reply_t *)")

fn("xcb_glx_get_fb_configs_reply_t * xcb_glx_get_fb_configs_reply(xcb_connection_t *, xcb_glx_get_fb_configs_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("int xcb_glx_create_pixmap_sizeof(const void *)")
fn("xcb_void_cookie_t xcb_glx_create_pixmap_checked(xcb_connection_t *, uint32_t, xcb_glx_fbconfig_t, xcb_pixmap_t, xcb_glx_pixmap_t, uint32_t, const uint32_t *)")
fn("xcb_void_cookie_t xcb_glx_create_pixmap(xcb_connection_t *, uint32_t, xcb_glx_fbconfig_t, xcb_pixmap_t, xcb_glx_pixmap_t, uint32_t, const uint32_t *)")
# ::Iterator::
fn("uint32_t * xcb_glx_create_pixmap_attribs(const xcb_glx_create_pixmap_request_t *)")
fn("int xcb_glx_create_pixmap_attribs_length(const xcb_glx_create_pixmap_request_t *)")
fn("xcb_generic_iterator_t xcb_glx_create_pixmap_attribs_end(const xcb_glx_create_pixmap_request_t *)")

fn("xcb_void_cookie_t xcb_glx_destroy_pixmap_checked(xcb_connection_t *, xcb_glx_pixmap_t)")
fn("xcb_void_cookie_t xcb_glx_destroy_pixmap(xcb_connection_t *, xcb_glx_pixmap_t)")
fn("xcb_void_cookie_t xcb_glx_create_new_context_checked(xcb_connection_t *, xcb_glx_context_t, xcb_glx_fbconfig_t, uint32_t, uint32_t, xcb_glx_context_t, uint8_t)")
fn("xcb_void_cookie_t xcb_glx_create_new_context(xcb_connection_t *, xcb_glx_context_t, xcb_glx_fbconfig_t, uint32_t, uint32_t, xcb_glx_context_t, uint8_t)")
fn("int xcb_glx_query_context_sizeof(const void *)")
fn("xcb_glx_query_context_cookie_t xcb_glx_query_context(xcb_connection_t *, xcb_glx_context_t)")
fn("xcb_glx_query_context_cookie_t xcb_glx_query_context_unchecked(xcb_connection_t *, xcb_glx_context_t)")
# ::Iterator::
fn("uint32_t * xcb_glx_query_context_attribs(const xcb_glx_query_context_reply_t *)")
fn("int xcb_glx_query_context_attribs_length(const xcb_glx_query_context_reply_t *)")
fn("xcb_generic_iterator_t xcb_glx_query_context_attribs_end(const xcb_glx_query_context_reply_t *)")

fn("xcb_glx_query_context_reply_t * xcb_glx_query_context_reply(xcb_connection_t *, xcb_glx_query_context_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("xcb_glx_make_context_current_cookie_t xcb_glx_make_context_current(xcb_connection_t *, xcb_glx_context_tag_t, xcb_glx_drawable_t, xcb_glx_drawable_t, xcb_glx_context_t)")
fn("xcb_glx_make_context_current_cookie_t xcb_glx_make_context_current_unchecked(xcb_connection_t *, xcb_glx_context_tag_t, xcb_glx_drawable_t, xcb_glx_drawable_t, xcb_glx_context_t)")
fn("xcb_glx_make_context_current_reply_t * xcb_glx_make_context_current_reply(xcb_connection_t *, xcb_glx_make_context_current_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("int xcb_glx_create_pbuffer_sizeof(const void *)")
fn("xcb_void_cookie_t xcb_glx_create_pbuffer_checked(xcb_connection_t *, uint32_t, xcb_glx_fbconfig_t, xcb_glx_pbuffer_t, uint32_t, const uint32_t *)")
fn("xcb_void_cookie_t xcb_glx_create_pbuffer(xcb_connection_t *, uint32_t, xcb_glx_fbconfig_t, xcb_glx_pbuffer_t, uint32_t, const uint32_t *)")
# ::Iterator::
fn("uint32_t * xcb_glx_create_pbuffer_attribs(const xcb_glx_create_pbuffer_request_t *)")
fn("int xcb_glx_create_pbuffer_attribs_length(const xcb_glx_create_pbuffer_request_t *)")
fn("xcb_generic_iterator_t xcb_glx_create_pbuffer_attribs_end(const xcb_glx_create_pbuffer_request_t *)")

fn("xcb_void_cookie_t xcb_glx_destroy_pbuffer_checked(xcb_connection_t *, xcb_glx_pbuffer_t)")
fn("xcb_void_cookie_t xcb_glx_destroy_pbuffer(xcb_connection_t *, xcb_glx_pbuffer_t)")
fn("int xcb_glx_get_drawable_attributes_sizeof(const void *)")
fn("xcb_glx_get_drawable_attributes_cookie_t xcb_glx_get_drawable_attributes(xcb_connection_t *, xcb_glx_drawable_t)")
fn("xcb_glx_get_drawable_attributes_cookie_t xcb_glx_get_drawable_attributes_unchecked(xcb_connection_t *, xcb_glx_drawable_t)")
# ::Iterator::
fn("uint32_t * xcb_glx_get_drawable_attributes_attribs(const xcb_glx_get_drawable_attributes_reply_t *)")
fn("int xcb_glx_get_drawable_attributes_attribs_length(const xcb_glx_get_drawable_attributes_reply_t *)")
fn("xcb_generic_iterator_t xcb_glx_get_drawable_attributes_attribs_end(const xcb_glx_get_drawable_attributes_reply_t *)")

fn("xcb_glx_get_drawable_attributes_reply_t * xcb_glx_get_drawable_attributes_reply(xcb_connection_t *, xcb_glx_get_drawable_attributes_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("int xcb_glx_change_drawable_attributes_sizeof(const void *)")
fn("xcb_void_cookie_t xcb_glx_change_drawable_attributes_checked(xcb_connection_t *, xcb_glx_drawable_t, uint32_t, const uint32_t *)")
fn("xcb_void_cookie_t xcb_glx_change_drawable_attributes(xcb_connection_t *, xcb_glx_drawable_t, uint32_t, const uint32_t *)")
# ::Iterator::
fn("uint32_t * xcb_glx_change_drawable_attributes_attribs(const xcb_glx_change_drawable_attributes_request_t *)")
fn("int xcb_glx_change_drawable_attributes_attribs_length(const xcb_glx_change_drawable_attributes_request_t *)")
fn("xcb_generic_iterator_t xcb_glx_change_drawable_attributes_attribs_end(const xcb_glx_change_drawable_attributes_request_t *)")

fn("int xcb_glx_create_window_sizeof(const void *)")
fn("xcb_void_cookie_t xcb_glx_create_window_checked(xcb_connection_t *, uint32_t, xcb_glx_fbconfig_t, xcb_window_t, xcb_glx_window_t, uint32_t, const uint32_t *)")
fn("xcb_void_cookie_t xcb_glx_create_window(xcb_connection_t *, uint32_t, xcb_glx_fbconfig_t, xcb_window_t, xcb_glx_window_t, uint32_t, const uint32_t *)")
# ::Iterator::
fn("uint32_t * xcb_glx_create_window_attribs(const xcb_glx_create_window_request_t *)")
fn("int xcb_glx_create_window_attribs_length(const xcb_glx_create_window_request_t *)")
fn("xcb_generic_iterator_t xcb_glx_create_window_attribs_end(const xcb_glx_create_window_request_t *)")

fn("xcb_void_cookie_t xcb_glx_delete_window_checked(xcb_connection_t *, xcb_glx_window_t)")
fn("xcb_void_cookie_t xcb_glx_delete_window(xcb_connection_t *, xcb_glx_window_t)")
fn("int xcb_glx_set_client_info_arb_sizeof(const void *)")
fn("xcb_void_cookie_t xcb_glx_set_client_info_arb_checked(xcb_connection_t *, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, const uint32_t *, const char *, const char *)")
fn("xcb_void_cookie_t xcb_glx_set_client_info_arb(xcb_connection_t *, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, const uint32_t *, const char *, const char *)")
# ::Iterator::
fn("uint32_t * xcb_glx_set_client_info_arb_gl_versions(const xcb_glx_set_client_info_arb_request_t *)")
fn("int xcb_glx_set_client_info_arb_gl_versions_length(const xcb_glx_set_client_info_arb_request_t *)")
fn("xcb_generic_iterator_t xcb_glx_set_client_info_arb_gl_versions_end(const xcb_glx_set_client_info_arb_request_t *)")

# ::Iterator::
fn("char * xcb_glx_set_client_info_arb_gl_extension_string(const xcb_glx_set_client_info_arb_request_t *)")
fn("int xcb_glx_set_client_info_arb_gl_extension_string_length(const xcb_glx_set_client_info_arb_request_t *)")
fn("xcb_generic_iterator_t xcb_glx_set_client_info_arb_gl_extension_string_end(const xcb_glx_set_client_info_arb_request_t *)")

# ::Iterator::
fn("char * xcb_glx_set_client_info_arb_glx_extension_string(const xcb_glx_set_client_info_arb_request_t *)")
fn("int xcb_glx_set_client_info_arb_glx_extension_string_length(const xcb_glx_set_client_info_arb_request_t *)")
fn("xcb_generic_iterator_t xcb_glx_set_client_info_arb_glx_extension_string_end(const xcb_glx_set_client_info_arb_request_t *)")

fn("int xcb_glx_create_context_attribs_arb_sizeof(const void *)")
fn("xcb_void_cookie_t xcb_glx_create_context_attribs_arb_checked(xcb_connection_t *, xcb_glx_context_t, xcb_glx_fbconfig_t, uint32_t, xcb_glx_context_t, uint8_t, uint32_t, const uint32_t *)")
fn("xcb_void_cookie_t xcb_glx_create_context_attribs_arb(xcb_connection_t *, xcb_glx_context_t, xcb_glx_fbconfig_t, uint32_t, xcb_glx_context_t, uint8_t, uint32_t, const uint32_t *)")
# ::Iterator::
fn("uint32_t * xcb_glx_create_context_attribs_arb_attribs(const xcb_glx_create_context_attribs_arb_request_t *)")
fn("int xcb_glx_create_context_attribs_arb_attribs_length(const xcb_glx_create_context_attribs_arb_request_t *)")
fn("xcb_generic_iterator_t xcb_glx_create_context_attribs_arb_attribs_end(const xcb_glx_create_context_attribs_arb_request_t *)")

fn("int xcb_glx_set_client_info_2arb_sizeof(const void *)")
fn("xcb_void_cookie_t xcb_glx_set_client_info_2arb_checked(xcb_connection_t *, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, const uint32_t *, const char *, const char *)")
fn("xcb_void_cookie_t xcb_glx_set_client_info_2arb(xcb_connection_t *, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, const uint32_t *, const char *, const char *)")
# ::Iterator::
fn("uint32_t * xcb_glx_set_client_info_2arb_gl_versions(const xcb_glx_set_client_info_2arb_request_t *)")
fn("int xcb_glx_set_client_info_2arb_gl_versions_length(const xcb_glx_set_client_info_2arb_request_t *)")
fn("xcb_generic_iterator_t xcb_glx_set_client_info_2arb_gl_versions_end(const xcb_glx_set_client_info_2arb_request_t *)")

# ::Iterator::
fn("char * xcb_glx_set_client_info_2arb_gl_extension_string(const xcb_glx_set_client_info_2arb_request_t *)")
fn("int xcb_glx_set_client_info_2arb_gl_extension_string_length(const xcb_glx_set_client_info_2arb_request_t *)")
fn("xcb_generic_iterator_t xcb_glx_set_client_info_2arb_gl_extension_string_end(const xcb_glx_set_client_info_2arb_request_t *)")

# ::Iterator::
fn("char * xcb_glx_set_client_info_2arb_glx_extension_string(const xcb_glx_set_client_info_2arb_request_t *)")
fn("int xcb_glx_set_client_info_2arb_glx_extension_string_length(const xcb_glx_set_client_info_2arb_request_t *)")
fn("xcb_generic_iterator_t xcb_glx_set_client_info_2arb_glx_extension_string_end(const xcb_glx_set_client_info_2arb_request_t *)")

fn("xcb_void_cookie_t xcb_glx_new_list_checked(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t)")
fn("xcb_void_cookie_t xcb_glx_new_list(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t)")
fn("xcb_void_cookie_t xcb_glx_end_list_checked(xcb_connection_t *, xcb_glx_context_tag_t)")
fn("xcb_void_cookie_t xcb_glx_end_list(xcb_connection_t *, xcb_glx_context_tag_t)")
fn("xcb_void_cookie_t xcb_glx_delete_lists_checked(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, int32_t)")
fn("xcb_void_cookie_t xcb_glx_delete_lists(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, int32_t)")
fn("xcb_glx_gen_lists_cookie_t xcb_glx_gen_lists(xcb_connection_t *, xcb_glx_context_tag_t, int32_t)")
fn("xcb_glx_gen_lists_cookie_t xcb_glx_gen_lists_unchecked(xcb_connection_t *, xcb_glx_context_tag_t, int32_t)")
fn("xcb_glx_gen_lists_reply_t * xcb_glx_gen_lists_reply(xcb_connection_t *, xcb_glx_gen_lists_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("xcb_void_cookie_t xcb_glx_feedback_buffer_checked(xcb_connection_t *, xcb_glx_context_tag_t, int32_t, int32_t)")
fn("xcb_void_cookie_t xcb_glx_feedback_buffer(xcb_connection_t *, xcb_glx_context_tag_t, int32_t, int32_t)")
fn("xcb_void_cookie_t xcb_glx_select_buffer_checked(xcb_connection_t *, xcb_glx_context_tag_t, int32_t)")
fn("xcb_void_cookie_t xcb_glx_select_buffer(xcb_connection_t *, xcb_glx_context_tag_t, int32_t)")
fn("int xcb_glx_render_mode_sizeof(const void *)")
fn("xcb_glx_render_mode_cookie_t xcb_glx_render_mode(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t)")
fn("xcb_glx_render_mode_cookie_t xcb_glx_render_mode_unchecked(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t)")
# ::Iterator::
fn("uint32_t * xcb_glx_render_mode_data(const xcb_glx_render_mode_reply_t *)")
fn("int xcb_glx_render_mode_data_length(const xcb_glx_render_mode_reply_t *)")
fn("xcb_generic_iterator_t xcb_glx_render_mode_data_end(const xcb_glx_render_mode_reply_t *)")

fn("xcb_glx_render_mode_reply_t * xcb_glx_render_mode_reply(xcb_connection_t *, xcb_glx_render_mode_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("xcb_glx_finish_cookie_t xcb_glx_finish(xcb_connection_t *, xcb_glx_context_tag_t)")
fn("xcb_glx_finish_cookie_t xcb_glx_finish_unchecked(xcb_connection_t *, xcb_glx_context_tag_t)")
fn("xcb_glx_finish_reply_t * xcb_glx_finish_reply(xcb_connection_t *, xcb_glx_finish_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("xcb_void_cookie_t xcb_glx_pixel_storef_checked(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, xcb_glx_float32_t)")
fn("xcb_void_cookie_t xcb_glx_pixel_storef(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, xcb_glx_float32_t)")
fn("xcb_void_cookie_t xcb_glx_pixel_storei_checked(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, int32_t)")
fn("xcb_void_cookie_t xcb_glx_pixel_storei(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, int32_t)")
fn("int xcb_glx_read_pixels_sizeof(const void *)")
fn("xcb_glx_read_pixels_cookie_t xcb_glx_read_pixels(xcb_connection_t *, xcb_glx_context_tag_t, int32_t, int32_t, int32_t, int32_t, uint32_t, uint32_t, uint8_t, uint8_t)")
fn("xcb_glx_read_pixels_cookie_t xcb_glx_read_pixels_unchecked(xcb_connection_t *, xcb_glx_context_tag_t, int32_t, int32_t, int32_t, int32_t, uint32_t, uint32_t, uint8_t, uint8_t)")
# ::Iterator::
fn("uint8_t * xcb_glx_read_pixels_data(const xcb_glx_read_pixels_reply_t *)")
fn("int xcb_glx_read_pixels_data_length(const xcb_glx_read_pixels_reply_t *)")
fn("xcb_generic_iterator_t xcb_glx_read_pixels_data_end(const xcb_glx_read_pixels_reply_t *)")

fn("xcb_glx_read_pixels_reply_t * xcb_glx_read_pixels_reply(xcb_connection_t *, xcb_glx_read_pixels_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("int xcb_glx_get_booleanv_sizeof(const void *)")
fn("xcb_glx_get_booleanv_cookie_t xcb_glx_get_booleanv(xcb_connection_t *, xcb_glx_context_tag_t, int32_t)")
fn("xcb_glx_get_booleanv_cookie_t xcb_glx_get_booleanv_unchecked(xcb_connection_t *, xcb_glx_context_tag_t, int32_t)")
# ::Iterator::
fn("uint8_t * xcb_glx_get_booleanv_data(const xcb_glx_get_booleanv_reply_t *)")
fn("int xcb_glx_get_booleanv_data_length(const xcb_glx_get_booleanv_reply_t *)")
fn("xcb_generic_iterator_t xcb_glx_get_booleanv_data_end(const xcb_glx_get_booleanv_reply_t *)")

fn("xcb_glx_get_booleanv_reply_t * xcb_glx_get_booleanv_reply(xcb_connection_t *, xcb_glx_get_booleanv_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("int xcb_glx_get_clip_plane_sizeof(const void *)")
fn("xcb_glx_get_clip_plane_cookie_t xcb_glx_get_clip_plane(xcb_connection_t *, xcb_glx_context_tag_t, int32_t)")
fn("xcb_glx_get_clip_plane_cookie_t xcb_glx_get_clip_plane_unchecked(xcb_connection_t *, xcb_glx_context_tag_t, int32_t)")
# ::Iterator::
fn("xcb_glx_float64_t * xcb_glx_get_clip_plane_data(const xcb_glx_get_clip_plane_reply_t *)")
fn("int xcb_glx_get_clip_plane_data_length(const xcb_glx_get_clip_plane_reply_t *)")
fn("xcb_generic_iterator_t xcb_glx_get_clip_plane_data_end(const xcb_glx_get_clip_plane_reply_t *)")

fn("xcb_glx_get_clip_plane_reply_t * xcb_glx_get_clip_plane_reply(xcb_connection_t *, xcb_glx_get_clip_plane_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("int xcb_glx_get_doublev_sizeof(const void *)")
fn("xcb_glx_get_doublev_cookie_t xcb_glx_get_doublev(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t)")
fn("xcb_glx_get_doublev_cookie_t xcb_glx_get_doublev_unchecked(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t)")
# ::Iterator::
fn("xcb_glx_float64_t * xcb_glx_get_doublev_data(const xcb_glx_get_doublev_reply_t *)")
fn("int xcb_glx_get_doublev_data_length(const xcb_glx_get_doublev_reply_t *)")
fn("xcb_generic_iterator_t xcb_glx_get_doublev_data_end(const xcb_glx_get_doublev_reply_t *)")

fn("xcb_glx_get_doublev_reply_t * xcb_glx_get_doublev_reply(xcb_connection_t *, xcb_glx_get_doublev_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("xcb_glx_get_error_cookie_t xcb_glx_get_error(xcb_connection_t *, xcb_glx_context_tag_t)")
fn("xcb_glx_get_error_cookie_t xcb_glx_get_error_unchecked(xcb_connection_t *, xcb_glx_context_tag_t)")
fn("xcb_glx_get_error_reply_t * xcb_glx_get_error_reply(xcb_connection_t *, xcb_glx_get_error_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("int xcb_glx_get_floatv_sizeof(const void *)")
fn("xcb_glx_get_floatv_cookie_t xcb_glx_get_floatv(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t)")
fn("xcb_glx_get_floatv_cookie_t xcb_glx_get_floatv_unchecked(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t)")
# ::Iterator::
fn("xcb_glx_float32_t * xcb_glx_get_floatv_data(const xcb_glx_get_floatv_reply_t *)")
fn("int xcb_glx_get_floatv_data_length(const xcb_glx_get_floatv_reply_t *)")
fn("xcb_generic_iterator_t xcb_glx_get_floatv_data_end(const xcb_glx_get_floatv_reply_t *)")

fn("xcb_glx_get_floatv_reply_t * xcb_glx_get_floatv_reply(xcb_connection_t *, xcb_glx_get_floatv_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("int xcb_glx_get_integerv_sizeof(const void *)")
fn("xcb_glx_get_integerv_cookie_t xcb_glx_get_integerv(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t)")
fn("xcb_glx_get_integerv_cookie_t xcb_glx_get_integerv_unchecked(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t)")
# ::Iterator::
fn("int32_t * xcb_glx_get_integerv_data(const xcb_glx_get_integerv_reply_t *)")
fn("int xcb_glx_get_integerv_data_length(const xcb_glx_get_integerv_reply_t *)")
fn("xcb_generic_iterator_t xcb_glx_get_integerv_data_end(const xcb_glx_get_integerv_reply_t *)")

fn("xcb_glx_get_integerv_reply_t * xcb_glx_get_integerv_reply(xcb_connection_t *, xcb_glx_get_integerv_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("int xcb_glx_get_lightfv_sizeof(const void *)")
fn("xcb_glx_get_lightfv_cookie_t xcb_glx_get_lightfv(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t)")
fn("xcb_glx_get_lightfv_cookie_t xcb_glx_get_lightfv_unchecked(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t)")
# ::Iterator::
fn("xcb_glx_float32_t * xcb_glx_get_lightfv_data(const xcb_glx_get_lightfv_reply_t *)")
fn("int xcb_glx_get_lightfv_data_length(const xcb_glx_get_lightfv_reply_t *)")
fn("xcb_generic_iterator_t xcb_glx_get_lightfv_data_end(const xcb_glx_get_lightfv_reply_t *)")

fn("xcb_glx_get_lightfv_reply_t * xcb_glx_get_lightfv_reply(xcb_connection_t *, xcb_glx_get_lightfv_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("int xcb_glx_get_lightiv_sizeof(const void *)")
fn("xcb_glx_get_lightiv_cookie_t xcb_glx_get_lightiv(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t)")
fn("xcb_glx_get_lightiv_cookie_t xcb_glx_get_lightiv_unchecked(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t)")
# ::Iterator::
fn("int32_t * xcb_glx_get_lightiv_data(const xcb_glx_get_lightiv_reply_t *)")
fn("int xcb_glx_get_lightiv_data_length(const xcb_glx_get_lightiv_reply_t *)")
fn("xcb_generic_iterator_t xcb_glx_get_lightiv_data_end(const xcb_glx_get_lightiv_reply_t *)")

fn("xcb_glx_get_lightiv_reply_t * xcb_glx_get_lightiv_reply(xcb_connection_t *, xcb_glx_get_lightiv_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("int xcb_glx_get_mapdv_sizeof(const void *)")
fn("xcb_glx_get_mapdv_cookie_t xcb_glx_get_mapdv(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t)")
fn("xcb_glx_get_mapdv_cookie_t xcb_glx_get_mapdv_unchecked(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t)")
# ::Iterator::
fn("xcb_glx_float64_t * xcb_glx_get_mapdv_data(const xcb_glx_get_mapdv_reply_t *)")
fn("int xcb_glx_get_mapdv_data_length(const xcb_glx_get_mapdv_reply_t *)")
fn("xcb_generic_iterator_t xcb_glx_get_mapdv_data_end(const xcb_glx_get_mapdv_reply_t *)")

fn("xcb_glx_get_mapdv_reply_t * xcb_glx_get_mapdv_reply(xcb_connection_t *, xcb_glx_get_mapdv_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("int xcb_glx_get_mapfv_sizeof(const void *)")
fn("xcb_glx_get_mapfv_cookie_t xcb_glx_get_mapfv(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t)")
fn("xcb_glx_get_mapfv_cookie_t xcb_glx_get_mapfv_unchecked(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t)")
# ::Iterator::
fn("xcb_glx_float32_t * xcb_glx_get_mapfv_data(const xcb_glx_get_mapfv_reply_t *)")
fn("int xcb_glx_get_mapfv_data_length(const xcb_glx_get_mapfv_reply_t *)")
fn("xcb_generic_iterator_t xcb_glx_get_mapfv_data_end(const xcb_glx_get_mapfv_reply_t *)")

fn("xcb_glx_get_mapfv_reply_t * xcb_glx_get_mapfv_reply(xcb_connection_t *, xcb_glx_get_mapfv_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("int xcb_glx_get_mapiv_sizeof(const void *)")
fn("xcb_glx_get_mapiv_cookie_t xcb_glx_get_mapiv(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t)")
fn("xcb_glx_get_mapiv_cookie_t xcb_glx_get_mapiv_unchecked(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t)")
# ::Iterator::
fn("int32_t * xcb_glx_get_mapiv_data(const xcb_glx_get_mapiv_reply_t *)")
fn("int xcb_glx_get_mapiv_data_length(const xcb_glx_get_mapiv_reply_t *)")
fn("xcb_generic_iterator_t xcb_glx_get_mapiv_data_end(const xcb_glx_get_mapiv_reply_t *)")

fn("xcb_glx_get_mapiv_reply_t * xcb_glx_get_mapiv_reply(xcb_connection_t *, xcb_glx_get_mapiv_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("int xcb_glx_get_materialfv_sizeof(const void *)")
fn("xcb_glx_get_materialfv_cookie_t xcb_glx_get_materialfv(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t)")
fn("xcb_glx_get_materialfv_cookie_t xcb_glx_get_materialfv_unchecked(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t)")
# ::Iterator::
fn("xcb_glx_float32_t * xcb_glx_get_materialfv_data(const xcb_glx_get_materialfv_reply_t *)")
fn("int xcb_glx_get_materialfv_data_length(const xcb_glx_get_materialfv_reply_t *)")
fn("xcb_generic_iterator_t xcb_glx_get_materialfv_data_end(const xcb_glx_get_materialfv_reply_t *)")

fn("xcb_glx_get_materialfv_reply_t * xcb_glx_get_materialfv_reply(xcb_connection_t *, xcb_glx_get_materialfv_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("int xcb_glx_get_materialiv_sizeof(const void *)")
fn("xcb_glx_get_materialiv_cookie_t xcb_glx_get_materialiv(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t)")
fn("xcb_glx_get_materialiv_cookie_t xcb_glx_get_materialiv_unchecked(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t)")
# ::Iterator::
fn("int32_t * xcb_glx_get_materialiv_data(const xcb_glx_get_materialiv_reply_t *)")
fn("int xcb_glx_get_materialiv_data_length(const xcb_glx_get_materialiv_reply_t *)")
fn("xcb_generic_iterator_t xcb_glx_get_materialiv_data_end(const xcb_glx_get_materialiv_reply_t *)")

fn("xcb_glx_get_materialiv_reply_t * xcb_glx_get_materialiv_reply(xcb_connection_t *, xcb_glx_get_materialiv_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("int xcb_glx_get_pixel_mapfv_sizeof(const void *)")
fn("xcb_glx_get_pixel_mapfv_cookie_t xcb_glx_get_pixel_mapfv(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t)")
fn("xcb_glx_get_pixel_mapfv_cookie_t xcb_glx_get_pixel_mapfv_unchecked(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t)")
# ::Iterator::
fn("xcb_glx_float32_t * xcb_glx_get_pixel_mapfv_data(const xcb_glx_get_pixel_mapfv_reply_t *)")
fn("int xcb_glx_get_pixel_mapfv_data_length(const xcb_glx_get_pixel_mapfv_reply_t *)")
fn("xcb_generic_iterator_t xcb_glx_get_pixel_mapfv_data_end(const xcb_glx_get_pixel_mapfv_reply_t *)")

fn("xcb_glx_get_pixel_mapfv_reply_t * xcb_glx_get_pixel_mapfv_reply(xcb_connection_t *, xcb_glx_get_pixel_mapfv_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("int xcb_glx_get_pixel_mapuiv_sizeof(const void *)")
fn("xcb_glx_get_pixel_mapuiv_cookie_t xcb_glx_get_pixel_mapuiv(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t)")
fn("xcb_glx_get_pixel_mapuiv_cookie_t xcb_glx_get_pixel_mapuiv_unchecked(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t)")
# ::Iterator::
fn("uint32_t * xcb_glx_get_pixel_mapuiv_data(const xcb_glx_get_pixel_mapuiv_reply_t *)")
fn("int xcb_glx_get_pixel_mapuiv_data_length(const xcb_glx_get_pixel_mapuiv_reply_t *)")
fn("xcb_generic_iterator_t xcb_glx_get_pixel_mapuiv_data_end(const xcb_glx_get_pixel_mapuiv_reply_t *)")

fn("xcb_glx_get_pixel_mapuiv_reply_t * xcb_glx_get_pixel_mapuiv_reply(xcb_connection_t *, xcb_glx_get_pixel_mapuiv_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("int xcb_glx_get_pixel_mapusv_sizeof(const void *)")
fn("xcb_glx_get_pixel_mapusv_cookie_t xcb_glx_get_pixel_mapusv(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t)")
fn("xcb_glx_get_pixel_mapusv_cookie_t xcb_glx_get_pixel_mapusv_unchecked(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t)")
# ::Iterator::
fn("uint16_t * xcb_glx_get_pixel_mapusv_data(const xcb_glx_get_pixel_mapusv_reply_t *)")
fn("int xcb_glx_get_pixel_mapusv_data_length(const xcb_glx_get_pixel_mapusv_reply_t *)")
fn("xcb_generic_iterator_t xcb_glx_get_pixel_mapusv_data_end(const xcb_glx_get_pixel_mapusv_reply_t *)")

fn("xcb_glx_get_pixel_mapusv_reply_t * xcb_glx_get_pixel_mapusv_reply(xcb_connection_t *, xcb_glx_get_pixel_mapusv_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("int xcb_glx_get_polygon_stipple_sizeof(const void *)")
fn("xcb_glx_get_polygon_stipple_cookie_t xcb_glx_get_polygon_stipple(xcb_connection_t *, xcb_glx_context_tag_t, uint8_t)")
fn("xcb_glx_get_polygon_stipple_cookie_t xcb_glx_get_polygon_stipple_unchecked(xcb_connection_t *, xcb_glx_context_tag_t, uint8_t)")
# ::Iterator::
fn("uint8_t * xcb_glx_get_polygon_stipple_data(const xcb_glx_get_polygon_stipple_reply_t *)")
fn("int xcb_glx_get_polygon_stipple_data_length(const xcb_glx_get_polygon_stipple_reply_t *)")
fn("xcb_generic_iterator_t xcb_glx_get_polygon_stipple_data_end(const xcb_glx_get_polygon_stipple_reply_t *)")

fn("xcb_glx_get_polygon_stipple_reply_t * xcb_glx_get_polygon_stipple_reply(xcb_connection_t *, xcb_glx_get_polygon_stipple_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("int xcb_glx_get_string_sizeof(const void *)")
fn("xcb_glx_get_string_cookie_t xcb_glx_get_string(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t)")
fn("xcb_glx_get_string_cookie_t xcb_glx_get_string_unchecked(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t)")
# ::Iterator::
fn("char * xcb_glx_get_string_string(const xcb_glx_get_string_reply_t *)")
fn("int xcb_glx_get_string_string_length(const xcb_glx_get_string_reply_t *)")
fn("xcb_generic_iterator_t xcb_glx_get_string_string_end(const xcb_glx_get_string_reply_t *)")

fn("xcb_glx_get_string_reply_t * xcb_glx_get_string_reply(xcb_connection_t *, xcb_glx_get_string_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("int xcb_glx_get_tex_envfv_sizeof(const void *)")
fn("xcb_glx_get_tex_envfv_cookie_t xcb_glx_get_tex_envfv(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t)")
fn("xcb_glx_get_tex_envfv_cookie_t xcb_glx_get_tex_envfv_unchecked(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t)")
fn("xcb_glx_float32_t * xcb_glx_get_tex_envfv_data(const xcb_glx_get_tex_envfv_reply_t *)")
fn("int xcb_glx_get_tex_envfv_data_length(const xcb_glx_get_tex_envfv_reply_t *)")
fn("xcb_generic_iterator_t xcb_glx_get_tex_envfv_data_end(const xcb_glx_get_tex_envfv_reply_t *)")
fn("xcb_glx_get_tex_envfv_reply_t * xcb_glx_get_tex_envfv_reply(xcb_connection_t *, xcb_glx_get_tex_envfv_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("int xcb_glx_get_tex_enviv_sizeof(const void *)")
fn("xcb_glx_get_tex_enviv_cookie_t xcb_glx_get_tex_enviv(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t)")
fn("xcb_glx_get_tex_enviv_cookie_t xcb_glx_get_tex_enviv_unchecked(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t)")
# ::Iterator::
fn("int32_t * xcb_glx_get_tex_enviv_data(const xcb_glx_get_tex_enviv_reply_t *)")
fn("int xcb_glx_get_tex_enviv_data_length(const xcb_glx_get_tex_enviv_reply_t *)")
fn("xcb_generic_iterator_t xcb_glx_get_tex_enviv_data_end(const xcb_glx_get_tex_enviv_reply_t *)")

fn("xcb_glx_get_tex_enviv_reply_t * xcb_glx_get_tex_enviv_reply(xcb_connection_t *, xcb_glx_get_tex_enviv_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("int xcb_glx_get_tex_gendv_sizeof(const void *)")
fn("xcb_glx_get_tex_gendv_cookie_t xcb_glx_get_tex_gendv(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t)")
fn("xcb_glx_get_tex_gendv_cookie_t xcb_glx_get_tex_gendv_unchecked(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t)")
# ::Iterator::
fn("xcb_glx_float64_t * xcb_glx_get_tex_gendv_data(const xcb_glx_get_tex_gendv_reply_t *)")
fn("int xcb_glx_get_tex_gendv_data_length(const xcb_glx_get_tex_gendv_reply_t *)")
fn("xcb_generic_iterator_t xcb_glx_get_tex_gendv_data_end(const xcb_glx_get_tex_gendv_reply_t *)")

fn("xcb_glx_get_tex_gendv_reply_t * xcb_glx_get_tex_gendv_reply(xcb_connection_t *, xcb_glx_get_tex_gendv_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("int xcb_glx_get_tex_genfv_sizeof(const void *)")
fn("xcb_glx_get_tex_genfv_cookie_t xcb_glx_get_tex_genfv(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t)")
fn("xcb_glx_get_tex_genfv_cookie_t xcb_glx_get_tex_genfv_unchecked(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t)")
# ::Iterator::
fn("xcb_glx_float32_t * xcb_glx_get_tex_genfv_data(const xcb_glx_get_tex_genfv_reply_t *)")
fn("int xcb_glx_get_tex_genfv_data_length(const xcb_glx_get_tex_genfv_reply_t *)")
fn("xcb_generic_iterator_t xcb_glx_get_tex_genfv_data_end(const xcb_glx_get_tex_genfv_reply_t *)")

fn("xcb_glx_get_tex_genfv_reply_t * xcb_glx_get_tex_genfv_reply(xcb_connection_t *, xcb_glx_get_tex_genfv_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("int xcb_glx_get_tex_geniv_sizeof(const void *)")
fn("xcb_glx_get_tex_geniv_cookie_t xcb_glx_get_tex_geniv(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t)")
fn("xcb_glx_get_tex_geniv_cookie_t xcb_glx_get_tex_geniv_unchecked(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t)")
# ::Iterator::
fn("int32_t * xcb_glx_get_tex_geniv_data(const xcb_glx_get_tex_geniv_reply_t *)")
fn("int xcb_glx_get_tex_geniv_data_length(const xcb_glx_get_tex_geniv_reply_t *)")
fn("xcb_generic_iterator_t xcb_glx_get_tex_geniv_data_end(const xcb_glx_get_tex_geniv_reply_t *)")

fn("xcb_glx_get_tex_geniv_reply_t * xcb_glx_get_tex_geniv_reply(xcb_connection_t *, xcb_glx_get_tex_geniv_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("int xcb_glx_get_tex_image_sizeof(const void *)")
fn("xcb_glx_get_tex_image_cookie_t xcb_glx_get_tex_image(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, int32_t, uint32_t, uint32_t, uint8_t)")
fn("xcb_glx_get_tex_image_cookie_t xcb_glx_get_tex_image_unchecked(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, int32_t, uint32_t, uint32_t, uint8_t)")
# ::Iterator::
fn("uint8_t * xcb_glx_get_tex_image_data(const xcb_glx_get_tex_image_reply_t *)")
fn("int xcb_glx_get_tex_image_data_length(const xcb_glx_get_tex_image_reply_t *)")
fn("xcb_generic_iterator_t xcb_glx_get_tex_image_data_end(const xcb_glx_get_tex_image_reply_t *)")

fn("xcb_glx_get_tex_image_reply_t * xcb_glx_get_tex_image_reply(xcb_connection_t *, xcb_glx_get_tex_image_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("int xcb_glx_get_tex_parameterfv_sizeof(const void *)")
fn("xcb_glx_get_tex_parameterfv_cookie_t xcb_glx_get_tex_parameterfv(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t)")
fn("xcb_glx_get_tex_parameterfv_cookie_t xcb_glx_get_tex_parameterfv_unchecked(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t)")
# ::Iterator::
fn("xcb_glx_float32_t * xcb_glx_get_tex_parameterfv_data(const xcb_glx_get_tex_parameterfv_reply_t *)")
fn("int xcb_glx_get_tex_parameterfv_data_length(const xcb_glx_get_tex_parameterfv_reply_t *)")
fn("xcb_generic_iterator_t xcb_glx_get_tex_parameterfv_data_end(const xcb_glx_get_tex_parameterfv_reply_t *)")

fn("xcb_glx_get_tex_parameterfv_reply_t * xcb_glx_get_tex_parameterfv_reply(xcb_connection_t *, xcb_glx_get_tex_parameterfv_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("int xcb_glx_get_tex_parameteriv_sizeof(const void *)")
fn("xcb_glx_get_tex_parameteriv_cookie_t xcb_glx_get_tex_parameteriv(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t)")
fn("xcb_glx_get_tex_parameteriv_cookie_t xcb_glx_get_tex_parameteriv_unchecked(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t)")
# ::Iterator::
fn("int32_t * xcb_glx_get_tex_parameteriv_data(const xcb_glx_get_tex_parameteriv_reply_t *)")
fn("int xcb_glx_get_tex_parameteriv_data_length(const xcb_glx_get_tex_parameteriv_reply_t *)")
fn("xcb_generic_iterator_t xcb_glx_get_tex_parameteriv_data_end(const xcb_glx_get_tex_parameteriv_reply_t *)")

fn("xcb_glx_get_tex_parameteriv_reply_t * xcb_glx_get_tex_parameteriv_reply(xcb_connection_t *, xcb_glx_get_tex_parameteriv_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("int xcb_glx_get_tex_level_parameterfv_sizeof(const void *)")
fn("xcb_glx_get_tex_level_parameterfv_cookie_t xcb_glx_get_tex_level_parameterfv(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, int32_t, uint32_t)")
fn("xcb_glx_get_tex_level_parameterfv_cookie_t xcb_glx_get_tex_level_parameterfv_unchecked(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, int32_t, uint32_t)")
# ::Iterator::
fn("xcb_glx_float32_t * xcb_glx_get_tex_level_parameterfv_data(const xcb_glx_get_tex_level_parameterfv_reply_t *)")
fn("int xcb_glx_get_tex_level_parameterfv_data_length(const xcb_glx_get_tex_level_parameterfv_reply_t *)")
fn("xcb_generic_iterator_t xcb_glx_get_tex_level_parameterfv_data_end(const xcb_glx_get_tex_level_parameterfv_reply_t *)")

fn("xcb_glx_get_tex_level_parameterfv_reply_t * xcb_glx_get_tex_level_parameterfv_reply(xcb_connection_t *, xcb_glx_get_tex_level_parameterfv_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("int xcb_glx_get_tex_level_parameteriv_sizeof(const void *)")
fn("xcb_glx_get_tex_level_parameteriv_cookie_t xcb_glx_get_tex_level_parameteriv(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, int32_t, uint32_t)")
fn("xcb_glx_get_tex_level_parameteriv_cookie_t xcb_glx_get_tex_level_parameteriv_unchecked(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, int32_t, uint32_t)")
# ::Iterator::
fn("int32_t * xcb_glx_get_tex_level_parameteriv_data(const xcb_glx_get_tex_level_parameteriv_reply_t *)")
fn("int xcb_glx_get_tex_level_parameteriv_data_length(const xcb_glx_get_tex_level_parameteriv_reply_t *)")
fn("xcb_generic_iterator_t xcb_glx_get_tex_level_parameteriv_data_end(const xcb_glx_get_tex_level_parameteriv_reply_t *)")

fn("xcb_glx_get_tex_level_parameteriv_reply_t * xcb_glx_get_tex_level_parameteriv_reply(xcb_connection_t *, xcb_glx_get_tex_level_parameteriv_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("xcb_glx_is_enabled_cookie_t xcb_glx_is_enabled(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t)")
fn("xcb_glx_is_enabled_cookie_t xcb_glx_is_enabled_unchecked(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t)")
fn("xcb_glx_is_enabled_reply_t * xcb_glx_is_enabled_reply(xcb_connection_t *, xcb_glx_is_enabled_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("xcb_glx_is_list_cookie_t xcb_glx_is_list(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t)")
fn("xcb_glx_is_list_cookie_t xcb_glx_is_list_unchecked(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t)")
fn("xcb_glx_is_list_reply_t * xcb_glx_is_list_reply(xcb_connection_t *, xcb_glx_is_list_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("xcb_void_cookie_t xcb_glx_flush_checked(xcb_connection_t *, xcb_glx_context_tag_t)")
fn("xcb_void_cookie_t xcb_glx_flush(xcb_connection_t *, xcb_glx_context_tag_t)")
fn("int xcb_glx_are_textures_resident_sizeof(const void *)")
fn("xcb_glx_are_textures_resident_cookie_t xcb_glx_are_textures_resident(xcb_connection_t *, xcb_glx_context_tag_t, int32_t, const uint32_t *)")
fn("xcb_glx_are_textures_resident_cookie_t xcb_glx_are_textures_resident_unchecked(xcb_connection_t *, xcb_glx_context_tag_t, int32_t, const uint32_t *)")
# ::Iterator::
fn("uint8_t * xcb_glx_are_textures_resident_data(const xcb_glx_are_textures_resident_reply_t *)")
fn("int xcb_glx_are_textures_resident_data_length(const xcb_glx_are_textures_resident_reply_t *)")
fn("xcb_generic_iterator_t xcb_glx_are_textures_resident_data_end(const xcb_glx_are_textures_resident_reply_t *)")

fn("xcb_glx_are_textures_resident_reply_t * xcb_glx_are_textures_resident_reply(xcb_connection_t *, xcb_glx_are_textures_resident_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("int xcb_glx_delete_textures_sizeof(const void *)")
fn("xcb_void_cookie_t xcb_glx_delete_textures_checked(xcb_connection_t *, xcb_glx_context_tag_t, int32_t, const uint32_t *)")
fn("xcb_void_cookie_t xcb_glx_delete_textures(xcb_connection_t *, xcb_glx_context_tag_t, int32_t, const uint32_t *)")
# ::Iterator::
fn("uint32_t * xcb_glx_delete_textures_textures(const xcb_glx_delete_textures_request_t *)")
fn("int xcb_glx_delete_textures_textures_length(const xcb_glx_delete_textures_request_t *)")
fn("xcb_generic_iterator_t xcb_glx_delete_textures_textures_end(const xcb_glx_delete_textures_request_t *)")

fn("int xcb_glx_gen_textures_sizeof(const void *)")
fn("xcb_glx_gen_textures_cookie_t xcb_glx_gen_textures(xcb_connection_t *, xcb_glx_context_tag_t, int32_t)")
fn("xcb_glx_gen_textures_cookie_t xcb_glx_gen_textures_unchecked(xcb_connection_t *, xcb_glx_context_tag_t, int32_t)")
# ::Iterator::
fn("uint32_t * xcb_glx_gen_textures_data(const xcb_glx_gen_textures_reply_t *)")
fn("int xcb_glx_gen_textures_data_length(const xcb_glx_gen_textures_reply_t *)")
fn("xcb_generic_iterator_t xcb_glx_gen_textures_data_end(const xcb_glx_gen_textures_reply_t *)")

fn("xcb_glx_gen_textures_reply_t * xcb_glx_gen_textures_reply(xcb_connection_t *, xcb_glx_gen_textures_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("xcb_glx_is_texture_cookie_t xcb_glx_is_texture(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t)")
fn("xcb_glx_is_texture_cookie_t xcb_glx_is_texture_unchecked(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t)")
fn("xcb_glx_is_texture_reply_t * xcb_glx_is_texture_reply(xcb_connection_t *, xcb_glx_is_texture_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("int xcb_glx_get_color_table_sizeof(const void *)")
fn("xcb_glx_get_color_table_cookie_t xcb_glx_get_color_table(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t, uint32_t, uint8_t)")
fn("xcb_glx_get_color_table_cookie_t xcb_glx_get_color_table_unchecked(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t, uint32_t, uint8_t)")
# ::Iterator::
fn("uint8_t * xcb_glx_get_color_table_data(const xcb_glx_get_color_table_reply_t *)")
fn("int xcb_glx_get_color_table_data_length(const xcb_glx_get_color_table_reply_t *)")
fn("xcb_generic_iterator_t xcb_glx_get_color_table_data_end(const xcb_glx_get_color_table_reply_t *)")

fn("xcb_glx_get_color_table_reply_t * xcb_glx_get_color_table_reply(xcb_connection_t *, xcb_glx_get_color_table_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("int xcb_glx_get_color_table_parameterfv_sizeof(const void *)")
fn("xcb_glx_get_color_table_parameterfv_cookie_t xcb_glx_get_color_table_parameterfv(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t)")
fn("xcb_glx_get_color_table_parameterfv_cookie_t xcb_glx_get_color_table_parameterfv_unchecked(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t)")
# ::Iterator::
fn("xcb_glx_float32_t * xcb_glx_get_color_table_parameterfv_data(const xcb_glx_get_color_table_parameterfv_reply_t *)")
fn("int xcb_glx_get_color_table_parameterfv_data_length(const xcb_glx_get_color_table_parameterfv_reply_t *)")
fn("xcb_generic_iterator_t xcb_glx_get_color_table_parameterfv_data_end(const xcb_glx_get_color_table_parameterfv_reply_t *)")

fn("xcb_glx_get_color_table_parameterfv_reply_t * xcb_glx_get_color_table_parameterfv_reply(xcb_connection_t *, xcb_glx_get_color_table_parameterfv_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("int xcb_glx_get_color_table_parameteriv_sizeof(const void *)")
fn("xcb_glx_get_color_table_parameteriv_cookie_t xcb_glx_get_color_table_parameteriv(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t)")
fn("xcb_glx_get_color_table_parameteriv_cookie_t xcb_glx_get_color_table_parameteriv_unchecked(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t)")
# ::Iterator::
fn("int32_t * xcb_glx_get_color_table_parameteriv_data(const xcb_glx_get_color_table_parameteriv_reply_t *)")
fn("int xcb_glx_get_color_table_parameteriv_data_length(const xcb_glx_get_color_table_parameteriv_reply_t *)")
fn("xcb_generic_iterator_t xcb_glx_get_color_table_parameteriv_data_end(const xcb_glx_get_color_table_parameteriv_reply_t *)")

fn("xcb_glx_get_color_table_parameteriv_reply_t * xcb_glx_get_color_table_parameteriv_reply(xcb_connection_t *, xcb_glx_get_color_table_parameteriv_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("int xcb_glx_get_convolution_filter_sizeof(const void *)")
fn("xcb_glx_get_convolution_filter_cookie_t xcb_glx_get_convolution_filter(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t, uint32_t, uint8_t)")
fn("xcb_glx_get_convolution_filter_cookie_t xcb_glx_get_convolution_filter_unchecked(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t, uint32_t, uint8_t)")
# ::Iterator::
fn("uint8_t * xcb_glx_get_convolution_filter_data(const xcb_glx_get_convolution_filter_reply_t *)")
fn("int xcb_glx_get_convolution_filter_data_length(const xcb_glx_get_convolution_filter_reply_t *)")
fn("xcb_generic_iterator_t xcb_glx_get_convolution_filter_data_end(const xcb_glx_get_convolution_filter_reply_t *)")

fn("xcb_glx_get_convolution_filter_reply_t * xcb_glx_get_convolution_filter_reply(xcb_connection_t *, xcb_glx_get_convolution_filter_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("int xcb_glx_get_convolution_parameterfv_sizeof(const void *)")
fn("xcb_glx_get_convolution_parameterfv_cookie_t xcb_glx_get_convolution_parameterfv(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t)")
fn("xcb_glx_get_convolution_parameterfv_cookie_t xcb_glx_get_convolution_parameterfv_unchecked(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t)")
# ::Iterator::
fn("xcb_glx_float32_t * xcb_glx_get_convolution_parameterfv_data(const xcb_glx_get_convolution_parameterfv_reply_t *)")
fn("int xcb_glx_get_convolution_parameterfv_data_length(const xcb_glx_get_convolution_parameterfv_reply_t *)")
fn("xcb_generic_iterator_t xcb_glx_get_convolution_parameterfv_data_end(const xcb_glx_get_convolution_parameterfv_reply_t *)")

fn("xcb_glx_get_convolution_parameterfv_reply_t * xcb_glx_get_convolution_parameterfv_reply(xcb_connection_t *, xcb_glx_get_convolution_parameterfv_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("int xcb_glx_get_convolution_parameteriv_sizeof(const void *)")
fn("xcb_glx_get_convolution_parameteriv_cookie_t xcb_glx_get_convolution_parameteriv(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t)")
fn("xcb_glx_get_convolution_parameteriv_cookie_t xcb_glx_get_convolution_parameteriv_unchecked(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t)")
# ::Iterator::
fn("int32_t * xcb_glx_get_convolution_parameteriv_data(const xcb_glx_get_convolution_parameteriv_reply_t *)")
fn("int xcb_glx_get_convolution_parameteriv_data_length(const xcb_glx_get_convolution_parameteriv_reply_t *)")
fn("xcb_generic_iterator_t xcb_glx_get_convolution_parameteriv_data_end(const xcb_glx_get_convolution_parameteriv_reply_t *)")

fn("xcb_glx_get_convolution_parameteriv_reply_t * xcb_glx_get_convolution_parameteriv_reply(xcb_connection_t *, xcb_glx_get_convolution_parameteriv_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("int xcb_glx_get_separable_filter_sizeof(const void *)")
fn("xcb_glx_get_separable_filter_cookie_t xcb_glx_get_separable_filter(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t, uint32_t, uint8_t)")
fn("xcb_glx_get_separable_filter_cookie_t xcb_glx_get_separable_filter_unchecked(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t, uint32_t, uint8_t)")
# ::Iterator::
fn("uint8_t * xcb_glx_get_separable_filter_rows_and_cols(const xcb_glx_get_separable_filter_reply_t *)")
fn("int xcb_glx_get_separable_filter_rows_and_cols_length(const xcb_glx_get_separable_filter_reply_t *)")
fn("xcb_generic_iterator_t xcb_glx_get_separable_filter_rows_and_cols_end(const xcb_glx_get_separable_filter_reply_t *)")

fn("xcb_glx_get_separable_filter_reply_t * xcb_glx_get_separable_filter_reply(xcb_connection_t *, xcb_glx_get_separable_filter_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("int xcb_glx_get_histogram_sizeof(const void *)")
fn("xcb_glx_get_histogram_cookie_t xcb_glx_get_histogram(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t, uint32_t, uint8_t, uint8_t)")
fn("xcb_glx_get_histogram_cookie_t xcb_glx_get_histogram_unchecked(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t, uint32_t, uint8_t, uint8_t)")
# ::Iterator::
fn("uint8_t * xcb_glx_get_histogram_data(const xcb_glx_get_histogram_reply_t *)")
fn("int xcb_glx_get_histogram_data_length(const xcb_glx_get_histogram_reply_t *)")
fn("xcb_generic_iterator_t xcb_glx_get_histogram_data_end(const xcb_glx_get_histogram_reply_t *)")

fn("xcb_glx_get_histogram_reply_t * xcb_glx_get_histogram_reply(xcb_connection_t *, xcb_glx_get_histogram_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("int xcb_glx_get_histogram_parameterfv_sizeof(const void *)")
fn("xcb_glx_get_histogram_parameterfv_cookie_t xcb_glx_get_histogram_parameterfv(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t)")
fn("xcb_glx_get_histogram_parameterfv_cookie_t xcb_glx_get_histogram_parameterfv_unchecked(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t)")
# ::Iterator::
fn("xcb_glx_float32_t * xcb_glx_get_histogram_parameterfv_data(const xcb_glx_get_histogram_parameterfv_reply_t *)")
fn("int xcb_glx_get_histogram_parameterfv_data_length(const xcb_glx_get_histogram_parameterfv_reply_t *)")
fn("xcb_generic_iterator_t xcb_glx_get_histogram_parameterfv_data_end(const xcb_glx_get_histogram_parameterfv_reply_t *)")

fn("xcb_glx_get_histogram_parameterfv_reply_t * xcb_glx_get_histogram_parameterfv_reply(xcb_connection_t *, xcb_glx_get_histogram_parameterfv_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("int xcb_glx_get_histogram_parameteriv_sizeof(const void *)")
fn("xcb_glx_get_histogram_parameteriv_cookie_t xcb_glx_get_histogram_parameteriv(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t)")
fn("xcb_glx_get_histogram_parameteriv_cookie_t xcb_glx_get_histogram_parameteriv_unchecked(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t)")
# ::Iterator::
fn("int32_t * xcb_glx_get_histogram_parameteriv_data(const xcb_glx_get_histogram_parameteriv_reply_t *)")
fn("int xcb_glx_get_histogram_parameteriv_data_length(const xcb_glx_get_histogram_parameteriv_reply_t *)")
fn("xcb_generic_iterator_t xcb_glx_get_histogram_parameteriv_data_end(const xcb_glx_get_histogram_parameteriv_reply_t *)")

fn("xcb_glx_get_histogram_parameteriv_reply_t * xcb_glx_get_histogram_parameteriv_reply(xcb_connection_t *, xcb_glx_get_histogram_parameteriv_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("int xcb_glx_get_minmax_sizeof(const void *)")
fn("xcb_glx_get_minmax_cookie_t xcb_glx_get_minmax(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t, uint32_t, uint8_t, uint8_t)")
fn("xcb_glx_get_minmax_cookie_t xcb_glx_get_minmax_unchecked(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t, uint32_t, uint8_t, uint8_t)")
# ::Iterator::
fn("uint8_t * xcb_glx_get_minmax_data(const xcb_glx_get_minmax_reply_t *)")
fn("int xcb_glx_get_minmax_data_length(const xcb_glx_get_minmax_reply_t *)")
fn("xcb_generic_iterator_t xcb_glx_get_minmax_data_end(const xcb_glx_get_minmax_reply_t *)")

fn("xcb_glx_get_minmax_reply_t * xcb_glx_get_minmax_reply(xcb_connection_t *, xcb_glx_get_minmax_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("int xcb_glx_get_minmax_parameterfv_sizeof(const void *)")
fn("xcb_glx_get_minmax_parameterfv_cookie_t xcb_glx_get_minmax_parameterfv(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t)")
fn("xcb_glx_get_minmax_parameterfv_cookie_t xcb_glx_get_minmax_parameterfv_unchecked(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t)")
# ::Iterator::
fn("xcb_glx_float32_t * xcb_glx_get_minmax_parameterfv_data(const xcb_glx_get_minmax_parameterfv_reply_t *)")
fn("int xcb_glx_get_minmax_parameterfv_data_length(const xcb_glx_get_minmax_parameterfv_reply_t *)")
fn("xcb_generic_iterator_t xcb_glx_get_minmax_parameterfv_data_end(const xcb_glx_get_minmax_parameterfv_reply_t *)")

fn("xcb_glx_get_minmax_parameterfv_reply_t * xcb_glx_get_minmax_parameterfv_reply(xcb_connection_t *, xcb_glx_get_minmax_parameterfv_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("int xcb_glx_get_minmax_parameteriv_sizeof(const void *)")
fn("xcb_glx_get_minmax_parameteriv_cookie_t xcb_glx_get_minmax_parameteriv(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t)")
fn("xcb_glx_get_minmax_parameteriv_cookie_t xcb_glx_get_minmax_parameteriv_unchecked(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t)")
# ::Iterator::
fn("int32_t * xcb_glx_get_minmax_parameteriv_data(const xcb_glx_get_minmax_parameteriv_reply_t *)")
fn("int xcb_glx_get_minmax_parameteriv_data_length(const xcb_glx_get_minmax_parameteriv_reply_t *)")
fn("xcb_generic_iterator_t xcb_glx_get_minmax_parameteriv_data_end(const xcb_glx_get_minmax_parameteriv_reply_t *)")

fn("xcb_glx_get_minmax_parameteriv_reply_t * xcb_glx_get_minmax_parameteriv_reply(xcb_connection_t *, xcb_glx_get_minmax_parameteriv_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("int xcb_glx_get_compressed_tex_image_arb_sizeof(const void *)")
fn("xcb_glx_get_compressed_tex_image_arb_cookie_t xcb_glx_get_compressed_tex_image_arb(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, int32_t)")
fn("xcb_glx_get_compressed_tex_image_arb_cookie_t xcb_glx_get_compressed_tex_image_arb_unchecked(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, int32_t)")
# ::Iterator::
fn("uint8_t * xcb_glx_get_compressed_tex_image_arb_data(const xcb_glx_get_compressed_tex_image_arb_reply_t *)")
fn("int xcb_glx_get_compressed_tex_image_arb_data_length(const xcb_glx_get_compressed_tex_image_arb_reply_t *)")
fn("xcb_generic_iterator_t xcb_glx_get_compressed_tex_image_arb_data_end(const xcb_glx_get_compressed_tex_image_arb_reply_t *)")

fn("xcb_glx_get_compressed_tex_image_arb_reply_t * xcb_glx_get_compressed_tex_image_arb_reply(xcb_connection_t *, xcb_glx_get_compressed_tex_image_arb_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("int xcb_glx_delete_queries_arb_sizeof(const void *)")
fn("xcb_void_cookie_t xcb_glx_delete_queries_arb_checked(xcb_connection_t *, xcb_glx_context_tag_t, int32_t, const uint32_t *)")
fn("xcb_void_cookie_t xcb_glx_delete_queries_arb(xcb_connection_t *, xcb_glx_context_tag_t, int32_t, const uint32_t *)")
# ::Iterator::
fn("uint32_t * xcb_glx_delete_queries_arb_ids(const xcb_glx_delete_queries_arb_request_t *)")
fn("int xcb_glx_delete_queries_arb_ids_length(const xcb_glx_delete_queries_arb_request_t *)")
fn("xcb_generic_iterator_t xcb_glx_delete_queries_arb_ids_end(const xcb_glx_delete_queries_arb_request_t *)")

fn("int xcb_glx_gen_queries_arb_sizeof(const void *)")
fn("xcb_glx_gen_queries_arb_cookie_t xcb_glx_gen_queries_arb(xcb_connection_t *, xcb_glx_context_tag_t, int32_t)")
fn("xcb_glx_gen_queries_arb_cookie_t xcb_glx_gen_queries_arb_unchecked(xcb_connection_t *, xcb_glx_context_tag_t, int32_t)")
# ::Iterator::
fn("uint32_t * xcb_glx_gen_queries_arb_data(const xcb_glx_gen_queries_arb_reply_t *)")
fn("int xcb_glx_gen_queries_arb_data_length(const xcb_glx_gen_queries_arb_reply_t *)")
fn("xcb_generic_iterator_t xcb_glx_gen_queries_arb_data_end(const xcb_glx_gen_queries_arb_reply_t *)")

fn("xcb_glx_gen_queries_arb_reply_t * xcb_glx_gen_queries_arb_reply(xcb_connection_t *, xcb_glx_gen_queries_arb_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("xcb_glx_is_query_arb_cookie_t xcb_glx_is_query_arb(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t)")
fn("xcb_glx_is_query_arb_cookie_t xcb_glx_is_query_arb_unchecked(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t)")
fn("xcb_glx_is_query_arb_reply_t * xcb_glx_is_query_arb_reply(xcb_connection_t *, xcb_glx_is_query_arb_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("int xcb_glx_get_queryiv_arb_sizeof(const void *)")
fn("xcb_glx_get_queryiv_arb_cookie_t xcb_glx_get_queryiv_arb(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t)")
fn("xcb_glx_get_queryiv_arb_cookie_t xcb_glx_get_queryiv_arb_unchecked(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t)")
# ::Iterator::
fn("int32_t * xcb_glx_get_queryiv_arb_data(const xcb_glx_get_queryiv_arb_reply_t *)")
fn("int xcb_glx_get_queryiv_arb_data_length(const xcb_glx_get_queryiv_arb_reply_t *)")
fn("xcb_generic_iterator_t xcb_glx_get_queryiv_arb_data_end(const xcb_glx_get_queryiv_arb_reply_t *)")

fn("xcb_glx_get_queryiv_arb_reply_t * xcb_glx_get_queryiv_arb_reply(xcb_connection_t *, xcb_glx_get_queryiv_arb_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("int xcb_glx_get_query_objectiv_arb_sizeof(const void *)")
fn("xcb_glx_get_query_objectiv_arb_cookie_t xcb_glx_get_query_objectiv_arb(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t)")
fn("xcb_glx_get_query_objectiv_arb_cookie_t xcb_glx_get_query_objectiv_arb_unchecked(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t)")
# ::Iterator::
fn("int32_t * xcb_glx_get_query_objectiv_arb_data(const xcb_glx_get_query_objectiv_arb_reply_t *)")
fn("int xcb_glx_get_query_objectiv_arb_data_length(const xcb_glx_get_query_objectiv_arb_reply_t *)")
fn("xcb_generic_iterator_t xcb_glx_get_query_objectiv_arb_data_end(const xcb_glx_get_query_objectiv_arb_reply_t *)")

fn("xcb_glx_get_query_objectiv_arb_reply_t * xcb_glx_get_query_objectiv_arb_reply(xcb_connection_t *, xcb_glx_get_query_objectiv_arb_cookie_t, xcb_generic_error_t **)"); no_pack()
fn("int xcb_glx_get_query_objectuiv_arb_sizeof(const void *)")
fn("xcb_glx_get_query_objectuiv_arb_cookie_t xcb_glx_get_query_objectuiv_arb(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t)")
fn("xcb_glx_get_query_objectuiv_arb_cookie_t xcb_glx_get_query_objectuiv_arb_unchecked(xcb_connection_t *, xcb_glx_context_tag_t, uint32_t, uint32_t)")
# ::Iterator::
fn("uint32_t * xcb_glx_get_query_objectuiv_arb_data(const xcb_glx_get_query_objectuiv_arb_reply_t *)")
fn("int xcb_glx_get_query_objectuiv_arb_data_length(const xcb_glx_get_query_objectuiv_arb_reply_t *)")
fn("xcb_generic_iterator_t xcb_glx_get_query_objectuiv_arb_data_end(const xcb_glx_get_query_objectuiv_arb_reply_t *)")

fn("xcb_glx_get_query_objectuiv_arb_reply_t * xcb_glx_get_query_objectuiv_arb_reply(xcb_connection_t *, xcb_glx_get_query_objectuiv_arb_cookie_t, xcb_generic_error_t **)"); no_pack()

Generate()
