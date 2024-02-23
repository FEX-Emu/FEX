/*
$info$
tags: thunklibs|Vulkan
$end_info$
*/

#define VK_USE_64_BIT_PTR_DEFINES 0

#define VK_USE_PLATFORM_XLIB_XRANDR_EXT
#define VK_USE_PLATFORM_XLIB_KHR
#define VK_USE_PLATFORM_XCB_KHR
#define VK_USE_PLATFORM_WAYLAND_KHR
#include <vulkan/vulkan.h>

#include "common/Host.h"

#include <cstring>
#include <mutex>
#include <unordered_map>
#include <string_view>

#include <dlfcn.h>

#include "thunkgen_host_libvulkan.inl"

static bool SetupInstance {};
static std::mutex SetupMutex {};

#define LDR_PTR(fn) fexldr_ptr_libvulkan_##fn

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

static VkResult
FEXFN_IMPL(vkCreateShaderModule)(VkDevice a_0, const VkShaderModuleCreateInfo* a_1, const VkAllocationCallbacks* a_2, VkShaderModule* a_3) {
  (void*&)LDR_PTR(vkCreateShaderModule) = (void*)LDR_PTR(vkGetDeviceProcAddr)(a_0, "vkCreateShaderModule");
  return LDR_PTR(vkCreateShaderModule)(a_0, a_1, nullptr, a_3);
}

static VkBool32
DummyVkDebugReportCallback(VkDebugReportFlagsEXT, VkDebugReportObjectTypeEXT, uint64_t, size_t, int32_t, const char*, const char*, void*) {
  return VK_FALSE;
}

static VkResult FEXFN_IMPL(vkCreateInstance)(const VkInstanceCreateInfo* a_0, const VkAllocationCallbacks* a_1, guest_layout<VkInstance*> a_2) {
  const VkInstanceCreateInfo* vk_struct_base = a_0;
  for (const VkBaseInStructure* vk_struct = reinterpret_cast<const VkBaseInStructure*>(vk_struct_base); vk_struct->pNext;
       vk_struct = vk_struct->pNext) {
    // Override guest callbacks used for VK_EXT_debug_report
    if (reinterpret_cast<const VkBaseInStructure*>(vk_struct->pNext)->sType == VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT) {
      // Overwrite the pNext pointer, ignoring its const-qualifier
      const_cast<VkBaseInStructure*>(vk_struct)->pNext = vk_struct->pNext->pNext;

      // If we copied over a nullptr for pNext then early exit
      if (!vk_struct->pNext) {
        break;
      }
    }
  }

  VkInstance out;
  auto ret = LDR_PTR(vkCreateInstance)(vk_struct_base, nullptr, &out);
  if (ret == VK_SUCCESS) {
    *a_2.get_pointer() = to_guest(to_host_layout(out));
  }
  return ret;
}

static VkResult FEXFN_IMPL(vkCreateDevice)(VkPhysicalDevice a_0, const VkDeviceCreateInfo* a_1, const VkAllocationCallbacks* a_2,
                                           guest_layout<VkDevice*> a_3) {
  VkDevice out;
  auto ret = LDR_PTR(vkCreateDevice)(a_0, a_1, nullptr, &out);
  if (ret == VK_SUCCESS) {
    *a_3.get_pointer() = to_guest(to_host_layout(out));
  }
  return ret;
}

static VkResult FEXFN_IMPL(vkAllocateMemory)(VkDevice a_0, const VkMemoryAllocateInfo* a_1, const VkAllocationCallbacks* a_2, VkDeviceMemory* a_3) {
  (void*&)LDR_PTR(vkAllocateMemory) = (void*)LDR_PTR(vkGetDeviceProcAddr)(a_0, "vkAllocateMemory");
  return LDR_PTR(vkAllocateMemory)(a_0, a_1, nullptr, a_3);
}

static void FEXFN_IMPL(vkFreeMemory)(VkDevice a_0, VkDeviceMemory a_1, const VkAllocationCallbacks* a_2) {
  (void*&)LDR_PTR(vkFreeMemory) = (void*)LDR_PTR(vkGetDeviceProcAddr)(a_0, "vkFreeMemory");
  LDR_PTR(vkFreeMemory)(a_0, a_1, nullptr);
}

