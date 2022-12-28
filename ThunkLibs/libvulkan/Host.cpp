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

#include "common/Host.h"

#include <cstring>
#include <mutex>
#include <unordered_map>

#include <dlfcn.h>

#include "thunkgen_host_libvulkan.inl"

static bool SetupInstance{};
static std::mutex SetupMutex{};

#define LDR_PTR(fn) fexldr_ptr_libvulkan_##fn

#if 0
static void DoSetupWithInstance(VkInstance instance) {
    std::unique_lock lk {SetupMutex};

    // Needed since the Guest-endpoint calls without a function pointer
    // TODO: Support use of multiple instances
    (void*&)LDR_PTR(vkGetDeviceProcAddr) = (void*)LDR_PTR(vkGetInstanceProcAddr)(instance, "vkGetDeviceProcAddr");
    if (LDR_PTR(vkGetDeviceProcAddr) == nullptr) {
      std::abort();
    }

    // Query pointers for functions customized below
    (void*&)LDR_PTR(vkCreateInstance) = (void*)LDR_PTR(vkGetInstanceProcAddr)(instance, "vkCreateInstance");
    (void*&)LDR_PTR(vkCreateDevice) = (void*)LDR_PTR(vkGetInstanceProcAddr)(instance, "vkCreateDevice");

    // Only do this lookup once.
    // NOTE: If vkGetInstanceProcAddr was called with a null instance, only a few function pointers will be filled with non-null values, so we do repeat the lookup in that case
    if (instance) {
        SetupInstance = true;
    }
}

#define FEXFN_IMPL(fn) fexfn_impl_libvulkan_##fn

// Functions with callbacks are overridden to ignore the guest-side callbacks

static VkResult FEXFN_IMPL(vkCreateShaderModule)(VkDevice a_0, const VkShaderModuleCreateInfo* a_1, const VkAllocationCallbacks* a_2, VkShaderModule* a_3) {
  (void*&)LDR_PTR(vkCreateShaderModule) = (void*)LDR_PTR(vkGetDeviceProcAddr)(a_0, "vkCreateShaderModule");
  return LDR_PTR(vkCreateShaderModule)(a_0, a_1, nullptr, a_3);
}

static VkBool32 DummyVkDebugReportCallback(VkDebugReportFlagsEXT, VkDebugReportObjectTypeEXT, uint64_t, size_t,
                                           int32_t, const char*, const char*, void*) {
  return VK_FALSE;
}

static VkResult FEXFN_IMPL(vkCreateInstance)(const VkInstanceCreateInfo* a_0, const VkAllocationCallbacks* a_1, VkInstance* a_2) {
  const VkInstanceCreateInfo* vk_struct_base = a_0;
  for (const VkBaseInStructure* vk_struct = reinterpret_cast<const VkBaseInStructure*>(vk_struct_base); vk_struct->pNext; vk_struct = vk_struct->pNext) {
    // Override guest callbacks used for VK_EXT_debug_report
    if (reinterpret_cast<const VkBaseInStructure*>(vk_struct->pNext)->sType == VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT) {
      // Overwrite the pNext pointer, ignoring its const-qualifier
      const_cast<VkBaseInStructure*>(vk_struct)->pNext = vk_struct->pNext->pNext;

      // If we copied over a nullptr for pNext then early exit
      if (!vk_struct->pNext)
        break;
    }
  }

  return LDR_PTR(vkCreateInstance)(vk_struct_base, nullptr, a_2);
}

static VkResult FEXFN_IMPL(vkCreateDevice)(VkPhysicalDevice a_0, const VkDeviceCreateInfo* a_1, const VkAllocationCallbacks* a_2, VkDevice* a_3){
  return LDR_PTR(vkCreateDevice)(a_0, a_1, nullptr, a_3);
}

