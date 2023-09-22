#include <common/GeneratorInterface.h>

#include <xcb/randr.h>
#include <xcb/xcbext.h>

template<auto>
struct fex_gen_config {
    unsigned version = 0;
};

template<typename>
struct fex_gen_type {};

template<> struct fex_gen_type<xcb_connection_t> : fexgen::opaque_type {};

// Union type (includes pointers, so only compatible across 64-bit architectures)
#ifndef IS_32BIT_THUNK
template<> struct fex_gen_type<xcb_randr_notify_data_t> : fexgen::assume_compatible_data_layout {};
#endif

void FEX_xcb_randr_init_extension(xcb_connection_t*, xcb_extension_t*);
size_t FEX_usable_size(void*);
void FEX_free_on_host(void*);

template<> struct fex_gen_config<FEX_xcb_randr_init_extension> : fexgen::custom_host_impl {};
template<> struct fex_gen_config<FEX_usable_size> : fexgen::custom_host_impl, fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<FEX_free_on_host> : fexgen::custom_host_impl, fexgen::custom_guest_entrypoint {};

template<> struct fex_gen_config<xcb_randr_mode_next> {};
template<> struct fex_gen_config<xcb_randr_mode_end> {};
template<> struct fex_gen_config<xcb_randr_crtc_next> {};
template<> struct fex_gen_config<xcb_randr_crtc_end> {};
template<> struct fex_gen_config<xcb_randr_output_next> {};
template<> struct fex_gen_config<xcb_randr_output_end> {};
template<> struct fex_gen_config<xcb_randr_provider_next> {};
template<> struct fex_gen_config<xcb_randr_provider_end> {};
template<> struct fex_gen_config<xcb_randr_lease_next> {};
template<> struct fex_gen_config<xcb_randr_lease_end> {};
template<> struct fex_gen_config<xcb_randr_screen_size_next> {};
template<> struct fex_gen_config<xcb_randr_screen_size_end> {};
template<> struct fex_gen_config<xcb_randr_refresh_rates_sizeof> {};
template<> struct fex_gen_config<xcb_randr_refresh_rates_rates> {};
template<> struct fex_gen_config<xcb_randr_refresh_rates_rates_length> {};
template<> struct fex_gen_config<xcb_randr_refresh_rates_rates_end> {};

template<> struct fex_gen_config<xcb_randr_refresh_rates_next> {};
template<> struct fex_gen_config<xcb_randr_refresh_rates_end> {};
template<> struct fex_gen_config<xcb_randr_query_version> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_randr_query_version_unchecked> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_randr_query_version_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_randr_set_screen_config> {};
template<> struct fex_gen_config<xcb_randr_set_screen_config_unchecked> {};
template<> struct fex_gen_config<xcb_randr_set_screen_config_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_randr_select_input_checked> {};
template<> struct fex_gen_config<xcb_randr_select_input> {};
template<> struct fex_gen_config<xcb_randr_get_screen_info_sizeof> {};
template<> struct fex_gen_config<xcb_randr_get_screen_info> {};
template<> struct fex_gen_config<xcb_randr_get_screen_info_unchecked> {};
template<> struct fex_gen_config<xcb_randr_get_screen_info_sizes> {};
template<> struct fex_gen_config<xcb_randr_get_screen_info_sizes_length> {};
template<> struct fex_gen_config<xcb_randr_get_screen_info_sizes_iterator> {};

template<> struct fex_gen_config<xcb_randr_get_screen_info_rates_length> {};
template<> struct fex_gen_config<xcb_randr_get_screen_info_rates_iterator> {};
template<> struct fex_gen_config<xcb_randr_get_screen_info_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_randr_get_screen_size_range> {};
template<> struct fex_gen_config<xcb_randr_get_screen_size_range_unchecked> {};
template<> struct fex_gen_config<xcb_randr_get_screen_size_range_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_randr_set_screen_size_checked> {};
template<> struct fex_gen_config<xcb_randr_set_screen_size> {};
template<> struct fex_gen_config<xcb_randr_mode_info_next> {};
template<> struct fex_gen_config<xcb_randr_mode_info_end> {};
template<> struct fex_gen_config<xcb_randr_get_screen_resources_sizeof> {};
template<> struct fex_gen_config<xcb_randr_get_screen_resources> {};
template<> struct fex_gen_config<xcb_randr_get_screen_resources_unchecked> {};
template<> struct fex_gen_config<xcb_randr_get_screen_resources_crtcs> {};
template<> struct fex_gen_config<xcb_randr_get_screen_resources_crtcs_length> {};
template<> struct fex_gen_config<xcb_randr_get_screen_resources_crtcs_end> {};
template<> struct fex_gen_config<xcb_randr_get_screen_resources_outputs> {};
template<> struct fex_gen_config<xcb_randr_get_screen_resources_outputs_length> {};
template<> struct fex_gen_config<xcb_randr_get_screen_resources_outputs_end> {};
template<> struct fex_gen_config<xcb_randr_get_screen_resources_modes> {};
template<> struct fex_gen_config<xcb_randr_get_screen_resources_modes_length> {};
template<> struct fex_gen_config<xcb_randr_get_screen_resources_modes_iterator> {};
template<> struct fex_gen_config<xcb_randr_get_screen_resources_names> {};
template<> struct fex_gen_config<xcb_randr_get_screen_resources_names_length> {};
template<> struct fex_gen_config<xcb_randr_get_screen_resources_names_end> {};

