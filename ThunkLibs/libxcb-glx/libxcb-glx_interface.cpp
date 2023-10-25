#include <common/GeneratorInterface.h>

#include <xcb/glx.h>
#include <xcb/xcbext.h>

template<auto>
struct fex_gen_config {
    unsigned version = 0;
};

template<typename>
struct fex_gen_type {};

template<> struct fex_gen_type<xcb_connection_t> : fexgen::opaque_type {};

void FEX_xcb_glx_init_extension(xcb_connection_t*, xcb_extension_t*);
size_t FEX_usable_size(void*);
void FEX_free_on_host(void*);

template<> struct fex_gen_config<FEX_xcb_glx_init_extension> : fexgen::custom_host_impl {};
template<> struct fex_gen_config<FEX_usable_size> : fexgen::custom_host_impl, fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<FEX_free_on_host> : fexgen::custom_host_impl, fexgen::custom_guest_entrypoint {};

template<> struct fex_gen_config<xcb_glx_pixmap_next> {};
template<> struct fex_gen_config<xcb_glx_pixmap_end> {};
template<> struct fex_gen_config<xcb_glx_context_next> {};
template<> struct fex_gen_config<xcb_glx_context_end> {};
template<> struct fex_gen_config<xcb_glx_pbuffer_next> {};
template<> struct fex_gen_config<xcb_glx_pbuffer_end> {};
template<> struct fex_gen_config<xcb_glx_window_next> {};
template<> struct fex_gen_config<xcb_glx_window_end> {};
template<> struct fex_gen_config<xcb_glx_fbconfig_next> {};
template<> struct fex_gen_config<xcb_glx_fbconfig_end> {};
template<> struct fex_gen_config<xcb_glx_drawable_next> {};
template<> struct fex_gen_config<xcb_glx_drawable_end> {};
template<> struct fex_gen_config<xcb_glx_float32_next> {};
template<> struct fex_gen_config<xcb_glx_float32_end> {};
template<> struct fex_gen_config<xcb_glx_float64_next> {};
template<> struct fex_gen_config<xcb_glx_float64_end> {};
template<> struct fex_gen_config<xcb_glx_bool32_next> {};
template<> struct fex_gen_config<xcb_glx_bool32_end> {};
template<> struct fex_gen_config<xcb_glx_context_tag_next> {};
template<> struct fex_gen_config<xcb_glx_context_tag_end> {};
template<> struct fex_gen_config<xcb_glx_render_sizeof> {};
template<> struct fex_gen_config<xcb_glx_render_checked> {};
template<> struct fex_gen_config<xcb_glx_render> {};
template<> struct fex_gen_config<xcb_glx_render_data> {};
template<> struct fex_gen_config<xcb_glx_render_data_length> {};
template<> struct fex_gen_config<xcb_glx_render_data_end> {};

template<> struct fex_gen_config<xcb_glx_render_large_sizeof> {};
template<> struct fex_gen_config<xcb_glx_render_large_checked> {};
template<> struct fex_gen_config<xcb_glx_render_large> {};
template<> struct fex_gen_config<xcb_glx_render_large_data> {};
template<> struct fex_gen_config<xcb_glx_render_large_data_length> {};
template<> struct fex_gen_config<xcb_glx_render_large_data_end> {};
template<> struct fex_gen_config<xcb_glx_create_context_checked> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_create_context> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_destroy_context_checked> {};
template<> struct fex_gen_config<xcb_glx_destroy_context> {};
template<> struct fex_gen_config<xcb_glx_make_current> {};
template<> struct fex_gen_config<xcb_glx_make_current_unchecked> {};
template<> struct fex_gen_config<xcb_glx_make_current_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_is_direct> {};
template<> struct fex_gen_config<xcb_glx_is_direct_unchecked> {};
template<> struct fex_gen_config<xcb_glx_is_direct_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_query_version> {};
template<> struct fex_gen_config<xcb_glx_query_version_unchecked> {};
template<> struct fex_gen_config<xcb_glx_query_version_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_wait_gl_checked> {};
template<> struct fex_gen_config<xcb_glx_wait_gl> {};
template<> struct fex_gen_config<xcb_glx_wait_x_checked> {};
template<> struct fex_gen_config<xcb_glx_wait_x> {};
template<> struct fex_gen_config<xcb_glx_copy_context_checked> {};
template<> struct fex_gen_config<xcb_glx_copy_context> {};
template<> struct fex_gen_config<xcb_glx_swap_buffers_checked> {};
template<> struct fex_gen_config<xcb_glx_swap_buffers> {};
template<> struct fex_gen_config<xcb_glx_use_x_font_checked> {};
template<> struct fex_gen_config<xcb_glx_use_x_font> {};
template<> struct fex_gen_config<xcb_glx_create_glx_pixmap_checked> {};
template<> struct fex_gen_config<xcb_glx_create_glx_pixmap> {};
template<> struct fex_gen_config<xcb_glx_get_visual_configs_sizeof> {};
template<> struct fex_gen_config<xcb_glx_get_visual_configs> {};
template<> struct fex_gen_config<xcb_glx_get_visual_configs_unchecked> {};
template<> struct fex_gen_config<xcb_glx_get_visual_configs_property_list> {};
template<> struct fex_gen_config<xcb_glx_get_visual_configs_property_list_length> {};
template<> struct fex_gen_config<xcb_glx_get_visual_configs_property_list_end> {};

template<> struct fex_gen_config<xcb_glx_get_visual_configs_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_destroy_glx_pixmap_checked> {};
template<> struct fex_gen_config<xcb_glx_destroy_glx_pixmap> {};
template<> struct fex_gen_config<xcb_glx_vendor_private_sizeof> {};
template<> struct fex_gen_config<xcb_glx_vendor_private_checked> {};
template<> struct fex_gen_config<xcb_glx_vendor_private> {};
template<> struct fex_gen_config<xcb_glx_vendor_private_data> {};
template<> struct fex_gen_config<xcb_glx_vendor_private_data_length> {};
template<> struct fex_gen_config<xcb_glx_vendor_private_data_end> {};

