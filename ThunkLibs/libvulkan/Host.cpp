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

#include <cassert>
#include <cstring>
#include <mutex>
#include <span>
#include <string_view>
#include <unordered_map>
#include <vector>

#ifdef IS_32BIT_THUNK
// Union type embedded in VkDescriptorGetInfoEXT
template<>
struct guest_layout<VkDescriptorDataEXT> {
  char union_storage[8];
};
#endif

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
  *a_2.get_pointer() = to_guest(to_host_layout(out));
  return ret;
}

static VkResult FEXFN_IMPL(vkCreateDevice)(VkPhysicalDevice a_0, const VkDeviceCreateInfo* a_1, const VkAllocationCallbacks* a_2,
                                           guest_layout<VkDevice*> a_3) {
  VkDevice out;
  auto ret = LDR_PTR(vkCreateDevice)(a_0, a_1, nullptr, &out);
  *a_3.get_pointer() = to_guest(to_host_layout(out));

  // Reload device-specific function pointers used in custom implementations.
  // This is only done in advance for functions that don't take a VkDevice
  // argument. Since this breaks multi-device scenarios, other functions reload
  // the function pointer on-demand.
  // NOTE: Running KHR-GLES31.core.compute_shader.simple-compute-shared_context with zink may trigger related issues
  // TODO: Support multi-device scenarios everywhere
#ifdef IS_32BIT_THUNK
  fexldr_ptr_libvulkan_vkCmdSetVertexInputEXT = (PFN_vkCmdSetVertexInputEXT)fexldr_ptr_libvulkan_vkGetDeviceProcAddr(out, "vkCmdSetVertexIn"
                                                                                                                          "putEXT");
  fexldr_ptr_libvulkan_vkQueueSubmit = (PFN_vkQueueSubmit)fexldr_ptr_libvulkan_vkGetDeviceProcAddr(out, "vkQueueSubmit");
#else
  // No functions affected on 64-bit
#endif

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

#ifdef IS_32BIT_THUNK
VkResult fexfn_impl_libvulkan_vkEnumeratePhysicalDevices(VkInstance instance, uint32_t* count, guest_layout<VkPhysicalDevice*> devices) {
  if (!devices.get_pointer()) {
    return fexldr_ptr_libvulkan_vkEnumeratePhysicalDevices(instance, count, nullptr);
  }

  auto input_count = *count;
  std::vector<VkPhysicalDevice> out(input_count);
  auto ret = fexldr_ptr_libvulkan_vkEnumeratePhysicalDevices(instance, count, out.data());
  for (size_t i = 0; i < std::min(input_count, *count); ++i) {
    devices.get_pointer()[i] = to_guest(to_host_layout(out[i]));
  }
  return ret;
}

void fexfn_impl_libvulkan_vkGetDeviceQueue(VkDevice device, uint32_t family_index, uint32_t queue_index, guest_layout<VkQueue*> queue) {
  VkQueue out;
  (void*&)fexldr_ptr_libvulkan_vkGetDeviceQueue = (void*)LDR_PTR(vkGetDeviceProcAddr)(device, "vkGetDeviceQueue");
  fexldr_ptr_libvulkan_vkGetDeviceQueue(device, family_index, queue_index, &out);
  *queue.get_pointer() = to_guest(to_host_layout(out));
}

VkResult fexfn_impl_libvulkan_vkAllocateCommandBuffers(VkDevice device, const VkCommandBufferAllocateInfo* info,
                                                       guest_layout<VkCommandBuffer*> buffers) {
  std::vector<VkCommandBuffer> out(info->commandBufferCount);
  (void*&)fexldr_ptr_libvulkan_vkAllocateCommandBuffers = (void*)LDR_PTR(vkGetDeviceProcAddr)(device, "vkAllocateCommandBuffers");
  auto ret = fexldr_ptr_libvulkan_vkAllocateCommandBuffers(device, info, out.data());
  if (ret == VK_SUCCESS) {
    for (size_t i = 0; i < info->commandBufferCount; ++i) {
      buffers.get_pointer()[i] = to_guest(to_host_layout(out[i]));
    }
  }
  return ret;
}

VkResult fexfn_impl_libvulkan_vkMapMemory(VkDevice device, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size,
                                          VkMemoryMapFlags flags, guest_layout<void**> data) {
  host_layout<void*> host_data {};
  void* mapped;
  (void*&)fexldr_ptr_libvulkan_vkMapMemory = (void*)LDR_PTR(vkGetDeviceProcAddr)(device, "vkMapMemory");
  auto ret = fexldr_ptr_libvulkan_vkMapMemory(device, memory, offset, size, flags, &mapped);
  if (ret == VK_SUCCESS) {
    host_data.data = mapped;
    *data.get_pointer() = to_guest(host_data);
  }
  return ret;
}

// Allocates storage on the heap that must be de-allocated using delete[] or DeleteRepackedStructArray
template<bool NeedsRepack = true, typename T>
std::span<std::remove_cv_t<T>> RepackStructArray(uint32_t Count, const guest_layout<T*> GuestData) {
  if (!GuestData.get_pointer() || Count == 0) {
    return {};
  }

  auto HostData = new std::remove_cv_t<T>[Count];
  for (size_t i = 0; i < Count; ++i) {
    auto& GuestElement = (const guest_layout<std::remove_cv_t<T>>&)GuestData.get_pointer()[i];
    auto Element = host_layout<std::remove_cv_t<T>> {GuestElement};
    if constexpr (NeedsRepack) {
      fex_apply_custom_repacking_entry(Element, GuestElement);
    }
    HostData[i] = Element.data;
  }
  return {HostData, Count};
}

template<typename T>
void DeleteRepackedStructArray(uint32_t Count, T* HostData, guest_layout<T*>& GuestData) {
  for (uint32_t i = 0; i < Count; ++i) {
    fex_apply_custom_repacking_exit(GuestData.get_pointer()[i], to_host_layout(HostData[i]));
  }
  delete[] HostData;
}

void fexfn_impl_libvulkan_vkCmdSetVertexInputEXT(
  VkCommandBuffer Buffer, uint32_t BindingDescCount, guest_layout<const VkVertexInputBindingDescription2EXT*> GuestBindingDescs,
  uint32_t AttributeDescCount, guest_layout<const VkVertexInputAttributeDescription2EXT*> GuestAttributeDescs) {

  assert(GuestBindingDescs.get_pointer() && BindingDescCount > 0);
  assert(GuestAttributeDescs.get_pointer() && AttributeDescCount > 0);

  auto BindingDescs = RepackStructArray(BindingDescCount, GuestBindingDescs);
  auto AttributeDescs = RepackStructArray(AttributeDescCount, GuestAttributeDescs);

  fexldr_ptr_libvulkan_vkCmdSetVertexInputEXT(Buffer, BindingDescCount, BindingDescs.data(), AttributeDescCount, AttributeDescs.data());

  delete[] AttributeDescs.data();
  delete[] BindingDescs.data();
}

void fexfn_impl_libvulkan_vkUpdateDescriptorSets(VkDevice device, unsigned int descriptorWriteCount,
                                                 guest_layout<const VkWriteDescriptorSet*> pDescriptorWrites, unsigned int descriptorCopyCount,
                                                 guest_layout<const VkCopyDescriptorSet*> pDescriptorCopies) {

  auto HostDescriptorWrites = RepackStructArray(descriptorWriteCount, pDescriptorWrites);
  auto HostDescriptorCopies = RepackStructArray(descriptorCopyCount, pDescriptorCopies);

  (void*&)fexldr_ptr_libvulkan_vkUpdateDescriptorSets = (void*)LDR_PTR(vkGetDeviceProcAddr)(device, "vkUpdateDescriptorSets");
  fexldr_ptr_libvulkan_vkUpdateDescriptorSets(device, descriptorWriteCount, HostDescriptorWrites.data(), descriptorCopyCount,
                                              HostDescriptorCopies.data());

  delete[] HostDescriptorCopies.data();
  delete[] HostDescriptorWrites.data();
}

VkResult fexfn_impl_libvulkan_vkQueueSubmit(VkQueue queue, uint32_t submit_count, guest_layout<const VkSubmitInfo*> submit_infos, VkFence fence) {

  auto HostSubmitInfos = RepackStructArray(submit_count, submit_infos);
  auto ret = fexldr_ptr_libvulkan_vkQueueSubmit(queue, submit_count, HostSubmitInfos.data(), fence);
  delete[] HostSubmitInfos.data();
  return ret;
}

void fexfn_impl_libvulkan_vkFreeCommandBuffers(VkDevice device, VkCommandPool pool, uint32_t num_buffers,
                                               guest_layout<const VkCommandBuffer*> buffers) {

  auto HostBuffers = RepackStructArray<false>(num_buffers, buffers);
  (void*&)fexldr_ptr_libvulkan_vkFreeCommandBuffers = (void*)LDR_PTR(vkGetDeviceProcAddr)(device, "vkFreeCommandBuffers");
  fexldr_ptr_libvulkan_vkFreeCommandBuffers(device, pool, num_buffers, HostBuffers.data());
  delete[] HostBuffers.data();
}

VkResult fexfn_impl_libvulkan_vkGetPipelineCacheData(VkDevice device, VkPipelineCache cache, guest_layout<uint32_t*> guest_data_size, void* data) {
  size_t data_size = guest_data_size.get_pointer()->data;
  (void*&)fexldr_ptr_libvulkan_vkGetPipelineCacheData = (void*)LDR_PTR(vkGetDeviceProcAddr)(device, "vkGetPipelineCacheData");
  auto ret = fexldr_ptr_libvulkan_vkGetPipelineCacheData(device, cache, &data_size, data);
  *guest_data_size.get_pointer() = data_size;
  return ret;
}

#endif

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
#ifdef IS_32BIT_THUNK
  } else if (a_1 == "vkAllocateCommandBuffers"sv) {
    return (PFN_vkVoidFunction)fexfn_impl_libvulkan_vkAllocateCommandBuffers;
  } else if (a_1 == "vkEnumeratePhysicalDevices"sv) {
    return (PFN_vkVoidFunction)fexfn_impl_libvulkan_vkEnumeratePhysicalDevices;
  } else if (a_1 == "vkFreeCommandBuffers"sv) {
    return (PFN_vkVoidFunction)fexfn_impl_libvulkan_vkFreeCommandBuffers;
  } else if (a_1 == "vkGetDeviceQueue"sv) {
    return (PFN_vkVoidFunction)fexfn_impl_libvulkan_vkGetDeviceQueue;
  } else if (a_1 == "vkGetPipelineCacheData"sv) {
    return (PFN_vkVoidFunction)fexfn_impl_libvulkan_vkGetPipelineCacheData;
  } else if (a_1 == "vkMapMemory"sv) {
    return (PFN_vkVoidFunction)fexfn_impl_libvulkan_vkMapMemory;
  } else if (a_1 == "vkQueueSubmit"sv) {
    return (PFN_vkVoidFunction)fexfn_impl_libvulkan_vkQueueSubmit;
  } else if (a_1 == "vkCmdSetVertexInputEXT"sv) {
    return (PFN_vkVoidFunction)fexfn_impl_libvulkan_vkCmdSetVertexInputEXT;
  } else if (a_1 == "vkUpdateDescriptorSets"sv) {
    return (PFN_vkVoidFunction)fexfn_impl_libvulkan_vkUpdateDescriptorSets;
#endif
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

#ifdef IS_32BIT_THUNK
template<VkStructureType TypeIndex, typename Type>
static VkBaseOutStructure* convert(const guest_layout<VkBaseOutStructure>* source) {
  // Using malloc here since no easily available type information is available at the time of destruction.
  auto typed_source = reinterpret_cast<const guest_layout<Type>*>(source);
  auto child_mem = (char*)aligned_alloc(alignof(host_layout<Type>), sizeof(host_layout<Type>));
  auto child = new (child_mem) host_layout<Type> {*typed_source};

  fex_custom_repack_entry(*child, *typed_source);

  return reinterpret_cast<VkBaseOutStructure*>(&child->data);
}

template<VkStructureType TypeIndex, typename Type>
static void convert_to_guest(void* into, const VkBaseOutStructure* from) {
  auto typed_into = reinterpret_cast<guest_layout<Type>*>(into);
  auto oldNext = typed_into->data.pNext; // TODO: This assumes Vulkan never modifies pNext internally
  *typed_into = to_guest(to_host_layout(*(Type*)from));
  typed_into->data.pNext = oldNext;

  fex_custom_repack_exit(*typed_into, to_host_layout(*(Type*)from));
}

template<VkStructureType TypeIndex, typename Type>
inline constexpr std::pair<VkStructureType, std::pair<VkBaseOutStructure* (*)(const guest_layout<VkBaseOutStructure>*), void (*)(void*, const VkBaseOutStructure*)>>
  converters = {TypeIndex, {convert<TypeIndex, Type>, convert_to_guest<TypeIndex, Type>}};

// NOTE: Not all Vulkan structures with pNext members are listed here. This is because excluding structs exclusively used as top-level entries is useful to detect repacking bugs.
static std::unordered_map<VkStructureType, std::pair<VkBaseOutStructure* (*)(const guest_layout<VkBaseOutStructure>*), void (*)(void*, const VkBaseOutStructure*)>> next_handlers {
  converters<VkStructureType::VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MOTION_INFO_NV, VkAccelerationStructureMotionInfoNV>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_AMIGO_PROFILING_SUBMIT_INFO_SEC, VkAmigoProfilingSubmitInfoSEC>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_STENCIL_LAYOUT, VkAttachmentDescriptionStencilLayout>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_STENCIL_LAYOUT, VkAttachmentReferenceStencilLayout>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_ATTACHMENT_SAMPLE_COUNT_INFO_AMD, VkAttachmentSampleCountInfoAMD>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_DEVICE_GROUP_INFO, VkBindBufferMemoryDeviceGroupInfo>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_DEVICE_GROUP_INFO, VkBindImageMemoryDeviceGroupInfo>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_SWAPCHAIN_INFO_KHR, VkBindImageMemorySwapchainInfoKHR>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_BIND_IMAGE_PLANE_MEMORY_INFO, VkBindImagePlaneMemoryInfo>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_CREATE_INFO_EXT, VkBufferDeviceAddressCreateInfoEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_BUFFER_OPAQUE_CAPTURE_ADDRESS_CREATE_INFO, VkBufferOpaqueCaptureAddressCreateInfo>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_BUFFER_USAGE_FLAGS_2_CREATE_INFO_KHR, VkBufferUsageFlags2CreateInfoKHR>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_CONDITIONAL_RENDERING_INFO_EXT, VkCommandBufferInheritanceConditionalRenderingInfoEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_RENDERING_INFO, VkCommandBufferInheritanceRenderingInfo>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_RENDER_PASS_TRANSFORM_INFO_QCOM, VkCommandBufferInheritanceRenderPassTransformInfoQCOM>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_VIEWPORT_SCISSOR_INFO_NV, VkCommandBufferInheritanceViewportScissorInfoNV>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_COPY_COMMAND_TRANSFORM_INFO_QCOM, VkCopyCommandTransformInfoQCOM>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT, VkDebugReportCallbackCreateInfoEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT, VkDebugUtilsMessengerCreateInfoEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT, VkDebugUtilsObjectNameInfoEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_BUFFER_CREATE_INFO_NV, VkDedicatedAllocationBufferCreateInfoNV>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_IMAGE_CREATE_INFO_NV, VkDedicatedAllocationImageCreateInfoNV>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_MEMORY_ALLOCATE_INFO_NV, VkDedicatedAllocationMemoryAllocateInfoNV>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_DEPTH_BIAS_REPRESENTATION_INFO_EXT, VkDepthBiasRepresentationInfoEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_PUSH_DESCRIPTOR_BUFFER_HANDLE_EXT, VkDescriptorBufferBindingPushDescriptorBufferHandleEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_INLINE_UNIFORM_BLOCK_CREATE_INFO, VkDescriptorPoolInlineUniformBlockCreateInfo>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO, VkDescriptorSetLayoutBindingFlagsCreateInfo>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO, VkDescriptorSetVariableDescriptorCountAllocateInfo>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_LAYOUT_SUPPORT, VkDescriptorSetVariableDescriptorCountLayoutSupport>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_DEVICE_ADDRESS_BINDING_CALLBACK_DATA_EXT, VkDeviceAddressBindingCallbackDataEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_DEVICE_DIAGNOSTICS_CONFIG_CREATE_INFO_NV, VkDeviceDiagnosticsConfigCreateInfoNV>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_DEVICE_GROUP_BIND_SPARSE_INFO, VkDeviceGroupBindSparseInfo>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_DEVICE_GROUP_COMMAND_BUFFER_BEGIN_INFO, VkDeviceGroupCommandBufferBeginInfo>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_DEVICE_GROUP_PRESENT_INFO_KHR, VkDeviceGroupPresentInfoKHR>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_DEVICE_GROUP_RENDER_PASS_BEGIN_INFO, VkDeviceGroupRenderPassBeginInfo>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_DEVICE_GROUP_SUBMIT_INFO, VkDeviceGroupSubmitInfo>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_DEVICE_GROUP_SWAPCHAIN_CREATE_INFO_KHR, VkDeviceGroupSwapchainCreateInfoKHR>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_DEVICE_MEMORY_OVERALLOCATION_CREATE_INFO_AMD, VkDeviceMemoryOverallocationCreateInfoAMD>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_DEVICE_PRIVATE_DATA_CREATE_INFO, VkDevicePrivateDataCreateInfo>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_DEVICE_QUEUE_GLOBAL_PRIORITY_CREATE_INFO_KHR, VkDeviceQueueGlobalPriorityCreateInfoKHR>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_DISPLAY_NATIVE_HDR_SURFACE_CAPABILITIES_AMD, VkDisplayNativeHdrSurfaceCapabilitiesAMD>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_DISPLAY_PRESENT_INFO_KHR, VkDisplayPresentInfoKHR>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_EXPORT_FENCE_CREATE_INFO, VkExportFenceCreateInfo>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO, VkExportMemoryAllocateInfo>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO_NV, VkExportMemoryAllocateInfoNV>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO, VkExportSemaphoreCreateInfo>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_EXTERNAL_IMAGE_FORMAT_PROPERTIES, VkExternalImageFormatProperties>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_ACQUIRE_UNMODIFIED_EXT, VkExternalMemoryAcquireUnmodifiedEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO, VkExternalMemoryBufferCreateInfo>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO, VkExternalMemoryImageCreateInfo>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO_NV, VkExternalMemoryImageCreateInfoNV>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_FILTER_CUBIC_IMAGE_VIEW_IMAGE_FORMAT_PROPERTIES_EXT, VkFilterCubicImageViewImageFormatPropertiesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_3, VkFormatProperties3>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_LIBRARY_CREATE_INFO_EXT, VkGraphicsPipelineLibraryCreateInfoEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_HOST_IMAGE_COPY_DEVICE_PERFORMANCE_QUERY_EXT, VkHostImageCopyDevicePerformanceQueryEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_IMAGE_COMPRESSION_CONTROL_EXT, VkImageCompressionControlEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_IMAGE_COMPRESSION_PROPERTIES_EXT, VkImageCompressionPropertiesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_LIST_CREATE_INFO_EXT, VkImageDrmFormatModifierListCreateInfoEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO, VkImageFormatListCreateInfo>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_IMAGE_PLANE_MEMORY_REQUIREMENTS_INFO, VkImagePlaneMemoryRequirementsInfo>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_IMAGE_STENCIL_USAGE_CREATE_INFO, VkImageStencilUsageCreateInfo>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_IMAGE_SWAPCHAIN_CREATE_INFO_KHR, VkImageSwapchainCreateInfoKHR>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_IMAGE_VIEW_ASTC_DECODE_MODE_EXT, VkImageViewASTCDecodeModeEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_IMAGE_VIEW_MIN_LOD_CREATE_INFO_EXT, VkImageViewMinLodCreateInfoEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_IMAGE_VIEW_SAMPLE_WEIGHT_CREATE_INFO_QCOM, VkImageViewSampleWeightCreateInfoQCOM>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_IMAGE_VIEW_SLICED_CREATE_INFO_EXT, VkImageViewSlicedCreateInfoEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO, VkImageViewUsageCreateInfo>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR, VkImportMemoryFdInfoKHR>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO, VkMemoryAllocateFlagsInfo>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_MEMORY_BARRIER_2, VkMemoryBarrier2>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO, VkMemoryDedicatedAllocateInfo>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS, VkMemoryDedicatedRequirements>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_MEMORY_OPAQUE_CAPTURE_ADDRESS_ALLOCATE_INFO, VkMemoryOpaqueCaptureAddressAllocateInfo>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_MEMORY_PRIORITY_ALLOCATE_INFO_EXT, VkMemoryPriorityAllocateInfoEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_MULTISAMPLED_RENDER_TO_SINGLE_SAMPLED_INFO_EXT, VkMultisampledRenderToSingleSampledInfoEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_MULTIVIEW_PER_VIEW_ATTRIBUTES_INFO_NVX, VkMultiviewPerViewAttributesInfoNVX>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_MULTIVIEW_PER_VIEW_RENDER_AREAS_RENDER_PASS_BEGIN_INFO_QCOM, VkMultiviewPerViewRenderAreasRenderPassBeginInfoQCOM>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_OPTICAL_FLOW_IMAGE_FORMAT_INFO_NV, VkOpticalFlowImageFormatInfoNV>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PERFORMANCE_QUERY_SUBMIT_INFO_KHR, VkPerformanceQuerySubmitInfoKHR>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES, VkPhysicalDevice16BitStorageFeatures>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_4444_FORMATS_FEATURES_EXT, VkPhysicalDevice4444FormatsFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES, VkPhysicalDevice8BitStorageFeatures>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR, VkPhysicalDeviceAccelerationStructureFeaturesKHR>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR, VkPhysicalDeviceAccelerationStructurePropertiesKHR>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ADDRESS_BINDING_REPORT_FEATURES_EXT, VkPhysicalDeviceAddressBindingReportFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_AMIGO_PROFILING_FEATURES_SEC, VkPhysicalDeviceAmigoProfilingFeaturesSEC>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ASTC_DECODE_FEATURES_EXT, VkPhysicalDeviceASTCDecodeFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ATTACHMENT_FEEDBACK_LOOP_DYNAMIC_STATE_FEATURES_EXT, VkPhysicalDeviceAttachmentFeedbackLoopDynamicStateFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ATTACHMENT_FEEDBACK_LOOP_LAYOUT_FEATURES_EXT, VkPhysicalDeviceAttachmentFeedbackLoopLayoutFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BLEND_OPERATION_ADVANCED_FEATURES_EXT, VkPhysicalDeviceBlendOperationAdvancedFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BLEND_OPERATION_ADVANCED_PROPERTIES_EXT, VkPhysicalDeviceBlendOperationAdvancedPropertiesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BORDER_COLOR_SWIZZLE_FEATURES_EXT, VkPhysicalDeviceBorderColorSwizzleFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES, VkPhysicalDeviceBufferDeviceAddressFeatures>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_EXT, VkPhysicalDeviceBufferDeviceAddressFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CLUSTER_CULLING_SHADER_FEATURES_HUAWEI, VkPhysicalDeviceClusterCullingShaderFeaturesHUAWEI>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CLUSTER_CULLING_SHADER_PROPERTIES_HUAWEI, VkPhysicalDeviceClusterCullingShaderPropertiesHUAWEI>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COHERENT_MEMORY_FEATURES_AMD, VkPhysicalDeviceCoherentMemoryFeaturesAMD>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COLOR_WRITE_ENABLE_FEATURES_EXT, VkPhysicalDeviceColorWriteEnableFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COMPUTE_SHADER_DERIVATIVES_FEATURES_NV, VkPhysicalDeviceComputeShaderDerivativesFeaturesNV>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONDITIONAL_RENDERING_FEATURES_EXT, VkPhysicalDeviceConditionalRenderingFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONSERVATIVE_RASTERIZATION_PROPERTIES_EXT, VkPhysicalDeviceConservativeRasterizationPropertiesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_MATRIX_FEATURES_KHR, VkPhysicalDeviceCooperativeMatrixFeaturesKHR>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_MATRIX_FEATURES_NV, VkPhysicalDeviceCooperativeMatrixFeaturesNV>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_MATRIX_PROPERTIES_KHR, VkPhysicalDeviceCooperativeMatrixPropertiesKHR>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_MATRIX_PROPERTIES_NV, VkPhysicalDeviceCooperativeMatrixPropertiesNV>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COPY_MEMORY_INDIRECT_FEATURES_NV, VkPhysicalDeviceCopyMemoryIndirectFeaturesNV>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COPY_MEMORY_INDIRECT_PROPERTIES_NV, VkPhysicalDeviceCopyMemoryIndirectPropertiesNV>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CORNER_SAMPLED_IMAGE_FEATURES_NV, VkPhysicalDeviceCornerSampledImageFeaturesNV>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COVERAGE_REDUCTION_MODE_FEATURES_NV, VkPhysicalDeviceCoverageReductionModeFeaturesNV>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CUSTOM_BORDER_COLOR_FEATURES_EXT, VkPhysicalDeviceCustomBorderColorFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CUSTOM_BORDER_COLOR_PROPERTIES_EXT, VkPhysicalDeviceCustomBorderColorPropertiesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEDICATED_ALLOCATION_IMAGE_ALIASING_FEATURES_NV, VkPhysicalDeviceDedicatedAllocationImageAliasingFeaturesNV>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_BIAS_CONTROL_FEATURES_EXT, VkPhysicalDeviceDepthBiasControlFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLAMP_ZERO_ONE_FEATURES_EXT, VkPhysicalDeviceDepthClampZeroOneFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_CONTROL_FEATURES_EXT, VkPhysicalDeviceDepthClipControlFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_ENABLE_FEATURES_EXT, VkPhysicalDeviceDepthClipEnableFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_STENCIL_RESOLVE_PROPERTIES, VkPhysicalDeviceDepthStencilResolveProperties>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_DENSITY_MAP_PROPERTIES_EXT, VkPhysicalDeviceDescriptorBufferDensityMapPropertiesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_FEATURES_EXT, VkPhysicalDeviceDescriptorBufferFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT, VkPhysicalDeviceDescriptorBufferPropertiesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES, VkPhysicalDeviceDescriptorIndexingFeatures>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_PROPERTIES, VkPhysicalDeviceDescriptorIndexingProperties>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_SET_HOST_MAPPING_FEATURES_VALVE, VkPhysicalDeviceDescriptorSetHostMappingFeaturesVALVE>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEVICE_GENERATED_COMMANDS_COMPUTE_FEATURES_NV, VkPhysicalDeviceDeviceGeneratedCommandsComputeFeaturesNV>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEVICE_GENERATED_COMMANDS_FEATURES_NV, VkPhysicalDeviceDeviceGeneratedCommandsFeaturesNV>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEVICE_GENERATED_COMMANDS_PROPERTIES_NV, VkPhysicalDeviceDeviceGeneratedCommandsPropertiesNV>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEVICE_MEMORY_REPORT_FEATURES_EXT, VkPhysicalDeviceDeviceMemoryReportFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DIAGNOSTICS_CONFIG_FEATURES_NV, VkPhysicalDeviceDiagnosticsConfigFeaturesNV>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DISCARD_RECTANGLE_PROPERTIES_EXT, VkPhysicalDeviceDiscardRectanglePropertiesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES, VkPhysicalDeviceDriverProperties>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRM_PROPERTIES_EXT, VkPhysicalDeviceDrmPropertiesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES, VkPhysicalDeviceDynamicRenderingFeatures>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_UNUSED_ATTACHMENTS_FEATURES_EXT, VkPhysicalDeviceDynamicRenderingUnusedAttachmentsFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXCLUSIVE_SCISSOR_FEATURES_NV, VkPhysicalDeviceExclusiveScissorFeaturesNV>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_2_FEATURES_EXT, VkPhysicalDeviceExtendedDynamicState2FeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT, VkPhysicalDeviceExtendedDynamicState3FeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_PROPERTIES_EXT, VkPhysicalDeviceExtendedDynamicState3PropertiesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT, VkPhysicalDeviceExtendedDynamicStateFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_IMAGE_FORMAT_INFO, VkPhysicalDeviceExternalImageFormatInfo>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_MEMORY_HOST_PROPERTIES_EXT, VkPhysicalDeviceExternalMemoryHostPropertiesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_MEMORY_RDMA_FEATURES_NV, VkPhysicalDeviceExternalMemoryRDMAFeaturesNV>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FAULT_FEATURES_EXT, VkPhysicalDeviceFaultFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, VkPhysicalDeviceFeatures2>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FLOAT_CONTROLS_PROPERTIES, VkPhysicalDeviceFloatControlsProperties>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_2_FEATURES_EXT, VkPhysicalDeviceFragmentDensityMap2FeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_2_PROPERTIES_EXT, VkPhysicalDeviceFragmentDensityMap2PropertiesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_FEATURES_EXT, VkPhysicalDeviceFragmentDensityMapFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_OFFSET_FEATURES_QCOM, VkPhysicalDeviceFragmentDensityMapOffsetFeaturesQCOM>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_OFFSET_PROPERTIES_QCOM, VkPhysicalDeviceFragmentDensityMapOffsetPropertiesQCOM>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_PROPERTIES_EXT, VkPhysicalDeviceFragmentDensityMapPropertiesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_FEATURES_KHR, VkPhysicalDeviceFragmentShaderBarycentricFeaturesKHR>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_PROPERTIES_KHR, VkPhysicalDeviceFragmentShaderBarycentricPropertiesKHR>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_INTERLOCK_FEATURES_EXT, VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_ENUMS_FEATURES_NV, VkPhysicalDeviceFragmentShadingRateEnumsFeaturesNV>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_ENUMS_PROPERTIES_NV, VkPhysicalDeviceFragmentShadingRateEnumsPropertiesNV>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR, VkPhysicalDeviceFragmentShadingRateFeaturesKHR>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_PROPERTIES_KHR, VkPhysicalDeviceFragmentShadingRatePropertiesKHR>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GLOBAL_PRIORITY_QUERY_FEATURES_KHR, VkPhysicalDeviceGlobalPriorityQueryFeaturesKHR>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GRAPHICS_PIPELINE_LIBRARY_FEATURES_EXT, VkPhysicalDeviceGraphicsPipelineLibraryFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GRAPHICS_PIPELINE_LIBRARY_PROPERTIES_EXT, VkPhysicalDeviceGraphicsPipelineLibraryPropertiesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_IMAGE_COPY_FEATURES_EXT, VkPhysicalDeviceHostImageCopyFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_IMAGE_COPY_PROPERTIES_EXT, VkPhysicalDeviceHostImageCopyPropertiesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES, VkPhysicalDeviceHostQueryResetFeatures>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES, VkPhysicalDeviceIDProperties>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_2D_VIEW_OF_3D_FEATURES_EXT, VkPhysicalDeviceImage2DViewOf3DFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_COMPRESSION_CONTROL_FEATURES_EXT, VkPhysicalDeviceImageCompressionControlFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_COMPRESSION_CONTROL_SWAPCHAIN_FEATURES_EXT, VkPhysicalDeviceImageCompressionControlSwapchainFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_DRM_FORMAT_MODIFIER_INFO_EXT, VkPhysicalDeviceImageDrmFormatModifierInfoEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGELESS_FRAMEBUFFER_FEATURES, VkPhysicalDeviceImagelessFramebufferFeatures>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_PROCESSING_FEATURES_QCOM, VkPhysicalDeviceImageProcessingFeaturesQCOM>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_PROCESSING_PROPERTIES_QCOM, VkPhysicalDeviceImageProcessingPropertiesQCOM>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_ROBUSTNESS_FEATURES, VkPhysicalDeviceImageRobustnessFeatures>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_SLICED_VIEW_OF_3D_FEATURES_EXT, VkPhysicalDeviceImageSlicedViewOf3DFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_VIEW_IMAGE_FORMAT_INFO_EXT, VkPhysicalDeviceImageViewImageFormatInfoEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_VIEW_MIN_LOD_FEATURES_EXT, VkPhysicalDeviceImageViewMinLodFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INDEX_TYPE_UINT8_FEATURES_EXT, VkPhysicalDeviceIndexTypeUint8FeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INHERITED_VIEWPORT_SCISSOR_FEATURES_NV, VkPhysicalDeviceInheritedViewportScissorFeaturesNV>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INLINE_UNIFORM_BLOCK_FEATURES, VkPhysicalDeviceInlineUniformBlockFeatures>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INLINE_UNIFORM_BLOCK_PROPERTIES, VkPhysicalDeviceInlineUniformBlockProperties>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INVOCATION_MASK_FEATURES_HUAWEI, VkPhysicalDeviceInvocationMaskFeaturesHUAWEI>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LEGACY_DITHERING_FEATURES_EXT, VkPhysicalDeviceLegacyDitheringFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINEAR_COLOR_ATTACHMENT_FEATURES_NV, VkPhysicalDeviceLinearColorAttachmentFeaturesNV>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_FEATURES_EXT, VkPhysicalDeviceLineRasterizationFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_PROPERTIES_EXT, VkPhysicalDeviceLineRasterizationPropertiesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_3_PROPERTIES, VkPhysicalDeviceMaintenance3Properties>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_FEATURES, VkPhysicalDeviceMaintenance4Features>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_PROPERTIES, VkPhysicalDeviceMaintenance4Properties>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_5_FEATURES_KHR, VkPhysicalDeviceMaintenance5FeaturesKHR>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_5_PROPERTIES_KHR, VkPhysicalDeviceMaintenance5PropertiesKHR>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT, VkPhysicalDeviceMemoryBudgetPropertiesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_DECOMPRESSION_FEATURES_NV, VkPhysicalDeviceMemoryDecompressionFeaturesNV>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_DECOMPRESSION_PROPERTIES_NV, VkPhysicalDeviceMemoryDecompressionPropertiesNV>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PRIORITY_FEATURES_EXT, VkPhysicalDeviceMemoryPriorityFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT, VkPhysicalDeviceMeshShaderFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV, VkPhysicalDeviceMeshShaderFeaturesNV>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_EXT, VkPhysicalDeviceMeshShaderPropertiesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_NV, VkPhysicalDeviceMeshShaderPropertiesNV>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTI_DRAW_FEATURES_EXT, VkPhysicalDeviceMultiDrawFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTI_DRAW_PROPERTIES_EXT, VkPhysicalDeviceMultiDrawPropertiesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTISAMPLED_RENDER_TO_SINGLE_SAMPLED_FEATURES_EXT, VkPhysicalDeviceMultisampledRenderToSingleSampledFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES, VkPhysicalDeviceMultiviewFeatures>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_PER_VIEW_ATTRIBUTES_PROPERTIES_NVX, VkPhysicalDeviceMultiviewPerViewAttributesPropertiesNVX>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_PER_VIEW_RENDER_AREAS_FEATURES_QCOM, VkPhysicalDeviceMultiviewPerViewRenderAreasFeaturesQCOM>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_PER_VIEW_VIEWPORTS_FEATURES_QCOM, VkPhysicalDeviceMultiviewPerViewViewportsFeaturesQCOM>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_PROPERTIES, VkPhysicalDeviceMultiviewProperties>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MUTABLE_DESCRIPTOR_TYPE_FEATURES_EXT, VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_NON_SEAMLESS_CUBE_MAP_FEATURES_EXT, VkPhysicalDeviceNonSeamlessCubeMapFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_OPACITY_MICROMAP_FEATURES_EXT, VkPhysicalDeviceOpacityMicromapFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_OPACITY_MICROMAP_PROPERTIES_EXT, VkPhysicalDeviceOpacityMicromapPropertiesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_OPTICAL_FLOW_FEATURES_NV, VkPhysicalDeviceOpticalFlowFeaturesNV>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_OPTICAL_FLOW_PROPERTIES_NV, VkPhysicalDeviceOpticalFlowPropertiesNV>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PAGEABLE_DEVICE_LOCAL_MEMORY_FEATURES_EXT, VkPhysicalDevicePageableDeviceLocalMemoryFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PCI_BUS_INFO_PROPERTIES_EXT, VkPhysicalDevicePCIBusInfoPropertiesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PERFORMANCE_QUERY_FEATURES_KHR, VkPhysicalDevicePerformanceQueryFeaturesKHR>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PERFORMANCE_QUERY_PROPERTIES_KHR, VkPhysicalDevicePerformanceQueryPropertiesKHR>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_CREATION_CACHE_CONTROL_FEATURES, VkPhysicalDevicePipelineCreationCacheControlFeatures>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_EXECUTABLE_PROPERTIES_FEATURES_KHR, VkPhysicalDevicePipelineExecutablePropertiesFeaturesKHR>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_LIBRARY_GROUP_HANDLES_FEATURES_EXT, VkPhysicalDevicePipelineLibraryGroupHandlesFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_PROPERTIES_FEATURES_EXT, VkPhysicalDevicePipelinePropertiesFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_PROTECTED_ACCESS_FEATURES_EXT, VkPhysicalDevicePipelineProtectedAccessFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_ROBUSTNESS_FEATURES_EXT, VkPhysicalDevicePipelineRobustnessFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_ROBUSTNESS_PROPERTIES_EXT, VkPhysicalDevicePipelineRobustnessPropertiesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_POINT_CLIPPING_PROPERTIES, VkPhysicalDevicePointClippingProperties>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_BARRIER_FEATURES_NV, VkPhysicalDevicePresentBarrierFeaturesNV>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_ID_FEATURES_KHR, VkPhysicalDevicePresentIdFeaturesKHR>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_WAIT_FEATURES_KHR, VkPhysicalDevicePresentWaitFeaturesKHR>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRIMITIVES_GENERATED_QUERY_FEATURES_EXT, VkPhysicalDevicePrimitivesGeneratedQueryFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRIMITIVE_TOPOLOGY_LIST_RESTART_FEATURES_EXT, VkPhysicalDevicePrimitiveTopologyListRestartFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRIVATE_DATA_FEATURES, VkPhysicalDevicePrivateDataFeatures>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROTECTED_MEMORY_FEATURES, VkPhysicalDeviceProtectedMemoryFeatures>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROTECTED_MEMORY_PROPERTIES, VkPhysicalDeviceProtectedMemoryProperties>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROVOKING_VERTEX_FEATURES_EXT, VkPhysicalDeviceProvokingVertexFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROVOKING_VERTEX_PROPERTIES_EXT, VkPhysicalDeviceProvokingVertexPropertiesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PUSH_DESCRIPTOR_PROPERTIES_KHR, VkPhysicalDevicePushDescriptorPropertiesKHR>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RASTERIZATION_ORDER_ATTACHMENT_ACCESS_FEATURES_EXT, VkPhysicalDeviceRasterizationOrderAttachmentAccessFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR, VkPhysicalDeviceRayQueryFeaturesKHR>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_INVOCATION_REORDER_FEATURES_NV, VkPhysicalDeviceRayTracingInvocationReorderFeaturesNV>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_INVOCATION_REORDER_PROPERTIES_NV, VkPhysicalDeviceRayTracingInvocationReorderPropertiesNV>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_MAINTENANCE_1_FEATURES_KHR, VkPhysicalDeviceRayTracingMaintenance1FeaturesKHR>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_MOTION_BLUR_FEATURES_NV, VkPhysicalDeviceRayTracingMotionBlurFeaturesNV>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR, VkPhysicalDeviceRayTracingPipelineFeaturesKHR>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR, VkPhysicalDeviceRayTracingPipelinePropertiesKHR>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_POSITION_FETCH_FEATURES_KHR, VkPhysicalDeviceRayTracingPositionFetchFeaturesKHR>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_NV, VkPhysicalDeviceRayTracingPropertiesNV>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_REPRESENTATIVE_FRAGMENT_TEST_FEATURES_NV, VkPhysicalDeviceRepresentativeFragmentTestFeaturesNV>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RGBA10X6_FORMATS_FEATURES_EXT, VkPhysicalDeviceRGBA10X6FormatsFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT, VkPhysicalDeviceRobustness2FeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_PROPERTIES_EXT, VkPhysicalDeviceRobustness2PropertiesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLE_LOCATIONS_PROPERTIES_EXT, VkPhysicalDeviceSampleLocationsPropertiesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLER_FILTER_MINMAX_PROPERTIES, VkPhysicalDeviceSamplerFilterMinmaxProperties>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLER_YCBCR_CONVERSION_FEATURES, VkPhysicalDeviceSamplerYcbcrConversionFeatures>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES, VkPhysicalDeviceScalarBlockLayoutFeatures>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SEPARATE_DEPTH_STENCIL_LAYOUTS_FEATURES, VkPhysicalDeviceSeparateDepthStencilLayoutsFeatures>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_2_FEATURES_EXT, VkPhysicalDeviceShaderAtomicFloat2FeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_FEATURES_EXT, VkPhysicalDeviceShaderAtomicFloatFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_INT64_FEATURES, VkPhysicalDeviceShaderAtomicInt64Features>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CLOCK_FEATURES_KHR, VkPhysicalDeviceShaderClockFeaturesKHR>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CORE_BUILTINS_FEATURES_ARM, VkPhysicalDeviceShaderCoreBuiltinsFeaturesARM>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CORE_BUILTINS_PROPERTIES_ARM, VkPhysicalDeviceShaderCoreBuiltinsPropertiesARM>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CORE_PROPERTIES_2_AMD, VkPhysicalDeviceShaderCoreProperties2AMD>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CORE_PROPERTIES_AMD, VkPhysicalDeviceShaderCorePropertiesAMD>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CORE_PROPERTIES_ARM, VkPhysicalDeviceShaderCorePropertiesARM>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DEMOTE_TO_HELPER_INVOCATION_FEATURES, VkPhysicalDeviceShaderDemoteToHelperInvocationFeatures>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES, VkPhysicalDeviceShaderDrawParametersFeatures>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_EARLY_AND_LATE_FRAGMENT_TESTS_FEATURES_AMD, VkPhysicalDeviceShaderEarlyAndLateFragmentTestsFeaturesAMD>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES, VkPhysicalDeviceShaderFloat16Int8Features>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_IMAGE_ATOMIC_INT64_FEATURES_EXT, VkPhysicalDeviceShaderImageAtomicInt64FeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_IMAGE_FOOTPRINT_FEATURES_NV, VkPhysicalDeviceShaderImageFootprintFeaturesNV>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_INTEGER_DOT_PRODUCT_FEATURES, VkPhysicalDeviceShaderIntegerDotProductFeatures>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_INTEGER_DOT_PRODUCT_PROPERTIES, VkPhysicalDeviceShaderIntegerDotProductProperties>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_INTEGER_FUNCTIONS_2_FEATURES_INTEL, VkPhysicalDeviceShaderIntegerFunctions2FeaturesINTEL>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_MODULE_IDENTIFIER_FEATURES_EXT, VkPhysicalDeviceShaderModuleIdentifierFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_MODULE_IDENTIFIER_PROPERTIES_EXT, VkPhysicalDeviceShaderModuleIdentifierPropertiesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_OBJECT_FEATURES_EXT, VkPhysicalDeviceShaderObjectFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_OBJECT_PROPERTIES_EXT, VkPhysicalDeviceShaderObjectPropertiesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SM_BUILTINS_FEATURES_NV, VkPhysicalDeviceShaderSMBuiltinsFeaturesNV>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SM_BUILTINS_PROPERTIES_NV, VkPhysicalDeviceShaderSMBuiltinsPropertiesNV>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SUBGROUP_EXTENDED_TYPES_FEATURES, VkPhysicalDeviceShaderSubgroupExtendedTypesFeatures>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SUBGROUP_UNIFORM_CONTROL_FLOW_FEATURES_KHR, VkPhysicalDeviceShaderSubgroupUniformControlFlowFeaturesKHR>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_TERMINATE_INVOCATION_FEATURES, VkPhysicalDeviceShaderTerminateInvocationFeatures>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_TILE_IMAGE_FEATURES_EXT, VkPhysicalDeviceShaderTileImageFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_TILE_IMAGE_PROPERTIES_EXT, VkPhysicalDeviceShaderTileImagePropertiesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADING_RATE_IMAGE_FEATURES_NV, VkPhysicalDeviceShadingRateImageFeaturesNV>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADING_RATE_IMAGE_PROPERTIES_NV, VkPhysicalDeviceShadingRateImagePropertiesNV>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES, VkPhysicalDeviceSubgroupProperties>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_FEATURES, VkPhysicalDeviceSubgroupSizeControlFeatures>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_PROPERTIES, VkPhysicalDeviceSubgroupSizeControlProperties>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBPASS_MERGE_FEEDBACK_FEATURES_EXT, VkPhysicalDeviceSubpassMergeFeedbackFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBPASS_SHADING_FEATURES_HUAWEI, VkPhysicalDeviceSubpassShadingFeaturesHUAWEI>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBPASS_SHADING_PROPERTIES_HUAWEI, VkPhysicalDeviceSubpassShadingPropertiesHUAWEI>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SWAPCHAIN_MAINTENANCE_1_FEATURES_EXT, VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES, VkPhysicalDeviceSynchronization2Features>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TEXEL_BUFFER_ALIGNMENT_FEATURES_EXT, VkPhysicalDeviceTexelBufferAlignmentFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TEXEL_BUFFER_ALIGNMENT_PROPERTIES, VkPhysicalDeviceTexelBufferAlignmentProperties>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TEXTURE_COMPRESSION_ASTC_HDR_FEATURES, VkPhysicalDeviceTextureCompressionASTCHDRFeatures>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TILE_PROPERTIES_FEATURES_QCOM, VkPhysicalDeviceTilePropertiesFeaturesQCOM>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES, VkPhysicalDeviceTimelineSemaphoreFeatures>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_PROPERTIES, VkPhysicalDeviceTimelineSemaphoreProperties>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_FEATURES_EXT, VkPhysicalDeviceTransformFeedbackFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_PROPERTIES_EXT, VkPhysicalDeviceTransformFeedbackPropertiesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_UNIFORM_BUFFER_STANDARD_LAYOUT_FEATURES, VkPhysicalDeviceUniformBufferStandardLayoutFeatures>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VARIABLE_POINTERS_FEATURES, VkPhysicalDeviceVariablePointersFeatures>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_ATTRIBUTE_DIVISOR_FEATURES_EXT, VkPhysicalDeviceVertexAttributeDivisorFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_ATTRIBUTE_DIVISOR_PROPERTIES_EXT, VkPhysicalDeviceVertexAttributeDivisorPropertiesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_INPUT_DYNAMIC_STATE_FEATURES_EXT, VkPhysicalDeviceVertexInputDynamicStateFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES, VkPhysicalDeviceVulkan11Features>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES, VkPhysicalDeviceVulkan11Properties>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES, VkPhysicalDeviceVulkan12Features>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES, VkPhysicalDeviceVulkan12Properties>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES, VkPhysicalDeviceVulkan13Features>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES, VkPhysicalDeviceVulkan13Properties>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_MEMORY_MODEL_FEATURES, VkPhysicalDeviceVulkanMemoryModelFeatures>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_WORKGROUP_MEMORY_EXPLICIT_LAYOUT_FEATURES_KHR, VkPhysicalDeviceWorkgroupMemoryExplicitLayoutFeaturesKHR>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_YCBCR_2_PLANE_444_FORMATS_FEATURES_EXT, VkPhysicalDeviceYcbcr2Plane444FormatsFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_YCBCR_IMAGE_ARRAYS_FEATURES_EXT, VkPhysicalDeviceYcbcrImageArraysFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ZERO_INITIALIZE_WORKGROUP_MEMORY_FEATURES, VkPhysicalDeviceZeroInitializeWorkgroupMemoryFeatures>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_ADVANCED_STATE_CREATE_INFO_EXT, VkPipelineColorBlendAdvancedStateCreateInfoEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_COLOR_WRITE_CREATE_INFO_EXT, VkPipelineColorWriteCreateInfoEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_COMPILER_CONTROL_CREATE_INFO_AMD, VkPipelineCompilerControlCreateInfoAMD>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_COVERAGE_MODULATION_STATE_CREATE_INFO_NV, VkPipelineCoverageModulationStateCreateInfoNV>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_COVERAGE_REDUCTION_STATE_CREATE_INFO_NV, VkPipelineCoverageReductionStateCreateInfoNV>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_COVERAGE_TO_COLOR_STATE_CREATE_INFO_NV, VkPipelineCoverageToColorStateCreateInfoNV>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_CREATE_FLAGS_2_CREATE_INFO_KHR, VkPipelineCreateFlags2CreateInfoKHR>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_DISCARD_RECTANGLE_STATE_CREATE_INFO_EXT, VkPipelineDiscardRectangleStateCreateInfoEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_FRAGMENT_SHADING_RATE_ENUM_STATE_CREATE_INFO_NV, VkPipelineFragmentShadingRateEnumStateCreateInfoNV>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_FRAGMENT_SHADING_RATE_STATE_CREATE_INFO_KHR, VkPipelineFragmentShadingRateStateCreateInfoKHR>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_LIBRARY_CREATE_INFO_KHR, VkPipelineLibraryCreateInfoKHR>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_CONSERVATIVE_STATE_CREATE_INFO_EXT, VkPipelineRasterizationConservativeStateCreateInfoEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_DEPTH_CLIP_STATE_CREATE_INFO_EXT, VkPipelineRasterizationDepthClipStateCreateInfoEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_LINE_STATE_CREATE_INFO_EXT, VkPipelineRasterizationLineStateCreateInfoEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_PROVOKING_VERTEX_STATE_CREATE_INFO_EXT, VkPipelineRasterizationProvokingVertexStateCreateInfoEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_RASTERIZATION_ORDER_AMD, VkPipelineRasterizationStateRasterizationOrderAMD>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_STREAM_CREATE_INFO_EXT, VkPipelineRasterizationStateStreamCreateInfoEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO, VkPipelineRenderingCreateInfo>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_REPRESENTATIVE_FRAGMENT_TEST_STATE_CREATE_INFO_NV, VkPipelineRepresentativeFragmentTestStateCreateInfoNV>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_ROBUSTNESS_CREATE_INFO_EXT, VkPipelineRobustnessCreateInfoEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_SAMPLE_LOCATIONS_STATE_CREATE_INFO_EXT, VkPipelineSampleLocationsStateCreateInfoEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_MODULE_IDENTIFIER_CREATE_INFO_EXT, VkPipelineShaderStageModuleIdentifierCreateInfoEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_REQUIRED_SUBGROUP_SIZE_CREATE_INFO, VkPipelineShaderStageRequiredSubgroupSizeCreateInfo>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_DOMAIN_ORIGIN_STATE_CREATE_INFO, VkPipelineTessellationDomainOriginStateCreateInfo>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_DIVISOR_STATE_CREATE_INFO_EXT, VkPipelineVertexInputDivisorStateCreateInfoEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_DEPTH_CLIP_CONTROL_CREATE_INFO_EXT, VkPipelineViewportDepthClipControlCreateInfoEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_EXCLUSIVE_SCISSOR_STATE_CREATE_INFO_NV, VkPipelineViewportExclusiveScissorStateCreateInfoNV>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_SWIZZLE_STATE_CREATE_INFO_NV, VkPipelineViewportSwizzleStateCreateInfoNV>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_W_SCALING_STATE_CREATE_INFO_NV, VkPipelineViewportWScalingStateCreateInfoNV>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PRESENT_ID_KHR, VkPresentIdKHR>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PROTECTED_SUBMIT_INFO, VkProtectedSubmitInfo>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_QUERY_POOL_PERFORMANCE_CREATE_INFO_KHR, VkQueryPoolPerformanceCreateInfoKHR>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_QUERY_POOL_PERFORMANCE_QUERY_CREATE_INFO_INTEL, VkQueryPoolPerformanceQueryCreateInfoINTEL>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_QUEUE_FAMILY_CHECKPOINT_PROPERTIES_2_NV, VkQueueFamilyCheckpointProperties2NV>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_QUEUE_FAMILY_CHECKPOINT_PROPERTIES_NV, VkQueueFamilyCheckpointPropertiesNV>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_QUEUE_FAMILY_GLOBAL_PRIORITY_PROPERTIES_KHR, VkQueueFamilyGlobalPriorityPropertiesKHR>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_QUEUE_FAMILY_QUERY_RESULT_STATUS_PROPERTIES_KHR, VkQueueFamilyQueryResultStatusPropertiesKHR>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_QUEUE_FAMILY_VIDEO_PROPERTIES_KHR, VkQueueFamilyVideoPropertiesKHR>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_RENDERING_FRAGMENT_DENSITY_MAP_ATTACHMENT_INFO_EXT, VkRenderingFragmentDensityMapAttachmentInfoEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_RENDERING_FRAGMENT_SHADING_RATE_ATTACHMENT_INFO_KHR, VkRenderingFragmentShadingRateAttachmentInfoKHR>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_RENDER_PASS_ATTACHMENT_BEGIN_INFO, VkRenderPassAttachmentBeginInfo>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_RENDER_PASS_CREATION_CONTROL_EXT, VkRenderPassCreationControlEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_RENDER_PASS_CREATION_FEEDBACK_CREATE_INFO_EXT, VkRenderPassCreationFeedbackCreateInfoEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_RENDER_PASS_FRAGMENT_DENSITY_MAP_CREATE_INFO_EXT, VkRenderPassFragmentDensityMapCreateInfoEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_RENDER_PASS_INPUT_ATTACHMENT_ASPECT_CREATE_INFO, VkRenderPassInputAttachmentAspectCreateInfo>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_RENDER_PASS_MULTIVIEW_CREATE_INFO, VkRenderPassMultiviewCreateInfo>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_RENDER_PASS_SUBPASS_FEEDBACK_CREATE_INFO_EXT, VkRenderPassSubpassFeedbackCreateInfoEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_RENDER_PASS_TRANSFORM_BEGIN_INFO_QCOM, VkRenderPassTransformBeginInfoQCOM>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_SAMPLE_LOCATIONS_INFO_EXT, VkSampleLocationsInfoEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_SAMPLER_BORDER_COLOR_COMPONENT_MAPPING_CREATE_INFO_EXT, VkSamplerBorderColorComponentMappingCreateInfoEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_SAMPLER_CUSTOM_BORDER_COLOR_CREATE_INFO_EXT, VkSamplerCustomBorderColorCreateInfoEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO, VkSamplerReductionModeCreateInfo>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_IMAGE_FORMAT_PROPERTIES, VkSamplerYcbcrConversionImageFormatProperties>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_INFO, VkSamplerYcbcrConversionInfo>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO, VkSemaphoreTypeCreateInfo>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, VkShaderModuleCreateInfo>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_SHADER_MODULE_VALIDATION_CACHE_CREATE_INFO_EXT, VkShaderModuleValidationCacheCreateInfoEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_SHARED_PRESENT_SURFACE_CAPABILITIES_KHR, VkSharedPresentSurfaceCapabilitiesKHR>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_SUBPASS_FRAGMENT_DENSITY_MAP_OFFSET_END_INFO_QCOM, VkSubpassFragmentDensityMapOffsetEndInfoQCOM>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_SUBPASS_RESOLVE_PERFORMANCE_QUERY_EXT, VkSubpassResolvePerformanceQueryEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_SUBPASS_SHADING_PIPELINE_CREATE_INFO_HUAWEI, VkSubpassShadingPipelineCreateInfoHUAWEI>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_SUBRESOURCE_HOST_MEMCPY_SIZE_EXT, VkSubresourceHostMemcpySizeEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_PRESENT_BARRIER_NV, VkSurfaceCapabilitiesPresentBarrierNV>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_SURFACE_PRESENT_MODE_COMPATIBILITY_EXT, VkSurfacePresentModeCompatibilityEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_SURFACE_PRESENT_MODE_EXT, VkSurfacePresentModeEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_SURFACE_PRESENT_SCALING_CAPABILITIES_EXT, VkSurfacePresentScalingCapabilitiesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_SURFACE_PROTECTED_CAPABILITIES_KHR, VkSurfaceProtectedCapabilitiesKHR>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_SWAPCHAIN_COUNTER_CREATE_INFO_EXT, VkSwapchainCounterCreateInfoEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_SWAPCHAIN_DISPLAY_NATIVE_HDR_CREATE_INFO_AMD, VkSwapchainDisplayNativeHdrCreateInfoAMD>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_BARRIER_CREATE_INFO_NV, VkSwapchainPresentBarrierCreateInfoNV>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_FENCE_INFO_EXT, VkSwapchainPresentFenceInfoEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_MODE_INFO_EXT, VkSwapchainPresentModeInfoEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_MODES_CREATE_INFO_EXT, VkSwapchainPresentModesCreateInfoEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_SCALING_CREATE_INFO_EXT, VkSwapchainPresentScalingCreateInfoEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_TEXTURE_LOD_GATHER_FORMAT_PROPERTIES_AMD, VkTextureLODGatherFormatPropertiesAMD>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO, VkTimelineSemaphoreSubmitInfo>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT, VkValidationFeaturesEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_VALIDATION_FLAGS_EXT, VkValidationFlagsEXT>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_VIDEO_DECODE_CAPABILITIES_KHR, VkVideoDecodeCapabilitiesKHR>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_CAPABILITIES_KHR, VkVideoDecodeH264CapabilitiesKHR>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_DPB_SLOT_INFO_KHR, VkVideoDecodeH264DpbSlotInfoKHR>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_PICTURE_INFO_KHR, VkVideoDecodeH264PictureInfoKHR>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_PROFILE_INFO_KHR, VkVideoDecodeH264ProfileInfoKHR>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_CAPABILITIES_KHR, VkVideoDecodeH265CapabilitiesKHR>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_DPB_SLOT_INFO_KHR, VkVideoDecodeH265DpbSlotInfoKHR>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_PICTURE_INFO_KHR, VkVideoDecodeH265PictureInfoKHR>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_PROFILE_INFO_KHR, VkVideoDecodeH265ProfileInfoKHR>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_VIDEO_DECODE_USAGE_INFO_KHR, VkVideoDecodeUsageInfoKHR>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_VIDEO_PROFILE_INFO_KHR, VkVideoProfileInfoKHR>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR, VkWriteDescriptorSetAccelerationStructureKHR>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_NV, VkWriteDescriptorSetAccelerationStructureNV>,
};

static void default_fex_custom_repack_entry(VkBaseOutStructure& into, const guest_layout<VkBaseOutStructure>* from) {
  if (!from->data.pNext.get_pointer()) {
    into.pNext = nullptr;
    return;
  }
  auto typed_source = reinterpret_cast<const guest_layout<VkBaseOutStructure>*>(from->data.pNext.get_pointer());

  auto next_handler = next_handlers.find(static_cast<VkStructureType>(typed_source->data.sType.data));
  if (next_handler == next_handlers.end()) {
    fprintf(stderr, "ERROR: Unrecognized VkStructureType %u referenced by pNext\n", typed_source->data.sType.data);
    std::abort();
  }

  into.pNext = next_handler->second.first(typed_source);
}

template<typename T>
void default_fex_custom_repack_entry(host_layout<T>& into, const guest_layout<T>& from) {
  default_fex_custom_repack_entry(*(VkBaseOutStructure*)&into.data, reinterpret_cast<const guest_layout<VkBaseOutStructure>*>(&from));
}

static void default_fex_custom_repack_reverse(guest_layout<VkBaseOutStructure>& into, const VkBaseOutStructure* from) {
  auto pNextHost = from->pNext;
  if (!pNextHost) {
    return;
  }

  auto next_handler = next_handlers.find(static_cast<VkStructureType>(into.data.pNext.get_pointer()->data.sType.data));
  if (next_handler == next_handlers.end()) {
    fprintf(stderr, "ERROR: Unrecognized VkStructureType %u referenced by pNext when converting to guest\n", from->sType);
    std::abort();
  }
  next_handler->second.second((void*)into.data.pNext.get_pointer(), from->pNext);

  free(pNextHost);
}

// Default repacking functions that only traverses and repacks the pNext chain.
// If other members need to be repacked, use VULKAN_NONDEFAULT_CUSTOM_REPACK instead
#define VULKAN_DEFAULT_CUSTOM_REPACK(name)                                                             \
  void fex_custom_repack_entry(host_layout<name>& into, const guest_layout<name>& from) {              \
    default_fex_custom_repack_entry(reinterpret_cast<VkBaseOutStructure&>(into.data),                  \
                                    &reinterpret_cast<const guest_layout<VkBaseOutStructure>&>(from)); \
  }                                                                                                    \
                                                                                                       \
  bool fex_custom_repack_exit(guest_layout<name>& into, const host_layout<name>& from) {               \
    auto prev_next = into.data.pNext;                                                                  \
    default_fex_custom_repack_reverse(*reinterpret_cast<guest_layout<VkBaseOutStructure>*>(&into),     \
                                      &reinterpret_cast<const VkBaseOutStructure&>(from.data));        \
    into = to_guest(from);                                                                             \
    into.data.pNext = prev_next;                                                                       \
    return true;                                                                                       \
  }

// Intentionally left empty. This macro doesn't automate anything, but it
// helps ensure we don't forget any Vulkan types in the list. The actual
// repacking functions are defined manually later
#define VULKAN_NONDEFAULT_CUSTOM_REPACK(name)

// VULKAN_DEFAULT_CUSTOM_REPACK(VkAccelerationStructureBuildGeometryInfoKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkAccelerationStructureBuildSizesInfoKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkAccelerationStructureCaptureDescriptorDataInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkAccelerationStructureCreateInfoKHR)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkAccelerationStructureCreateInfoNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkAccelerationStructureDeviceAddressInfoKHR)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkAccelerationStructureGeometryAabbsDataKHR)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkAccelerationStructureGeometryInstancesDataKHR)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkAccelerationStructureGeometryKHR)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkAccelerationStructureGeometryMotionTrianglesDataNV)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkAccelerationStructureGeometryTrianglesDataKHR)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkAccelerationStructureInfoNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkAccelerationStructureMemoryRequirementsInfoNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkAccelerationStructureMotionInfoNV)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkAccelerationStructureTrianglesOpacityMicromapEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkAccelerationStructureVersionInfoKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkAcquireNextImageInfoKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkAcquireProfilingLockInfoKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkAmigoProfilingSubmitInfoSEC)
VULKAN_DEFAULT_CUSTOM_REPACK(VkApplicationInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkAttachmentDescription2)
VULKAN_DEFAULT_CUSTOM_REPACK(VkAttachmentDescriptionStencilLayout)
VULKAN_DEFAULT_CUSTOM_REPACK(VkAttachmentReference2)
VULKAN_DEFAULT_CUSTOM_REPACK(VkAttachmentReferenceStencilLayout)
VULKAN_DEFAULT_CUSTOM_REPACK(VkAttachmentSampleCountInfoAMD)
VULKAN_DEFAULT_CUSTOM_REPACK(VkBindAccelerationStructureMemoryInfoNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkBindBufferMemoryDeviceGroupInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkBindBufferMemoryInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkBindImageMemoryDeviceGroupInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkBindImageMemoryInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkBindImageMemorySwapchainInfoKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkBindImagePlaneMemoryInfo)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkBindSparseInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkBindVideoSessionMemoryInfoKHR)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkBlitImageInfo2)
VULKAN_DEFAULT_CUSTOM_REPACK(VkBufferCaptureDescriptorDataInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkBufferCopy2)
VULKAN_DEFAULT_CUSTOM_REPACK(VkBufferCreateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkBufferDeviceAddressCreateInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkBufferDeviceAddressInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkBufferImageCopy2)
VULKAN_DEFAULT_CUSTOM_REPACK(VkBufferMemoryBarrier)
VULKAN_DEFAULT_CUSTOM_REPACK(VkBufferMemoryBarrier2)
VULKAN_DEFAULT_CUSTOM_REPACK(VkBufferMemoryRequirementsInfo2)
VULKAN_DEFAULT_CUSTOM_REPACK(VkBufferOpaqueCaptureAddressCreateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkBufferUsageFlags2CreateInfoKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkBufferViewCreateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkCalibratedTimestampInfoEXT)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkCheckpointData2NV)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkCheckpointDataNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkCommandBufferAllocateInfo)
VULKAN_NONDEFAULT_CUSTOM_REPACK(VkCommandBufferBeginInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkCommandBufferInheritanceConditionalRenderingInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkCommandBufferInheritanceInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkCommandBufferInheritanceRenderingInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkCommandBufferInheritanceRenderPassTransformInfoQCOM)
VULKAN_DEFAULT_CUSTOM_REPACK(VkCommandBufferInheritanceViewportScissorInfoNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkCommandBufferSubmitInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkCommandPoolCreateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkComputePipelineCreateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkComputePipelineIndirectBufferInfoNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkConditionalRenderingBeginInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkCooperativeMatrixPropertiesKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkCooperativeMatrixPropertiesNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkCopyAccelerationStructureInfoKHR)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkCopyAccelerationStructureToMemoryInfoKHR)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkCopyBufferInfo2)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkCopyBufferToImageInfo2)
VULKAN_DEFAULT_CUSTOM_REPACK(VkCopyCommandTransformInfoQCOM)
VULKAN_DEFAULT_CUSTOM_REPACK(VkCopyDescriptorSet)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkCopyImageInfo2)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkCopyImageToBufferInfo2)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkCopyImageToImageInfoEXT)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkCopyImageToMemoryInfoEXT)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkCopyMemoryToAccelerationStructureInfoKHR)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkCopyMemoryToImageInfoEXT)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkCopyMemoryToMicromapInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkCopyMicromapInfoEXT)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkCopyMicromapToMemoryInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkCuFunctionCreateInfoNVX)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkCuLaunchInfoNVX)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkCuModuleCreateInfoNVX)
VULKAN_DEFAULT_CUSTOM_REPACK(VkDebugMarkerMarkerInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkDebugMarkerObjectNameInfoEXT)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkDebugMarkerObjectTagInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkDebugReportCallbackCreateInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkDebugUtilsLabelEXT)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkDebugUtilsMessengerCallbackDataEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkDebugUtilsMessengerCreateInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkDebugUtilsObjectNameInfoEXT)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkDebugUtilsObjectTagInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkDedicatedAllocationBufferCreateInfoNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkDedicatedAllocationImageCreateInfoNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkDedicatedAllocationMemoryAllocateInfoNV)
VULKAN_NONDEFAULT_CUSTOM_REPACK(VkDependencyInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkDepthBiasInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkDepthBiasRepresentationInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkDescriptorAddressInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkDescriptorBufferBindingInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkDescriptorBufferBindingPushDescriptorBufferHandleEXT)
VULKAN_NONDEFAULT_CUSTOM_REPACK(VkDescriptorGetInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkDescriptorPoolCreateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkDescriptorPoolInlineUniformBlockCreateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkDescriptorSetAllocateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkDescriptorSetBindingReferenceVALVE)
VULKAN_DEFAULT_CUSTOM_REPACK(VkDescriptorSetLayoutBindingFlagsCreateInfo)
VULKAN_NONDEFAULT_CUSTOM_REPACK(VkDescriptorSetLayoutCreateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkDescriptorSetLayoutHostMappingInfoVALVE)
VULKAN_DEFAULT_CUSTOM_REPACK(VkDescriptorSetLayoutSupport)
VULKAN_DEFAULT_CUSTOM_REPACK(VkDescriptorSetVariableDescriptorCountAllocateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkDescriptorSetVariableDescriptorCountLayoutSupport)
VULKAN_NONDEFAULT_CUSTOM_REPACK(VkDescriptorUpdateTemplateCreateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkDeviceAddressBindingCallbackDataEXT)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkDeviceBufferMemoryRequirements)
VULKAN_NONDEFAULT_CUSTOM_REPACK(VkDeviceCreateInfo)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkDeviceDeviceMemoryReportCreateInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkDeviceDiagnosticsConfigCreateInfoNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkDeviceEventInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkDeviceFaultCountsEXT)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkDeviceFaultInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkDeviceGroupBindSparseInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkDeviceGroupCommandBufferBeginInfo)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkDeviceGroupDeviceCreateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkDeviceGroupPresentCapabilitiesKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkDeviceGroupPresentInfoKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkDeviceGroupRenderPassBeginInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkDeviceGroupSubmitInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkDeviceGroupSwapchainCreateInfoKHR)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkDeviceImageMemoryRequirements)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkDeviceImageSubresourceInfoKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkDeviceMemoryOpaqueCaptureAddressInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkDeviceMemoryOverallocationCreateInfoAMD)
VULKAN_DEFAULT_CUSTOM_REPACK(VkDeviceMemoryReportCallbackDataEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkDevicePrivateDataCreateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkDeviceQueueCreateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkDeviceQueueGlobalPriorityCreateInfoKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkDeviceQueueInfo2)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkDirectDriverLoadingInfoLUNARG)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkDirectDriverLoadingListLUNARG)
VULKAN_DEFAULT_CUSTOM_REPACK(VkDisplayEventInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkDisplayModeCreateInfoKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkDisplayModeProperties2KHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkDisplayNativeHdrSurfaceCapabilitiesAMD)
VULKAN_DEFAULT_CUSTOM_REPACK(VkDisplayPlaneCapabilities2KHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkDisplayPlaneInfo2KHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkDisplayPlaneProperties2KHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkDisplayPowerInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkDisplayPresentInfoKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkDisplayProperties2KHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkDisplaySurfaceCreateInfoKHR)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkDrmFormatModifierPropertiesList2EXT)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkDrmFormatModifierPropertiesListEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkEventCreateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkExportFenceCreateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkExportMemoryAllocateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkExportMemoryAllocateInfoNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkExportSemaphoreCreateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkExternalBufferProperties)
VULKAN_DEFAULT_CUSTOM_REPACK(VkExternalFenceProperties)
VULKAN_DEFAULT_CUSTOM_REPACK(VkExternalImageFormatProperties)
VULKAN_DEFAULT_CUSTOM_REPACK(VkExternalMemoryAcquireUnmodifiedEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkExternalMemoryBufferCreateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkExternalMemoryImageCreateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkExternalMemoryImageCreateInfoNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkExternalSemaphoreProperties)
VULKAN_DEFAULT_CUSTOM_REPACK(VkFenceCreateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkFenceGetFdInfoKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkFilterCubicImageViewImageFormatPropertiesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkFormatProperties2)
VULKAN_DEFAULT_CUSTOM_REPACK(VkFormatProperties3)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkFragmentShadingRateAttachmentInfoKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkFramebufferAttachmentImageInfo)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkFramebufferAttachmentsCreateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkFramebufferCreateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkFramebufferMixedSamplesCombinationNV)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkGeneratedCommandsInfoNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkGeneratedCommandsMemoryRequirementsInfoNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkGeometryAABBNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkGeometryNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkGeometryTrianglesNV)
VULKAN_NONDEFAULT_CUSTOM_REPACK(VkGraphicsPipelineCreateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkGraphicsPipelineLibraryCreateInfoEXT)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkGraphicsPipelineShaderGroupsCreateInfoNV)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkGraphicsShaderGroupCreateInfoNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkHeadlessSurfaceCreateInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkHostImageCopyDevicePerformanceQueryEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkHostImageLayoutTransitionInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkImageBlit2)
VULKAN_DEFAULT_CUSTOM_REPACK(VkImageCaptureDescriptorDataInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkImageCompressionControlEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkImageCompressionPropertiesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkImageCopy2)
VULKAN_DEFAULT_CUSTOM_REPACK(VkImageCreateInfo)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkImageDrmFormatModifierExplicitCreateInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkImageDrmFormatModifierListCreateInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkImageDrmFormatModifierPropertiesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkImageFormatListCreateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkImageFormatProperties2)
VULKAN_DEFAULT_CUSTOM_REPACK(VkImageMemoryBarrier)
VULKAN_DEFAULT_CUSTOM_REPACK(VkImageMemoryBarrier2)
VULKAN_DEFAULT_CUSTOM_REPACK(VkImageMemoryRequirementsInfo2)
VULKAN_DEFAULT_CUSTOM_REPACK(VkImagePlaneMemoryRequirementsInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkImageResolve2)
VULKAN_DEFAULT_CUSTOM_REPACK(VkImageSparseMemoryRequirementsInfo2)
VULKAN_DEFAULT_CUSTOM_REPACK(VkImageStencilUsageCreateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkImageSubresource2KHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkImageSwapchainCreateInfoKHR)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkImageToMemoryCopyEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkImageViewAddressPropertiesNVX)
VULKAN_DEFAULT_CUSTOM_REPACK(VkImageViewASTCDecodeModeEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkImageViewCaptureDescriptorDataInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkImageViewCreateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkImageViewHandleInfoNVX)
VULKAN_DEFAULT_CUSTOM_REPACK(VkImageViewMinLodCreateInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkImageViewSampleWeightCreateInfoQCOM)
VULKAN_DEFAULT_CUSTOM_REPACK(VkImageViewSlicedCreateInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkImageViewUsageCreateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkImportFenceFdInfoKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkImportMemoryFdInfoKHR)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkImportMemoryHostPointerInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkImportSemaphoreFdInfoKHR)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkIndirectCommandsLayoutCreateInfoNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkIndirectCommandsLayoutTokenNV)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkInitializePerformanceApiInfoINTEL)
VULKAN_NONDEFAULT_CUSTOM_REPACK(VkInstanceCreateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkMappedMemoryRange)
VULKAN_DEFAULT_CUSTOM_REPACK(VkMemoryAllocateFlagsInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkMemoryAllocateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkMemoryBarrier)
VULKAN_DEFAULT_CUSTOM_REPACK(VkMemoryBarrier2)
VULKAN_DEFAULT_CUSTOM_REPACK(VkMemoryDedicatedAllocateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkMemoryDedicatedRequirements)
VULKAN_DEFAULT_CUSTOM_REPACK(VkMemoryFdPropertiesKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkMemoryGetFdInfoKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkMemoryGetRemoteAddressInfoNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkMemoryHostPointerPropertiesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkMemoryMapInfoKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkMemoryOpaqueCaptureAddressAllocateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkMemoryPriorityAllocateInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkMemoryRequirements2)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkMemoryToImageCopyEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkMemoryUnmapInfoKHR)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkMicromapBuildInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkMicromapBuildSizesInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkMicromapCreateInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkMicromapVersionInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkMultisampledRenderToSingleSampledInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkMultisamplePropertiesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkMultiviewPerViewAttributesInfoNVX)
VULKAN_DEFAULT_CUSTOM_REPACK(VkMultiviewPerViewRenderAreasRenderPassBeginInfoQCOM)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkMutableDescriptorTypeCreateInfoEXT)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkOpaqueCaptureDescriptorDataCreateInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkOpticalFlowExecuteInfoNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkOpticalFlowImageFormatInfoNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkOpticalFlowImageFormatPropertiesNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkOpticalFlowSessionCreateInfoNV)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkOpticalFlowSessionCreatePrivateDataInfoNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPerformanceConfigurationAcquireInfoINTEL)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPerformanceCounterDescriptionKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPerformanceCounterKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPerformanceMarkerInfoINTEL)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPerformanceOverrideInfoINTEL)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPerformanceQuerySubmitInfoKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPerformanceStreamMarkerInfoINTEL)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDevice16BitStorageFeatures)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDevice4444FormatsFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDevice8BitStorageFeatures)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceAccelerationStructureFeaturesKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceAccelerationStructurePropertiesKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceAddressBindingReportFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceAmigoProfilingFeaturesSEC)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceASTCDecodeFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceAttachmentFeedbackLoopDynamicStateFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceAttachmentFeedbackLoopLayoutFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceBlendOperationAdvancedFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceBlendOperationAdvancedPropertiesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceBorderColorSwizzleFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceBufferDeviceAddressFeatures)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceBufferDeviceAddressFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceClusterCullingShaderFeaturesHUAWEI)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceClusterCullingShaderPropertiesHUAWEI)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceCoherentMemoryFeaturesAMD)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceColorWriteEnableFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceComputeShaderDerivativesFeaturesNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceConditionalRenderingFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceConservativeRasterizationPropertiesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceCooperativeMatrixFeaturesKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceCooperativeMatrixFeaturesNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceCooperativeMatrixPropertiesKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceCooperativeMatrixPropertiesNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceCopyMemoryIndirectFeaturesNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceCopyMemoryIndirectPropertiesNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceCornerSampledImageFeaturesNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceCoverageReductionModeFeaturesNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceCustomBorderColorFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceCustomBorderColorPropertiesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceDedicatedAllocationImageAliasingFeaturesNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceDepthBiasControlFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceDepthClampZeroOneFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceDepthClipControlFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceDepthClipEnableFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceDepthStencilResolveProperties)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceDescriptorBufferDensityMapPropertiesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceDescriptorBufferFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceDescriptorBufferPropertiesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceDescriptorIndexingFeatures)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceDescriptorIndexingProperties)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceDescriptorSetHostMappingFeaturesVALVE)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceDeviceGeneratedCommandsComputeFeaturesNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceDeviceGeneratedCommandsFeaturesNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceDeviceGeneratedCommandsPropertiesNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceDeviceMemoryReportFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceDiagnosticsConfigFeaturesNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceDiscardRectanglePropertiesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceDriverProperties)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceDrmPropertiesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceDynamicRenderingFeatures)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceDynamicRenderingUnusedAttachmentsFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceExclusiveScissorFeaturesNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceExtendedDynamicState2FeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceExtendedDynamicState3FeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceExtendedDynamicState3PropertiesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceExtendedDynamicStateFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceExternalBufferInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceExternalFenceInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceExternalImageFormatInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceExternalMemoryHostPropertiesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceExternalMemoryRDMAFeaturesNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceExternalSemaphoreInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceFaultFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceFeatures2)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceFloatControlsProperties)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceFragmentDensityMap2FeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceFragmentDensityMap2PropertiesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceFragmentDensityMapFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceFragmentDensityMapOffsetFeaturesQCOM)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceFragmentDensityMapOffsetPropertiesQCOM)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceFragmentDensityMapPropertiesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceFragmentShaderBarycentricFeaturesKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceFragmentShaderBarycentricPropertiesKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceFragmentShadingRateEnumsFeaturesNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceFragmentShadingRateEnumsPropertiesNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceFragmentShadingRateFeaturesKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceFragmentShadingRateKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceFragmentShadingRatePropertiesKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceGlobalPriorityQueryFeaturesKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceGraphicsPipelineLibraryFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceGraphicsPipelineLibraryPropertiesEXT)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceGroupProperties)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceHostImageCopyFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceHostImageCopyPropertiesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceHostQueryResetFeatures)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceIDProperties)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceImage2DViewOf3DFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceImageCompressionControlFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceImageCompressionControlSwapchainFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceImageDrmFormatModifierInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceImageFormatInfo2)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceImagelessFramebufferFeatures)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceImageProcessingFeaturesQCOM)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceImageProcessingPropertiesQCOM)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceImageRobustnessFeatures)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceImageSlicedViewOf3DFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceImageViewImageFormatInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceImageViewMinLodFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceIndexTypeUint8FeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceInheritedViewportScissorFeaturesNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceInlineUniformBlockFeatures)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceInlineUniformBlockProperties)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceInvocationMaskFeaturesHUAWEI)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceLegacyDitheringFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceLinearColorAttachmentFeaturesNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceLineRasterizationFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceLineRasterizationPropertiesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceMaintenance3Properties)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceMaintenance4Features)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceMaintenance4Properties)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceMaintenance5FeaturesKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceMaintenance5PropertiesKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceMemoryBudgetPropertiesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceMemoryDecompressionFeaturesNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceMemoryDecompressionPropertiesNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceMemoryPriorityFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceMemoryProperties2)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceMeshShaderFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceMeshShaderFeaturesNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceMeshShaderPropertiesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceMeshShaderPropertiesNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceMultiDrawFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceMultiDrawPropertiesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceMultisampledRenderToSingleSampledFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceMultiviewFeatures)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceMultiviewPerViewAttributesPropertiesNVX)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceMultiviewPerViewRenderAreasFeaturesQCOM)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceMultiviewPerViewViewportsFeaturesQCOM)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceMultiviewProperties)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceNonSeamlessCubeMapFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceOpacityMicromapFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceOpacityMicromapPropertiesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceOpticalFlowFeaturesNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceOpticalFlowPropertiesNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDevicePageableDeviceLocalMemoryFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDevicePCIBusInfoPropertiesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDevicePerformanceQueryFeaturesKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDevicePerformanceQueryPropertiesKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDevicePipelineCreationCacheControlFeatures)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDevicePipelineExecutablePropertiesFeaturesKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDevicePipelineLibraryGroupHandlesFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDevicePipelinePropertiesFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDevicePipelineProtectedAccessFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDevicePipelineRobustnessFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDevicePipelineRobustnessPropertiesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDevicePointClippingProperties)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDevicePresentBarrierFeaturesNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDevicePresentIdFeaturesKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDevicePresentWaitFeaturesKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDevicePrimitivesGeneratedQueryFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDevicePrimitiveTopologyListRestartFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDevicePrivateDataFeatures)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceProperties2)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceProtectedMemoryFeatures)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceProtectedMemoryProperties)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceProvokingVertexFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceProvokingVertexPropertiesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDevicePushDescriptorPropertiesKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceRasterizationOrderAttachmentAccessFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceRayQueryFeaturesKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceRayTracingInvocationReorderFeaturesNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceRayTracingInvocationReorderPropertiesNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceRayTracingMaintenance1FeaturesKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceRayTracingMotionBlurFeaturesNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceRayTracingPipelineFeaturesKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceRayTracingPipelinePropertiesKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceRayTracingPositionFetchFeaturesKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceRayTracingPropertiesNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceRepresentativeFragmentTestFeaturesNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceRGBA10X6FormatsFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceRobustness2FeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceRobustness2PropertiesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceSampleLocationsPropertiesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceSamplerFilterMinmaxProperties)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceSamplerYcbcrConversionFeatures)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceScalarBlockLayoutFeatures)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceSeparateDepthStencilLayoutsFeatures)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceShaderAtomicFloat2FeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceShaderAtomicFloatFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceShaderAtomicInt64Features)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceShaderClockFeaturesKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceShaderCoreBuiltinsFeaturesARM)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceShaderCoreBuiltinsPropertiesARM)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceShaderCoreProperties2AMD)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceShaderCorePropertiesAMD)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceShaderCorePropertiesARM)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceShaderDemoteToHelperInvocationFeatures)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceShaderDrawParametersFeatures)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceShaderEarlyAndLateFragmentTestsFeaturesAMD)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceShaderFloat16Int8Features)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceShaderImageAtomicInt64FeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceShaderImageFootprintFeaturesNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceShaderIntegerDotProductFeatures)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceShaderIntegerDotProductProperties)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceShaderIntegerFunctions2FeaturesINTEL)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceShaderModuleIdentifierFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceShaderModuleIdentifierPropertiesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceShaderObjectFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceShaderObjectPropertiesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceShaderSMBuiltinsFeaturesNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceShaderSMBuiltinsPropertiesNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceShaderSubgroupExtendedTypesFeatures)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceShaderSubgroupUniformControlFlowFeaturesKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceShaderTerminateInvocationFeatures)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceShaderTileImageFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceShaderTileImagePropertiesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceShadingRateImageFeaturesNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceShadingRateImagePropertiesNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceSparseImageFormatInfo2)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceSubgroupProperties)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceSubgroupSizeControlFeatures)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceSubgroupSizeControlProperties)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceSubpassMergeFeedbackFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceSubpassShadingFeaturesHUAWEI)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceSubpassShadingPropertiesHUAWEI)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceSurfaceInfo2KHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceSynchronization2Features)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceTexelBufferAlignmentFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceTexelBufferAlignmentProperties)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceTextureCompressionASTCHDRFeatures)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceTilePropertiesFeaturesQCOM)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceTimelineSemaphoreFeatures)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceTimelineSemaphoreProperties)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceToolProperties)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceTransformFeedbackFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceTransformFeedbackPropertiesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceUniformBufferStandardLayoutFeatures)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceVariablePointersFeatures)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceVertexAttributeDivisorFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceVertexAttributeDivisorPropertiesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceVertexInputDynamicStateFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceVideoFormatInfoKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceVulkan11Features)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceVulkan11Properties)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceVulkan12Features)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceVulkan12Properties)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceVulkan13Features)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceVulkan13Properties)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceVulkanMemoryModelFeatures)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceWorkgroupMemoryExplicitLayoutFeaturesKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceYcbcr2Plane444FormatsFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceYcbcrImageArraysFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPhysicalDeviceZeroInitializeWorkgroupMemoryFeatures)
VULKAN_NONDEFAULT_CUSTOM_REPACK(VkPipelineCacheCreateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPipelineColorBlendAdvancedStateCreateInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPipelineColorBlendStateCreateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPipelineColorWriteCreateInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPipelineCompilerControlCreateInfoAMD)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPipelineCoverageModulationStateCreateInfoNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPipelineCoverageReductionStateCreateInfoNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPipelineCoverageToColorStateCreateInfoNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPipelineCreateFlags2CreateInfoKHR)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkPipelineCreationFeedbackCreateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPipelineDepthStencilStateCreateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPipelineDiscardRectangleStateCreateInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPipelineDynamicStateCreateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPipelineExecutableInfoKHR)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkPipelineExecutableInternalRepresentationKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPipelineExecutablePropertiesKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPipelineExecutableStatisticKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPipelineFragmentShadingRateEnumStateCreateInfoNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPipelineFragmentShadingRateStateCreateInfoKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPipelineIndirectDeviceAddressInfoNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPipelineInfoKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPipelineInputAssemblyStateCreateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPipelineLayoutCreateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPipelineLibraryCreateInfoKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPipelineMultisampleStateCreateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPipelinePropertiesIdentifierEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPipelineRasterizationConservativeStateCreateInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPipelineRasterizationDepthClipStateCreateInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPipelineRasterizationLineStateCreateInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPipelineRasterizationProvokingVertexStateCreateInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPipelineRasterizationStateCreateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPipelineRasterizationStateRasterizationOrderAMD)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPipelineRasterizationStateStreamCreateInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPipelineRenderingCreateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPipelineRepresentativeFragmentTestStateCreateInfoNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPipelineRobustnessCreateInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPipelineSampleLocationsStateCreateInfoEXT)
VULKAN_NONDEFAULT_CUSTOM_REPACK(VkPipelineShaderStageCreateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPipelineShaderStageModuleIdentifierCreateInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPipelineShaderStageRequiredSubgroupSizeCreateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPipelineTessellationDomainOriginStateCreateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPipelineTessellationStateCreateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPipelineVertexInputDivisorStateCreateInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPipelineVertexInputStateCreateInfo)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkPipelineViewportCoarseSampleOrderStateCreateInfoNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPipelineViewportDepthClipControlCreateInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPipelineViewportExclusiveScissorStateCreateInfoNV)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkPipelineViewportShadingRateImageStateCreateInfoNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPipelineViewportStateCreateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPipelineViewportSwizzleStateCreateInfoNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPipelineViewportWScalingStateCreateInfoNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPresentIdKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPresentInfoKHR)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkPresentRegionsKHR)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkPresentTimesInfoGOOGLE)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPrivateDataSlotCreateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkProtectedSubmitInfo)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkQueryLowLatencySupportNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkQueryPoolCreateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkQueryPoolPerformanceCreateInfoKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkQueryPoolPerformanceQueryCreateInfoINTEL)
VULKAN_DEFAULT_CUSTOM_REPACK(VkQueueFamilyCheckpointProperties2NV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkQueueFamilyCheckpointPropertiesNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkQueueFamilyGlobalPriorityPropertiesKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkQueueFamilyProperties2)
VULKAN_DEFAULT_CUSTOM_REPACK(VkQueueFamilyQueryResultStatusPropertiesKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkQueueFamilyVideoPropertiesKHR)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkRayTracingPipelineCreateInfoKHR)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkRayTracingPipelineCreateInfoNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkRayTracingPipelineInterfaceCreateInfoKHR)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkRayTracingShaderGroupCreateInfoKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkRayTracingShaderGroupCreateInfoNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkReleaseSwapchainImagesInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkRenderingAreaInfoKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkRenderingAttachmentInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkRenderingFragmentDensityMapAttachmentInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkRenderingFragmentShadingRateAttachmentInfoKHR)
VULKAN_NONDEFAULT_CUSTOM_REPACK(VkRenderingInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkRenderPassAttachmentBeginInfo)
VULKAN_NONDEFAULT_CUSTOM_REPACK(VkRenderPassBeginInfo)
VULKAN_NONDEFAULT_CUSTOM_REPACK(VkRenderPassCreateInfo)
VULKAN_NONDEFAULT_CUSTOM_REPACK(VkRenderPassCreateInfo2)
VULKAN_DEFAULT_CUSTOM_REPACK(VkRenderPassCreationControlEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkRenderPassCreationFeedbackCreateInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkRenderPassFragmentDensityMapCreateInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkRenderPassInputAttachmentAspectCreateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkRenderPassMultiviewCreateInfo)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkRenderPassSampleLocationsBeginInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkRenderPassSubpassFeedbackCreateInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkRenderPassTransformBeginInfoQCOM)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkResolveImageInfo2)
VULKAN_DEFAULT_CUSTOM_REPACK(VkSampleLocationsInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkSamplerBorderColorComponentMappingCreateInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkSamplerCaptureDescriptorDataInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkSamplerCreateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkSamplerCustomBorderColorCreateInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkSamplerReductionModeCreateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkSamplerYcbcrConversionCreateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkSamplerYcbcrConversionImageFormatProperties)
VULKAN_DEFAULT_CUSTOM_REPACK(VkSamplerYcbcrConversionInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkSemaphoreCreateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkSemaphoreGetFdInfoKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkSemaphoreSignalInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkSemaphoreSubmitInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkSemaphoreTypeCreateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkSemaphoreWaitInfo)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkShaderCreateInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkShaderModuleCreateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkShaderModuleIdentifierEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkShaderModuleValidationCacheCreateInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkSharedPresentSurfaceCapabilitiesKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkSparseImageFormatProperties2)
VULKAN_DEFAULT_CUSTOM_REPACK(VkSparseImageMemoryRequirements2)
VULKAN_NONDEFAULT_CUSTOM_REPACK(VkSubmitInfo)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkSubmitInfo2)
VULKAN_DEFAULT_CUSTOM_REPACK(VkSubpassBeginInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkSubpassDependency2)
VULKAN_NONDEFAULT_CUSTOM_REPACK(VkSubpassDescription2)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkSubpassDescriptionDepthStencilResolve)
VULKAN_DEFAULT_CUSTOM_REPACK(VkSubpassEndInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkSubpassFragmentDensityMapOffsetEndInfoQCOM)
VULKAN_DEFAULT_CUSTOM_REPACK(VkSubpassResolvePerformanceQueryEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkSubpassShadingPipelineCreateInfoHUAWEI)
VULKAN_DEFAULT_CUSTOM_REPACK(VkSubresourceHostMemcpySizeEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkSubresourceLayout2KHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkSurfaceCapabilities2EXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkSurfaceCapabilities2KHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkSurfaceCapabilitiesPresentBarrierNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkSurfaceFormat2KHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkSurfacePresentModeCompatibilityEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkSurfacePresentModeEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkSurfacePresentScalingCapabilitiesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkSurfaceProtectedCapabilitiesKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkSwapchainCounterCreateInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkSwapchainCreateInfoKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkSwapchainDisplayNativeHdrCreateInfoAMD)
VULKAN_DEFAULT_CUSTOM_REPACK(VkSwapchainPresentBarrierCreateInfoNV)
VULKAN_DEFAULT_CUSTOM_REPACK(VkSwapchainPresentFenceInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkSwapchainPresentModeInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkSwapchainPresentModesCreateInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkSwapchainPresentScalingCreateInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkTextureLODGatherFormatPropertiesAMD)
VULKAN_DEFAULT_CUSTOM_REPACK(VkTilePropertiesQCOM)
VULKAN_DEFAULT_CUSTOM_REPACK(VkTimelineSemaphoreSubmitInfo)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkValidationCacheCreateInfoEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkValidationFeaturesEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkValidationFlagsEXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkVertexInputAttributeDescription2EXT)
VULKAN_DEFAULT_CUSTOM_REPACK(VkVertexInputBindingDescription2EXT)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkVideoBeginCodingInfoKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkVideoCapabilitiesKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkVideoCodingControlInfoKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkVideoDecodeCapabilitiesKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkVideoDecodeH264CapabilitiesKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkVideoDecodeH264DpbSlotInfoKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkVideoDecodeH264PictureInfoKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkVideoDecodeH264ProfileInfoKHR)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkVideoDecodeH264SessionParametersAddInfoKHR)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkVideoDecodeH264SessionParametersCreateInfoKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkVideoDecodeH265CapabilitiesKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkVideoDecodeH265DpbSlotInfoKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkVideoDecodeH265PictureInfoKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkVideoDecodeH265ProfileInfoKHR)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkVideoDecodeH265SessionParametersAddInfoKHR)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkVideoDecodeH265SessionParametersCreateInfoKHR)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkVideoDecodeInfoKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkVideoDecodeUsageInfoKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkVideoEndCodingInfoKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkVideoFormatPropertiesKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkVideoPictureResourceInfoKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkVideoProfileInfoKHR)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkVideoProfileListInfoKHR)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkVideoReferenceSlotInfoKHR)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkVideoSessionCreateInfoKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkVideoSessionMemoryRequirementsKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkVideoSessionParametersCreateInfoKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkVideoSessionParametersUpdateInfoKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkWaylandSurfaceCreateInfoKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkWriteDescriptorSet) // TODO: This should be non-default instead
VULKAN_DEFAULT_CUSTOM_REPACK(VkWriteDescriptorSetAccelerationStructureKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkWriteDescriptorSetAccelerationStructureNV)
// VULKAN_DEFAULT_CUSTOM_REPACK(VkWriteDescriptorSetInlineUniformBlock)
VULKAN_NONDEFAULT_CUSTOM_REPACK(VkXcbSurfaceCreateInfoKHR)
VULKAN_NONDEFAULT_CUSTOM_REPACK(VkXlibSurfaceCreateInfoKHR)