template<> struct fex_gen_config<xcb_randr_get_screen_resources_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_randr_get_output_info_sizeof> {};
template<> struct fex_gen_config<xcb_randr_get_output_info> {};
template<> struct fex_gen_config<xcb_randr_get_output_info_unchecked> {};
template<> struct fex_gen_config<xcb_randr_get_output_info_crtcs> {};
template<> struct fex_gen_config<xcb_randr_get_output_info_crtcs_length> {};
template<> struct fex_gen_config<xcb_randr_get_output_info_crtcs_end> {};
template<> struct fex_gen_config<xcb_randr_get_output_info_modes> {};
template<> struct fex_gen_config<xcb_randr_get_output_info_modes_length> {};
template<> struct fex_gen_config<xcb_randr_get_output_info_modes_end> {};
template<> struct fex_gen_config<xcb_randr_get_output_info_clones> {};
template<> struct fex_gen_config<xcb_randr_get_output_info_clones_length> {};
template<> struct fex_gen_config<xcb_randr_get_output_info_clones_end> {};
template<> struct fex_gen_config<xcb_randr_get_output_info_name> {};
template<> struct fex_gen_config<xcb_randr_get_output_info_name_length> {};
template<> struct fex_gen_config<xcb_randr_get_output_info_name_end> {};

template<> struct fex_gen_config<xcb_randr_get_output_info_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_randr_list_output_properties_sizeof> {};
template<> struct fex_gen_config<xcb_randr_list_output_properties> {};
template<> struct fex_gen_config<xcb_randr_list_output_properties_unchecked> {};
template<> struct fex_gen_config<xcb_randr_list_output_properties_atoms> {};
template<> struct fex_gen_config<xcb_randr_list_output_properties_atoms_length> {};
template<> struct fex_gen_config<xcb_randr_list_output_properties_atoms_end> {};

template<> struct fex_gen_config<xcb_randr_list_output_properties_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_randr_query_output_property_sizeof> {};
template<> struct fex_gen_config<xcb_randr_query_output_property> {};
template<> struct fex_gen_config<xcb_randr_query_output_property_unchecked> {};
template<> struct fex_gen_config<xcb_randr_query_output_property_valid_values> {};
template<> struct fex_gen_config<xcb_randr_query_output_property_valid_values_length> {};
template<> struct fex_gen_config<xcb_randr_query_output_property_valid_values_end> {};

template<> struct fex_gen_config<xcb_randr_query_output_property_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_randr_configure_output_property_sizeof> {};
template<> struct fex_gen_config<xcb_randr_configure_output_property_checked> {};
template<> struct fex_gen_config<xcb_randr_configure_output_property> {};
template<> struct fex_gen_config<xcb_randr_configure_output_property_values> {};
template<> struct fex_gen_config<xcb_randr_configure_output_property_values_length> {};
template<> struct fex_gen_config<xcb_randr_configure_output_property_values_end> {};

template<> struct fex_gen_config<xcb_randr_change_output_property_sizeof> {};
template<> struct fex_gen_config<xcb_randr_change_output_property_checked> {};
template<> struct fex_gen_config<xcb_randr_change_output_property> {};
template<> struct fex_gen_config<xcb_randr_change_output_property_data> {};
template<> struct fex_gen_config<xcb_randr_change_output_property_data_length> {};
template<> struct fex_gen_config<xcb_randr_change_output_property_data_end> {};