static VkResult FEXFN_IMPL(vkAllocateMemory)(VkDevice a_0, const VkMemoryAllocateInfo* a_1, const VkAllocationCallbacks* a_2, VkDeviceMemory* a_3){
  (void*&)LDR_PTR(vkAllocateMemory) = (void*)LDR_PTR(vkGetDeviceProcAddr)(a_0, "vkAllocateMemory");
  return LDR_PTR(vkAllocateMemory)(a_0, a_1, nullptr, a_3);
}

static void FEXFN_IMPL(vkFreeMemory)(VkDevice a_0, VkDeviceMemory a_1, const VkAllocationCallbacks* a_2) {
  (void*&)LDR_PTR(vkFreeMemory) = (void*)LDR_PTR(vkGetDeviceProcAddr)(a_0, "vkFreeMemory");
  LDR_PTR(vkFreeMemory)(a_0, a_1, nullptr);
}

static VkResult FEXFN_IMPL(vkCreateDebugReportCallbackEXT)(VkInstance a_0, const VkDebugReportCallbackCreateInfoEXT* a_1, const VkAllocationCallbacks* a_2, VkDebugReportCallbackEXT* a_3) {
  VkDebugReportCallbackCreateInfoEXT overridden_callback = *a_1;
  overridden_callback.pfnCallback = DummyVkDebugReportCallback;
  (void*&)LDR_PTR(vkCreateDebugReportCallbackEXT) = (void*)LDR_PTR(vkGetInstanceProcAddr)(a_0, "vkCreateDebugReportCallbackEXT");
  return LDR_PTR(vkCreateDebugReportCallbackEXT)(a_0, &overridden_callback, nullptr, a_3);
}

static void FEXFN_IMPL(vkDestroyDebugReportCallbackEXT)(VkInstance a_0, VkDebugReportCallbackEXT a_1, const VkAllocationCallbacks* a_2) {
  (void*&)LDR_PTR(vkDestroyDebugReportCallbackEXT) = (void*)LDR_PTR(vkGetInstanceProcAddr)(a_0, "vkDestroyDebugReportCallbackEXT");
  LDR_PTR(vkDestroyDebugReportCallbackEXT)(a_0, a_1, nullptr);
}

static PFN_vkVoidFunction FEXFN_IMPL(vkGetDeviceProcAddr)(VkDevice a_0, const char* a_1) {
  // Just return the host facing function pointer
  // The guest will handle mapping if this exists

  // Check for functions with stubbed callbacks first
  if (std::strcmp(a_1, "vkCreateShaderModule") == 0) {
      return (PFN_vkVoidFunction)fexfn_impl_libvulkan_vkCreateShaderModule;
  } else if (std::strcmp(a_1, "vkCreateInstance") == 0) {
      return (PFN_vkVoidFunction)fexfn_impl_libvulkan_vkCreateInstance;
  } else if (std::strcmp(a_1, "vkCreateDevice") == 0) {
      return (PFN_vkVoidFunction)fexfn_impl_libvulkan_vkCreateDevice;
  } else if (std::strcmp(a_1, "vkAllocateMemory") == 0) {
      return (PFN_vkVoidFunction)fexfn_impl_libvulkan_vkAllocateMemory;
  } else if (std::strcmp(a_1, "vkFreeMemory") == 0) {
      return (PFN_vkVoidFunction)fexfn_impl_libvulkan_vkFreeMemory;
  }

  auto ret = LDR_PTR(vkGetDeviceProcAddr)(a_0, a_1);
  return ret;
}

static PFN_vkVoidFunction FEXFN_IMPL(vkGetInstanceProcAddr)(VkInstance a_0, const char* a_1) {
  if (!SetupInstance && a_0) {
    DoSetupWithInstance(a_0);
  }

  // Just return the host facing function pointer
  // The guest will handle mapping if it exists
  auto ret = LDR_PTR(vkGetInstanceProcAddr)(a_0, a_1);
  return ret;
}
#endif
EXPORTS(libvulkan)
