#include <common/GeneratorInterface.h>

#include <xcb/dri2.h>

template<auto>
struct fex_gen_config;

void FEX_xcb_dri2_init_extension(xcb_connection_t*, xcb_extension_t*);
size_t FEX_usable_size(void*);
void FEX_free_on_host(void*);

template<> struct fex_gen_type<xcb_connection_t> : fexgen::opaque_to_guest {};
template<> struct fex_gen_type<xcb_extension_t> : fexgen::opaque_to_guest {};

template<> struct fex_gen_config<FEX_xcb_dri2_init_extension> : fexgen::custom_host_impl {};
template<> struct fex_gen_config<FEX_usable_size> : fexgen::custom_host_impl, fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_param<FEX_usable_size, 0, void*> : fexgen::ptr_is_untyped_address {};
template<> struct fex_gen_config<FEX_free_on_host> : fexgen::custom_host_impl, fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_param<FEX_free_on_host, 0, void*> : fexgen::ptr_is_untyped_address {};

template<> struct fex_gen_config<xcb_dri2_dri2_buffer_next> {};
template<> struct fex_gen_config<xcb_dri2_dri2_buffer_end> {};
template<> struct fex_gen_config<xcb_dri2_attach_format_next> {};
template<> struct fex_gen_config<xcb_dri2_attach_format_end> {};
template<> struct fex_gen_config<xcb_dri2_query_version> {};
template<> struct fex_gen_config<xcb_dri2_query_version_unchecked> {};

template<> struct fex_gen_config<xcb_dri2_query_version_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_param<xcb_dri2_query_version_reply, 2, xcb_generic_error_t**> : fexgen::ptr_todo_only64 {};

// TODO: _sizeof functions compute the size of serialized objects, so this parameter can be treated as an opaque object of unspecified type
template<> struct fex_gen_config<xcb_dri2_connect_sizeof> {};
template<> struct fex_gen_param<xcb_dri2_connect_sizeof, 0, const void*> : fexgen::ptr_todo_only64 {};

template<> struct fex_gen_config<xcb_dri2_connect> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_dri2_connect_unchecked> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_dri2_connect_driver_name> {};
template<> struct fex_gen_config<xcb_dri2_connect_driver_name_length> {};
template<> struct fex_gen_config<xcb_dri2_connect_driver_name_end> {};
template<> struct fex_gen_config<xcb_dri2_connect_alignment_pad> {};
template<> struct fex_gen_config<xcb_dri2_connect_alignment_pad_length> {};
template<> struct fex_gen_config<xcb_dri2_connect_alignment_pad_end> {};
template<> struct fex_gen_config<xcb_dri2_connect_device_name> {};
template<> struct fex_gen_config<xcb_dri2_connect_device_name_length> {};
template<> struct fex_gen_config<xcb_dri2_connect_device_name_end> {};

template<> struct fex_gen_config<xcb_dri2_connect_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_param<xcb_dri2_connect_reply, 2, xcb_generic_error_t**> : fexgen::ptr_todo_only64 {};

template<> struct fex_gen_config<xcb_dri2_authenticate> {};
template<> struct fex_gen_config<xcb_dri2_authenticate_unchecked> {};

template<> struct fex_gen_config<xcb_dri2_authenticate_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_param<xcb_dri2_authenticate_reply, 2, xcb_generic_error_t**> : fexgen::ptr_todo_only64 {};

template<> struct fex_gen_config<xcb_dri2_create_drawable_checked> {};
template<> struct fex_gen_config<xcb_dri2_create_drawable> {};
template<> struct fex_gen_config<xcb_dri2_destroy_drawable_checked> {};
template<> struct fex_gen_config<xcb_dri2_destroy_drawable> {};

template<> struct fex_gen_config<xcb_dri2_get_buffers_sizeof> {};
template<> struct fex_gen_param<xcb_dri2_get_buffers_sizeof, 0, const void*> : fexgen::ptr_todo_only64 {};