template<> struct fex_gen_config<xcb_randr_delete_output_property_checked> {};
template<> struct fex_gen_config<xcb_randr_delete_output_property> {};
template<> struct fex_gen_config<xcb_randr_get_output_property_sizeof> {};
template<> struct fex_gen_config<xcb_randr_get_output_property> {};
template<> struct fex_gen_config<xcb_randr_get_output_property_unchecked> {};
template<> struct fex_gen_config<xcb_randr_get_output_property_data> {};
template<> struct fex_gen_config<xcb_randr_get_output_property_data_length> {};
template<> struct fex_gen_config<xcb_randr_get_output_property_data_end> {};

template<> struct fex_gen_config<xcb_randr_get_output_property_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_randr_create_mode_sizeof> {};
template<> struct fex_gen_config<xcb_randr_create_mode> {};
template<> struct fex_gen_config<xcb_randr_create_mode_unchecked> {};
template<> struct fex_gen_config<xcb_randr_create_mode_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_randr_destroy_mode_checked> {};
template<> struct fex_gen_config<xcb_randr_destroy_mode> {};
template<> struct fex_gen_config<xcb_randr_add_output_mode_checked> {};
template<> struct fex_gen_config<xcb_randr_add_output_mode> {};
template<> struct fex_gen_config<xcb_randr_delete_output_mode_checked> {};
template<> struct fex_gen_config<xcb_randr_delete_output_mode> {};
template<> struct fex_gen_config<xcb_randr_get_crtc_info_sizeof> {};
template<> struct fex_gen_config<xcb_randr_get_crtc_info> {};
template<> struct fex_gen_config<xcb_randr_get_crtc_info_unchecked> {};
template<> struct fex_gen_config<xcb_randr_get_crtc_info_outputs> {};
template<> struct fex_gen_config<xcb_randr_get_crtc_info_outputs_length> {};
template<> struct fex_gen_config<xcb_randr_get_crtc_info_outputs_end> {};
template<> struct fex_gen_config<xcb_randr_get_crtc_info_possible> {};
template<> struct fex_gen_config<xcb_randr_get_crtc_info_possible_length> {};
template<> struct fex_gen_config<xcb_randr_get_crtc_info_possible_end> {};

template<> struct fex_gen_config<xcb_randr_get_crtc_info_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_randr_set_crtc_config_sizeof> {};
template<> struct fex_gen_config<xcb_randr_set_crtc_config> {};
template<> struct fex_gen_config<xcb_randr_set_crtc_config_unchecked> {};
template<> struct fex_gen_config<xcb_randr_set_crtc_config_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_randr_get_crtc_gamma_size> {};
template<> struct fex_gen_config<xcb_randr_get_crtc_gamma_size_unchecked> {};
template<> struct fex_gen_config<xcb_randr_get_crtc_gamma_size_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_randr_get_crtc_gamma_sizeof> {};
template<> struct fex_gen_config<xcb_randr_get_crtc_gamma> {};
template<> struct fex_gen_config<xcb_randr_get_crtc_gamma_unchecked> {};
template<> struct fex_gen_config<xcb_randr_get_crtc_gamma_red> {};
template<> struct fex_gen_config<xcb_randr_get_crtc_gamma_red_length> {};
template<> struct fex_gen_config<xcb_randr_get_crtc_gamma_red_end> {};
template<> struct fex_gen_config<xcb_randr_get_crtc_gamma_green> {};
template<> struct fex_gen_config<xcb_randr_get_crtc_gamma_green_length> {};
template<> struct fex_gen_config<xcb_randr_get_crtc_gamma_green_end> {};
template<> struct fex_gen_config<xcb_randr_get_crtc_gamma_blue> {};
template<> struct fex_gen_config<xcb_randr_get_crtc_gamma_blue_length> {};
template<> struct fex_gen_config<xcb_randr_get_crtc_gamma_blue_end> {};

template<> struct fex_gen_config<xcb_randr_get_crtc_gamma_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_randr_set_crtc_gamma_sizeof> {};
template<> struct fex_gen_config<xcb_randr_set_crtc_gamma_checked> {};
template<> struct fex_gen_config<xcb_randr_set_crtc_gamma> {};
template<> struct fex_gen_config<xcb_randr_set_crtc_gamma_red> {};
template<> struct fex_gen_config<xcb_randr_set_crtc_gamma_red_length> {};
template<> struct fex_gen_config<xcb_randr_set_crtc_gamma_red_end> {};
template<> struct fex_gen_config<xcb_randr_set_crtc_gamma_green> {};
template<> struct fex_gen_config<xcb_randr_set_crtc_gamma_green_length> {};
template<> struct fex_gen_config<xcb_randr_set_crtc_gamma_green_end> {};
template<> struct fex_gen_config<xcb_randr_set_crtc_gamma_blue> {};
template<> struct fex_gen_config<xcb_randr_set_crtc_gamma_blue_length> {};
template<> struct fex_gen_config<xcb_randr_set_crtc_gamma_blue_end> {};

