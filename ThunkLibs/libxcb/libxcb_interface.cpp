#include <common/GeneratorInterface.h>

#include <xcb/xcb.h>
#include <xcb/xcbext.h>

template<auto>
struct fex_gen_config {
    unsigned version = 1;
};

template<typename>
struct fex_gen_type {};

template<> struct fex_gen_type<xcb_connection_t> : fexgen::opaque_type {};
template<> struct fex_gen_type<xcb_special_event> : fexgen::opaque_type {};

// Union type with consistent data layout across host/x86/x86-64
template<> struct fex_gen_type<xcb_client_message_data_t> : fexgen::assume_compatible_data_layout {};

void FEX_xcb_init_extension(xcb_connection_t*, xcb_extension_t*);
size_t FEX_usable_size(void*);
void FEX_free_on_host(void*);

template<> struct fex_gen_config<FEX_xcb_init_extension> : fexgen::custom_host_impl {};
template<> struct fex_gen_config<FEX_usable_size> : fexgen::custom_host_impl, fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<FEX_free_on_host> : fexgen::custom_host_impl, fexgen::custom_guest_entrypoint {};

template<> struct fex_gen_config<xcb_flush> {};
template<> struct fex_gen_config<xcb_get_maximum_request_length> {};
template<> struct fex_gen_config<xcb_prefetch_maximum_request_length> {};
template<> struct fex_gen_config<xcb_wait_for_event> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_poll_for_event> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_poll_for_queued_event> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_poll_for_special_event> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_wait_for_special_event> : fexgen::custom_guest_entrypoint {};

template<> struct fex_gen_config<xcb_register_for_special_xge> {};
template<> struct fex_gen_config<xcb_unregister_for_special_event> {};

template<> struct fex_gen_config<xcb_request_check> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_discard_reply> {};
template<> struct fex_gen_config<xcb_discard_reply64> {};
template<> struct fex_gen_config<xcb_get_extension_data> {};
template<> struct fex_gen_config<xcb_prefetch_extension_data> {};

template<> struct fex_gen_config<xcb_get_setup> {};
template<> struct fex_gen_config<xcb_get_file_descriptor> {};
template<> struct fex_gen_config<xcb_connection_has_error> {};
template<> struct fex_gen_config<xcb_connect_to_fd> : fexgen::custom_guest_entrypoint {};

template<> struct fex_gen_config<xcb_disconnect> : fexgen::custom_guest_entrypoint {};

template<> struct fex_gen_config<xcb_parse_display> : fexgen::custom_guest_entrypoint {};

template<> struct fex_gen_config<xcb_connect> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_connect_to_display_with_auth_info> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_generate_id> {};

#if XCB_VERSION_MAJOR >= 1 && XCB_VERSION_MINOR >= 14 && XCB_VERSION_PATCH >= 0
template<> struct fex_gen_config<xcb_total_read> {};
template<> struct fex_gen_config<xcb_total_written> {};
#endif

template<> struct fex_gen_config<xcb_send_request> {};
template<> struct fex_gen_config<xcb_send_request_with_fds> {};
template<> struct fex_gen_config<xcb_send_request64> {};
template<> struct fex_gen_config<xcb_send_request_with_fds64> {};
template<> struct fex_gen_config<xcb_send_fd> {};
template<> struct fex_gen_config<xcb_take_socket> {};

template<> struct fex_gen_config<xcb_writev> {};
template<> struct fex_gen_config<xcb_wait_for_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_wait_for_reply64> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_poll_for_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_poll_for_reply64> : fexgen::custom_guest_entrypoint {};

template<> struct fex_gen_config<xcb_get_reply_fds> {};
template<> struct fex_gen_config<xcb_popcount> {};
template<> struct fex_gen_config<xcb_sumof> {};

template<> struct fex_gen_config<xcb_char2b_next> {};
template<> struct fex_gen_config<xcb_char2b_end> {};
template<> struct fex_gen_config<xcb_window_next> {};
template<> struct fex_gen_config<xcb_window_end> {};
template<> struct fex_gen_config<xcb_pixmap_next> {};
template<> struct fex_gen_config<xcb_pixmap_end> {};
template<> struct fex_gen_config<xcb_cursor_next> {};
template<> struct fex_gen_config<xcb_cursor_end> {};
template<> struct fex_gen_config<xcb_font_next> {};
template<> struct fex_gen_config<xcb_font_end> {};
template<> struct fex_gen_config<xcb_gcontext_next> {};
template<> struct fex_gen_config<xcb_gcontext_end> {};
template<> struct fex_gen_config<xcb_colormap_next> {};
template<> struct fex_gen_config<xcb_colormap_end> {};
template<> struct fex_gen_config<xcb_atom_next> {};
template<> struct fex_gen_config<xcb_atom_end> {};
template<> struct fex_gen_config<xcb_drawable_next> {};
template<> struct fex_gen_config<xcb_drawable_end> {};
template<> struct fex_gen_config<xcb_fontable_next> {};
template<> struct fex_gen_config<xcb_fontable_end> {};
template<> struct fex_gen_config<xcb_bool32_next> {};
template<> struct fex_gen_config<xcb_bool32_end> {};
template<> struct fex_gen_config<xcb_visualid_next> {};
template<> struct fex_gen_config<xcb_visualid_end> {};
template<> struct fex_gen_config<xcb_timestamp_next> {};
template<> struct fex_gen_config<xcb_timestamp_end> {};
template<> struct fex_gen_config<xcb_keysym_next> {};
template<> struct fex_gen_config<xcb_keysym_end> {};
template<> struct fex_gen_config<xcb_keycode_next> {};
template<> struct fex_gen_config<xcb_keycode_end> {};
template<> struct fex_gen_config<xcb_keycode32_next> {};
template<> struct fex_gen_config<xcb_keycode32_end> {};
template<> struct fex_gen_config<xcb_button_next> {};
template<> struct fex_gen_config<xcb_button_end> {};
template<> struct fex_gen_config<xcb_point_next> {};
template<> struct fex_gen_config<xcb_point_end> {};
template<> struct fex_gen_config<xcb_rectangle_next> {};
template<> struct fex_gen_config<xcb_rectangle_end> {};
template<> struct fex_gen_config<xcb_arc_next> {};
template<> struct fex_gen_config<xcb_arc_end> {};
template<> struct fex_gen_config<xcb_format_next> {};
template<> struct fex_gen_config<xcb_format_end> {};
template<> struct fex_gen_config<xcb_visualtype_next> {};
template<> struct fex_gen_config<xcb_visualtype_end> {};
template<> struct fex_gen_config<xcb_depth_sizeof> {};
template<> struct fex_gen_config<xcb_depth_visuals> {};
template<> struct fex_gen_config<xcb_depth_visuals_length> {};
template<> struct fex_gen_config<xcb_depth_visuals_iterator> {};

