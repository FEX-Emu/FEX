#define VK_USE_PLATFORM_XLIB_XRANDR_EXT
#define VK_USE_PLATFORM_XLIB_KHR
#define VK_USE_PLATFORM_XCB_KHR
#define VK_USE_PLATFORM_WAYLAND_KHR
#include <vulkan/vulkan.h>

#include "common/Host.h"

#include <mutex>
#include <unordered_map>

#include <dlfcn.h>

#include "ldr_ptrs.inl"

#include "symbol_list.inl"

static bool SetupDev{};
static bool SetupInstance{};
std::mutex SetupMutex{};

static std::unordered_map<std::string_view,PFN_vkVoidFunction*> PtrsToLookUp{};


#define CONCAT(a, b, c) a##_##b##_##c
#define EVAL(a, b) CONCAT(fexldr_ptr, a, b)
#define LDR_PTR(fn) EVAL(LIBLIB_NAME, fn)

static void DoSetupWithDevice(VkDevice dev) {
    std::unique_lock lk {SetupMutex};

    auto add_ptr = [&](const char* name) {
        auto Lookup = PtrsToLookUp.find(name);
        if (Lookup != PtrsToLookUp.end() && *Lookup->second == nullptr) {
            auto Res = LDR_PTR(vkGetDeviceProcAddr)(dev, name);
            if (Res) {
                *Lookup->second = Res;
            }
        }
    };

#define PAIR(name, unused) add_ptr(#name);
FOREACH_SYMBOL(PAIR)
#undef PAIR

    SetupDev = true;
}

static void DoSetupWithInstance(VkInstance instance) {
    std::unique_lock lk {SetupMutex};

    auto add_ptr = [&](const char* name) {
        auto Lookup = PtrsToLookUp.find(name);
        auto Res = LDR_PTR(vkGetInstanceProcAddr)(instance, name);
        if (Res) {
            *Lookup->second = Res;
        }
    };

#define PAIR(name, unused) add_ptr(#name);
FOREACH_SYMBOL(PAIR);
#undef PAIR

    // Only do this lookup once.
    // NOTE: If vkGetInstanceProcAddr was called with a null instance, only a few function pointers will be filled with non-null values, so we do repeat the lookup in that case
    if (instance) {
        SetupInstance = true;
    }
}

#define FEXFN_IMPL3(a, b, c) a##_##b##_##c
#define FEXFN_IMPL2(a, b) FEXFN_IMPL3(fexfn_impl, a, b)
#define FEXFN_IMPL(fn) FEXFN_IMPL2(LIBLIB_NAME, fn)

static PFN_vkVoidFunction FEXFN_IMPL(vkGetDeviceProcAddr)(VkDevice a_0, const char* a_1) {
  if (!SetupDev) {
    DoSetupWithDevice(a_0);
  }

  // Just return the host facing function pointer
  // The guest will handle mapping if this exists
  auto ret = LDR_PTR(vkGetDeviceProcAddr)(a_0, a_1);
  return ret;
}

static PFN_vkVoidFunction FEXFN_IMPL(vkGetInstanceProcAddr)(VkInstance a_0, const char* a_1) {
  if (!SetupInstance) {
    DoSetupWithInstance(a_0);
  }

  // Just return the host facing function pointer
  // The guest will handle mapping if it exists
  auto ret = LDR_PTR(vkGetInstanceProcAddr)(a_0, a_1);
  return ret;
}

static VkResult FEXFN_IMPL(vkCreateShaderModule)(VkDevice a_0, const VkShaderModuleCreateInfo* a_1, const VkAllocationCallbacks* a_2, VkShaderModule* a_3) {
    return LDR_PTR(vkCreateShaderModule)(a_0, a_1, nullptr, a_3);
}

static VkResult FEXFN_IMPL(vkCreateInstance)(const VkInstanceCreateInfo* a_0, const VkAllocationCallbacks* a_1, VkInstance* a_2) {
  return LDR_PTR(vkCreateInstance)(a_0, nullptr, a_2);
}

static VkResult FEXFN_IMPL(vkCreateDevice)(VkPhysicalDevice a_0, const VkDeviceCreateInfo* a_1, const VkAllocationCallbacks* a_2, VkDevice* a_3){
  return LDR_PTR(vkCreateDevice)(a_0, a_1, nullptr, a_3);
}

static VkResult FEXFN_IMPL(vkAllocateMemory)(VkDevice a_0, const VkMemoryAllocateInfo* a_1, const VkAllocationCallbacks* a_2, VkDeviceMemory* a_3){
  return LDR_PTR(vkAllocateMemory)(a_0, a_1, nullptr, a_3);
}

static void FEXFN_IMPL(vkFreeMemory)(VkDevice a_0, VkDeviceMemory a_1, const VkAllocationCallbacks* a_2) {
  LDR_PTR(vkFreeMemory)(a_0, a_1, nullptr);
}


#include "function_unpacks.inl"

static ExportEntry exports[] = {
    #include "tab_function_unpacks.inl"
    { nullptr, nullptr }
};

static void DoSetup() {
    // Initialize unordered_map from generated initializer-list
#define PAIR(name, unused) { #name, (PFN_vkVoidFunction*)&LDR_PTR(name) },
    PtrsToLookUp = {
        FOREACH_SYMBOL(PAIR)
    };
#undef PAIR
}

#include "ldr.inl"

#define LDR_HANDLE3(a) fexldr_ptr##_##a##_##so
#define LDR_HANDLE2(a) LDR_HANDLE3(a)
#define LDR_HANDLE LDR_HANDLE2(LIBLIB_NAME)

static void init_func() {
  // Initialize some initial pointers that we can get while loading
  (void*&)LDR_PTR(vkGetInstanceProcAddr) = dlsym(LDR_HANDLE, "vkGetInstanceProcAddr");
  if (LDR_PTR(vkGetInstanceProcAddr) == nullptr) {
    (void*&)LDR_PTR(vkGetInstanceProcAddr) = dlsym(LDR_HANDLE, "vk_icdGetInstanceProcAddr");
  }

  (void*&)LDR_PTR(vkEnumerateInstanceVersion) = (void *) LDR_PTR(vkGetInstanceProcAddr)(nullptr, "vkEnumerateInstanceVersion");
  (void*&)LDR_PTR(vkEnumerateInstanceExtensionProperties) = (void *) LDR_PTR(vkGetInstanceProcAddr)(nullptr, "vkEnumerateInstanceExtensionProperties");
  (void*&)LDR_PTR(vkEnumerateInstanceLayerProperties) = (void*) LDR_PTR(vkGetInstanceProcAddr)(nullptr, "vkEnumerateInstanceLayerProperties");
  (void*&)LDR_PTR(vkCreateInstance) = (void*) LDR_PTR(vkGetInstanceProcAddr)(nullptr, "vkCreateInstance");

  DoSetup();
}

#define DOEXPORT_INIT(name, init) EXPORTS_INIT(name, init)
DOEXPORT_INIT(LIBLIB_NAME, init_func)