template<> struct fex_gen_config<xcb_randr_get_screen_resources_current_sizeof> {};
template<> struct fex_gen_config<xcb_randr_get_screen_resources_current> {};
template<> struct fex_gen_config<xcb_randr_get_screen_resources_current_unchecked> {};
template<> struct fex_gen_config<xcb_randr_get_screen_resources_current_crtcs> {};
template<> struct fex_gen_config<xcb_randr_get_screen_resources_current_crtcs_length> {};
template<> struct fex_gen_config<xcb_randr_get_screen_resources_current_crtcs_end> {};
template<> struct fex_gen_config<xcb_randr_get_screen_resources_current_outputs> {};
template<> struct fex_gen_config<xcb_randr_get_screen_resources_current_outputs_length> {};
template<> struct fex_gen_config<xcb_randr_get_screen_resources_current_outputs_end> {};
template<> struct fex_gen_config<xcb_randr_get_screen_resources_current_modes> {};
template<> struct fex_gen_config<xcb_randr_get_screen_resources_current_modes_length> {};
template<> struct fex_gen_config<xcb_randr_get_screen_resources_current_modes_iterator> {};
template<> struct fex_gen_config<xcb_randr_get_screen_resources_current_names> {};
template<> struct fex_gen_config<xcb_randr_get_screen_resources_current_names_length> {};
template<> struct fex_gen_config<xcb_randr_get_screen_resources_current_names_end> {};

template<> struct fex_gen_config<xcb_randr_get_screen_resources_current_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_randr_set_crtc_transform_sizeof> {};
template<> struct fex_gen_config<xcb_randr_set_crtc_transform_checked> {};
template<> struct fex_gen_config<xcb_randr_set_crtc_transform> {};
template<> struct fex_gen_config<xcb_randr_set_crtc_transform_filter_name> {};
template<> struct fex_gen_config<xcb_randr_set_crtc_transform_filter_name_length> {};
template<> struct fex_gen_config<xcb_randr_set_crtc_transform_filter_name_end> {};
template<> struct fex_gen_config<xcb_randr_set_crtc_transform_filter_params> {};
template<> struct fex_gen_config<xcb_randr_set_crtc_transform_filter_params_length> {};
template<> struct fex_gen_config<xcb_randr_set_crtc_transform_filter_params_end> {};

template<> struct fex_gen_config<xcb_randr_get_crtc_transform_sizeof> {};
template<> struct fex_gen_config<xcb_randr_get_crtc_transform> {};
template<> struct fex_gen_config<xcb_randr_get_crtc_transform_unchecked> {};
template<> struct fex_gen_config<xcb_randr_get_crtc_transform_pending_filter_name> {};
template<> struct fex_gen_config<xcb_randr_get_crtc_transform_pending_filter_name_length> {};
template<> struct fex_gen_config<xcb_randr_get_crtc_transform_pending_filter_name_end> {};
template<> struct fex_gen_config<xcb_randr_get_crtc_transform_pending_params> {};
template<> struct fex_gen_config<xcb_randr_get_crtc_transform_pending_params_length> {};
template<> struct fex_gen_config<xcb_randr_get_crtc_transform_pending_params_end> {};
template<> struct fex_gen_config<xcb_randr_get_crtc_transform_current_filter_name> {};
template<> struct fex_gen_config<xcb_randr_get_crtc_transform_current_filter_name_length> {};
template<> struct fex_gen_config<xcb_randr_get_crtc_transform_current_filter_name_end> {};
template<> struct fex_gen_config<xcb_randr_get_crtc_transform_current_params> {};
template<> struct fex_gen_config<xcb_randr_get_crtc_transform_current_params_length> {};
template<> struct fex_gen_config<xcb_randr_get_crtc_transform_current_params_end> {};