template<> struct fex_gen_config<xcb_glx_vendor_private_with_reply_sizeof> {};
template<> struct fex_gen_config<xcb_glx_vendor_private_with_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_vendor_private_with_reply_unchecked> {};
template<> struct fex_gen_config<xcb_glx_vendor_private_with_reply_data_2> {};
template<> struct fex_gen_config<xcb_glx_vendor_private_with_reply_data_2_length> {};
template<> struct fex_gen_config<xcb_glx_vendor_private_with_reply_data_2_end> {};

template<> struct fex_gen_config<xcb_glx_vendor_private_with_reply_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_query_extensions_string> {};
template<> struct fex_gen_config<xcb_glx_query_extensions_string_unchecked> {};
template<> struct fex_gen_config<xcb_glx_query_extensions_string_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_query_server_string_sizeof> {};
template<> struct fex_gen_config<xcb_glx_query_server_string> {};
template<> struct fex_gen_config<xcb_glx_query_server_string_unchecked> {};
template<> struct fex_gen_config<xcb_glx_query_server_string_string> {};
template<> struct fex_gen_config<xcb_glx_query_server_string_string_length> {};
template<> struct fex_gen_config<xcb_glx_query_server_string_string_end> {};

template<> struct fex_gen_config<xcb_glx_query_server_string_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_client_info_sizeof> {};
template<> struct fex_gen_config<xcb_glx_client_info_checked> {};
template<> struct fex_gen_config<xcb_glx_client_info> {};
template<> struct fex_gen_config<xcb_glx_client_info_string> {};
template<> struct fex_gen_config<xcb_glx_client_info_string_length> {};
template<> struct fex_gen_config<xcb_glx_client_info_string_end> {};

template<> struct fex_gen_config<xcb_glx_get_fb_configs_sizeof> {};
template<> struct fex_gen_config<xcb_glx_get_fb_configs> {};
template<> struct fex_gen_config<xcb_glx_get_fb_configs_unchecked> {};
template<> struct fex_gen_config<xcb_glx_get_fb_configs_property_list> {};
template<> struct fex_gen_config<xcb_glx_get_fb_configs_property_list_length> {};
template<> struct fex_gen_config<xcb_glx_get_fb_configs_property_list_end> {};

template<> struct fex_gen_config<xcb_glx_get_fb_configs_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_create_pixmap_sizeof> {};
template<> struct fex_gen_config<xcb_glx_create_pixmap_checked> {};
template<> struct fex_gen_config<xcb_glx_create_pixmap> {};
template<> struct fex_gen_config<xcb_glx_create_pixmap_attribs> {};
template<> struct fex_gen_config<xcb_glx_create_pixmap_attribs_length> {};
template<> struct fex_gen_config<xcb_glx_create_pixmap_attribs_end> {};

template<> struct fex_gen_config<xcb_glx_destroy_pixmap_checked> {};
template<> struct fex_gen_config<xcb_glx_destroy_pixmap> {};
template<> struct fex_gen_config<xcb_glx_create_new_context_checked> {};
template<> struct fex_gen_config<xcb_glx_create_new_context> {};
template<> struct fex_gen_config<xcb_glx_query_context_sizeof> {};
template<> struct fex_gen_config<xcb_glx_query_context> {};
template<> struct fex_gen_config<xcb_glx_query_context_unchecked> {};
template<> struct fex_gen_config<xcb_glx_query_context_attribs> {};
template<> struct fex_gen_config<xcb_glx_query_context_attribs_length> {};
template<> struct fex_gen_config<xcb_glx_query_context_attribs_end> {};

template<> struct fex_gen_config<xcb_glx_query_context_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_make_context_current> {};
template<> struct fex_gen_config<xcb_glx_make_context_current_unchecked> {};
template<> struct fex_gen_config<xcb_glx_make_context_current_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_create_pbuffer_sizeof> {};
template<> struct fex_gen_config<xcb_glx_create_pbuffer_checked> {};
template<> struct fex_gen_config<xcb_glx_create_pbuffer> {};
template<> struct fex_gen_config<xcb_glx_create_pbuffer_attribs> {};
template<> struct fex_gen_config<xcb_glx_create_pbuffer_attribs_length> {};
template<> struct fex_gen_config<xcb_glx_create_pbuffer_attribs_end> {};

template<> struct fex_gen_config<xcb_glx_destroy_pbuffer_checked> {};
template<> struct fex_gen_config<xcb_glx_destroy_pbuffer> {};
template<> struct fex_gen_config<xcb_glx_get_drawable_attributes_sizeof> {};
template<> struct fex_gen_config<xcb_glx_get_drawable_attributes> {};
template<> struct fex_gen_config<xcb_glx_get_drawable_attributes_unchecked> {};
template<> struct fex_gen_config<xcb_glx_get_drawable_attributes_attribs> {};
template<> struct fex_gen_config<xcb_glx_get_drawable_attributes_attribs_length> {};
template<> struct fex_gen_config<xcb_glx_get_drawable_attributes_attribs_end> {};

template<> struct fex_gen_config<xcb_glx_get_drawable_attributes_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_change_drawable_attributes_sizeof> {};
template<> struct fex_gen_config<xcb_glx_change_drawable_attributes_checked> {};
template<> struct fex_gen_config<xcb_glx_change_drawable_attributes> {};
template<> struct fex_gen_config<xcb_glx_change_drawable_attributes_attribs> {};
template<> struct fex_gen_config<xcb_glx_change_drawable_attributes_attribs_length> {};
template<> struct fex_gen_config<xcb_glx_change_drawable_attributes_attribs_end> {};

