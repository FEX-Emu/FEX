#include <common/GeneratorInterface.h>

#include <xcb/xfixes.h>
#include <xcb/xcbext.h>

template<auto>
struct fex_gen_config {
    unsigned version = 0;
};

template<typename>
struct fex_gen_type {};

template<> struct fex_gen_type<xcb_connection_t> : fexgen::opaque_type {};

void FEX_xcb_xfixes_init_extension(xcb_connection_t*, xcb_extension_t*);
size_t FEX_usable_size(void*);
void FEX_free_on_host(void*);

template<> struct fex_gen_config<FEX_xcb_xfixes_init_extension> : fexgen::custom_host_impl {};
template<> struct fex_gen_config<FEX_usable_size> : fexgen::custom_host_impl, fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<FEX_free_on_host> : fexgen::custom_host_impl, fexgen::custom_guest_entrypoint {};

template<> struct fex_gen_config<xcb_xfixes_query_version> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_xfixes_query_version_unchecked> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_xfixes_query_version_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_xfixes_change_save_set_checked> {};
template<> struct fex_gen_config<xcb_xfixes_change_save_set> {};
template<> struct fex_gen_config<xcb_xfixes_select_selection_input_checked> {};
template<> struct fex_gen_config<xcb_xfixes_select_selection_input> {};
template<> struct fex_gen_config<xcb_xfixes_select_cursor_input_checked> {};
template<> struct fex_gen_config<xcb_xfixes_select_cursor_input> {};
template<> struct fex_gen_config<xcb_xfixes_get_cursor_image_sizeof> {};
template<> struct fex_gen_config<xcb_xfixes_get_cursor_image> {};
template<> struct fex_gen_config<xcb_xfixes_get_cursor_image_unchecked> {};
template<> struct fex_gen_config<xcb_xfixes_get_cursor_image_cursor_image> {};
template<> struct fex_gen_config<xcb_xfixes_get_cursor_image_cursor_image_length> {};
template<> struct fex_gen_config<xcb_xfixes_get_cursor_image_cursor_image_end> {};

template<> struct fex_gen_config<xcb_xfixes_get_cursor_image_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_xfixes_region_next> {};
template<> struct fex_gen_config<xcb_xfixes_region_end> {};
template<> struct fex_gen_config<xcb_xfixes_create_region_sizeof> {};
template<> struct fex_gen_config<xcb_xfixes_create_region_checked> {};
template<> struct fex_gen_config<xcb_xfixes_create_region> {};
template<> struct fex_gen_config<xcb_xfixes_create_region_rectangles> {};
template<> struct fex_gen_config<xcb_xfixes_create_region_rectangles_length> {};
template<> struct fex_gen_config<xcb_xfixes_create_region_rectangles_iterator> {};

template<> struct fex_gen_config<xcb_xfixes_create_region_from_bitmap_checked> {};
template<> struct fex_gen_config<xcb_xfixes_create_region_from_bitmap> {};
template<> struct fex_gen_config<xcb_xfixes_create_region_from_window_checked> {};
template<> struct fex_gen_config<xcb_xfixes_create_region_from_window> {};
template<> struct fex_gen_config<xcb_xfixes_create_region_from_gc_checked> {};
template<> struct fex_gen_config<xcb_xfixes_create_region_from_gc> {};
template<> struct fex_gen_config<xcb_xfixes_create_region_from_picture_checked> {};
template<> struct fex_gen_config<xcb_xfixes_create_region_from_picture> {};
template<> struct fex_gen_config<xcb_xfixes_destroy_region_checked> {};
template<> struct fex_gen_config<xcb_xfixes_destroy_region> {};
template<> struct fex_gen_config<xcb_xfixes_set_region_sizeof> {};
template<> struct fex_gen_config<xcb_xfixes_set_region_checked> {};
template<> struct fex_gen_config<xcb_xfixes_set_region> {};
template<> struct fex_gen_config<xcb_xfixes_set_region_rectangles> {};
template<> struct fex_gen_config<xcb_xfixes_set_region_rectangles_length> {};
template<> struct fex_gen_config<xcb_xfixes_set_region_rectangles_iterator> {};

template<> struct fex_gen_config<xcb_xfixes_copy_region_checked> {};
template<> struct fex_gen_config<xcb_xfixes_copy_region> {};
template<> struct fex_gen_config<xcb_xfixes_union_region_checked> {};
template<> struct fex_gen_config<xcb_xfixes_union_region> {};
template<> struct fex_gen_config<xcb_xfixes_intersect_region_checked> {};
template<> struct fex_gen_config<xcb_xfixes_intersect_region> {};
template<> struct fex_gen_config<xcb_xfixes_subtract_region_checked> {};
template<> struct fex_gen_config<xcb_xfixes_subtract_region> {};
template<> struct fex_gen_config<xcb_xfixes_invert_region_checked> {};
template<> struct fex_gen_config<xcb_xfixes_invert_region> {};
template<> struct fex_gen_config<xcb_xfixes_translate_region_checked> {};
template<> struct fex_gen_config<xcb_xfixes_translate_region> {};
template<> struct fex_gen_config<xcb_xfixes_region_extents_checked> {};
template<> struct fex_gen_config<xcb_xfixes_region_extents> {};
template<> struct fex_gen_config<xcb_xfixes_fetch_region_sizeof> {};
template<> struct fex_gen_config<xcb_xfixes_fetch_region> {};
template<> struct fex_gen_config<xcb_xfixes_fetch_region_unchecked> {};
template<> struct fex_gen_config<xcb_xfixes_fetch_region_rectangles> {};
template<> struct fex_gen_config<xcb_xfixes_fetch_region_rectangles_length> {};
template<> struct fex_gen_config<xcb_xfixes_fetch_region_rectangles_iterator> {};

