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
#include <string_view>

#include "thunkgen_host_libvulkan.inl"

#include <common/X11Manager.h>

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

  // Query pointers for non-EXT functions customized below
  (void*&)LDR_PTR(vkCreateDevice) = (void*)LDR_PTR(vkGetInstanceProcAddr)(instance, "vkCreateDevice");

  // Only do this lookup once.
  // NOTE: If vkGetInstanceProcAddr was called with a null instance, only a few function pointers will be filled with non-null values, so we do repeat the lookup in that case
  if (instance) {
    SetupInstance = true;
  }
}

#define FEXFN_IMPL(fn) fexfn_impl_libvulkan_##fn

static X11Manager x11_manager;

static void fexfn_impl_libvulkan_SetGuestXGetVisualInfo(uintptr_t GuestTarget, uintptr_t GuestUnpacker) {
  MakeHostTrampolineForGuestFunctionAt(GuestTarget, GuestUnpacker, &x11_manager.GuestXGetVisualInfo);
}

static void fexfn_impl_libvulkan_SetGuestXSync(uintptr_t GuestTarget, uintptr_t GuestUnpacker) {
  MakeHostTrampolineForGuestFunctionAt(GuestTarget, GuestUnpacker, &x11_manager.GuestXSync);
}

static void fexfn_impl_libvulkan_SetGuestXDisplayString(uintptr_t GuestTarget, uintptr_t GuestUnpacker) {
  MakeHostTrampolineForGuestFunctionAt(GuestTarget, GuestUnpacker, &x11_manager.GuestXDisplayString);
}

void fex_custom_repack_entry(host_layout<VkXcbSurfaceCreateInfoKHR>& to, const guest_layout<VkXcbSurfaceCreateInfoKHR>& from) {
  // TODO: xcb_aux_sync?
  to.data.connection = x11_manager.GuestToHostConnection(const_cast<xcb_connection_t*>(from.data.connection.force_get_host_pointer()));
}

bool fex_custom_repack_exit(guest_layout<VkXcbSurfaceCreateInfoKHR>&, const host_layout<VkXcbSurfaceCreateInfoKHR>&) {
  // TODO: xcb_sync?
  return false;
}

void fex_custom_repack_entry(host_layout<VkXlibSurfaceCreateInfoKHR>& to, const guest_layout<VkXlibSurfaceCreateInfoKHR>& from) {
  to.data.dpy = x11_manager.GuestToHostDisplay(const_cast<Display*>(from.data.dpy.force_get_host_pointer()));
}

bool fex_custom_repack_exit(guest_layout<VkXlibSurfaceCreateInfoKHR>&, const host_layout<VkXlibSurfaceCreateInfoKHR>& from) {
  x11_manager.HostXFlush(from.data.dpy);
  return false;
}

static VkResult fexfn_impl_libvulkan_vkAcquireXlibDisplayEXT(VkPhysicalDevice a_0, guest_layout<Display*> a_1, VkDisplayKHR a_2) {
  auto host_display = x11_manager.GuestToHostDisplay(a_1.force_get_host_pointer());
  auto ret = fexldr_ptr_libvulkan_vkAcquireXlibDisplayEXT(a_0, host_display, a_2);
  x11_manager.HostXFlush(host_display);
  return ret;
}

static VkResult fexfn_impl_libvulkan_vkGetRandROutputDisplayEXT(VkPhysicalDevice a_0, guest_layout<Display*> a_1, RROutput a_2, VkDisplayKHR* a_3) {
  auto host_display = x11_manager.GuestToHostDisplay(a_1.force_get_host_pointer());
  auto ret = fexldr_ptr_libvulkan_vkGetRandROutputDisplayEXT(a_0, host_display, a_2, a_3);
  x11_manager.HostXFlush(host_display);
  return ret;
}

