#include <common/GeneratorInterface.h>

#include <xcb/sync.h>

template<auto>
struct fex_gen_config {
    unsigned version = 1;
};

void FEX_xcb_sync_init_extension(xcb_connection_t*, xcb_extension_t*);
size_t FEX_usable_size(void*);
void FEX_free_on_host(void*);

template<> struct fex_gen_type<xcb_connection_t> : fexgen::opaque_to_guest {};
template<> struct fex_gen_type<xcb_extension_t> : fexgen::opaque_to_guest {};

template<> struct fex_gen_config<FEX_xcb_sync_init_extension> : fexgen::custom_host_impl {};
template<> struct fex_gen_config<FEX_usable_size> : fexgen::custom_host_impl, fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_param<FEX_usable_size, 0, void*> : fexgen::ptr_is_untyped_address {};
template<> struct fex_gen_config<FEX_free_on_host> : fexgen::custom_host_impl, fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_param<FEX_free_on_host, 0, void*> : fexgen::ptr_is_untyped_address {};

template<> struct fex_gen_config<xcb_sync_alarm_next> {};
template<> struct fex_gen_config<xcb_sync_alarm_end> {};
template<> struct fex_gen_config<xcb_sync_counter_next> {};
template<> struct fex_gen_config<xcb_sync_counter_end> {};
template<> struct fex_gen_config<xcb_sync_fence_next> {};
template<> struct fex_gen_config<xcb_sync_fence_end> {};
template<> struct fex_gen_config<xcb_sync_int64_next> {};
template<> struct fex_gen_config<xcb_sync_int64_end> {};

// TODO: _sizeof functions compute the size of serialized objects, so this parameter can be treated as an opaque object of unspecified type
template<> struct fex_gen_config<xcb_sync_systemcounter_sizeof> {};
template<> struct fex_gen_param<xcb_sync_systemcounter_sizeof, 0, const void*> : fexgen::ptr_todo_only64 {};

template<> struct fex_gen_config<xcb_sync_systemcounter_name> {};
template<> struct fex_gen_config<xcb_sync_systemcounter_name_length> {};
template<> struct fex_gen_config<xcb_sync_systemcounter_name_end> {};
template<> struct fex_gen_config<xcb_sync_systemcounter_next> {};
template<> struct fex_gen_config<xcb_sync_systemcounter_end> {};
template<> struct fex_gen_config<xcb_sync_trigger_next> {};
template<> struct fex_gen_config<xcb_sync_trigger_end> {};
template<> struct fex_gen_config<xcb_sync_waitcondition_next> {};
template<> struct fex_gen_config<xcb_sync_waitcondition_end> {};
template<> struct fex_gen_config<xcb_sync_initialize> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_sync_initialize_unchecked> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_sync_initialize_reply> : fexgen::custom_guest_entrypoint {};

template<> struct fex_gen_config<xcb_sync_list_system_counters_sizeof> {};
template<> struct fex_gen_param<xcb_sync_list_system_counters_sizeof, 0, const void*> : fexgen::ptr_todo_only64 {};

template<> struct fex_gen_config<xcb_sync_list_system_counters> {};
template<> struct fex_gen_config<xcb_sync_list_system_counters_unchecked> {};
template<> struct fex_gen_config<xcb_sync_list_system_counters_counters_length> {};
template<> struct fex_gen_config<xcb_sync_list_system_counters_counters_iterator> {};
template<> struct fex_gen_config<xcb_sync_list_system_counters_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_sync_create_counter_checked> {};
template<> struct fex_gen_config<xcb_sync_create_counter> {};
template<> struct fex_gen_config<xcb_sync_destroy_counter_checked> {};
template<> struct fex_gen_config<xcb_sync_destroy_counter> {};
template<> struct fex_gen_config<xcb_sync_query_counter> {};
template<> struct fex_gen_config<xcb_sync_query_counter_unchecked> {};
template<> struct fex_gen_config<xcb_sync_query_counter_reply> : fexgen::custom_guest_entrypoint {};

template<> struct fex_gen_config<xcb_sync_await_sizeof> {};
template<> struct fex_gen_param<xcb_sync_await_sizeof, 0, const void*> : fexgen::ptr_todo_only64 {};

template<> struct fex_gen_config<xcb_sync_await_checked> {};
template<> struct fex_gen_config<xcb_sync_await> {};
template<> struct fex_gen_config<xcb_sync_await_wait_list> {};
template<> struct fex_gen_config<xcb_sync_await_wait_list_length> {};
template<> struct fex_gen_config<xcb_sync_await_wait_list_iterator> {};

template<> struct fex_gen_config<xcb_sync_change_counter_checked> {};
template<> struct fex_gen_config<xcb_sync_change_counter> {};
template<> struct fex_gen_config<xcb_sync_set_counter_checked> {};
template<> struct fex_gen_config<xcb_sync_set_counter> {};

// TODO: Stable ABI (hopefully?)
template<> struct fex_gen_config<xcb_sync_create_alarm_value_list_serialize> {};
template<> struct fex_gen_param<xcb_sync_create_alarm_value_list_serialize, 0, void**> : fexgen::ptr_todo_only64 {};

// TODO: Stable ABI (hopefully?)
template<> struct fex_gen_config<xcb_sync_create_alarm_value_list_unpack> {};
template<> struct fex_gen_param<xcb_sync_create_alarm_value_list_unpack, 0, const void*> : fexgen::ptr_todo_only64 {};