template<> struct fex_gen_config<xcb_depth_next> {};
template<> struct fex_gen_config<xcb_depth_end> {};
template<> struct fex_gen_config<xcb_screen_sizeof> {};
template<> struct fex_gen_config<xcb_screen_allowed_depths_length> {};
template<> struct fex_gen_config<xcb_screen_allowed_depths_iterator> {};
template<> struct fex_gen_config<xcb_screen_next> {};
template<> struct fex_gen_config<xcb_screen_end> {};
template<> struct fex_gen_config<xcb_setup_request_sizeof> {};

template<> struct fex_gen_config<xcb_setup_request_authorization_protocol_name> {};
template<> struct fex_gen_config<xcb_setup_request_authorization_protocol_name_length> {};
template<> struct fex_gen_config<xcb_setup_request_authorization_protocol_name_end> {};

template<> struct fex_gen_config<xcb_setup_request_authorization_protocol_data> {};
template<> struct fex_gen_config<xcb_setup_request_authorization_protocol_data_length> {};
template<> struct fex_gen_config<xcb_setup_request_authorization_protocol_data_end> {};

template<> struct fex_gen_config<xcb_setup_request_next> {};
template<> struct fex_gen_config<xcb_setup_request_end> {};
template<> struct fex_gen_config<xcb_setup_failed_sizeof> {};
template<> struct fex_gen_config<xcb_setup_failed_reason> {};

template<> struct fex_gen_config<xcb_setup_failed_reason_length> {};
template<> struct fex_gen_config<xcb_setup_failed_reason_end> {};
template<> struct fex_gen_config<xcb_setup_failed_next> {};
template<> struct fex_gen_config<xcb_setup_failed_end> {};
template<> struct fex_gen_config<xcb_setup_authenticate_sizeof> {};
template<> struct fex_gen_config<xcb_setup_authenticate_reason> {};
template<> struct fex_gen_config<xcb_setup_authenticate_reason_length> {};
template<> struct fex_gen_config<xcb_setup_authenticate_reason_end> {};

template<> struct fex_gen_config<xcb_setup_authenticate_next> {};
template<> struct fex_gen_config<xcb_setup_authenticate_end> {};
template<> struct fex_gen_config<xcb_setup_sizeof> {};
template<> struct fex_gen_config<xcb_setup_vendor> {};
template<> struct fex_gen_config<xcb_setup_vendor_length> {};
template<> struct fex_gen_config<xcb_setup_vendor_end> {};

template<> struct fex_gen_config<xcb_setup_pixmap_formats> {};
template<> struct fex_gen_config<xcb_setup_pixmap_formats_length> {};
template<> struct fex_gen_config<xcb_setup_pixmap_formats_iterator> {};

template<> struct fex_gen_config<xcb_setup_roots_length> {};
template<> struct fex_gen_config<xcb_setup_roots_iterator> {};
template<> struct fex_gen_config<xcb_setup_next> {};
template<> struct fex_gen_config<xcb_setup_end> {};
template<> struct fex_gen_config<xcb_client_message_data_next> {};
template<> struct fex_gen_config<xcb_client_message_data_end> {};
template<> struct fex_gen_config<xcb_create_window_value_list_serialize> {};
template<> struct fex_gen_config<xcb_create_window_value_list_unpack> {};
template<> struct fex_gen_config<xcb_create_window_value_list_sizeof> {};
template<> struct fex_gen_config<xcb_create_window_sizeof> {};
template<> struct fex_gen_config<xcb_create_window_checked> {};
template<> struct fex_gen_config<xcb_create_window> {};
template<> struct fex_gen_config<xcb_create_window_aux_checked> {};
template<> struct fex_gen_config<xcb_create_window_aux> {};
template<> struct fex_gen_config<xcb_create_window_value_list> {};
template<> struct fex_gen_config<xcb_change_window_attributes_value_list_serialize> {};
template<> struct fex_gen_config<xcb_change_window_attributes_value_list_unpack> {};
template<> struct fex_gen_config<xcb_change_window_attributes_value_list_sizeof> {};
template<> struct fex_gen_config<xcb_change_window_attributes_sizeof> {};
template<> struct fex_gen_config<xcb_change_window_attributes_checked> {};
template<> struct fex_gen_config<xcb_change_window_attributes> {};
template<> struct fex_gen_config<xcb_change_window_attributes_aux_checked> {};
template<> struct fex_gen_config<xcb_change_window_attributes_aux> {};
template<> struct fex_gen_config<xcb_change_window_attributes_value_list> {};
template<> struct fex_gen_config<xcb_get_window_attributes> {};
template<> struct fex_gen_config<xcb_get_window_attributes_unchecked> {};
template<> struct fex_gen_config<xcb_get_window_attributes_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_destroy_window_checked> {};
template<> struct fex_gen_config<xcb_destroy_window> {};
template<> struct fex_gen_config<xcb_destroy_subwindows_checked> {};
template<> struct fex_gen_config<xcb_destroy_subwindows> {};
template<> struct fex_gen_config<xcb_change_save_set_checked> {};
template<> struct fex_gen_config<xcb_change_save_set> {};
template<> struct fex_gen_config<xcb_reparent_window_checked> {};
template<> struct fex_gen_config<xcb_reparent_window> {};
template<> struct fex_gen_config<xcb_map_window_checked> {};
template<> struct fex_gen_config<xcb_map_window> {};
template<> struct fex_gen_config<xcb_map_subwindows_checked> {};
template<> struct fex_gen_config<xcb_map_subwindows> {};
template<> struct fex_gen_config<xcb_unmap_window_checked> {};
template<> struct fex_gen_config<xcb_unmap_window> {};
template<> struct fex_gen_config<xcb_unmap_subwindows_checked> {};
template<> struct fex_gen_config<xcb_unmap_subwindows> {};
template<> struct fex_gen_config<xcb_configure_window_value_list_serialize> {};
template<> struct fex_gen_config<xcb_configure_window_value_list_unpack> {};
template<> struct fex_gen_config<xcb_configure_window_value_list_sizeof> {};
template<> struct fex_gen_config<xcb_configure_window_sizeof> {};
template<> struct fex_gen_config<xcb_configure_window_checked> {};
template<> struct fex_gen_config<xcb_configure_window> {};
template<> struct fex_gen_config<xcb_configure_window_aux_checked> {};
template<> struct fex_gen_config<xcb_configure_window_aux> {};
template<> struct fex_gen_config<xcb_configure_window_value_list> {};
template<> struct fex_gen_config<xcb_circulate_window_checked> {};
template<> struct fex_gen_config<xcb_circulate_window> {};
template<> struct fex_gen_config<xcb_get_geometry> {};
template<> struct fex_gen_config<xcb_get_geometry_unchecked> {};
template<> struct fex_gen_config<xcb_get_geometry_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_query_tree_sizeof> {};
template<> struct fex_gen_config<xcb_query_tree> {};
template<> struct fex_gen_config<xcb_query_tree_unchecked> {};
template<> struct fex_gen_config<xcb_query_tree_children> {};
template<> struct fex_gen_config<xcb_query_tree_children_length> {};
template<> struct fex_gen_config<xcb_query_tree_children_end> {};

