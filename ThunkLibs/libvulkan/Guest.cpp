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

#include "thunkgen_guest_libvulkan.inl"

extern "C" {

#if 0
// Maps Vulkan API function names to the address of a guest function which is
// linked to the corresponding host function pointer
const std::unordered_map<std::string_view, uintptr_t /* guest function address */> HostPtrInvokers =
    std::invoke([]() {
#define PAIR(name, unused) Ret[#name] = reinterpret_cast<uintptr_t>(GetCallerForHostFunction(name));
        std::unordered_map<std::string_view, uintptr_t> Ret;
        FOREACH_internal_SYMBOL(PAIR);
        return Ret;
#undef PAIR
    });

// This variable controls the behavior of vkGetDevice/InstanceProcAddr for functions we don't know the signature of:
// - if false (default), we return a nullptr (since the application might have a fallback code path)
// - if true, we return a stub function that fatally errors upon being called
constexpr bool stub_unknown_functions = false;

// Fatally erroring function with a thunk-like interface. This is used as a placeholder for unknown Vulkan functions
[[noreturn]] static void FatalError(void* raw_args) {
    auto called_function = reinterpret_cast<PackedArguments<void, uintptr_t>*>(raw_args)->a0;
    fprintf(stderr, "FATAL: Called unknown Vulkan function at address %p\n", reinterpret_cast<void*>(called_function));
    __builtin_trap();
}

static PFN_vkVoidFunction MakeGuestCallable(const char* origin, PFN_vkVoidFunction func, const char* name) {
    auto It = HostPtrInvokers.find(name);
    if (It == HostPtrInvokers.end()) {
        fprintf(stderr, "%s: Unknown Vulkan function at address %p: %s\n", origin, func, name);
        if (stub_unknown_functions) {
            const auto StubHostPtrInvoker = CallHostFunction<FatalError, void>;
            LinkAddressToFunction((uintptr_t)func, reinterpret_cast<uintptr_t>(StubHostPtrInvoker));
            return func;
        }
        return nullptr;
    }
    fprintf(stderr, "Linking address %p to host invoker %#zx\n", func, It->second);
    LinkAddressToFunction((uintptr_t)func, It->second);
    return func;
}

PFN_vkVoidFunction vkGetDeviceProcAddr(VkDevice a_0,const char* a_1){
    auto Ret = fexfn_pack_vkGetDeviceProcAddr(a_0, a_1);
    if (!Ret) {
        return nullptr;
    }
    return MakeGuestCallable(__FUNCTION__, Ret, a_1);
}

PFN_vkVoidFunction vkGetInstanceProcAddr(VkInstance a_0,const char* a_1){
    if (a_1 == std::string_view { "vkGetDeviceProcAddr" }) {
        return (PFN_vkVoidFunction)vkGetDeviceProcAddr;
    } else {
        auto Ret = fexfn_pack_vkGetInstanceProcAddr(a_0, a_1);
        if (!Ret) {
            return nullptr;
        }
        return MakeGuestCallable(__FUNCTION__, Ret, a_1);
    }
}

#endif
}
LOAD_LIB(libvulkan)