template<> struct fex_gen_config<xcb_glx_create_window_sizeof> {};
template<> struct fex_gen_config<xcb_glx_create_window_checked> {};
template<> struct fex_gen_config<xcb_glx_create_window> {};
template<> struct fex_gen_config<xcb_glx_create_window_attribs> {};
template<> struct fex_gen_config<xcb_glx_create_window_attribs_length> {};
template<> struct fex_gen_config<xcb_glx_create_window_attribs_end> {};

template<> struct fex_gen_config<xcb_glx_delete_window_checked> {};
template<> struct fex_gen_config<xcb_glx_delete_window> {};
template<> struct fex_gen_config<xcb_glx_set_client_info_arb_sizeof> {};
template<> struct fex_gen_config<xcb_glx_set_client_info_arb_checked> {};
template<> struct fex_gen_config<xcb_glx_set_client_info_arb> {};
template<> struct fex_gen_config<xcb_glx_set_client_info_arb_gl_versions> {};
template<> struct fex_gen_config<xcb_glx_set_client_info_arb_gl_versions_length> {};
template<> struct fex_gen_config<xcb_glx_set_client_info_arb_gl_versions_end> {};

template<> struct fex_gen_config<xcb_glx_set_client_info_arb_gl_extension_string> {};
template<> struct fex_gen_config<xcb_glx_set_client_info_arb_gl_extension_string_length> {};
template<> struct fex_gen_config<xcb_glx_set_client_info_arb_gl_extension_string_end> {};

template<> struct fex_gen_config<xcb_glx_set_client_info_arb_glx_extension_string> {};
template<> struct fex_gen_config<xcb_glx_set_client_info_arb_glx_extension_string_length> {};
template<> struct fex_gen_config<xcb_glx_set_client_info_arb_glx_extension_string_end> {};

template<> struct fex_gen_config<xcb_glx_create_context_attribs_arb_sizeof> {};
template<> struct fex_gen_config<xcb_glx_create_context_attribs_arb_checked> {};
template<> struct fex_gen_config<xcb_glx_create_context_attribs_arb> {};
template<> struct fex_gen_config<xcb_glx_create_context_attribs_arb_attribs> {};
template<> struct fex_gen_config<xcb_glx_create_context_attribs_arb_attribs_length> {};
template<> struct fex_gen_config<xcb_glx_create_context_attribs_arb_attribs_end> {};

template<> struct fex_gen_config<xcb_glx_set_client_info_2arb_sizeof> {};
template<> struct fex_gen_config<xcb_glx_set_client_info_2arb_checked> {};
template<> struct fex_gen_config<xcb_glx_set_client_info_2arb> {};
template<> struct fex_gen_config<xcb_glx_set_client_info_2arb_gl_versions> {};
template<> struct fex_gen_config<xcb_glx_set_client_info_2arb_gl_versions_length> {};
template<> struct fex_gen_config<xcb_glx_set_client_info_2arb_gl_versions_end> {};

template<> struct fex_gen_config<xcb_glx_set_client_info_2arb_gl_extension_string> {};
template<> struct fex_gen_config<xcb_glx_set_client_info_2arb_gl_extension_string_length> {};
template<> struct fex_gen_config<xcb_glx_set_client_info_2arb_gl_extension_string_end> {};

template<> struct fex_gen_config<xcb_glx_set_client_info_2arb_glx_extension_string> {};
template<> struct fex_gen_config<xcb_glx_set_client_info_2arb_glx_extension_string_length> {};
template<> struct fex_gen_config<xcb_glx_set_client_info_2arb_glx_extension_string_end> {};

template<> struct fex_gen_config<xcb_glx_new_list_checked> {};
template<> struct fex_gen_config<xcb_glx_new_list> {};
template<> struct fex_gen_config<xcb_glx_end_list_checked> {};
template<> struct fex_gen_config<xcb_glx_end_list> {};
template<> struct fex_gen_config<xcb_glx_delete_lists_checked> {};
template<> struct fex_gen_config<xcb_glx_delete_lists> {};
template<> struct fex_gen_config<xcb_glx_gen_lists> {};
template<> struct fex_gen_config<xcb_glx_gen_lists_unchecked> {};
template<> struct fex_gen_config<xcb_glx_gen_lists_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_feedback_buffer_checked> {};
template<> struct fex_gen_config<xcb_glx_feedback_buffer> {};
template<> struct fex_gen_config<xcb_glx_select_buffer_checked> {};
template<> struct fex_gen_config<xcb_glx_select_buffer> {};
template<> struct fex_gen_config<xcb_glx_render_mode_sizeof> {};
template<> struct fex_gen_config<xcb_glx_render_mode> {};
template<> struct fex_gen_config<xcb_glx_render_mode_unchecked> {};
template<> struct fex_gen_config<xcb_glx_render_mode_data> {};
template<> struct fex_gen_config<xcb_glx_render_mode_data_length> {};
template<> struct fex_gen_config<xcb_glx_render_mode_data_end> {};

template<> struct fex_gen_config<xcb_glx_render_mode_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_finish> {};
template<> struct fex_gen_config<xcb_glx_finish_unchecked> {};
template<> struct fex_gen_config<xcb_glx_finish_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_pixel_storef_checked> {};
template<> struct fex_gen_config<xcb_glx_pixel_storef> {};
template<> struct fex_gen_config<xcb_glx_pixel_storei_checked> {};
template<> struct fex_gen_config<xcb_glx_pixel_storei> {};
template<> struct fex_gen_config<xcb_glx_read_pixels_sizeof> {};
template<> struct fex_gen_config<xcb_glx_read_pixels> {};
template<> struct fex_gen_config<xcb_glx_read_pixels_unchecked> {};
template<> struct fex_gen_config<xcb_glx_read_pixels_data> {};
template<> struct fex_gen_config<xcb_glx_read_pixels_data_length> {};
template<> struct fex_gen_config<xcb_glx_read_pixels_data_end> {};