void fex_custom_repack_entry(host_layout<VkInstanceCreateInfo>& into, const guest_layout<VkInstanceCreateInfo>& from) {
  default_fex_custom_repack_entry(into, from);

  auto HostApplicationInfo = new host_layout<VkApplicationInfo> {*from.data.pApplicationInfo.get_pointer()};
  fex_apply_custom_repacking_entry(*HostApplicationInfo, *from.data.pApplicationInfo.get_pointer());

  into.data.pApplicationInfo = &HostApplicationInfo->data;

  auto extension_count = from.data.enabledExtensionCount.data;
  into.data.ppEnabledExtensionNames = RepackStructArray<false>(extension_count, from.data.ppEnabledExtensionNames).data();

  auto layer_count = from.data.enabledLayerCount.data;
  into.data.ppEnabledLayerNames = RepackStructArray<false>(layer_count, from.data.ppEnabledLayerNames).data();
}

bool fex_custom_repack_exit(guest_layout<VkInstanceCreateInfo>& into, const host_layout<VkInstanceCreateInfo>& from) {
  delete from.data.pApplicationInfo;
  delete[] from.data.ppEnabledExtensionNames;
  delete[] from.data.ppEnabledLayerNames;
  return false;
}

void fex_custom_repack_entry(host_layout<VkDeviceCreateInfo>& into, const guest_layout<VkDeviceCreateInfo>& from) {
  default_fex_custom_repack_entry(into, from);

  auto HostQueueCreateInfo = new host_layout<VkDeviceQueueCreateInfo> {*from.data.pQueueCreateInfos.get_pointer()};
  fex_apply_custom_repacking_entry(*HostQueueCreateInfo, *from.data.pQueueCreateInfos.get_pointer());
  into.data.pQueueCreateInfos = &HostQueueCreateInfo->data;

  auto layer_count = from.data.enabledExtensionCount.data;
  fprintf(stderr, "  Repacking %d ppEnabledLayerNames\n", layer_count);
  into.data.ppEnabledLayerNames = RepackStructArray<false>(layer_count, from.data.ppEnabledLayerNames).data();

  auto extension_count = from.data.enabledExtensionCount.data;
  fprintf(stderr, "  Repacking %d ppEnabledExtensionNames\n", extension_count);
  into.data.ppEnabledExtensionNames = RepackStructArray<false>(extension_count, from.data.ppEnabledExtensionNames).data();
}

