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
#include <vector>

#include <dlfcn.h>

#include "thunkgen_host_libvulkan.inl"

static bool SetupInstance{};
static std::mutex SetupMutex{};

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

#if 0
// Functions with callbacks are overridden to ignore the guest-side callbacks

static VkResult FEXFN_IMPL(vkCreateShaderModule)(VkDevice a_0, const VkShaderModuleCreateInfo* a_1, const VkAllocationCallbacks* a_2, VkShaderModule* a_3) {
  (void*&)LDR_PTR(vkCreateShaderModule) = (void*)LDR_PTR(vkGetDeviceProcAddr)(a_0, "vkCreateShaderModule");
  return LDR_PTR(vkCreateShaderModule)(a_0, a_1, nullptr, a_3);
}

static VkBool32 DummyVkDebugReportCallback(VkDebugReportFlagsEXT, VkDebugReportObjectTypeEXT, uint64_t, size_t,
                                           int32_t, const char*, const char*, void*) {
  return VK_FALSE;
}
#endif

static VkResult FEXFN_IMPL(vkCreateInstance)(const VkInstanceCreateInfo* a_0, const VkAllocationCallbacks* a_1, guest_layout<VkInstance*> a_2) {
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

  VkInstance out;
  auto ret = LDR_PTR(vkCreateInstance)(vk_struct_base, nullptr, &out);
  *a_2.get_pointer() = out;
  return ret;
}

static VkResult FEXFN_IMPL(vkCreateDevice)(VkPhysicalDevice a_0, const VkDeviceCreateInfo* a_1, const VkAllocationCallbacks* a_2, guest_layout<VkDevice*> a_3) {
  VkDevice out;
  auto ret = LDR_PTR(vkCreateDevice)(a_0, a_1, nullptr, &out);
  *a_3.get_pointer() = out;
  return ret;
}

static VkResult FEXFN_IMPL(vkAllocateMemory)(VkDevice a_0, const VkMemoryAllocateInfo* a_1, const VkAllocationCallbacks* a_2, VkDeviceMemory* a_3){
  (void*&)LDR_PTR(vkAllocateMemory) = (void*)LDR_PTR(vkGetDeviceProcAddr)(a_0, "vkAllocateMemory");
  return LDR_PTR(vkAllocateMemory)(a_0, a_1, nullptr, a_3);
}

static void FEXFN_IMPL(vkFreeMemory)(VkDevice a_0, VkDeviceMemory a_1, const VkAllocationCallbacks* a_2) {
  (void*&)LDR_PTR(vkFreeMemory) = (void*)LDR_PTR(vkGetDeviceProcAddr)(a_0, "vkFreeMemory");
  LDR_PTR(vkFreeMemory)(a_0, a_1, nullptr);
}

#if 0
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
#endif

#ifdef IS_32BIT_THUNK
VkResult fexfn_impl_libvulkan_vkEnumeratePhysicalDevices(VkInstance instance, uint32_t* count, guest_layout<VkPhysicalDevice*> devices) {
  if (!devices.get_pointer()) {
    return fexldr_ptr_libvulkan_vkEnumeratePhysicalDevices(instance, count, nullptr);
  }

  auto input_count = *count;
  std::vector<VkPhysicalDevice> out(input_count);
  auto ret = fexldr_ptr_libvulkan_vkEnumeratePhysicalDevices(instance, count, out.data());
  for (size_t i = 0; i < std::min(input_count, *count); ++i) {
    devices.get_pointer()[i] = out[i];
  }
  return ret;
}

void fexfn_impl_libvulkan_vkGetDeviceQueue(VkDevice device, uint32_t family_index, uint32_t queue_index, guest_layout<VkQueue*> queue) {
  VkQueue out;
  fexldr_ptr_libvulkan_vkGetDeviceQueue(device, family_index, queue_index, &out);
  *queue.get_pointer() = out;
}

VkResult fexfn_impl_libvulkan_vkAllocateCommandBuffers(VkDevice device, const VkCommandBufferAllocateInfo* info, guest_layout<VkCommandBuffer*> buffers) {
  std::vector<VkCommandBuffer> out(info->commandBufferCount);
  auto ret = fexldr_ptr_libvulkan_vkAllocateCommandBuffers(device, info, out.data());
  if (ret == VK_SUCCESS) {
    for (size_t i = 0; i < info->commandBufferCount; ++i) {
      buffers.get_pointer()[i] = out[i];
    }
  }
  return ret;
}

VkResult fexfn_impl_libvulkan_vkMapMemory(VkDevice device, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, guest_layout<void**> data) {
  host_layout<void*> host_data {};
  void* mapped;
  auto ret = fexldr_ptr_libvulkan_vkMapMemory(device, memory, offset, size, flags, &mapped);
  fprintf(stderr, "%s: Mapped %p\n", __FUNCTION__, mapped);
  if (ret == VK_SUCCESS) {
    host_data.data = mapped;
    *data.get_pointer() = to_guest(host_data);
  }
  return ret;
}

void fexfn_impl_libvulkan_vkUpdateDescriptorSets(
    VkDevice_T* device, unsigned int descriptorWriteCount,
    guest_layout<const VkWriteDescriptorSet*> pDescriptorWrites,
    unsigned int descriptorCopyCount, VkCopyDescriptorSet const* pDescriptorCopies) {

  VkWriteDescriptorSet* HostDescriptorWrites;
  if (!descriptorWriteCount || !descriptorWriteCount) {
    HostDescriptorWrites = nullptr;
  } else {
    HostDescriptorWrites = new VkWriteDescriptorSet[descriptorWriteCount];
    for (size_t i = 0; i < descriptorWriteCount; ++i) {
      auto src = host_layout<VkWriteDescriptorSet> { pDescriptorWrites.get_pointer()[i] };
      fex_apply_custom_repacking(src, pDescriptorWrites.get_pointer()[i]);
      static_assert(sizeof(src.data) == sizeof(HostDescriptorWrites[i]));
      memcpy((void*)&HostDescriptorWrites[i], &src.data, sizeof(src.data));
    }
  }

// TODO: Is this needed?
//  if (!from.data.pDescriptorCopies.get_pointer() || !from.data.descriptorCopyCount.data) {
//    into.data.pDescriptorCopies = nullptr;
//    return;
//  }
//  into.data.pDescriptorCopies = new VkCopyDescriptorSet[from.data.descriptorCopyCount.data];
//  for (size_t i = 0; i < from.data.descriptorCopyCount.data; ++i) {
//    auto src = host_layout<VkCopyDescriptorSet> { from.data.pDescriptorCopies.get_pointer()[i] }.data;
//    static_assert(sizeof(src) == sizeof(into.data.pDescriptorCopies[i]));
//    memcpy((void*)&into.data.pDescriptorCopies[i], &src, sizeof(src));
//  }
//  delete[] info;


  fexldr_ptr_libvulkan_vkUpdateDescriptorSets(device, descriptorWriteCount, HostDescriptorWrites, descriptorCopyCount, pDescriptorCopies);

  delete[] HostDescriptorWrites;
}