template<> struct fex_gen_config<xcb_glx_read_pixels_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_get_booleanv_sizeof> {};
template<> struct fex_gen_config<xcb_glx_get_booleanv> {};
template<> struct fex_gen_config<xcb_glx_get_booleanv_unchecked> {};
template<> struct fex_gen_config<xcb_glx_get_booleanv_data> {};
template<> struct fex_gen_config<xcb_glx_get_booleanv_data_length> {};
template<> struct fex_gen_config<xcb_glx_get_booleanv_data_end> {};

template<> struct fex_gen_config<xcb_glx_get_booleanv_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_get_clip_plane_sizeof> {};
template<> struct fex_gen_config<xcb_glx_get_clip_plane> {};
template<> struct fex_gen_config<xcb_glx_get_clip_plane_unchecked> {};
template<> struct fex_gen_config<xcb_glx_get_clip_plane_data> {};
template<> struct fex_gen_config<xcb_glx_get_clip_plane_data_length> {};
template<> struct fex_gen_config<xcb_glx_get_clip_plane_data_end> {};

template<> struct fex_gen_config<xcb_glx_get_clip_plane_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_get_doublev_sizeof> {};
template<> struct fex_gen_config<xcb_glx_get_doublev> {};
template<> struct fex_gen_config<xcb_glx_get_doublev_unchecked> {};
template<> struct fex_gen_config<xcb_glx_get_doublev_data> {};
template<> struct fex_gen_config<xcb_glx_get_doublev_data_length> {};
template<> struct fex_gen_config<xcb_glx_get_doublev_data_end> {};

template<> struct fex_gen_config<xcb_glx_get_doublev_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_get_error> {};
template<> struct fex_gen_config<xcb_glx_get_error_unchecked> {};
template<> struct fex_gen_config<xcb_glx_get_error_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_get_floatv_sizeof> {};
template<> struct fex_gen_config<xcb_glx_get_floatv> {};
template<> struct fex_gen_config<xcb_glx_get_floatv_unchecked> {};
template<> struct fex_gen_config<xcb_glx_get_floatv_data> {};
template<> struct fex_gen_config<xcb_glx_get_floatv_data_length> {};
template<> struct fex_gen_config<xcb_glx_get_floatv_data_end> {};

template<> struct fex_gen_config<xcb_glx_get_floatv_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_get_integerv_sizeof> {};
template<> struct fex_gen_config<xcb_glx_get_integerv> {};
template<> struct fex_gen_config<xcb_glx_get_integerv_unchecked> {};
template<> struct fex_gen_config<xcb_glx_get_integerv_data> {};
template<> struct fex_gen_config<xcb_glx_get_integerv_data_length> {};
template<> struct fex_gen_config<xcb_glx_get_integerv_data_end> {};

template<> struct fex_gen_config<xcb_glx_get_integerv_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_get_lightfv_sizeof> {};
template<> struct fex_gen_config<xcb_glx_get_lightfv> {};
template<> struct fex_gen_config<xcb_glx_get_lightfv_unchecked> {};
template<> struct fex_gen_config<xcb_glx_get_lightfv_data> {};
template<> struct fex_gen_config<xcb_glx_get_lightfv_data_length> {};
template<> struct fex_gen_config<xcb_glx_get_lightfv_data_end> {};

template<> struct fex_gen_config<xcb_glx_get_lightfv_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_get_lightiv_sizeof> {};
template<> struct fex_gen_config<xcb_glx_get_lightiv> {};
template<> struct fex_gen_config<xcb_glx_get_lightiv_unchecked> {};
template<> struct fex_gen_config<xcb_glx_get_lightiv_data> {};
template<> struct fex_gen_config<xcb_glx_get_lightiv_data_length> {};
template<> struct fex_gen_config<xcb_glx_get_lightiv_data_end> {};

template<> struct fex_gen_config<xcb_glx_get_lightiv_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_get_mapdv_sizeof> {};
template<> struct fex_gen_config<xcb_glx_get_mapdv> {};
template<> struct fex_gen_config<xcb_glx_get_mapdv_unchecked> {};
template<> struct fex_gen_config<xcb_glx_get_mapdv_data> {};
template<> struct fex_gen_config<xcb_glx_get_mapdv_data_length> {};
template<> struct fex_gen_config<xcb_glx_get_mapdv_data_end> {};

template<> struct fex_gen_config<xcb_glx_get_mapdv_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_get_mapfv_sizeof> {};
template<> struct fex_gen_config<xcb_glx_get_mapfv> {};
template<> struct fex_gen_config<xcb_glx_get_mapfv_unchecked> {};
template<> struct fex_gen_config<xcb_glx_get_mapfv_data> {};
template<> struct fex_gen_config<xcb_glx_get_mapfv_data_length> {};
template<> struct fex_gen_config<xcb_glx_get_mapfv_data_end> {};

template<> struct fex_gen_config<xcb_glx_get_mapfv_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_get_mapiv_sizeof> {};
template<> struct fex_gen_config<xcb_glx_get_mapiv> {};
template<> struct fex_gen_config<xcb_glx_get_mapiv_unchecked> {};
template<> struct fex_gen_config<xcb_glx_get_mapiv_data> {};
template<> struct fex_gen_config<xcb_glx_get_mapiv_data_length> {};
template<> struct fex_gen_config<xcb_glx_get_mapiv_data_end> {};

