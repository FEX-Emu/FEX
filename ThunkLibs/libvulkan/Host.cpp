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
#include <span>
#include <string_view>
#include <unordered_map>
#include <vector>

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

// Allocates storage on the heap that must be de-allocated using delete[]
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
    // TODO: Review if 32-bit changes are needed
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
  converters<VkStructureType::VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO, VkSemaphoreTypeCreateInfo>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS, VkMemoryDedicatedRequirements>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO, VkMemoryDedicatedAllocateInfo>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO, VkImageFormatListCreateInfo>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_RENDER_PASS_ATTACHMENT_BEGIN_INFO, VkRenderPassAttachmentBeginInfo>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO, VkTimelineSemaphoreSubmitInfo>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO, VkPipelineRenderingCreateInfo>,
  converters<VkStructureType::VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO, VkMemoryAllocateFlagsInfo>,
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

VULKAN_DEFAULT_CUSTOM_REPACK(VkApplicationInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkAttachmentDescription2)
VULKAN_DEFAULT_CUSTOM_REPACK(VkAttachmentReference2)
VULKAN_DEFAULT_CUSTOM_REPACK(VkBufferMemoryBarrier2)
VULKAN_NONDEFAULT_CUSTOM_REPACK(VkDependencyInfo)
VULKAN_NONDEFAULT_CUSTOM_REPACK(VkDescriptorUpdateTemplateCreateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkDeviceQueueCreateInfo)
VULKAN_NONDEFAULT_CUSTOM_REPACK(VkGraphicsPipelineCreateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkImageFormatListCreateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkImageMemoryBarrier2)
VULKAN_DEFAULT_CUSTOM_REPACK(VkImageMemoryRequirementsInfo2)
VULKAN_DEFAULT_CUSTOM_REPACK(VkImageViewCreateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkMemoryAllocateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkMemoryAllocateFlagsInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkMemoryBarrier2)
VULKAN_DEFAULT_CUSTOM_REPACK(VkMemoryDedicatedAllocateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkMemoryDedicatedRequirements)
VULKAN_DEFAULT_CUSTOM_REPACK(VkMemoryRequirements2)
VULKAN_DEFAULT_CUSTOM_REPACK(VkPipelineRenderingCreateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkRenderPassAttachmentBeginInfo)
VULKAN_NONDEFAULT_CUSTOM_REPACK(VkRenderPassBeginInfo)
VULKAN_NONDEFAULT_CUSTOM_REPACK(VkRenderPassCreateInfo)
VULKAN_NONDEFAULT_CUSTOM_REPACK(VkRenderPassCreateInfo2)
VULKAN_DEFAULT_CUSTOM_REPACK(VkSemaphoreCreateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkSemaphoreTypeCreateInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkSemaphoreWaitInfo)
VULKAN_NONDEFAULT_CUSTOM_REPACK(VkSubmitInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkSubpassBeginInfo)
VULKAN_DEFAULT_CUSTOM_REPACK(VkSubpassDependency2)
VULKAN_NONDEFAULT_CUSTOM_REPACK(VkSubpassDescription2)
VULKAN_DEFAULT_CUSTOM_REPACK(VkSwapchainCreateInfoKHR)
VULKAN_DEFAULT_CUSTOM_REPACK(VkTimelineSemaphoreSubmitInfo)


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
  delete[] from.data.pAttachments;
  // TODO: Run custom exit-repacking
  delete[] from.data.pSubpasses;
  // TODO: Run custom exit-repacking
  delete[] from.data.pDependencies;
  // TODO: Run custom exit-repacking
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
  delete[] from.data.pInputAttachments;
  // TODO: Run custom exit-repacking
  delete[] from.data.pColorAttachments;
  // TODO: Run custom exit-repacking
  delete[] from.data.pResolveAttachments;
  // TODO: Run custom exit-repacking
  delete /*[]*/ from.data.pDepthStencilAttachment;
  // TODO: Run custom exit-repacking
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
  delete[] from.data.pColorAttachments;
  // TODO: Run custom exit-repacking
  delete from.data.pDepthAttachment;
  // TODO: Run custom exit-repacking
  delete from.data.pStencilAttachment;
  // TODO: Run custom exit-repacking
  return false;
}

void fex_custom_repack_entry(host_layout<VkDependencyInfo>& into, const guest_layout<VkDependencyInfo>& from) {
  default_fex_custom_repack_entry(into, from);
  into.data.pMemoryBarriers = RepackStructArray(from.data.memoryBarrierCount.data, from.data.pMemoryBarriers).data();
  into.data.pImageMemoryBarriers = RepackStructArray(from.data.imageMemoryBarrierCount.data, from.data.pImageMemoryBarriers).data();
  into.data.pBufferMemoryBarriers = RepackStructArray(from.data.bufferMemoryBarrierCount.data, from.data.pBufferMemoryBarriers).data();
}

bool fex_custom_repack_exit(guest_layout<VkDependencyInfo>& into, const host_layout<VkDependencyInfo>& from) {
  delete[] from.data.pMemoryBarriers;
  // TODO: Run custom exit-repacking
  delete[] from.data.pBufferMemoryBarriers;
  // TODO: Run custom exit-repacking
  delete[] from.data.pImageMemoryBarriers;
  // TODO: Run custom exit-repacking
  return false;
}

void fex_custom_repack_entry(host_layout<VkDescriptorUpdateTemplateCreateInfo>& into,
                             const guest_layout<VkDescriptorUpdateTemplateCreateInfo>& from) {
  default_fex_custom_repack_entry(into, from);
  into.data.pDescriptorUpdateEntries = RepackStructArray(from.data.descriptorUpdateEntryCount.data, from.data.pDescriptorUpdateEntries).data();
}

bool fex_custom_repack_exit(guest_layout<VkDescriptorUpdateTemplateCreateInfo>& into, const host_layout<VkDescriptorUpdateTemplateCreateInfo>& from) {
  delete[] from.data.pDescriptorUpdateEntries;
  // TODO: Run custom exit-repacking
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