template<> struct fex_gen_config<xcb_query_tree_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_intern_atom_sizeof> {};
template<> struct fex_gen_config<xcb_intern_atom> {};
template<> struct fex_gen_config<xcb_intern_atom_unchecked> {};
template<> struct fex_gen_config<xcb_intern_atom_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_get_atom_name_sizeof> {};
template<> struct fex_gen_config<xcb_get_atom_name> {};
template<> struct fex_gen_config<xcb_get_atom_name_unchecked> {};
template<> struct fex_gen_config<xcb_get_atom_name_name> {};
template<> struct fex_gen_config<xcb_get_atom_name_name_length> {};
template<> struct fex_gen_config<xcb_get_atom_name_name_end> {};

template<> struct fex_gen_config<xcb_get_atom_name_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_change_property_sizeof> {};
template<> struct fex_gen_config<xcb_change_property_checked> {};
template<> struct fex_gen_config<xcb_change_property> {};
template<> struct fex_gen_config<xcb_change_property_data> {};
template<> struct fex_gen_config<xcb_change_property_data_length> {};
template<> struct fex_gen_config<xcb_change_property_data_end> {};

template<> struct fex_gen_config<xcb_delete_property_checked> {};
template<> struct fex_gen_config<xcb_delete_property> {};
template<> struct fex_gen_config<xcb_get_property_sizeof> {};
template<> struct fex_gen_config<xcb_get_property> {};
template<> struct fex_gen_config<xcb_get_property_unchecked> {};
template<> struct fex_gen_config<xcb_get_property_value> {};
template<> struct fex_gen_config<xcb_get_property_value_length> {};
template<> struct fex_gen_config<xcb_get_property_value_end> {};

template<> struct fex_gen_config<xcb_get_property_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_list_properties_sizeof> {};
template<> struct fex_gen_config<xcb_list_properties> {};
template<> struct fex_gen_config<xcb_list_properties_unchecked> {};
template<> struct fex_gen_config<xcb_list_properties_atoms> {};
template<> struct fex_gen_config<xcb_list_properties_atoms_length> {};
template<> struct fex_gen_config<xcb_list_properties_atoms_end> {};

template<> struct fex_gen_config<xcb_list_properties_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_set_selection_owner_checked> {};
template<> struct fex_gen_config<xcb_set_selection_owner> {};
template<> struct fex_gen_config<xcb_get_selection_owner> {};
template<> struct fex_gen_config<xcb_get_selection_owner_unchecked> {};
template<> struct fex_gen_config<xcb_get_selection_owner_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_convert_selection_checked> {};
template<> struct fex_gen_config<xcb_convert_selection> {};
template<> struct fex_gen_config<xcb_send_event_checked> {};
template<> struct fex_gen_config<xcb_send_event> {};
template<> struct fex_gen_config<xcb_grab_pointer> {};
template<> struct fex_gen_config<xcb_grab_pointer_unchecked> {};
template<> struct fex_gen_config<xcb_grab_pointer_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_ungrab_pointer_checked> {};
template<> struct fex_gen_config<xcb_ungrab_pointer> {};
template<> struct fex_gen_config<xcb_grab_button_checked> {};
template<> struct fex_gen_config<xcb_grab_button> {};
template<> struct fex_gen_config<xcb_ungrab_button_checked> {};
template<> struct fex_gen_config<xcb_ungrab_button> {};
template<> struct fex_gen_config<xcb_change_active_pointer_grab_checked> {};
template<> struct fex_gen_config<xcb_change_active_pointer_grab> {};
template<> struct fex_gen_config<xcb_grab_keyboard> {};
template<> struct fex_gen_config<xcb_grab_keyboard_unchecked> {};
template<> struct fex_gen_config<xcb_grab_keyboard_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_ungrab_keyboard_checked> {};
template<> struct fex_gen_config<xcb_ungrab_keyboard> {};
template<> struct fex_gen_config<xcb_grab_key_checked> {};
template<> struct fex_gen_config<xcb_grab_key> {};
template<> struct fex_gen_config<xcb_ungrab_key_checked> {};
template<> struct fex_gen_config<xcb_ungrab_key> {};
template<> struct fex_gen_config<xcb_allow_events_checked> {};
template<> struct fex_gen_config<xcb_allow_events> {};
template<> struct fex_gen_config<xcb_grab_server_checked> {};
template<> struct fex_gen_config<xcb_grab_server> {};
template<> struct fex_gen_config<xcb_ungrab_server_checked> {};
template<> struct fex_gen_config<xcb_ungrab_server> {};
template<> struct fex_gen_config<xcb_query_pointer> {};
template<> struct fex_gen_config<xcb_query_pointer_unchecked> {};
template<> struct fex_gen_config<xcb_query_pointer_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_timecoord_next> {};
template<> struct fex_gen_config<xcb_timecoord_end> {};
template<> struct fex_gen_config<xcb_get_motion_events_sizeof> {};
template<> struct fex_gen_config<xcb_get_motion_events> {};
template<> struct fex_gen_config<xcb_get_motion_events_unchecked> {};
template<> struct fex_gen_config<xcb_get_motion_events_events> {};
template<> struct fex_gen_config<xcb_get_motion_events_events_length> {};
template<> struct fex_gen_config<xcb_get_motion_events_events_iterator> {};