static VkResult FEXFN_IMPL(vkCreateDebugReportCallbackEXT)(VkInstance a_0, guest_layout<const VkDebugReportCallbackCreateInfoEXT*> a_1,
                                                           const VkAllocationCallbacks* a_2, VkDebugReportCallbackEXT* a_3) {
  auto overridden_callback = host_layout<VkDebugReportCallbackCreateInfoEXT> {*a_1.get_pointer()}.data;
  overridden_callback.pfnCallback = DummyVkDebugReportCallback;
  (void*&)LDR_PTR(vkCreateDebugReportCallbackEXT) = (void*)LDR_PTR(vkGetInstanceProcAddr)(a_0, "vkCreateDebugReportCallbackEXT");
  return LDR_PTR(vkCreateDebugReportCallbackEXT)(a_0, &overridden_callback, nullptr, a_3);
}

static void FEXFN_IMPL(vkDestroyDebugReportCallbackEXT)(VkInstance a_0, VkDebugReportCallbackEXT a_1, const VkAllocationCallbacks* a_2) {
  (void*&)LDR_PTR(vkDestroyDebugReportCallbackEXT) = (void*)LDR_PTR(vkGetInstanceProcAddr)(a_0, "vkDestroyDebugReportCallbackEXT");
  LDR_PTR(vkDestroyDebugReportCallbackEXT)(a_0, a_1, nullptr);
}

extern "C" VkBool32 DummyVkDebugUtilsMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT,
                                                       const VkDebugUtilsMessengerCallbackDataEXT*, void*) {
  return VK_FALSE;
}

static VkResult FEXFN_IMPL(vkCreateDebugUtilsMessengerEXT)(VkInstance_T* a_0, guest_layout<const VkDebugUtilsMessengerCreateInfoEXT*> a_1,
                                                           const VkAllocationCallbacks* a_2, VkDebugUtilsMessengerEXT* a_3) {
  auto overridden_callback = host_layout<VkDebugUtilsMessengerCreateInfoEXT> {*a_1.get_pointer()}.data;
  overridden_callback.pfnUserCallback = DummyVkDebugUtilsMessengerCallback;
  (void*&)LDR_PTR(vkCreateDebugUtilsMessengerEXT) = (void*)LDR_PTR(vkGetInstanceProcAddr)(a_0, "vkCreateDebugUtilsMessengerEXT");
  return LDR_PTR(vkCreateDebugUtilsMessengerEXT)(a_0, &overridden_callback, nullptr, a_3);
}

static PFN_vkVoidFunction LookupCustomVulkanFunction(const char* a_1) {
  using namespace std::string_view_literals;

  if (a_1 == "vkCreateShaderModule"sv) {
    return (PFN_vkVoidFunction)fexfn_impl_libvulkan_vkCreateShaderModule;
  } else if (a_1 == "vkCreateInstance"sv) {
    return (PFN_vkVoidFunction)fexfn_impl_libvulkan_vkCreateInstance;
  } else if (a_1 == "vkCreateDevice"sv) {
    return (PFN_vkVoidFunction)fexfn_impl_libvulkan_vkCreateDevice;
  } else if (a_1 == "vkAllocateMemory"sv) {
    return (PFN_vkVoidFunction)fexfn_impl_libvulkan_vkAllocateMemory;
  } else if (a_1 == "vkFreeMemory"sv) {
    return (PFN_vkVoidFunction)fexfn_impl_libvulkan_vkFreeMemory;
  }
  return nullptr;
}

static PFN_vkVoidFunction FEXFN_IMPL(vkGetDeviceProcAddr)(VkDevice a_0, const char* a_1) {
  // Just return the host facing function pointer
  // The guest will handle mapping if this exists

  // Check for functions with custom implementations first
  if (auto ptr = LookupCustomVulkanFunction(a_1)) {
    return ptr;
  }

  return LDR_PTR(vkGetDeviceProcAddr)(a_0, a_1);
}

static PFN_vkVoidFunction FEXFN_IMPL(vkGetInstanceProcAddr)(VkInstance a_0, const char* a_1) {
  // Just return the host facing function pointer
  // The guest will handle mapping if it exists

  if (!SetupInstance && a_0) {
    DoSetupWithInstance(a_0);
  }

  // Check for functions with custom implementations first
  if (auto ptr = LookupCustomVulkanFunction(a_1)) {
    return ptr;
  }

  return LDR_PTR(vkGetInstanceProcAddr)(a_0, a_1);
}

EXPORTS(libvulkan)