template<> struct fex_gen_config<xcb_dri2_get_buffers> {};
template<> struct fex_gen_config<xcb_dri2_get_buffers_unchecked> {};
template<> struct fex_gen_config<xcb_dri2_get_buffers_buffers> {};
template<> struct fex_gen_config<xcb_dri2_get_buffers_buffers_length> {};
template<> struct fex_gen_config<xcb_dri2_get_buffers_buffers_iterator> {};

template<> struct fex_gen_config<xcb_dri2_get_buffers_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_param<xcb_dri2_get_buffers_reply, 2, xcb_generic_error_t**> : fexgen::ptr_todo_only64 {};

template<> struct fex_gen_config<xcb_dri2_copy_region> {};
template<> struct fex_gen_config<xcb_dri2_copy_region_unchecked> {};

template<> struct fex_gen_config<xcb_dri2_copy_region_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_param<xcb_dri2_copy_region_reply, 2, xcb_generic_error_t**> : fexgen::ptr_todo_only64 {};

template<> struct fex_gen_config<xcb_dri2_get_buffers_with_format_sizeof> {};
template<> struct fex_gen_param<xcb_dri2_get_buffers_with_format_sizeof, 0, const void*> : fexgen::ptr_todo_only64 {};

template<> struct fex_gen_config<xcb_dri2_get_buffers_with_format> {};
template<> struct fex_gen_config<xcb_dri2_get_buffers_with_format_unchecked> {};
template<> struct fex_gen_config<xcb_dri2_get_buffers_with_format_buffers> {};
template<> struct fex_gen_config<xcb_dri2_get_buffers_with_format_buffers_length> {};
template<> struct fex_gen_config<xcb_dri2_get_buffers_with_format_buffers_iterator> {};

template<> struct fex_gen_config<xcb_dri2_get_buffers_with_format_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_param<xcb_dri2_get_buffers_with_format_reply, 2, xcb_generic_error_t**> : fexgen::ptr_todo_only64 {};

template<> struct fex_gen_config<xcb_dri2_swap_buffers> {};
template<> struct fex_gen_config<xcb_dri2_swap_buffers_unchecked> {};

template<> struct fex_gen_config<xcb_dri2_swap_buffers_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_param<xcb_dri2_swap_buffers_reply, 2, xcb_generic_error_t**> : fexgen::ptr_todo_only64 {};

template<> struct fex_gen_config<xcb_dri2_get_msc> {};
template<> struct fex_gen_config<xcb_dri2_get_msc_unchecked> {};

template<> struct fex_gen_config<xcb_dri2_get_msc_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_param<xcb_dri2_get_msc_reply, 2, xcb_generic_error_t**> : fexgen::ptr_todo_only64 {};

template<> struct fex_gen_config<xcb_dri2_wait_msc> {};
template<> struct fex_gen_config<xcb_dri2_wait_msc_unchecked> {};

template<> struct fex_gen_config<xcb_dri2_wait_msc_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_param<xcb_dri2_wait_msc_reply, 2, xcb_generic_error_t**> : fexgen::ptr_todo_only64 {};

template<> struct fex_gen_config<xcb_dri2_wait_sbc> {};
template<> struct fex_gen_config<xcb_dri2_wait_sbc_unchecked> {};

template<> struct fex_gen_config<xcb_dri2_wait_sbc_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_param<xcb_dri2_wait_sbc_reply, 2, xcb_generic_error_t**> : fexgen::ptr_todo_only64 {};

template<> struct fex_gen_config<xcb_dri2_swap_interval_checked> {};
template<> struct fex_gen_config<xcb_dri2_swap_interval> {};
template<> struct fex_gen_config<xcb_dri2_get_param> {};
template<> struct fex_gen_config<xcb_dri2_get_param_unchecked> {};

template<> struct fex_gen_config<xcb_dri2_get_param_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_param<xcb_dri2_get_param_reply, 2, xcb_generic_error_t**> : fexgen::ptr_todo_only64 {};