bool fex_custom_repack_exit(guest_layout<VkDeviceCreateInfo>& into, const host_layout<VkDeviceCreateInfo>& from) {
  delete from.data.pQueueCreateInfos;
  delete[] from.data.ppEnabledExtensionNames;
  delete[] from.data.ppEnabledLayerNames;
  return false;
}

void fex_custom_repack_entry(host_layout<VkDescriptorSetLayoutCreateInfo>& into, const guest_layout<VkDescriptorSetLayoutCreateInfo>& from) {
  default_fex_custom_repack_entry(into, from);
  into.data.pBindings = RepackStructArray(from.data.bindingCount.data, from.data.pBindings).data();
}

bool fex_custom_repack_exit(guest_layout<VkDescriptorSetLayoutCreateInfo>& into, const host_layout<VkDescriptorSetLayoutCreateInfo>& from) {
  delete[] from.data.pBindings;
  return false;
}

void fex_custom_repack_entry(host_layout<VkRenderPassCreateInfo>& into, const guest_layout<VkRenderPassCreateInfo>& from) {
  default_fex_custom_repack_entry(into, from);
  into.data.pSubpasses = RepackStructArray(from.data.subpassCount.data, from.data.pSubpasses).data();
}

bool fex_custom_repack_exit(guest_layout<VkRenderPassCreateInfo>& into, const host_layout<VkRenderPassCreateInfo>& from) {
  delete[] from.data.pSubpasses;
  return false;
}