template<> struct fex_gen_config<xcb_get_motion_events_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_translate_coordinates> {};
template<> struct fex_gen_config<xcb_translate_coordinates_unchecked> {};
template<> struct fex_gen_config<xcb_translate_coordinates_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_warp_pointer_checked> {};
template<> struct fex_gen_config<xcb_warp_pointer> {};
template<> struct fex_gen_config<xcb_set_input_focus_checked> {};
template<> struct fex_gen_config<xcb_set_input_focus> {};
template<> struct fex_gen_config<xcb_get_input_focus> {};
template<> struct fex_gen_config<xcb_get_input_focus_unchecked> {};
template<> struct fex_gen_config<xcb_get_input_focus_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_query_keymap> {};
template<> struct fex_gen_config<xcb_query_keymap_unchecked> {};
template<> struct fex_gen_config<xcb_query_keymap_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_open_font_sizeof> {};
template<> struct fex_gen_config<xcb_open_font_checked> {};
template<> struct fex_gen_config<xcb_open_font> {};
template<> struct fex_gen_config<xcb_open_font_name> {};
template<> struct fex_gen_config<xcb_open_font_name_length> {};
template<> struct fex_gen_config<xcb_open_font_name_end> {};

template<> struct fex_gen_config<xcb_close_font_checked> {};
template<> struct fex_gen_config<xcb_close_font> {};
template<> struct fex_gen_config<xcb_fontprop_next> {};
template<> struct fex_gen_config<xcb_fontprop_end> {};
template<> struct fex_gen_config<xcb_charinfo_next> {};
template<> struct fex_gen_config<xcb_charinfo_end> {};
template<> struct fex_gen_config<xcb_query_font_sizeof> {};
template<> struct fex_gen_config<xcb_query_font> {};
template<> struct fex_gen_config<xcb_query_font_unchecked> {};
template<> struct fex_gen_config<xcb_query_font_properties> {};
template<> struct fex_gen_config<xcb_query_font_properties_length> {};
template<> struct fex_gen_config<xcb_query_font_properties_iterator> {};

template<> struct fex_gen_config<xcb_query_font_char_infos> {};
template<> struct fex_gen_config<xcb_query_font_char_infos_length> {};
template<> struct fex_gen_config<xcb_query_font_char_infos_iterator> {};

template<> struct fex_gen_config<xcb_query_font_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_query_text_extents_sizeof> {};
template<> struct fex_gen_config<xcb_query_text_extents> {};
template<> struct fex_gen_config<xcb_query_text_extents_unchecked> {};
template<> struct fex_gen_config<xcb_query_text_extents_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_str_sizeof> {};
template<> struct fex_gen_config<xcb_str_name> {};
template<> struct fex_gen_config<xcb_str_name_length> {};
template<> struct fex_gen_config<xcb_str_name_end> {};

template<> struct fex_gen_config<xcb_str_next> {};
template<> struct fex_gen_config<xcb_str_end> {};
template<> struct fex_gen_config<xcb_list_fonts_sizeof> {};
template<> struct fex_gen_config<xcb_list_fonts> {};
template<> struct fex_gen_config<xcb_list_fonts_unchecked> {};
template<> struct fex_gen_config<xcb_list_fonts_names_length> {};
template<> struct fex_gen_config<xcb_list_fonts_names_iterator> {};
template<> struct fex_gen_config<xcb_list_fonts_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_list_fonts_with_info_sizeof> {};
template<> struct fex_gen_config<xcb_list_fonts_with_info> {};
template<> struct fex_gen_config<xcb_list_fonts_with_info_unchecked> {};
template<> struct fex_gen_config<xcb_list_fonts_with_info_properties> {};
template<> struct fex_gen_config<xcb_list_fonts_with_info_properties_length> {};
template<> struct fex_gen_config<xcb_list_fonts_with_info_properties_iterator> {};

template<> struct fex_gen_config<xcb_list_fonts_with_info_name> {};
template<> struct fex_gen_config<xcb_list_fonts_with_info_name_length> {};
template<> struct fex_gen_config<xcb_list_fonts_with_info_name_end> {};