template<> struct fex_gen_config<xcb_xfixes_fetch_region_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_xfixes_set_gc_clip_region_checked> {};
template<> struct fex_gen_config<xcb_xfixes_set_gc_clip_region> {};
template<> struct fex_gen_config<xcb_xfixes_set_window_shape_region_checked> {};
template<> struct fex_gen_config<xcb_xfixes_set_window_shape_region> {};
template<> struct fex_gen_config<xcb_xfixes_set_picture_clip_region_checked> {};
template<> struct fex_gen_config<xcb_xfixes_set_picture_clip_region> {};
template<> struct fex_gen_config<xcb_xfixes_set_cursor_name_sizeof> {};
template<> struct fex_gen_config<xcb_xfixes_set_cursor_name_checked> {};
template<> struct fex_gen_config<xcb_xfixes_set_cursor_name> {};
template<> struct fex_gen_config<xcb_xfixes_set_cursor_name_name> {};
template<> struct fex_gen_config<xcb_xfixes_set_cursor_name_name_length> {};
template<> struct fex_gen_config<xcb_xfixes_set_cursor_name_name_end> {};

template<> struct fex_gen_config<xcb_xfixes_get_cursor_name_sizeof> {};
template<> struct fex_gen_config<xcb_xfixes_get_cursor_name> {};
template<> struct fex_gen_config<xcb_xfixes_get_cursor_name_unchecked> {};
template<> struct fex_gen_config<xcb_xfixes_get_cursor_name_name> {};
template<> struct fex_gen_config<xcb_xfixes_get_cursor_name_name_length> {};
template<> struct fex_gen_config<xcb_xfixes_get_cursor_name_name_end> {};

template<> struct fex_gen_config<xcb_xfixes_get_cursor_name_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_xfixes_get_cursor_image_and_name_sizeof> {};
template<> struct fex_gen_config<xcb_xfixes_get_cursor_image_and_name> {};
template<> struct fex_gen_config<xcb_xfixes_get_cursor_image_and_name_unchecked> {};
template<> struct fex_gen_config<xcb_xfixes_get_cursor_image_and_name_cursor_image> {};
template<> struct fex_gen_config<xcb_xfixes_get_cursor_image_and_name_cursor_image_length> {};
template<> struct fex_gen_config<xcb_xfixes_get_cursor_image_and_name_cursor_image_end> {};

template<> struct fex_gen_config<xcb_xfixes_get_cursor_image_and_name_name> {};
template<> struct fex_gen_config<xcb_xfixes_get_cursor_image_and_name_name_length> {};
template<> struct fex_gen_config<xcb_xfixes_get_cursor_image_and_name_name_end> {};

template<> struct fex_gen_config<xcb_xfixes_get_cursor_image_and_name_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_xfixes_change_cursor_checked> {};
template<> struct fex_gen_config<xcb_xfixes_change_cursor> {};
template<> struct fex_gen_config<xcb_xfixes_change_cursor_by_name_sizeof> {};
template<> struct fex_gen_config<xcb_xfixes_change_cursor_by_name_checked> {};
template<> struct fex_gen_config<xcb_xfixes_change_cursor_by_name> {};
template<> struct fex_gen_config<xcb_xfixes_change_cursor_by_name_name> {};
template<> struct fex_gen_config<xcb_xfixes_change_cursor_by_name_name_length> {};
template<> struct fex_gen_config<xcb_xfixes_change_cursor_by_name_name_end> {};

template<> struct fex_gen_config<xcb_xfixes_expand_region_checked> {};
template<> struct fex_gen_config<xcb_xfixes_expand_region> {};
template<> struct fex_gen_config<xcb_xfixes_hide_cursor_checked> {};
template<> struct fex_gen_config<xcb_xfixes_hide_cursor> {};
template<> struct fex_gen_config<xcb_xfixes_show_cursor_checked> {};
template<> struct fex_gen_config<xcb_xfixes_show_cursor> {};
template<> struct fex_gen_config<xcb_xfixes_barrier_next> {};
template<> struct fex_gen_config<xcb_xfixes_barrier_end> {};
template<> struct fex_gen_config<xcb_xfixes_create_pointer_barrier_sizeof> {};
template<> struct fex_gen_config<xcb_xfixes_create_pointer_barrier_checked> {};
template<> struct fex_gen_config<xcb_xfixes_create_pointer_barrier> {};
template<> struct fex_gen_config<xcb_xfixes_create_pointer_barrier_devices> {};
template<> struct fex_gen_config<xcb_xfixes_create_pointer_barrier_devices_length> {};
template<> struct fex_gen_config<xcb_xfixes_create_pointer_barrier_devices_end> {};

template<> struct fex_gen_config<xcb_xfixes_delete_pointer_barrier_checked> {};
template<> struct fex_gen_config<xcb_xfixes_delete_pointer_barrier> {};