void fex_custom_repack_entry(host_layout<VkRenderPassCreateInfo2>& into, const guest_layout<VkRenderPassCreateInfo2>& from) {
  default_fex_custom_repack_entry(into, from);
  into.data.pAttachments = RepackStructArray(from.data.attachmentCount.data, from.data.pAttachments).data();
  into.data.pSubpasses = RepackStructArray(from.data.subpassCount.data, from.data.pSubpasses).data();
  into.data.pDependencies = RepackStructArray(from.data.dependencyCount.data, from.data.pDependencies).data();
}

bool fex_custom_repack_exit(guest_layout<VkRenderPassCreateInfo2>& into, const host_layout<VkRenderPassCreateInfo2>& from) {
  DeleteRepackedStructArray(from.data.attachmentCount, from.data.pAttachments, into.data.pAttachments);
  DeleteRepackedStructArray(from.data.subpassCount, from.data.pSubpasses, into.data.pSubpasses);
  DeleteRepackedStructArray(from.data.dependencyCount, from.data.pDependencies, into.data.pDependencies);
  return false;
}

void fex_custom_repack_entry(host_layout<VkSubpassDescription2>& into, const guest_layout<VkSubpassDescription2>& from) {
  default_fex_custom_repack_entry(into, from);
  into.data.pInputAttachments = RepackStructArray(from.data.inputAttachmentCount.data, from.data.pInputAttachments).data();
  into.data.pColorAttachments = RepackStructArray(from.data.colorAttachmentCount.data, from.data.pColorAttachments).data();
  into.data.pResolveAttachments = RepackStructArray(from.data.colorAttachmentCount.data, from.data.pResolveAttachments).data();

  if (from.data.pDepthStencilAttachment.data == 0) {
    into.data.pDepthStencilAttachment = nullptr;
  } else {
    into.data.pDepthStencilAttachment = new VkAttachmentReference2;
    auto in_data = host_layout<VkAttachmentReference2> {*from.data.pDepthStencilAttachment.get_pointer()};
    fex_apply_custom_repacking_entry(in_data, *from.data.pDepthStencilAttachment.get_pointer());
    memcpy((void*)into.data.pDepthStencilAttachment, &in_data.data, sizeof(VkAttachmentReference2));
  }
}