template<> struct fex_gen_config<xcb_list_fonts_with_info_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_set_font_path_sizeof> {};
template<> struct fex_gen_config<xcb_set_font_path_checked> {};
template<> struct fex_gen_config<xcb_set_font_path> {};
template<> struct fex_gen_config<xcb_set_font_path_font_length> {};
template<> struct fex_gen_config<xcb_set_font_path_font_iterator> {};
template<> struct fex_gen_config<xcb_get_font_path_sizeof> {};
template<> struct fex_gen_config<xcb_get_font_path> {};
template<> struct fex_gen_config<xcb_get_font_path_unchecked> {};
template<> struct fex_gen_config<xcb_get_font_path_path_length> {};
template<> struct fex_gen_config<xcb_get_font_path_path_iterator> {};
template<> struct fex_gen_config<xcb_get_font_path_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_create_pixmap_checked> {};
template<> struct fex_gen_config<xcb_create_pixmap> {};
template<> struct fex_gen_config<xcb_free_pixmap_checked> {};
template<> struct fex_gen_config<xcb_free_pixmap> {};
template<> struct fex_gen_config<xcb_create_gc_value_list_serialize> {};
template<> struct fex_gen_config<xcb_create_gc_value_list_unpack> {};
template<> struct fex_gen_config<xcb_create_gc_value_list_sizeof> {};
template<> struct fex_gen_config<xcb_create_gc_sizeof> {};
template<> struct fex_gen_config<xcb_create_gc_checked> {};
template<> struct fex_gen_config<xcb_create_gc> {};
template<> struct fex_gen_config<xcb_create_gc_aux_checked> {};
template<> struct fex_gen_config<xcb_create_gc_aux> {};
template<> struct fex_gen_config<xcb_create_gc_value_list> {};
template<> struct fex_gen_config<xcb_change_gc_value_list_serialize> {};
template<> struct fex_gen_config<xcb_change_gc_value_list_unpack> {};
template<> struct fex_gen_config<xcb_change_gc_value_list_sizeof> {};
template<> struct fex_gen_config<xcb_change_gc_sizeof> {};
template<> struct fex_gen_config<xcb_change_gc_checked> {};
template<> struct fex_gen_config<xcb_change_gc> {};
template<> struct fex_gen_config<xcb_change_gc_aux_checked> {};
template<> struct fex_gen_config<xcb_change_gc_aux> {};
template<> struct fex_gen_config<xcb_change_gc_value_list> {};

template<> struct fex_gen_config<xcb_copy_gc_checked> {};
template<> struct fex_gen_config<xcb_copy_gc> {};
template<> struct fex_gen_config<xcb_set_dashes_sizeof> {};
template<> struct fex_gen_config<xcb_set_dashes_checked> {};
template<> struct fex_gen_config<xcb_set_dashes> {};
template<> struct fex_gen_config<xcb_set_dashes_dashes> {};
template<> struct fex_gen_config<xcb_set_dashes_dashes_length> {};
template<> struct fex_gen_config<xcb_set_dashes_dashes_end> {};

template<> struct fex_gen_config<xcb_set_clip_rectangles_sizeof> {};
template<> struct fex_gen_config<xcb_set_clip_rectangles_checked> {};
template<> struct fex_gen_config<xcb_set_clip_rectangles> {};
template<> struct fex_gen_config<xcb_set_clip_rectangles_rectangles> {};
template<> struct fex_gen_config<xcb_set_clip_rectangles_rectangles_length> {};
template<> struct fex_gen_config<xcb_set_clip_rectangles_rectangles_iterator> {};

template<> struct fex_gen_config<xcb_free_gc_checked> {};
template<> struct fex_gen_config<xcb_free_gc> {};
template<> struct fex_gen_config<xcb_clear_area_checked> {};
template<> struct fex_gen_config<xcb_clear_area> {};
template<> struct fex_gen_config<xcb_copy_area_checked> {};
template<> struct fex_gen_config<xcb_copy_area> {};
template<> struct fex_gen_config<xcb_copy_plane_checked> {};
template<> struct fex_gen_config<xcb_copy_plane> {};
template<> struct fex_gen_config<xcb_poly_point_sizeof> {};
template<> struct fex_gen_config<xcb_poly_point_checked> {};
template<> struct fex_gen_config<xcb_poly_point> {};
template<> struct fex_gen_config<xcb_poly_point_points> {};
template<> struct fex_gen_config<xcb_poly_point_points_length> {};
template<> struct fex_gen_config<xcb_poly_point_points_iterator> {};

template<> struct fex_gen_config<xcb_poly_line_sizeof> {};
template<> struct fex_gen_config<xcb_poly_line_checked> {};
template<> struct fex_gen_config<xcb_poly_line> {};
template<> struct fex_gen_config<xcb_poly_line_points> {};
template<> struct fex_gen_config<xcb_poly_line_points_length> {};
template<> struct fex_gen_config<xcb_poly_line_points_iterator> {};

template<> struct fex_gen_config<xcb_segment_next> {};
template<> struct fex_gen_config<xcb_segment_end> {};
template<> struct fex_gen_config<xcb_poly_segment_sizeof> {};
template<> struct fex_gen_config<xcb_poly_segment_checked> {};
template<> struct fex_gen_config<xcb_poly_segment> {};
template<> struct fex_gen_config<xcb_poly_segment_segments> {};
template<> struct fex_gen_config<xcb_poly_segment_segments_length> {};
template<> struct fex_gen_config<xcb_poly_segment_segments_iterator> {};

template<> struct fex_gen_config<xcb_poly_rectangle_sizeof> {};
template<> struct fex_gen_config<xcb_poly_rectangle_checked> {};
template<> struct fex_gen_config<xcb_poly_rectangle> {};
template<> struct fex_gen_config<xcb_poly_rectangle_rectangles> {};
template<> struct fex_gen_config<xcb_poly_rectangle_rectangles_length> {};
template<> struct fex_gen_config<xcb_poly_rectangle_rectangles_iterator> {};