VkResult fexfn_impl_libvulkan_vkQueueSubmit(VkQueue queue, uint32_t submit_count,
                                            guest_layout<const VkSubmitInfo*> submit_infos, VkFence fence) {

  VkSubmitInfo* HostSubmitInfos = nullptr;
  if (submit_count) {
    HostSubmitInfos = new VkSubmitInfo[submit_count];
    for (size_t i = 0; i < submit_count; ++i) {
      auto src = host_layout<VkSubmitInfo> { submit_infos.get_pointer()[i] };
      fex_apply_custom_repacking(src, submit_infos.get_pointer()[i]);
      static_assert(sizeof(src.data) == sizeof(HostSubmitInfos[i]));
      memcpy((void*)&HostSubmitInfos[i], &src.data, sizeof(src.data));
    }
  }

  auto ret = fexldr_ptr_libvulkan_vkQueueSubmit(queue, submit_count, HostSubmitInfos, fence);

  delete[] HostSubmitInfos;

  return ret;
}

void fexfn_impl_libvulkan_vkFreeCommandBuffers(VkDevice device, VkCommandPool pool, uint32_t num_buffers,
                                            guest_layout<const VkCommandBuffer*> buffers) {

  VkCommandBuffer* HostBuffers = nullptr;
  if (num_buffers) {
    HostBuffers = new VkCommandBuffer[num_buffers];
    for (size_t i = 0; i < num_buffers; ++i) {
      auto src = host_layout<VkCommandBuffer_T*> { (const guest_layout<VkCommandBuffer_T *>&)buffers.get_pointer()[i] };
      static_assert(sizeof(src.data) == sizeof(HostBuffers[i]));
      memcpy((void*)&HostBuffers[i], &src.data, sizeof(src.data));
    }
  }

  fexldr_ptr_libvulkan_vkFreeCommandBuffers(device, pool, num_buffers, HostBuffers);

  delete[] HostBuffers;
}

VkResult fexfn_impl_libvulkan_vkGetPipelineCacheData(VkDevice device, VkPipelineCache cache, guest_layout<size_t*> guest_data_size, void* data) {
  size_t data_size = guest_data_size.get_pointer()->data;
  auto ret = fexldr_ptr_libvulkan_vkGetPipelineCacheData(device, cache, &data_size, data);
  *guest_data_size.get_pointer() = data_size;
  return ret;
}

#endif