bool fex_custom_repack_exit(guest_layout<VkSubpassDescription2>& into, const host_layout<VkSubpassDescription2>& from) {
  DeleteRepackedStructArray(from.data.inputAttachmentCount, from.data.pInputAttachments, into.data.pInputAttachments);
  DeleteRepackedStructArray(from.data.colorAttachmentCount, from.data.pColorAttachments, into.data.pColorAttachments);
  DeleteRepackedStructArray(from.data.colorAttachmentCount, from.data.pResolveAttachments, into.data.pResolveAttachments);
  if (from.data.pDepthStencilAttachment) {
    fex_apply_custom_repacking_exit(*into.data.pDepthStencilAttachment.get_pointer(), to_host_layout(*from.data.pDepthStencilAttachment));
    delete from.data.pDepthStencilAttachment;
  }
  return false;
}

void fex_custom_repack_entry(host_layout<VkRenderingInfo>& into, const guest_layout<VkRenderingInfo>& from) {
  default_fex_custom_repack_entry(into, from);

  into.data.pColorAttachments = RepackStructArray(from.data.colorAttachmentCount.data, from.data.pColorAttachments).data();

  if (from.data.pDepthAttachment.get_pointer() == nullptr) {
    into.data.pDepthAttachment = nullptr;
  } else {
    into.data.pDepthAttachment = new VkRenderingAttachmentInfo;
    auto in_data = host_layout<VkRenderingAttachmentInfo> {*from.data.pDepthAttachment.get_pointer()};
    fex_apply_custom_repacking_entry(in_data, *from.data.pDepthAttachment.get_pointer());
    memcpy((void*)into.data.pDepthAttachment, &in_data.data, sizeof(VkRenderingAttachmentInfo));
  }

  if (from.data.pStencilAttachment.get_pointer() == nullptr) {
    into.data.pStencilAttachment = nullptr;
  } else {
    into.data.pStencilAttachment = new VkRenderingAttachmentInfo;
    auto in_data = host_layout<VkRenderingAttachmentInfo> {*from.data.pStencilAttachment.get_pointer()};
    fex_apply_custom_repacking_entry(in_data, *from.data.pStencilAttachment.get_pointer());
    memcpy((void*)into.data.pStencilAttachment, &in_data.data, sizeof(VkRenderingAttachmentInfo));
  }
}