template<> struct fex_gen_config<xcb_poly_arc_sizeof> {};
template<> struct fex_gen_config<xcb_poly_arc_checked> {};
template<> struct fex_gen_config<xcb_poly_arc> {};
template<> struct fex_gen_config<xcb_poly_arc_arcs> {};
template<> struct fex_gen_config<xcb_poly_arc_arcs_length> {};
template<> struct fex_gen_config<xcb_poly_arc_arcs_iterator> {};

template<> struct fex_gen_config<xcb_fill_poly_sizeof> {};
template<> struct fex_gen_config<xcb_fill_poly_checked> {};
template<> struct fex_gen_config<xcb_fill_poly> {};
template<> struct fex_gen_config<xcb_fill_poly_points> {};
template<> struct fex_gen_config<xcb_fill_poly_points_length> {};
template<> struct fex_gen_config<xcb_fill_poly_points_iterator> {};

template<> struct fex_gen_config<xcb_poly_fill_rectangle_sizeof> {};
template<> struct fex_gen_config<xcb_poly_fill_rectangle_checked> {};
template<> struct fex_gen_config<xcb_poly_fill_rectangle> {};
template<> struct fex_gen_config<xcb_poly_fill_rectangle_rectangles> {};
template<> struct fex_gen_config<xcb_poly_fill_rectangle_rectangles_length> {};
template<> struct fex_gen_config<xcb_poly_fill_rectangle_rectangles_iterator> {};

template<> struct fex_gen_config<xcb_poly_fill_arc_sizeof> {};
template<> struct fex_gen_config<xcb_poly_fill_arc_checked> {};
template<> struct fex_gen_config<xcb_poly_fill_arc> {};
template<> struct fex_gen_config<xcb_poly_fill_arc_arcs> {};
template<> struct fex_gen_config<xcb_poly_fill_arc_arcs_length> {};
template<> struct fex_gen_config<xcb_poly_fill_arc_arcs_iterator> {};

template<> struct fex_gen_config<xcb_put_image_sizeof> {};
template<> struct fex_gen_config<xcb_put_image_checked> {};
template<> struct fex_gen_config<xcb_put_image> {};
template<> struct fex_gen_config<xcb_put_image_data> {};
template<> struct fex_gen_config<xcb_put_image_data_length> {};
template<> struct fex_gen_config<xcb_put_image_data_end> {};

template<> struct fex_gen_config<xcb_get_image_sizeof> {};
template<> struct fex_gen_config<xcb_get_image> {};
template<> struct fex_gen_config<xcb_get_image_unchecked> {};
template<> struct fex_gen_config<xcb_get_image_data> {};
template<> struct fex_gen_config<xcb_get_image_data_length> {};
template<> struct fex_gen_config<xcb_get_image_data_end> {};

template<> struct fex_gen_config<xcb_get_image_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_poly_text_8_sizeof> {};
template<> struct fex_gen_config<xcb_poly_text_8_checked> {};
template<> struct fex_gen_config<xcb_poly_text_8> {};
template<> struct fex_gen_config<xcb_poly_text_8_items> {};
template<> struct fex_gen_config<xcb_poly_text_8_items_length> {};
template<> struct fex_gen_config<xcb_poly_text_8_items_end> {};

template<> struct fex_gen_config<xcb_poly_text_16_sizeof> {};
template<> struct fex_gen_config<xcb_poly_text_16_checked> {};
template<> struct fex_gen_config<xcb_poly_text_16> {};
template<> struct fex_gen_config<xcb_poly_text_16_items> {};
template<> struct fex_gen_config<xcb_poly_text_16_items_length> {};
template<> struct fex_gen_config<xcb_poly_text_16_items_end> {};

template<> struct fex_gen_config<xcb_image_text_8_sizeof> {};
template<> struct fex_gen_config<xcb_image_text_8_checked> {};
template<> struct fex_gen_config<xcb_image_text_8> {};
template<> struct fex_gen_config<xcb_image_text_8_string> {};
template<> struct fex_gen_config<xcb_image_text_8_string_length> {};
template<> struct fex_gen_config<xcb_image_text_8_string_end> {};

template<> struct fex_gen_config<xcb_image_text_16_sizeof> {};
template<> struct fex_gen_config<xcb_image_text_16_checked> {};
template<> struct fex_gen_config<xcb_image_text_16> {};
template<> struct fex_gen_config<xcb_image_text_16_string> {};
template<> struct fex_gen_config<xcb_image_text_16_string_length> {};
template<> struct fex_gen_config<xcb_image_text_16_string_iterator> {};

template<> struct fex_gen_config<xcb_create_colormap_checked> {};
template<> struct fex_gen_config<xcb_create_colormap> {};
template<> struct fex_gen_config<xcb_free_colormap_checked> {};
template<> struct fex_gen_config<xcb_free_colormap> {};
template<> struct fex_gen_config<xcb_copy_colormap_and_free_checked> {};
template<> struct fex_gen_config<xcb_copy_colormap_and_free> {};
template<> struct fex_gen_config<xcb_install_colormap_checked> {};
template<> struct fex_gen_config<xcb_install_colormap> {};
template<> struct fex_gen_config<xcb_uninstall_colormap_checked> {};
template<> struct fex_gen_config<xcb_uninstall_colormap> {};
template<> struct fex_gen_config<xcb_list_installed_colormaps_sizeof> {};
template<> struct fex_gen_config<xcb_list_installed_colormaps> {};
template<> struct fex_gen_config<xcb_list_installed_colormaps_unchecked> {};
template<> struct fex_gen_config<xcb_list_installed_colormaps_cmaps> {};
template<> struct fex_gen_config<xcb_list_installed_colormaps_cmaps_length> {};
template<> struct fex_gen_config<xcb_list_installed_colormaps_cmaps_end> {};

