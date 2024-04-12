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

template<> struct fex_gen_config<vkGetDeviceProcAddr> : fexgen::custom_host_impl, fexgen::custom_guest_entrypoint, fexgen::returns_guest_pointer {};
template<> struct fex_gen_config<vkGetInstanceProcAddr> : fexgen::custom_host_impl, fexgen::custom_guest_entrypoint, fexgen::returns_guest_pointer {};

template<typename>
struct fex_gen_type {};

// So-called "dispatchable" handles are represented as opaque pointers.
// In addition to marking them as such, API functions that create these objects
// need special care since they wrap these handles in another pointer, which
// the thunk generator can't automatically handle.
//
// So-called "non-dispatchable" handles don't need this extra treatment, since
// they are uint64_t IDs on both 32-bit and 64-bit systems.
template<> struct fex_gen_type<VkCommandBuffer_T> : fexgen::opaque_type {};
template<> struct fex_gen_type<VkDevice_T> : fexgen::opaque_type {};
template<> struct fex_gen_type<VkInstance_T> : fexgen::opaque_type {};
template<> struct fex_gen_type<VkPhysicalDevice_T> : fexgen::opaque_type {};
template<> struct fex_gen_type<VkQueue_T> : fexgen::opaque_type {};

// Mark union types with compatible layout as such
// TODO: These may still have different alignment requirements!
template<> struct fex_gen_type<VkClearValue> : fexgen::assume_compatible_data_layout {};
template<> struct fex_gen_type<VkClearColorValue> : fexgen::assume_compatible_data_layout {};
template<> struct fex_gen_type<VkPipelineExecutableStatisticValueKHR> : fexgen::assume_compatible_data_layout {};
#ifndef IS_32BIT_THUNK
template<> struct fex_gen_type<VkAccelerationStructureGeometryDataKHR> : fexgen::assume_compatible_data_layout {};
template<> struct fex_gen_type<VkDescriptorDataEXT> : fexgen::assume_compatible_data_layout {};
template<> struct fex_gen_type<VkDeviceOrHostAddressKHR> : fexgen::assume_compatible_data_layout {};
template<> struct fex_gen_type<VkDeviceOrHostAddressConstKHR> : fexgen::assume_compatible_data_layout {};
template<> struct fex_gen_type<VkPerformanceValueDataINTEL> : fexgen::assume_compatible_data_layout {};
#endif

// Explicitly register types that are only ever referenced through nested pointers
template<> struct fex_gen_type<VkAccelerationStructureBuildRangeInfoKHR> {};

// Structures that contain function pointers
// TODO: Use custom repacking for these instead
template<> struct fex_gen_type<VkDebugReportCallbackCreateInfoEXT> : fexgen::emit_layout_wrappers {};
template<> struct fex_gen_type<VkDebugUtilsMessengerCreateInfoEXT> : fexgen::emit_layout_wrappers {};

#ifdef IS_32BIT_THUNK
template<> struct fex_gen_type<VkBaseOutStructure> : fexgen::emit_layout_wrappers {};