bool fex_custom_repack_exit(guest_layout<VkRenderingInfo>& into, const host_layout<VkRenderingInfo>& from) {
  DeleteRepackedStructArray(from.data.colorAttachmentCount, from.data.pColorAttachments, into.data.pColorAttachments);
  if (from.data.pDepthAttachment) {
    fex_apply_custom_repacking_exit(*into.data.pDepthAttachment.get_pointer(), to_host_layout(*from.data.pDepthAttachment));
    delete from.data.pDepthAttachment;
  }
  if (from.data.pStencilAttachment) {
    fex_apply_custom_repacking_exit(*into.data.pStencilAttachment.get_pointer(), to_host_layout(*from.data.pStencilAttachment));
    delete from.data.pStencilAttachment;
  }
  return false;
}

void fex_custom_repack_entry(host_layout<VkDescriptorGetInfoEXT>& into, const guest_layout<VkDescriptorGetInfoEXT>& from) {
  default_fex_custom_repack_entry(into, from);

  switch (into.data.type) {
  case VK_DESCRIPTOR_TYPE_SAMPLER:
  case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
  case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
  case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
  case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: {
    // VkSampler* or VkDescriptorImageInfo*. Handle by zero-extending
    guest_layout<VkSampler*> guest_data;
    memcpy(&guest_data, from.data.data.union_storage, sizeof(guest_data));
    into.data.data.pSampler = host_layout<VkSampler*> {guest_data}.data;
    break;
  }

  case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
  case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
  case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
  case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER: {
    // VkDescriptorAddressInfoEXT*. Repacking required
    guest_layout<VkDescriptorAddressInfoEXT*> guest_ptr;
    memcpy(&guest_ptr, from.data.data.union_storage, sizeof(guest_ptr));
    auto child_mem = (char*)aligned_alloc(alignof(host_layout<VkDescriptorAddressInfoEXT>), sizeof(host_layout<VkDescriptorAddressInfoEXT>));
    auto child = new (child_mem) host_layout<VkDescriptorAddressInfoEXT> {*guest_ptr.get_pointer()};

    default_fex_custom_repack_entry(*child, *guest_ptr.get_pointer());
    into.data.data.pUniformBuffer = &child->data;
    break;
  }

  case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
  case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV: {
    // Copy unmodified
    static_assert(sizeof(guest_layout<VkDeviceAddress>) == sizeof(uint64_t));
    memcpy(&into.data.data.accelerationStructure, &from.data.data, sizeof(uint64_t));
  }

  case VK_DESCRIPTOR_TYPE_SAMPLE_WEIGHT_IMAGE_QCOM:
  case VK_DESCRIPTOR_TYPE_BLOCK_MATCH_IMAGE_QCOM:
  case VK_DESCRIPTOR_TYPE_MUTABLE_EXT:
  case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
  case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
  case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK:
  default: fprintf(stderr, "ERROR: Invalid descriptor type used in VkDescriptorGetInfoEXT"); std::abort();
  }
}