template<> struct fex_gen_config<xcb_list_installed_colormaps_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_alloc_color> {};
template<> struct fex_gen_config<xcb_alloc_color_unchecked> {};
template<> struct fex_gen_config<xcb_alloc_color_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_alloc_named_color_sizeof> {};
template<> struct fex_gen_config<xcb_alloc_named_color> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_alloc_named_color_unchecked> {};
template<> struct fex_gen_config<xcb_alloc_named_color_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_alloc_color_cells_sizeof> {};
template<> struct fex_gen_config<xcb_alloc_color_cells> {};
template<> struct fex_gen_config<xcb_alloc_color_cells_unchecked> {};
template<> struct fex_gen_config<xcb_alloc_color_cells_pixels> {};
template<> struct fex_gen_config<xcb_alloc_color_cells_pixels_length> {};
template<> struct fex_gen_config<xcb_alloc_color_cells_pixels_end> {};

template<> struct fex_gen_config<xcb_alloc_color_cells_masks> {};
template<> struct fex_gen_config<xcb_alloc_color_cells_masks_length> {};
template<> struct fex_gen_config<xcb_alloc_color_cells_masks_end> {};

template<> struct fex_gen_config<xcb_alloc_color_cells_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_alloc_color_planes_sizeof> {};
template<> struct fex_gen_config<xcb_alloc_color_planes> {};
template<> struct fex_gen_config<xcb_alloc_color_planes_unchecked> {};
template<> struct fex_gen_config<xcb_alloc_color_planes_pixels> {};
template<> struct fex_gen_config<xcb_alloc_color_planes_pixels_length> {};
template<> struct fex_gen_config<xcb_alloc_color_planes_pixels_end> {};

template<> struct fex_gen_config<xcb_alloc_color_planes_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_free_colors_sizeof> {};
template<> struct fex_gen_config<xcb_free_colors_checked> {};
template<> struct fex_gen_config<xcb_free_colors> {};
template<> struct fex_gen_config<xcb_free_colors_pixels> {};
template<> struct fex_gen_config<xcb_free_colors_pixels_length> {};
template<> struct fex_gen_config<xcb_free_colors_pixels_end> {};

template<> struct fex_gen_config<xcb_coloritem_next> {};
template<> struct fex_gen_config<xcb_coloritem_end> {};
template<> struct fex_gen_config<xcb_store_colors_sizeof> {};
template<> struct fex_gen_config<xcb_store_colors_checked> {};
template<> struct fex_gen_config<xcb_store_colors> {};
template<> struct fex_gen_config<xcb_store_colors_items> {};
template<> struct fex_gen_config<xcb_store_colors_items_length> {};
template<> struct fex_gen_config<xcb_store_colors_items_iterator> {};

template<> struct fex_gen_config<xcb_store_named_color_sizeof> {};
template<> struct fex_gen_config<xcb_store_named_color_checked> {};
template<> struct fex_gen_config<xcb_store_named_color> {};
template<> struct fex_gen_config<xcb_store_named_color_name> {};

template<> struct fex_gen_config<xcb_store_named_color_name_length> {};
template<> struct fex_gen_config<xcb_store_named_color_name_end> {};
template<> struct fex_gen_config<xcb_rgb_next> {};
template<> struct fex_gen_config<xcb_rgb_end> {};
template<> struct fex_gen_config<xcb_query_colors_sizeof> {};
template<> struct fex_gen_config<xcb_query_colors> {};
template<> struct fex_gen_config<xcb_query_colors_unchecked> {};
template<> struct fex_gen_config<xcb_query_colors_colors> {};
template<> struct fex_gen_config<xcb_query_colors_colors_length> {};
template<> struct fex_gen_config<xcb_query_colors_colors_iterator> {};

template<> struct fex_gen_config<xcb_query_colors_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_lookup_color_sizeof> {};
template<> struct fex_gen_config<xcb_lookup_color> {};
template<> struct fex_gen_config<xcb_lookup_color_unchecked> {};
template<> struct fex_gen_config<xcb_lookup_color_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_create_cursor_checked> {};
template<> struct fex_gen_config<xcb_create_cursor> {};
template<> struct fex_gen_config<xcb_create_glyph_cursor_checked> {};
template<> struct fex_gen_config<xcb_create_glyph_cursor> {};
template<> struct fex_gen_config<xcb_free_cursor_checked> {};
template<> struct fex_gen_config<xcb_free_cursor> {};
template<> struct fex_gen_config<xcb_recolor_cursor_checked> {};
template<> struct fex_gen_config<xcb_recolor_cursor> {};
template<> struct fex_gen_config<xcb_query_best_size> {};
template<> struct fex_gen_config<xcb_query_best_size_unchecked> {};
template<> struct fex_gen_config<xcb_query_best_size_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_query_extension_sizeof> {};
template<> struct fex_gen_config<xcb_query_extension> {};
template<> struct fex_gen_config<xcb_query_extension_unchecked> {};
template<> struct fex_gen_config<xcb_query_extension_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_list_extensions_sizeof> {};
template<> struct fex_gen_config<xcb_list_extensions> {};
template<> struct fex_gen_config<xcb_list_extensions_unchecked> {};
template<> struct fex_gen_config<xcb_list_extensions_names_length> {};
template<> struct fex_gen_config<xcb_list_extensions_names_iterator> {};
template<> struct fex_gen_config<xcb_list_extensions_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_change_keyboard_mapping_sizeof> {};
template<> struct fex_gen_config<xcb_change_keyboard_mapping_checked> {};
template<> struct fex_gen_config<xcb_change_keyboard_mapping> {};
template<> struct fex_gen_config<xcb_change_keyboard_mapping_keysyms> {};
template<> struct fex_gen_config<xcb_change_keyboard_mapping_keysyms_length> {};
template<> struct fex_gen_config<xcb_change_keyboard_mapping_keysyms_end> {};

template<> struct fex_gen_config<xcb_get_keyboard_mapping_sizeof> {};
template<> struct fex_gen_config<xcb_get_keyboard_mapping> {};
template<> struct fex_gen_config<xcb_get_keyboard_mapping_unchecked> {};
template<> struct fex_gen_config<xcb_get_keyboard_mapping_keysyms> {};
template<> struct fex_gen_config<xcb_get_keyboard_mapping_keysyms_length> {};
template<> struct fex_gen_config<xcb_get_keyboard_mapping_keysyms_end> {};

