/*
$info$
tags: thunklibs|Vulkan
$end_info$
*/

#define VK_USE_PLATFORM_XLIB_XRANDR_EXT
#define VK_USE_PLATFORM_XLIB_KHR
#define VK_USE_PLATFORM_XCB_KHR
#define VK_USE_PLATFORM_WAYLAND_KHR
#include <vulkan/vulkan.h>

#include "common/Guest.h"

#include <cstdio>
#include <dlfcn.h>
#include <functional>
#include <string_view>
#include <unordered_map>

#include "thunks.inl"
#include "function_packs.inl"
#include "function_packs_public.inl"
#include "symbol_list.inl"

extern "C" {

// Maps Vulkan API function names to the address of a guest function which is
// linked to the corresponding host function pointer
const std::unordered_map<std::string_view, uintptr_t /* guest function address */> HostPtrInvokers =
    std::invoke([]() {
#define PAIR(name, unused) Ret[#name] = reinterpret_cast<uintptr_t>(fexfn_pack_hostcall_##name);
        std::unordered_map<std::string_view, uintptr_t> Ret;
        FOREACH_internal_SYMBOL(PAIR)
        return Ret;
#undef PAIR
    });

PFN_vkVoidFunction vkGetDeviceProcAddr(VkDevice a_0,const char* a_1){
    auto Ret = fexfn_pack_vkGetDeviceProcAddr(a_0, a_1);
    if (!Ret) {
        return nullptr;
    }
    auto It = HostPtrInvokers.find(a_1);
    if (It == HostPtrInvokers.end() || !It->second) {
      fprintf(stderr, "\tvkGetDeviceProcAddr: Couldn't find Guest symbol: '%s'\n", a_1);
      __builtin_trap();
    }
    LinkHostAddressToGuestFunction((uintptr_t)Ret, It->second);
    return Ret;
}

PFN_vkVoidFunction vkGetInstanceProcAddr(VkInstance a_0,const char* a_1){
    if (a_1 == std::string_view { "vkGetDeviceProcAddr" }) {
        return (PFN_vkVoidFunction)vkGetDeviceProcAddr;
    } else {
        auto Ret = fexfn_pack_vkGetInstanceProcAddr(a_0, a_1);
        if (!Ret) {
            return nullptr;
        }
        auto It = HostPtrInvokers.find(a_1);
        if (It == HostPtrInvokers.end() || !It->second) {
          fprintf(stderr, "\tvkGetInstanceProcAddr: Couldn't find Guest symbol: '%s'\n", a_1);
          __builtin_trap();
        }
        LinkHostAddressToGuestFunction((uintptr_t)Ret, It->second);
        return Ret;
    }
}

}

LOAD_LIB(libvulkan)
