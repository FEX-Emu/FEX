// This is a completely generated file!
// To generate this file, Execute the following command!
// ./Scripts/DefinitionExtract.py --json_def ThunkLibs/libvulkan/vulkan_interface.json -- -isystem `readlink -f External/Vulkan-Headers/include/` > ThunkLibs/libvulkan/libvulkan_interface.cpp

#include <common/GeneratorInterface.h>
#include <type_traits>

template<auto>
struct fex_gen_config {
  unsigned version = 1;
};

// Some of Vulkan's handle types are so-called "non-dispatchable handles".
// On 64-bit, these are defined as dedicated types by default, which makes
// annotating these handle types unnecessarily complicated. Instead, setting
// the following define will make the Vulkan headers alias all handle types
// to uint64_t.
#define VK_USE_64_BIT_PTR_DEFINES 0

#define VK_USE_PLATFORM_XLIB_XRANDR_EXT
#define VK_USE_PLATFORM_XLIB_KHR
#define VK_USE_PLATFORM_XCB_KHR
#define VK_USE_PLATFORM_WAYLAND_KHR
#include <vulkan/vulkan.h>

#include "libvulkan_pnext_gen.inl"

template<typename>
struct fex_gen_type {};

namespace internal {
// Function, parameter index, parameter type [optional]
template<auto, int, typename = void>
struct fex_gen_param {};

template<auto>
struct fex_gen_config : fexgen::generate_guest_symtable, fexgen::indirect_guest_calls {};
} // namespace internal

// internal use
void SetGuestXSync(uintptr_t, uintptr_t);
void SetGuestXGetVisualInfo(uintptr_t, uintptr_t);
void SetGuestXDisplayString(uintptr_t, uintptr_t);

// X11 interop
template<>
struct fex_gen_config<&VkXcbSurfaceCreateInfoKHR::connection> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkXlibSurfaceCreateInfoKHR::dpy> : fexgen::custom_repack {};