bool fex_custom_repack_exit(guest_layout<VkDescriptorGetInfoEXT>& into, const host_layout<VkDescriptorGetInfoEXT>& from) {
  switch (from.data.type) {
  case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
  case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
  case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
  case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
    // Delete storage allocated on entry
    free((void*)from.data.data.pUniformBuffer);

  default:
    // Nothing to do for the rest
    break;
  }
  return false;
}

void fex_custom_repack_entry(host_layout<VkDependencyInfo>& into, const guest_layout<VkDependencyInfo>& from) {
  default_fex_custom_repack_entry(into, from);
  into.data.pMemoryBarriers = RepackStructArray(from.data.memoryBarrierCount.data, from.data.pMemoryBarriers).data();
  into.data.pImageMemoryBarriers = RepackStructArray(from.data.imageMemoryBarrierCount.data, from.data.pImageMemoryBarriers).data();
  into.data.pBufferMemoryBarriers = RepackStructArray(from.data.bufferMemoryBarrierCount.data, from.data.pBufferMemoryBarriers).data();
}

bool fex_custom_repack_exit(guest_layout<VkDependencyInfo>& into, const host_layout<VkDependencyInfo>& from) {
  DeleteRepackedStructArray(from.data.memoryBarrierCount, from.data.pMemoryBarriers, into.data.pMemoryBarriers);
  DeleteRepackedStructArray(from.data.imageMemoryBarrierCount, from.data.pImageMemoryBarriers, into.data.pImageMemoryBarriers);
  DeleteRepackedStructArray(from.data.bufferMemoryBarrierCount, from.data.pBufferMemoryBarriers, into.data.pBufferMemoryBarriers);
  return false;
}