template<> struct fex_gen_config<xcb_glx_get_mapiv_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_get_materialfv_sizeof> {};
template<> struct fex_gen_config<xcb_glx_get_materialfv> {};
template<> struct fex_gen_config<xcb_glx_get_materialfv_unchecked> {};
template<> struct fex_gen_config<xcb_glx_get_materialfv_data> {};
template<> struct fex_gen_config<xcb_glx_get_materialfv_data_length> {};
template<> struct fex_gen_config<xcb_glx_get_materialfv_data_end> {};

template<> struct fex_gen_config<xcb_glx_get_materialfv_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_get_materialiv_sizeof> {};
template<> struct fex_gen_config<xcb_glx_get_materialiv> {};
template<> struct fex_gen_config<xcb_glx_get_materialiv_unchecked> {};
template<> struct fex_gen_config<xcb_glx_get_materialiv_data> {};
template<> struct fex_gen_config<xcb_glx_get_materialiv_data_length> {};
template<> struct fex_gen_config<xcb_glx_get_materialiv_data_end> {};

template<> struct fex_gen_config<xcb_glx_get_materialiv_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_get_pixel_mapfv_sizeof> {};
template<> struct fex_gen_config<xcb_glx_get_pixel_mapfv> {};
template<> struct fex_gen_config<xcb_glx_get_pixel_mapfv_unchecked> {};
template<> struct fex_gen_config<xcb_glx_get_pixel_mapfv_data> {};
template<> struct fex_gen_config<xcb_glx_get_pixel_mapfv_data_length> {};
template<> struct fex_gen_config<xcb_glx_get_pixel_mapfv_data_end> {};

template<> struct fex_gen_config<xcb_glx_get_pixel_mapfv_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_get_pixel_mapuiv_sizeof> {};
template<> struct fex_gen_config<xcb_glx_get_pixel_mapuiv> {};
template<> struct fex_gen_config<xcb_glx_get_pixel_mapuiv_unchecked> {};
template<> struct fex_gen_config<xcb_glx_get_pixel_mapuiv_data> {};
template<> struct fex_gen_config<xcb_glx_get_pixel_mapuiv_data_length> {};
template<> struct fex_gen_config<xcb_glx_get_pixel_mapuiv_data_end> {};

template<> struct fex_gen_config<xcb_glx_get_pixel_mapuiv_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_get_pixel_mapusv_sizeof> {};
template<> struct fex_gen_config<xcb_glx_get_pixel_mapusv> {};
template<> struct fex_gen_config<xcb_glx_get_pixel_mapusv_unchecked> {};
template<> struct fex_gen_config<xcb_glx_get_pixel_mapusv_data> {};
template<> struct fex_gen_config<xcb_glx_get_pixel_mapusv_data_length> {};
template<> struct fex_gen_config<xcb_glx_get_pixel_mapusv_data_end> {};

template<> struct fex_gen_config<xcb_glx_get_pixel_mapusv_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_get_polygon_stipple_sizeof> {};
template<> struct fex_gen_config<xcb_glx_get_polygon_stipple> {};
template<> struct fex_gen_config<xcb_glx_get_polygon_stipple_unchecked> {};
template<> struct fex_gen_config<xcb_glx_get_polygon_stipple_data> {};
template<> struct fex_gen_config<xcb_glx_get_polygon_stipple_data_length> {};
template<> struct fex_gen_config<xcb_glx_get_polygon_stipple_data_end> {};

template<> struct fex_gen_config<xcb_glx_get_polygon_stipple_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_get_string_sizeof> {};
template<> struct fex_gen_config<xcb_glx_get_string> {};
template<> struct fex_gen_config<xcb_glx_get_string_unchecked> {};
template<> struct fex_gen_config<xcb_glx_get_string_string> {};
template<> struct fex_gen_config<xcb_glx_get_string_string_length> {};
template<> struct fex_gen_config<xcb_glx_get_string_string_end> {};

template<> struct fex_gen_config<xcb_glx_get_string_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_get_tex_envfv_sizeof> {};
template<> struct fex_gen_config<xcb_glx_get_tex_envfv> {};
template<> struct fex_gen_config<xcb_glx_get_tex_envfv_unchecked> {};
template<> struct fex_gen_config<xcb_glx_get_tex_envfv_data> {};
template<> struct fex_gen_config<xcb_glx_get_tex_envfv_data_length> {};
template<> struct fex_gen_config<xcb_glx_get_tex_envfv_data_end> {};
template<> struct fex_gen_config<xcb_glx_get_tex_envfv_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_get_tex_enviv_sizeof> {};
template<> struct fex_gen_config<xcb_glx_get_tex_enviv> {};
template<> struct fex_gen_config<xcb_glx_get_tex_enviv_unchecked> {};
template<> struct fex_gen_config<xcb_glx_get_tex_enviv_data> {};
template<> struct fex_gen_config<xcb_glx_get_tex_enviv_data_length> {};
template<> struct fex_gen_config<xcb_glx_get_tex_enviv_data_end> {};

template<> struct fex_gen_config<xcb_glx_get_tex_enviv_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_get_tex_gendv_sizeof> {};
template<> struct fex_gen_config<xcb_glx_get_tex_gendv> {};
template<> struct fex_gen_config<xcb_glx_get_tex_gendv_unchecked> {};
template<> struct fex_gen_config<xcb_glx_get_tex_gendv_data> {};
template<> struct fex_gen_config<xcb_glx_get_tex_gendv_data_length> {};
template<> struct fex_gen_config<xcb_glx_get_tex_gendv_data_end> {};

template<> struct fex_gen_config<xcb_glx_get_tex_gendv_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_get_tex_genfv_sizeof> {};
template<> struct fex_gen_config<xcb_glx_get_tex_genfv> {};
template<> struct fex_gen_config<xcb_glx_get_tex_genfv_unchecked> {};
template<> struct fex_gen_config<xcb_glx_get_tex_genfv_data> {};
template<> struct fex_gen_config<xcb_glx_get_tex_genfv_data_length> {};
template<> struct fex_gen_config<xcb_glx_get_tex_genfv_data_end> {};