template<> struct fex_gen_config<xcb_get_keyboard_mapping_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_change_keyboard_control_value_list_serialize> {};
template<> struct fex_gen_config<xcb_change_keyboard_control_value_list_unpack> {};
template<> struct fex_gen_config<xcb_change_keyboard_control_value_list_sizeof> {};
template<> struct fex_gen_config<xcb_change_keyboard_control_sizeof> {};
template<> struct fex_gen_config<xcb_change_keyboard_control_checked> {};
template<> struct fex_gen_config<xcb_change_keyboard_control> {};
template<> struct fex_gen_config<xcb_change_keyboard_control_aux_checked> {};
template<> struct fex_gen_config<xcb_change_keyboard_control_aux> {};
template<> struct fex_gen_config<xcb_change_keyboard_control_value_list> {};

template<> struct fex_gen_config<xcb_get_keyboard_control> {};
template<> struct fex_gen_config<xcb_get_keyboard_control_unchecked> {};
template<> struct fex_gen_config<xcb_get_keyboard_control_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_bell_checked> {};
template<> struct fex_gen_config<xcb_bell> {};
template<> struct fex_gen_config<xcb_change_pointer_control_checked> {};
template<> struct fex_gen_config<xcb_change_pointer_control> {};
template<> struct fex_gen_config<xcb_get_pointer_control> {};
template<> struct fex_gen_config<xcb_get_pointer_control_unchecked> {};
template<> struct fex_gen_config<xcb_get_pointer_control_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_set_screen_saver_checked> {};
template<> struct fex_gen_config<xcb_set_screen_saver> {};
template<> struct fex_gen_config<xcb_get_screen_saver> {};
template<> struct fex_gen_config<xcb_get_screen_saver_unchecked> {};
template<> struct fex_gen_config<xcb_get_screen_saver_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_change_hosts_sizeof> {};
template<> struct fex_gen_config<xcb_change_hosts_checked> {};
template<> struct fex_gen_config<xcb_change_hosts> {};
template<> struct fex_gen_config<xcb_change_hosts_address> {};
template<> struct fex_gen_config<xcb_change_hosts_address_length> {};
template<> struct fex_gen_config<xcb_change_hosts_address_end> {};

template<> struct fex_gen_config<xcb_host_sizeof> {};
template<> struct fex_gen_config<xcb_host_address> {};
template<> struct fex_gen_config<xcb_host_address_length> {};
template<> struct fex_gen_config<xcb_host_address_end> {};

template<> struct fex_gen_config<xcb_host_next> {};
template<> struct fex_gen_config<xcb_host_end> {};
template<> struct fex_gen_config<xcb_list_hosts_sizeof> {};
template<> struct fex_gen_config<xcb_list_hosts> {};
template<> struct fex_gen_config<xcb_list_hosts_unchecked> {};
template<> struct fex_gen_config<xcb_list_hosts_hosts_length> {};
template<> struct fex_gen_config<xcb_list_hosts_hosts_iterator> {};
template<> struct fex_gen_config<xcb_list_hosts_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_set_access_control_checked> {};
template<> struct fex_gen_config<xcb_set_access_control> {};
template<> struct fex_gen_config<xcb_set_close_down_mode_checked> {};
template<> struct fex_gen_config<xcb_set_close_down_mode> {};
template<> struct fex_gen_config<xcb_kill_client_checked> {};
template<> struct fex_gen_config<xcb_kill_client> {};
template<> struct fex_gen_config<xcb_rotate_properties_sizeof> {};
template<> struct fex_gen_config<xcb_rotate_properties_checked> {};
template<> struct fex_gen_config<xcb_rotate_properties> {};
template<> struct fex_gen_config<xcb_rotate_properties_atoms> {};
template<> struct fex_gen_config<xcb_rotate_properties_atoms_length> {};
template<> struct fex_gen_config<xcb_rotate_properties_atoms_end> {};

template<> struct fex_gen_config<xcb_force_screen_saver_checked> {};
template<> struct fex_gen_config<xcb_force_screen_saver> {};
template<> struct fex_gen_config<xcb_set_pointer_mapping_sizeof> {};
template<> struct fex_gen_config<xcb_set_pointer_mapping> {};
template<> struct fex_gen_config<xcb_set_pointer_mapping_unchecked> {};
template<> struct fex_gen_config<xcb_set_pointer_mapping_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_get_pointer_mapping_sizeof> {};
template<> struct fex_gen_config<xcb_get_pointer_mapping> {};
template<> struct fex_gen_config<xcb_get_pointer_mapping_unchecked> {};
template<> struct fex_gen_config<xcb_get_pointer_mapping_map> {};
template<> struct fex_gen_config<xcb_get_pointer_mapping_map_length> {};
template<> struct fex_gen_config<xcb_get_pointer_mapping_map_end> {};

template<> struct fex_gen_config<xcb_get_pointer_mapping_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_set_modifier_mapping_sizeof> {};
template<> struct fex_gen_config<xcb_set_modifier_mapping> {};
template<> struct fex_gen_config<xcb_set_modifier_mapping_unchecked> {};
template<> struct fex_gen_config<xcb_set_modifier_mapping_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_get_modifier_mapping_sizeof> {};
template<> struct fex_gen_config<xcb_get_modifier_mapping> {};
template<> struct fex_gen_config<xcb_get_modifier_mapping_unchecked> {};
template<> struct fex_gen_config<xcb_get_modifier_mapping_keycodes> {};
template<> struct fex_gen_config<xcb_get_modifier_mapping_keycodes_length> {};
template<> struct fex_gen_config<xcb_get_modifier_mapping_keycodes_end> {};

template<> struct fex_gen_config<xcb_get_modifier_mapping_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_no_operation_checked> {};
template<> struct fex_gen_config<xcb_no_operation> {};