// Register structs with an extension point (pNext). Any other members that need customization are listed below.
// Generated using
// for i in `grep VK_STRUCTURE_TYPE vk.xml -B1 | grep category=\"struct\" | cut -d'"' -f 4 | sort`
// do
//   grep $i vulkan_{core,wayland,xcb,xlib,xlib_xrandr}.h >& /dev/null && echo $i
// done | awk '{ print "template<> struct fex_gen_config<&"$1"::pNext> : fexgen::custom_repack {};" }'
//template<> struct fex_gen_config<&VkAccelerationStructureBuildGeometryInfoKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkAccelerationStructureBuildSizesInfoKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkAccelerationStructureCaptureDescriptorDataInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkAccelerationStructureCreateInfoKHR::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkAccelerationStructureCreateInfoNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkAccelerationStructureDeviceAddressInfoKHR::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkAccelerationStructureGeometryAabbsDataKHR::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkAccelerationStructureGeometryInstancesDataKHR::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkAccelerationStructureGeometryKHR::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkAccelerationStructureGeometryMotionTrianglesDataNV::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkAccelerationStructureGeometryTrianglesDataKHR::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkAccelerationStructureInfoNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkAccelerationStructureMemoryRequirementsInfoNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkAccelerationStructureMotionInfoNV::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkAccelerationStructureTrianglesOpacityMicromapEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkAccelerationStructureVersionInfoKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkAcquireNextImageInfoKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkAcquireProfilingLockInfoKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkAmigoProfilingSubmitInfoSEC::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkApplicationInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkAttachmentDescription2::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkAttachmentDescriptionStencilLayout::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkAttachmentReference2::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkAttachmentReferenceStencilLayout::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkAttachmentSampleCountInfoAMD::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkBindAccelerationStructureMemoryInfoNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkBindBufferMemoryDeviceGroupInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkBindBufferMemoryInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkBindImageMemoryDeviceGroupInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkBindImageMemoryInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkBindImageMemorySwapchainInfoKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkBindImagePlaneMemoryInfo::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkBindSparseInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkBindVideoSessionMemoryInfoKHR::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkBlitImageInfo2::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkBufferCaptureDescriptorDataInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkBufferCopy2::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkBufferCreateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkBufferDeviceAddressCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkBufferDeviceAddressInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkBufferImageCopy2::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkBufferMemoryBarrier::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkBufferMemoryBarrier2::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkBufferMemoryRequirementsInfo2::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkBufferOpaqueCaptureAddressCreateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkBufferUsageFlags2CreateInfoKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkBufferViewCreateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkCalibratedTimestampInfoEXT::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkCheckpointData2NV::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkCheckpointDataNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkCommandBufferAllocateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkCommandBufferBeginInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkCommandBufferInheritanceConditionalRenderingInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkCommandBufferInheritanceInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkCommandBufferInheritanceRenderingInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkCommandBufferInheritanceRenderPassTransformInfoQCOM::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkCommandBufferInheritanceViewportScissorInfoNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkCommandBufferSubmitInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkCommandPoolCreateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkComputePipelineCreateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkComputePipelineIndirectBufferInfoNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkConditionalRenderingBeginInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkCooperativeMatrixPropertiesKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkCooperativeMatrixPropertiesNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkCopyAccelerationStructureInfoKHR::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkCopyAccelerationStructureToMemoryInfoKHR::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkCopyBufferInfo2::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkCopyBufferToImageInfo2::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkCopyCommandTransformInfoQCOM::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkCopyDescriptorSet::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkCopyImageInfo2::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkCopyImageToBufferInfo2::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkCopyImageToImageInfoEXT::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkCopyImageToMemoryInfoEXT::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkCopyMemoryToAccelerationStructureInfoKHR::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkCopyMemoryToImageInfoEXT::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkCopyMemoryToMicromapInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkCopyMicromapInfoEXT::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkCopyMicromapToMemoryInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkCuFunctionCreateInfoNVX::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkCuLaunchInfoNVX::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkCuModuleCreateInfoNVX::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDebugMarkerMarkerInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDebugMarkerObjectNameInfoEXT::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkDebugMarkerObjectTagInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDebugReportCallbackCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDebugUtilsLabelEXT::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkDebugUtilsMessengerCallbackDataEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDebugUtilsMessengerCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDebugUtilsObjectNameInfoEXT::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkDebugUtilsObjectTagInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDedicatedAllocationBufferCreateInfoNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDedicatedAllocationImageCreateInfoNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDedicatedAllocationMemoryAllocateInfoNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDependencyInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDepthBiasInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDepthBiasRepresentationInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDescriptorAddressInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDescriptorBufferBindingInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDescriptorBufferBindingPushDescriptorBufferHandleEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDescriptorGetInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDescriptorPoolCreateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDescriptorPoolInlineUniformBlockCreateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDescriptorSetAllocateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDescriptorSetBindingReferenceVALVE::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDescriptorSetLayoutBindingFlagsCreateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDescriptorSetLayoutCreateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDescriptorSetLayoutHostMappingInfoVALVE::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDescriptorSetLayoutSupport::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDescriptorSetVariableDescriptorCountAllocateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDescriptorSetVariableDescriptorCountLayoutSupport::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDescriptorUpdateTemplateCreateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDeviceAddressBindingCallbackDataEXT::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkDeviceBufferMemoryRequirements::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDeviceCreateInfo::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkDeviceDeviceMemoryReportCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDeviceDiagnosticsConfigCreateInfoNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDeviceEventInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDeviceFaultCountsEXT::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkDeviceFaultInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDeviceGroupBindSparseInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDeviceGroupCommandBufferBeginInfo::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkDeviceGroupDeviceCreateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDeviceGroupPresentCapabilitiesKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDeviceGroupPresentInfoKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDeviceGroupRenderPassBeginInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDeviceGroupSubmitInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDeviceGroupSwapchainCreateInfoKHR::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkDeviceImageMemoryRequirements::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkDeviceImageSubresourceInfoKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDeviceMemoryOpaqueCaptureAddressInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDeviceMemoryOverallocationCreateInfoAMD::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDeviceMemoryReportCallbackDataEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDevicePrivateDataCreateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDeviceQueueCreateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDeviceQueueGlobalPriorityCreateInfoKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDeviceQueueInfo2::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkDirectDriverLoadingInfoLUNARG::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkDirectDriverLoadingListLUNARG::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDisplayEventInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDisplayModeCreateInfoKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDisplayModeProperties2KHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDisplayNativeHdrSurfaceCapabilitiesAMD::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDisplayPlaneCapabilities2KHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDisplayPlaneInfo2KHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDisplayPlaneProperties2KHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDisplayPowerInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDisplayPresentInfoKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDisplayProperties2KHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDisplaySurfaceCreateInfoKHR::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkDrmFormatModifierPropertiesList2EXT::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkDrmFormatModifierPropertiesListEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkEventCreateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkExportFenceCreateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkExportMemoryAllocateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkExportMemoryAllocateInfoNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkExportSemaphoreCreateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkExternalBufferProperties::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkExternalFenceProperties::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkExternalImageFormatProperties::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkExternalMemoryAcquireUnmodifiedEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkExternalMemoryBufferCreateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkExternalMemoryImageCreateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkExternalMemoryImageCreateInfoNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkExternalSemaphoreProperties::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkFenceCreateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkFenceGetFdInfoKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkFilterCubicImageViewImageFormatPropertiesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkFormatProperties2::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkFormatProperties3::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkFragmentShadingRateAttachmentInfoKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkFramebufferAttachmentImageInfo::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkFramebufferAttachmentsCreateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkFramebufferCreateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkFramebufferMixedSamplesCombinationNV::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkGeneratedCommandsInfoNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkGeneratedCommandsMemoryRequirementsInfoNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkGeometryAABBNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkGeometryNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkGeometryTrianglesNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkGraphicsPipelineCreateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkGraphicsPipelineLibraryCreateInfoEXT::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkGraphicsPipelineShaderGroupsCreateInfoNV::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkGraphicsShaderGroupCreateInfoNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkHeadlessSurfaceCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkHostImageCopyDevicePerformanceQueryEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkHostImageLayoutTransitionInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkImageBlit2::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkImageCaptureDescriptorDataInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkImageCompressionControlEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkImageCompressionPropertiesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkImageCopy2::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkImageCreateInfo::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkImageDrmFormatModifierExplicitCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkImageDrmFormatModifierListCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkImageDrmFormatModifierPropertiesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkImageFormatListCreateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkImageFormatProperties2::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkImageMemoryBarrier::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkImageMemoryBarrier2::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkImageMemoryRequirementsInfo2::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkImagePlaneMemoryRequirementsInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkImageResolve2::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkImageSparseMemoryRequirementsInfo2::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkImageStencilUsageCreateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkImageSubresource2KHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkImageSwapchainCreateInfoKHR::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkImageToMemoryCopyEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkImageViewAddressPropertiesNVX::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkImageViewASTCDecodeModeEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkImageViewCaptureDescriptorDataInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkImageViewCreateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkImageViewHandleInfoNVX::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkImageViewMinLodCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkImageViewSampleWeightCreateInfoQCOM::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkImageViewSlicedCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkImageViewUsageCreateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkImportFenceFdInfoKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkImportMemoryFdInfoKHR::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkImportMemoryHostPointerInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkImportSemaphoreFdInfoKHR::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkIndirectCommandsLayoutCreateInfoNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkIndirectCommandsLayoutTokenNV::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkInitializePerformanceApiInfoINTEL::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkInstanceCreateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkMappedMemoryRange::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkMemoryAllocateFlagsInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkMemoryAllocateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkMemoryBarrier::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkMemoryBarrier2::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkMemoryDedicatedAllocateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkMemoryDedicatedRequirements::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkMemoryFdPropertiesKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkMemoryGetFdInfoKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkMemoryGetRemoteAddressInfoNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkMemoryHostPointerPropertiesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkMemoryMapInfoKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkMemoryOpaqueCaptureAddressAllocateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkMemoryPriorityAllocateInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkMemoryRequirements2::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkMemoryToImageCopyEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkMemoryUnmapInfoKHR::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkMicromapBuildInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkMicromapBuildSizesInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkMicromapCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkMicromapVersionInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkMultisampledRenderToSingleSampledInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkMultisamplePropertiesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkMultiviewPerViewAttributesInfoNVX::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkMultiviewPerViewRenderAreasRenderPassBeginInfoQCOM::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkMutableDescriptorTypeCreateInfoEXT::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkOpaqueCaptureDescriptorDataCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkOpticalFlowExecuteInfoNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkOpticalFlowImageFormatInfoNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkOpticalFlowImageFormatPropertiesNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkOpticalFlowSessionCreateInfoNV::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkOpticalFlowSessionCreatePrivateDataInfoNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPerformanceConfigurationAcquireInfoINTEL::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPerformanceCounterDescriptionKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPerformanceCounterKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPerformanceMarkerInfoINTEL::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPerformanceOverrideInfoINTEL::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPerformanceQuerySubmitInfoKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPerformanceStreamMarkerInfoINTEL::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDevice16BitStorageFeatures::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDevice4444FormatsFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDevice8BitStorageFeatures::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceAccelerationStructureFeaturesKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceAccelerationStructurePropertiesKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceAddressBindingReportFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceAmigoProfilingFeaturesSEC::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceASTCDecodeFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceAttachmentFeedbackLoopDynamicStateFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceAttachmentFeedbackLoopLayoutFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceBlendOperationAdvancedFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceBlendOperationAdvancedPropertiesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceBorderColorSwizzleFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceBufferDeviceAddressFeatures::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceBufferDeviceAddressFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceClusterCullingShaderFeaturesHUAWEI::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceClusterCullingShaderPropertiesHUAWEI::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceCoherentMemoryFeaturesAMD::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceColorWriteEnableFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceComputeShaderDerivativesFeaturesNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceConditionalRenderingFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceConservativeRasterizationPropertiesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceCooperativeMatrixFeaturesKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceCooperativeMatrixFeaturesNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceCooperativeMatrixPropertiesKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceCooperativeMatrixPropertiesNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceCopyMemoryIndirectFeaturesNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceCopyMemoryIndirectPropertiesNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceCornerSampledImageFeaturesNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceCoverageReductionModeFeaturesNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceCustomBorderColorFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceCustomBorderColorPropertiesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceDedicatedAllocationImageAliasingFeaturesNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceDepthBiasControlFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceDepthClampZeroOneFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceDepthClipControlFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceDepthClipEnableFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceDepthStencilResolveProperties::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceDescriptorBufferDensityMapPropertiesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceDescriptorBufferFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceDescriptorBufferPropertiesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceDescriptorIndexingFeatures::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceDescriptorIndexingProperties::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceDescriptorSetHostMappingFeaturesVALVE::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceDeviceGeneratedCommandsComputeFeaturesNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceDeviceGeneratedCommandsFeaturesNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceDeviceGeneratedCommandsPropertiesNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceDeviceMemoryReportFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceDiagnosticsConfigFeaturesNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceDiscardRectanglePropertiesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceDriverProperties::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceDrmPropertiesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceDynamicRenderingFeatures::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceDynamicRenderingUnusedAttachmentsFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceExclusiveScissorFeaturesNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceExtendedDynamicState2FeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceExtendedDynamicState3FeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceExtendedDynamicState3PropertiesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceExtendedDynamicStateFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceExternalBufferInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceExternalFenceInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceExternalImageFormatInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceExternalMemoryHostPropertiesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceExternalMemoryRDMAFeaturesNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceExternalSemaphoreInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceFaultFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceFeatures2::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceFloatControlsProperties::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceFragmentDensityMap2FeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceFragmentDensityMap2PropertiesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceFragmentDensityMapFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceFragmentDensityMapOffsetFeaturesQCOM::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceFragmentDensityMapOffsetPropertiesQCOM::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceFragmentDensityMapPropertiesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceFragmentShaderBarycentricFeaturesKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceFragmentShaderBarycentricPropertiesKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceFragmentShadingRateEnumsFeaturesNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceFragmentShadingRateEnumsPropertiesNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceFragmentShadingRateFeaturesKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceFragmentShadingRateKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceFragmentShadingRatePropertiesKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceGlobalPriorityQueryFeaturesKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceGraphicsPipelineLibraryFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceGraphicsPipelineLibraryPropertiesEXT::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkPhysicalDeviceGroupProperties::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceHostImageCopyFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceHostImageCopyPropertiesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceHostQueryResetFeatures::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceIDProperties::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceImage2DViewOf3DFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceImageCompressionControlFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceImageCompressionControlSwapchainFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceImageDrmFormatModifierInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceImageFormatInfo2::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceImagelessFramebufferFeatures::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceImageProcessingFeaturesQCOM::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceImageProcessingPropertiesQCOM::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceImageRobustnessFeatures::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceImageSlicedViewOf3DFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceImageViewImageFormatInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceImageViewMinLodFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceIndexTypeUint8FeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceInheritedViewportScissorFeaturesNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceInlineUniformBlockFeatures::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceInlineUniformBlockProperties::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceInvocationMaskFeaturesHUAWEI::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceLegacyDitheringFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceLinearColorAttachmentFeaturesNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceLineRasterizationFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceLineRasterizationPropertiesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceMaintenance3Properties::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceMaintenance4Features::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceMaintenance4Properties::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceMaintenance5FeaturesKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceMaintenance5PropertiesKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceMemoryBudgetPropertiesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceMemoryDecompressionFeaturesNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceMemoryDecompressionPropertiesNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceMemoryPriorityFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceMemoryProperties2::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceMeshShaderFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceMeshShaderFeaturesNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceMeshShaderPropertiesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceMeshShaderPropertiesNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceMultiDrawFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceMultiDrawPropertiesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceMultisampledRenderToSingleSampledFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceMultiviewFeatures::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceMultiviewPerViewAttributesPropertiesNVX::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceMultiviewPerViewRenderAreasFeaturesQCOM::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceMultiviewPerViewViewportsFeaturesQCOM::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceMultiviewProperties::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceNonSeamlessCubeMapFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceOpacityMicromapFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceOpacityMicromapPropertiesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceOpticalFlowFeaturesNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceOpticalFlowPropertiesNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDevicePageableDeviceLocalMemoryFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDevicePCIBusInfoPropertiesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDevicePerformanceQueryFeaturesKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDevicePerformanceQueryPropertiesKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDevicePipelineCreationCacheControlFeatures::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDevicePipelineExecutablePropertiesFeaturesKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDevicePipelineLibraryGroupHandlesFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDevicePipelinePropertiesFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDevicePipelineProtectedAccessFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDevicePipelineRobustnessFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDevicePipelineRobustnessPropertiesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDevicePointClippingProperties::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDevicePresentBarrierFeaturesNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDevicePresentIdFeaturesKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDevicePresentWaitFeaturesKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDevicePrimitivesGeneratedQueryFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDevicePrimitiveTopologyListRestartFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDevicePrivateDataFeatures::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceProperties2::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceProtectedMemoryFeatures::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceProtectedMemoryProperties::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceProvokingVertexFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceProvokingVertexPropertiesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDevicePushDescriptorPropertiesKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceRasterizationOrderAttachmentAccessFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceRayQueryFeaturesKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceRayTracingInvocationReorderFeaturesNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceRayTracingInvocationReorderPropertiesNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceRayTracingMaintenance1FeaturesKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceRayTracingMotionBlurFeaturesNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceRayTracingPipelineFeaturesKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceRayTracingPipelinePropertiesKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceRayTracingPositionFetchFeaturesKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceRayTracingPropertiesNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceRepresentativeFragmentTestFeaturesNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceRGBA10X6FormatsFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceRobustness2FeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceRobustness2PropertiesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceSampleLocationsPropertiesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceSamplerFilterMinmaxProperties::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceSamplerYcbcrConversionFeatures::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceScalarBlockLayoutFeatures::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceSeparateDepthStencilLayoutsFeatures::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceShaderAtomicFloat2FeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceShaderAtomicFloatFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceShaderAtomicInt64Features::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceShaderClockFeaturesKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceShaderCoreBuiltinsFeaturesARM::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceShaderCoreBuiltinsPropertiesARM::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceShaderCoreProperties2AMD::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceShaderCorePropertiesAMD::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceShaderCorePropertiesARM::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceShaderDemoteToHelperInvocationFeatures::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceShaderDrawParametersFeatures::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceShaderEarlyAndLateFragmentTestsFeaturesAMD::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceShaderFloat16Int8Features::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceShaderImageAtomicInt64FeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceShaderImageFootprintFeaturesNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceShaderIntegerDotProductFeatures::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceShaderIntegerDotProductProperties::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceShaderIntegerFunctions2FeaturesINTEL::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceShaderModuleIdentifierFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceShaderModuleIdentifierPropertiesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceShaderObjectFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceShaderObjectPropertiesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceShaderSMBuiltinsFeaturesNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceShaderSMBuiltinsPropertiesNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceShaderSubgroupExtendedTypesFeatures::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceShaderSubgroupUniformControlFlowFeaturesKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceShaderTerminateInvocationFeatures::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceShaderTileImageFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceShaderTileImagePropertiesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceShadingRateImageFeaturesNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceShadingRateImagePropertiesNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceSparseImageFormatInfo2::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceSubgroupProperties::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceSubgroupSizeControlFeatures::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceSubgroupSizeControlProperties::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceSubpassMergeFeedbackFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceSubpassShadingFeaturesHUAWEI::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceSubpassShadingPropertiesHUAWEI::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceSurfaceInfo2KHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceSynchronization2Features::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceTexelBufferAlignmentFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceTexelBufferAlignmentProperties::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceTextureCompressionASTCHDRFeatures::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceTilePropertiesFeaturesQCOM::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceTimelineSemaphoreFeatures::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceTimelineSemaphoreProperties::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceToolProperties::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceTransformFeedbackFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceTransformFeedbackPropertiesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceUniformBufferStandardLayoutFeatures::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceVariablePointersFeatures::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceVertexAttributeDivisorFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceVertexAttributeDivisorPropertiesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceVertexInputDynamicStateFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceVideoFormatInfoKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceVulkan11Features::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceVulkan11Properties::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceVulkan12Features::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceVulkan12Properties::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceVulkan13Features::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceVulkan13Properties::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceVulkanMemoryModelFeatures::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceWorkgroupMemoryExplicitLayoutFeaturesKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceYcbcr2Plane444FormatsFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceYcbcrImageArraysFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPhysicalDeviceZeroInitializeWorkgroupMemoryFeatures::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPipelineCacheCreateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPipelineColorBlendAdvancedStateCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPipelineColorBlendStateCreateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPipelineColorWriteCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPipelineCompilerControlCreateInfoAMD::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPipelineCoverageModulationStateCreateInfoNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPipelineCoverageReductionStateCreateInfoNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPipelineCoverageToColorStateCreateInfoNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPipelineCreateFlags2CreateInfoKHR::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkPipelineCreationFeedbackCreateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPipelineDepthStencilStateCreateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPipelineDiscardRectangleStateCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPipelineDynamicStateCreateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPipelineExecutableInfoKHR::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkPipelineExecutableInternalRepresentationKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPipelineExecutablePropertiesKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPipelineExecutableStatisticKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPipelineFragmentShadingRateEnumStateCreateInfoNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPipelineFragmentShadingRateStateCreateInfoKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPipelineIndirectDeviceAddressInfoNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPipelineInfoKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPipelineInputAssemblyStateCreateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPipelineLayoutCreateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPipelineLibraryCreateInfoKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPipelineMultisampleStateCreateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPipelinePropertiesIdentifierEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPipelineRasterizationConservativeStateCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPipelineRasterizationDepthClipStateCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPipelineRasterizationLineStateCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPipelineRasterizationProvokingVertexStateCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPipelineRasterizationStateCreateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPipelineRasterizationStateRasterizationOrderAMD::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPipelineRasterizationStateStreamCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPipelineRenderingCreateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPipelineRepresentativeFragmentTestStateCreateInfoNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPipelineRobustnessCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPipelineSampleLocationsStateCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPipelineShaderStageCreateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPipelineShaderStageModuleIdentifierCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPipelineShaderStageRequiredSubgroupSizeCreateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPipelineTessellationDomainOriginStateCreateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPipelineTessellationStateCreateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPipelineVertexInputDivisorStateCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPipelineVertexInputStateCreateInfo::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkPipelineViewportCoarseSampleOrderStateCreateInfoNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPipelineViewportDepthClipControlCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPipelineViewportExclusiveScissorStateCreateInfoNV::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkPipelineViewportShadingRateImageStateCreateInfoNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPipelineViewportStateCreateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPipelineViewportSwizzleStateCreateInfoNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPipelineViewportWScalingStateCreateInfoNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPresentIdKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPresentInfoKHR::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkPresentRegionsKHR::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkPresentTimesInfoGOOGLE::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkPrivateDataSlotCreateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkProtectedSubmitInfo::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkQueryLowLatencySupportNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkQueryPoolCreateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkQueryPoolPerformanceCreateInfoKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkQueryPoolPerformanceQueryCreateInfoINTEL::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkQueueFamilyCheckpointProperties2NV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkQueueFamilyCheckpointPropertiesNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkQueueFamilyGlobalPriorityPropertiesKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkQueueFamilyProperties2::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkQueueFamilyQueryResultStatusPropertiesKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkQueueFamilyVideoPropertiesKHR::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkRayTracingPipelineCreateInfoKHR::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkRayTracingPipelineCreateInfoNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkRayTracingPipelineInterfaceCreateInfoKHR::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkRayTracingShaderGroupCreateInfoKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkRayTracingShaderGroupCreateInfoNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkReleaseSwapchainImagesInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkRenderingAreaInfoKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkRenderingAttachmentInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkRenderingFragmentDensityMapAttachmentInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkRenderingFragmentShadingRateAttachmentInfoKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkRenderingInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkRenderPassAttachmentBeginInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkRenderPassBeginInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkRenderPassCreateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkRenderPassCreateInfo2::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkRenderPassCreationControlEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkRenderPassCreationFeedbackCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkRenderPassFragmentDensityMapCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkRenderPassInputAttachmentAspectCreateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkRenderPassMultiviewCreateInfo::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkRenderPassSampleLocationsBeginInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkRenderPassSubpassFeedbackCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkRenderPassTransformBeginInfoQCOM::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkResolveImageInfo2::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkSampleLocationsInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkSamplerBorderColorComponentMappingCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkSamplerCaptureDescriptorDataInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkSamplerCreateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkSamplerCustomBorderColorCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkSamplerReductionModeCreateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkSamplerYcbcrConversionCreateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkSamplerYcbcrConversionImageFormatProperties::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkSamplerYcbcrConversionInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkSemaphoreCreateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkSemaphoreGetFdInfoKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkSemaphoreSignalInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkSemaphoreSubmitInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkSemaphoreTypeCreateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkSemaphoreWaitInfo::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkShaderCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkShaderModuleCreateInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkShaderModuleIdentifierEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkShaderModuleValidationCacheCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkSharedPresentSurfaceCapabilitiesKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkSparseImageFormatProperties2::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkSparseImageMemoryRequirements2::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkSubmitInfo::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkSubmitInfo2::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkSubpassBeginInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkSubpassDependency2::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkSubpassDescription2::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkSubpassDescriptionDepthStencilResolve::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkSubpassEndInfo::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkSubpassFragmentDensityMapOffsetEndInfoQCOM::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkSubpassResolvePerformanceQueryEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkSubpassShadingPipelineCreateInfoHUAWEI::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkSubresourceHostMemcpySizeEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkSubresourceLayout2KHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkSurfaceCapabilities2EXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkSurfaceCapabilities2KHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkSurfaceCapabilitiesPresentBarrierNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkSurfaceFormat2KHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkSurfacePresentModeCompatibilityEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkSurfacePresentModeEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkSurfacePresentScalingCapabilitiesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkSurfaceProtectedCapabilitiesKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkSwapchainCounterCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkSwapchainCreateInfoKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkSwapchainDisplayNativeHdrCreateInfoAMD::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkSwapchainPresentBarrierCreateInfoNV::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkSwapchainPresentFenceInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkSwapchainPresentModeInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkSwapchainPresentModesCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkSwapchainPresentScalingCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkTextureLODGatherFormatPropertiesAMD::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkTilePropertiesQCOM::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkTimelineSemaphoreSubmitInfo::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkValidationCacheCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkValidationFeaturesEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkValidationFlagsEXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkVertexInputAttributeDescription2EXT::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkVertexInputBindingDescription2EXT::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkVideoBeginCodingInfoKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkVideoCapabilitiesKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkVideoCodingControlInfoKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkVideoDecodeCapabilitiesKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkVideoDecodeH264CapabilitiesKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkVideoDecodeH264DpbSlotInfoKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkVideoDecodeH264PictureInfoKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkVideoDecodeH264ProfileInfoKHR::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkVideoDecodeH264SessionParametersAddInfoKHR::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkVideoDecodeH264SessionParametersCreateInfoKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkVideoDecodeH265CapabilitiesKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkVideoDecodeH265DpbSlotInfoKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkVideoDecodeH265PictureInfoKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkVideoDecodeH265ProfileInfoKHR::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkVideoDecodeH265SessionParametersAddInfoKHR::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkVideoDecodeH265SessionParametersCreateInfoKHR::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkVideoDecodeInfoKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkVideoDecodeUsageInfoKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkVideoEndCodingInfoKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkVideoFormatPropertiesKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkVideoPictureResourceInfoKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkVideoProfileInfoKHR::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkVideoProfileListInfoKHR::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkVideoReferenceSlotInfoKHR::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkVideoSessionCreateInfoKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkVideoSessionMemoryRequirementsKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkVideoSessionParametersCreateInfoKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkVideoSessionParametersUpdateInfoKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkWaylandSurfaceCreateInfoKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkWriteDescriptorSet::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkWriteDescriptorSetAccelerationStructureKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkWriteDescriptorSetAccelerationStructureNV::pNext> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkWriteDescriptorSetInlineUniformBlock::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkXcbSurfaceCreateInfoKHR::pNext> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkXlibSurfaceCreateInfoKHR::pNext> : fexgen::custom_repack {};