static VkBool32 fexfn_impl_libvulkan_vkGetPhysicalDeviceXcbPresentationSupportKHR(VkPhysicalDevice a_0, uint32_t a_1,
                                                                                  guest_layout<xcb_connection_t*> a_2, xcb_visualid_t a_3) {
  auto host_connection = x11_manager.GuestToHostConnection(a_2.force_get_host_pointer());
  return fexldr_ptr_libvulkan_vkGetPhysicalDeviceXcbPresentationSupportKHR(a_0, a_1, host_connection, a_3);
}

static VkBool32 fexfn_impl_libvulkan_vkGetPhysicalDeviceXlibPresentationSupportKHR(VkPhysicalDevice a_0, uint32_t a_1,
                                                                                   guest_layout<Display*> a_2, VisualID a_3) {
  auto host_display = x11_manager.GuestToHostDisplay(a_2.force_get_host_pointer());
  auto ret = fexldr_ptr_libvulkan_vkGetPhysicalDeviceXlibPresentationSupportKHR(a_0, a_1, host_display, a_3);
  x11_manager.HostXFlush(host_display);
  return ret;
}

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
  } else if (a_1 == "vkAcquireXlibDisplayEXT"sv) {
    return (PFN_vkVoidFunction)fexfn_impl_libvulkan_vkAcquireXlibDisplayEXT;
  } else if (a_1 == "vkGetRandROutputDisplayEXT"sv) {
    return (PFN_vkVoidFunction)fexfn_impl_libvulkan_vkGetRandROutputDisplayEXT;
  } else if (a_1 == "vkGetPhysicalDeviceXcbPresentationSupportKHR"sv) {
    return (PFN_vkVoidFunction)fexfn_impl_libvulkan_vkGetPhysicalDeviceXcbPresentationSupportKHR;
  } else if (a_1 == "vkGetPhysicalDeviceXlibPresentationSupportKHR"sv) {
    return (PFN_vkVoidFunction)fexfn_impl_libvulkan_vkGetPhysicalDeviceXlibPresentationSupportKHR;
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
    // If this function belongs to an instance extension, requery its address.
    // This ensures fexldr_ptr_* is valid if the application creates a minimal
    // VkInstance with no extensions before creating its actual instance.
    using namespace std::string_view_literals;
    if (a_1 == "vkGetRandROutputDisplayEXT"sv && !LDR_PTR(vkGetRandROutputDisplayEXT)) {
      (void*&)LDR_PTR(vkGetRandROutputDisplayEXT) = (void*)LDR_PTR(vkGetInstanceProcAddr)(a_0, "vkGetRandROutputDisplayEXT");
    }
    if (a_1 == "vkAcquireXlibDisplayEXT"sv && !LDR_PTR(vkAcquireXlibDisplayEXT)) {
      (void*&)LDR_PTR(vkAcquireXlibDisplayEXT) = (void*)LDR_PTR(vkGetInstanceProcAddr)(a_0, "vkAcquireXlibDisplayEXT");
    }
    const char* XcbPresent = "vkGetPhysicalDeviceXcbPresentationSupportKHR";
    if (a_1 == std::string_view {XcbPresent} && !LDR_PTR(vkGetPhysicalDeviceXcbPresentationSupportKHR)) {
      (void*&)LDR_PTR(vkGetPhysicalDeviceXcbPresentationSupportKHR) = (void*)LDR_PTR(vkGetInstanceProcAddr)(a_0, XcbPresent);
    }
    const char* XlibPresent = "vkGetPhysicalDeviceXlibPresentationSupportKHR";
    if (a_1 == std::string_view {XlibPresent} && !LDR_PTR(vkGetPhysicalDeviceXlibPresentationSupportKHR)) {
      (void*&)LDR_PTR(vkGetPhysicalDeviceXlibPresentationSupportKHR) = (void*)LDR_PTR(vkGetInstanceProcAddr)(a_0, XlibPresent);
    }

    return ptr;
  }

  return LDR_PTR(vkGetInstanceProcAddr)(a_0, a_1);
}

EXPORTS(libvulkan)