template<> struct fex_gen_config<xcb_sync_create_alarm_value_list_sizeof> {};
template<> struct fex_gen_param<xcb_sync_create_alarm_value_list_sizeof, 0, const void*> : fexgen::ptr_todo_only64 {};

template<> struct fex_gen_config<xcb_sync_create_alarm_sizeof> {};
template<> struct fex_gen_param<xcb_sync_create_alarm_sizeof, 0, const void*> : fexgen::ptr_todo_only64 {};

// TODO: Stable ABI (hopefully?)
template<> struct fex_gen_config<xcb_sync_create_alarm_checked> {};
template<> struct fex_gen_param<xcb_sync_create_alarm_checked, 3, const void*> : fexgen::ptr_todo_only64 {};

// TODO: Stable ABI (hopefully?)
template<> struct fex_gen_config<xcb_sync_create_alarm> {};
template<> struct fex_gen_param<xcb_sync_create_alarm, 3, const void*> : fexgen::ptr_todo_only64 {};

template<> struct fex_gen_config<xcb_sync_create_alarm_aux_checked> {};
template<> struct fex_gen_config<xcb_sync_create_alarm_aux> {};
template<> struct fex_gen_config<xcb_sync_create_alarm_value_list> {};

// TODO: Stable ABI (hopefully?)
template<> struct fex_gen_config<xcb_sync_change_alarm_value_list_serialize> {};
template<> struct fex_gen_param<xcb_sync_change_alarm_value_list_serialize, 0, void**> : fexgen::ptr_todo_only64 {};

// TODO: Stable ABI (hopefully?)
template<> struct fex_gen_config<xcb_sync_change_alarm_value_list_unpack> {};
template<> struct fex_gen_param<xcb_sync_change_alarm_value_list_unpack, 0, const void*> : fexgen::ptr_todo_only64 {};

template<> struct fex_gen_config<xcb_sync_change_alarm_value_list_sizeof> {};
template<> struct fex_gen_param<xcb_sync_change_alarm_value_list_sizeof, 0, const void*> : fexgen::ptr_todo_only64 {};

template<> struct fex_gen_config<xcb_sync_change_alarm_sizeof> {};
template<> struct fex_gen_param<xcb_sync_change_alarm_sizeof, 0, const void*> : fexgen::ptr_todo_only64 {};

// TODO: Stable ABI (hopefully?)
template<> struct fex_gen_config<xcb_sync_change_alarm_checked> {};
template<> struct fex_gen_param<xcb_sync_change_alarm_checked, 3, const void*> : fexgen::ptr_todo_only64 {};

// TODO: Stable ABI (hopefully?)
template<> struct fex_gen_config<xcb_sync_change_alarm> {};
template<> struct fex_gen_param<xcb_sync_change_alarm, 3, const void*> : fexgen::ptr_todo_only64 {};

template<> struct fex_gen_config<xcb_sync_change_alarm_aux_checked> {};
template<> struct fex_gen_config<xcb_sync_change_alarm_aux> {};
template<> struct fex_gen_config<xcb_sync_change_alarm_value_list> {};
template<> struct fex_gen_config<xcb_sync_destroy_alarm_checked> {};
template<> struct fex_gen_config<xcb_sync_destroy_alarm> {};
template<> struct fex_gen_config<xcb_sync_query_alarm> {};
template<> struct fex_gen_config<xcb_sync_query_alarm_unchecked> {};
template<> struct fex_gen_config<xcb_sync_query_alarm_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_sync_set_priority_checked> {};
template<> struct fex_gen_config<xcb_sync_set_priority> {};
template<> struct fex_gen_config<xcb_sync_get_priority> {};
template<> struct fex_gen_config<xcb_sync_get_priority_unchecked> {};
template<> struct fex_gen_config<xcb_sync_get_priority_reply> : fexgen::custom_guest_entrypoint {};
template<> struct fex_gen_config<xcb_sync_create_fence_checked> {};
template<> struct fex_gen_config<xcb_sync_create_fence> {};
template<> struct fex_gen_config<xcb_sync_trigger_fence_checked> {};
template<> struct fex_gen_config<xcb_sync_trigger_fence> {};
template<> struct fex_gen_config<xcb_sync_reset_fence_checked> {};
template<> struct fex_gen_config<xcb_sync_reset_fence> {};
template<> struct fex_gen_config<xcb_sync_destroy_fence_checked> {};
template<> struct fex_gen_config<xcb_sync_destroy_fence> {};
template<> struct fex_gen_config<xcb_sync_query_fence> {};
template<> struct fex_gen_config<xcb_sync_query_fence_unchecked> {};
template<> struct fex_gen_config<xcb_sync_query_fence_reply> : fexgen::custom_guest_entrypoint {};

template<> struct fex_gen_config<xcb_sync_await_fence_sizeof> {};
template<> struct fex_gen_param<xcb_sync_await_fence_sizeof, 0, const void*> : fexgen::ptr_todo_only64 {};

template<> struct fex_gen_config<xcb_sync_await_fence_checked> {};
template<> struct fex_gen_config<xcb_sync_await_fence> {};
template<> struct fex_gen_config<xcb_sync_await_fence_fence_list> {};
template<> struct fex_gen_config<xcb_sync_await_fence_fence_list_length> {};
template<> struct fex_gen_config<xcb_sync_await_fence_fence_list_end> {};