template<> struct fex_gen_config<&VkCommandBufferBeginInfo::pInheritanceInfo> : fexgen::custom_repack {};

template<> struct fex_gen_config<&VkDeviceCreateInfo::pQueueCreateInfos> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDeviceCreateInfo::ppEnabledLayerNames> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDeviceCreateInfo::ppEnabledExtensionNames> : fexgen::custom_repack {};

template<> struct fex_gen_config<&VkDependencyInfo::pMemoryBarriers> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDependencyInfo::pBufferMemoryBarriers> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkDependencyInfo::pImageMemoryBarriers> : fexgen::custom_repack {};

template<> struct fex_gen_config<&VkDescriptorGetInfoEXT::data> : fexgen::custom_repack {};

template<> struct fex_gen_config<&VkDescriptorSetLayoutCreateInfo::pBindings> : fexgen::custom_repack {};

template<> struct fex_gen_config<&VkDescriptorUpdateTemplateCreateInfo::pDescriptorUpdateEntries> : fexgen::custom_repack {};

template<> struct fex_gen_config<&VkGraphicsPipelineCreateInfo::pStages> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkGraphicsPipelineCreateInfo::pVertexInputState> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkGraphicsPipelineCreateInfo::pInputAssemblyState> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkGraphicsPipelineCreateInfo::pTessellationState> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkGraphicsPipelineCreateInfo::pViewportState> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkGraphicsPipelineCreateInfo::pRasterizationState> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkGraphicsPipelineCreateInfo::pMultisampleState> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkGraphicsPipelineCreateInfo::pDepthStencilState> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkGraphicsPipelineCreateInfo::pColorBlendState> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkGraphicsPipelineCreateInfo::pDynamicState> : fexgen::custom_repack {};

template<> struct fex_gen_config<&VkInstanceCreateInfo::pApplicationInfo> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkInstanceCreateInfo::ppEnabledLayerNames> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkInstanceCreateInfo::ppEnabledExtensionNames> : fexgen::custom_repack {};

template<> struct fex_gen_config<&VkRenderPassCreateInfo::pSubpasses> : fexgen::custom_repack {};
// NOTE: pDependencies and pAttachments point to ABI-compatible data

template<> struct fex_gen_config<&VkRenderPassCreateInfo2::pAttachments> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkRenderPassCreateInfo2::pSubpasses> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkRenderPassCreateInfo2::pDependencies> : fexgen::custom_repack {};

template<> struct fex_gen_config<&VkPipelineShaderStageCreateInfo::pSpecializationInfo> : fexgen::custom_repack {};
//template<> struct fex_gen_config<&VkSpecializationInfo::pMapEntries> : fexgen::custom_repack {};

// TODO: Support annotating as assume_compatible_data_layout instead
template<> struct fex_gen_config<&VkPipelineCacheCreateInfo::pInitialData> : fexgen::custom_repack {};

// Command buffers are dispatchable handles, so on 32-bit they need to be repacked
template<> struct fex_gen_config<&VkSubmitInfo::pCommandBuffers> : fexgen::custom_repack {};

template<> struct fex_gen_config<&VkRenderingInfo::pColorAttachments> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkRenderingInfo::pDepthAttachment> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkRenderingInfo::pStencilAttachment> : fexgen::custom_repack {};

// TODO: Support annotating as assume_compatible_data_layout instead
template<> struct fex_gen_config<&VkRenderPassBeginInfo::pClearValues> : fexgen::custom_repack {};

template<> struct fex_gen_config<&VkSubpassDescription2::pInputAttachments> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkSubpassDescription2::pColorAttachments> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkSubpassDescription2::pResolveAttachments> : fexgen::custom_repack {};
template<> struct fex_gen_config<&VkSubpassDescription2::pDepthStencilAttachment> : fexgen::custom_repack {};

// These types have incompatible data layout but we use their layout wrappers elsewhere
template<> struct fex_gen_type<VkWriteDescriptorSet> : fexgen::emit_layout_wrappers {};
#else
// The pNext member of this is a pointer to another VkBaseOutStructure, so data layout compatibility can't be inferred automatically
template<> struct fex_gen_type<VkBaseOutStructure> : fexgen::assume_compatible_data_layout {};
#endif


// TODO: Should not be opaque, but it's usually NULL anyway. Supporting the contained function pointers will need more work.
template<> struct fex_gen_type<VkAllocationCallbacks> : fexgen::opaque_type {};

// X11 interop
template<> struct fex_gen_type<Display> : fexgen::opaque_type {};
template<> struct fex_gen_type<xcb_connection_t> : fexgen::opaque_type {};

// Wayland interop
template<> struct fex_gen_type<wl_display> : fexgen::opaque_type {};
template<> struct fex_gen_type<wl_surface> : fexgen::opaque_type {};