template<> struct fex_gen_config<xcb_randr_get_crtc_transform_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_randr_get_panning> {};
template<> struct fex_gen_config<xcb_randr_get_panning_unchecked> {};
template<> struct fex_gen_config<xcb_randr_get_panning_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_randr_set_panning> {};
template<> struct fex_gen_config<xcb_randr_set_panning_unchecked> {};
template<> struct fex_gen_config<xcb_randr_set_panning_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_randr_set_output_primary_checked> {};
template<> struct fex_gen_config<xcb_randr_set_output_primary> {};
template<> struct fex_gen_config<xcb_randr_get_output_primary> {};
template<> struct fex_gen_config<xcb_randr_get_output_primary_unchecked> {};
template<> struct fex_gen_config<xcb_randr_get_output_primary_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_randr_get_providers_sizeof> {};
template<> struct fex_gen_config<xcb_randr_get_providers> {};
template<> struct fex_gen_config<xcb_randr_get_providers_unchecked> {};
template<> struct fex_gen_config<xcb_randr_get_providers_providers> {};
template<> struct fex_gen_config<xcb_randr_get_providers_providers_length> {};
template<> struct fex_gen_config<xcb_randr_get_providers_providers_end> {};

template<> struct fex_gen_config<xcb_randr_get_providers_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_randr_get_provider_info_sizeof> {};
template<> struct fex_gen_config<xcb_randr_get_provider_info> {};
template<> struct fex_gen_config<xcb_randr_get_provider_info_unchecked> {};
template<> struct fex_gen_config<xcb_randr_get_provider_info_crtcs> {};
template<> struct fex_gen_config<xcb_randr_get_provider_info_crtcs_length> {};
template<> struct fex_gen_config<xcb_randr_get_provider_info_crtcs_end> {};
template<> struct fex_gen_config<xcb_randr_get_provider_info_outputs> {};
template<> struct fex_gen_config<xcb_randr_get_provider_info_outputs_length> {};
template<> struct fex_gen_config<xcb_randr_get_provider_info_outputs_end> {};
template<> struct fex_gen_config<xcb_randr_get_provider_info_associated_providers> {};
template<> struct fex_gen_config<xcb_randr_get_provider_info_associated_providers_length> {};
template<> struct fex_gen_config<xcb_randr_get_provider_info_associated_providers_end> {};
template<> struct fex_gen_config<xcb_randr_get_provider_info_associated_capability> {};
template<> struct fex_gen_config<xcb_randr_get_provider_info_associated_capability_length> {};
template<> struct fex_gen_config<xcb_randr_get_provider_info_associated_capability_end> {};
template<> struct fex_gen_config<xcb_randr_get_provider_info_name> {};
template<> struct fex_gen_config<xcb_randr_get_provider_info_name_length> {};
template<> struct fex_gen_config<xcb_randr_get_provider_info_name_end> {};

template<> struct fex_gen_config<xcb_randr_get_provider_info_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_randr_set_provider_offload_sink_checked> {};
template<> struct fex_gen_config<xcb_randr_set_provider_offload_sink> {};
template<> struct fex_gen_config<xcb_randr_set_provider_output_source_checked> {};
template<> struct fex_gen_config<xcb_randr_set_provider_output_source> {};
template<> struct fex_gen_config<xcb_randr_list_provider_properties_sizeof> {};
template<> struct fex_gen_config<xcb_randr_list_provider_properties> {};
template<> struct fex_gen_config<xcb_randr_list_provider_properties_unchecked> {};
template<> struct fex_gen_config<xcb_randr_list_provider_properties_atoms> {};
template<> struct fex_gen_config<xcb_randr_list_provider_properties_atoms_length> {};
template<> struct fex_gen_config<xcb_randr_list_provider_properties_atoms_end> {};

template<> struct fex_gen_config<xcb_randr_list_provider_properties_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_randr_query_provider_property_sizeof> {};
template<> struct fex_gen_config<xcb_randr_query_provider_property> {};
template<> struct fex_gen_config<xcb_randr_query_provider_property_unchecked> {};
template<> struct fex_gen_config<xcb_randr_query_provider_property_valid_values> {};
template<> struct fex_gen_config<xcb_randr_query_provider_property_valid_values_length> {};
template<> struct fex_gen_config<xcb_randr_query_provider_property_valid_values_end> {};

template<> struct fex_gen_config<xcb_randr_query_provider_property_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_randr_configure_provider_property_sizeof> {};
template<> struct fex_gen_config<xcb_randr_configure_provider_property_checked> {};
template<> struct fex_gen_config<xcb_randr_configure_provider_property> {};
template<> struct fex_gen_config<xcb_randr_configure_provider_property_values> {};
template<> struct fex_gen_config<xcb_randr_configure_provider_property_values_length> {};
template<> struct fex_gen_config<xcb_randr_configure_provider_property_values_end> {};