static PFN_vkVoidFunction FEXFN_IMPL(vkGetDeviceProcAddr)(VkDevice a_0, const char* a_1) {
  // Just return the host facing function pointer
  // The guest will handle mapping if this exists

  // Check for functions with custom implementations first
  /*if (std::strcmp(a_1, "vkCreateShaderModule") == 0) {
      return (PFN_vkVoidFunction)fexfn_impl_libvulkan_vkCreateShaderModule;
  } else */if (std::strcmp(a_1, "vkCreateInstance") == 0) {
      return (PFN_vkVoidFunction)fexfn_impl_libvulkan_vkCreateInstance;
  } else if (std::strcmp(a_1, "vkCreateDevice") == 0) {
      return (PFN_vkVoidFunction)fexfn_impl_libvulkan_vkCreateDevice;
  } else if (std::strcmp(a_1, "vkAllocateMemory") == 0) {
      return (PFN_vkVoidFunction)fexfn_impl_libvulkan_vkAllocateMemory;
  } else if (std::strcmp(a_1, "vkFreeMemory") == 0) {
      return (PFN_vkVoidFunction)fexfn_impl_libvulkan_vkFreeMemory;
#ifdef IS_32BIT_THUNK
  } else if (std::strcmp(a_1, "vkEnumeratePhysicalDevices") == 0) {
      return (PFN_vkVoidFunction)fexfn_impl_libvulkan_vkEnumeratePhysicalDevices;
  } else if (std::strcmp(a_1, "vkGetDeviceQueue") == 0) {
      return (PFN_vkVoidFunction)fexfn_impl_libvulkan_vkGetDeviceQueue;
  } else if (std::strcmp(a_1, "vkAllocateCommandBuffers") == 0) {
      return (PFN_vkVoidFunction)fexfn_impl_libvulkan_vkAllocateCommandBuffers;
  } else if (std::strcmp(a_1, "vkMapMemory") == 0) {
      return (PFN_vkVoidFunction)fexfn_impl_libvulkan_vkMapMemory;
  } else if (std::strcmp(a_1, "vkUpdateDescriptorSets") == 0) {
      return (PFN_vkVoidFunction)fexfn_impl_libvulkan_vkUpdateDescriptorSets;
  } else if (std::strcmp(a_1, "vkQueueSubmit") == 0) {
      return (PFN_vkVoidFunction)fexfn_impl_libvulkan_vkQueueSubmit;
  } else if (std::strcmp(a_1, "vkFreeCommandBuffers") == 0) {
      return (PFN_vkVoidFunction)fexfn_impl_libvulkan_vkFreeCommandBuffers;
  } else if (std::strcmp(a_1, "vkGetPipelineCacheData") == 0) {
      return (PFN_vkVoidFunction)fexfn_impl_libvulkan_vkGetPipelineCacheData;
#endif
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

#ifdef IS_32BIT_THUNK
template<VkStructureType TypeIndex, typename Type>
static const VkBaseOutStructure* convert(const VkBaseOutStructure* source) {
    // Using malloc here since no easily available type information is available at the time of destruction
    auto child = (host_layout<Type>*)malloc(sizeof(host_layout<Type>));
    new (child) host_layout<Type> { *reinterpret_cast<guest_layout<Type>*>((void*)(source)) }; // TODO: Use proper cast?

    // TODO: Trigger *full* custom repack for children, not just the Next member
    fex_custom_repack<&Type::pNext>(*child, *reinterpret_cast<guest_layout<Type>*>((void*)(source)));

    return (const VkBaseOutStructure*)child; // TODO: Use proper cast?
}

template<VkStructureType TypeIndex, typename Type>
inline constexpr std::pair<VkStructureType, const VkBaseOutStructure*(*)(const VkBaseOutStructure*)> converters =
  { TypeIndex, convert<TypeIndex, Type> };


static std::unordered_map<VkStructureType, const VkBaseOutStructure*(*)(const VkBaseOutStructure*)> next_handlers {
    converters<VkStructureType::VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO, VkSemaphoreTypeCreateInfo>,
    converters<VkStructureType::VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS, VkMemoryDedicatedRequirements>,
    converters<VkStructureType::VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO, VkMemoryDedicatedAllocateInfo>,
    converters<VkStructureType::VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO, VkImageFormatListCreateInfo>,
    converters<VkStructureType::VK_STRUCTURE_TYPE_RENDER_PASS_ATTACHMENT_BEGIN_INFO, VkRenderPassAttachmentBeginInfo>,
    converters<VkStructureType::VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO, VkTimelineSemaphoreSubmitInfo>,
    converters<VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO, VkPipelineRenderingCreateInfo>,
    converters<VkStructureType::VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO, VkMemoryAllocateFlagsInfo>,
};

// Normally, we would implement fex_custom_repack individually for each customized struct.
// In this case, they all need the same repacking, so we just implement it once and alias all fex_custom_repack instances
extern "C" void default_fex_custom_repack(/*host_layout<*/VkBaseOutStructure/*>*/& into, /* guest_layout<VkBaseOutStructure>& */void* from) {
  struct guest_vk_struct {
    VkStructureType sType;
    guest_layout<struct VkBaseOutStructure*> pNext;
  };

  auto* typed_source = reinterpret_cast<guest_vk_struct*>(from);
  if (!typed_source->pNext.get_pointer()) {
  fprintf(stderr, "  CUSTOM REPACK: no more pNext\n");
      into.pNext = nullptr;
      return;
  }
  typed_source = reinterpret_cast<guest_vk_struct*>(typed_source->pNext.get_pointer());

  auto next_handler = next_handlers.find(typed_source->sType);
  if (next_handler == next_handlers.end()) {
    fprintf(stderr, "ERROR: Unrecognized VkStructureType %u referenced by pNext\n", typed_source->sType);
    std::abort();
  }
  // TODO: Avoid redundant cast to VkBaseOutStructure...
  const_cast<const VkBaseOutStructure*&>(into.pNext) = next_handler->second((const VkBaseOutStructure*)typed_source);
}

template<>
void fex_custom_repack<&VkInstanceCreateInfo::pNext>(host_layout<VkInstanceCreateInfo>& into, const guest_layout<VkInstanceCreateInfo>& from) {
  // TODO
  default_fex_custom_repack((VkBaseOutStructure&)into.data, (void*)&from);
}

template<>
void fex_custom_repack_postcall<&VkInstanceCreateInfo::pNext>(const void* const&) {
  // TODO
}

#define CREATE_INFO_DEFAULT_CUSTOM_REPACK(name) \
template<> \
void fex_custom_repack<&name::pNext>(host_layout<name>& into, const guest_layout<name>& from) { \
fprintf(stderr, "CUSTOM REPACK: %s\n", __PRETTY_FUNCTION__);\
  default_fex_custom_repack((VkBaseOutStructure&)into.data, (void*)&from); \
} \
\
template<> \
void fex_custom_repack_postcall<&name::pNext>(decltype(std::declval<name>().pNext) const&) { \
  /* TODO */ \
}

CREATE_INFO_DEFAULT_CUSTOM_REPACK(VkAttachmentDescription2)
CREATE_INFO_DEFAULT_CUSTOM_REPACK(VkAttachmentReference2)
CREATE_INFO_DEFAULT_CUSTOM_REPACK(VkBaseOutStructure)
CREATE_INFO_DEFAULT_CUSTOM_REPACK(VkBufferMemoryBarrier2)
CREATE_INFO_DEFAULT_CUSTOM_REPACK(VkDependencyInfo)
CREATE_INFO_DEFAULT_CUSTOM_REPACK(VkDescriptorUpdateTemplateCreateInfo)
CREATE_INFO_DEFAULT_CUSTOM_REPACK(VkGraphicsPipelineCreateInfo)
CREATE_INFO_DEFAULT_CUSTOM_REPACK(VkImageFormatListCreateInfo)
CREATE_INFO_DEFAULT_CUSTOM_REPACK(VkImageMemoryBarrier2)
CREATE_INFO_DEFAULT_CUSTOM_REPACK(VkImageMemoryRequirementsInfo2)
CREATE_INFO_DEFAULT_CUSTOM_REPACK(VkMemoryAllocateInfo)
CREATE_INFO_DEFAULT_CUSTOM_REPACK(VkMemoryAllocateFlagsInfo)
CREATE_INFO_DEFAULT_CUSTOM_REPACK(VkMemoryBarrier2)
CREATE_INFO_DEFAULT_CUSTOM_REPACK(VkMemoryDedicatedAllocateInfo)
CREATE_INFO_DEFAULT_CUSTOM_REPACK(VkMemoryDedicatedRequirements)
CREATE_INFO_DEFAULT_CUSTOM_REPACK(VkMemoryRequirements2)
CREATE_INFO_DEFAULT_CUSTOM_REPACK(VkPipelineRenderingCreateInfo)
CREATE_INFO_DEFAULT_CUSTOM_REPACK(VkRenderPassAttachmentBeginInfo)
CREATE_INFO_DEFAULT_CUSTOM_REPACK(VkRenderPassBeginInfo)
CREATE_INFO_DEFAULT_CUSTOM_REPACK(VkRenderPassCreateInfo)
CREATE_INFO_DEFAULT_CUSTOM_REPACK(VkRenderPassCreateInfo2)
CREATE_INFO_DEFAULT_CUSTOM_REPACK(VkSemaphoreCreateInfo)
CREATE_INFO_DEFAULT_CUSTOM_REPACK(VkSemaphoreTypeCreateInfo)
CREATE_INFO_DEFAULT_CUSTOM_REPACK(VkSemaphoreWaitInfo)
CREATE_INFO_DEFAULT_CUSTOM_REPACK(VkSubmitInfo)
CREATE_INFO_DEFAULT_CUSTOM_REPACK(VkSubpassBeginInfo)
CREATE_INFO_DEFAULT_CUSTOM_REPACK(VkSubpassDependency2)
CREATE_INFO_DEFAULT_CUSTOM_REPACK(VkSubpassDescription2)
CREATE_INFO_DEFAULT_CUSTOM_REPACK(VkSwapchainCreateInfoKHR)
CREATE_INFO_DEFAULT_CUSTOM_REPACK(VkTimelineSemaphoreSubmitInfo)

template<>
void fex_custom_repack<&VkInstanceCreateInfo::pApplicationInfo>(host_layout<VkInstanceCreateInfo>& into, const guest_layout<VkInstanceCreateInfo>& from) {
  auto HostApplicationInfo = new host_layout<VkApplicationInfo> { *from.data.pApplicationInfo.get_pointer() };
  fex_apply_custom_repacking(*HostApplicationInfo, *from.data.pApplicationInfo.get_pointer());
  into.data.pApplicationInfo = &HostApplicationInfo->data;
}

template<>
void fex_custom_repack_postcall<&VkInstanceCreateInfo::pApplicationInfo>(const VkApplicationInfo* const& pApplicationInfo) {
  delete pApplicationInfo;
}

template<>
void fex_custom_repack<&VkInstanceCreateInfo::ppEnabledExtensionNames>(host_layout<VkInstanceCreateInfo>& into, const guest_layout<VkInstanceCreateInfo>& from) {
  auto extension_count = from.data.enabledExtensionCount.data;
  fprintf(stderr, "  Repacking %d ppEnabledExtensionNames\n", extension_count);
  into.data.ppEnabledExtensionNames = new const char*[extension_count];

  for (uint32_t i = 0; i < extension_count; ++i) {
    const guest_layout<const char* const>& ExtensionName = from.data.ppEnabledExtensionNames.get_pointer()[i];
    fprintf(stderr, "    EXT %p[%d]: %p %s\n", from.data.ppEnabledExtensionNames.get_pointer(), i, (void*)(uintptr_t)ExtensionName.data, (const char*)(uintptr_t)ExtensionName.data);
    const_cast<const char*&>(into.data.ppEnabledExtensionNames[i]) = host_layout<const char* const> { ExtensionName }.data;
  }
}

template<>
void fex_custom_repack_postcall<&VkInstanceCreateInfo::ppEnabledExtensionNames>(const char* const* const& data) {
  delete[] data;
}

template<>
void fex_custom_repack<&VkInstanceCreateInfo::ppEnabledLayerNames>(host_layout<VkInstanceCreateInfo>& into, const guest_layout<VkInstanceCreateInfo>& from) {
  auto layer_count = from.data.enabledLayerCount.data;
  fprintf(stderr, "  Repacking %d ppEnabledLayerNames\n", layer_count);
  into.data.ppEnabledLayerNames = new const char*[layer_count];

  for (uint32_t i = 0; i < layer_count; ++i) {
    const guest_layout<const char* const>& LayerName = from.data.ppEnabledLayerNames.get_pointer()[i];
    fprintf(stderr, "    EXT %p[%d]: %p %s\n", from.data.ppEnabledLayerNames.get_pointer(), i, (void*)(uintptr_t)LayerName.data, (const char*)(uintptr_t)LayerName.data);
    const_cast<const char*&>(into.data.ppEnabledLayerNames[i]) = host_layout<const char* const> { LayerName }.data;
  }
}

template<>
void fex_custom_repack_postcall<&VkInstanceCreateInfo::ppEnabledLayerNames>(const char* const* const& data) {
  delete[] data;
}

template<>
void fex_custom_repack<&VkApplicationInfo::pNext>(host_layout<VkApplicationInfo>& into, const guest_layout<VkApplicationInfo>& from) {
  // TODO
}

template<>
void fex_custom_repack_postcall<&VkApplicationInfo::pNext>(const void* const&) {
  // TODO
}

template<>
void fex_custom_repack<&VkDeviceQueueCreateInfo::pNext>(host_layout<VkDeviceQueueCreateInfo>& into, const guest_layout<VkDeviceQueueCreateInfo>& from) {
  // TODO
}

template<>
void fex_custom_repack_postcall<&VkDeviceQueueCreateInfo::pNext>(const void* const&) {
  // TODO
}

template<>
void fex_custom_repack<&VkDeviceCreateInfo::pNext>(host_layout<VkDeviceCreateInfo>& into, const guest_layout<VkDeviceCreateInfo>& from) {
  // TODO
}

template<>
void fex_custom_repack_postcall<&VkDeviceCreateInfo::pNext>(const void* const&) {
  // TODO
}

template<>
void fex_custom_repack<&VkDeviceCreateInfo::pQueueCreateInfos>(host_layout<VkDeviceCreateInfo>& into, const guest_layout<VkDeviceCreateInfo>& from) {
  auto HostQueueCreateInfo = new host_layout<VkDeviceQueueCreateInfo> { *from.data.pQueueCreateInfos.get_pointer() };
  fex_apply_custom_repacking(*HostQueueCreateInfo, *from.data.pQueueCreateInfos.get_pointer());
  into.data.pQueueCreateInfos = &HostQueueCreateInfo->data;
}

template<>
void fex_custom_repack_postcall<&VkDeviceCreateInfo::pQueueCreateInfos>(const VkDeviceQueueCreateInfo* const& pQueueCreateInfos) {
  delete pQueueCreateInfos;
}

template<>
void fex_custom_repack<&VkDeviceCreateInfo::ppEnabledExtensionNames>(host_layout<VkDeviceCreateInfo>& into, const guest_layout<VkDeviceCreateInfo>& from) {
  auto extension_count = from.data.enabledExtensionCount.data;
  fprintf(stderr, "  Repacking %d ppEnabledExtensionNames\n", extension_count);
  into.data.ppEnabledExtensionNames = new const char*[extension_count];

  for (uint32_t i = 0; i < extension_count; ++i) {
    const guest_layout<const char* const>& ExtensionName = from.data.ppEnabledExtensionNames.get_pointer()[i];
    fprintf(stderr, "    EXT %p[%d]: %p %s\n", from.data.ppEnabledExtensionNames.get_pointer(), i, (const char*)(uintptr_t)ExtensionName.data, (const char*)(uintptr_t)ExtensionName.data);
    const_cast<const char*&>(into.data.ppEnabledExtensionNames[i]) = host_layout<const char* const> { ExtensionName }.data;
  }
}

template<>
void fex_custom_repack_postcall<&VkDeviceCreateInfo::ppEnabledExtensionNames>(const char* const* const& data) {
  delete[] data;
}

template<>
void fex_custom_repack<&VkDeviceCreateInfo::ppEnabledLayerNames>(host_layout<VkDeviceCreateInfo>& into, const guest_layout<VkDeviceCreateInfo>& from) {
  // TODO
}

template<>
void fex_custom_repack_postcall<&VkDeviceCreateInfo::ppEnabledLayerNames>(const char* const* const& data) {
  // TODO
}

template<>
void fex_custom_repack<&VkImageViewCreateInfo::pNext>(host_layout<VkImageViewCreateInfo>& into, const guest_layout<VkImageViewCreateInfo>& from) {
  // TODO
}

template<>
void fex_custom_repack_postcall<&VkImageViewCreateInfo::pNext>(const void* const&) {
  // TODO
}

template<>
void fex_custom_repack<&VkDescriptorSetLayoutCreateInfo::pNext>(host_layout<VkDescriptorSetLayoutCreateInfo>& into, const guest_layout<VkDescriptorSetLayoutCreateInfo>& from) {
  // TODO
}

template<>
void fex_custom_repack_postcall<&VkDescriptorSetLayoutCreateInfo::pNext>(const void* const&) {
  // TODO
}

template<>
void fex_custom_repack<&VkDescriptorSetLayoutCreateInfo::pBindings>(host_layout<VkDescriptorSetLayoutCreateInfo>& into, const guest_layout<VkDescriptorSetLayoutCreateInfo>& from) {
  if (!from.data.bindingCount.data) {
    into.data.pBindings = nullptr;
    return;
  }

  into.data.pBindings = new VkDescriptorSetLayoutBinding[from.data.bindingCount.data];
  for (size_t i = 0; i < from.data.bindingCount.data; ++i) {
    auto in_data = host_layout<VkDescriptorSetLayoutBinding> { from.data.pBindings.get_pointer()[i] }.data;
    memcpy((void*)&into.data.pBindings[i], &in_data, sizeof(VkDescriptorSetLayoutBinding));
  }
}

template<>
void fex_custom_repack_postcall<&VkDescriptorSetLayoutCreateInfo::pBindings>(const VkDescriptorSetLayoutBinding* const& bindings) {
  delete[] bindings;
}

template<>
void fex_custom_repack<&VkRenderPassCreateInfo::pSubpasses>(host_layout<VkRenderPassCreateInfo>& into, const guest_layout<VkRenderPassCreateInfo>& from) {
  if (from.data.subpassCount.data == 0) {
    into.data.pSubpasses = nullptr;
    return;
  }

  into.data.pSubpasses = new VkSubpassDescription[from.data.subpassCount.data];
  for (size_t i = 0; i < from.data.subpassCount.data; ++i) {
    auto in_data = host_layout<VkSubpassDescription> { from.data.pSubpasses.get_pointer()[i] }.data;
    memcpy((void*)&into.data.pSubpasses[i], &in_data, sizeof(VkSubpassDescription));
  }
}

template<>
void fex_custom_repack_postcall<&VkRenderPassCreateInfo::pSubpasses>(const VkSubpassDescription* const& subpasses) {
  delete[] subpasses;
}

template<>
void fex_custom_repack<&VkRenderPassCreateInfo2::pAttachments>(host_layout<VkRenderPassCreateInfo2>& into, const guest_layout<VkRenderPassCreateInfo2>& from) {
  if (from.data.attachmentCount.data == 0) {
    into.data.pAttachments = nullptr;
    return;
  }

  into.data.pAttachments = new VkAttachmentDescription2[from.data.attachmentCount.data];
  for (size_t i = 0; i < from.data.attachmentCount.data; ++i) {
    auto in_data = host_layout<VkAttachmentDescription2> { from.data.pAttachments.get_pointer()[i] };
    fex_apply_custom_repacking(in_data, from.data.pAttachments.get_pointer()[i]);
    memcpy((void*)&into.data.pAttachments[i], &in_data.data, sizeof(VkAttachmentDescription2));
  }
}

template<>
void fex_custom_repack_postcall<&VkRenderPassCreateInfo2::pAttachments>(const VkAttachmentDescription2* const& attachments) {
  delete[] attachments;
  // TODO: Run custom exit-repacking
}

template<>
void fex_custom_repack<&VkRenderPassCreateInfo2::pSubpasses>(host_layout<VkRenderPassCreateInfo2>& into, const guest_layout<VkRenderPassCreateInfo2>& from) {
  if (from.data.subpassCount.data == 0) {
    into.data.pSubpasses = nullptr;
    return;
  }

  into.data.pSubpasses = new VkSubpassDescription2[from.data.subpassCount.data];
  for (size_t i = 0; i < from.data.subpassCount.data; ++i) {
    auto in_data = host_layout<VkSubpassDescription2> { from.data.pSubpasses.get_pointer()[i] };
    fex_apply_custom_repacking(in_data, from.data.pSubpasses.get_pointer()[i]);
    memcpy((void*)&into.data.pSubpasses[i], &in_data.data, sizeof(VkSubpassDescription2));
  }
}

template<>
void fex_custom_repack_postcall<&VkRenderPassCreateInfo2::pSubpasses>(const VkSubpassDescription2* const& subpasses) {
  delete[] subpasses;
  // TODO: Run custom exit-repacking
}

template<>
void fex_custom_repack<&VkRenderPassCreateInfo2::pDependencies>(host_layout<VkRenderPassCreateInfo2>& into, const guest_layout<VkRenderPassCreateInfo2>& from) {
  if (from.data.dependencyCount.data == 0) {
    into.data.pDependencies = nullptr;
    return;
  }

  into.data.pDependencies = new VkSubpassDependency2[from.data.dependencyCount.data];
  for (size_t i = 0; i < from.data.dependencyCount.data; ++i) {
    auto in_data = host_layout<VkSubpassDependency2> { from.data.pDependencies.get_pointer()[i] };
    fex_apply_custom_repacking(in_data, from.data.pDependencies.get_pointer()[i]);
    memcpy((void*)&into.data.pDependencies[i], &in_data.data, sizeof(VkSubpassDependency2));
  }
}

template<>
void fex_custom_repack_postcall<&VkRenderPassCreateInfo2::pDependencies>(const VkSubpassDependency2* const& dependencies) {
  delete[] dependencies;
  // TODO: Run custom exit-repacking
}

template<>
void fex_custom_repack<&VkSubpassDescription2::pInputAttachments>(host_layout<VkSubpassDescription2>& into, const guest_layout<VkSubpassDescription2>& from) {
  if (from.data.inputAttachmentCount.data == 0) {
    into.data.pInputAttachments = nullptr;
    return;
  }

  into.data.pInputAttachments = new VkAttachmentReference2[from.data.inputAttachmentCount.data];
  for (size_t i = 0; i < from.data.inputAttachmentCount.data; ++i) {
    auto in_data = host_layout<VkAttachmentReference2> { from.data.pInputAttachments.get_pointer()[i] };
    fex_apply_custom_repacking(in_data, from.data.pInputAttachments.get_pointer()[i]);
    memcpy((void*)&into.data.pInputAttachments[i], &in_data.data, sizeof(VkAttachmentReference2));
  }
}

template<>
void fex_custom_repack_postcall<&VkSubpassDescription2::pInputAttachments>(const VkAttachmentReference2* const& references) {
  delete[] references;
  // TODO: Run custom exit-repacking
}

template<>
void fex_custom_repack<&VkSubpassDescription2::pColorAttachments>(host_layout<VkSubpassDescription2>& into, const guest_layout<VkSubpassDescription2>& from) {
  if (from.data.colorAttachmentCount.data == 0) {
    into.data.pColorAttachments = nullptr;
    return;
  }

  into.data.pColorAttachments = new VkAttachmentReference2[from.data.colorAttachmentCount.data];
  for (size_t i = 0; i < from.data.colorAttachmentCount.data; ++i) {
    auto in_data = host_layout<VkAttachmentReference2> { from.data.pColorAttachments.get_pointer()[i] };
    fex_apply_custom_repacking(in_data, from.data.pColorAttachments.get_pointer()[i]);
    memcpy((void*)&into.data.pColorAttachments[i], &in_data.data, sizeof(VkAttachmentReference2));
  }
}

template<>
void fex_custom_repack_postcall<&VkSubpassDescription2::pColorAttachments>(const VkAttachmentReference2* const& references) {
  delete[] references;
  // TODO: Run custom exit-repacking
}

template<>
void fex_custom_repack<&VkSubpassDescription2::pResolveAttachments>(host_layout<VkSubpassDescription2>& into, const guest_layout<VkSubpassDescription2>& from) {
  if (from.data.colorAttachmentCount.data == 0 || from.data.pResolveAttachments.get_pointer() == nullptr) {
    into.data.pResolveAttachments = nullptr;
    return;
  }

  into.data.pResolveAttachments = new VkAttachmentReference2[from.data.colorAttachmentCount.data];
  for (size_t i = 0; i < from.data.colorAttachmentCount.data; ++i) {
    auto in_data = host_layout<VkAttachmentReference2> { from.data.pResolveAttachments.get_pointer()[i] };
    fex_apply_custom_repacking(in_data, from.data.pResolveAttachments.get_pointer()[i]);
    memcpy((void*)&into.data.pResolveAttachments[i], &in_data.data, sizeof(VkAttachmentReference2));
  }
}

template<>
void fex_custom_repack_postcall<&VkSubpassDescription2::pResolveAttachments>(const VkAttachmentReference2* const& references) {
  delete[] references;
  // TODO: Run custom exit-repacking
}

template<>
void fex_custom_repack<&VkSubpassDescription2::pDepthStencilAttachment>(host_layout<VkSubpassDescription2>& into, const guest_layout<VkSubpassDescription2>& from) {
  if (from.data.pDepthStencilAttachment.data == 0) {
    into.data.pDepthStencilAttachment = nullptr;
    return;
  }

  into.data.pDepthStencilAttachment = new VkAttachmentReference2;
  auto in_data = host_layout<VkAttachmentReference2> { *from.data.pDepthStencilAttachment.get_pointer() };
  fex_apply_custom_repacking(in_data, *from.data.pDepthStencilAttachment.get_pointer());
  memcpy((void*)into.data.pDepthStencilAttachment, &in_data.data, sizeof(VkAttachmentReference2));
}

template<>
void fex_custom_repack_postcall<&VkSubpassDescription2::pDepthStencilAttachment>(const VkAttachmentReference2* const& references) {
  delete[] references;
  // TODO: Run custom exit-repacking
}

template<>
void fex_custom_repack<&VkRenderingInfo::pColorAttachments>(host_layout<VkRenderingInfo>& into, const guest_layout<VkRenderingInfo>& from) {
  if (from.data.colorAttachmentCount.data == 0) {
    into.data.pColorAttachments = nullptr;
    return;
  }

  into.data.pColorAttachments = new VkRenderingAttachmentInfo[from.data.colorAttachmentCount.data];
  for (size_t i = 0; i < from.data.colorAttachmentCount.data; ++i) {
    auto in_data = host_layout<VkRenderingAttachmentInfo> { from.data.pColorAttachments.get_pointer()[i] };
    fex_apply_custom_repacking(in_data, from.data.pColorAttachments.get_pointer()[i]);
    memcpy((void*)&into.data.pColorAttachments[i], &in_data.data, sizeof(VkRenderingAttachmentInfo));
  }
}

template<>
void fex_custom_repack_postcall<&VkRenderingInfo::pColorAttachments>(const VkRenderingAttachmentInfo* const& info) {
  delete[] info;
  // TODO: Run custom exit-repacking
}

template<>
void fex_custom_repack<&VkRenderingInfo::pDepthAttachment>(host_layout<VkRenderingInfo>& into, const guest_layout<VkRenderingInfo>& from) {
  if (from.data.pDepthAttachment.get_pointer() == nullptr) {
    into.data.pDepthAttachment = nullptr;
    return;
  }

  into.data.pDepthAttachment = new VkRenderingAttachmentInfo;
  auto in_data = host_layout<VkRenderingAttachmentInfo> { *from.data.pDepthAttachment.get_pointer() };
  fex_apply_custom_repacking(in_data, *from.data.pDepthAttachment.get_pointer());
  memcpy((void*)into.data.pDepthAttachment, &in_data.data, sizeof(VkRenderingAttachmentInfo));
}

template<>
void fex_custom_repack_postcall<&VkRenderingInfo::pDepthAttachment>(const VkRenderingAttachmentInfo* const& info) {
  delete info;
  // TODO: Run custom exit-repacking
}

template<>
void fex_custom_repack<&VkRenderingInfo::pStencilAttachment>(host_layout<VkRenderingInfo>& into, const guest_layout<VkRenderingInfo>& from) {
  if (from.data.pStencilAttachment.get_pointer() == nullptr) {
    into.data.pStencilAttachment = nullptr;
    return;
  }

  into.data.pStencilAttachment = new VkRenderingAttachmentInfo;
  auto in_data = host_layout<VkRenderingAttachmentInfo> { *from.data.pStencilAttachment.get_pointer() };
  fex_apply_custom_repacking(in_data, *from.data.pStencilAttachment.get_pointer());
  memcpy((void*)into.data.pStencilAttachment, &in_data.data, sizeof(VkRenderingAttachmentInfo));
}

template<>
void fex_custom_repack_postcall<&VkRenderingInfo::pStencilAttachment>(const VkRenderingAttachmentInfo* const& info) {
  delete info;
  // TODO: Run custom exit-repacking
}

template<>
void fex_custom_repack<&VkDependencyInfo::pMemoryBarriers>(host_layout<VkDependencyInfo>& into, const guest_layout<VkDependencyInfo>& from) {
  if (from.data.memoryBarrierCount.data == 0) {
    into.data.pMemoryBarriers = nullptr;
    return;
  }

  into.data.pMemoryBarriers = new VkMemoryBarrier2[from.data.memoryBarrierCount.data];
  for (size_t i = 0; i < from.data.memoryBarrierCount.data; ++i) {
    auto in_data = host_layout<VkMemoryBarrier2> { from.data.pMemoryBarriers.get_pointer()[i] };
    fex_apply_custom_repacking(in_data, from.data.pMemoryBarriers.get_pointer()[i]);
    memcpy((void*)&into.data.pMemoryBarriers[i], &in_data.data, sizeof(VkMemoryBarrier2));
  }
}

template<>
void fex_custom_repack_postcall<&VkDependencyInfo::pMemoryBarriers>(const VkMemoryBarrier2* const& barriers) {
  delete[] barriers;
  // TODO: Run custom exit-repacking
}

template<>
void fex_custom_repack<&VkDependencyInfo::pBufferMemoryBarriers>(host_layout<VkDependencyInfo>& into, const guest_layout<VkDependencyInfo>& from) {
  if (from.data.bufferMemoryBarrierCount.data == 0) {
    into.data.pBufferMemoryBarriers = nullptr;
    return;
  }

  into.data.pBufferMemoryBarriers = new VkBufferMemoryBarrier2[from.data.bufferMemoryBarrierCount.data];
  for (size_t i = 0; i < from.data.bufferMemoryBarrierCount.data; ++i) {
    auto in_data = host_layout<VkBufferMemoryBarrier2> { from.data.pBufferMemoryBarriers.get_pointer()[i] };
    fex_apply_custom_repacking(in_data, from.data.pBufferMemoryBarriers.get_pointer()[i]);
    memcpy((void*)&into.data.pBufferMemoryBarriers[i], &in_data.data, sizeof(VkBufferMemoryBarrier2));
  }
}

template<>
void fex_custom_repack_postcall<&VkDependencyInfo::pBufferMemoryBarriers>(const VkBufferMemoryBarrier2* const& barriers) {
  delete[] barriers;
  // TODO: Run custom exit-repacking
}

template<>
void fex_custom_repack<&VkDependencyInfo::pImageMemoryBarriers>(host_layout<VkDependencyInfo>& into, const guest_layout<VkDependencyInfo>& from) {
  if (from.data.imageMemoryBarrierCount.data == 0) {
    into.data.pImageMemoryBarriers = nullptr;
    return;
  }

  into.data.pImageMemoryBarriers = new VkImageMemoryBarrier2[from.data.imageMemoryBarrierCount.data];
  for (size_t i = 0; i < from.data.imageMemoryBarrierCount.data; ++i) {
    auto in_data = host_layout<VkImageMemoryBarrier2> { from.data.pImageMemoryBarriers.get_pointer()[i] };
    fex_apply_custom_repacking(in_data, from.data.pImageMemoryBarriers.get_pointer()[i]);
    memcpy((void*)&into.data.pImageMemoryBarriers[i], &in_data.data, sizeof(VkImageMemoryBarrier2));
  }
}

template<>
void fex_custom_repack_postcall<&VkDependencyInfo::pImageMemoryBarriers>(const VkImageMemoryBarrier2* const& barriers) {
  delete[] barriers;
  // TODO: Run custom exit-repacking
}

template<>
void fex_custom_repack<&VkDescriptorUpdateTemplateCreateInfo::pDescriptorUpdateEntries>(host_layout<VkDescriptorUpdateTemplateCreateInfo>& into, const guest_layout<VkDescriptorUpdateTemplateCreateInfo>& from) {
  if (from.data.descriptorUpdateEntryCount.data == 0) {
    into.data.pDescriptorUpdateEntries = nullptr;
    return;
  }

  into.data.pDescriptorUpdateEntries = new VkDescriptorUpdateTemplateEntry[from.data.descriptorUpdateEntryCount.data];
  for (size_t i = 0; i < from.data.descriptorUpdateEntryCount.data; ++i) {
    auto in_data = host_layout<VkDescriptorUpdateTemplateEntry> { from.data.pDescriptorUpdateEntries.get_pointer()[i] };
    fex_apply_custom_repacking(in_data, from.data.pDescriptorUpdateEntries.get_pointer()[i]);
    memcpy((void*)&into.data.pDescriptorUpdateEntries[i], &in_data.data, sizeof(VkDescriptorUpdateTemplateEntry));
  }
}

template<>
void fex_custom_repack_postcall<&VkDescriptorUpdateTemplateCreateInfo::pDescriptorUpdateEntries>(const VkDescriptorUpdateTemplateEntry* const& entries) {
  delete[] entries;
  // TODO: Run custom exit-repacking
}

template<>
void fex_custom_repack<&VkPipelineShaderStageCreateInfo::pSpecializationInfo>(host_layout<VkPipelineShaderStageCreateInfo>& into, const guest_layout<VkPipelineShaderStageCreateInfo>& from) {
  if (from.data.pSpecializationInfo.get_pointer()) {
    fprintf(stderr, "ERROR: Cannot repack non-null VkPipelineShaderStageCreateInfo::pSpecializationInfo yet");
    std::abort();
  }
}

template<>
void fex_custom_repack_postcall<&VkPipelineShaderStageCreateInfo::pSpecializationInfo>(const VkSpecializationInfo* const&) {
  // TODO
}

template<>
void fex_custom_repack<&VkGraphicsPipelineCreateInfo::pStages>(host_layout<VkGraphicsPipelineCreateInfo>& into, const guest_layout<VkGraphicsPipelineCreateInfo>& from) {
  if (!from.data.stageCount.data) {
    into.data.pStages = nullptr;
    return;
  }
  auto host_stages = new VkPipelineShaderStageCreateInfo[from.data.stageCount.data];
  for (size_t i = 0; i < from.data.stageCount.data; ++i) {
     host_stages[i] = host_layout<VkPipelineShaderStageCreateInfo> { from.data.pStages.get_pointer()[i] }.data;
  }
  into.data.pStages = host_stages;
}

template<>
void fex_custom_repack_postcall<&VkGraphicsPipelineCreateInfo::pStages>(const VkPipelineShaderStageCreateInfo* const& stages) {
  delete[] stages;
}

template<>
void fex_custom_repack<&VkGraphicsPipelineCreateInfo::pVertexInputState>(host_layout<VkGraphicsPipelineCreateInfo>& into, const guest_layout<VkGraphicsPipelineCreateInfo>& from) {
  if (!from.data.pVertexInputState.get_pointer()) {
    into.data.pVertexInputState = nullptr;
    return;
  }
  into.data.pVertexInputState = &(new host_layout<VkPipelineVertexInputStateCreateInfo> { *from.data.pVertexInputState.get_pointer() })->data;
}

template<>
void fex_custom_repack_postcall<&VkGraphicsPipelineCreateInfo::pVertexInputState>(const VkPipelineVertexInputStateCreateInfo* const& info) {
  delete info;
}

template<>
void fex_custom_repack<&VkGraphicsPipelineCreateInfo::pInputAssemblyState>(host_layout<VkGraphicsPipelineCreateInfo>& into, const guest_layout<VkGraphicsPipelineCreateInfo>& from) {
  if (!from.data.pInputAssemblyState.get_pointer()) {
    into.data.pInputAssemblyState = nullptr;
    return;
  }
  into.data.pInputAssemblyState = &(new host_layout<VkPipelineInputAssemblyStateCreateInfo> { *from.data.pInputAssemblyState.get_pointer() })->data;
}

template<>
void fex_custom_repack_postcall<&VkGraphicsPipelineCreateInfo::pInputAssemblyState>(const VkPipelineInputAssemblyStateCreateInfo* const& info) {
  delete info;
}

template<>
void fex_custom_repack<&VkGraphicsPipelineCreateInfo::pTessellationState>(host_layout<VkGraphicsPipelineCreateInfo>& into, const guest_layout<VkGraphicsPipelineCreateInfo>& from) {
  if (!from.data.pTessellationState.get_pointer()) {
    into.data.pTessellationState = nullptr;
    return;
  }
  into.data.pTessellationState = &(new host_layout<VkPipelineTessellationStateCreateInfo> { *from.data.pTessellationState.get_pointer() })->data;
}

template<>
void fex_custom_repack_postcall<&VkGraphicsPipelineCreateInfo::pTessellationState>(const VkPipelineTessellationStateCreateInfo* const& info) {
  delete info;
}

template<>
void fex_custom_repack<&VkGraphicsPipelineCreateInfo::pViewportState>(host_layout<VkGraphicsPipelineCreateInfo>& into, const guest_layout<VkGraphicsPipelineCreateInfo>& from) {
  if (!from.data.pViewportState.get_pointer()) {
    into.data.pViewportState = nullptr;
    return;
  }
  into.data.pViewportState = &(new host_layout<VkPipelineViewportStateCreateInfo> { *from.data.pViewportState.get_pointer() })->data;
}

template<>
void fex_custom_repack_postcall<&VkGraphicsPipelineCreateInfo::pViewportState>(const VkPipelineViewportStateCreateInfo* const& info) {
  delete info;
}

template<>
void fex_custom_repack<&VkGraphicsPipelineCreateInfo::pRasterizationState>(host_layout<VkGraphicsPipelineCreateInfo>& into, const guest_layout<VkGraphicsPipelineCreateInfo>& from) {
  if (!from.data.pRasterizationState.get_pointer()) {
    into.data.pRasterizationState = nullptr;
    return;
  }
  into.data.pRasterizationState = &(new host_layout<VkPipelineRasterizationStateCreateInfo> { *from.data.pRasterizationState.get_pointer() })->data;
}

template<>
void fex_custom_repack_postcall<&VkGraphicsPipelineCreateInfo::pRasterizationState>(const VkPipelineRasterizationStateCreateInfo* const& info) {
  delete info;
}

template<>
void fex_custom_repack<&VkGraphicsPipelineCreateInfo::pMultisampleState>(host_layout<VkGraphicsPipelineCreateInfo>& into, const guest_layout<VkGraphicsPipelineCreateInfo>& from) {
  if (!from.data.pMultisampleState.get_pointer()) {
    into.data.pMultisampleState = nullptr;
    return;
  }
  into.data.pMultisampleState = &(new host_layout<VkPipelineMultisampleStateCreateInfo> { *from.data.pMultisampleState.get_pointer() })->data;
}

template<>
void fex_custom_repack_postcall<&VkGraphicsPipelineCreateInfo::pMultisampleState>(const VkPipelineMultisampleStateCreateInfo* const& info) {
  delete info;
}

template<>
void fex_custom_repack<&VkGraphicsPipelineCreateInfo::pDepthStencilState>(host_layout<VkGraphicsPipelineCreateInfo>& into, const guest_layout<VkGraphicsPipelineCreateInfo>& from) {
  if (!from.data.pDepthStencilState.get_pointer()) {
    into.data.pDepthStencilState = nullptr;
    return;
  }
  into.data.pDepthStencilState = &(new host_layout<VkPipelineDepthStencilStateCreateInfo> { *from.data.pDepthStencilState.get_pointer() })->data;
}

template<>
void fex_custom_repack_postcall<&VkGraphicsPipelineCreateInfo::pDepthStencilState>(const VkPipelineDepthStencilStateCreateInfo* const& info) {
  delete info;
}

template<>
void fex_custom_repack<&VkGraphicsPipelineCreateInfo::pColorBlendState>(host_layout<VkGraphicsPipelineCreateInfo>& into, const guest_layout<VkGraphicsPipelineCreateInfo>& from) {
  if (!from.data.pColorBlendState.get_pointer()) {
    into.data.pColorBlendState = nullptr;
    return;
  }
  into.data.pColorBlendState = &(new host_layout<VkPipelineColorBlendStateCreateInfo> { *from.data.pColorBlendState.get_pointer() })->data;
}

template<>
void fex_custom_repack_postcall<&VkGraphicsPipelineCreateInfo::pColorBlendState>(const VkPipelineColorBlendStateCreateInfo* const& info) {
  delete info;
}

template<>
void fex_custom_repack<&VkGraphicsPipelineCreateInfo::pDynamicState>(host_layout<VkGraphicsPipelineCreateInfo>& into, const guest_layout<VkGraphicsPipelineCreateInfo>& from) {
  if (!from.data.pDynamicState.get_pointer()) {
    into.data.pDynamicState = nullptr;
    return;
  }
  into.data.pDynamicState = &(new host_layout<VkPipelineDynamicStateCreateInfo> { *from.data.pDynamicState.get_pointer() })->data;
}

template<>
void fex_custom_repack_postcall<&VkGraphicsPipelineCreateInfo::pDynamicState>(const VkPipelineDynamicStateCreateInfo* const& info) {
  delete info;
}

template<>
void fex_custom_repack<&VkSubmitInfo::pCommandBuffers>(host_layout<VkSubmitInfo>& into, const guest_layout<VkSubmitInfo>& from) {
  if (!from.data.pCommandBuffers.get_pointer() || !from.data.commandBufferCount.data) {
    into.data.pCommandBuffers = nullptr;
    return;
  }
  into.data.pCommandBuffers = new VkCommandBuffer[from.data.commandBufferCount.data];
  for (size_t i = 0; i < from.data.commandBufferCount.data; ++i) {
    auto src = host_layout<VkCommandBuffer_T*> { (const guest_layout<VkCommandBuffer_T *>&)from.data.pCommandBuffers.get_pointer()[i] }.data;
    static_assert(sizeof(src) == sizeof(into.data.pCommandBuffers[i]));
    memcpy((void*)&into.data.pCommandBuffers[i], &src, sizeof(src));
  }
}

template<>
void fex_custom_repack_postcall<&VkSubmitInfo::pCommandBuffers>(const VkCommandBuffer* const& buffers) {
  delete[] buffers;
}

template<>
void fex_custom_repack<&VkCommandBufferBeginInfo::pInheritanceInfo>(host_layout<VkCommandBufferBeginInfo>& into, const guest_layout<VkCommandBufferBeginInfo>& from) {
  if (!from.data.pInheritanceInfo.get_pointer() || !from.data.pInheritanceInfo.data) {
    into.data.pInheritanceInfo = nullptr;
    return;
  }
  into.data.pInheritanceInfo = new VkCommandBufferInheritanceInfo;
  auto src = host_layout<VkCommandBufferInheritanceInfo> { *from.data.pInheritanceInfo.get_pointer() }.data;
  static_assert(sizeof(src) == sizeof(*into.data.pInheritanceInfo));
  memcpy((void*)into.data.pInheritanceInfo, &src, sizeof(src));
}

template<>
void fex_custom_repack_postcall<&VkCommandBufferBeginInfo::pInheritanceInfo>(const VkCommandBufferInheritanceInfo* const& info) {
  delete info;
}

template<>
void fex_custom_repack<&VkPipelineCacheCreateInfo::pInitialData>(host_layout<VkPipelineCacheCreateInfo>& into, const guest_layout<VkPipelineCacheCreateInfo>& from) {
  // Same underlying layout, so there's nothing to do
  into.data.pInitialData = from.data.pInitialData.get_pointer();
}

template<>
void fex_custom_repack_postcall<&VkPipelineCacheCreateInfo::pInitialData>(const void* const&) {
  // Nothing to do
}

template<>
void fex_custom_repack<&VkRenderPassBeginInfo::pClearValues>(host_layout<VkRenderPassBeginInfo>& into, const guest_layout<VkRenderPassBeginInfo>& from) {
  // Same underlying layout, so there's nothing to do
  into.data.pClearValues = reinterpret_cast<const VkClearValue*>(from.data.pClearValues.get_pointer());
}

template<>
void fex_custom_repack_postcall<&VkRenderPassBeginInfo::pClearValues>(const VkClearValue* const&) {
  // Nothing to do
}
#endif


EXPORTS(libvulkan)