template<> struct fex_gen_config<xcb_glx_get_tex_genfv_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_get_tex_geniv_sizeof> {};
template<> struct fex_gen_config<xcb_glx_get_tex_geniv> {};
template<> struct fex_gen_config<xcb_glx_get_tex_geniv_unchecked> {};
template<> struct fex_gen_config<xcb_glx_get_tex_geniv_data> {};
template<> struct fex_gen_config<xcb_glx_get_tex_geniv_data_length> {};
template<> struct fex_gen_config<xcb_glx_get_tex_geniv_data_end> {};

template<> struct fex_gen_config<xcb_glx_get_tex_geniv_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_get_tex_image_sizeof> {};
template<> struct fex_gen_config<xcb_glx_get_tex_image> {};
template<> struct fex_gen_config<xcb_glx_get_tex_image_unchecked> {};
template<> struct fex_gen_config<xcb_glx_get_tex_image_data> {};
template<> struct fex_gen_config<xcb_glx_get_tex_image_data_length> {};
template<> struct fex_gen_config<xcb_glx_get_tex_image_data_end> {};

template<> struct fex_gen_config<xcb_glx_get_tex_image_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_get_tex_parameterfv_sizeof> {};
template<> struct fex_gen_config<xcb_glx_get_tex_parameterfv> {};
template<> struct fex_gen_config<xcb_glx_get_tex_parameterfv_unchecked> {};
template<> struct fex_gen_config<xcb_glx_get_tex_parameterfv_data> {};
template<> struct fex_gen_config<xcb_glx_get_tex_parameterfv_data_length> {};
template<> struct fex_gen_config<xcb_glx_get_tex_parameterfv_data_end> {};

template<> struct fex_gen_config<xcb_glx_get_tex_parameterfv_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_get_tex_parameteriv_sizeof> {};
template<> struct fex_gen_config<xcb_glx_get_tex_parameteriv> {};
template<> struct fex_gen_config<xcb_glx_get_tex_parameteriv_unchecked> {};
template<> struct fex_gen_config<xcb_glx_get_tex_parameteriv_data> {};
template<> struct fex_gen_config<xcb_glx_get_tex_parameteriv_data_length> {};
template<> struct fex_gen_config<xcb_glx_get_tex_parameteriv_data_end> {};

template<> struct fex_gen_config<xcb_glx_get_tex_parameteriv_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_get_tex_level_parameterfv_sizeof> {};
template<> struct fex_gen_config<xcb_glx_get_tex_level_parameterfv> {};
template<> struct fex_gen_config<xcb_glx_get_tex_level_parameterfv_unchecked> {};
template<> struct fex_gen_config<xcb_glx_get_tex_level_parameterfv_data> {};
template<> struct fex_gen_config<xcb_glx_get_tex_level_parameterfv_data_length> {};
template<> struct fex_gen_config<xcb_glx_get_tex_level_parameterfv_data_end> {};

template<> struct fex_gen_config<xcb_glx_get_tex_level_parameterfv_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_get_tex_level_parameteriv_sizeof> {};
template<> struct fex_gen_config<xcb_glx_get_tex_level_parameteriv> {};
template<> struct fex_gen_config<xcb_glx_get_tex_level_parameteriv_unchecked> {};
template<> struct fex_gen_config<xcb_glx_get_tex_level_parameteriv_data> {};
template<> struct fex_gen_config<xcb_glx_get_tex_level_parameteriv_data_length> {};
template<> struct fex_gen_config<xcb_glx_get_tex_level_parameteriv_data_end> {};

template<> struct fex_gen_config<xcb_glx_get_tex_level_parameteriv_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_is_enabled> {};
template<> struct fex_gen_config<xcb_glx_is_enabled_unchecked> {};
template<> struct fex_gen_config<xcb_glx_is_enabled_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_is_list> {};
template<> struct fex_gen_config<xcb_glx_is_list_unchecked> {};
template<> struct fex_gen_config<xcb_glx_is_list_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_flush_checked> {};
template<> struct fex_gen_config<xcb_glx_flush> {};
template<> struct fex_gen_config<xcb_glx_are_textures_resident_sizeof> {};
template<> struct fex_gen_config<xcb_glx_are_textures_resident> {};
template<> struct fex_gen_config<xcb_glx_are_textures_resident_unchecked> {};
template<> struct fex_gen_config<xcb_glx_are_textures_resident_data> {};
template<> struct fex_gen_config<xcb_glx_are_textures_resident_data_length> {};
template<> struct fex_gen_config<xcb_glx_are_textures_resident_data_end> {};

template<> struct fex_gen_config<xcb_glx_are_textures_resident_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_delete_textures_sizeof> {};
template<> struct fex_gen_config<xcb_glx_delete_textures_checked> {};
template<> struct fex_gen_config<xcb_glx_delete_textures> {};
template<> struct fex_gen_config<xcb_glx_delete_textures_textures> {};
template<> struct fex_gen_config<xcb_glx_delete_textures_textures_length> {};
template<> struct fex_gen_config<xcb_glx_delete_textures_textures_end> {};

template<> struct fex_gen_config<xcb_glx_gen_textures_sizeof> {};
template<> struct fex_gen_config<xcb_glx_gen_textures> {};
template<> struct fex_gen_config<xcb_glx_gen_textures_unchecked> {};
template<> struct fex_gen_config<xcb_glx_gen_textures_data> {};
template<> struct fex_gen_config<xcb_glx_gen_textures_data_length> {};
template<> struct fex_gen_config<xcb_glx_gen_textures_data_end> {};