namespace internal {

// Function, parameter index, parameter type [optional]
template<auto, int, typename = void>
struct fex_gen_param {};

template<auto>
struct fex_gen_config : fexgen::generate_guest_symtable, fexgen::indirect_guest_calls {
};

template<> struct fex_gen_config<vkCreateInstance> : fexgen::custom_host_impl {};
template<> struct fex_gen_param<vkCreateInstance, 2, VkInstance*> : fexgen::ptr_passthrough {};
template<> struct fex_gen_config<vkDestroyInstance> {};
#ifndef IS_32BIT_THUNK
template<> struct fex_gen_config<vkEnumeratePhysicalDevices> {};
#else
template<> struct fex_gen_config<vkEnumeratePhysicalDevices> : fexgen::custom_host_impl {};
template<> struct fex_gen_param<vkEnumeratePhysicalDevices, 2, VkPhysicalDevice*> : fexgen::ptr_passthrough {};
#endif


template<> struct fex_gen_config<vkGetPhysicalDeviceFeatures> {};
template<> struct fex_gen_config<vkGetPhysicalDeviceFormatProperties> {};
template<> struct fex_gen_config<vkGetPhysicalDeviceImageFormatProperties> {};
// TODO: Output parameter must repack on exit!
template<> struct fex_gen_config<vkGetPhysicalDeviceProperties> {};
template<> struct fex_gen_config<vkGetPhysicalDeviceQueueFamilyProperties> {};
template<> struct fex_gen_config<vkGetPhysicalDeviceMemoryProperties> {};
template<> struct fex_gen_config<vkCreateDevice> : fexgen::custom_host_impl {};
template<> struct fex_gen_param<vkCreateDevice, 3, VkDevice*> : fexgen::ptr_passthrough {};
template<> struct fex_gen_config<vkDestroyDevice> {};
template<> struct fex_gen_config<vkEnumerateInstanceExtensionProperties> {};
template<> struct fex_gen_config<vkEnumerateDeviceExtensionProperties> {};
template<> struct fex_gen_config<vkEnumerateInstanceLayerProperties> {};
template<> struct fex_gen_config<vkEnumerateDeviceLayerProperties> {};
#ifndef IS_32BIT_THUNK
template<> struct fex_gen_config<vkGetDeviceQueue> {};
#else
template<> struct fex_gen_config<vkGetDeviceQueue> : fexgen::custom_host_impl {};
template<> struct fex_gen_param<vkGetDeviceQueue, 3, VkQueue*> : fexgen::ptr_passthrough {};
#endif
#ifndef IS_32BIT_THUNK
template<> struct fex_gen_config<vkQueueSubmit> {};
#else
// Needs array repacking for multiple submit infos
template<> struct fex_gen_config<vkQueueSubmit> : fexgen::custom_host_impl {};
template<> struct fex_gen_param<vkQueueSubmit, 2, const VkSubmitInfo*> : fexgen::ptr_passthrough {};
#endif
template<> struct fex_gen_config<vkQueueWaitIdle> {};
template<> struct fex_gen_config<vkDeviceWaitIdle> {};
template<> struct fex_gen_config<vkAllocateMemory> : fexgen::custom_host_impl {};
template<> struct fex_gen_config<vkFreeMemory> : fexgen::custom_host_impl {};
#ifndef IS_32BIT_THUNK
template<> struct fex_gen_config<vkMapMemory> {};
#else
template<> struct fex_gen_config<vkMapMemory> : fexgen::custom_host_impl {};
template<> struct fex_gen_param<vkMapMemory, 5, void**> : fexgen::ptr_passthrough {};
#endif
template<> struct fex_gen_config<vkUnmapMemory> {};
#ifndef IS_32BIT_THUNK
template<> struct fex_gen_config<vkFlushMappedMemoryRanges> {};
template<> struct fex_gen_config<vkInvalidateMappedMemoryRanges> {};
template<> struct fex_gen_config<vkGetDeviceMemoryCommitment> {};
#endif
template<> struct fex_gen_config<vkBindBufferMemory> {};
template<> struct fex_gen_config<vkBindImageMemory> {};
template<> struct fex_gen_config<vkGetBufferMemoryRequirements> {};
template<> struct fex_gen_config<vkGetImageMemoryRequirements> {};
#ifndef IS_32BIT_THUNK
template<> struct fex_gen_config<vkGetImageSparseMemoryRequirements> {};
template<> struct fex_gen_config<vkGetPhysicalDeviceSparseImageFormatProperties> {};
template<> struct fex_gen_config<vkQueueBindSparse> {};
#endif
template<> struct fex_gen_config<vkCreateFence> {};
template<> struct fex_gen_config<vkDestroyFence> {};
template<> struct fex_gen_config<vkResetFences> {};
#ifndef IS_32BIT_THUNK
template<> struct fex_gen_config<vkGetFenceStatus> {};
#endif
template<> struct fex_gen_config<vkWaitForFences> {};
template<> struct fex_gen_config<vkCreateSemaphore> {};
template<> struct fex_gen_config<vkDestroySemaphore> {};
#ifndef IS_32BIT_THUNK
template<> struct fex_gen_config<vkCreateEvent> {};
template<> struct fex_gen_config<vkDestroyEvent> {};
template<> struct fex_gen_config<vkGetEventStatus> {};
template<> struct fex_gen_config<vkSetEvent> {};
template<> struct fex_gen_config<vkResetEvent> {};
template<> struct fex_gen_config<vkCreateQueryPool> {};
template<> struct fex_gen_config<vkDestroyQueryPool> {};
template<> struct fex_gen_config<vkGetQueryPoolResults> {};
template<> struct fex_gen_param<vkGetQueryPoolResults, 5, void*> : fexgen::assume_compatible_data_layout {};
#endif
template<> struct fex_gen_config<vkCreateBuffer> {};
template<> struct fex_gen_config<vkDestroyBuffer> {};
template<> struct fex_gen_config<vkCreateBufferView> {};
template<> struct fex_gen_config<vkDestroyBufferView> {};
template<> struct fex_gen_config<vkCreateImage> {};
template<> struct fex_gen_config<vkDestroyImage> {};
template<> struct fex_gen_config<vkGetImageSubresourceLayout> {};
template<> struct fex_gen_config<vkCreateImageView> {};
template<> struct fex_gen_config<vkDestroyImageView> {};
template<> struct fex_gen_config<vkCreateShaderModule> : fexgen::custom_host_impl {};
template<> struct fex_gen_config<vkDestroyShaderModule> {};
template<> struct fex_gen_config<vkCreatePipelineCache> {};
template<> struct fex_gen_config<vkDestroyPipelineCache> {};
#ifndef IS_32BIT_THUNK
template<> struct fex_gen_config<vkGetPipelineCacheData> {};
#else
template<> struct fex_gen_config<vkGetPipelineCacheData> : fexgen::custom_host_impl {};
template<> struct fex_gen_param<vkGetPipelineCacheData, 2, size_t*> : fexgen::ptr_passthrough {};
#endif
template<> struct fex_gen_param<vkGetPipelineCacheData, 3, void*> : fexgen::assume_compatible_data_layout {};
#ifndef IS_32BIT_THUNK
template<> struct fex_gen_config<vkMergePipelineCaches> {};
#endif
// TODO: Should be custom_host_impl since there may be more than one VkGraphicsPipelineCreateInfo and more than one output pipeline
template<> struct fex_gen_config<vkCreateGraphicsPipelines> {};
#ifndef IS_32BIT_THUNK
template<> struct fex_gen_config<vkCreateComputePipelines> {};
#endif
template<> struct fex_gen_config<vkDestroyPipeline> {};
template<> struct fex_gen_config<vkCreatePipelineLayout> {};
template<> struct fex_gen_config<vkDestroyPipelineLayout> {};
template<> struct fex_gen_config<vkCreateSampler> {};
template<> struct fex_gen_config<vkDestroySampler> {};
template<> struct fex_gen_config<vkCreateDescriptorSetLayout> {};
template<> struct fex_gen_config<vkDestroyDescriptorSetLayout> {};
template<> struct fex_gen_config<vkCreateDescriptorPool> {};
template<> struct fex_gen_config<vkDestroyDescriptorPool> {};
template<> struct fex_gen_config<vkResetDescriptorPool> {};
template<> struct fex_gen_config<vkAllocateDescriptorSets> {};
template<> struct fex_gen_config<vkFreeDescriptorSets> {};
#ifndef IS_32BIT_THUNK
template<> struct fex_gen_config<vkUpdateDescriptorSets> {};
#else
template<> struct fex_gen_config<vkUpdateDescriptorSets> : fexgen::custom_host_impl {};
template<> struct fex_gen_param<vkUpdateDescriptorSets, 2, const VkWriteDescriptorSet*> : fexgen::ptr_passthrough {};
template<> struct fex_gen_param<vkUpdateDescriptorSets, 4, const VkCopyDescriptorSet*> : fexgen::ptr_passthrough {};
#endif
template<> struct fex_gen_config<vkCreateFramebuffer> {};
template<> struct fex_gen_config<vkDestroyFramebuffer> {};
template<> struct fex_gen_config<vkCreateRenderPass> {};
template<> struct fex_gen_config<vkDestroyRenderPass> {};
template<> struct fex_gen_config<vkGetRenderAreaGranularity> {};
template<> struct fex_gen_config<vkCreateCommandPool> {};
template<> struct fex_gen_config<vkDestroyCommandPool> {};
template<> struct fex_gen_config<vkResetCommandPool> {};
#ifndef IS_32BIT_THUNK
template<> struct fex_gen_config<vkAllocateCommandBuffers> {};
#else
template<> struct fex_gen_config<vkAllocateCommandBuffers> : fexgen::custom_host_impl {};
template<> struct fex_gen_param<vkAllocateCommandBuffers, 2, VkCommandBuffer*> : fexgen::ptr_passthrough {};
#endif

#ifndef IS_32BIT_THUNK
template<> struct fex_gen_config<vkFreeCommandBuffers> {};
#else
template<> struct fex_gen_config<vkFreeCommandBuffers> : fexgen::custom_host_impl {};
template<> struct fex_gen_param<vkFreeCommandBuffers, 3, const VkCommandBuffer*> : fexgen::ptr_passthrough {};
#endif
template<> struct fex_gen_config<vkBeginCommandBuffer> {};
template<> struct fex_gen_config<vkEndCommandBuffer> {};
template<> struct fex_gen_config<vkResetCommandBuffer> {};
template<> struct fex_gen_config<vkCmdBindPipeline> {};
template<> struct fex_gen_config<vkCmdSetViewport> {};
template<> struct fex_gen_config<vkCmdSetScissor> {};
template<> struct fex_gen_config<vkCmdSetLineWidth> {};
template<> struct fex_gen_config<vkCmdSetDepthBias> {};
template<> struct fex_gen_config<vkCmdSetBlendConstants> {};
template<> struct fex_gen_config<vkCmdSetDepthBounds> {};
template<> struct fex_gen_config<vkCmdSetStencilCompareMask> {};
template<> struct fex_gen_config<vkCmdSetStencilWriteMask> {};
template<> struct fex_gen_config<vkCmdSetStencilReference> {};
template<> struct fex_gen_config<vkCmdBindDescriptorSets> {};
template<> struct fex_gen_config<vkCmdBindIndexBuffer> {};
template<> struct fex_gen_config<vkCmdBindVertexBuffers> {};
template<> struct fex_gen_config<vkCmdDraw> {};
#ifndef IS_32BIT_THUNK
template<> struct fex_gen_config<vkCmdDrawIndexed> {};
template<> struct fex_gen_config<vkCmdDrawIndirect> {};
template<> struct fex_gen_config<vkCmdDrawIndexedIndirect> {};
template<> struct fex_gen_config<vkCmdDispatch> {};
template<> struct fex_gen_config<vkCmdDispatchIndirect> {};
template<> struct fex_gen_config<vkCmdCopyBuffer> {};
template<> struct fex_gen_config<vkCmdCopyImage> {};
template<> struct fex_gen_config<vkCmdBlitImage> {};
#endif
template<> struct fex_gen_config<vkCmdCopyBufferToImage> {};
#ifndef IS_32BIT_THUNK
template<> struct fex_gen_config<vkCmdCopyImageToBuffer> {};
template<> struct fex_gen_config<vkCmdUpdateBuffer> {};
template<> struct fex_gen_param<vkCmdUpdateBuffer, 4, const void*> : fexgen::assume_compatible_data_layout {};
template<> struct fex_gen_config<vkCmdFillBuffer> {};
template<> struct fex_gen_config<vkCmdClearColorImage> {};
template<> struct fex_gen_config<vkCmdClearDepthStencilImage> {};
#endif
template<> struct fex_gen_config<vkCmdClearAttachments> {};
#ifndef IS_32BIT_THUNK
template<> struct fex_gen_config<vkCmdResolveImage> {};
template<> struct fex_gen_config<vkCmdSetEvent> {};
template<> struct fex_gen_config<vkCmdResetEvent> {};
template<> struct fex_gen_config<vkCmdWaitEvents> {};
#endif
template<> struct fex_gen_config<vkCmdPipelineBarrier> {};
#ifndef IS_32BIT_THUNK
template<> struct fex_gen_config<vkCmdBeginQuery> {};
template<> struct fex_gen_config<vkCmdEndQuery> {};
template<> struct fex_gen_config<vkCmdResetQueryPool> {};
template<> struct fex_gen_config<vkCmdWriteTimestamp> {};
template<> struct fex_gen_config<vkCmdCopyQueryPoolResults> {};
template<> struct fex_gen_config<vkCmdPushConstants> {};
template<> struct fex_gen_param<vkCmdPushConstants, 5, const void*> : fexgen::assume_compatible_data_layout {};
#endif
template<> struct fex_gen_config<vkCmdBeginRenderPass> {};
#ifndef IS_32BIT_THUNK
template<> struct fex_gen_config<vkCmdNextSubpass> {};
#endif
template<> struct fex_gen_config<vkCmdEndRenderPass> {};
#ifndef IS_32BIT_THUNK
template<> struct fex_gen_config<vkCmdExecuteCommands> {};
#endif
template<> struct fex_gen_config<vkEnumerateInstanceVersion> {};
#ifndef IS_32BIT_THUNK
template<> struct fex_gen_config<vkBindBufferMemory2> {};
template<> struct fex_gen_config<vkBindImageMemory2> {};
template<> struct fex_gen_config<vkGetDeviceGroupPeerMemoryFeatures> {};
template<> struct fex_gen_config<vkCmdSetDeviceMask> {};
template<> struct fex_gen_config<vkCmdDispatchBase> {};
template<> struct fex_gen_config<vkEnumeratePhysicalDeviceGroups> {};
#endif
template<> struct fex_gen_config<vkGetImageMemoryRequirements2> {};
#ifndef IS_32BIT_THUNK
template<> struct fex_gen_config<vkGetBufferMemoryRequirements2> {};
template<> struct fex_gen_config<vkGetImageSparseMemoryRequirements2> {};
#endif
template<> struct fex_gen_config<vkGetPhysicalDeviceFeatures2> {};
template<> struct fex_gen_config<vkGetPhysicalDeviceProperties2> {};
template<> struct fex_gen_config<vkGetPhysicalDeviceFormatProperties2> {};
template<> struct fex_gen_config<vkGetPhysicalDeviceImageFormatProperties2> {};
#ifndef IS_32BIT_THUNK
template<> struct fex_gen_config<vkGetPhysicalDeviceQueueFamilyProperties2> {};
template<> struct fex_gen_config<vkGetPhysicalDeviceMemoryProperties2> {};
template<> struct fex_gen_config<vkGetPhysicalDeviceSparseImageFormatProperties2> {};
template<> struct fex_gen_config<vkTrimCommandPool> {};
template<> struct fex_gen_config<vkGetDeviceQueue2> {};
template<> struct fex_gen_config<vkCreateSamplerYcbcrConversion> {};
template<> struct fex_gen_config<vkDestroySamplerYcbcrConversion> {};
#endif
template<> struct fex_gen_config<vkCreateDescriptorUpdateTemplate> {};
template<> struct fex_gen_config<vkDestroyDescriptorUpdateTemplate> {};
template<> struct fex_gen_config<vkUpdateDescriptorSetWithTemplate> {};
template<> struct fex_gen_param<vkUpdateDescriptorSetWithTemplate, 3, const void*> : fexgen::assume_compatible_data_layout {};
#ifndef IS_32BIT_THUNK
template<> struct fex_gen_config<vkGetPhysicalDeviceExternalBufferProperties> {};
template<> struct fex_gen_config<vkGetPhysicalDeviceExternalFenceProperties> {};
template<> struct fex_gen_config<vkGetPhysicalDeviceExternalSemaphoreProperties> {};
#endif
template<> struct fex_gen_config<vkGetDescriptorSetLayoutSupport> {};
#ifndef IS_32BIT_THUNK
template<> struct fex_gen_config<vkCmdDrawIndirectCount> {};
template<> struct fex_gen_config<vkCmdDrawIndexedIndirectCount> {};
#endif
template<> struct fex_gen_config<vkCreateRenderPass2> {};
#ifndef IS_32BIT_THUNK
template<> struct fex_gen_config<vkCmdBeginRenderPass2> {};
template<> struct fex_gen_config<vkCmdNextSubpass2> {};
template<> struct fex_gen_config<vkCmdEndRenderPass2> {};
template<> struct fex_gen_config<vkResetQueryPool> {};
template<> struct fex_gen_config<vkGetSemaphoreCounterValue> {};
#endif
template<> struct fex_gen_config<vkWaitSemaphores> {};
#ifndef IS_32BIT_THUNK
template<> struct fex_gen_config<vkSignalSemaphore> {};
#endif
template<> struct fex_gen_config<vkGetBufferDeviceAddress> {};
#ifndef IS_32BIT_THUNK
template<> struct fex_gen_config<vkGetBufferOpaqueCaptureAddress> {};
template<> struct fex_gen_config<vkGetDeviceMemoryOpaqueCaptureAddress> {};
template<> struct fex_gen_config<vkGetPhysicalDeviceToolProperties> {};
template<> struct fex_gen_config<vkCreatePrivateDataSlot> {};
template<> struct fex_gen_config<vkDestroyPrivateDataSlot> {};
template<> struct fex_gen_config<vkSetPrivateData> {};
template<> struct fex_gen_config<vkGetPrivateData> {};
template<> struct fex_gen_config<vkCmdSetEvent2> {};
template<> struct fex_gen_config<vkCmdResetEvent2> {};
template<> struct fex_gen_config<vkCmdWaitEvents2> {};
#endif
template<> struct fex_gen_config<vkCmdPipelineBarrier2> {};
#ifndef IS_32BIT_THUNK
template<> struct fex_gen_config<vkCmdWriteTimestamp2> {};
template<> struct fex_gen_config<vkQueueSubmit2> {};
template<> struct fex_gen_config<vkCmdCopyBuffer2> {};
template<> struct fex_gen_config<vkCmdCopyImage2> {};
template<> struct fex_gen_config<vkCmdCopyBufferToImage2> {};
template<> struct fex_gen_config<vkCmdCopyImageToBuffer2> {};
template<> struct fex_gen_config<vkCmdBlitImage2> {};
template<> struct fex_gen_config<vkCmdResolveImage2> {};
#endif
template<> struct fex_gen_config<vkCmdBeginRendering> {};
template<> struct fex_gen_config<vkCmdEndRendering> {};
template<> struct fex_gen_config<vkCmdSetCullMode> {};
template<> struct fex_gen_config<vkCmdSetFrontFace> {};
template<> struct fex_gen_config<vkCmdSetPrimitiveTopology> {};
template<> struct fex_gen_config<vkCmdSetViewportWithCount> {};
template<> struct fex_gen_config<vkCmdSetScissorWithCount> {};
template<> struct fex_gen_config<vkCmdBindVertexBuffers2> {};
template<> struct fex_gen_config<vkCmdSetDepthTestEnable> {};
template<> struct fex_gen_config<vkCmdSetDepthWriteEnable> {};
template<> struct fex_gen_config<vkCmdSetDepthCompareOp> {};
template<> struct fex_gen_config<vkCmdSetDepthBoundsTestEnable> {};
template<> struct fex_gen_config<vkCmdSetStencilTestEnable> {};
template<> struct fex_gen_config<vkCmdSetStencilOp> {};
template<> struct fex_gen_config<vkCmdSetRasterizerDiscardEnable> {};
template<> struct fex_gen_config<vkCmdSetDepthBiasEnable> {};
template<> struct fex_gen_config<vkCmdSetPrimitiveRestartEnable> {};
#ifndef IS_32BIT_THUNK
template<> struct fex_gen_config<vkGetDeviceBufferMemoryRequirements> {};
template<> struct fex_gen_config<vkGetDeviceImageMemoryRequirements> {};
template<> struct fex_gen_config<vkGetDeviceImageSparseMemoryRequirements> {};
#endif
template<> struct fex_gen_config<vkDestroySurfaceKHR> {};
template<> struct fex_gen_config<vkGetPhysicalDeviceSurfaceSupportKHR> {};
template<> struct fex_gen_config<vkGetPhysicalDeviceSurfaceCapabilitiesKHR> {};
template<> struct fex_gen_config<vkGetPhysicalDeviceSurfaceFormatsKHR> {}; // TODO: Need to figure out how *not* to repack the last parameter on input...
template<> struct fex_gen_config<vkGetPhysicalDeviceSurfacePresentModesKHR> {};
template<> struct fex_gen_config<vkCreateSwapchainKHR> {};
template<> struct fex_gen_config<vkDestroySwapchainKHR> {};
template<> struct fex_gen_config<vkGetSwapchainImagesKHR> {};
template<> struct fex_gen_config<vkAcquireNextImageKHR> {};
template<> struct fex_gen_config<vkQueuePresentKHR> {};
#ifndef IS_32BIT_THUNK
template<> struct fex_gen_config<vkGetDeviceGroupPresentCapabilitiesKHR> {};
template<> struct fex_gen_config<vkGetDeviceGroupSurfacePresentModesKHR> {};
template<> struct fex_gen_config<vkGetPhysicalDevicePresentRectanglesKHR> {};
template<> struct fex_gen_config<vkAcquireNextImage2KHR> {};
template<> struct fex_gen_config<vkGetPhysicalDeviceDisplayPropertiesKHR> {};
template<> struct fex_gen_config<vkGetPhysicalDeviceDisplayPlanePropertiesKHR> {};
template<> struct fex_gen_config<vkGetDisplayPlaneSupportedDisplaysKHR> {};
template<> struct fex_gen_config<vkGetDisplayModePropertiesKHR> {};
template<> struct fex_gen_config<vkCreateDisplayModeKHR> {};
template<> struct fex_gen_config<vkGetDisplayPlaneCapabilitiesKHR> {};
template<> struct fex_gen_config<vkCreateDisplayPlaneSurfaceKHR> {};
template<> struct fex_gen_config<vkCreateSharedSwapchainsKHR> {};
template<> struct fex_gen_config<vkGetPhysicalDeviceVideoCapabilitiesKHR> {};
template<> struct fex_gen_config<vkGetPhysicalDeviceVideoFormatPropertiesKHR> {};
template<> struct fex_gen_config<vkCreateVideoSessionKHR> {};
template<> struct fex_gen_config<vkDestroyVideoSessionKHR> {};
template<> struct fex_gen_config<vkGetVideoSessionMemoryRequirementsKHR> {};
template<> struct fex_gen_config<vkBindVideoSessionMemoryKHR> {};
template<> struct fex_gen_config<vkCreateVideoSessionParametersKHR> {};
template<> struct fex_gen_config<vkUpdateVideoSessionParametersKHR> {};
template<> struct fex_gen_config<vkDestroyVideoSessionParametersKHR> {};
template<> struct fex_gen_config<vkCmdBeginVideoCodingKHR> {};
template<> struct fex_gen_config<vkCmdEndVideoCodingKHR> {};
template<> struct fex_gen_config<vkCmdControlVideoCodingKHR> {};
template<> struct fex_gen_config<vkCmdDecodeVideoKHR> {};
template<> struct fex_gen_config<vkCmdBeginRenderingKHR> {};
template<> struct fex_gen_config<vkCmdEndRenderingKHR> {};
template<> struct fex_gen_config<vkGetPhysicalDeviceFeatures2KHR> {};
template<> struct fex_gen_config<vkGetPhysicalDeviceProperties2KHR> {};
template<> struct fex_gen_config<vkGetPhysicalDeviceFormatProperties2KHR> {};
template<> struct fex_gen_config<vkGetPhysicalDeviceImageFormatProperties2KHR> {};
template<> struct fex_gen_config<vkGetPhysicalDeviceQueueFamilyProperties2KHR> {};
template<> struct fex_gen_config<vkGetPhysicalDeviceMemoryProperties2KHR> {};
template<> struct fex_gen_config<vkGetPhysicalDeviceSparseImageFormatProperties2KHR> {};
template<> struct fex_gen_config<vkGetDeviceGroupPeerMemoryFeaturesKHR> {};
template<> struct fex_gen_config<vkCmdSetDeviceMaskKHR> {};
template<> struct fex_gen_config<vkCmdDispatchBaseKHR> {};
template<> struct fex_gen_config<vkTrimCommandPoolKHR> {};
template<> struct fex_gen_config<vkEnumeratePhysicalDeviceGroupsKHR> {};
template<> struct fex_gen_config<vkGetPhysicalDeviceExternalBufferPropertiesKHR> {};
template<> struct fex_gen_config<vkGetMemoryFdKHR> {};
template<> struct fex_gen_config<vkGetMemoryFdPropertiesKHR> {};
template<> struct fex_gen_config<vkGetPhysicalDeviceExternalSemaphorePropertiesKHR> {};
template<> struct fex_gen_config<vkImportSemaphoreFdKHR> {};
template<> struct fex_gen_config<vkGetSemaphoreFdKHR> {};
template<> struct fex_gen_config<vkCmdPushDescriptorSetKHR> {};
#endif
template<> struct fex_gen_config<vkCmdPushDescriptorSetWithTemplateKHR> {};
template<> struct fex_gen_param<vkCmdPushDescriptorSetWithTemplateKHR, 4, const void*> : fexgen::assume_compatible_data_layout {};
#ifndef IS_32BIT_THUNK
template<> struct fex_gen_config<vkCreateDescriptorUpdateTemplateKHR> {};
template<> struct fex_gen_config<vkDestroyDescriptorUpdateTemplateKHR> {};
#endif
template<> struct fex_gen_config<vkUpdateDescriptorSetWithTemplateKHR> {};
template<> struct fex_gen_param<vkUpdateDescriptorSetWithTemplateKHR, 3, const void*> : fexgen::assume_compatible_data_layout {};
template<> struct fex_gen_config<vkCreateRenderPass2KHR> {};
#ifndef IS_32BIT_THUNK
template<> struct fex_gen_config<vkCmdBeginRenderPass2KHR> {};
template<> struct fex_gen_config<vkCmdNextSubpass2KHR> {};
template<> struct fex_gen_config<vkCmdEndRenderPass2KHR> {};
template<> struct fex_gen_config<vkGetSwapchainStatusKHR> {};
template<> struct fex_gen_config<vkGetPhysicalDeviceExternalFencePropertiesKHR> {};
template<> struct fex_gen_config<vkImportFenceFdKHR> {};
template<> struct fex_gen_config<vkGetFenceFdKHR> {};
template<> struct fex_gen_config<vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR> {};
template<> struct fex_gen_config<vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR> {};
template<> struct fex_gen_config<vkAcquireProfilingLockKHR> {};
template<> struct fex_gen_config<vkReleaseProfilingLockKHR> {};
template<> struct fex_gen_config<vkGetPhysicalDeviceSurfaceCapabilities2KHR> {};
template<> struct fex_gen_config<vkGetPhysicalDeviceSurfaceFormats2KHR> {};
template<> struct fex_gen_config<vkGetPhysicalDeviceDisplayProperties2KHR> {};
template<> struct fex_gen_config<vkGetPhysicalDeviceDisplayPlaneProperties2KHR> {};
template<> struct fex_gen_config<vkGetDisplayModeProperties2KHR> {};
template<> struct fex_gen_config<vkGetDisplayPlaneCapabilities2KHR> {};
template<> struct fex_gen_config<vkGetImageMemoryRequirements2KHR> {};
template<> struct fex_gen_config<vkGetBufferMemoryRequirements2KHR> {};
template<> struct fex_gen_config<vkGetImageSparseMemoryRequirements2KHR> {};
template<> struct fex_gen_config<vkCreateSamplerYcbcrConversionKHR> {};
template<> struct fex_gen_config<vkDestroySamplerYcbcrConversionKHR> {};
template<> struct fex_gen_config<vkBindBufferMemory2KHR> {};
template<> struct fex_gen_config<vkBindImageMemory2KHR> {};
template<> struct fex_gen_config<vkGetDescriptorSetLayoutSupportKHR> {};
template<> struct fex_gen_config<vkCmdDrawIndirectCountKHR> {};
template<> struct fex_gen_config<vkCmdDrawIndexedIndirectCountKHR> {};
template<> struct fex_gen_config<vkGetSemaphoreCounterValueKHR> {};
template<> struct fex_gen_config<vkWaitSemaphoresKHR> {};
template<> struct fex_gen_config<vkSignalSemaphoreKHR> {};
template<> struct fex_gen_config<vkGetPhysicalDeviceFragmentShadingRatesKHR> {};
template<> struct fex_gen_config<vkCmdSetFragmentShadingRateKHR> {};
template<> struct fex_gen_config<vkCmdSetRenderingAttachmentLocationsKHR> {};
template<> struct fex_gen_config<vkCmdSetRenderingInputAttachmentIndicesKHR> {};
template<> struct fex_gen_config<vkWaitForPresentKHR> {};
template<> struct fex_gen_config<vkGetBufferDeviceAddressKHR> {};
template<> struct fex_gen_config<vkGetBufferOpaqueCaptureAddressKHR> {};
template<> struct fex_gen_config<vkGetDeviceMemoryOpaqueCaptureAddressKHR> {};
template<> struct fex_gen_config<vkCreateDeferredOperationKHR> {};
template<> struct fex_gen_config<vkDestroyDeferredOperationKHR> {};
template<> struct fex_gen_config<vkGetDeferredOperationMaxConcurrencyKHR> {};
template<> struct fex_gen_config<vkGetDeferredOperationResultKHR> {};
template<> struct fex_gen_config<vkDeferredOperationJoinKHR> {};
template<> struct fex_gen_config<vkGetPipelineExecutablePropertiesKHR> {};
template<> struct fex_gen_config<vkGetPipelineExecutableStatisticsKHR> {};
template<> struct fex_gen_config<vkGetPipelineExecutableInternalRepresentationsKHR> {};
template<> struct fex_gen_config<vkMapMemory2KHR> {};
template<> struct fex_gen_config<vkUnmapMemory2KHR> {};
template<> struct fex_gen_config<vkGetPhysicalDeviceVideoEncodeQualityLevelPropertiesKHR> {};
template<> struct fex_gen_config<vkGetEncodedVideoSessionParametersKHR> {};
template<> struct fex_gen_config<vkCmdEncodeVideoKHR> {};
template<> struct fex_gen_config<vkCmdSetEvent2KHR> {};
template<> struct fex_gen_config<vkCmdResetEvent2KHR> {};
template<> struct fex_gen_config<vkCmdWaitEvents2KHR> {};
template<> struct fex_gen_config<vkCmdPipelineBarrier2KHR> {};
template<> struct fex_gen_config<vkCmdWriteTimestamp2KHR> {};
template<> struct fex_gen_config<vkQueueSubmit2KHR> {};
template<> struct fex_gen_config<vkCmdWriteBufferMarker2AMD> {};
template<> struct fex_gen_config<vkGetQueueCheckpointData2NV> {};
template<> struct fex_gen_config<vkCmdCopyBuffer2KHR> {};
template<> struct fex_gen_config<vkCmdCopyImage2KHR> {};
template<> struct fex_gen_config<vkCmdCopyBufferToImage2KHR> {};
template<> struct fex_gen_config<vkCmdCopyImageToBuffer2KHR> {};
template<> struct fex_gen_config<vkCmdBlitImage2KHR> {};
template<> struct fex_gen_config<vkCmdResolveImage2KHR> {};
template<> struct fex_gen_config<vkCmdTraceRaysIndirect2KHR> {};
template<> struct fex_gen_config<vkGetDeviceBufferMemoryRequirementsKHR> {};
template<> struct fex_gen_config<vkGetDeviceImageMemoryRequirementsKHR> {};
template<> struct fex_gen_config<vkGetDeviceImageSparseMemoryRequirementsKHR> {};
template<> struct fex_gen_config<vkCmdBindIndexBuffer2KHR> {};
template<> struct fex_gen_config<vkGetRenderingAreaGranularityKHR> {};
template<> struct fex_gen_config<vkGetDeviceImageSubresourceLayoutKHR> {};
template<> struct fex_gen_config<vkGetImageSubresourceLayout2KHR> {};
template<> struct fex_gen_config<vkGetPhysicalDeviceCooperativeMatrixPropertiesKHR> {};
template<> struct fex_gen_config<vkCmdSetLineStippleKHR> {};
template<> struct fex_gen_config<vkGetPhysicalDeviceCalibrateableTimeDomainsKHR> {};
template<> struct fex_gen_config<vkGetCalibratedTimestampsKHR> {};
template<> struct fex_gen_config<vkCmdBindDescriptorSets2KHR> {};
template<> struct fex_gen_config<vkCmdPushConstants2KHR> {};
template<> struct fex_gen_config<vkCmdPushDescriptorSet2KHR> {};
template<> struct fex_gen_config<vkCmdPushDescriptorSetWithTemplate2KHR> {};
template<> struct fex_gen_config<vkCmdSetDescriptorBufferOffsets2EXT> {};
template<> struct fex_gen_config<vkCmdBindDescriptorBufferEmbeddedSamplers2EXT> {};
#endif
template<> struct fex_gen_config<vkCreateDebugReportCallbackEXT> : fexgen::custom_host_impl {};
template<> struct fex_gen_param<vkCreateDebugReportCallbackEXT, 1, const VkDebugReportCallbackCreateInfoEXT*> : fexgen::ptr_passthrough {};
template<> struct fex_gen_config<vkDestroyDebugReportCallbackEXT> : fexgen::custom_host_impl {};
#ifndef IS_32BIT_THUNK
template<> struct fex_gen_config<vkDebugReportMessageEXT> {};
template<> struct fex_gen_config<vkDebugMarkerSetObjectTagEXT> {};
template<> struct fex_gen_config<vkDebugMarkerSetObjectNameEXT> {};
template<> struct fex_gen_config<vkCmdDebugMarkerBeginEXT> {};
template<> struct fex_gen_config<vkCmdDebugMarkerEndEXT> {};
template<> struct fex_gen_config<vkCmdDebugMarkerInsertEXT> {};
template<> struct fex_gen_config<vkCmdBindTransformFeedbackBuffersEXT> {};
template<> struct fex_gen_config<vkCmdBeginTransformFeedbackEXT> {};
template<> struct fex_gen_config<vkCmdEndTransformFeedbackEXT> {};
template<> struct fex_gen_config<vkCmdBeginQueryIndexedEXT> {};
template<> struct fex_gen_config<vkCmdEndQueryIndexedEXT> {};
template<> struct fex_gen_config<vkCmdDrawIndirectByteCountEXT> {};
template<> struct fex_gen_config<vkCreateCuModuleNVX> {};
template<> struct fex_gen_config<vkCreateCuFunctionNVX> {};
template<> struct fex_gen_config<vkDestroyCuModuleNVX> {};
template<> struct fex_gen_config<vkDestroyCuFunctionNVX> {};
template<> struct fex_gen_config<vkCmdCuLaunchKernelNVX> {};
template<> struct fex_gen_config<vkGetImageViewHandleNVX> {};
template<> struct fex_gen_config<vkGetImageViewAddressNVX> {};
template<> struct fex_gen_config<vkCmdDrawIndirectCountAMD> {};
template<> struct fex_gen_config<vkCmdDrawIndexedIndirectCountAMD> {};
template<> struct fex_gen_config<vkGetShaderInfoAMD> {};
template<> struct fex_gen_param<vkGetShaderInfoAMD, 5, void*> : fexgen::assume_compatible_data_layout {};
template<> struct fex_gen_config<vkGetPhysicalDeviceExternalImageFormatPropertiesNV> {};
template<> struct fex_gen_config<vkCmdBeginConditionalRenderingEXT> {};
template<> struct fex_gen_config<vkCmdEndConditionalRenderingEXT> {};
template<> struct fex_gen_config<vkCmdSetViewportWScalingNV> {};
template<> struct fex_gen_config<vkReleaseDisplayEXT> {};
template<> struct fex_gen_config<vkGetPhysicalDeviceSurfaceCapabilities2EXT> {};
template<> struct fex_gen_config<vkDisplayPowerControlEXT> {};
template<> struct fex_gen_config<vkRegisterDeviceEventEXT> {};
template<> struct fex_gen_config<vkRegisterDisplayEventEXT> {};
template<> struct fex_gen_config<vkGetSwapchainCounterEXT> {};
template<> struct fex_gen_config<vkGetRefreshCycleDurationGOOGLE> {};
template<> struct fex_gen_config<vkGetPastPresentationTimingGOOGLE> {};
template<> struct fex_gen_config<vkCmdSetDiscardRectangleEXT> {};
template<> struct fex_gen_config<vkCmdSetDiscardRectangleEnableEXT> {};
template<> struct fex_gen_config<vkCmdSetDiscardRectangleModeEXT> {};
template<> struct fex_gen_config<vkSetHdrMetadataEXT> {};
template<> struct fex_gen_config<vkSetDebugUtilsObjectNameEXT> {};
template<> struct fex_gen_config<vkSetDebugUtilsObjectTagEXT> {};
template<> struct fex_gen_config<vkQueueBeginDebugUtilsLabelEXT> {};
template<> struct fex_gen_config<vkQueueEndDebugUtilsLabelEXT> {};
template<> struct fex_gen_config<vkQueueInsertDebugUtilsLabelEXT> {};
template<> struct fex_gen_config<vkCmdBeginDebugUtilsLabelEXT> {};
template<> struct fex_gen_config<vkCmdEndDebugUtilsLabelEXT> {};
template<> struct fex_gen_config<vkCmdInsertDebugUtilsLabelEXT> {};
#endif
template<> struct fex_gen_config<vkCreateDebugUtilsMessengerEXT> : fexgen::custom_host_impl {};
template<> struct fex_gen_param<vkCreateDebugUtilsMessengerEXT, 1, const VkDebugUtilsMessengerCreateInfoEXT*> : fexgen::ptr_passthrough {};
template<> struct fex_gen_config<vkDestroyDebugUtilsMessengerEXT> {};
#ifndef IS_32BIT_THUNK
template<> struct fex_gen_config<vkSubmitDebugUtilsMessageEXT> {};
template<> struct fex_gen_config<vkCmdSetSampleLocationsEXT> {};
template<> struct fex_gen_config<vkGetPhysicalDeviceMultisamplePropertiesEXT> {};
template<> struct fex_gen_config<vkGetImageDrmFormatModifierPropertiesEXT> {};
template<> struct fex_gen_config<vkCreateValidationCacheEXT> {};
template<> struct fex_gen_config<vkDestroyValidationCacheEXT> {};
template<> struct fex_gen_config<vkMergeValidationCachesEXT> {};
template<> struct fex_gen_config<vkGetValidationCacheDataEXT> {};
template<> struct fex_gen_param<vkGetValidationCacheDataEXT, 3, void*> : fexgen::assume_compatible_data_layout {};
template<> struct fex_gen_config<vkCmdBindShadingRateImageNV> {};
template<> struct fex_gen_config<vkCmdSetViewportShadingRatePaletteNV> {};
template<> struct fex_gen_config<vkCmdSetCoarseSampleOrderNV> {};
template<> struct fex_gen_config<vkCreateAccelerationStructureNV> {};
template<> struct fex_gen_config<vkDestroyAccelerationStructureNV> {};
template<> struct fex_gen_config<vkGetAccelerationStructureMemoryRequirementsNV> {};
template<> struct fex_gen_config<vkBindAccelerationStructureMemoryNV> {};
template<> struct fex_gen_config<vkCmdBuildAccelerationStructureNV> {};
template<> struct fex_gen_config<vkCmdCopyAccelerationStructureNV> {};
template<> struct fex_gen_config<vkCmdTraceRaysNV> {};
template<> struct fex_gen_config<vkCreateRayTracingPipelinesNV> {};
template<> struct fex_gen_config<vkGetRayTracingShaderGroupHandlesKHR> {};
template<> struct fex_gen_param<vkGetRayTracingShaderGroupHandlesKHR, 5, void*> : fexgen::assume_compatible_data_layout {};
template<> struct fex_gen_config<vkGetRayTracingShaderGroupHandlesNV> {};
template<> struct fex_gen_param<vkGetRayTracingShaderGroupHandlesNV, 5, void*> : fexgen::assume_compatible_data_layout {};
template<> struct fex_gen_config<vkGetAccelerationStructureHandleNV> {};
template<> struct fex_gen_param<vkGetAccelerationStructureHandleNV, 3, void*> : fexgen::assume_compatible_data_layout {};
template<> struct fex_gen_config<vkCmdWriteAccelerationStructuresPropertiesNV> {};
template<> struct fex_gen_config<vkCompileDeferredNV> {};
template<> struct fex_gen_config<vkGetMemoryHostPointerPropertiesEXT> {};
template<> struct fex_gen_param<vkGetMemoryHostPointerPropertiesEXT, 2, const void*> : fexgen::assume_compatible_data_layout {};
template<> struct fex_gen_config<vkCmdWriteBufferMarkerAMD> {};
#endif
template<> struct fex_gen_config<vkGetPhysicalDeviceCalibrateableTimeDomainsEXT> {};
#ifndef IS_32BIT_THUNK
template<> struct fex_gen_config<vkGetCalibratedTimestampsEXT> {};
template<> struct fex_gen_config<vkCmdDrawMeshTasksNV> {};
template<> struct fex_gen_config<vkCmdDrawMeshTasksIndirectNV> {};
template<> struct fex_gen_config<vkCmdDrawMeshTasksIndirectCountNV> {};
template<> struct fex_gen_config<vkCmdSetExclusiveScissorEnableNV> {};
template<> struct fex_gen_config<vkCmdSetExclusiveScissorNV> {};
template<> struct fex_gen_config<vkCmdSetCheckpointNV> {};
template<> struct fex_gen_param<vkCmdSetCheckpointNV, 1, const void*> : fexgen::assume_compatible_data_layout {};
template<> struct fex_gen_config<vkGetQueueCheckpointDataNV> {};
template<> struct fex_gen_config<vkInitializePerformanceApiINTEL> {};
template<> struct fex_gen_config<vkUninitializePerformanceApiINTEL> {};
template<> struct fex_gen_config<vkCmdSetPerformanceMarkerINTEL> {};
template<> struct fex_gen_config<vkCmdSetPerformanceStreamMarkerINTEL> {};
template<> struct fex_gen_config<vkCmdSetPerformanceOverrideINTEL> {};
template<> struct fex_gen_config<vkAcquirePerformanceConfigurationINTEL> {};
template<> struct fex_gen_config<vkReleasePerformanceConfigurationINTEL> {};
template<> struct fex_gen_config<vkQueueSetPerformanceConfigurationINTEL> {};
template<> struct fex_gen_config<vkGetPerformanceParameterINTEL> {};
template<> struct fex_gen_config<vkSetLocalDimmingAMD> {};
template<> struct fex_gen_config<vkGetBufferDeviceAddressEXT> {};
template<> struct fex_gen_config<vkGetPhysicalDeviceToolPropertiesEXT> {};
template<> struct fex_gen_config<vkGetPhysicalDeviceCooperativeMatrixPropertiesNV> {};
template<> struct fex_gen_config<vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV> {};
template<> struct fex_gen_config<vkCreateHeadlessSurfaceEXT> {};
#endif
template<> struct fex_gen_config<vkCmdSetLineStippleEXT> {};
#ifndef IS_32BIT_THUNK
template<> struct fex_gen_config<vkResetQueryPoolEXT> {};
template<> struct fex_gen_config<vkCmdSetCullModeEXT> {};
template<> struct fex_gen_config<vkCmdSetFrontFaceEXT> {};
template<> struct fex_gen_config<vkCmdSetPrimitiveTopologyEXT> {};
template<> struct fex_gen_config<vkCmdSetViewportWithCountEXT> {};
template<> struct fex_gen_config<vkCmdSetScissorWithCountEXT> {};
template<> struct fex_gen_config<vkCmdBindVertexBuffers2EXT> {};
template<> struct fex_gen_config<vkCmdSetDepthTestEnableEXT> {};
template<> struct fex_gen_config<vkCmdSetDepthWriteEnableEXT> {};
template<> struct fex_gen_config<vkCmdSetDepthCompareOpEXT> {};
template<> struct fex_gen_config<vkCmdSetDepthBoundsTestEnableEXT> {};
template<> struct fex_gen_config<vkCmdSetStencilTestEnableEXT> {};
template<> struct fex_gen_config<vkCmdSetStencilOpEXT> {};
template<> struct fex_gen_config<vkCopyMemoryToImageEXT> {};
template<> struct fex_gen_config<vkCopyImageToMemoryEXT> {};
template<> struct fex_gen_config<vkCopyImageToImageEXT> {};
template<> struct fex_gen_config<vkTransitionImageLayoutEXT> {};
template<> struct fex_gen_config<vkGetImageSubresourceLayout2EXT> {};
template<> struct fex_gen_config<vkReleaseSwapchainImagesEXT> {};
template<> struct fex_gen_config<vkGetGeneratedCommandsMemoryRequirementsNV> {};
template<> struct fex_gen_config<vkCmdPreprocessGeneratedCommandsNV> {};
template<> struct fex_gen_config<vkCmdExecuteGeneratedCommandsNV> {};
template<> struct fex_gen_config<vkCmdBindPipelineShaderGroupNV> {};
template<> struct fex_gen_config<vkCreateIndirectCommandsLayoutNV> {};
template<> struct fex_gen_config<vkDestroyIndirectCommandsLayoutNV> {};
template<> struct fex_gen_config<vkCmdSetDepthBias2EXT> {};
template<> struct fex_gen_config<vkAcquireDrmDisplayEXT> {};
template<> struct fex_gen_config<vkGetDrmDisplayEXT> {};
template<> struct fex_gen_config<vkCreatePrivateDataSlotEXT> {};
template<> struct fex_gen_config<vkDestroyPrivateDataSlotEXT> {};
template<> struct fex_gen_config<vkSetPrivateDataEXT> {};
template<> struct fex_gen_config<vkGetPrivateDataEXT> {};
template<> struct fex_gen_config<vkCreateCudaModuleNV> {};
template<> struct fex_gen_config<vkGetCudaModuleCacheNV> {};
template<> struct fex_gen_config<vkCreateCudaFunctionNV> {};
template<> struct fex_gen_config<vkDestroyCudaModuleNV> {};
template<> struct fex_gen_config<vkDestroyCudaFunctionNV> {};
template<> struct fex_gen_config<vkCmdCudaLaunchKernelNV> {};
#endif
template<> struct fex_gen_config<vkGetDescriptorSetLayoutSizeEXT> {};
template<> struct fex_gen_config<vkGetDescriptorSetLayoutBindingOffsetEXT> {};
template<> struct fex_gen_config<vkGetDescriptorEXT> {};
template<> struct fex_gen_param<vkGetDescriptorEXT, 3, void*> : fexgen::assume_compatible_data_layout {};
template<> struct fex_gen_config<vkCmdBindDescriptorBuffersEXT> {};
template<> struct fex_gen_config<vkCmdSetDescriptorBufferOffsetsEXT> {};
#ifndef IS_32BIT_THUNK
template<> struct fex_gen_config<vkCmdBindDescriptorBufferEmbeddedSamplersEXT> {};
template<> struct fex_gen_config<vkGetBufferOpaqueCaptureDescriptorDataEXT> {};
template<> struct fex_gen_config<vkGetImageOpaqueCaptureDescriptorDataEXT> {};
template<> struct fex_gen_config<vkGetImageViewOpaqueCaptureDescriptorDataEXT> {};
template<> struct fex_gen_config<vkGetSamplerOpaqueCaptureDescriptorDataEXT> {};
template<> struct fex_gen_config<vkGetAccelerationStructureOpaqueCaptureDescriptorDataEXT> {};
template<> struct fex_gen_config<vkCmdSetFragmentShadingRateEnumNV> {};
template<> struct fex_gen_config<vkGetDeviceFaultInfoEXT> {};
#endif
#ifndef IS_32BIT_THUNK
template<> struct fex_gen_config<vkCmdSetVertexInputEXT> {};
#else
template<> struct fex_gen_config<vkCmdSetVertexInputEXT> : fexgen::custom_host_impl {};
template<> struct fex_gen_param<vkCmdSetVertexInputEXT, 2, const VkVertexInputBindingDescription2EXT*> : fexgen::ptr_passthrough {};
template<> struct fex_gen_param<vkCmdSetVertexInputEXT, 4, const VkVertexInputAttributeDescription2EXT*> : fexgen::ptr_passthrough {};
#endif
#ifndef IS_32BIT_THUNK
template<> struct fex_gen_config<vkGetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI> {};
template<> struct fex_gen_config<vkCmdSubpassShadingHUAWEI> {};
template<> struct fex_gen_config<vkCmdBindInvocationMaskHUAWEI> {};
#ifndef IS_32BIT_THUNK
// VkRemoteAddressNV* expands to void**, so it needs custom repacking on on 32-bit
template<> struct fex_gen_config<vkGetMemoryRemoteAddressNV> {};
#endif
template<> struct fex_gen_config<vkGetPipelinePropertiesEXT> {};
#endif
template<> struct fex_gen_config<vkCmdSetPatchControlPointsEXT> {};
template<> struct fex_gen_config<vkCmdSetRasterizerDiscardEnableEXT> {};
template<> struct fex_gen_config<vkCmdSetDepthBiasEnableEXT> {};
template<> struct fex_gen_config<vkCmdSetLogicOpEXT> {};
template<> struct fex_gen_config<vkCmdSetPrimitiveRestartEnableEXT> {};
template<> struct fex_gen_config<vkCmdSetColorWriteEnableEXT> {};
template<> struct fex_gen_config<vkCmdDrawMultiEXT> {};
template<> struct fex_gen_config<vkCmdDrawMultiIndexedEXT> {};
#ifndef IS_32BIT_THUNK
template<> struct fex_gen_config<vkCreateMicromapEXT> {};
template<> struct fex_gen_config<vkDestroyMicromapEXT> {};
template<> struct fex_gen_config<vkCmdBuildMicromapsEXT> {};
template<> struct fex_gen_config<vkBuildMicromapsEXT> {};
template<> struct fex_gen_config<vkCopyMicromapEXT> {};
template<> struct fex_gen_config<vkCopyMicromapToMemoryEXT> {};
template<> struct fex_gen_config<vkCopyMemoryToMicromapEXT> {};
template<> struct fex_gen_config<vkWriteMicromapsPropertiesEXT> {};
template<> struct fex_gen_param<vkWriteMicromapsPropertiesEXT, 5, void*> : fexgen::assume_compatible_data_layout {};
template<> struct fex_gen_config<vkCmdCopyMicromapEXT> {};
template<> struct fex_gen_config<vkCmdCopyMicromapToMemoryEXT> {};
template<> struct fex_gen_config<vkCmdCopyMemoryToMicromapEXT> {};
template<> struct fex_gen_config<vkCmdWriteMicromapsPropertiesEXT> {};
template<> struct fex_gen_config<vkGetDeviceMicromapCompatibilityEXT> {};
template<> struct fex_gen_config<vkGetMicromapBuildSizesEXT> {};
template<> struct fex_gen_config<vkCmdDrawClusterHUAWEI> {};
template<> struct fex_gen_config<vkCmdDrawClusterIndirectHUAWEI> {};
template<> struct fex_gen_config<vkSetDeviceMemoryPriorityEXT> {};
template<> struct fex_gen_config<vkGetDescriptorSetLayoutHostMappingInfoVALVE> {};
template<> struct fex_gen_config<vkGetDescriptorSetHostMappingVALVE> {};
template<> struct fex_gen_config<vkCmdCopyMemoryIndirectNV> {};
template<> struct fex_gen_config<vkCmdCopyMemoryToImageIndirectNV> {};
template<> struct fex_gen_config<vkCmdDecompressMemoryNV> {};
template<> struct fex_gen_config<vkCmdDecompressMemoryIndirectCountNV> {};
template<> struct fex_gen_config<vkGetPipelineIndirectMemoryRequirementsNV> {};
template<> struct fex_gen_config<vkCmdUpdatePipelineIndirectBufferNV> {};
template<> struct fex_gen_config<vkGetPipelineIndirectDeviceAddressNV> {};
#endif
template<> struct fex_gen_config<vkCmdSetDepthClampEnableEXT> {};
template<> struct fex_gen_config<vkCmdSetPolygonModeEXT> {};
template<> struct fex_gen_config<vkCmdSetRasterizationSamplesEXT> {};
template<> struct fex_gen_config<vkCmdSetSampleMaskEXT> {};
template<> struct fex_gen_config<vkCmdSetAlphaToCoverageEnableEXT> {};
template<> struct fex_gen_config<vkCmdSetAlphaToOneEnableEXT> {};
template<> struct fex_gen_config<vkCmdSetLogicOpEnableEXT> {};
template<> struct fex_gen_config<vkCmdSetColorBlendEnableEXT> {};
template<> struct fex_gen_config<vkCmdSetColorBlendEquationEXT> {};
template<> struct fex_gen_config<vkCmdSetColorWriteMaskEXT> {};
template<> struct fex_gen_config<vkCmdSetTessellationDomainOriginEXT> {};
template<> struct fex_gen_config<vkCmdSetRasterizationStreamEXT> {};
template<> struct fex_gen_config<vkCmdSetConservativeRasterizationModeEXT> {};
template<> struct fex_gen_config<vkCmdSetExtraPrimitiveOverestimationSizeEXT> {};
template<> struct fex_gen_config<vkCmdSetDepthClipEnableEXT> {};
template<> struct fex_gen_config<vkCmdSetSampleLocationsEnableEXT> {};
template<> struct fex_gen_config<vkCmdSetColorBlendAdvancedEXT> {};
template<> struct fex_gen_config<vkCmdSetProvokingVertexModeEXT> {};
template<> struct fex_gen_config<vkCmdSetLineRasterizationModeEXT> {};
template<> struct fex_gen_config<vkCmdSetLineStippleEnableEXT> {};
template<> struct fex_gen_config<vkCmdSetDepthClipNegativeOneToOneEXT> {};
#ifndef IS_32BIT_THUNK
template<> struct fex_gen_config<vkCmdSetViewportWScalingEnableNV> {};
template<> struct fex_gen_config<vkCmdSetViewportSwizzleNV> {};
template<> struct fex_gen_config<vkCmdSetCoverageToColorEnableNV> {};
template<> struct fex_gen_config<vkCmdSetCoverageToColorLocationNV> {};
template<> struct fex_gen_config<vkCmdSetCoverageModulationModeNV> {};
template<> struct fex_gen_config<vkCmdSetCoverageModulationTableEnableNV> {};
template<> struct fex_gen_config<vkCmdSetCoverageModulationTableNV> {};
template<> struct fex_gen_config<vkCmdSetShadingRateImageEnableNV> {};
template<> struct fex_gen_config<vkCmdSetRepresentativeFragmentTestEnableNV> {};
template<> struct fex_gen_config<vkCmdSetCoverageReductionModeNV> {};
template<> struct fex_gen_config<vkGetShaderModuleIdentifierEXT> {};
template<> struct fex_gen_config<vkGetShaderModuleCreateInfoIdentifierEXT> {};
template<> struct fex_gen_config<vkGetPhysicalDeviceOpticalFlowImageFormatsNV> {};
template<> struct fex_gen_config<vkCreateOpticalFlowSessionNV> {};
template<> struct fex_gen_config<vkDestroyOpticalFlowSessionNV> {};
template<> struct fex_gen_config<vkBindOpticalFlowSessionImageNV> {};
template<> struct fex_gen_config<vkCmdOpticalFlowExecuteNV> {};
template<> struct fex_gen_config<vkCreateShadersEXT> {};
template<> struct fex_gen_config<vkDestroyShaderEXT> {};
template<> struct fex_gen_config<vkGetShaderBinaryDataEXT> {};
template<> struct fex_gen_config<vkCmdBindShadersEXT> {};
template<> struct fex_gen_config<vkGetFramebufferTilePropertiesQCOM> {};
template<> struct fex_gen_config<vkGetDynamicRenderingTilePropertiesQCOM> {};
template<> struct fex_gen_config<vkSetLatencySleepModeNV> {};
template<> struct fex_gen_config<vkLatencySleepNV> {};
template<> struct fex_gen_config<vkSetLatencyMarkerNV> {};
template<> struct fex_gen_config<vkGetLatencyTimingsNV> {};
template<> struct fex_gen_config<vkQueueNotifyOutOfBandNV> {};
template<> struct fex_gen_config<vkCmdSetAttachmentFeedbackLoopEnableEXT> {};
template<> struct fex_gen_config<vkCreateAccelerationStructureKHR> {};
template<> struct fex_gen_config<vkDestroyAccelerationStructureKHR> {};
template<> struct fex_gen_config<vkCmdBuildAccelerationStructuresKHR> {};
template<> struct fex_gen_config<vkCmdBuildAccelerationStructuresIndirectKHR> {};
template<> struct fex_gen_config<vkBuildAccelerationStructuresKHR> {};
template<> struct fex_gen_config<vkCopyAccelerationStructureKHR> {};
template<> struct fex_gen_config<vkCopyAccelerationStructureToMemoryKHR> {};
template<> struct fex_gen_config<vkCopyMemoryToAccelerationStructureKHR> {};
template<> struct fex_gen_config<vkWriteAccelerationStructuresPropertiesKHR> {};
template<> struct fex_gen_param<vkWriteAccelerationStructuresPropertiesKHR, 5, void*> : fexgen::assume_compatible_data_layout {};
template<> struct fex_gen_config<vkCmdCopyAccelerationStructureKHR> {};
template<> struct fex_gen_config<vkCmdCopyAccelerationStructureToMemoryKHR> {};
template<> struct fex_gen_config<vkCmdCopyMemoryToAccelerationStructureKHR> {};
template<> struct fex_gen_config<vkGetAccelerationStructureDeviceAddressKHR> {};
template<> struct fex_gen_config<vkCmdWriteAccelerationStructuresPropertiesKHR> {};
template<> struct fex_gen_config<vkGetDeviceAccelerationStructureCompatibilityKHR> {};
template<> struct fex_gen_config<vkGetAccelerationStructureBuildSizesKHR> {};
template<> struct fex_gen_config<vkCmdTraceRaysKHR> {};
template<> struct fex_gen_config<vkCreateRayTracingPipelinesKHR> {};
template<> struct fex_gen_config<vkGetRayTracingCaptureReplayShaderGroupHandlesKHR> {};
template<> struct fex_gen_param<vkGetRayTracingCaptureReplayShaderGroupHandlesKHR, 5, void*> : fexgen::assume_compatible_data_layout {};
template<> struct fex_gen_config<vkCmdTraceRaysIndirectKHR> {};
template<> struct fex_gen_config<vkGetRayTracingShaderGroupStackSizeKHR> {};
template<> struct fex_gen_config<vkCmdSetRayTracingPipelineStackSizeKHR> {};
template<> struct fex_gen_config<vkCmdDrawMeshTasksEXT> {};
template<> struct fex_gen_config<vkCmdDrawMeshTasksIndirectEXT> {};
template<> struct fex_gen_config<vkCmdDrawMeshTasksIndirectCountEXT> {};

// vulkan_xlib_xrandr.h
template<> struct fex_gen_config<vkAcquireXlibDisplayEXT> {};
template<> struct fex_gen_config<vkGetRandROutputDisplayEXT> {};

// vulkan_wayland.h
#endif
template<> struct fex_gen_config<vkCreateWaylandSurfaceKHR> {};
#ifndef IS_32BIT_THUNK
template<> struct fex_gen_config<vkGetPhysicalDeviceWaylandPresentationSupportKHR> {};

// vulkan_xcb.h
template<> struct fex_gen_config<vkCreateXcbSurfaceKHR> {};
template<> struct fex_gen_config<vkGetPhysicalDeviceXcbPresentationSupportKHR> {};

// vulkan_xlib.h
template<> struct fex_gen_config<vkCreateXlibSurfaceKHR> {};
template<> struct fex_gen_config<vkGetPhysicalDeviceXlibPresentationSupportKHR> {};
#endif
} // namespace internal