template<>
struct fex_gen_type<VkInstance_T> : fexgen::opaque_type {};
template<>
struct fex_gen_type<VkPhysicalDevice_T> : fexgen::opaque_type {};
template<>
struct fex_gen_type<VkDevice_T> : fexgen::opaque_type {};
template<>
struct fex_gen_type<VkQueue_T> : fexgen::opaque_type {};
// So-called "dispatchable" handles are represented as opaque pointers.
// In addition to marking them as such, API functions that create these objects
// need special care since they wrap these handles in another pointer, which
// the thunk generator can't automatically handle.
//
// So-called "non-dispatchable" handles don't need this extra treatment, since
// they are uint64_t IDs on both 32-bit and 64-bit systems.
template<>
struct fex_gen_type<VkCommandBuffer_T> : fexgen::opaque_type {};
template<>
struct fex_gen_type<VkExtent2D> {};
template<>
struct fex_gen_type<VkExtent3D> {};
template<>
struct fex_gen_type<VkOffset2D> {};
template<>
struct fex_gen_type<VkOffset3D> {};
template<>
struct fex_gen_type<VkRect2D> {};
#ifdef IS_32BIT_THUNK
// The pNext member of this is a pointer to another VkBaseOutStructure, so data layout compatibility can't be inferred automatically
template<>
struct fex_gen_type<VkBaseOutStructure> : fexgen::emit_layout_wrappers {};
#endif
#ifndef IS_32BIT_THUNK
// The pNext member of this is a pointer to another VkBaseOutStructure, so data layout compatibility can't be inferred automatically
template<>
struct fex_gen_type<VkBaseOutStructure> : fexgen::assume_compatible_data_layout {};
#endif
template<>
struct fex_gen_type<VkBufferMemoryBarrier> {};
template<>
struct fex_gen_type<VkDispatchIndirectCommand> {};
template<>
struct fex_gen_type<VkDrawIndexedIndirectCommand> {};
template<>
struct fex_gen_type<VkDrawIndirectCommand> {};
template<>
struct fex_gen_type<VkImageSubresourceRange> {};
template<>
struct fex_gen_type<VkImageMemoryBarrier> {};
template<>
struct fex_gen_type<VkMemoryBarrier> {};
template<>
struct fex_gen_type<VkPipelineCacheHeaderVersionOne> {};
// TODO: Should not be opaque, but it's usually NULL anyway. Supporting the contained function pointers will need more work.
template<>
struct fex_gen_type<VkAllocationCallbacks> : fexgen::opaque_type {};
template<>
struct fex_gen_type<VkApplicationInfo> {};
template<>
struct fex_gen_type<VkFormatProperties> {};
template<>
struct fex_gen_type<VkImageFormatProperties> {};
template<>
struct fex_gen_type<VkInstanceCreateInfo> {};
template<>
struct fex_gen_type<VkMemoryHeap> {};
template<>
struct fex_gen_type<VkMemoryType> {};
template<>
struct fex_gen_type<VkPhysicalDeviceFeatures> {};
template<>
struct fex_gen_type<VkPhysicalDeviceLimits> {};
template<>
struct fex_gen_type<VkPhysicalDeviceMemoryProperties> {};
template<>
struct fex_gen_type<VkPhysicalDeviceSparseProperties> {};
template<>
struct fex_gen_type<VkPhysicalDeviceProperties> {};
template<>
struct fex_gen_type<VkQueueFamilyProperties> {};
template<>
struct fex_gen_type<VkDeviceQueueCreateInfo> {};
template<>
struct fex_gen_type<VkDeviceCreateInfo> {};
template<>
struct fex_gen_type<VkExtensionProperties> {};
template<>
struct fex_gen_type<VkLayerProperties> {};
template<>
struct fex_gen_type<VkSubmitInfo> {};
template<>
struct fex_gen_type<VkMappedMemoryRange> {};
template<>
struct fex_gen_type<VkMemoryAllocateInfo> {};
template<>
struct fex_gen_type<VkMemoryRequirements> {};
template<>
struct fex_gen_type<VkSparseMemoryBind> {};
template<>
struct fex_gen_type<VkSparseBufferMemoryBindInfo> {};
template<>
struct fex_gen_type<VkSparseImageOpaqueMemoryBindInfo> {};
template<>
struct fex_gen_type<VkImageSubresource> {};
template<>
struct fex_gen_type<VkSparseImageMemoryBind> {};
template<>
struct fex_gen_type<VkSparseImageMemoryBindInfo> {};
template<>
struct fex_gen_type<VkBindSparseInfo> {};
template<>
struct fex_gen_type<VkSparseImageFormatProperties> {};
template<>
struct fex_gen_type<VkSparseImageMemoryRequirements> {};
template<>
struct fex_gen_type<VkFenceCreateInfo> {};
template<>
struct fex_gen_type<VkSemaphoreCreateInfo> {};
template<>
struct fex_gen_type<VkEventCreateInfo> {};
template<>
struct fex_gen_type<VkQueryPoolCreateInfo> {};
template<>
struct fex_gen_type<VkBufferCreateInfo> {};
template<>
struct fex_gen_type<VkBufferViewCreateInfo> {};
template<>
struct fex_gen_type<VkImageCreateInfo> {};
template<>
struct fex_gen_type<VkSubresourceLayout> {};
template<>
struct fex_gen_type<VkComponentMapping> {};
template<>
struct fex_gen_type<VkImageViewCreateInfo> {};
template<>
struct fex_gen_type<VkShaderModuleCreateInfo> {};
template<>
struct fex_gen_type<VkPipelineCacheCreateInfo> {};
template<>
struct fex_gen_type<VkSpecializationMapEntry> {};
template<>
struct fex_gen_type<VkSpecializationInfo> {};
template<>
struct fex_gen_type<VkPipelineShaderStageCreateInfo> {};
template<>
struct fex_gen_type<VkComputePipelineCreateInfo> {};
template<>
struct fex_gen_type<VkVertexInputBindingDescription> {};
template<>
struct fex_gen_type<VkVertexInputAttributeDescription> {};
template<>
struct fex_gen_type<VkPipelineVertexInputStateCreateInfo> {};
template<>
struct fex_gen_type<VkPipelineInputAssemblyStateCreateInfo> {};
template<>
struct fex_gen_type<VkPipelineTessellationStateCreateInfo> {};
template<>
struct fex_gen_type<VkViewport> {};
template<>
struct fex_gen_type<VkPipelineViewportStateCreateInfo> {};
template<>
struct fex_gen_type<VkPipelineRasterizationStateCreateInfo> {};
template<>
struct fex_gen_type<VkPipelineMultisampleStateCreateInfo> {};
template<>
struct fex_gen_type<VkStencilOpState> {};
template<>
struct fex_gen_type<VkPipelineDepthStencilStateCreateInfo> {};
template<>
struct fex_gen_type<VkPipelineColorBlendAttachmentState> {};
template<>
struct fex_gen_type<VkPipelineColorBlendStateCreateInfo> {};
template<>
struct fex_gen_type<VkPipelineDynamicStateCreateInfo> {};
template<>
struct fex_gen_type<VkGraphicsPipelineCreateInfo> {};
template<>
struct fex_gen_type<VkPushConstantRange> {};
template<>
struct fex_gen_type<VkPipelineLayoutCreateInfo> {};
template<>
struct fex_gen_type<VkSamplerCreateInfo> {};
template<>
struct fex_gen_type<VkCopyDescriptorSet> {};
template<>
struct fex_gen_type<VkDescriptorBufferInfo> {};
template<>
struct fex_gen_type<VkDescriptorImageInfo> {};
template<>
struct fex_gen_type<VkDescriptorPoolSize> {};
template<>
struct fex_gen_type<VkDescriptorPoolCreateInfo> {};
template<>
struct fex_gen_type<VkDescriptorSetAllocateInfo> {};
template<>
struct fex_gen_type<VkDescriptorSetLayoutBinding> {};
template<>
struct fex_gen_type<VkDescriptorSetLayoutCreateInfo> {};
#ifdef IS_32BIT_THUNK
// These types have incompatible data layout but we use their layout wrappers elsewhere
template<>
struct fex_gen_type<VkWriteDescriptorSet> : fexgen::emit_layout_wrappers {};
#endif
#ifndef IS_32BIT_THUNK
// These types have incompatible data layout but we use their layout wrappers elsewhere
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_type<VkWriteDescriptorSet> {};
#endif
template<>
struct fex_gen_type<VkAttachmentDescription> {};
template<>
struct fex_gen_type<VkAttachmentReference> {};
template<>
struct fex_gen_type<VkFramebufferCreateInfo> {};
template<>
struct fex_gen_type<VkSubpassDescription> {};
template<>
struct fex_gen_type<VkSubpassDependency> {};
template<>
struct fex_gen_type<VkRenderPassCreateInfo> {};
template<>
struct fex_gen_type<VkCommandPoolCreateInfo> {};
template<>
struct fex_gen_type<VkCommandBufferAllocateInfo> {};
template<>
struct fex_gen_type<VkCommandBufferInheritanceInfo> {};
template<>
struct fex_gen_type<VkCommandBufferBeginInfo> {};
template<>
struct fex_gen_type<VkBufferCopy> {};
template<>
struct fex_gen_type<VkImageSubresourceLayers> {};
template<>
struct fex_gen_type<VkBufferImageCopy> {};
template<>
struct fex_gen_type<VkClearColorValue> : fexgen::assume_compatible_data_layout {};
template<>
struct fex_gen_type<VkClearDepthStencilValue> {};
// Mark union types with compatible layout as such
// TODO: These may still have different alignment requirements!
template<>
struct fex_gen_type<VkClearValue> : fexgen::assume_compatible_data_layout {};
template<>
struct fex_gen_type<VkClearAttachment> {};
template<>
struct fex_gen_type<VkClearRect> {};
template<>
struct fex_gen_type<VkImageBlit> {};
template<>
struct fex_gen_type<VkImageCopy> {};
template<>
struct fex_gen_type<VkImageResolve> {};
template<>
struct fex_gen_type<VkRenderPassBeginInfo> {};
template<>
struct fex_gen_type<VkPhysicalDeviceSubgroupProperties> {};
template<>
struct fex_gen_type<VkBindBufferMemoryInfo> {};
template<>
struct fex_gen_type<VkBindImageMemoryInfo> {};
template<>
struct fex_gen_type<VkPhysicalDevice16BitStorageFeatures> {};
template<>
struct fex_gen_type<VkMemoryDedicatedRequirements> {};
template<>
struct fex_gen_type<VkMemoryDedicatedAllocateInfo> {};
template<>
struct fex_gen_type<VkMemoryAllocateFlagsInfo> {};
template<>
struct fex_gen_type<VkDeviceGroupRenderPassBeginInfo> {};
template<>
struct fex_gen_type<VkDeviceGroupCommandBufferBeginInfo> {};
template<>
struct fex_gen_type<VkDeviceGroupSubmitInfo> {};
template<>
struct fex_gen_type<VkDeviceGroupBindSparseInfo> {};
template<>
struct fex_gen_type<VkBindBufferMemoryDeviceGroupInfo> {};
template<>
struct fex_gen_type<VkBindImageMemoryDeviceGroupInfo> {};
template<>
struct fex_gen_type<VkPhysicalDeviceGroupProperties> {};
template<>
struct fex_gen_type<VkDeviceGroupDeviceCreateInfo> {};
template<>
struct fex_gen_type<VkBufferMemoryRequirementsInfo2> {};
template<>
struct fex_gen_type<VkImageMemoryRequirementsInfo2> {};
template<>
struct fex_gen_type<VkImageSparseMemoryRequirementsInfo2> {};
template<>
struct fex_gen_type<VkMemoryRequirements2> {};
template<>
struct fex_gen_type<VkSparseImageMemoryRequirements2> {};
template<>
struct fex_gen_type<VkPhysicalDeviceFeatures2> {};
template<>
struct fex_gen_type<VkPhysicalDeviceProperties2> {};
template<>
struct fex_gen_type<VkFormatProperties2> {};
template<>
struct fex_gen_type<VkImageFormatProperties2> {};
template<>
struct fex_gen_type<VkPhysicalDeviceImageFormatInfo2> {};
template<>
struct fex_gen_type<VkQueueFamilyProperties2> {};
template<>
struct fex_gen_type<VkPhysicalDeviceMemoryProperties2> {};
template<>
struct fex_gen_type<VkSparseImageFormatProperties2> {};
template<>
struct fex_gen_type<VkPhysicalDeviceSparseImageFormatInfo2> {};
template<>
struct fex_gen_type<VkPhysicalDevicePointClippingProperties> {};
template<>
struct fex_gen_type<VkInputAttachmentAspectReference> {};
template<>
struct fex_gen_type<VkRenderPassInputAttachmentAspectCreateInfo> {};
template<>
struct fex_gen_type<VkImageViewUsageCreateInfo> {};
template<>
struct fex_gen_type<VkPipelineTessellationDomainOriginStateCreateInfo> {};
template<>
struct fex_gen_type<VkRenderPassMultiviewCreateInfo> {};
template<>
struct fex_gen_type<VkPhysicalDeviceMultiviewFeatures> {};
template<>
struct fex_gen_type<VkPhysicalDeviceMultiviewProperties> {};
template<>
struct fex_gen_type<VkPhysicalDeviceVariablePointersFeatures> {};
template<>
struct fex_gen_type<VkPhysicalDeviceProtectedMemoryFeatures> {};
template<>
struct fex_gen_type<VkPhysicalDeviceProtectedMemoryProperties> {};
template<>
struct fex_gen_type<VkDeviceQueueInfo2> {};
template<>
struct fex_gen_type<VkProtectedSubmitInfo> {};
template<>
struct fex_gen_type<VkSamplerYcbcrConversionCreateInfo> {};
template<>
struct fex_gen_type<VkSamplerYcbcrConversionInfo> {};
template<>
struct fex_gen_type<VkBindImagePlaneMemoryInfo> {};
template<>
struct fex_gen_type<VkImagePlaneMemoryRequirementsInfo> {};
template<>
struct fex_gen_type<VkPhysicalDeviceSamplerYcbcrConversionFeatures> {};
template<>
struct fex_gen_type<VkSamplerYcbcrConversionImageFormatProperties> {};
template<>
struct fex_gen_type<VkDescriptorUpdateTemplateEntry> {};
template<>
struct fex_gen_type<VkDescriptorUpdateTemplateCreateInfo> {};
template<>
struct fex_gen_type<VkExternalMemoryProperties> {};
template<>
struct fex_gen_type<VkPhysicalDeviceExternalImageFormatInfo> {};
template<>
struct fex_gen_type<VkExternalImageFormatProperties> {};
template<>
struct fex_gen_type<VkPhysicalDeviceExternalBufferInfo> {};
template<>
struct fex_gen_type<VkExternalBufferProperties> {};
template<>
struct fex_gen_type<VkPhysicalDeviceIDProperties> {};
template<>
struct fex_gen_type<VkExternalMemoryImageCreateInfo> {};
template<>
struct fex_gen_type<VkExternalMemoryBufferCreateInfo> {};
template<>
struct fex_gen_type<VkExportMemoryAllocateInfo> {};
template<>
struct fex_gen_type<VkPhysicalDeviceExternalFenceInfo> {};
template<>
struct fex_gen_type<VkExternalFenceProperties> {};
template<>
struct fex_gen_type<VkExportFenceCreateInfo> {};
template<>
struct fex_gen_type<VkExportSemaphoreCreateInfo> {};
template<>
struct fex_gen_type<VkPhysicalDeviceExternalSemaphoreInfo> {};
template<>
struct fex_gen_type<VkExternalSemaphoreProperties> {};
template<>
struct fex_gen_type<VkPhysicalDeviceMaintenance3Properties> {};
template<>
struct fex_gen_type<VkDescriptorSetLayoutSupport> {};
template<>
struct fex_gen_type<VkPhysicalDeviceShaderDrawParametersFeatures> {};
template<>
struct fex_gen_type<VkPhysicalDeviceVulkan11Features> {};
template<>
struct fex_gen_type<VkPhysicalDeviceVulkan11Properties> {};
template<>
struct fex_gen_type<VkPhysicalDeviceVulkan12Features> {};
template<>
struct fex_gen_type<VkConformanceVersion> {};
template<>
struct fex_gen_type<VkPhysicalDeviceVulkan12Properties> {};
template<>
struct fex_gen_type<VkImageFormatListCreateInfo> {};
template<>
struct fex_gen_type<VkAttachmentDescription2> {};
template<>
struct fex_gen_type<VkAttachmentReference2> {};
template<>
struct fex_gen_type<VkSubpassDescription2> {};
template<>
struct fex_gen_type<VkSubpassDependency2> {};
template<>
struct fex_gen_type<VkRenderPassCreateInfo2> {};
template<>
struct fex_gen_type<VkSubpassBeginInfo> {};
template<>
struct fex_gen_type<VkSubpassEndInfo> {};
template<>
struct fex_gen_type<VkPhysicalDevice8BitStorageFeatures> {};
template<>
struct fex_gen_type<VkPhysicalDeviceDriverProperties> {};
template<>
struct fex_gen_type<VkPhysicalDeviceShaderAtomicInt64Features> {};
template<>
struct fex_gen_type<VkPhysicalDeviceShaderFloat16Int8Features> {};
template<>
struct fex_gen_type<VkPhysicalDeviceFloatControlsProperties> {};
template<>
struct fex_gen_type<VkDescriptorSetLayoutBindingFlagsCreateInfo> {};
template<>
struct fex_gen_type<VkPhysicalDeviceDescriptorIndexingFeatures> {};
template<>
struct fex_gen_type<VkPhysicalDeviceDescriptorIndexingProperties> {};
template<>
struct fex_gen_type<VkDescriptorSetVariableDescriptorCountAllocateInfo> {};
template<>
struct fex_gen_type<VkDescriptorSetVariableDescriptorCountLayoutSupport> {};
template<>
struct fex_gen_type<VkSubpassDescriptionDepthStencilResolve> {};
template<>
struct fex_gen_type<VkPhysicalDeviceDepthStencilResolveProperties> {};
template<>
struct fex_gen_type<VkPhysicalDeviceScalarBlockLayoutFeatures> {};
template<>
struct fex_gen_type<VkImageStencilUsageCreateInfo> {};
template<>
struct fex_gen_type<VkSamplerReductionModeCreateInfo> {};
template<>
struct fex_gen_type<VkPhysicalDeviceSamplerFilterMinmaxProperties> {};
template<>
struct fex_gen_type<VkPhysicalDeviceVulkanMemoryModelFeatures> {};
template<>
struct fex_gen_type<VkPhysicalDeviceImagelessFramebufferFeatures> {};
template<>
struct fex_gen_type<VkFramebufferAttachmentImageInfo> {};
template<>
struct fex_gen_type<VkFramebufferAttachmentsCreateInfo> {};
template<>
struct fex_gen_type<VkRenderPassAttachmentBeginInfo> {};
template<>
struct fex_gen_type<VkPhysicalDeviceUniformBufferStandardLayoutFeatures> {};
template<>
struct fex_gen_type<VkPhysicalDeviceShaderSubgroupExtendedTypesFeatures> {};
template<>
struct fex_gen_type<VkPhysicalDeviceSeparateDepthStencilLayoutsFeatures> {};
template<>
struct fex_gen_type<VkAttachmentReferenceStencilLayout> {};
template<>
struct fex_gen_type<VkAttachmentDescriptionStencilLayout> {};
template<>
struct fex_gen_type<VkPhysicalDeviceHostQueryResetFeatures> {};
template<>
struct fex_gen_type<VkPhysicalDeviceTimelineSemaphoreFeatures> {};
template<>
struct fex_gen_type<VkPhysicalDeviceTimelineSemaphoreProperties> {};
template<>
struct fex_gen_type<VkSemaphoreTypeCreateInfo> {};
template<>
struct fex_gen_type<VkTimelineSemaphoreSubmitInfo> {};
template<>
struct fex_gen_type<VkSemaphoreWaitInfo> {};
template<>
struct fex_gen_type<VkSemaphoreSignalInfo> {};
template<>
struct fex_gen_type<VkPhysicalDeviceBufferDeviceAddressFeatures> {};
template<>
struct fex_gen_type<VkBufferDeviceAddressInfo> {};
template<>
struct fex_gen_type<VkBufferOpaqueCaptureAddressCreateInfo> {};
template<>
struct fex_gen_type<VkMemoryOpaqueCaptureAddressAllocateInfo> {};
template<>
struct fex_gen_type<VkDeviceMemoryOpaqueCaptureAddressInfo> {};
template<>
struct fex_gen_type<VkPhysicalDeviceVulkan13Features> {};
template<>
struct fex_gen_type<VkPhysicalDeviceVulkan13Properties> {};
template<>
struct fex_gen_type<VkPipelineCreationFeedback> {};
template<>
struct fex_gen_type<VkPipelineCreationFeedbackCreateInfo> {};
template<>
struct fex_gen_type<VkPhysicalDeviceShaderTerminateInvocationFeatures> {};
template<>
struct fex_gen_type<VkPhysicalDeviceToolProperties> {};
template<>
struct fex_gen_type<VkPhysicalDeviceShaderDemoteToHelperInvocationFeatures> {};
template<>
struct fex_gen_type<VkPhysicalDevicePrivateDataFeatures> {};
template<>
struct fex_gen_type<VkDevicePrivateDataCreateInfo> {};
template<>
struct fex_gen_type<VkPrivateDataSlotCreateInfo> {};
template<>
struct fex_gen_type<VkPhysicalDevicePipelineCreationCacheControlFeatures> {};
template<>
struct fex_gen_type<VkMemoryBarrier2> {};
template<>
struct fex_gen_type<VkBufferMemoryBarrier2> {};
template<>
struct fex_gen_type<VkImageMemoryBarrier2> {};
template<>
struct fex_gen_type<VkDependencyInfo> {};
template<>
struct fex_gen_type<VkSemaphoreSubmitInfo> {};
template<>
struct fex_gen_type<VkCommandBufferSubmitInfo> {};
template<>
struct fex_gen_type<VkSubmitInfo2> {};
template<>
struct fex_gen_type<VkPhysicalDeviceSynchronization2Features> {};
template<>
struct fex_gen_type<VkPhysicalDeviceZeroInitializeWorkgroupMemoryFeatures> {};
template<>
struct fex_gen_type<VkPhysicalDeviceImageRobustnessFeatures> {};
template<>
struct fex_gen_type<VkBufferCopy2> {};
template<>
struct fex_gen_type<VkCopyBufferInfo2> {};
template<>
struct fex_gen_type<VkImageCopy2> {};
template<>
struct fex_gen_type<VkCopyImageInfo2> {};
template<>
struct fex_gen_type<VkBufferImageCopy2> {};
template<>
struct fex_gen_type<VkCopyBufferToImageInfo2> {};
template<>
struct fex_gen_type<VkCopyImageToBufferInfo2> {};
template<>
struct fex_gen_type<VkImageBlit2> {};
template<>
struct fex_gen_type<VkBlitImageInfo2> {};
template<>
struct fex_gen_type<VkImageResolve2> {};
template<>
struct fex_gen_type<VkResolveImageInfo2> {};
template<>
struct fex_gen_type<VkPhysicalDeviceSubgroupSizeControlFeatures> {};
template<>
struct fex_gen_type<VkPhysicalDeviceSubgroupSizeControlProperties> {};
template<>
struct fex_gen_type<VkPipelineShaderStageRequiredSubgroupSizeCreateInfo> {};
template<>
struct fex_gen_type<VkPhysicalDeviceInlineUniformBlockFeatures> {};
template<>
struct fex_gen_type<VkPhysicalDeviceInlineUniformBlockProperties> {};
template<>
struct fex_gen_type<VkWriteDescriptorSetInlineUniformBlock> {};
template<>
struct fex_gen_type<VkDescriptorPoolInlineUniformBlockCreateInfo> {};
template<>
struct fex_gen_type<VkPhysicalDeviceTextureCompressionASTCHDRFeatures> {};
template<>
struct fex_gen_type<VkRenderingAttachmentInfo> {};
template<>
struct fex_gen_type<VkRenderingInfo> {};
template<>
struct fex_gen_type<VkPipelineRenderingCreateInfo> {};
template<>
struct fex_gen_type<VkPhysicalDeviceDynamicRenderingFeatures> {};
template<>
struct fex_gen_type<VkCommandBufferInheritanceRenderingInfo> {};
template<>
struct fex_gen_type<VkPhysicalDeviceShaderIntegerDotProductFeatures> {};
template<>
struct fex_gen_type<VkPhysicalDeviceShaderIntegerDotProductProperties> {};
template<>
struct fex_gen_type<VkPhysicalDeviceTexelBufferAlignmentProperties> {};
template<>
struct fex_gen_type<VkFormatProperties3> {};
template<>
struct fex_gen_type<VkPhysicalDeviceMaintenance4Features> {};
template<>
struct fex_gen_type<VkPhysicalDeviceMaintenance4Properties> {};
template<>
struct fex_gen_type<VkDeviceBufferMemoryRequirements> {};
template<>
struct fex_gen_type<VkDeviceImageMemoryRequirements> {};
template<>
struct fex_gen_type<VkPhysicalDeviceVulkan14Features> {};
template<>
struct fex_gen_type<VkPhysicalDeviceVulkan14Properties> {};
template<>
struct fex_gen_type<VkDeviceQueueGlobalPriorityCreateInfo> {};
template<>
struct fex_gen_type<VkPhysicalDeviceGlobalPriorityQueryFeatures> {};
template<>
struct fex_gen_type<VkQueueFamilyGlobalPriorityProperties> {};
template<>
struct fex_gen_type<VkPhysicalDeviceShaderSubgroupRotateFeatures> {};
template<>
struct fex_gen_type<VkPhysicalDeviceShaderFloatControls2Features> {};
template<>
struct fex_gen_type<VkPhysicalDeviceShaderExpectAssumeFeatures> {};
template<>
struct fex_gen_type<VkPhysicalDeviceLineRasterizationFeatures> {};
template<>
struct fex_gen_type<VkPhysicalDeviceLineRasterizationProperties> {};
template<>
struct fex_gen_type<VkPipelineRasterizationLineStateCreateInfo> {};
template<>
struct fex_gen_type<VkPhysicalDeviceVertexAttributeDivisorProperties> {};
template<>
struct fex_gen_type<VkVertexInputBindingDivisorDescription> {};
template<>
struct fex_gen_type<VkPipelineVertexInputDivisorStateCreateInfo> {};
template<>
struct fex_gen_type<VkPhysicalDeviceVertexAttributeDivisorFeatures> {};
template<>
struct fex_gen_type<VkPhysicalDeviceIndexTypeUint8Features> {};
template<>
struct fex_gen_type<VkMemoryMapInfo> {};
template<>
struct fex_gen_type<VkMemoryUnmapInfo> {};
template<>
struct fex_gen_type<VkPhysicalDeviceMaintenance5Features> {};
template<>
struct fex_gen_type<VkPhysicalDeviceMaintenance5Properties> {};
template<>
struct fex_gen_type<VkRenderingAreaInfo> {};
template<>
struct fex_gen_type<VkImageSubresource2> {};
template<>
struct fex_gen_type<VkDeviceImageSubresourceInfo> {};
template<>
struct fex_gen_type<VkSubresourceLayout2> {};
template<>
struct fex_gen_type<VkPipelineCreateFlags2CreateInfo> {};
template<>
struct fex_gen_type<VkBufferUsageFlags2CreateInfo> {};
template<>
struct fex_gen_type<VkPhysicalDevicePushDescriptorProperties> {};
template<>
struct fex_gen_type<VkPhysicalDeviceDynamicRenderingLocalReadFeatures> {};
template<>
struct fex_gen_type<VkRenderingAttachmentLocationInfo> {};
template<>
struct fex_gen_type<VkRenderingInputAttachmentIndexInfo> {};
template<>
struct fex_gen_type<VkPhysicalDeviceMaintenance6Features> {};
template<>
struct fex_gen_type<VkPhysicalDeviceMaintenance6Properties> {};
template<>
struct fex_gen_type<VkBindMemoryStatus> {};
template<>
struct fex_gen_type<VkBindDescriptorSetsInfo> {};
template<>
struct fex_gen_type<VkPushConstantsInfo> {};
template<>
struct fex_gen_type<VkPushDescriptorSetInfo> {};
template<>
struct fex_gen_type<VkPushDescriptorSetWithTemplateInfo> {};
template<>
struct fex_gen_type<VkPhysicalDevicePipelineProtectedAccessFeatures> {};
template<>
struct fex_gen_type<VkPhysicalDevicePipelineRobustnessFeatures> {};
template<>
struct fex_gen_type<VkPhysicalDevicePipelineRobustnessProperties> {};
template<>
struct fex_gen_type<VkPipelineRobustnessCreateInfo> {};
template<>
struct fex_gen_type<VkPhysicalDeviceHostImageCopyFeatures> {};
template<>
struct fex_gen_type<VkPhysicalDeviceHostImageCopyProperties> {};
template<>
struct fex_gen_type<VkMemoryToImageCopy> {};
template<>
struct fex_gen_type<VkImageToMemoryCopy> {};
template<>
struct fex_gen_type<VkCopyMemoryToImageInfo> {};
template<>
struct fex_gen_type<VkCopyImageToMemoryInfo> {};
template<>
struct fex_gen_type<VkCopyImageToImageInfo> {};
template<>
struct fex_gen_type<VkHostImageLayoutTransitionInfo> {};
template<>
struct fex_gen_type<VkSubresourceHostMemcpySize> {};
template<>
struct fex_gen_type<VkHostImageCopyDevicePerformanceQuery> {};
template<>
struct fex_gen_type<VkSurfaceCapabilitiesKHR> {};
template<>
struct fex_gen_type<VkSurfaceFormatKHR> {};
template<>
struct fex_gen_type<VkSwapchainCreateInfoKHR> {};
template<>
struct fex_gen_type<VkPresentInfoKHR> {};
template<>
struct fex_gen_type<VkImageSwapchainCreateInfoKHR> {};
template<>
struct fex_gen_type<VkBindImageMemorySwapchainInfoKHR> {};
template<>
struct fex_gen_type<VkAcquireNextImageInfoKHR> {};
template<>
struct fex_gen_type<VkDeviceGroupPresentCapabilitiesKHR> {};
template<>
struct fex_gen_type<VkDeviceGroupPresentInfoKHR> {};
template<>
struct fex_gen_type<VkDeviceGroupSwapchainCreateInfoKHR> {};
template<>
struct fex_gen_type<VkDisplayModeParametersKHR> {};
template<>
struct fex_gen_type<VkDisplayModeCreateInfoKHR> {};
template<>
struct fex_gen_type<VkDisplayModePropertiesKHR> {};
template<>
struct fex_gen_type<VkDisplayPlaneCapabilitiesKHR> {};
template<>
struct fex_gen_type<VkDisplayPlanePropertiesKHR> {};
template<>
struct fex_gen_type<VkDisplayPropertiesKHR> {};
template<>
struct fex_gen_type<VkDisplaySurfaceCreateInfoKHR> {};
template<>
struct fex_gen_type<VkDisplayPresentInfoKHR> {};
template<>
struct fex_gen_type<VkQueueFamilyQueryResultStatusPropertiesKHR> {};
template<>
struct fex_gen_type<VkQueueFamilyVideoPropertiesKHR> {};
template<>
struct fex_gen_type<VkPhysicalDeviceVideoFormatInfoKHR> {};
template<>
struct fex_gen_type<VkBindVideoSessionMemoryInfoKHR> {};
template<>
struct fex_gen_type<VkImportMemoryFdInfoKHR> {};
template<>
struct fex_gen_type<VkMemoryFdPropertiesKHR> {};
template<>
struct fex_gen_type<VkMemoryGetFdInfoKHR> {};
template<>
struct fex_gen_type<VkImportSemaphoreFdInfoKHR> {};
template<>
struct fex_gen_type<VkSemaphoreGetFdInfoKHR> {};
template<>
struct fex_gen_type<VkRectLayerKHR> {};
template<>
struct fex_gen_type<VkPresentRegionKHR> {};
template<>
struct fex_gen_type<VkPresentRegionsKHR> {};
template<>
struct fex_gen_type<VkSharedPresentSurfaceCapabilitiesKHR> {};
template<>
struct fex_gen_type<VkImportFenceFdInfoKHR> {};
template<>
struct fex_gen_type<VkFenceGetFdInfoKHR> {};
template<>
struct fex_gen_type<VkPhysicalDevicePerformanceQueryFeaturesKHR> {};
template<>
struct fex_gen_type<VkPhysicalDevicePerformanceQueryPropertiesKHR> {};
template<>
struct fex_gen_type<VkPerformanceCounterKHR> {};
template<>
struct fex_gen_type<VkPerformanceCounterDescriptionKHR> {};
template<>
struct fex_gen_type<VkQueryPoolPerformanceCreateInfoKHR> {};
template<>
struct fex_gen_type<VkAcquireProfilingLockInfoKHR> {};
template<>
struct fex_gen_type<VkPerformanceQuerySubmitInfoKHR> {};
template<>
struct fex_gen_type<VkPhysicalDeviceSurfaceInfo2KHR> {};
template<>
struct fex_gen_type<VkSurfaceCapabilities2KHR> {};
template<>
struct fex_gen_type<VkSurfaceFormat2KHR> {};
template<>
struct fex_gen_type<VkDisplayProperties2KHR> {};
template<>
struct fex_gen_type<VkDisplayPlaneProperties2KHR> {};
template<>
struct fex_gen_type<VkDisplayModeProperties2KHR> {};
template<>
struct fex_gen_type<VkDisplayPlaneInfo2KHR> {};
template<>
struct fex_gen_type<VkDisplayPlaneCapabilities2KHR> {};
template<>
struct fex_gen_type<VkPhysicalDeviceShaderClockFeaturesKHR> {};
template<>
struct fex_gen_type<VkFragmentShadingRateAttachmentInfoKHR> {};
template<>
struct fex_gen_type<VkPipelineFragmentShadingRateStateCreateInfoKHR> {};
template<>
struct fex_gen_type<VkPhysicalDeviceFragmentShadingRateFeaturesKHR> {};
template<>
struct fex_gen_type<VkPhysicalDeviceFragmentShadingRatePropertiesKHR> {};
template<>
struct fex_gen_type<VkPhysicalDeviceFragmentShadingRateKHR> {};
template<>
struct fex_gen_type<VkRenderingFragmentShadingRateAttachmentInfoKHR> {};
template<>
struct fex_gen_type<VkPhysicalDeviceShaderQuadControlFeaturesKHR> {};
template<>
struct fex_gen_type<VkSurfaceProtectedCapabilitiesKHR> {};
template<>
struct fex_gen_type<VkPhysicalDevicePresentWaitFeaturesKHR> {};
template<>
struct fex_gen_type<VkPhysicalDevicePipelineExecutablePropertiesFeaturesKHR> {};
template<>
struct fex_gen_type<VkPipelineInfoKHR> {};
template<>
struct fex_gen_type<VkPipelineExecutablePropertiesKHR> {};
template<>
struct fex_gen_type<VkPipelineExecutableInfoKHR> {};
template<>
struct fex_gen_type<VkPipelineExecutableStatisticValueKHR> : fexgen::assume_compatible_data_layout {};
template<>
struct fex_gen_type<VkPipelineExecutableStatisticKHR> {};
template<>
struct fex_gen_type<VkPipelineExecutableInternalRepresentationKHR> {};
template<>
struct fex_gen_type<VkPipelineLibraryCreateInfoKHR> {};
template<>
struct fex_gen_type<VkPresentIdKHR> {};
template<>
struct fex_gen_type<VkPhysicalDevicePresentIdFeaturesKHR> {};
template<>
struct fex_gen_type<VkQueryPoolVideoEncodeFeedbackCreateInfoKHR> {};
template<>
struct fex_gen_type<VkPhysicalDeviceVideoEncodeQualityLevelInfoKHR> {};
template<>
struct fex_gen_type<VkPhysicalDeviceFragmentShaderBarycentricFeaturesKHR> {};
template<>
struct fex_gen_type<VkPhysicalDeviceFragmentShaderBarycentricPropertiesKHR> {};
template<>
struct fex_gen_type<VkPhysicalDeviceShaderSubgroupUniformControlFlowFeaturesKHR> {};
template<>
struct fex_gen_type<VkPhysicalDeviceWorkgroupMemoryExplicitLayoutFeaturesKHR> {};
template<>
struct fex_gen_type<VkPhysicalDeviceRayTracingMaintenance1FeaturesKHR> {};
template<>
struct fex_gen_type<VkTraceRaysIndirectCommand2KHR> {};
template<>
struct fex_gen_type<VkPhysicalDeviceShaderMaximalReconvergenceFeaturesKHR> {};
template<>
struct fex_gen_type<VkPhysicalDeviceRayTracingPositionFetchFeaturesKHR> {};
template<>
struct fex_gen_type<VkPhysicalDevicePipelineBinaryFeaturesKHR> {};
template<>
struct fex_gen_type<VkPhysicalDevicePipelineBinaryPropertiesKHR> {};
template<>
struct fex_gen_type<VkDevicePipelineBinaryInternalCacheControlKHR> {};
template<>
struct fex_gen_type<VkPipelineBinaryKeyKHR> {};
template<>
struct fex_gen_type<VkPipelineBinaryDataKHR> {};
template<>
struct fex_gen_type<VkPipelineBinaryKeysAndDataKHR> {};
template<>
struct fex_gen_type<VkPipelineCreateInfoKHR> {};
template<>
struct fex_gen_type<VkPipelineBinaryCreateInfoKHR> {};
template<>
struct fex_gen_type<VkPipelineBinaryInfoKHR> {};
template<>
struct fex_gen_type<VkReleaseCapturedPipelineDataInfoKHR> {};
template<>
struct fex_gen_type<VkPipelineBinaryDataInfoKHR> {};
template<>
struct fex_gen_type<VkPipelineBinaryHandlesInfoKHR> {};
template<>
struct fex_gen_type<VkCooperativeMatrixPropertiesKHR> {};
template<>
struct fex_gen_type<VkPhysicalDeviceCooperativeMatrixFeaturesKHR> {};
template<>
struct fex_gen_type<VkPhysicalDeviceCooperativeMatrixPropertiesKHR> {};
template<>
struct fex_gen_type<VkPhysicalDeviceComputeShaderDerivativesFeaturesKHR> {};
template<>
struct fex_gen_type<VkPhysicalDeviceComputeShaderDerivativesPropertiesKHR> {};
template<>
struct fex_gen_type<VkPhysicalDeviceVideoEncodeAV1FeaturesKHR> {};
template<>
struct fex_gen_type<VkPhysicalDeviceVideoMaintenance1FeaturesKHR> {};
template<>
struct fex_gen_type<VkCalibratedTimestampInfoKHR> {};
template<>
struct fex_gen_type<VkSetDescriptorBufferOffsetsInfoEXT> {};
template<>
struct fex_gen_type<VkBindDescriptorBufferEmbeddedSamplersInfoEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceVideoEncodeQuantizationMapFeaturesKHR> {};
template<>
struct fex_gen_type<VkPhysicalDeviceShaderRelaxedExtendedInstructionFeaturesKHR> {};
template<>
struct fex_gen_type<VkPhysicalDeviceMaintenance7FeaturesKHR> {};
template<>
struct fex_gen_type<VkPhysicalDeviceMaintenance7PropertiesKHR> {};
template<>
struct fex_gen_type<VkPhysicalDeviceLayeredApiPropertiesKHR> {};
template<>
struct fex_gen_type<VkPhysicalDeviceLayeredApiPropertiesListKHR> {};
template<>
struct fex_gen_type<VkPhysicalDeviceLayeredApiVulkanPropertiesKHR> {};
template<>
struct fex_gen_type<VkPhysicalDeviceMaintenance8FeaturesKHR> {};
template<>
struct fex_gen_type<VkMemoryBarrierAccessFlags3KHR> {};
template<>
struct fex_gen_type<VkPhysicalDeviceVideoMaintenance2FeaturesKHR> {};
template<>
struct fex_gen_type<VkPhysicalDeviceDepthClampZeroOneFeaturesKHR> {};
// Structures that contain function pointers
// TODO: Use custom repacking for these instead
template<>
struct fex_gen_type<VkDebugReportCallbackCreateInfoEXT> : fexgen::emit_layout_wrappers {};
template<>
struct fex_gen_type<VkPipelineRasterizationStateRasterizationOrderAMD> {};
template<>
struct fex_gen_type<VkDebugMarkerObjectNameInfoEXT> {};
template<>
struct fex_gen_type<VkDebugMarkerObjectTagInfoEXT> {};
template<>
struct fex_gen_type<VkDebugMarkerMarkerInfoEXT> {};
template<>
struct fex_gen_type<VkDedicatedAllocationImageCreateInfoNV> {};
template<>
struct fex_gen_type<VkDedicatedAllocationBufferCreateInfoNV> {};
template<>
struct fex_gen_type<VkDedicatedAllocationMemoryAllocateInfoNV> {};
template<>
struct fex_gen_type<VkPhysicalDeviceTransformFeedbackFeaturesEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceTransformFeedbackPropertiesEXT> {};
template<>
struct fex_gen_type<VkPipelineRasterizationStateStreamCreateInfoEXT> {};
template<>
struct fex_gen_type<VkCuModuleCreateInfoNVX> {};
template<>
struct fex_gen_type<VkCuModuleTexturingModeCreateInfoNVX> {};
template<>
struct fex_gen_type<VkCuFunctionCreateInfoNVX> {};
template<>
struct fex_gen_type<VkCuLaunchInfoNVX> {};
template<>
struct fex_gen_type<VkImageViewHandleInfoNVX> {};
template<>
struct fex_gen_type<VkImageViewAddressPropertiesNVX> {};
template<>
struct fex_gen_type<VkTextureLODGatherFormatPropertiesAMD> {};
template<>
struct fex_gen_type<VkShaderResourceUsageAMD> {};
template<>
struct fex_gen_type<VkShaderStatisticsInfoAMD> {};
template<>
struct fex_gen_type<VkPhysicalDeviceCornerSampledImageFeaturesNV> {};
template<>
struct fex_gen_type<VkExternalImageFormatPropertiesNV> {};
template<>
struct fex_gen_type<VkExternalMemoryImageCreateInfoNV> {};
template<>
struct fex_gen_type<VkExportMemoryAllocateInfoNV> {};
template<>
struct fex_gen_type<VkValidationFlagsEXT> {};
template<>
struct fex_gen_type<VkImageViewASTCDecodeModeEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceASTCDecodeFeaturesEXT> {};
template<>
struct fex_gen_type<VkConditionalRenderingBeginInfoEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceConditionalRenderingFeaturesEXT> {};
template<>
struct fex_gen_type<VkCommandBufferInheritanceConditionalRenderingInfoEXT> {};
template<>
struct fex_gen_type<VkViewportWScalingNV> {};
template<>
struct fex_gen_type<VkPipelineViewportWScalingStateCreateInfoNV> {};
template<>
struct fex_gen_type<VkSurfaceCapabilities2EXT> {};
template<>
struct fex_gen_type<VkDisplayPowerInfoEXT> {};
template<>
struct fex_gen_type<VkDeviceEventInfoEXT> {};
template<>
struct fex_gen_type<VkDisplayEventInfoEXT> {};
template<>
struct fex_gen_type<VkSwapchainCounterCreateInfoEXT> {};
template<>
struct fex_gen_type<VkRefreshCycleDurationGOOGLE> {};
template<>
struct fex_gen_type<VkPastPresentationTimingGOOGLE> {};
template<>
struct fex_gen_type<VkPresentTimeGOOGLE> {};
template<>
struct fex_gen_type<VkPresentTimesInfoGOOGLE> {};
template<>
struct fex_gen_type<VkPhysicalDeviceMultiviewPerViewAttributesPropertiesNVX> {};
template<>
struct fex_gen_type<VkMultiviewPerViewAttributesInfoNVX> {};
template<>
struct fex_gen_type<VkViewportSwizzleNV> {};
template<>
struct fex_gen_type<VkPipelineViewportSwizzleStateCreateInfoNV> {};
template<>
struct fex_gen_type<VkPhysicalDeviceDiscardRectanglePropertiesEXT> {};
template<>
struct fex_gen_type<VkPipelineDiscardRectangleStateCreateInfoEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceConservativeRasterizationPropertiesEXT> {};
template<>
struct fex_gen_type<VkPipelineRasterizationConservativeStateCreateInfoEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceDepthClipEnableFeaturesEXT> {};
template<>
struct fex_gen_type<VkPipelineRasterizationDepthClipStateCreateInfoEXT> {};
template<>
struct fex_gen_type<VkXYColorEXT> {};
template<>
struct fex_gen_type<VkHdrMetadataEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceRelaxedLineRasterizationFeaturesIMG> {};
template<>
struct fex_gen_type<VkDebugUtilsLabelEXT> {};
template<>
struct fex_gen_type<VkDebugUtilsObjectNameInfoEXT> {};
template<>
struct fex_gen_type<VkDebugUtilsMessengerCallbackDataEXT> {};
// Structures that contain function pointers
// TODO: Use custom repacking for these instead
template<>
struct fex_gen_type<VkDebugUtilsMessengerCreateInfoEXT> : fexgen::emit_layout_wrappers {};
template<>
struct fex_gen_type<VkDebugUtilsObjectTagInfoEXT> {};
template<>
struct fex_gen_type<VkAttachmentSampleCountInfoAMD> {};
template<>
struct fex_gen_type<VkSampleLocationEXT> {};
template<>
struct fex_gen_type<VkSampleLocationsInfoEXT> {};
template<>
struct fex_gen_type<VkAttachmentSampleLocationsEXT> {};
template<>
struct fex_gen_type<VkSubpassSampleLocationsEXT> {};
template<>
struct fex_gen_type<VkRenderPassSampleLocationsBeginInfoEXT> {};
template<>
struct fex_gen_type<VkPipelineSampleLocationsStateCreateInfoEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceSampleLocationsPropertiesEXT> {};
template<>
struct fex_gen_type<VkMultisamplePropertiesEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceBlendOperationAdvancedFeaturesEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceBlendOperationAdvancedPropertiesEXT> {};
template<>
struct fex_gen_type<VkPipelineColorBlendAdvancedStateCreateInfoEXT> {};
template<>
struct fex_gen_type<VkPipelineCoverageToColorStateCreateInfoNV> {};
template<>
struct fex_gen_type<VkPipelineCoverageModulationStateCreateInfoNV> {};
template<>
struct fex_gen_type<VkPhysicalDeviceShaderSMBuiltinsPropertiesNV> {};
template<>
struct fex_gen_type<VkPhysicalDeviceShaderSMBuiltinsFeaturesNV> {};
template<>
struct fex_gen_type<VkDrmFormatModifierPropertiesEXT> {};
template<>
struct fex_gen_type<VkDrmFormatModifierPropertiesListEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceImageDrmFormatModifierInfoEXT> {};
template<>
struct fex_gen_type<VkImageDrmFormatModifierListCreateInfoEXT> {};
template<>
struct fex_gen_type<VkImageDrmFormatModifierExplicitCreateInfoEXT> {};
template<>
struct fex_gen_type<VkImageDrmFormatModifierPropertiesEXT> {};
template<>
struct fex_gen_type<VkDrmFormatModifierProperties2EXT> {};
template<>
struct fex_gen_type<VkDrmFormatModifierPropertiesList2EXT> {};
template<>
struct fex_gen_type<VkValidationCacheCreateInfoEXT> {};
template<>
struct fex_gen_type<VkShaderModuleValidationCacheCreateInfoEXT> {};
template<>
struct fex_gen_type<VkShadingRatePaletteNV> {};
template<>
struct fex_gen_type<VkPipelineViewportShadingRateImageStateCreateInfoNV> {};
template<>
struct fex_gen_type<VkPhysicalDeviceShadingRateImageFeaturesNV> {};
template<>
struct fex_gen_type<VkPhysicalDeviceShadingRateImagePropertiesNV> {};
template<>
struct fex_gen_type<VkCoarseSampleLocationNV> {};
template<>
struct fex_gen_type<VkCoarseSampleOrderCustomNV> {};
template<>
struct fex_gen_type<VkPipelineViewportCoarseSampleOrderStateCreateInfoNV> {};
template<>
struct fex_gen_type<VkRayTracingShaderGroupCreateInfoNV> {};
template<>
struct fex_gen_type<VkRayTracingPipelineCreateInfoNV> {};
template<>
struct fex_gen_type<VkGeometryTrianglesNV> {};
template<>
struct fex_gen_type<VkGeometryAABBNV> {};
template<>
struct fex_gen_type<VkGeometryDataNV> {};
template<>
struct fex_gen_type<VkGeometryNV> {};
template<>
struct fex_gen_type<VkAccelerationStructureInfoNV> {};
template<>
struct fex_gen_type<VkAccelerationStructureCreateInfoNV> {};
template<>
struct fex_gen_type<VkBindAccelerationStructureMemoryInfoNV> {};
template<>
struct fex_gen_type<VkWriteDescriptorSetAccelerationStructureNV> {};
template<>
struct fex_gen_type<VkAccelerationStructureMemoryRequirementsInfoNV> {};
template<>
struct fex_gen_type<VkPhysicalDeviceRayTracingPropertiesNV> {};
template<>
struct fex_gen_type<VkAabbPositionsKHR> {};
template<>
struct fex_gen_type<VkPhysicalDeviceRepresentativeFragmentTestFeaturesNV> {};
template<>
struct fex_gen_type<VkPipelineRepresentativeFragmentTestStateCreateInfoNV> {};
template<>
struct fex_gen_type<VkPhysicalDeviceImageViewImageFormatInfoEXT> {};
template<>
struct fex_gen_type<VkFilterCubicImageViewImageFormatPropertiesEXT> {};
template<>
struct fex_gen_type<VkImportMemoryHostPointerInfoEXT> {};
template<>
struct fex_gen_type<VkMemoryHostPointerPropertiesEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceExternalMemoryHostPropertiesEXT> {};
template<>
struct fex_gen_type<VkPipelineCompilerControlCreateInfoAMD> {};
template<>
struct fex_gen_type<VkPhysicalDeviceShaderCorePropertiesAMD> {};
template<>
struct fex_gen_type<VkDeviceMemoryOverallocationCreateInfoAMD> {};
template<>
struct fex_gen_type<VkPhysicalDeviceVertexAttributeDivisorPropertiesEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceMeshShaderFeaturesNV> {};
template<>
struct fex_gen_type<VkPhysicalDeviceMeshShaderPropertiesNV> {};
template<>
struct fex_gen_type<VkDrawMeshTasksIndirectCommandNV> {};
template<>
struct fex_gen_type<VkPhysicalDeviceShaderImageFootprintFeaturesNV> {};
template<>
struct fex_gen_type<VkPipelineViewportExclusiveScissorStateCreateInfoNV> {};
template<>
struct fex_gen_type<VkPhysicalDeviceExclusiveScissorFeaturesNV> {};
template<>
struct fex_gen_type<VkQueueFamilyCheckpointPropertiesNV> {};
template<>
struct fex_gen_type<VkCheckpointDataNV> {};
template<>
struct fex_gen_type<VkQueueFamilyCheckpointProperties2NV> {};
template<>
struct fex_gen_type<VkCheckpointData2NV> {};
template<>
struct fex_gen_type<VkPhysicalDeviceShaderIntegerFunctions2FeaturesINTEL> {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_type<VkPerformanceValueDataINTEL> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_type<VkPerformanceValueDataINTEL> : fexgen::assume_compatible_data_layout {};
#endif
template<>
struct fex_gen_type<VkInitializePerformanceApiInfoINTEL> {};
template<>
struct fex_gen_type<VkQueryPoolPerformanceQueryCreateInfoINTEL> {};
template<>
struct fex_gen_type<VkPerformanceMarkerInfoINTEL> {};
template<>
struct fex_gen_type<VkPerformanceStreamMarkerInfoINTEL> {};
template<>
struct fex_gen_type<VkPerformanceOverrideInfoINTEL> {};
template<>
struct fex_gen_type<VkPerformanceConfigurationAcquireInfoINTEL> {};
template<>
struct fex_gen_type<VkPhysicalDevicePCIBusInfoPropertiesEXT> {};
template<>
struct fex_gen_type<VkDisplayNativeHdrSurfaceCapabilitiesAMD> {};
template<>
struct fex_gen_type<VkSwapchainDisplayNativeHdrCreateInfoAMD> {};
template<>
struct fex_gen_type<VkPhysicalDeviceFragmentDensityMapFeaturesEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceFragmentDensityMapPropertiesEXT> {};
template<>
struct fex_gen_type<VkRenderPassFragmentDensityMapCreateInfoEXT> {};
template<>
struct fex_gen_type<VkRenderingFragmentDensityMapAttachmentInfoEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceShaderCoreProperties2AMD> {};
template<>
struct fex_gen_type<VkPhysicalDeviceCoherentMemoryFeaturesAMD> {};
template<>
struct fex_gen_type<VkPhysicalDeviceShaderImageAtomicInt64FeaturesEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceMemoryBudgetPropertiesEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceMemoryPriorityFeaturesEXT> {};
template<>
struct fex_gen_type<VkMemoryPriorityAllocateInfoEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceDedicatedAllocationImageAliasingFeaturesNV> {};
template<>
struct fex_gen_type<VkPhysicalDeviceBufferDeviceAddressFeaturesEXT> {};
template<>
struct fex_gen_type<VkBufferDeviceAddressCreateInfoEXT> {};
template<>
struct fex_gen_type<VkValidationFeaturesEXT> {};
template<>
struct fex_gen_type<VkCooperativeMatrixPropertiesNV> {};
template<>
struct fex_gen_type<VkPhysicalDeviceCooperativeMatrixFeaturesNV> {};
template<>
struct fex_gen_type<VkPhysicalDeviceCooperativeMatrixPropertiesNV> {};
template<>
struct fex_gen_type<VkPhysicalDeviceCoverageReductionModeFeaturesNV> {};
template<>
struct fex_gen_type<VkPipelineCoverageReductionStateCreateInfoNV> {};
template<>
struct fex_gen_type<VkFramebufferMixedSamplesCombinationNV> {};
template<>
struct fex_gen_type<VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceYcbcrImageArraysFeaturesEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceProvokingVertexFeaturesEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceProvokingVertexPropertiesEXT> {};
template<>
struct fex_gen_type<VkPipelineRasterizationProvokingVertexStateCreateInfoEXT> {};
template<>
struct fex_gen_type<VkHeadlessSurfaceCreateInfoEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceShaderAtomicFloatFeaturesEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceExtendedDynamicStateFeaturesEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceMapMemoryPlacedFeaturesEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceMapMemoryPlacedPropertiesEXT> {};
template<>
struct fex_gen_type<VkMemoryMapPlacedInfoEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceShaderAtomicFloat2FeaturesEXT> {};
template<>
struct fex_gen_type<VkSurfacePresentModeEXT> {};
template<>
struct fex_gen_type<VkSurfacePresentScalingCapabilitiesEXT> {};
template<>
struct fex_gen_type<VkSurfacePresentModeCompatibilityEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT> {};
template<>
struct fex_gen_type<VkSwapchainPresentFenceInfoEXT> {};
template<>
struct fex_gen_type<VkSwapchainPresentModesCreateInfoEXT> {};
template<>
struct fex_gen_type<VkSwapchainPresentModeInfoEXT> {};
template<>
struct fex_gen_type<VkSwapchainPresentScalingCreateInfoEXT> {};
template<>
struct fex_gen_type<VkReleaseSwapchainImagesInfoEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceDeviceGeneratedCommandsPropertiesNV> {};
template<>
struct fex_gen_type<VkPhysicalDeviceDeviceGeneratedCommandsFeaturesNV> {};
template<>
struct fex_gen_type<VkGraphicsShaderGroupCreateInfoNV> {};
template<>
struct fex_gen_type<VkGraphicsPipelineShaderGroupsCreateInfoNV> {};
template<>
struct fex_gen_type<VkBindShaderGroupIndirectCommandNV> {};
template<>
struct fex_gen_type<VkBindIndexBufferIndirectCommandNV> {};
template<>
struct fex_gen_type<VkBindVertexBufferIndirectCommandNV> {};
template<>
struct fex_gen_type<VkSetStateFlagsIndirectCommandNV> {};
template<>
struct fex_gen_type<VkIndirectCommandsStreamNV> {};
template<>
struct fex_gen_type<VkIndirectCommandsLayoutTokenNV> {};
template<>
struct fex_gen_type<VkIndirectCommandsLayoutCreateInfoNV> {};
template<>
struct fex_gen_type<VkGeneratedCommandsInfoNV> {};
template<>
struct fex_gen_type<VkGeneratedCommandsMemoryRequirementsInfoNV> {};
template<>
struct fex_gen_type<VkPhysicalDeviceInheritedViewportScissorFeaturesNV> {};
template<>
struct fex_gen_type<VkCommandBufferInheritanceViewportScissorInfoNV> {};
template<>
struct fex_gen_type<VkPhysicalDeviceTexelBufferAlignmentFeaturesEXT> {};
template<>
struct fex_gen_type<VkRenderPassTransformBeginInfoQCOM> {};
template<>
struct fex_gen_type<VkCommandBufferInheritanceRenderPassTransformInfoQCOM> {};
template<>
struct fex_gen_type<VkPhysicalDeviceDepthBiasControlFeaturesEXT> {};
template<>
struct fex_gen_type<VkDepthBiasInfoEXT> {};
template<>
struct fex_gen_type<VkDepthBiasRepresentationInfoEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceDeviceMemoryReportFeaturesEXT> {};
template<>
struct fex_gen_type<VkDeviceMemoryReportCallbackDataEXT> {};
template<>
struct fex_gen_type<VkDeviceDeviceMemoryReportCreateInfoEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceRobustness2FeaturesEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceRobustness2PropertiesEXT> {};
template<>
struct fex_gen_type<VkSamplerCustomBorderColorCreateInfoEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceCustomBorderColorPropertiesEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceCustomBorderColorFeaturesEXT> {};
template<>
struct fex_gen_type<VkPhysicalDevicePresentBarrierFeaturesNV> {};
template<>
struct fex_gen_type<VkSurfaceCapabilitiesPresentBarrierNV> {};
template<>
struct fex_gen_type<VkSwapchainPresentBarrierCreateInfoNV> {};
template<>
struct fex_gen_type<VkPhysicalDeviceDiagnosticsConfigFeaturesNV> {};
template<>
struct fex_gen_type<VkDeviceDiagnosticsConfigCreateInfoNV> {};
template<>
struct fex_gen_type<VkCudaModuleCreateInfoNV> {};
template<>
struct fex_gen_type<VkCudaFunctionCreateInfoNV> {};
template<>
struct fex_gen_type<VkCudaLaunchInfoNV> {};
template<>
struct fex_gen_type<VkPhysicalDeviceCudaKernelLaunchFeaturesNV> {};
template<>
struct fex_gen_type<VkPhysicalDeviceCudaKernelLaunchPropertiesNV> {};
template<>
struct fex_gen_type<VkQueryLowLatencySupportNV> {};
template<>
struct fex_gen_type<VkPhysicalDeviceDescriptorBufferPropertiesEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceDescriptorBufferDensityMapPropertiesEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceDescriptorBufferFeaturesEXT> {};
template<>
struct fex_gen_type<VkDescriptorAddressInfoEXT> {};
template<>
struct fex_gen_type<VkDescriptorBufferBindingInfoEXT> {};
template<>
struct fex_gen_type<VkDescriptorBufferBindingPushDescriptorBufferHandleEXT> {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_type<VkDescriptorDataEXT> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_type<VkDescriptorDataEXT> : fexgen::assume_compatible_data_layout {};
#endif
template<>
struct fex_gen_type<VkDescriptorGetInfoEXT> {};
template<>
struct fex_gen_type<VkBufferCaptureDescriptorDataInfoEXT> {};
template<>
struct fex_gen_type<VkImageCaptureDescriptorDataInfoEXT> {};
template<>
struct fex_gen_type<VkImageViewCaptureDescriptorDataInfoEXT> {};
template<>
struct fex_gen_type<VkSamplerCaptureDescriptorDataInfoEXT> {};
template<>
struct fex_gen_type<VkOpaqueCaptureDescriptorDataCreateInfoEXT> {};
template<>
struct fex_gen_type<VkAccelerationStructureCaptureDescriptorDataInfoEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceGraphicsPipelineLibraryFeaturesEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceGraphicsPipelineLibraryPropertiesEXT> {};
template<>
struct fex_gen_type<VkGraphicsPipelineLibraryCreateInfoEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceShaderEarlyAndLateFragmentTestsFeaturesAMD> {};
template<>
struct fex_gen_type<VkPhysicalDeviceFragmentShadingRateEnumsFeaturesNV> {};
template<>
struct fex_gen_type<VkPhysicalDeviceFragmentShadingRateEnumsPropertiesNV> {};
template<>
struct fex_gen_type<VkPipelineFragmentShadingRateEnumStateCreateInfoNV> {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_type<VkDeviceOrHostAddressConstKHR> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_type<VkDeviceOrHostAddressConstKHR> : fexgen::assume_compatible_data_layout {};
#endif
template<>
struct fex_gen_type<VkAccelerationStructureMotionInfoNV> {};
template<>
struct fex_gen_type<VkSRTDataNV> {};
template<>
struct fex_gen_type<VkAccelerationStructureSRTMotionInstanceNV> {};
template<>
struct fex_gen_type<VkPhysicalDeviceRayTracingMotionBlurFeaturesNV> {};
template<>
struct fex_gen_type<VkPhysicalDeviceYcbcr2Plane444FormatsFeaturesEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceFragmentDensityMap2FeaturesEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceFragmentDensityMap2PropertiesEXT> {};
template<>
struct fex_gen_type<VkCopyCommandTransformInfoQCOM> {};
template<>
struct fex_gen_type<VkPhysicalDeviceImageCompressionControlFeaturesEXT> {};
template<>
struct fex_gen_type<VkImageCompressionControlEXT> {};
template<>
struct fex_gen_type<VkImageCompressionPropertiesEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceAttachmentFeedbackLoopLayoutFeaturesEXT> {};
template<>
struct fex_gen_type<VkPhysicalDevice4444FormatsFeaturesEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceFaultFeaturesEXT> {};
template<>
struct fex_gen_type<VkDeviceFaultCountsEXT> {};
template<>
struct fex_gen_type<VkDeviceFaultAddressInfoEXT> {};
template<>
struct fex_gen_type<VkDeviceFaultVendorInfoEXT> {};
template<>
struct fex_gen_type<VkDeviceFaultInfoEXT> {};
template<>
struct fex_gen_type<VkDeviceFaultVendorBinaryHeaderVersionOneEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceRasterizationOrderAttachmentAccessFeaturesEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceRGBA10X6FormatsFeaturesEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT> {};
template<>
struct fex_gen_type<VkMutableDescriptorTypeListEXT> {};
template<>
struct fex_gen_type<VkMutableDescriptorTypeCreateInfoEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceVertexInputDynamicStateFeaturesEXT> {};
template<>
struct fex_gen_type<VkVertexInputBindingDescription2EXT> {};
template<>
struct fex_gen_type<VkVertexInputAttributeDescription2EXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceDrmPropertiesEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceAddressBindingReportFeaturesEXT> {};
template<>
struct fex_gen_type<VkDeviceAddressBindingCallbackDataEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceDepthClipControlFeaturesEXT> {};
template<>
struct fex_gen_type<VkPipelineViewportDepthClipControlCreateInfoEXT> {};
template<>
struct fex_gen_type<VkPhysicalDevicePrimitiveTopologyListRestartFeaturesEXT> {};
template<>
struct fex_gen_type<VkPhysicalDevicePresentModeFifoLatestReadyFeaturesEXT> {};
template<>
struct fex_gen_type<VkSubpassShadingPipelineCreateInfoHUAWEI> {};
template<>
struct fex_gen_type<VkPhysicalDeviceSubpassShadingFeaturesHUAWEI> {};
template<>
struct fex_gen_type<VkPhysicalDeviceSubpassShadingPropertiesHUAWEI> {};
template<>
struct fex_gen_type<VkPhysicalDeviceInvocationMaskFeaturesHUAWEI> {};
template<>
struct fex_gen_type<VkMemoryGetRemoteAddressInfoNV> {};
template<>
struct fex_gen_type<VkPhysicalDeviceExternalMemoryRDMAFeaturesNV> {};
template<>
struct fex_gen_type<VkPipelinePropertiesIdentifierEXT> {};
template<>
struct fex_gen_type<VkPhysicalDevicePipelinePropertiesFeaturesEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceFrameBoundaryFeaturesEXT> {};
template<>
struct fex_gen_type<VkFrameBoundaryEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceMultisampledRenderToSingleSampledFeaturesEXT> {};
template<>
struct fex_gen_type<VkSubpassResolvePerformanceQueryEXT> {};
template<>
struct fex_gen_type<VkMultisampledRenderToSingleSampledInfoEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceExtendedDynamicState2FeaturesEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceColorWriteEnableFeaturesEXT> {};
template<>
struct fex_gen_type<VkPipelineColorWriteCreateInfoEXT> {};
template<>
struct fex_gen_type<VkPhysicalDevicePrimitivesGeneratedQueryFeaturesEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceImageViewMinLodFeaturesEXT> {};
template<>
struct fex_gen_type<VkImageViewMinLodCreateInfoEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceMultiDrawFeaturesEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceMultiDrawPropertiesEXT> {};
template<>
struct fex_gen_type<VkMultiDrawInfoEXT> {};
template<>
struct fex_gen_type<VkMultiDrawIndexedInfoEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceImage2DViewOf3DFeaturesEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceShaderTileImageFeaturesEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceShaderTileImagePropertiesEXT> {};
template<>
struct fex_gen_type<VkMicromapUsageEXT> {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_type<VkDeviceOrHostAddressKHR> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_type<VkDeviceOrHostAddressKHR> : fexgen::assume_compatible_data_layout {};
#endif
template<>
struct fex_gen_type<VkMicromapCreateInfoEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceOpacityMicromapFeaturesEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceOpacityMicromapPropertiesEXT> {};
template<>
struct fex_gen_type<VkMicromapVersionInfoEXT> {};
template<>
struct fex_gen_type<VkCopyMicromapInfoEXT> {};
template<>
struct fex_gen_type<VkMicromapBuildSizesInfoEXT> {};
template<>
struct fex_gen_type<VkMicromapTriangleEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceClusterCullingShaderFeaturesHUAWEI> {};
template<>
struct fex_gen_type<VkPhysicalDeviceClusterCullingShaderPropertiesHUAWEI> {};
template<>
struct fex_gen_type<VkPhysicalDeviceClusterCullingShaderVrsFeaturesHUAWEI> {};
template<>
struct fex_gen_type<VkPhysicalDeviceBorderColorSwizzleFeaturesEXT> {};
template<>
struct fex_gen_type<VkSamplerBorderColorComponentMappingCreateInfoEXT> {};
template<>
struct fex_gen_type<VkPhysicalDevicePageableDeviceLocalMemoryFeaturesEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceShaderCorePropertiesARM> {};
template<>
struct fex_gen_type<VkDeviceQueueShaderCoreControlCreateInfoARM> {};
template<>
struct fex_gen_type<VkPhysicalDeviceSchedulingControlsFeaturesARM> {};
template<>
struct fex_gen_type<VkPhysicalDeviceSchedulingControlsPropertiesARM> {};
template<>
struct fex_gen_type<VkPhysicalDeviceImageSlicedViewOf3DFeaturesEXT> {};
template<>
struct fex_gen_type<VkImageViewSlicedCreateInfoEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceDescriptorSetHostMappingFeaturesVALVE> {};
template<>
struct fex_gen_type<VkDescriptorSetBindingReferenceVALVE> {};
template<>
struct fex_gen_type<VkDescriptorSetLayoutHostMappingInfoVALVE> {};
template<>
struct fex_gen_type<VkPhysicalDeviceNonSeamlessCubeMapFeaturesEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceRenderPassStripedFeaturesARM> {};
template<>
struct fex_gen_type<VkPhysicalDeviceRenderPassStripedPropertiesARM> {};
template<>
struct fex_gen_type<VkRenderPassStripeInfoARM> {};
template<>
struct fex_gen_type<VkRenderPassStripeBeginInfoARM> {};
template<>
struct fex_gen_type<VkRenderPassStripeSubmitInfoARM> {};
template<>
struct fex_gen_type<VkPhysicalDeviceFragmentDensityMapOffsetFeaturesQCOM> {};
template<>
struct fex_gen_type<VkPhysicalDeviceFragmentDensityMapOffsetPropertiesQCOM> {};
template<>
struct fex_gen_type<VkSubpassFragmentDensityMapOffsetEndInfoQCOM> {};
template<>
struct fex_gen_type<VkCopyMemoryIndirectCommandNV> {};
template<>
struct fex_gen_type<VkCopyMemoryToImageIndirectCommandNV> {};
template<>
struct fex_gen_type<VkPhysicalDeviceCopyMemoryIndirectFeaturesNV> {};
template<>
struct fex_gen_type<VkPhysicalDeviceCopyMemoryIndirectPropertiesNV> {};
template<>
struct fex_gen_type<VkDecompressMemoryRegionNV> {};
template<>
struct fex_gen_type<VkPhysicalDeviceMemoryDecompressionFeaturesNV> {};
template<>
struct fex_gen_type<VkPhysicalDeviceMemoryDecompressionPropertiesNV> {};
template<>
struct fex_gen_type<VkPhysicalDeviceDeviceGeneratedCommandsComputeFeaturesNV> {};
template<>
struct fex_gen_type<VkComputePipelineIndirectBufferInfoNV> {};
template<>
struct fex_gen_type<VkPipelineIndirectDeviceAddressInfoNV> {};
template<>
struct fex_gen_type<VkBindPipelineIndirectCommandNV> {};
template<>
struct fex_gen_type<VkPhysicalDeviceRayTracingLinearSweptSpheresFeaturesNV> {};
template<>
struct fex_gen_type<VkPhysicalDeviceLinearColorAttachmentFeaturesNV> {};
template<>
struct fex_gen_type<VkPhysicalDeviceImageCompressionControlSwapchainFeaturesEXT> {};
template<>
struct fex_gen_type<VkImageViewSampleWeightCreateInfoQCOM> {};
template<>
struct fex_gen_type<VkPhysicalDeviceImageProcessingFeaturesQCOM> {};
template<>
struct fex_gen_type<VkPhysicalDeviceImageProcessingPropertiesQCOM> {};
template<>
struct fex_gen_type<VkPhysicalDeviceNestedCommandBufferFeaturesEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceNestedCommandBufferPropertiesEXT> {};
template<>
struct fex_gen_type<VkExternalMemoryAcquireUnmodifiedEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceExtendedDynamicState3FeaturesEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceExtendedDynamicState3PropertiesEXT> {};
template<>
struct fex_gen_type<VkColorBlendEquationEXT> {};
template<>
struct fex_gen_type<VkColorBlendAdvancedEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceSubpassMergeFeedbackFeaturesEXT> {};
template<>
struct fex_gen_type<VkRenderPassCreationControlEXT> {};
template<>
struct fex_gen_type<VkRenderPassCreationFeedbackInfoEXT> {};
template<>
struct fex_gen_type<VkRenderPassCreationFeedbackCreateInfoEXT> {};
template<>
struct fex_gen_type<VkRenderPassSubpassFeedbackInfoEXT> {};
template<>
struct fex_gen_type<VkRenderPassSubpassFeedbackCreateInfoEXT> {};
template<>
struct fex_gen_type<VkDirectDriverLoadingInfoLUNARG> {};
template<>
struct fex_gen_type<VkDirectDriverLoadingListLUNARG> {};
template<>
struct fex_gen_type<VkPhysicalDeviceShaderModuleIdentifierFeaturesEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceShaderModuleIdentifierPropertiesEXT> {};
template<>
struct fex_gen_type<VkPipelineShaderStageModuleIdentifierCreateInfoEXT> {};
template<>
struct fex_gen_type<VkShaderModuleIdentifierEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceOpticalFlowFeaturesNV> {};
template<>
struct fex_gen_type<VkPhysicalDeviceOpticalFlowPropertiesNV> {};
template<>
struct fex_gen_type<VkOpticalFlowImageFormatInfoNV> {};
template<>
struct fex_gen_type<VkOpticalFlowImageFormatPropertiesNV> {};
template<>
struct fex_gen_type<VkOpticalFlowSessionCreateInfoNV> {};
template<>
struct fex_gen_type<VkOpticalFlowSessionCreatePrivateDataInfoNV> {};
template<>
struct fex_gen_type<VkOpticalFlowExecuteInfoNV> {};
template<>
struct fex_gen_type<VkPhysicalDeviceLegacyDitheringFeaturesEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceAntiLagFeaturesAMD> {};
template<>
struct fex_gen_type<VkAntiLagPresentationInfoAMD> {};
template<>
struct fex_gen_type<VkAntiLagDataAMD> {};
template<>
struct fex_gen_type<VkPhysicalDeviceShaderObjectFeaturesEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceShaderObjectPropertiesEXT> {};
template<>
struct fex_gen_type<VkShaderCreateInfoEXT> {};
template<>
struct fex_gen_type<VkDepthClampRangeEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceTilePropertiesFeaturesQCOM> {};
template<>
struct fex_gen_type<VkTilePropertiesQCOM> {};
template<>
struct fex_gen_type<VkPhysicalDeviceAmigoProfilingFeaturesSEC> {};
template<>
struct fex_gen_type<VkAmigoProfilingSubmitInfoSEC> {};
template<>
struct fex_gen_type<VkPhysicalDeviceMultiviewPerViewViewportsFeaturesQCOM> {};
template<>
struct fex_gen_type<VkPhysicalDeviceRayTracingInvocationReorderPropertiesNV> {};
template<>
struct fex_gen_type<VkPhysicalDeviceRayTracingInvocationReorderFeaturesNV> {};
template<>
struct fex_gen_type<VkPhysicalDeviceCooperativeVectorPropertiesNV> {};
template<>
struct fex_gen_type<VkPhysicalDeviceCooperativeVectorFeaturesNV> {};
template<>
struct fex_gen_type<VkCooperativeVectorPropertiesNV> {};
template<>
struct fex_gen_type<VkPhysicalDeviceExtendedSparseAddressSpaceFeaturesNV> {};
template<>
struct fex_gen_type<VkPhysicalDeviceExtendedSparseAddressSpacePropertiesNV> {};
template<>
struct fex_gen_type<VkPhysicalDeviceLegacyVertexAttributesFeaturesEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceLegacyVertexAttributesPropertiesEXT> {};
template<>
struct fex_gen_type<VkLayerSettingEXT> {};
template<>
struct fex_gen_type<VkLayerSettingsCreateInfoEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceShaderCoreBuiltinsFeaturesARM> {};
template<>
struct fex_gen_type<VkPhysicalDeviceShaderCoreBuiltinsPropertiesARM> {};
template<>
struct fex_gen_type<VkPhysicalDevicePipelineLibraryGroupHandlesFeaturesEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceDynamicRenderingUnusedAttachmentsFeaturesEXT> {};
template<>
struct fex_gen_type<VkLatencySleepModeInfoNV> {};
template<>
struct fex_gen_type<VkLatencySleepInfoNV> {};
template<>
struct fex_gen_type<VkSetLatencyMarkerInfoNV> {};
template<>
struct fex_gen_type<VkLatencyTimingsFrameReportNV> {};
template<>
struct fex_gen_type<VkGetLatencyMarkerInfoNV> {};
template<>
struct fex_gen_type<VkLatencySubmissionPresentIdNV> {};
template<>
struct fex_gen_type<VkSwapchainLatencyCreateInfoNV> {};
template<>
struct fex_gen_type<VkOutOfBandQueueTypeInfoNV> {};
template<>
struct fex_gen_type<VkLatencySurfaceCapabilitiesNV> {};
template<>
struct fex_gen_type<VkPhysicalDeviceMultiviewPerViewRenderAreasFeaturesQCOM> {};
template<>
struct fex_gen_type<VkMultiviewPerViewRenderAreasRenderPassBeginInfoQCOM> {};
template<>
struct fex_gen_type<VkPhysicalDevicePerStageDescriptorSetFeaturesNV> {};
template<>
struct fex_gen_type<VkPhysicalDeviceImageProcessing2FeaturesQCOM> {};
template<>
struct fex_gen_type<VkPhysicalDeviceImageProcessing2PropertiesQCOM> {};
template<>
struct fex_gen_type<VkSamplerBlockMatchWindowCreateInfoQCOM> {};
template<>
struct fex_gen_type<VkPhysicalDeviceCubicWeightsFeaturesQCOM> {};
template<>
struct fex_gen_type<VkSamplerCubicWeightsCreateInfoQCOM> {};
template<>
struct fex_gen_type<VkBlitImageCubicWeightsInfoQCOM> {};
template<>
struct fex_gen_type<VkPhysicalDeviceYcbcrDegammaFeaturesQCOM> {};
template<>
struct fex_gen_type<VkSamplerYcbcrConversionYcbcrDegammaCreateInfoQCOM> {};
template<>
struct fex_gen_type<VkPhysicalDeviceCubicClampFeaturesQCOM> {};
template<>
struct fex_gen_type<VkPhysicalDeviceAttachmentFeedbackLoopDynamicStateFeaturesEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceLayeredDriverPropertiesMSFT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceDescriptorPoolOverallocationFeaturesNV> {};
template<>
struct fex_gen_type<VkDisplaySurfaceStereoCreateInfoNV> {};
template<>
struct fex_gen_type<VkDisplayModeStereoPropertiesNV> {};
template<>
struct fex_gen_type<VkPhysicalDeviceRawAccessChainsFeaturesNV> {};
template<>
struct fex_gen_type<VkPhysicalDeviceCommandBufferInheritanceFeaturesNV> {};
template<>
struct fex_gen_type<VkPhysicalDeviceShaderAtomicFloat16VectorFeaturesNV> {};
template<>
struct fex_gen_type<VkPhysicalDeviceShaderReplicatedCompositesFeaturesEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceRayTracingValidationFeaturesNV> {};
template<>
struct fex_gen_type<VkPhysicalDeviceClusterAccelerationStructureFeaturesNV> {};
template<>
struct fex_gen_type<VkPhysicalDeviceClusterAccelerationStructurePropertiesNV> {};
template<>
struct fex_gen_type<VkClusterAccelerationStructureClustersBottomLevelInputNV> {};
template<>
struct fex_gen_type<VkClusterAccelerationStructureTriangleClusterInputNV> {};
template<>
struct fex_gen_type<VkClusterAccelerationStructureMoveObjectsInputNV> {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_type<VkClusterAccelerationStructureOpInputNV> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_type<VkClusterAccelerationStructureOpInputNV> : fexgen::assume_compatible_data_layout {};
#endif
template<>
struct fex_gen_type<VkStridedDeviceAddressRegionKHR> {};
template<>
struct fex_gen_type<VkStridedDeviceAddressNV> {};
template<>
struct fex_gen_type<VkClusterAccelerationStructureGeometryIndexAndGeometryFlagsNV> {};
template<>
struct fex_gen_type<VkClusterAccelerationStructureMoveObjectsInfoNV> {};
template<>
struct fex_gen_type<VkClusterAccelerationStructureBuildClustersBottomLevelInfoNV> {};
template<>
struct fex_gen_type<VkClusterAccelerationStructureBuildTriangleClusterInfoNV> {};
template<>
struct fex_gen_type<VkClusterAccelerationStructureBuildTriangleClusterTemplateInfoNV> {};
template<>
struct fex_gen_type<VkClusterAccelerationStructureInstantiateClusterInfoNV> {};
template<>
struct fex_gen_type<VkAccelerationStructureBuildSizesInfoKHR> {};
template<>
struct fex_gen_type<VkRayTracingPipelineClusterAccelerationStructureCreateInfoNV> {};
template<>
struct fex_gen_type<VkPhysicalDevicePartitionedAccelerationStructureFeaturesNV> {};
template<>
struct fex_gen_type<VkPhysicalDevicePartitionedAccelerationStructurePropertiesNV> {};
template<>
struct fex_gen_type<VkPartitionedAccelerationStructureFlagsNV> {};
template<>
struct fex_gen_type<VkBuildPartitionedAccelerationStructureIndirectCommandNV> {};
template<>
struct fex_gen_type<VkPartitionedAccelerationStructureUpdateInstanceDataNV> {};
template<>
struct fex_gen_type<VkPartitionedAccelerationStructureWritePartitionTranslationDataNV> {};
template<>
struct fex_gen_type<VkWriteDescriptorSetPartitionedAccelerationStructureNV> {};
template<>
struct fex_gen_type<VkPartitionedAccelerationStructureInstancesInputNV> {};
template<>
struct fex_gen_type<VkBuildPartitionedAccelerationStructureInfoNV> {};
template<>
struct fex_gen_type<VkPhysicalDeviceDeviceGeneratedCommandsFeaturesEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceDeviceGeneratedCommandsPropertiesEXT> {};
template<>
struct fex_gen_type<VkGeneratedCommandsMemoryRequirementsInfoEXT> {};
template<>
struct fex_gen_type<VkIndirectExecutionSetPipelineInfoEXT> {};
template<>
struct fex_gen_type<VkIndirectExecutionSetShaderLayoutInfoEXT> {};
template<>
struct fex_gen_type<VkIndirectExecutionSetShaderInfoEXT> {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_type<VkIndirectExecutionSetInfoEXT> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_type<VkIndirectExecutionSetInfoEXT> : fexgen::assume_compatible_data_layout {};
#endif
template<>
struct fex_gen_type<VkGeneratedCommandsInfoEXT> {};
template<>
struct fex_gen_type<VkWriteIndirectExecutionSetPipelineEXT> {};
template<>
struct fex_gen_type<VkIndirectCommandsPushConstantTokenEXT> {};
template<>
struct fex_gen_type<VkIndirectCommandsVertexBufferTokenEXT> {};
template<>
struct fex_gen_type<VkIndirectCommandsIndexBufferTokenEXT> {};
template<>
struct fex_gen_type<VkIndirectCommandsExecutionSetTokenEXT> {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_type<VkIndirectCommandsTokenDataEXT> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_type<VkIndirectCommandsTokenDataEXT> : fexgen::assume_compatible_data_layout {};
#endif
template<>
struct fex_gen_type<VkDrawIndirectCountIndirectCommandEXT> {};
template<>
struct fex_gen_type<VkBindVertexBufferIndirectCommandEXT> {};
template<>
struct fex_gen_type<VkBindIndexBufferIndirectCommandEXT> {};
template<>
struct fex_gen_type<VkGeneratedCommandsPipelineInfoEXT> {};
template<>
struct fex_gen_type<VkGeneratedCommandsShaderInfoEXT> {};
template<>
struct fex_gen_type<VkWriteIndirectExecutionSetShaderEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceImageAlignmentControlFeaturesMESA> {};
template<>
struct fex_gen_type<VkPhysicalDeviceImageAlignmentControlPropertiesMESA> {};
template<>
struct fex_gen_type<VkImageAlignmentControlCreateInfoMESA> {};
template<>
struct fex_gen_type<VkPhysicalDeviceDepthClampControlFeaturesEXT> {};
template<>
struct fex_gen_type<VkPipelineViewportDepthClampControlCreateInfoEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceHdrVividFeaturesHUAWEI> {};
template<>
struct fex_gen_type<VkHdrVividDynamicMetadataHUAWEI> {};
template<>
struct fex_gen_type<VkCooperativeMatrixFlexibleDimensionsPropertiesNV> {};
template<>
struct fex_gen_type<VkPhysicalDeviceCooperativeMatrix2FeaturesNV> {};
template<>
struct fex_gen_type<VkPhysicalDeviceCooperativeMatrix2PropertiesNV> {};
template<>
struct fex_gen_type<VkPhysicalDevicePipelineOpacityMicromapFeaturesARM> {};
template<>
struct fex_gen_type<VkPhysicalDeviceVertexAttributeRobustnessFeaturesEXT> {};
template<>
struct fex_gen_type<VkSetPresentConfigNV> {};
template<>
struct fex_gen_type<VkPhysicalDevicePresentMeteringFeaturesNV> {};
template<>
struct fex_gen_type<VkAccelerationStructureBuildRangeInfoKHR> {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_type<VkAccelerationStructureGeometryDataKHR> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_type<VkAccelerationStructureGeometryDataKHR> : fexgen::assume_compatible_data_layout {};
#endif
template<>
struct fex_gen_type<VkAccelerationStructureCreateInfoKHR> {};
template<>
struct fex_gen_type<VkWriteDescriptorSetAccelerationStructureKHR> {};
template<>
struct fex_gen_type<VkPhysicalDeviceAccelerationStructureFeaturesKHR> {};
template<>
struct fex_gen_type<VkPhysicalDeviceAccelerationStructurePropertiesKHR> {};
template<>
struct fex_gen_type<VkAccelerationStructureDeviceAddressInfoKHR> {};
template<>
struct fex_gen_type<VkAccelerationStructureVersionInfoKHR> {};
template<>
struct fex_gen_type<VkCopyAccelerationStructureInfoKHR> {};
template<>
struct fex_gen_type<VkRayTracingShaderGroupCreateInfoKHR> {};
template<>
struct fex_gen_type<VkRayTracingPipelineInterfaceCreateInfoKHR> {};
template<>
struct fex_gen_type<VkRayTracingPipelineCreateInfoKHR> {};
template<>
struct fex_gen_type<VkPhysicalDeviceRayTracingPipelineFeaturesKHR> {};
template<>
struct fex_gen_type<VkPhysicalDeviceRayTracingPipelinePropertiesKHR> {};
template<>
struct fex_gen_type<VkTraceRaysIndirectCommandKHR> {};
template<>
struct fex_gen_type<VkPhysicalDeviceRayQueryFeaturesKHR> {};
template<>
struct fex_gen_type<VkPhysicalDeviceMeshShaderFeaturesEXT> {};
template<>
struct fex_gen_type<VkPhysicalDeviceMeshShaderPropertiesEXT> {};
template<>
struct fex_gen_type<VkDrawMeshTasksIndirectCommandEXT> {};
// Wayland interop
template<>
struct fex_gen_type<wl_display> : fexgen::opaque_type {};
// Wayland interop
template<>
struct fex_gen_type<wl_surface> : fexgen::opaque_type {};
template<>
struct fex_gen_type<VkWaylandSurfaceCreateInfoKHR> {};
template<>
struct fex_gen_type<VkXcbSurfaceCreateInfoKHR> {};
template<>
struct fex_gen_type<VkXlibSurfaceCreateInfoKHR> {};
template<>
struct fex_gen_config<SetGuestXSync> : fexgen::custom_guest_entrypoint, fexgen::custom_host_impl {};
template<>
struct fex_gen_config<SetGuestXGetVisualInfo> : fexgen::custom_guest_entrypoint, fexgen::custom_host_impl {};
template<>
struct fex_gen_config<SetGuestXDisplayString> : fexgen::custom_guest_entrypoint, fexgen::custom_host_impl {};
template<>
struct fex_gen_config<vkGetInstanceProcAddr> : fexgen::custom_host_impl, fexgen::custom_guest_entrypoint, fexgen::returns_guest_pointer {};
template<>
struct fex_gen_config<vkGetDeviceProcAddr> : fexgen::custom_host_impl, fexgen::custom_guest_entrypoint, fexgen::returns_guest_pointer {};
namespace internal {
template<>
struct fex_gen_config<vkCreateInstance> : fexgen::custom_host_impl {};
template<>
struct fex_gen_param<vkCreateInstance, 2, VkInstance*> : fexgen::ptr_passthrough {};
template<>
struct fex_gen_config<vkDestroyInstance> {};
#ifdef IS_32BIT_THUNK
template<>
struct fex_gen_config<vkEnumeratePhysicalDevices> : fexgen::custom_host_impl {};
#else
template<>
struct fex_gen_config<vkEnumeratePhysicalDevices> {};
#endif
#ifdef IS_32BIT_THUNK
template<>
struct fex_gen_param<vkEnumeratePhysicalDevices, 2, VkPhysicalDevice*> : fexgen::ptr_passthrough {};
#endif
template<>
struct fex_gen_config<vkGetPhysicalDeviceFeatures> {};
template<>
struct fex_gen_config<vkGetPhysicalDeviceFormatProperties> {};
template<>
struct fex_gen_config<vkGetPhysicalDeviceImageFormatProperties> {};
// TODO: Output parameter must repack on exit!
template<>
struct fex_gen_config<vkGetPhysicalDeviceProperties> {};
template<>
struct fex_gen_config<vkGetPhysicalDeviceQueueFamilyProperties> {};
template<>
struct fex_gen_config<vkGetPhysicalDeviceMemoryProperties> {};
template<>
struct fex_gen_config<vkCreateDevice> : fexgen::custom_host_impl {};
template<>
struct fex_gen_param<vkCreateDevice, 3, VkDevice*> : fexgen::ptr_passthrough {};
template<>
struct fex_gen_config<vkDestroyDevice> {};
template<>
struct fex_gen_config<vkEnumerateInstanceExtensionProperties> {};
template<>
struct fex_gen_config<vkEnumerateDeviceExtensionProperties> {};
template<>
struct fex_gen_config<vkEnumerateInstanceLayerProperties> {};
template<>
struct fex_gen_config<vkEnumerateDeviceLayerProperties> {};
#ifdef IS_32BIT_THUNK
template<>
struct fex_gen_config<vkGetDeviceQueue> : fexgen::custom_host_impl {};
#else
template<>
struct fex_gen_config<vkGetDeviceQueue> {};
#endif
#ifdef IS_32BIT_THUNK
template<>
struct fex_gen_param<vkGetDeviceQueue, 3, VkQueue*> : fexgen::ptr_passthrough {};
#endif
#ifdef IS_32BIT_THUNK
template<>
struct fex_gen_config<vkQueueSubmit> : fexgen::custom_host_impl {};
#else
template<>
struct fex_gen_config<vkQueueSubmit> {};
#endif
#ifdef IS_32BIT_THUNK
template<>
struct fex_gen_param<vkQueueSubmit, 2, const VkSubmitInfo*> : fexgen::ptr_passthrough {};
#endif
template<>
struct fex_gen_config<vkQueueWaitIdle> {};
template<>
struct fex_gen_config<vkDeviceWaitIdle> {};
template<>
struct fex_gen_config<vkAllocateMemory> : fexgen::custom_host_impl {};
template<>
struct fex_gen_config<vkFreeMemory> : fexgen::custom_host_impl {};
#ifdef IS_32BIT_THUNK
template<>
struct fex_gen_config<vkMapMemory> : fexgen::custom_host_impl {};
#else
template<>
struct fex_gen_config<vkMapMemory> {};
#endif
#ifdef IS_32BIT_THUNK
template<>
struct fex_gen_param<vkMapMemory, 5, void**> : fexgen::ptr_passthrough {};
#endif
template<>
struct fex_gen_config<vkUnmapMemory> {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkFlushMappedMemoryRanges> {};
#else
template<>
struct fex_gen_config<vkFlushMappedMemoryRanges> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkInvalidateMappedMemoryRanges> {};
#else
template<>
struct fex_gen_config<vkInvalidateMappedMemoryRanges> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetDeviceMemoryCommitment> {};
#else
template<>
struct fex_gen_config<vkGetDeviceMemoryCommitment> {};
#endif
template<>
struct fex_gen_config<vkBindBufferMemory> {};
template<>
struct fex_gen_config<vkBindImageMemory> {};
template<>
struct fex_gen_config<vkGetBufferMemoryRequirements> {};
template<>
struct fex_gen_config<vkGetImageMemoryRequirements> {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetImageSparseMemoryRequirements> {};
#else
template<>
struct fex_gen_config<vkGetImageSparseMemoryRequirements> {};
#endif
template<>
struct fex_gen_config<vkGetPhysicalDeviceSparseImageFormatProperties> {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkQueueBindSparse> {};
#else
template<>
struct fex_gen_config<vkQueueBindSparse> {};
#endif
template<>
struct fex_gen_config<vkCreateFence> {};
template<>
struct fex_gen_config<vkDestroyFence> {};
template<>
struct fex_gen_config<vkResetFences> {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetFenceStatus> {};
#else
template<>
struct fex_gen_config<vkGetFenceStatus> {};
#endif
template<>
struct fex_gen_config<vkWaitForFences> {};
template<>
struct fex_gen_config<vkCreateSemaphore> {};
template<>
struct fex_gen_config<vkDestroySemaphore> {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCreateEvent> {};
#else
template<>
struct fex_gen_config<vkCreateEvent> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkDestroyEvent> {};
#else
template<>
struct fex_gen_config<vkDestroyEvent> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetEventStatus> {};
#else
template<>
struct fex_gen_config<vkGetEventStatus> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkSetEvent> {};
#else
template<>
struct fex_gen_config<vkSetEvent> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkResetEvent> {};
#else
template<>
struct fex_gen_config<vkResetEvent> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCreateQueryPool> {};
#else
template<>
struct fex_gen_config<vkCreateQueryPool> {};
#endif
template<>
struct fex_gen_config<vkDestroyQueryPool> {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetQueryPoolResults> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_config<vkGetQueryPoolResults> {};
#endif
#ifndef IS_32BIT_THUNK
template<>
struct fex_gen_param<vkGetQueryPoolResults, 5, void*> : fexgen::assume_compatible_data_layout {};
#endif
template<>
struct fex_gen_config<vkCreateBuffer> {};
template<>
struct fex_gen_config<vkDestroyBuffer> {};
template<>
struct fex_gen_config<vkCreateBufferView> {};
template<>
struct fex_gen_config<vkDestroyBufferView> {};
template<>
struct fex_gen_config<vkCreateImage> {};
template<>
struct fex_gen_config<vkDestroyImage> {};
template<>
struct fex_gen_config<vkGetImageSubresourceLayout> {};
template<>
struct fex_gen_config<vkCreateImageView> {};
template<>
struct fex_gen_config<vkDestroyImageView> {};
template<>
struct fex_gen_config<vkCreateShaderModule> : fexgen::custom_host_impl {};
template<>
struct fex_gen_config<vkDestroyShaderModule> {};
template<>
struct fex_gen_config<vkCreatePipelineCache> {};
template<>
struct fex_gen_config<vkDestroyPipelineCache> {};
#ifdef IS_32BIT_THUNK
template<>
struct fex_gen_config<vkGetPipelineCacheData> : fexgen::custom_host_impl {};
#else
template<>
struct fex_gen_config<vkGetPipelineCacheData> {};
#endif
#ifdef IS_32BIT_THUNK
template<>
struct fex_gen_param<vkGetPipelineCacheData, 2, size_t*> : fexgen::ptr_passthrough {};
#endif
template<>
struct fex_gen_param<vkGetPipelineCacheData, 3, void*> : fexgen::assume_compatible_data_layout {};
template<>
struct fex_gen_config<vkMergePipelineCaches> {};
// TODO: Should be custom_host_impl since there may be more than one VkGraphicsPipelineCreateInfo and more than one output pipeline
template<>
struct fex_gen_config<vkCreateGraphicsPipelines> {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCreateComputePipelines> {};
#else
template<>
struct fex_gen_config<vkCreateComputePipelines> {};
#endif
template<>
struct fex_gen_config<vkDestroyPipeline> {};
template<>
struct fex_gen_config<vkCreatePipelineLayout> {};
template<>
struct fex_gen_config<vkDestroyPipelineLayout> {};
template<>
struct fex_gen_config<vkCreateSampler> {};
template<>
struct fex_gen_config<vkDestroySampler> {};
template<>
struct fex_gen_config<vkCreateDescriptorSetLayout> {};
template<>
struct fex_gen_config<vkDestroyDescriptorSetLayout> {};
template<>
struct fex_gen_config<vkCreateDescriptorPool> {};
template<>
struct fex_gen_config<vkDestroyDescriptorPool> {};
template<>
struct fex_gen_config<vkResetDescriptorPool> {};
template<>
struct fex_gen_config<vkAllocateDescriptorSets> {};
template<>
struct fex_gen_config<vkFreeDescriptorSets> {};
#ifdef IS_32BIT_THUNK
template<>
struct fex_gen_config<vkUpdateDescriptorSets> : fexgen::custom_host_impl {};
#else
template<>
struct fex_gen_config<vkUpdateDescriptorSets> {};
#endif
#ifdef IS_32BIT_THUNK
template<>
struct fex_gen_param<vkUpdateDescriptorSets, 2, const VkWriteDescriptorSet*> : fexgen::ptr_passthrough {};
template<>
struct fex_gen_param<vkUpdateDescriptorSets, 4, const VkCopyDescriptorSet*> : fexgen::ptr_passthrough {};
#endif
template<>
struct fex_gen_config<vkCreateFramebuffer> {};
template<>
struct fex_gen_config<vkDestroyFramebuffer> {};
template<>
struct fex_gen_config<vkCreateRenderPass> {};
template<>
struct fex_gen_config<vkDestroyRenderPass> {};
template<>
struct fex_gen_config<vkGetRenderAreaGranularity> {};
template<>
struct fex_gen_config<vkCreateCommandPool> {};
template<>
struct fex_gen_config<vkDestroyCommandPool> {};
template<>
struct fex_gen_config<vkResetCommandPool> {};
#ifdef IS_32BIT_THUNK
template<>
struct fex_gen_config<vkAllocateCommandBuffers> : fexgen::custom_host_impl {};
#else
template<>
struct fex_gen_config<vkAllocateCommandBuffers> {};
#endif
#ifdef IS_32BIT_THUNK
template<>
struct fex_gen_param<vkAllocateCommandBuffers, 2, VkCommandBuffer*> : fexgen::ptr_passthrough {};
#endif
#ifdef IS_32BIT_THUNK
template<>
struct fex_gen_config<vkFreeCommandBuffers> : fexgen::custom_host_impl {};
#else
template<>
struct fex_gen_config<vkFreeCommandBuffers> {};
#endif
#ifdef IS_32BIT_THUNK
template<>
struct fex_gen_param<vkFreeCommandBuffers, 3, const VkCommandBuffer*> : fexgen::ptr_passthrough {};
#endif
template<>
struct fex_gen_config<vkBeginCommandBuffer> {};
template<>
struct fex_gen_config<vkEndCommandBuffer> {};
template<>
struct fex_gen_config<vkResetCommandBuffer> {};
template<>
struct fex_gen_config<vkCmdBindPipeline> {};
template<>
struct fex_gen_config<vkCmdSetViewport> {};
template<>
struct fex_gen_config<vkCmdSetScissor> {};
template<>
struct fex_gen_config<vkCmdSetLineWidth> {};
template<>
struct fex_gen_config<vkCmdSetDepthBias> {};
template<>
struct fex_gen_config<vkCmdSetBlendConstants> {};
template<>
struct fex_gen_config<vkCmdSetDepthBounds> {};
template<>
struct fex_gen_config<vkCmdSetStencilCompareMask> {};
template<>
struct fex_gen_config<vkCmdSetStencilWriteMask> {};
template<>
struct fex_gen_config<vkCmdSetStencilReference> {};
template<>
struct fex_gen_config<vkCmdBindDescriptorSets> {};
template<>
struct fex_gen_config<vkCmdBindIndexBuffer> {};
template<>
struct fex_gen_config<vkCmdBindVertexBuffers> {};
template<>
struct fex_gen_config<vkCmdDraw> {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdDrawIndexed> {};
#else
template<>
struct fex_gen_config<vkCmdDrawIndexed> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdDrawIndirect> {};
#else
template<>
struct fex_gen_config<vkCmdDrawIndirect> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdDrawIndexedIndirect> {};
#else
template<>
struct fex_gen_config<vkCmdDrawIndexedIndirect> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdDispatch> {};
#else
template<>
struct fex_gen_config<vkCmdDispatch> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdDispatchIndirect> {};
#else
template<>
struct fex_gen_config<vkCmdDispatchIndirect> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdCopyBuffer> {};
#else
template<>
struct fex_gen_config<vkCmdCopyBuffer> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdCopyImage> {};
#else
template<>
struct fex_gen_config<vkCmdCopyImage> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdBlitImage> {};
#else
template<>
struct fex_gen_config<vkCmdBlitImage> {};
#endif
template<>
struct fex_gen_config<vkCmdCopyBufferToImage> {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdCopyImageToBuffer> {};
#else
template<>
struct fex_gen_config<vkCmdCopyImageToBuffer> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdUpdateBuffer> {};
#else
template<>
struct fex_gen_config<vkCmdUpdateBuffer> {};
#endif
template<>
struct fex_gen_param<vkCmdUpdateBuffer, 4, const void*> : fexgen::assume_compatible_data_layout {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdFillBuffer> {};
#else
template<>
struct fex_gen_config<vkCmdFillBuffer> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdClearColorImage> {};
#else
template<>
struct fex_gen_config<vkCmdClearColorImage> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdClearDepthStencilImage> {};
#else
template<>
struct fex_gen_config<vkCmdClearDepthStencilImage> {};
#endif
template<>
struct fex_gen_config<vkCmdClearAttachments> {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdResolveImage> {};
#else
template<>
struct fex_gen_config<vkCmdResolveImage> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdSetEvent> {};
#else
template<>
struct fex_gen_config<vkCmdSetEvent> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdResetEvent> {};
#else
template<>
struct fex_gen_config<vkCmdResetEvent> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdWaitEvents> {};
#else
template<>
struct fex_gen_config<vkCmdWaitEvents> {};
#endif
template<>
struct fex_gen_config<vkCmdPipelineBarrier> {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdBeginQuery> {};
#else
template<>
struct fex_gen_config<vkCmdBeginQuery> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdEndQuery> {};
#else
template<>
struct fex_gen_config<vkCmdEndQuery> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdResetQueryPool> {};
#else
template<>
struct fex_gen_config<vkCmdResetQueryPool> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdWriteTimestamp> {};
#else
template<>
struct fex_gen_config<vkCmdWriteTimestamp> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdCopyQueryPoolResults> {};
#else
template<>
struct fex_gen_config<vkCmdCopyQueryPoolResults> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdPushConstants> {};
#else
template<>
struct fex_gen_config<vkCmdPushConstants> {};
#endif
template<>
struct fex_gen_param<vkCmdPushConstants, 5, const void*> : fexgen::assume_compatible_data_layout {};
template<>
struct fex_gen_config<vkCmdBeginRenderPass> {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdNextSubpass> {};
#else
template<>
struct fex_gen_config<vkCmdNextSubpass> {};
#endif
template<>
struct fex_gen_config<vkCmdEndRenderPass> {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdExecuteCommands> {};
#else
template<>
struct fex_gen_config<vkCmdExecuteCommands> {};
#endif
template<>
struct fex_gen_config<vkEnumerateInstanceVersion> {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkBindBufferMemory2> {};
#else
template<>
struct fex_gen_config<vkBindBufferMemory2> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkBindImageMemory2> {};
#else
template<>
struct fex_gen_config<vkBindImageMemory2> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetDeviceGroupPeerMemoryFeatures> {};
#else
template<>
struct fex_gen_config<vkGetDeviceGroupPeerMemoryFeatures> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdSetDeviceMask> {};
#else
template<>
struct fex_gen_config<vkCmdSetDeviceMask> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdDispatchBase> {};
#else
template<>
struct fex_gen_config<vkCmdDispatchBase> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkEnumeratePhysicalDeviceGroups> {};
#else
template<>
struct fex_gen_config<vkEnumeratePhysicalDeviceGroups> {};
#endif
template<>
struct fex_gen_config<vkGetImageMemoryRequirements2> {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetBufferMemoryRequirements2> {};
#else
template<>
struct fex_gen_config<vkGetBufferMemoryRequirements2> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetImageSparseMemoryRequirements2> {};
#else
template<>
struct fex_gen_config<vkGetImageSparseMemoryRequirements2> {};
#endif
template<>
struct fex_gen_config<vkGetPhysicalDeviceFeatures2> {};
template<>
struct fex_gen_config<vkGetPhysicalDeviceProperties2> {};
template<>
struct fex_gen_config<vkGetPhysicalDeviceFormatProperties2> {};
template<>
struct fex_gen_config<vkGetPhysicalDeviceImageFormatProperties2> {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetPhysicalDeviceQueueFamilyProperties2> {};
#else
template<>
struct fex_gen_config<vkGetPhysicalDeviceQueueFamilyProperties2> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetPhysicalDeviceMemoryProperties2> {};
#else
template<>
struct fex_gen_config<vkGetPhysicalDeviceMemoryProperties2> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetPhysicalDeviceSparseImageFormatProperties2> {};
#else
template<>
struct fex_gen_config<vkGetPhysicalDeviceSparseImageFormatProperties2> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkTrimCommandPool> {};
#else
template<>
struct fex_gen_config<vkTrimCommandPool> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetDeviceQueue2> {};
#else
template<>
struct fex_gen_config<vkGetDeviceQueue2> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCreateSamplerYcbcrConversion> {};
#else
template<>
struct fex_gen_config<vkCreateSamplerYcbcrConversion> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkDestroySamplerYcbcrConversion> {};
#else
template<>
struct fex_gen_config<vkDestroySamplerYcbcrConversion> {};
#endif
template<>
struct fex_gen_config<vkCreateDescriptorUpdateTemplate> {};
template<>
struct fex_gen_config<vkDestroyDescriptorUpdateTemplate> {};
template<>
struct fex_gen_config<vkUpdateDescriptorSetWithTemplate> {};
template<>
struct fex_gen_param<vkUpdateDescriptorSetWithTemplate, 3, const void*> : fexgen::assume_compatible_data_layout {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetPhysicalDeviceExternalBufferProperties> {};
#else
template<>
struct fex_gen_config<vkGetPhysicalDeviceExternalBufferProperties> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetPhysicalDeviceExternalFenceProperties> {};
#else
template<>
struct fex_gen_config<vkGetPhysicalDeviceExternalFenceProperties> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetPhysicalDeviceExternalSemaphoreProperties> {};
#else
template<>
struct fex_gen_config<vkGetPhysicalDeviceExternalSemaphoreProperties> {};
#endif
template<>
struct fex_gen_config<vkGetDescriptorSetLayoutSupport> {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdDrawIndirectCount> {};
#else
template<>
struct fex_gen_config<vkCmdDrawIndirectCount> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdDrawIndexedIndirectCount> {};
#else
template<>
struct fex_gen_config<vkCmdDrawIndexedIndirectCount> {};
#endif
template<>
struct fex_gen_config<vkCreateRenderPass2> {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdBeginRenderPass2> {};
#else
template<>
struct fex_gen_config<vkCmdBeginRenderPass2> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdNextSubpass2> {};
#else
template<>
struct fex_gen_config<vkCmdNextSubpass2> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdEndRenderPass2> {};
#else
template<>
struct fex_gen_config<vkCmdEndRenderPass2> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkResetQueryPool> {};
#else
template<>
struct fex_gen_config<vkResetQueryPool> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetSemaphoreCounterValue> {};
#else
template<>
struct fex_gen_config<vkGetSemaphoreCounterValue> {};
#endif
template<>
struct fex_gen_config<vkWaitSemaphores> {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkSignalSemaphore> {};
#else
template<>
struct fex_gen_config<vkSignalSemaphore> {};
#endif
template<>
struct fex_gen_config<vkGetBufferDeviceAddress> {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetBufferOpaqueCaptureAddress> {};
#else
template<>
struct fex_gen_config<vkGetBufferOpaqueCaptureAddress> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetDeviceMemoryOpaqueCaptureAddress> {};
#else
template<>
struct fex_gen_config<vkGetDeviceMemoryOpaqueCaptureAddress> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetPhysicalDeviceToolProperties> {};
#else
template<>
struct fex_gen_config<vkGetPhysicalDeviceToolProperties> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCreatePrivateDataSlot> {};
#else
template<>
struct fex_gen_config<vkCreatePrivateDataSlot> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkDestroyPrivateDataSlot> {};
#else
template<>
struct fex_gen_config<vkDestroyPrivateDataSlot> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkSetPrivateData> {};
#else
template<>
struct fex_gen_config<vkSetPrivateData> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetPrivateData> {};
#else
template<>
struct fex_gen_config<vkGetPrivateData> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdSetEvent2> {};
#else
template<>
struct fex_gen_config<vkCmdSetEvent2> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdResetEvent2> {};
#else
template<>
struct fex_gen_config<vkCmdResetEvent2> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdWaitEvents2> {};
#else
template<>
struct fex_gen_config<vkCmdWaitEvents2> {};
#endif
template<>
struct fex_gen_config<vkCmdPipelineBarrier2> {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdWriteTimestamp2> {};
#else
template<>
struct fex_gen_config<vkCmdWriteTimestamp2> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkQueueSubmit2> {};
#else
template<>
struct fex_gen_config<vkQueueSubmit2> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdCopyBuffer2> {};
#else
template<>
struct fex_gen_config<vkCmdCopyBuffer2> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdCopyImage2> {};
#else
template<>
struct fex_gen_config<vkCmdCopyImage2> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdCopyBufferToImage2> {};
#else
template<>
struct fex_gen_config<vkCmdCopyBufferToImage2> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdCopyImageToBuffer2> {};
#else
template<>
struct fex_gen_config<vkCmdCopyImageToBuffer2> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdBlitImage2> {};
#else
template<>
struct fex_gen_config<vkCmdBlitImage2> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdResolveImage2> {};
#else
template<>
struct fex_gen_config<vkCmdResolveImage2> {};
#endif
template<>
struct fex_gen_config<vkCmdBeginRendering> {};
template<>
struct fex_gen_config<vkCmdEndRendering> {};
template<>
struct fex_gen_config<vkCmdSetCullMode> {};
template<>
struct fex_gen_config<vkCmdSetFrontFace> {};
template<>
struct fex_gen_config<vkCmdSetPrimitiveTopology> {};
template<>
struct fex_gen_config<vkCmdSetViewportWithCount> {};
template<>
struct fex_gen_config<vkCmdSetScissorWithCount> {};
template<>
struct fex_gen_config<vkCmdBindVertexBuffers2> {};
template<>
struct fex_gen_config<vkCmdSetDepthTestEnable> {};
template<>
struct fex_gen_config<vkCmdSetDepthWriteEnable> {};
template<>
struct fex_gen_config<vkCmdSetDepthCompareOp> {};
template<>
struct fex_gen_config<vkCmdSetDepthBoundsTestEnable> {};
template<>
struct fex_gen_config<vkCmdSetStencilTestEnable> {};
template<>
struct fex_gen_config<vkCmdSetStencilOp> {};
template<>
struct fex_gen_config<vkCmdSetRasterizerDiscardEnable> {};
template<>
struct fex_gen_config<vkCmdSetDepthBiasEnable> {};
template<>
struct fex_gen_config<vkCmdSetPrimitiveRestartEnable> {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetDeviceBufferMemoryRequirements> {};
#else
template<>
struct fex_gen_config<vkGetDeviceBufferMemoryRequirements> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetDeviceImageMemoryRequirements> {};
#else
template<>
struct fex_gen_config<vkGetDeviceImageMemoryRequirements> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetDeviceImageSparseMemoryRequirements> {};
#else
template<>
struct fex_gen_config<vkGetDeviceImageSparseMemoryRequirements> {};
#endif
template<>
struct fex_gen_config<vkCmdSetLineStipple> {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkMapMemory2> {};
#else
template<>
struct fex_gen_config<vkMapMemory2> {};
#endif
template<>
struct fex_gen_config<vkUnmapMemory2> {};
template<>
struct fex_gen_config<vkCmdBindIndexBuffer2> {};
template<>
struct fex_gen_config<vkGetRenderingAreaGranularity> {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetDeviceImageSubresourceLayout> {};
#else
template<>
struct fex_gen_config<vkGetDeviceImageSubresourceLayout> {};
#endif
template<>
struct fex_gen_config<vkGetImageSubresourceLayout2> {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdPushDescriptorSet> {};
#else
template<>
struct fex_gen_config<vkCmdPushDescriptorSet> {};
#endif
template<>
struct fex_gen_config<vkCmdPushDescriptorSetWithTemplate> {};
template<>
struct fex_gen_param<vkCmdPushDescriptorSetWithTemplate, 4, const void*> : fexgen::assume_compatible_data_layout {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdSetRenderingAttachmentLocations> {};
#else
template<>
struct fex_gen_config<vkCmdSetRenderingAttachmentLocations> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdSetRenderingInputAttachmentIndices> {};
#else
template<>
struct fex_gen_config<vkCmdSetRenderingInputAttachmentIndices> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdBindDescriptorSets2> {};
#else
template<>
struct fex_gen_config<vkCmdBindDescriptorSets2> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdPushConstants2> {};
#else
template<>
struct fex_gen_config<vkCmdPushConstants2> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdPushDescriptorSet2> {};
#else
template<>
struct fex_gen_config<vkCmdPushDescriptorSet2> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdPushDescriptorSetWithTemplate2> {};
#else
template<>
struct fex_gen_config<vkCmdPushDescriptorSetWithTemplate2> {};
#endif
template<>
struct fex_gen_config<vkCopyMemoryToImage> {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCopyImageToMemory> {};
#else
template<>
struct fex_gen_config<vkCopyImageToMemory> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCopyImageToImage> {};
#else
template<>
struct fex_gen_config<vkCopyImageToImage> {};
#endif
template<>
struct fex_gen_config<vkTransitionImageLayout> {};
template<>
struct fex_gen_config<vkDestroySurfaceKHR> {};
template<>
struct fex_gen_config<vkGetPhysicalDeviceSurfaceSupportKHR> {};
template<>
struct fex_gen_config<vkGetPhysicalDeviceSurfaceCapabilitiesKHR> {};
// TODO: Need to figure out how *not* to repack the last parameter on input...
template<>
struct fex_gen_config<vkGetPhysicalDeviceSurfaceFormatsKHR> {};
template<>
struct fex_gen_config<vkGetPhysicalDeviceSurfacePresentModesKHR> {};
template<>
struct fex_gen_config<vkCreateSwapchainKHR> {};
template<>
struct fex_gen_config<vkDestroySwapchainKHR> {};
template<>
struct fex_gen_config<vkGetSwapchainImagesKHR> {};
template<>
struct fex_gen_config<vkAcquireNextImageKHR> {};
template<>
struct fex_gen_config<vkQueuePresentKHR> {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetDeviceGroupPresentCapabilitiesKHR> {};
#else
template<>
struct fex_gen_config<vkGetDeviceGroupPresentCapabilitiesKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetDeviceGroupSurfacePresentModesKHR> {};
#else
template<>
struct fex_gen_config<vkGetDeviceGroupSurfacePresentModesKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetPhysicalDevicePresentRectanglesKHR> {};
#else
template<>
struct fex_gen_config<vkGetPhysicalDevicePresentRectanglesKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkAcquireNextImage2KHR> {};
#else
template<>
struct fex_gen_config<vkAcquireNextImage2KHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetPhysicalDeviceDisplayPropertiesKHR> {};
#else
template<>
struct fex_gen_config<vkGetPhysicalDeviceDisplayPropertiesKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetPhysicalDeviceDisplayPlanePropertiesKHR> {};
#else
template<>
struct fex_gen_config<vkGetPhysicalDeviceDisplayPlanePropertiesKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetDisplayPlaneSupportedDisplaysKHR> {};
#else
template<>
struct fex_gen_config<vkGetDisplayPlaneSupportedDisplaysKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetDisplayModePropertiesKHR> {};
#else
template<>
struct fex_gen_config<vkGetDisplayModePropertiesKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCreateDisplayModeKHR> {};
#else
template<>
struct fex_gen_config<vkCreateDisplayModeKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetDisplayPlaneCapabilitiesKHR> {};
#else
template<>
struct fex_gen_config<vkGetDisplayPlaneCapabilitiesKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCreateDisplayPlaneSurfaceKHR> {};
#else
template<>
struct fex_gen_config<vkCreateDisplayPlaneSurfaceKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCreateSharedSwapchainsKHR> {};
#else
template<>
struct fex_gen_config<vkCreateSharedSwapchainsKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetPhysicalDeviceVideoCapabilitiesKHR> {};
#else
template<>
struct fex_gen_config<vkGetPhysicalDeviceVideoCapabilitiesKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetPhysicalDeviceVideoFormatPropertiesKHR> {};
#else
template<>
struct fex_gen_config<vkGetPhysicalDeviceVideoFormatPropertiesKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCreateVideoSessionKHR> {};
#else
template<>
struct fex_gen_config<vkCreateVideoSessionKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkDestroyVideoSessionKHR> {};
#else
template<>
struct fex_gen_config<vkDestroyVideoSessionKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetVideoSessionMemoryRequirementsKHR> {};
#else
template<>
struct fex_gen_config<vkGetVideoSessionMemoryRequirementsKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkBindVideoSessionMemoryKHR> {};
#else
template<>
struct fex_gen_config<vkBindVideoSessionMemoryKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCreateVideoSessionParametersKHR> {};
#else
template<>
struct fex_gen_config<vkCreateVideoSessionParametersKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkUpdateVideoSessionParametersKHR> {};
#else
template<>
struct fex_gen_config<vkUpdateVideoSessionParametersKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkDestroyVideoSessionParametersKHR> {};
#else
template<>
struct fex_gen_config<vkDestroyVideoSessionParametersKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdBeginVideoCodingKHR> {};
#else
template<>
struct fex_gen_config<vkCmdBeginVideoCodingKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdEndVideoCodingKHR> {};
#else
template<>
struct fex_gen_config<vkCmdEndVideoCodingKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdControlVideoCodingKHR> {};
#else
template<>
struct fex_gen_config<vkCmdControlVideoCodingKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdDecodeVideoKHR> {};
#else
template<>
struct fex_gen_config<vkCmdDecodeVideoKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdBeginRenderingKHR> {};
#else
template<>
struct fex_gen_config<vkCmdBeginRenderingKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdEndRenderingKHR> {};
#else
template<>
struct fex_gen_config<vkCmdEndRenderingKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetPhysicalDeviceFeatures2KHR> {};
#else
template<>
struct fex_gen_config<vkGetPhysicalDeviceFeatures2KHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetPhysicalDeviceProperties2KHR> {};
#else
template<>
struct fex_gen_config<vkGetPhysicalDeviceProperties2KHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetPhysicalDeviceFormatProperties2KHR> {};
#else
template<>
struct fex_gen_config<vkGetPhysicalDeviceFormatProperties2KHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetPhysicalDeviceImageFormatProperties2KHR> {};
#else
template<>
struct fex_gen_config<vkGetPhysicalDeviceImageFormatProperties2KHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetPhysicalDeviceQueueFamilyProperties2KHR> {};
#else
template<>
struct fex_gen_config<vkGetPhysicalDeviceQueueFamilyProperties2KHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetPhysicalDeviceMemoryProperties2KHR> {};
#else
template<>
struct fex_gen_config<vkGetPhysicalDeviceMemoryProperties2KHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetPhysicalDeviceSparseImageFormatProperties2KHR> {};
#else
template<>
struct fex_gen_config<vkGetPhysicalDeviceSparseImageFormatProperties2KHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetDeviceGroupPeerMemoryFeaturesKHR> {};
#else
template<>
struct fex_gen_config<vkGetDeviceGroupPeerMemoryFeaturesKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdSetDeviceMaskKHR> {};
#else
template<>
struct fex_gen_config<vkCmdSetDeviceMaskKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdDispatchBaseKHR> {};
#else
template<>
struct fex_gen_config<vkCmdDispatchBaseKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkTrimCommandPoolKHR> {};
#else
template<>
struct fex_gen_config<vkTrimCommandPoolKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkEnumeratePhysicalDeviceGroupsKHR> {};
#else
template<>
struct fex_gen_config<vkEnumeratePhysicalDeviceGroupsKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetPhysicalDeviceExternalBufferPropertiesKHR> {};
#else
template<>
struct fex_gen_config<vkGetPhysicalDeviceExternalBufferPropertiesKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetMemoryFdKHR> {};
#else
template<>
struct fex_gen_config<vkGetMemoryFdKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetMemoryFdPropertiesKHR> {};
#else
template<>
struct fex_gen_config<vkGetMemoryFdPropertiesKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetPhysicalDeviceExternalSemaphorePropertiesKHR> {};
#else
template<>
struct fex_gen_config<vkGetPhysicalDeviceExternalSemaphorePropertiesKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkImportSemaphoreFdKHR> {};
#else
template<>
struct fex_gen_config<vkImportSemaphoreFdKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetSemaphoreFdKHR> {};
#else
template<>
struct fex_gen_config<vkGetSemaphoreFdKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdPushDescriptorSetKHR> {};
#else
template<>
struct fex_gen_config<vkCmdPushDescriptorSetKHR> {};
#endif
template<>
struct fex_gen_config<vkCmdPushDescriptorSetWithTemplateKHR> {};
template<>
struct fex_gen_param<vkCmdPushDescriptorSetWithTemplateKHR, 4, const void*> : fexgen::assume_compatible_data_layout {};
template<>
struct fex_gen_config<vkCreateDescriptorUpdateTemplateKHR> {};
template<>
struct fex_gen_config<vkDestroyDescriptorUpdateTemplateKHR> {};
template<>
struct fex_gen_config<vkUpdateDescriptorSetWithTemplateKHR> {};
template<>
struct fex_gen_param<vkUpdateDescriptorSetWithTemplateKHR, 3, const void*> : fexgen::assume_compatible_data_layout {};
template<>
struct fex_gen_config<vkCreateRenderPass2KHR> {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdBeginRenderPass2KHR> {};
#else
template<>
struct fex_gen_config<vkCmdBeginRenderPass2KHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdNextSubpass2KHR> {};
#else
template<>
struct fex_gen_config<vkCmdNextSubpass2KHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdEndRenderPass2KHR> {};
#else
template<>
struct fex_gen_config<vkCmdEndRenderPass2KHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetSwapchainStatusKHR> {};
#else
template<>
struct fex_gen_config<vkGetSwapchainStatusKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetPhysicalDeviceExternalFencePropertiesKHR> {};
#else
template<>
struct fex_gen_config<vkGetPhysicalDeviceExternalFencePropertiesKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkImportFenceFdKHR> {};
#else
template<>
struct fex_gen_config<vkImportFenceFdKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetFenceFdKHR> {};
#else
template<>
struct fex_gen_config<vkGetFenceFdKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR> {};
#else
template<>
struct fex_gen_config<vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR> {};
#else
template<>
struct fex_gen_config<vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkAcquireProfilingLockKHR> {};
#else
template<>
struct fex_gen_config<vkAcquireProfilingLockKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkReleaseProfilingLockKHR> {};
#else
template<>
struct fex_gen_config<vkReleaseProfilingLockKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetPhysicalDeviceSurfaceCapabilities2KHR> {};
#else
template<>
struct fex_gen_config<vkGetPhysicalDeviceSurfaceCapabilities2KHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetPhysicalDeviceSurfaceFormats2KHR> {};
#else
template<>
struct fex_gen_config<vkGetPhysicalDeviceSurfaceFormats2KHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetPhysicalDeviceDisplayProperties2KHR> {};
#else
template<>
struct fex_gen_config<vkGetPhysicalDeviceDisplayProperties2KHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetPhysicalDeviceDisplayPlaneProperties2KHR> {};
#else
template<>
struct fex_gen_config<vkGetPhysicalDeviceDisplayPlaneProperties2KHR> {};
#endif
template<>
struct fex_gen_config<vkGetDisplayModeProperties2KHR> {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetDisplayPlaneCapabilities2KHR> {};
#else
template<>
struct fex_gen_config<vkGetDisplayPlaneCapabilities2KHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetImageMemoryRequirements2KHR> {};
#else
template<>
struct fex_gen_config<vkGetImageMemoryRequirements2KHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetBufferMemoryRequirements2KHR> {};
#else
template<>
struct fex_gen_config<vkGetBufferMemoryRequirements2KHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetImageSparseMemoryRequirements2KHR> {};
#else
template<>
struct fex_gen_config<vkGetImageSparseMemoryRequirements2KHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCreateSamplerYcbcrConversionKHR> {};
#else
template<>
struct fex_gen_config<vkCreateSamplerYcbcrConversionKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkDestroySamplerYcbcrConversionKHR> {};
#else
template<>
struct fex_gen_config<vkDestroySamplerYcbcrConversionKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkBindBufferMemory2KHR> {};
#else
template<>
struct fex_gen_config<vkBindBufferMemory2KHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkBindImageMemory2KHR> {};
#else
template<>
struct fex_gen_config<vkBindImageMemory2KHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetDescriptorSetLayoutSupportKHR> {};
#else
template<>
struct fex_gen_config<vkGetDescriptorSetLayoutSupportKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdDrawIndirectCountKHR> {};
#else
template<>
struct fex_gen_config<vkCmdDrawIndirectCountKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdDrawIndexedIndirectCountKHR> {};
#else
template<>
struct fex_gen_config<vkCmdDrawIndexedIndirectCountKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetSemaphoreCounterValueKHR> {};
#else
template<>
struct fex_gen_config<vkGetSemaphoreCounterValueKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkWaitSemaphoresKHR> {};
#else
template<>
struct fex_gen_config<vkWaitSemaphoresKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkSignalSemaphoreKHR> {};
#else
template<>
struct fex_gen_config<vkSignalSemaphoreKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetPhysicalDeviceFragmentShadingRatesKHR> {};
#else
template<>
struct fex_gen_config<vkGetPhysicalDeviceFragmentShadingRatesKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdSetFragmentShadingRateKHR> {};
#else
template<>
struct fex_gen_config<vkCmdSetFragmentShadingRateKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdSetRenderingAttachmentLocationsKHR> {};
#else
template<>
struct fex_gen_config<vkCmdSetRenderingAttachmentLocationsKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdSetRenderingInputAttachmentIndicesKHR> {};
#else
template<>
struct fex_gen_config<vkCmdSetRenderingInputAttachmentIndicesKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkWaitForPresentKHR> {};
#else
template<>
struct fex_gen_config<vkWaitForPresentKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetBufferDeviceAddressKHR> {};
#else
template<>
struct fex_gen_config<vkGetBufferDeviceAddressKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetBufferOpaqueCaptureAddressKHR> {};
#else
template<>
struct fex_gen_config<vkGetBufferOpaqueCaptureAddressKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetDeviceMemoryOpaqueCaptureAddressKHR> {};
#else
template<>
struct fex_gen_config<vkGetDeviceMemoryOpaqueCaptureAddressKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCreateDeferredOperationKHR> {};
#else
template<>
struct fex_gen_config<vkCreateDeferredOperationKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkDestroyDeferredOperationKHR> {};
#else
template<>
struct fex_gen_config<vkDestroyDeferredOperationKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetDeferredOperationMaxConcurrencyKHR> {};
#else
template<>
struct fex_gen_config<vkGetDeferredOperationMaxConcurrencyKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetDeferredOperationResultKHR> {};
#else
template<>
struct fex_gen_config<vkGetDeferredOperationResultKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkDeferredOperationJoinKHR> {};
#else
template<>
struct fex_gen_config<vkDeferredOperationJoinKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetPipelineExecutablePropertiesKHR> {};
#else
template<>
struct fex_gen_config<vkGetPipelineExecutablePropertiesKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetPipelineExecutableStatisticsKHR> {};
#else
template<>
struct fex_gen_config<vkGetPipelineExecutableStatisticsKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetPipelineExecutableInternalRepresentationsKHR> {};
#else
template<>
struct fex_gen_config<vkGetPipelineExecutableInternalRepresentationsKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkMapMemory2KHR> {};
#else
template<>
struct fex_gen_config<vkMapMemory2KHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkUnmapMemory2KHR> {};
#else
template<>
struct fex_gen_config<vkUnmapMemory2KHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetPhysicalDeviceVideoEncodeQualityLevelPropertiesKHR> {};
#else
template<>
struct fex_gen_config<vkGetPhysicalDeviceVideoEncodeQualityLevelPropertiesKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetEncodedVideoSessionParametersKHR> {};
#else
template<>
struct fex_gen_config<vkGetEncodedVideoSessionParametersKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdEncodeVideoKHR> {};
#else
template<>
struct fex_gen_config<vkCmdEncodeVideoKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdSetEvent2KHR> {};
#else
template<>
struct fex_gen_config<vkCmdSetEvent2KHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdResetEvent2KHR> {};
#else
template<>
struct fex_gen_config<vkCmdResetEvent2KHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdWaitEvents2KHR> {};
#else
template<>
struct fex_gen_config<vkCmdWaitEvents2KHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdPipelineBarrier2KHR> {};
#else
template<>
struct fex_gen_config<vkCmdPipelineBarrier2KHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdWriteTimestamp2KHR> {};
#else
template<>
struct fex_gen_config<vkCmdWriteTimestamp2KHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkQueueSubmit2KHR> {};
#else
template<>
struct fex_gen_config<vkQueueSubmit2KHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdCopyBuffer2KHR> {};
#else
template<>
struct fex_gen_config<vkCmdCopyBuffer2KHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdCopyImage2KHR> {};
#else
template<>
struct fex_gen_config<vkCmdCopyImage2KHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdCopyBufferToImage2KHR> {};
#else
template<>
struct fex_gen_config<vkCmdCopyBufferToImage2KHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdCopyImageToBuffer2KHR> {};
#else
template<>
struct fex_gen_config<vkCmdCopyImageToBuffer2KHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdBlitImage2KHR> {};
#else
template<>
struct fex_gen_config<vkCmdBlitImage2KHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdResolveImage2KHR> {};
#else
template<>
struct fex_gen_config<vkCmdResolveImage2KHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdTraceRaysIndirect2KHR> {};
#else
template<>
struct fex_gen_config<vkCmdTraceRaysIndirect2KHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetDeviceBufferMemoryRequirementsKHR> {};
#else
template<>
struct fex_gen_config<vkGetDeviceBufferMemoryRequirementsKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetDeviceImageMemoryRequirementsKHR> {};
#else
template<>
struct fex_gen_config<vkGetDeviceImageMemoryRequirementsKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetDeviceImageSparseMemoryRequirementsKHR> {};
#else
template<>
struct fex_gen_config<vkGetDeviceImageSparseMemoryRequirementsKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdBindIndexBuffer2KHR> {};
#else
template<>
struct fex_gen_config<vkCmdBindIndexBuffer2KHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetRenderingAreaGranularityKHR> {};
#else
template<>
struct fex_gen_config<vkGetRenderingAreaGranularityKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetDeviceImageSubresourceLayoutKHR> {};
#else
template<>
struct fex_gen_config<vkGetDeviceImageSubresourceLayoutKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetImageSubresourceLayout2KHR> {};
#else
template<>
struct fex_gen_config<vkGetImageSubresourceLayout2KHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCreatePipelineBinariesKHR> {};
#else
template<>
struct fex_gen_config<vkCreatePipelineBinariesKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkDestroyPipelineBinaryKHR> {};
#else
template<>
struct fex_gen_config<vkDestroyPipelineBinaryKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetPipelineKeyKHR> {};
#else
template<>
struct fex_gen_config<vkGetPipelineKeyKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetPipelineBinaryDataKHR> {};
#else
template<>
struct fex_gen_config<vkGetPipelineBinaryDataKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkReleaseCapturedPipelineDataKHR> {};
#else
template<>
struct fex_gen_config<vkReleaseCapturedPipelineDataKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetPhysicalDeviceCooperativeMatrixPropertiesKHR> {};
#else
template<>
struct fex_gen_config<vkGetPhysicalDeviceCooperativeMatrixPropertiesKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdSetLineStippleKHR> {};
#else
template<>
struct fex_gen_config<vkCmdSetLineStippleKHR> {};
#endif
template<>
struct fex_gen_config<vkGetPhysicalDeviceCalibrateableTimeDomainsKHR> {};
template<>
struct fex_gen_config<vkGetCalibratedTimestampsKHR> {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdBindDescriptorSets2KHR> {};
#else
template<>
struct fex_gen_config<vkCmdBindDescriptorSets2KHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdPushConstants2KHR> {};
#else
template<>
struct fex_gen_config<vkCmdPushConstants2KHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdPushDescriptorSet2KHR> {};
#else
template<>
struct fex_gen_config<vkCmdPushDescriptorSet2KHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdPushDescriptorSetWithTemplate2KHR> {};
#else
template<>
struct fex_gen_config<vkCmdPushDescriptorSetWithTemplate2KHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdSetDescriptorBufferOffsets2EXT> {};
#else
template<>
struct fex_gen_config<vkCmdSetDescriptorBufferOffsets2EXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdBindDescriptorBufferEmbeddedSamplers2EXT> {};
#else
template<>
struct fex_gen_config<vkCmdBindDescriptorBufferEmbeddedSamplers2EXT> {};
#endif
template<>
struct fex_gen_config<vkCreateDebugReportCallbackEXT> : fexgen::custom_host_impl {};
template<>
struct fex_gen_param<vkCreateDebugReportCallbackEXT, 1, const VkDebugReportCallbackCreateInfoEXT*> : fexgen::ptr_passthrough {};
template<>
struct fex_gen_config<vkDestroyDebugReportCallbackEXT> : fexgen::custom_host_impl {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkDebugReportMessageEXT> {};
#else
template<>
struct fex_gen_config<vkDebugReportMessageEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkDebugMarkerSetObjectTagEXT> {};
#else
template<>
struct fex_gen_config<vkDebugMarkerSetObjectTagEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkDebugMarkerSetObjectNameEXT> {};
#else
template<>
struct fex_gen_config<vkDebugMarkerSetObjectNameEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdDebugMarkerBeginEXT> {};
#else
template<>
struct fex_gen_config<vkCmdDebugMarkerBeginEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdDebugMarkerEndEXT> {};
#else
template<>
struct fex_gen_config<vkCmdDebugMarkerEndEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdDebugMarkerInsertEXT> {};
#else
template<>
struct fex_gen_config<vkCmdDebugMarkerInsertEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdBindTransformFeedbackBuffersEXT> {};
#else
template<>
struct fex_gen_config<vkCmdBindTransformFeedbackBuffersEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdBeginTransformFeedbackEXT> {};
#else
template<>
struct fex_gen_config<vkCmdBeginTransformFeedbackEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdEndTransformFeedbackEXT> {};
#else
template<>
struct fex_gen_config<vkCmdEndTransformFeedbackEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdBeginQueryIndexedEXT> {};
#else
template<>
struct fex_gen_config<vkCmdBeginQueryIndexedEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdEndQueryIndexedEXT> {};
#else
template<>
struct fex_gen_config<vkCmdEndQueryIndexedEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdDrawIndirectByteCountEXT> {};
#else
template<>
struct fex_gen_config<vkCmdDrawIndirectByteCountEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCreateCuModuleNVX> {};
#else
template<>
struct fex_gen_config<vkCreateCuModuleNVX> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCreateCuFunctionNVX> {};
#else
template<>
struct fex_gen_config<vkCreateCuFunctionNVX> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkDestroyCuModuleNVX> {};
#else
template<>
struct fex_gen_config<vkDestroyCuModuleNVX> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkDestroyCuFunctionNVX> {};
#else
template<>
struct fex_gen_config<vkDestroyCuFunctionNVX> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdCuLaunchKernelNVX> {};
#else
template<>
struct fex_gen_config<vkCmdCuLaunchKernelNVX> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetImageViewHandleNVX> {};
#else
template<>
struct fex_gen_config<vkGetImageViewHandleNVX> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetImageViewHandle64NVX> {};
#else
template<>
struct fex_gen_config<vkGetImageViewHandle64NVX> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetImageViewAddressNVX> {};
#else
template<>
struct fex_gen_config<vkGetImageViewAddressNVX> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdDrawIndirectCountAMD> {};
#else
template<>
struct fex_gen_config<vkCmdDrawIndirectCountAMD> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdDrawIndexedIndirectCountAMD> {};
#else
template<>
struct fex_gen_config<vkCmdDrawIndexedIndirectCountAMD> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetShaderInfoAMD> {};
#else
template<>
struct fex_gen_config<vkGetShaderInfoAMD> {};
#endif
template<>
struct fex_gen_param<vkGetShaderInfoAMD, 5, void*> : fexgen::assume_compatible_data_layout {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetPhysicalDeviceExternalImageFormatPropertiesNV> {};
#else
template<>
struct fex_gen_config<vkGetPhysicalDeviceExternalImageFormatPropertiesNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdBeginConditionalRenderingEXT> {};
#else
template<>
struct fex_gen_config<vkCmdBeginConditionalRenderingEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdEndConditionalRenderingEXT> {};
#else
template<>
struct fex_gen_config<vkCmdEndConditionalRenderingEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdSetViewportWScalingNV> {};
#else
template<>
struct fex_gen_config<vkCmdSetViewportWScalingNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkReleaseDisplayEXT> {};
#else
template<>
struct fex_gen_config<vkReleaseDisplayEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetPhysicalDeviceSurfaceCapabilities2EXT> {};
#else
template<>
struct fex_gen_config<vkGetPhysicalDeviceSurfaceCapabilities2EXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkDisplayPowerControlEXT> {};
#else
template<>
struct fex_gen_config<vkDisplayPowerControlEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkRegisterDeviceEventEXT> {};
#else
template<>
struct fex_gen_config<vkRegisterDeviceEventEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkRegisterDisplayEventEXT> {};
#else
template<>
struct fex_gen_config<vkRegisterDisplayEventEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetSwapchainCounterEXT> {};
#else
template<>
struct fex_gen_config<vkGetSwapchainCounterEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetRefreshCycleDurationGOOGLE> {};
#else
template<>
struct fex_gen_config<vkGetRefreshCycleDurationGOOGLE> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetPastPresentationTimingGOOGLE> {};
#else
template<>
struct fex_gen_config<vkGetPastPresentationTimingGOOGLE> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdSetDiscardRectangleEXT> {};
#else
template<>
struct fex_gen_config<vkCmdSetDiscardRectangleEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdSetDiscardRectangleEnableEXT> {};
#else
template<>
struct fex_gen_config<vkCmdSetDiscardRectangleEnableEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdSetDiscardRectangleModeEXT> {};
#else
template<>
struct fex_gen_config<vkCmdSetDiscardRectangleModeEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkSetHdrMetadataEXT> {};
#else
template<>
struct fex_gen_config<vkSetHdrMetadataEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkSetDebugUtilsObjectNameEXT> {};
#else
template<>
struct fex_gen_config<vkSetDebugUtilsObjectNameEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkSetDebugUtilsObjectTagEXT> {};
#else
template<>
struct fex_gen_config<vkSetDebugUtilsObjectTagEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkQueueBeginDebugUtilsLabelEXT> {};
#else
template<>
struct fex_gen_config<vkQueueBeginDebugUtilsLabelEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkQueueEndDebugUtilsLabelEXT> {};
#else
template<>
struct fex_gen_config<vkQueueEndDebugUtilsLabelEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkQueueInsertDebugUtilsLabelEXT> {};
#else
template<>
struct fex_gen_config<vkQueueInsertDebugUtilsLabelEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdBeginDebugUtilsLabelEXT> {};
#else
template<>
struct fex_gen_config<vkCmdBeginDebugUtilsLabelEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdEndDebugUtilsLabelEXT> {};
#else
template<>
struct fex_gen_config<vkCmdEndDebugUtilsLabelEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdInsertDebugUtilsLabelEXT> {};
#else
template<>
struct fex_gen_config<vkCmdInsertDebugUtilsLabelEXT> {};
#endif
template<>
struct fex_gen_config<vkCreateDebugUtilsMessengerEXT> : fexgen::custom_host_impl {};
template<>
struct fex_gen_param<vkCreateDebugUtilsMessengerEXT, 1, const VkDebugUtilsMessengerCreateInfoEXT*> : fexgen::ptr_passthrough {};
template<>
struct fex_gen_config<vkDestroyDebugUtilsMessengerEXT> {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkSubmitDebugUtilsMessageEXT> {};
#else
template<>
struct fex_gen_config<vkSubmitDebugUtilsMessageEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdSetSampleLocationsEXT> {};
#else
template<>
struct fex_gen_config<vkCmdSetSampleLocationsEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetPhysicalDeviceMultisamplePropertiesEXT> {};
#else
template<>
struct fex_gen_config<vkGetPhysicalDeviceMultisamplePropertiesEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetImageDrmFormatModifierPropertiesEXT> {};
#else
template<>
struct fex_gen_config<vkGetImageDrmFormatModifierPropertiesEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCreateValidationCacheEXT> {};
#else
template<>
struct fex_gen_config<vkCreateValidationCacheEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkDestroyValidationCacheEXT> {};
#else
template<>
struct fex_gen_config<vkDestroyValidationCacheEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkMergeValidationCachesEXT> {};
#else
template<>
struct fex_gen_config<vkMergeValidationCachesEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetValidationCacheDataEXT> {};
#else
template<>
struct fex_gen_config<vkGetValidationCacheDataEXT> {};
#endif
template<>
struct fex_gen_param<vkGetValidationCacheDataEXT, 3, void*> : fexgen::assume_compatible_data_layout {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdBindShadingRateImageNV> {};
#else
template<>
struct fex_gen_config<vkCmdBindShadingRateImageNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdSetViewportShadingRatePaletteNV> {};
#else
template<>
struct fex_gen_config<vkCmdSetViewportShadingRatePaletteNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdSetCoarseSampleOrderNV> {};
#else
template<>
struct fex_gen_config<vkCmdSetCoarseSampleOrderNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCreateAccelerationStructureNV> {};
#else
template<>
struct fex_gen_config<vkCreateAccelerationStructureNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkDestroyAccelerationStructureNV> {};
#else
template<>
struct fex_gen_config<vkDestroyAccelerationStructureNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetAccelerationStructureMemoryRequirementsNV> {};
#else
template<>
struct fex_gen_config<vkGetAccelerationStructureMemoryRequirementsNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkBindAccelerationStructureMemoryNV> {};
#else
template<>
struct fex_gen_config<vkBindAccelerationStructureMemoryNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdBuildAccelerationStructureNV> {};
#else
template<>
struct fex_gen_config<vkCmdBuildAccelerationStructureNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdCopyAccelerationStructureNV> {};
#else
template<>
struct fex_gen_config<vkCmdCopyAccelerationStructureNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdTraceRaysNV> {};
#else
template<>
struct fex_gen_config<vkCmdTraceRaysNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCreateRayTracingPipelinesNV> {};
#else
template<>
struct fex_gen_config<vkCreateRayTracingPipelinesNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetRayTracingShaderGroupHandlesKHR> {};
#else
template<>
struct fex_gen_config<vkGetRayTracingShaderGroupHandlesKHR> {};
#endif
template<>
struct fex_gen_param<vkGetRayTracingShaderGroupHandlesKHR, 5, void*> : fexgen::assume_compatible_data_layout {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetRayTracingShaderGroupHandlesNV> {};
#else
template<>
struct fex_gen_config<vkGetRayTracingShaderGroupHandlesNV> {};
#endif
template<>
struct fex_gen_param<vkGetRayTracingShaderGroupHandlesNV, 5, void*> : fexgen::assume_compatible_data_layout {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetAccelerationStructureHandleNV> {};
#else
template<>
struct fex_gen_config<vkGetAccelerationStructureHandleNV> {};
#endif
template<>
struct fex_gen_param<vkGetAccelerationStructureHandleNV, 3, void*> : fexgen::assume_compatible_data_layout {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdWriteAccelerationStructuresPropertiesNV> {};
#else
template<>
struct fex_gen_config<vkCmdWriteAccelerationStructuresPropertiesNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCompileDeferredNV> {};
#else
template<>
struct fex_gen_config<vkCompileDeferredNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetMemoryHostPointerPropertiesEXT> {};
#else
template<>
struct fex_gen_config<vkGetMemoryHostPointerPropertiesEXT> {};
#endif
template<>
struct fex_gen_param<vkGetMemoryHostPointerPropertiesEXT, 2, const void*> : fexgen::assume_compatible_data_layout {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdWriteBufferMarkerAMD> {};
#else
template<>
struct fex_gen_config<vkCmdWriteBufferMarkerAMD> {};
#endif
template<>
struct fex_gen_config<vkCmdWriteBufferMarker2AMD> {};
template<>
struct fex_gen_config<vkGetPhysicalDeviceCalibrateableTimeDomainsEXT> {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetCalibratedTimestampsEXT> {};
#else
template<>
struct fex_gen_config<vkGetCalibratedTimestampsEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdDrawMeshTasksNV> {};
#else
template<>
struct fex_gen_config<vkCmdDrawMeshTasksNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdDrawMeshTasksIndirectNV> {};
#else
template<>
struct fex_gen_config<vkCmdDrawMeshTasksIndirectNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdDrawMeshTasksIndirectCountNV> {};
#else
template<>
struct fex_gen_config<vkCmdDrawMeshTasksIndirectCountNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdSetExclusiveScissorEnableNV> {};
#else
template<>
struct fex_gen_config<vkCmdSetExclusiveScissorEnableNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdSetExclusiveScissorNV> {};
#else
template<>
struct fex_gen_config<vkCmdSetExclusiveScissorNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdSetCheckpointNV> {};
#else
template<>
struct fex_gen_config<vkCmdSetCheckpointNV> {};
#endif
template<>
struct fex_gen_param<vkCmdSetCheckpointNV, 1, const void*> : fexgen::assume_compatible_data_layout {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetQueueCheckpointDataNV> {};
#else
template<>
struct fex_gen_config<vkGetQueueCheckpointDataNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetQueueCheckpointData2NV> {};
#else
template<>
struct fex_gen_config<vkGetQueueCheckpointData2NV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkInitializePerformanceApiINTEL> {};
#else
template<>
struct fex_gen_config<vkInitializePerformanceApiINTEL> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkUninitializePerformanceApiINTEL> {};
#else
template<>
struct fex_gen_config<vkUninitializePerformanceApiINTEL> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdSetPerformanceMarkerINTEL> {};
#else
template<>
struct fex_gen_config<vkCmdSetPerformanceMarkerINTEL> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdSetPerformanceStreamMarkerINTEL> {};
#else
template<>
struct fex_gen_config<vkCmdSetPerformanceStreamMarkerINTEL> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdSetPerformanceOverrideINTEL> {};
#else
template<>
struct fex_gen_config<vkCmdSetPerformanceOverrideINTEL> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkAcquirePerformanceConfigurationINTEL> {};
#else
template<>
struct fex_gen_config<vkAcquirePerformanceConfigurationINTEL> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkReleasePerformanceConfigurationINTEL> {};
#else
template<>
struct fex_gen_config<vkReleasePerformanceConfigurationINTEL> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkQueueSetPerformanceConfigurationINTEL> {};
#else
template<>
struct fex_gen_config<vkQueueSetPerformanceConfigurationINTEL> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetPerformanceParameterINTEL> {};
#else
template<>
struct fex_gen_config<vkGetPerformanceParameterINTEL> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkSetLocalDimmingAMD> {};
#else
template<>
struct fex_gen_config<vkSetLocalDimmingAMD> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetBufferDeviceAddressEXT> {};
#else
template<>
struct fex_gen_config<vkGetBufferDeviceAddressEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetPhysicalDeviceToolPropertiesEXT> {};
#else
template<>
struct fex_gen_config<vkGetPhysicalDeviceToolPropertiesEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetPhysicalDeviceCooperativeMatrixPropertiesNV> {};
#else
template<>
struct fex_gen_config<vkGetPhysicalDeviceCooperativeMatrixPropertiesNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV> {};
#else
template<>
struct fex_gen_config<vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCreateHeadlessSurfaceEXT> {};
#else
template<>
struct fex_gen_config<vkCreateHeadlessSurfaceEXT> {};
#endif
template<>
struct fex_gen_config<vkCmdSetLineStippleEXT> {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkResetQueryPoolEXT> {};
#else
template<>
struct fex_gen_config<vkResetQueryPoolEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdSetCullModeEXT> {};
#else
template<>
struct fex_gen_config<vkCmdSetCullModeEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdSetFrontFaceEXT> {};
#else
template<>
struct fex_gen_config<vkCmdSetFrontFaceEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdSetPrimitiveTopologyEXT> {};
#else
template<>
struct fex_gen_config<vkCmdSetPrimitiveTopologyEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdSetViewportWithCountEXT> {};
#else
template<>
struct fex_gen_config<vkCmdSetViewportWithCountEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdSetScissorWithCountEXT> {};
#else
template<>
struct fex_gen_config<vkCmdSetScissorWithCountEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdBindVertexBuffers2EXT> {};
#else
template<>
struct fex_gen_config<vkCmdBindVertexBuffers2EXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdSetDepthTestEnableEXT> {};
#else
template<>
struct fex_gen_config<vkCmdSetDepthTestEnableEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdSetDepthWriteEnableEXT> {};
#else
template<>
struct fex_gen_config<vkCmdSetDepthWriteEnableEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdSetDepthCompareOpEXT> {};
#else
template<>
struct fex_gen_config<vkCmdSetDepthCompareOpEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdSetDepthBoundsTestEnableEXT> {};
#else
template<>
struct fex_gen_config<vkCmdSetDepthBoundsTestEnableEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdSetStencilTestEnableEXT> {};
#else
template<>
struct fex_gen_config<vkCmdSetStencilTestEnableEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdSetStencilOpEXT> {};
#else
template<>
struct fex_gen_config<vkCmdSetStencilOpEXT> {};
#endif
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCopyMemoryToImageEXT> {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCopyImageToMemoryEXT> {};
#else
template<>
struct fex_gen_config<vkCopyImageToMemoryEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCopyImageToImageEXT> {};
#else
template<>
struct fex_gen_config<vkCopyImageToImageEXT> {};
#endif
template<>
struct fex_gen_config<vkTransitionImageLayoutEXT> {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetImageSubresourceLayout2EXT> {};
#else
template<>
struct fex_gen_config<vkGetImageSubresourceLayout2EXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkReleaseSwapchainImagesEXT> {};
#else
template<>
struct fex_gen_config<vkReleaseSwapchainImagesEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetGeneratedCommandsMemoryRequirementsNV> {};
#else
template<>
struct fex_gen_config<vkGetGeneratedCommandsMemoryRequirementsNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdPreprocessGeneratedCommandsNV> {};
#else
template<>
struct fex_gen_config<vkCmdPreprocessGeneratedCommandsNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdExecuteGeneratedCommandsNV> {};
#else
template<>
struct fex_gen_config<vkCmdExecuteGeneratedCommandsNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdBindPipelineShaderGroupNV> {};
#else
template<>
struct fex_gen_config<vkCmdBindPipelineShaderGroupNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCreateIndirectCommandsLayoutNV> {};
#else
template<>
struct fex_gen_config<vkCreateIndirectCommandsLayoutNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkDestroyIndirectCommandsLayoutNV> {};
#else
template<>
struct fex_gen_config<vkDestroyIndirectCommandsLayoutNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdSetDepthBias2EXT> {};
#else
template<>
struct fex_gen_config<vkCmdSetDepthBias2EXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkAcquireDrmDisplayEXT> {};
#else
template<>
struct fex_gen_config<vkAcquireDrmDisplayEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetDrmDisplayEXT> {};
#else
template<>
struct fex_gen_config<vkGetDrmDisplayEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCreatePrivateDataSlotEXT> {};
#else
template<>
struct fex_gen_config<vkCreatePrivateDataSlotEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkDestroyPrivateDataSlotEXT> {};
#else
template<>
struct fex_gen_config<vkDestroyPrivateDataSlotEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkSetPrivateDataEXT> {};
#else
template<>
struct fex_gen_config<vkSetPrivateDataEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetPrivateDataEXT> {};
#else
template<>
struct fex_gen_config<vkGetPrivateDataEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCreateCudaModuleNV> {};
#else
template<>
struct fex_gen_config<vkCreateCudaModuleNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetCudaModuleCacheNV> {};
#else
template<>
struct fex_gen_config<vkGetCudaModuleCacheNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCreateCudaFunctionNV> {};
#else
template<>
struct fex_gen_config<vkCreateCudaFunctionNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkDestroyCudaModuleNV> {};
#else
template<>
struct fex_gen_config<vkDestroyCudaModuleNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkDestroyCudaFunctionNV> {};
#else
template<>
struct fex_gen_config<vkDestroyCudaFunctionNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdCudaLaunchKernelNV> {};
#else
template<>
struct fex_gen_config<vkCmdCudaLaunchKernelNV> {};
#endif
template<>
struct fex_gen_config<vkGetDescriptorSetLayoutSizeEXT> {};
template<>
struct fex_gen_config<vkGetDescriptorSetLayoutBindingOffsetEXT> {};
template<>
struct fex_gen_config<vkGetDescriptorEXT> {};
template<>
struct fex_gen_param<vkGetDescriptorEXT, 3, void*> : fexgen::assume_compatible_data_layout {};
template<>
struct fex_gen_config<vkCmdBindDescriptorBuffersEXT> {};
template<>
struct fex_gen_config<vkCmdSetDescriptorBufferOffsetsEXT> {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdBindDescriptorBufferEmbeddedSamplersEXT> {};
#else
template<>
struct fex_gen_config<vkCmdBindDescriptorBufferEmbeddedSamplersEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetBufferOpaqueCaptureDescriptorDataEXT> {};
#else
template<>
struct fex_gen_config<vkGetBufferOpaqueCaptureDescriptorDataEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetImageOpaqueCaptureDescriptorDataEXT> {};
#else
template<>
struct fex_gen_config<vkGetImageOpaqueCaptureDescriptorDataEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetImageViewOpaqueCaptureDescriptorDataEXT> {};
#else
template<>
struct fex_gen_config<vkGetImageViewOpaqueCaptureDescriptorDataEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetSamplerOpaqueCaptureDescriptorDataEXT> {};
#else
template<>
struct fex_gen_config<vkGetSamplerOpaqueCaptureDescriptorDataEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetAccelerationStructureOpaqueCaptureDescriptorDataEXT> {};
#else
template<>
struct fex_gen_config<vkGetAccelerationStructureOpaqueCaptureDescriptorDataEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdSetFragmentShadingRateEnumNV> {};
#else
template<>
struct fex_gen_config<vkCmdSetFragmentShadingRateEnumNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetDeviceFaultInfoEXT> {};
#else
template<>
struct fex_gen_config<vkGetDeviceFaultInfoEXT> {};
#endif
#ifdef IS_32BIT_THUNK
template<>
struct fex_gen_config<vkCmdSetVertexInputEXT> : fexgen::custom_host_impl {};
#else
template<>
struct fex_gen_config<vkCmdSetVertexInputEXT> {};
#endif
#ifdef IS_32BIT_THUNK
template<>
struct fex_gen_param<vkCmdSetVertexInputEXT, 2, const VkVertexInputBindingDescription2EXT*> : fexgen::ptr_passthrough {};
template<>
struct fex_gen_param<vkCmdSetVertexInputEXT, 4, const VkVertexInputAttributeDescription2EXT*> : fexgen::ptr_passthrough {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI> {};
#else
template<>
struct fex_gen_config<vkGetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdSubpassShadingHUAWEI> {};
#else
template<>
struct fex_gen_config<vkCmdSubpassShadingHUAWEI> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdBindInvocationMaskHUAWEI> {};
#else
template<>
struct fex_gen_config<vkCmdBindInvocationMaskHUAWEI> {};
#endif
#ifdef IS_32BIT_THUNK
// VkRemoteAddressNV* expands to void**, so it needs custom repacking on on 32-bit
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetMemoryRemoteAddressNV> {};
#else
template<>
struct fex_gen_config<vkGetMemoryRemoteAddressNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetPipelinePropertiesEXT> {};
#else
template<>
struct fex_gen_config<vkGetPipelinePropertiesEXT> {};
#endif
template<>
struct fex_gen_config<vkCmdSetPatchControlPointsEXT> {};
template<>
struct fex_gen_config<vkCmdSetRasterizerDiscardEnableEXT> {};
template<>
struct fex_gen_config<vkCmdSetDepthBiasEnableEXT> {};
template<>
struct fex_gen_config<vkCmdSetLogicOpEXT> {};
template<>
struct fex_gen_config<vkCmdSetPrimitiveRestartEnableEXT> {};
template<>
struct fex_gen_config<vkCmdSetColorWriteEnableEXT> {};
template<>
struct fex_gen_config<vkCmdDrawMultiEXT> {};
template<>
struct fex_gen_config<vkCmdDrawMultiIndexedEXT> {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCreateMicromapEXT> {};
#else
template<>
struct fex_gen_config<vkCreateMicromapEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkDestroyMicromapEXT> {};
#else
template<>
struct fex_gen_config<vkDestroyMicromapEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdBuildMicromapsEXT> {};
#else
template<>
struct fex_gen_config<vkCmdBuildMicromapsEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkBuildMicromapsEXT> {};
#else
template<>
struct fex_gen_config<vkBuildMicromapsEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCopyMicromapEXT> {};
#else
template<>
struct fex_gen_config<vkCopyMicromapEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCopyMicromapToMemoryEXT> {};
#else
template<>
struct fex_gen_config<vkCopyMicromapToMemoryEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCopyMemoryToMicromapEXT> {};
#else
template<>
struct fex_gen_config<vkCopyMemoryToMicromapEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkWriteMicromapsPropertiesEXT> {};
#else
template<>
struct fex_gen_config<vkWriteMicromapsPropertiesEXT> {};
#endif
template<>
struct fex_gen_param<vkWriteMicromapsPropertiesEXT, 5, void*> : fexgen::assume_compatible_data_layout {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdCopyMicromapEXT> {};
#else
template<>
struct fex_gen_config<vkCmdCopyMicromapEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdCopyMicromapToMemoryEXT> {};
#else
template<>
struct fex_gen_config<vkCmdCopyMicromapToMemoryEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdCopyMemoryToMicromapEXT> {};
#else
template<>
struct fex_gen_config<vkCmdCopyMemoryToMicromapEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdWriteMicromapsPropertiesEXT> {};
#else
template<>
struct fex_gen_config<vkCmdWriteMicromapsPropertiesEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetDeviceMicromapCompatibilityEXT> {};
#else
template<>
struct fex_gen_config<vkGetDeviceMicromapCompatibilityEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetMicromapBuildSizesEXT> {};
#else
template<>
struct fex_gen_config<vkGetMicromapBuildSizesEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdDrawClusterHUAWEI> {};
#else
template<>
struct fex_gen_config<vkCmdDrawClusterHUAWEI> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdDrawClusterIndirectHUAWEI> {};
#else
template<>
struct fex_gen_config<vkCmdDrawClusterIndirectHUAWEI> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkSetDeviceMemoryPriorityEXT> {};
#else
template<>
struct fex_gen_config<vkSetDeviceMemoryPriorityEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetDescriptorSetLayoutHostMappingInfoVALVE> {};
#else
template<>
struct fex_gen_config<vkGetDescriptorSetLayoutHostMappingInfoVALVE> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetDescriptorSetHostMappingVALVE> {};
#else
template<>
struct fex_gen_config<vkGetDescriptorSetHostMappingVALVE> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdCopyMemoryIndirectNV> {};
#else
template<>
struct fex_gen_config<vkCmdCopyMemoryIndirectNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdCopyMemoryToImageIndirectNV> {};
#else
template<>
struct fex_gen_config<vkCmdCopyMemoryToImageIndirectNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdDecompressMemoryNV> {};
#else
template<>
struct fex_gen_config<vkCmdDecompressMemoryNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdDecompressMemoryIndirectCountNV> {};
#else
template<>
struct fex_gen_config<vkCmdDecompressMemoryIndirectCountNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetPipelineIndirectMemoryRequirementsNV> {};
#else
template<>
struct fex_gen_config<vkGetPipelineIndirectMemoryRequirementsNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdUpdatePipelineIndirectBufferNV> {};
#else
template<>
struct fex_gen_config<vkCmdUpdatePipelineIndirectBufferNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetPipelineIndirectDeviceAddressNV> {};
#else
template<>
struct fex_gen_config<vkGetPipelineIndirectDeviceAddressNV> {};
#endif
template<>
struct fex_gen_config<vkCmdSetDepthClampEnableEXT> {};
template<>
struct fex_gen_config<vkCmdSetPolygonModeEXT> {};
template<>
struct fex_gen_config<vkCmdSetRasterizationSamplesEXT> {};
template<>
struct fex_gen_config<vkCmdSetSampleMaskEXT> {};
template<>
struct fex_gen_config<vkCmdSetAlphaToCoverageEnableEXT> {};
template<>
struct fex_gen_config<vkCmdSetAlphaToOneEnableEXT> {};
template<>
struct fex_gen_config<vkCmdSetLogicOpEnableEXT> {};
template<>
struct fex_gen_config<vkCmdSetColorBlendEnableEXT> {};
template<>
struct fex_gen_config<vkCmdSetColorBlendEquationEXT> {};
template<>
struct fex_gen_config<vkCmdSetColorWriteMaskEXT> {};
template<>
struct fex_gen_config<vkCmdSetTessellationDomainOriginEXT> {};
template<>
struct fex_gen_config<vkCmdSetRasterizationStreamEXT> {};
template<>
struct fex_gen_config<vkCmdSetConservativeRasterizationModeEXT> {};
template<>
struct fex_gen_config<vkCmdSetExtraPrimitiveOverestimationSizeEXT> {};
template<>
struct fex_gen_config<vkCmdSetDepthClipEnableEXT> {};
template<>
struct fex_gen_config<vkCmdSetSampleLocationsEnableEXT> {};
template<>
struct fex_gen_config<vkCmdSetColorBlendAdvancedEXT> {};
template<>
struct fex_gen_config<vkCmdSetProvokingVertexModeEXT> {};
template<>
struct fex_gen_config<vkCmdSetLineRasterizationModeEXT> {};
template<>
struct fex_gen_config<vkCmdSetLineStippleEnableEXT> {};
template<>
struct fex_gen_config<vkCmdSetDepthClipNegativeOneToOneEXT> {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdSetViewportWScalingEnableNV> {};
#else
template<>
struct fex_gen_config<vkCmdSetViewportWScalingEnableNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdSetViewportSwizzleNV> {};
#else
template<>
struct fex_gen_config<vkCmdSetViewportSwizzleNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdSetCoverageToColorEnableNV> {};
#else
template<>
struct fex_gen_config<vkCmdSetCoverageToColorEnableNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdSetCoverageToColorLocationNV> {};
#else
template<>
struct fex_gen_config<vkCmdSetCoverageToColorLocationNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdSetCoverageModulationModeNV> {};
#else
template<>
struct fex_gen_config<vkCmdSetCoverageModulationModeNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdSetCoverageModulationTableEnableNV> {};
#else
template<>
struct fex_gen_config<vkCmdSetCoverageModulationTableEnableNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdSetCoverageModulationTableNV> {};
#else
template<>
struct fex_gen_config<vkCmdSetCoverageModulationTableNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdSetShadingRateImageEnableNV> {};
#else
template<>
struct fex_gen_config<vkCmdSetShadingRateImageEnableNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdSetRepresentativeFragmentTestEnableNV> {};
#else
template<>
struct fex_gen_config<vkCmdSetRepresentativeFragmentTestEnableNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdSetCoverageReductionModeNV> {};
#else
template<>
struct fex_gen_config<vkCmdSetCoverageReductionModeNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetShaderModuleIdentifierEXT> {};
#else
template<>
struct fex_gen_config<vkGetShaderModuleIdentifierEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetShaderModuleCreateInfoIdentifierEXT> {};
#else
template<>
struct fex_gen_config<vkGetShaderModuleCreateInfoIdentifierEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetPhysicalDeviceOpticalFlowImageFormatsNV> {};
#else
template<>
struct fex_gen_config<vkGetPhysicalDeviceOpticalFlowImageFormatsNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCreateOpticalFlowSessionNV> {};
#else
template<>
struct fex_gen_config<vkCreateOpticalFlowSessionNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkDestroyOpticalFlowSessionNV> {};
#else
template<>
struct fex_gen_config<vkDestroyOpticalFlowSessionNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkBindOpticalFlowSessionImageNV> {};
#else
template<>
struct fex_gen_config<vkBindOpticalFlowSessionImageNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdOpticalFlowExecuteNV> {};
#else
template<>
struct fex_gen_config<vkCmdOpticalFlowExecuteNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkAntiLagUpdateAMD> {};
#else
template<>
struct fex_gen_config<vkAntiLagUpdateAMD> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCreateShadersEXT> {};
#else
template<>
struct fex_gen_config<vkCreateShadersEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkDestroyShaderEXT> {};
#else
template<>
struct fex_gen_config<vkDestroyShaderEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetShaderBinaryDataEXT> {};
#else
template<>
struct fex_gen_config<vkGetShaderBinaryDataEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdBindShadersEXT> {};
#else
template<>
struct fex_gen_config<vkCmdBindShadersEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdSetDepthClampRangeEXT> {};
#else
template<>
struct fex_gen_config<vkCmdSetDepthClampRangeEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetFramebufferTilePropertiesQCOM> {};
#else
template<>
struct fex_gen_config<vkGetFramebufferTilePropertiesQCOM> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetDynamicRenderingTilePropertiesQCOM> {};
#else
template<>
struct fex_gen_config<vkGetDynamicRenderingTilePropertiesQCOM> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetPhysicalDeviceCooperativeVectorPropertiesNV> {};
#else
template<>
struct fex_gen_config<vkGetPhysicalDeviceCooperativeVectorPropertiesNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkConvertCooperativeVectorMatrixNV> {};
#else
template<>
struct fex_gen_config<vkConvertCooperativeVectorMatrixNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdConvertCooperativeVectorMatrixNV> {};
#else
template<>
struct fex_gen_config<vkCmdConvertCooperativeVectorMatrixNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkSetLatencySleepModeNV> {};
#else
template<>
struct fex_gen_config<vkSetLatencySleepModeNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkLatencySleepNV> {};
#else
template<>
struct fex_gen_config<vkLatencySleepNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkSetLatencyMarkerNV> {};
#else
template<>
struct fex_gen_config<vkSetLatencyMarkerNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetLatencyTimingsNV> {};
#else
template<>
struct fex_gen_config<vkGetLatencyTimingsNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkQueueNotifyOutOfBandNV> {};
#else
template<>
struct fex_gen_config<vkQueueNotifyOutOfBandNV> {};
#endif
template<>
struct fex_gen_config<vkCmdSetAttachmentFeedbackLoopEnableEXT> {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetClusterAccelerationStructureBuildSizesNV> {};
#else
template<>
struct fex_gen_config<vkGetClusterAccelerationStructureBuildSizesNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdBuildClusterAccelerationStructureIndirectNV> {};
#else
template<>
struct fex_gen_config<vkCmdBuildClusterAccelerationStructureIndirectNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetPartitionedAccelerationStructuresBuildSizesNV> {};
#else
template<>
struct fex_gen_config<vkGetPartitionedAccelerationStructuresBuildSizesNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdBuildPartitionedAccelerationStructuresNV> {};
#else
template<>
struct fex_gen_config<vkCmdBuildPartitionedAccelerationStructuresNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetGeneratedCommandsMemoryRequirementsEXT> {};
#else
template<>
struct fex_gen_config<vkGetGeneratedCommandsMemoryRequirementsEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdPreprocessGeneratedCommandsEXT> {};
#else
template<>
struct fex_gen_config<vkCmdPreprocessGeneratedCommandsEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdExecuteGeneratedCommandsEXT> {};
#else
template<>
struct fex_gen_config<vkCmdExecuteGeneratedCommandsEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCreateIndirectCommandsLayoutEXT> {};
#else
template<>
struct fex_gen_config<vkCreateIndirectCommandsLayoutEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkDestroyIndirectCommandsLayoutEXT> {};
#else
template<>
struct fex_gen_config<vkDestroyIndirectCommandsLayoutEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCreateIndirectExecutionSetEXT> {};
#else
template<>
struct fex_gen_config<vkCreateIndirectExecutionSetEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkDestroyIndirectExecutionSetEXT> {};
#else
template<>
struct fex_gen_config<vkDestroyIndirectExecutionSetEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkUpdateIndirectExecutionSetPipelineEXT> {};
#else
template<>
struct fex_gen_config<vkUpdateIndirectExecutionSetPipelineEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkUpdateIndirectExecutionSetShaderEXT> {};
#else
template<>
struct fex_gen_config<vkUpdateIndirectExecutionSetShaderEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetPhysicalDeviceCooperativeMatrixFlexibleDimensionsPropertiesNV> {};
#else
template<>
struct fex_gen_config<vkGetPhysicalDeviceCooperativeMatrixFlexibleDimensionsPropertiesNV> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCreateAccelerationStructureKHR> {};
#else
template<>
struct fex_gen_config<vkCreateAccelerationStructureKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkDestroyAccelerationStructureKHR> {};
#else
template<>
struct fex_gen_config<vkDestroyAccelerationStructureKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdBuildAccelerationStructuresKHR> {};
#else
template<>
struct fex_gen_config<vkCmdBuildAccelerationStructuresKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdBuildAccelerationStructuresIndirectKHR> {};
#else
template<>
struct fex_gen_config<vkCmdBuildAccelerationStructuresIndirectKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkBuildAccelerationStructuresKHR> {};
#else
template<>
struct fex_gen_config<vkBuildAccelerationStructuresKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCopyAccelerationStructureKHR> {};
#else
template<>
struct fex_gen_config<vkCopyAccelerationStructureKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCopyAccelerationStructureToMemoryKHR> {};
#else
template<>
struct fex_gen_config<vkCopyAccelerationStructureToMemoryKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCopyMemoryToAccelerationStructureKHR> {};
#else
template<>
struct fex_gen_config<vkCopyMemoryToAccelerationStructureKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkWriteAccelerationStructuresPropertiesKHR> {};
#else
template<>
struct fex_gen_config<vkWriteAccelerationStructuresPropertiesKHR> {};
#endif
template<>
struct fex_gen_param<vkWriteAccelerationStructuresPropertiesKHR, 5, void*> : fexgen::assume_compatible_data_layout {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdCopyAccelerationStructureKHR> {};
#else
template<>
struct fex_gen_config<vkCmdCopyAccelerationStructureKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdCopyAccelerationStructureToMemoryKHR> {};
#else
template<>
struct fex_gen_config<vkCmdCopyAccelerationStructureToMemoryKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdCopyMemoryToAccelerationStructureKHR> {};
#else
template<>
struct fex_gen_config<vkCmdCopyMemoryToAccelerationStructureKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetAccelerationStructureDeviceAddressKHR> {};
#else
template<>
struct fex_gen_config<vkGetAccelerationStructureDeviceAddressKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdWriteAccelerationStructuresPropertiesKHR> {};
#else
template<>
struct fex_gen_config<vkCmdWriteAccelerationStructuresPropertiesKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetDeviceAccelerationStructureCompatibilityKHR> {};
#else
template<>
struct fex_gen_config<vkGetDeviceAccelerationStructureCompatibilityKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetAccelerationStructureBuildSizesKHR> {};
#else
template<>
struct fex_gen_config<vkGetAccelerationStructureBuildSizesKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdTraceRaysKHR> {};
#else
template<>
struct fex_gen_config<vkCmdTraceRaysKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCreateRayTracingPipelinesKHR> {};
#else
template<>
struct fex_gen_config<vkCreateRayTracingPipelinesKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetRayTracingCaptureReplayShaderGroupHandlesKHR> {};
#else
template<>
struct fex_gen_config<vkGetRayTracingCaptureReplayShaderGroupHandlesKHR> {};
#endif
template<>
struct fex_gen_param<vkGetRayTracingCaptureReplayShaderGroupHandlesKHR, 5, void*> : fexgen::assume_compatible_data_layout {};
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdTraceRaysIndirectKHR> {};
#else
template<>
struct fex_gen_config<vkCmdTraceRaysIndirectKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkGetRayTracingShaderGroupStackSizeKHR> {};
#else
template<>
struct fex_gen_config<vkGetRayTracingShaderGroupStackSizeKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdSetRayTracingPipelineStackSizeKHR> {};
#else
template<>
struct fex_gen_config<vkCmdSetRayTracingPipelineStackSizeKHR> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdDrawMeshTasksEXT> {};
#else
template<>
struct fex_gen_config<vkCmdDrawMeshTasksEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdDrawMeshTasksIndirectEXT> {};
#else
template<>
struct fex_gen_config<vkCmdDrawMeshTasksIndirectEXT> {};
#endif
#ifdef IS_32BIT_THUNK
// TODO: Disabled by order of the JSON.
// template<>
// struct fex_gen_config<vkCmdDrawMeshTasksIndirectCountEXT> {};
#else
template<>
struct fex_gen_config<vkCmdDrawMeshTasksIndirectCountEXT> {};
#endif
template<>
struct fex_gen_config<vkCreateWaylandSurfaceKHR> {};
template<>
struct fex_gen_config<vkGetPhysicalDeviceWaylandPresentationSupportKHR> {};
template<>
struct fex_gen_config<vkCreateXcbSurfaceKHR> {};
template<>
struct fex_gen_config<vkGetPhysicalDeviceXcbPresentationSupportKHR> : fexgen::custom_host_impl {};
template<>
struct fex_gen_param<vkGetPhysicalDeviceXcbPresentationSupportKHR, 2, xcb_connection_t*> : fexgen::ptr_passthrough {};
template<>
struct fex_gen_config<vkCreateXlibSurfaceKHR> {};
template<>
struct fex_gen_config<vkGetPhysicalDeviceXlibPresentationSupportKHR> : fexgen::custom_host_impl {};
template<>
struct fex_gen_param<vkGetPhysicalDeviceXlibPresentationSupportKHR, 2, Display*> : fexgen::ptr_passthrough {};
template<>
struct fex_gen_config<vkAcquireXlibDisplayEXT> : fexgen::custom_host_impl {};
template<>
struct fex_gen_param<vkAcquireXlibDisplayEXT, 1, Display*> : fexgen::ptr_passthrough {};
template<>
struct fex_gen_config<vkGetRandROutputDisplayEXT> : fexgen::custom_host_impl {};
template<>
struct fex_gen_param<vkGetRandROutputDisplayEXT, 1, Display*> : fexgen::ptr_passthrough {};
} // namespace internal