template<> struct fex_gen_config<xcb_glx_gen_textures_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_is_texture> {};
template<> struct fex_gen_config<xcb_glx_is_texture_unchecked> {};
template<> struct fex_gen_config<xcb_glx_is_texture_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_get_color_table_sizeof> {};
template<> struct fex_gen_config<xcb_glx_get_color_table> {};
template<> struct fex_gen_config<xcb_glx_get_color_table_unchecked> {};
template<> struct fex_gen_config<xcb_glx_get_color_table_data> {};
template<> struct fex_gen_config<xcb_glx_get_color_table_data_length> {};
template<> struct fex_gen_config<xcb_glx_get_color_table_data_end> {};

template<> struct fex_gen_config<xcb_glx_get_color_table_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_get_color_table_parameterfv_sizeof> {};
template<> struct fex_gen_config<xcb_glx_get_color_table_parameterfv> {};
template<> struct fex_gen_config<xcb_glx_get_color_table_parameterfv_unchecked> {};
template<> struct fex_gen_config<xcb_glx_get_color_table_parameterfv_data> {};
template<> struct fex_gen_config<xcb_glx_get_color_table_parameterfv_data_length> {};
template<> struct fex_gen_config<xcb_glx_get_color_table_parameterfv_data_end> {};

template<> struct fex_gen_config<xcb_glx_get_color_table_parameterfv_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_get_color_table_parameteriv_sizeof> {};
template<> struct fex_gen_config<xcb_glx_get_color_table_parameteriv> {};
template<> struct fex_gen_config<xcb_glx_get_color_table_parameteriv_unchecked> {};
template<> struct fex_gen_config<xcb_glx_get_color_table_parameteriv_data> {};
template<> struct fex_gen_config<xcb_glx_get_color_table_parameteriv_data_length> {};
template<> struct fex_gen_config<xcb_glx_get_color_table_parameteriv_data_end> {};

template<> struct fex_gen_config<xcb_glx_get_color_table_parameteriv_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_get_convolution_filter_sizeof> {};
template<> struct fex_gen_config<xcb_glx_get_convolution_filter> {};
template<> struct fex_gen_config<xcb_glx_get_convolution_filter_unchecked> {};
template<> struct fex_gen_config<xcb_glx_get_convolution_filter_data> {};
template<> struct fex_gen_config<xcb_glx_get_convolution_filter_data_length> {};
template<> struct fex_gen_config<xcb_glx_get_convolution_filter_data_end> {};

template<> struct fex_gen_config<xcb_glx_get_convolution_filter_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_get_convolution_parameterfv_sizeof> {};
template<> struct fex_gen_config<xcb_glx_get_convolution_parameterfv> {};
template<> struct fex_gen_config<xcb_glx_get_convolution_parameterfv_unchecked> {};
template<> struct fex_gen_config<xcb_glx_get_convolution_parameterfv_data> {};
template<> struct fex_gen_config<xcb_glx_get_convolution_parameterfv_data_length> {};
template<> struct fex_gen_config<xcb_glx_get_convolution_parameterfv_data_end> {};

template<> struct fex_gen_config<xcb_glx_get_convolution_parameterfv_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_get_convolution_parameteriv_sizeof> {};
template<> struct fex_gen_config<xcb_glx_get_convolution_parameteriv> {};
template<> struct fex_gen_config<xcb_glx_get_convolution_parameteriv_unchecked> {};
template<> struct fex_gen_config<xcb_glx_get_convolution_parameteriv_data> {};
template<> struct fex_gen_config<xcb_glx_get_convolution_parameteriv_data_length> {};
template<> struct fex_gen_config<xcb_glx_get_convolution_parameteriv_data_end> {};

template<> struct fex_gen_config<xcb_glx_get_convolution_parameteriv_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_get_separable_filter_sizeof> {};
template<> struct fex_gen_config<xcb_glx_get_separable_filter> {};
template<> struct fex_gen_config<xcb_glx_get_separable_filter_unchecked> {};
template<> struct fex_gen_config<xcb_glx_get_separable_filter_rows_and_cols> {};
template<> struct fex_gen_config<xcb_glx_get_separable_filter_rows_and_cols_length> {};
template<> struct fex_gen_config<xcb_glx_get_separable_filter_rows_and_cols_end> {};

template<> struct fex_gen_config<xcb_glx_get_separable_filter_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_get_histogram_sizeof> {};
template<> struct fex_gen_config<xcb_glx_get_histogram> {};
template<> struct fex_gen_config<xcb_glx_get_histogram_unchecked> {};
template<> struct fex_gen_config<xcb_glx_get_histogram_data> {};
template<> struct fex_gen_config<xcb_glx_get_histogram_data_length> {};
template<> struct fex_gen_config<xcb_glx_get_histogram_data_end> {};

template<> struct fex_gen_config<xcb_glx_get_histogram_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_get_histogram_parameterfv_sizeof> {};
template<> struct fex_gen_config<xcb_glx_get_histogram_parameterfv> {};
template<> struct fex_gen_config<xcb_glx_get_histogram_parameterfv_unchecked> {};
template<> struct fex_gen_config<xcb_glx_get_histogram_parameterfv_data> {};
template<> struct fex_gen_config<xcb_glx_get_histogram_parameterfv_data_length> {};
template<> struct fex_gen_config<xcb_glx_get_histogram_parameterfv_data_end> {};

template<> struct fex_gen_config<xcb_glx_get_histogram_parameterfv_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_get_histogram_parameteriv_sizeof> {};
template<> struct fex_gen_config<xcb_glx_get_histogram_parameteriv> {};
template<> struct fex_gen_config<xcb_glx_get_histogram_parameteriv_unchecked> {};
template<> struct fex_gen_config<xcb_glx_get_histogram_parameteriv_data> {};
template<> struct fex_gen_config<xcb_glx_get_histogram_parameteriv_data_length> {};
template<> struct fex_gen_config<xcb_glx_get_histogram_parameteriv_data_end> {};