void fex_custom_repack_entry(host_layout<VkDescriptorUpdateTemplateCreateInfo>& into,
                             const guest_layout<VkDescriptorUpdateTemplateCreateInfo>& from) {
  default_fex_custom_repack_entry(into, from);
  into.data.pDescriptorUpdateEntries = RepackStructArray(from.data.descriptorUpdateEntryCount.data, from.data.pDescriptorUpdateEntries).data();
}

bool fex_custom_repack_exit(guest_layout<VkDescriptorUpdateTemplateCreateInfo>& into, const host_layout<VkDescriptorUpdateTemplateCreateInfo>& from) {
  DeleteRepackedStructArray(from.data.descriptorUpdateEntryCount, from.data.pDescriptorUpdateEntries, into.data.pDescriptorUpdateEntries);
  return false;
}

void fex_custom_repack_entry(host_layout<VkPipelineShaderStageCreateInfo>& into, const guest_layout<VkPipelineShaderStageCreateInfo>& from) {
  default_fex_custom_repack_entry(into, from);
  if (from.data.pSpecializationInfo.get_pointer()) {
    fprintf(stderr, "ERROR: Cannot repack non-null VkPipelineShaderStageCreateInfo::pSpecializationInfo yet");
    std::abort();
  }
}

bool fex_custom_repack_exit(guest_layout<VkPipelineShaderStageCreateInfo>& into, const host_layout<VkPipelineShaderStageCreateInfo>& from) {
  // TODO
  return false;
}

void fex_custom_repack_entry(host_layout<VkGraphicsPipelineCreateInfo>& into, const guest_layout<VkGraphicsPipelineCreateInfo>& from) {
  default_fex_custom_repack_entry(into, from);
  into.data.pStages = RepackStructArray(from.data.stageCount.data, from.data.pStages).data();

  if (!from.data.pVertexInputState.get_pointer()) {
    into.data.pVertexInputState = nullptr;
  } else {
    into.data.pVertexInputState = &(new host_layout<VkPipelineVertexInputStateCreateInfo> {*from.data.pVertexInputState.get_pointer()})->data;
  }

  if (!from.data.pInputAssemblyState.get_pointer()) {
    into.data.pInputAssemblyState = nullptr;
  } else {
    into.data.pInputAssemblyState =
      &(new host_layout<VkPipelineInputAssemblyStateCreateInfo> {*from.data.pInputAssemblyState.get_pointer()})->data;
  }

  if (!from.data.pTessellationState.get_pointer()) {
    into.data.pTessellationState = nullptr;
  } else {
    into.data.pTessellationState = &(new host_layout<VkPipelineTessellationStateCreateInfo> {*from.data.pTessellationState.get_pointer()})->data;
  }

  if (!from.data.pViewportState.get_pointer()) {
    into.data.pViewportState = nullptr;
  } else {
    into.data.pViewportState = &(new host_layout<VkPipelineViewportStateCreateInfo> {*from.data.pViewportState.get_pointer()})->data;
  }

  if (!from.data.pRasterizationState.get_pointer()) {
    into.data.pRasterizationState = nullptr;
  } else {
    into.data.pRasterizationState =
      &(new host_layout<VkPipelineRasterizationStateCreateInfo> {*from.data.pRasterizationState.get_pointer()})->data;
  }

  if (!from.data.pMultisampleState.get_pointer()) {
    into.data.pMultisampleState = nullptr;
  } else {
    into.data.pMultisampleState = &(new host_layout<VkPipelineMultisampleStateCreateInfo> {*from.data.pMultisampleState.get_pointer()})->data;
  }

  if (!from.data.pDepthStencilState.get_pointer()) {
    into.data.pDepthStencilState = nullptr;
  } else {
    into.data.pDepthStencilState = &(new host_layout<VkPipelineDepthStencilStateCreateInfo> {*from.data.pDepthStencilState.get_pointer()})->data;
  }

  if (!from.data.pColorBlendState.get_pointer()) {
    into.data.pColorBlendState = nullptr;
  } else {
    into.data.pColorBlendState = &(new host_layout<VkPipelineColorBlendStateCreateInfo> {*from.data.pColorBlendState.get_pointer()})->data;
  }

  if (!from.data.pDynamicState.get_pointer()) {
    into.data.pDynamicState = nullptr;
  } else {
    into.data.pDynamicState = &(new host_layout<VkPipelineDynamicStateCreateInfo> {*from.data.pDynamicState.get_pointer()})->data;
  }
}

bool fex_custom_repack_exit(guest_layout<VkGraphicsPipelineCreateInfo>& into, const host_layout<VkGraphicsPipelineCreateInfo>& from) {
  delete[] from.data.pStages;
  delete from.data.pVertexInputState;
  delete from.data.pInputAssemblyState;
  delete from.data.pTessellationState;
  delete from.data.pViewportState;
  delete from.data.pRasterizationState;
  delete from.data.pMultisampleState;
  delete from.data.pDepthStencilState;
  delete from.data.pColorBlendState;
  delete from.data.pDynamicState;
  return false;
}

void fex_custom_repack_entry(host_layout<VkSubmitInfo>& into, const guest_layout<VkSubmitInfo>& from) {
  default_fex_custom_repack_entry(into, from);
  into.data.pCommandBuffers = RepackStructArray<false>(from.data.commandBufferCount.data, from.data.pCommandBuffers).data();
}

bool fex_custom_repack_exit(guest_layout<VkSubmitInfo>& into, const host_layout<VkSubmitInfo>& from) {
  delete[] from.data.pCommandBuffers;
  return false;
}

void fex_custom_repack_entry(host_layout<VkCommandBufferBeginInfo>& into, const guest_layout<VkCommandBufferBeginInfo>& from) {
  default_fex_custom_repack_entry(into, from);

  if (!from.data.pInheritanceInfo.get_pointer() || !from.data.pInheritanceInfo.data) {
    into.data.pInheritanceInfo = nullptr;
    return;
  }
  into.data.pInheritanceInfo = new VkCommandBufferInheritanceInfo;
  auto src = host_layout<VkCommandBufferInheritanceInfo> {*from.data.pInheritanceInfo.get_pointer()}.data;
  static_assert(sizeof(src) == sizeof(*into.data.pInheritanceInfo));
  memcpy((void*)into.data.pInheritanceInfo, &src, sizeof(src));
}

bool fex_custom_repack_exit(guest_layout<VkCommandBufferBeginInfo>& into, const host_layout<VkCommandBufferBeginInfo>& from) {
  delete from.data.pInheritanceInfo;
  return false;
}

void fex_custom_repack_entry(host_layout<VkPipelineCacheCreateInfo>& into, const guest_layout<VkPipelineCacheCreateInfo>& from) {
  default_fex_custom_repack_entry(into, from);

  // Same underlying layout, so there's nothing to do
  into.data.pInitialData = from.data.pInitialData.get_pointer();
}

bool fex_custom_repack_exit(guest_layout<VkPipelineCacheCreateInfo>& into, const host_layout<VkPipelineCacheCreateInfo>& from) {
  // Nothing to do
  return false;
}

void fex_custom_repack_entry(host_layout<VkRenderPassBeginInfo>& into, const guest_layout<VkRenderPassBeginInfo>& from) {
  default_fex_custom_repack_entry(into, from);

  // Same underlying layout, so there's nothing to do
  into.data.pClearValues = reinterpret_cast<const VkClearValue*>(from.data.pClearValues.get_pointer());
}

bool fex_custom_repack_exit(guest_layout<VkRenderPassBeginInfo>& into, const host_layout<VkRenderPassBeginInfo>& from) {
  // Nothing to do
  return false;
}
#endif

EXPORTS(libvulkan)