template<> struct fex_gen_config<xcb_randr_change_provider_property_sizeof> {};
template<> struct fex_gen_config<xcb_randr_change_provider_property_checked> {};
template<> struct fex_gen_config<xcb_randr_change_provider_property> {};
template<> struct fex_gen_config<xcb_randr_change_provider_property_data> {};
template<> struct fex_gen_config<xcb_randr_change_provider_property_data_length> {};
template<> struct fex_gen_config<xcb_randr_change_provider_property_data_end> {};

template<> struct fex_gen_config<xcb_randr_delete_provider_property_checked> {};
template<> struct fex_gen_config<xcb_randr_delete_provider_property> {};
template<> struct fex_gen_config<xcb_randr_get_provider_property_sizeof> {};
template<> struct fex_gen_config<xcb_randr_get_provider_property> {};
template<> struct fex_gen_config<xcb_randr_get_provider_property_unchecked> {};
template<> struct fex_gen_config<xcb_randr_get_provider_property_data> {};
template<> struct fex_gen_config<xcb_randr_get_provider_property_data_length> {};
template<> struct fex_gen_config<xcb_randr_get_provider_property_data_end> {};

template<> struct fex_gen_config<xcb_randr_get_provider_property_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_randr_crtc_change_next> {};
template<> struct fex_gen_config<xcb_randr_crtc_change_end> {};
template<> struct fex_gen_config<xcb_randr_output_change_next> {};
template<> struct fex_gen_config<xcb_randr_output_change_end> {};
template<> struct fex_gen_config<xcb_randr_output_property_next> {};
template<> struct fex_gen_config<xcb_randr_output_property_end> {};
template<> struct fex_gen_config<xcb_randr_provider_change_next> {};
template<> struct fex_gen_config<xcb_randr_provider_change_end> {};
template<> struct fex_gen_config<xcb_randr_provider_property_next> {};
template<> struct fex_gen_config<xcb_randr_provider_property_end> {};
template<> struct fex_gen_config<xcb_randr_resource_change_next> {};
template<> struct fex_gen_config<xcb_randr_resource_change_end> {};
template<> struct fex_gen_config<xcb_randr_monitor_info_sizeof> {};
template<> struct fex_gen_config<xcb_randr_monitor_info_outputs> {};
template<> struct fex_gen_config<xcb_randr_monitor_info_outputs_length> {};
template<> struct fex_gen_config<xcb_randr_monitor_info_outputs_end> {};

template<> struct fex_gen_config<xcb_randr_monitor_info_next> {};
template<> struct fex_gen_config<xcb_randr_monitor_info_end> {};
template<> struct fex_gen_config<xcb_randr_get_monitors_sizeof> {};
template<> struct fex_gen_config<xcb_randr_get_monitors> {};
template<> struct fex_gen_config<xcb_randr_get_monitors_unchecked> {};
template<> struct fex_gen_config<xcb_randr_get_monitors_monitors_length> {};
template<> struct fex_gen_config<xcb_randr_get_monitors_monitors_iterator> {};
template<> struct fex_gen_config<xcb_randr_get_monitors_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_randr_set_monitor_sizeof> {};
template<> struct fex_gen_config<xcb_randr_set_monitor_checked> {};
template<> struct fex_gen_config<xcb_randr_set_monitor> {};
template<> struct fex_gen_config<xcb_randr_set_monitor_monitorinfo> {};
template<> struct fex_gen_config<xcb_randr_delete_monitor_checked> {};
template<> struct fex_gen_config<xcb_randr_delete_monitor> {};
template<> struct fex_gen_config<xcb_randr_create_lease_sizeof> {};
template<> struct fex_gen_config<xcb_randr_create_lease> {};
template<> struct fex_gen_config<xcb_randr_create_lease_unchecked> {};
template<> struct fex_gen_config<xcb_randr_create_lease_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_randr_create_lease_reply_fds> {};
template<> struct fex_gen_config<xcb_randr_free_lease_checked> {};
template<> struct fex_gen_config<xcb_randr_free_lease> {};
template<> struct fex_gen_config<xcb_randr_lease_notify_next> {};
template<> struct fex_gen_config<xcb_randr_lease_notify_end> {};
template<> struct fex_gen_config<xcb_randr_notify_data_next> {};
template<> struct fex_gen_config<xcb_randr_notify_data_end> {};