template<> struct fex_gen_config<xcb_glx_get_histogram_parameteriv_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_get_minmax_sizeof> {};
template<> struct fex_gen_config<xcb_glx_get_minmax> {};
template<> struct fex_gen_config<xcb_glx_get_minmax_unchecked> {};
template<> struct fex_gen_config<xcb_glx_get_minmax_data> {};
template<> struct fex_gen_config<xcb_glx_get_minmax_data_length> {};
template<> struct fex_gen_config<xcb_glx_get_minmax_data_end> {};

template<> struct fex_gen_config<xcb_glx_get_minmax_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_get_minmax_parameterfv_sizeof> {};
template<> struct fex_gen_config<xcb_glx_get_minmax_parameterfv> {};
template<> struct fex_gen_config<xcb_glx_get_minmax_parameterfv_unchecked> {};
template<> struct fex_gen_config<xcb_glx_get_minmax_parameterfv_data> {};
template<> struct fex_gen_config<xcb_glx_get_minmax_parameterfv_data_length> {};
template<> struct fex_gen_config<xcb_glx_get_minmax_parameterfv_data_end> {};

template<> struct fex_gen_config<xcb_glx_get_minmax_parameterfv_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_get_minmax_parameteriv_sizeof> {};
template<> struct fex_gen_config<xcb_glx_get_minmax_parameteriv> {};
template<> struct fex_gen_config<xcb_glx_get_minmax_parameteriv_unchecked> {};
template<> struct fex_gen_config<xcb_glx_get_minmax_parameteriv_data> {};
template<> struct fex_gen_config<xcb_glx_get_minmax_parameteriv_data_length> {};
template<> struct fex_gen_config<xcb_glx_get_minmax_parameteriv_data_end> {};

template<> struct fex_gen_config<xcb_glx_get_minmax_parameteriv_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_get_compressed_tex_image_arb_sizeof> {};
template<> struct fex_gen_config<xcb_glx_get_compressed_tex_image_arb> {};
template<> struct fex_gen_config<xcb_glx_get_compressed_tex_image_arb_unchecked> {};
template<> struct fex_gen_config<xcb_glx_get_compressed_tex_image_arb_data> {};
template<> struct fex_gen_config<xcb_glx_get_compressed_tex_image_arb_data_length> {};
template<> struct fex_gen_config<xcb_glx_get_compressed_tex_image_arb_data_end> {};

template<> struct fex_gen_config<xcb_glx_get_compressed_tex_image_arb_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_delete_queries_arb_sizeof> {};
template<> struct fex_gen_config<xcb_glx_delete_queries_arb_checked> {};
template<> struct fex_gen_config<xcb_glx_delete_queries_arb> {};
template<> struct fex_gen_config<xcb_glx_delete_queries_arb_ids> {};
template<> struct fex_gen_config<xcb_glx_delete_queries_arb_ids_length> {};
template<> struct fex_gen_config<xcb_glx_delete_queries_arb_ids_end> {};

template<> struct fex_gen_config<xcb_glx_gen_queries_arb_sizeof> {};
template<> struct fex_gen_config<xcb_glx_gen_queries_arb> {};
template<> struct fex_gen_config<xcb_glx_gen_queries_arb_unchecked> {};
template<> struct fex_gen_config<xcb_glx_gen_queries_arb_data> {};
template<> struct fex_gen_config<xcb_glx_gen_queries_arb_data_length> {};
template<> struct fex_gen_config<xcb_glx_gen_queries_arb_data_end> {};

template<> struct fex_gen_config<xcb_glx_gen_queries_arb_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_is_query_arb> {};
template<> struct fex_gen_config<xcb_glx_is_query_arb_unchecked> {};
template<> struct fex_gen_config<xcb_glx_is_query_arb_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_get_queryiv_arb_sizeof> {};
template<> struct fex_gen_config<xcb_glx_get_queryiv_arb> {};
template<> struct fex_gen_config<xcb_glx_get_queryiv_arb_unchecked> {};
template<> struct fex_gen_config<xcb_glx_get_queryiv_arb_data> {};
template<> struct fex_gen_config<xcb_glx_get_queryiv_arb_data_length> {};
template<> struct fex_gen_config<xcb_glx_get_queryiv_arb_data_end> {};

template<> struct fex_gen_config<xcb_glx_get_queryiv_arb_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_get_query_objectiv_arb_sizeof> {};
template<> struct fex_gen_config<xcb_glx_get_query_objectiv_arb> {};
template<> struct fex_gen_config<xcb_glx_get_query_objectiv_arb_unchecked> {};
template<> struct fex_gen_config<xcb_glx_get_query_objectiv_arb_data> {};
template<> struct fex_gen_config<xcb_glx_get_query_objectiv_arb_data_length> {};
template<> struct fex_gen_config<xcb_glx_get_query_objectiv_arb_data_end> {};

template<> struct fex_gen_config<xcb_glx_get_query_objectiv_arb_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_glx_get_query_objectuiv_arb_sizeof> {};
template<> struct fex_gen_config<xcb_glx_get_query_objectuiv_arb> {};
template<> struct fex_gen_config<xcb_glx_get_query_objectuiv_arb_unchecked> {};
template<> struct fex_gen_config<xcb_glx_get_query_objectuiv_arb_data> {};
template<> struct fex_gen_config<xcb_glx_get_query_objectuiv_arb_data_length> {};
template<> struct fex_gen_config<xcb_glx_get_query_objectuiv_arb_data_end> {};

template<> struct fex_gen_config<xcb_glx_get_query_objectuiv_arb_reply> : fexgen::custom_guest_entrypoint {};
