#ifdef IS_32BIT_THUNK
// Register structs with an extension point (pNext). Any other members that need customization are listed below.
// Generated using
// for i in `grep VK_STRUCTURE_TYPE vk.xml -B1 | grep category=\"struct\" | cut -d'"' -f 4 | sort`
// do
//   grep $i vulkan_{core,wayland,xcb,xlib,xlib_xrandr}.h >& /dev/null && echo $i
// done | awk '{ print "template<> struct fex_gen_config<&"$1"::pNext> : fexgen::custom_repack {};" }'
template<>
struct fex_gen_config<&VkAccelerationStructureBuildSizesInfoKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkAccelerationStructureCaptureDescriptorDataInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkAccelerationStructureCreateInfoKHR::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkAccelerationStructureCreateInfoNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkAccelerationStructureDeviceAddressInfoKHR::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkAccelerationStructureGeometryAabbsDataKHR::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkAccelerationStructureGeometryInstancesDataKHR::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkAccelerationStructureGeometryKHR::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkAccelerationStructureGeometryMotionTrianglesDataNV::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkAccelerationStructureGeometryTrianglesDataKHR::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkAccelerationStructureInfoNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkAccelerationStructureMemoryRequirementsInfoNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkAccelerationStructureMotionInfoNV::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkAccelerationStructureTrianglesOpacityMicromapEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkAccelerationStructureVersionInfoKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkAcquireNextImageInfoKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkAcquireProfilingLockInfoKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkAmigoProfilingSubmitInfoSEC::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkApplicationInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkAttachmentDescription2::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkAttachmentDescriptionStencilLayout::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkAttachmentReference2::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkAttachmentReferenceStencilLayout::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkAttachmentSampleCountInfoAMD::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkBindAccelerationStructureMemoryInfoNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkBindBufferMemoryDeviceGroupInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkBindBufferMemoryInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkBindImageMemoryDeviceGroupInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkBindImageMemoryInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkBindImageMemorySwapchainInfoKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkBindImagePlaneMemoryInfo::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkBindSparseInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkBindVideoSessionMemoryInfoKHR::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkBlitImageInfo2::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkBufferCaptureDescriptorDataInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkBufferCopy2::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkBufferCreateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkBufferDeviceAddressCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkBufferDeviceAddressInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkBufferImageCopy2::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkBufferMemoryBarrier::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkBufferMemoryBarrier2::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkBufferMemoryRequirementsInfo2::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkBufferOpaqueCaptureAddressCreateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkBufferUsageFlags2CreateInfoKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkBufferViewCreateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkCalibratedTimestampInfoEXT::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkCheckpointData2NV::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkCheckpointDataNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkCommandBufferAllocateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkCommandBufferBeginInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkCommandBufferInheritanceConditionalRenderingInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkCommandBufferInheritanceInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkCommandBufferInheritanceRenderingInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkCommandBufferInheritanceRenderPassTransformInfoQCOM::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkCommandBufferInheritanceViewportScissorInfoNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkCommandBufferSubmitInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkCommandPoolCreateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkComputePipelineCreateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkComputePipelineIndirectBufferInfoNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkConditionalRenderingBeginInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkCooperativeMatrixPropertiesKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkCooperativeMatrixPropertiesNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkCopyAccelerationStructureInfoKHR::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkCopyAccelerationStructureToMemoryInfoKHR::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkCopyBufferInfo2::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkCopyBufferToImageInfo2::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkCopyCommandTransformInfoQCOM::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkCopyDescriptorSet::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkCopyImageInfo2::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkCopyImageToBufferInfo2::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkCopyImageToImageInfoEXT::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkCopyImageToMemoryInfoEXT::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkCopyMemoryToAccelerationStructureInfoKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkCopyMemoryToImageInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkCopyMemoryToImageInfoEXT::pRegions> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkCopyMemoryToMicromapInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkCopyMicromapInfoEXT::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkCopyMicromapToMemoryInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkCuFunctionCreateInfoNVX::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkCuLaunchInfoNVX::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkCuModuleCreateInfoNVX::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDebugMarkerMarkerInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDebugMarkerObjectNameInfoEXT::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkDebugMarkerObjectTagInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDebugReportCallbackCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDebugUtilsLabelEXT::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkDebugUtilsMessengerCallbackDataEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDebugUtilsMessengerCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDebugUtilsObjectNameInfoEXT::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkDebugUtilsObjectTagInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDedicatedAllocationBufferCreateInfoNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDedicatedAllocationImageCreateInfoNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDedicatedAllocationMemoryAllocateInfoNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDependencyInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDepthBiasInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDepthBiasRepresentationInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDescriptorAddressInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDescriptorBufferBindingInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDescriptorBufferBindingPushDescriptorBufferHandleEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDescriptorGetInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDescriptorPoolCreateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDescriptorPoolInlineUniformBlockCreateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDescriptorSetAllocateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDescriptorSetBindingReferenceVALVE::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDescriptorSetLayoutBindingFlagsCreateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDescriptorSetLayoutCreateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDescriptorSetLayoutHostMappingInfoVALVE::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDescriptorSetLayoutSupport::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDescriptorSetVariableDescriptorCountAllocateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDescriptorSetVariableDescriptorCountLayoutSupport::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDescriptorUpdateTemplateCreateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDeviceAddressBindingCallbackDataEXT::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkDeviceBufferMemoryRequirements::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDeviceCreateInfo::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkDeviceDeviceMemoryReportCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDeviceDiagnosticsConfigCreateInfoNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDeviceEventInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDeviceFaultCountsEXT::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkDeviceFaultInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDeviceGroupBindSparseInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDeviceGroupCommandBufferBeginInfo::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkDeviceGroupDeviceCreateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDeviceGroupPresentCapabilitiesKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDeviceGroupPresentInfoKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDeviceGroupRenderPassBeginInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDeviceGroupSubmitInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDeviceGroupSwapchainCreateInfoKHR::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkDeviceImageMemoryRequirements::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkDeviceImageSubresourceInfoKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDeviceMemoryOpaqueCaptureAddressInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDeviceMemoryOverallocationCreateInfoAMD::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDeviceMemoryReportCallbackDataEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDevicePrivateDataCreateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDeviceQueueCreateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDeviceQueueGlobalPriorityCreateInfoKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDeviceQueueInfo2::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkDirectDriverLoadingInfoLUNARG::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkDirectDriverLoadingListLUNARG::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDisplayEventInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDisplayModeCreateInfoKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDisplayModeProperties2KHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDisplayNativeHdrSurfaceCapabilitiesAMD::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDisplayPlaneCapabilities2KHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDisplayPlaneInfo2KHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDisplayPlaneProperties2KHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDisplayPowerInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDisplayPresentInfoKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDisplayProperties2KHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDisplaySurfaceCreateInfoKHR::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkDrmFormatModifierPropertiesList2EXT::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkDrmFormatModifierPropertiesListEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkEventCreateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkExportFenceCreateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkExportMemoryAllocateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkExportMemoryAllocateInfoNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkExportSemaphoreCreateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkExternalBufferProperties::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkExternalFenceProperties::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkExternalImageFormatProperties::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkExternalMemoryAcquireUnmodifiedEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkExternalMemoryBufferCreateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkExternalMemoryImageCreateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkExternalMemoryImageCreateInfoNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkExternalSemaphoreProperties::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkFenceCreateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkFenceGetFdInfoKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkFilterCubicImageViewImageFormatPropertiesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkFormatProperties2::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkFormatProperties3::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkFragmentShadingRateAttachmentInfoKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkFramebufferAttachmentImageInfo::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkFramebufferAttachmentsCreateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkFramebufferCreateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkFramebufferMixedSamplesCombinationNV::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkGeneratedCommandsInfoNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkGeneratedCommandsMemoryRequirementsInfoNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkGeometryAABBNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkGeometryNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkGeometryTrianglesNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkGraphicsPipelineCreateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkGraphicsPipelineLibraryCreateInfoEXT::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkGraphicsPipelineShaderGroupsCreateInfoNV::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkGraphicsShaderGroupCreateInfoNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkHeadlessSurfaceCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkHostImageCopyDevicePerformanceQueryEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkHostImageLayoutTransitionInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkImageBlit2::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkImageCaptureDescriptorDataInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkImageCompressionControlEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkImageCompressionPropertiesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkImageCopy2::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkImageCreateInfo::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkImageDrmFormatModifierExplicitCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkImageDrmFormatModifierListCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkImageDrmFormatModifierPropertiesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkImageFormatListCreateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkImageFormatProperties2::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkImageMemoryBarrier::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkImageMemoryBarrier2::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkImageMemoryRequirementsInfo2::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkImagePlaneMemoryRequirementsInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkImageResolve2::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkImageSparseMemoryRequirementsInfo2::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkImageStencilUsageCreateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkImageSubresource2KHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkImageSwapchainCreateInfoKHR::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkImageToMemoryCopyEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkImageViewAddressPropertiesNVX::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkImageViewASTCDecodeModeEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkImageViewCaptureDescriptorDataInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkImageViewCreateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkImageViewHandleInfoNVX::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkImageViewMinLodCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkImageViewSampleWeightCreateInfoQCOM::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkImageViewSlicedCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkImageViewUsageCreateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkImportFenceFdInfoKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkImportMemoryFdInfoKHR::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkImportMemoryHostPointerInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkImportSemaphoreFdInfoKHR::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkIndirectCommandsLayoutCreateInfoNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkIndirectCommandsLayoutTokenNV::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkInitializePerformanceApiInfoINTEL::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkInstanceCreateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkMappedMemoryRange::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkMemoryAllocateFlagsInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkMemoryAllocateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkMemoryBarrier::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkMemoryBarrier2::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkMemoryDedicatedAllocateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkMemoryDedicatedRequirements::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkMemoryFdPropertiesKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkMemoryGetFdInfoKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkMemoryGetRemoteAddressInfoNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkMemoryHostPointerPropertiesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkMemoryMapInfoKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkMemoryOpaqueCaptureAddressAllocateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkMemoryPriorityAllocateInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkMemoryRequirements2::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkMemoryToImageCopyEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkMemoryToImageCopyEXT::pHostPointer> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkMemoryUnmapInfoKHR::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkMicromapBuildInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkMicromapBuildSizesInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkMicromapCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkMicromapVersionInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkMultisampledRenderToSingleSampledInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkMultisamplePropertiesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkMultiviewPerViewAttributesInfoNVX::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkMultiviewPerViewRenderAreasRenderPassBeginInfoQCOM::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkMutableDescriptorTypeCreateInfoEXT::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkOpaqueCaptureDescriptorDataCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkOpticalFlowExecuteInfoNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkOpticalFlowImageFormatInfoNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkOpticalFlowImageFormatPropertiesNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkOpticalFlowSessionCreateInfoNV::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkOpticalFlowSessionCreatePrivateDataInfoNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPerformanceConfigurationAcquireInfoINTEL::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPerformanceCounterDescriptionKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPerformanceCounterKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPerformanceMarkerInfoINTEL::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPerformanceOverrideInfoINTEL::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPerformanceQuerySubmitInfoKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPerformanceStreamMarkerInfoINTEL::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDevice16BitStorageFeatures::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDevice4444FormatsFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDevice8BitStorageFeatures::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceAccelerationStructureFeaturesKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceAccelerationStructurePropertiesKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceAddressBindingReportFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceAmigoProfilingFeaturesSEC::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceASTCDecodeFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceAttachmentFeedbackLoopDynamicStateFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceAttachmentFeedbackLoopLayoutFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceBlendOperationAdvancedFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceBlendOperationAdvancedPropertiesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceBorderColorSwizzleFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceBufferDeviceAddressFeatures::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceBufferDeviceAddressFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceClusterCullingShaderFeaturesHUAWEI::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceClusterCullingShaderPropertiesHUAWEI::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceCoherentMemoryFeaturesAMD::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceColorWriteEnableFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceComputeShaderDerivativesFeaturesNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceConditionalRenderingFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceConservativeRasterizationPropertiesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceCooperativeMatrixFeaturesKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceCooperativeMatrixFeaturesNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceCooperativeMatrixPropertiesKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceCooperativeMatrixPropertiesNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceCopyMemoryIndirectFeaturesNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceCopyMemoryIndirectPropertiesNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceCornerSampledImageFeaturesNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceCoverageReductionModeFeaturesNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceCustomBorderColorFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceCustomBorderColorPropertiesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceDedicatedAllocationImageAliasingFeaturesNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceDepthBiasControlFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceDepthClampZeroOneFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceDepthClipControlFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceDepthClipEnableFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceDepthStencilResolveProperties::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceDescriptorBufferDensityMapPropertiesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceDescriptorBufferFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceDescriptorBufferPropertiesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceDescriptorIndexingFeatures::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceDescriptorIndexingProperties::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceDescriptorSetHostMappingFeaturesVALVE::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceDeviceGeneratedCommandsComputeFeaturesNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceDeviceGeneratedCommandsFeaturesNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceDeviceGeneratedCommandsPropertiesNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceDeviceMemoryReportFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceDiagnosticsConfigFeaturesNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceDiscardRectanglePropertiesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceDriverProperties::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceDrmPropertiesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceDynamicRenderingFeatures::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceDynamicRenderingUnusedAttachmentsFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceExclusiveScissorFeaturesNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceExtendedDynamicState2FeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceExtendedDynamicState3FeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceExtendedDynamicState3PropertiesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceExtendedDynamicStateFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceExternalBufferInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceExternalFenceInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceExternalImageFormatInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceExternalMemoryHostPropertiesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceExternalMemoryRDMAFeaturesNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceExternalSemaphoreInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceFaultFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceFeatures2::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceFloatControlsProperties::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceFragmentDensityMap2FeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceFragmentDensityMap2PropertiesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceFragmentDensityMapFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceFragmentDensityMapOffsetFeaturesQCOM::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceFragmentDensityMapOffsetPropertiesQCOM::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceFragmentDensityMapPropertiesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceFragmentShaderBarycentricFeaturesKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceFragmentShaderBarycentricPropertiesKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceFragmentShadingRateEnumsFeaturesNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceFragmentShadingRateEnumsPropertiesNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceFragmentShadingRateFeaturesKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceFragmentShadingRateKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceFragmentShadingRatePropertiesKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceGlobalPriorityQueryFeaturesKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceGraphicsPipelineLibraryFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceGraphicsPipelineLibraryPropertiesEXT::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkPhysicalDeviceGroupProperties::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceHostImageCopyFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceHostImageCopyPropertiesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceHostQueryResetFeatures::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceIDProperties::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceImage2DViewOf3DFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceImageCompressionControlFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceImageCompressionControlSwapchainFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceImageDrmFormatModifierInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceImageFormatInfo2::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceImagelessFramebufferFeatures::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceImageProcessingFeaturesQCOM::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceImageProcessingPropertiesQCOM::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceImageRobustnessFeatures::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceImageSlicedViewOf3DFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceImageViewImageFormatInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceImageViewMinLodFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceIndexTypeUint8FeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceInheritedViewportScissorFeaturesNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceInlineUniformBlockFeatures::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceInlineUniformBlockProperties::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceInvocationMaskFeaturesHUAWEI::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceLegacyDitheringFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceLinearColorAttachmentFeaturesNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceLineRasterizationFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceLineRasterizationPropertiesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceMaintenance3Properties::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceMaintenance4Features::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceMaintenance4Properties::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceMaintenance5FeaturesKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceMaintenance5PropertiesKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceMemoryBudgetPropertiesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceMemoryDecompressionFeaturesNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceMemoryDecompressionPropertiesNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceMemoryPriorityFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceMemoryProperties2::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceMeshShaderFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceMeshShaderFeaturesNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceMeshShaderPropertiesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceMeshShaderPropertiesNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceMultiDrawFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceMultiDrawPropertiesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceMultisampledRenderToSingleSampledFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceMultiviewFeatures::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceMultiviewPerViewAttributesPropertiesNVX::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceMultiviewPerViewRenderAreasFeaturesQCOM::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceMultiviewPerViewViewportsFeaturesQCOM::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceMultiviewProperties::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceNonSeamlessCubeMapFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceOpacityMicromapFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceOpacityMicromapPropertiesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceOpticalFlowFeaturesNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceOpticalFlowPropertiesNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDevicePageableDeviceLocalMemoryFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDevicePCIBusInfoPropertiesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDevicePerformanceQueryFeaturesKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDevicePerformanceQueryPropertiesKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDevicePipelineCreationCacheControlFeatures::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDevicePipelineExecutablePropertiesFeaturesKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDevicePipelineLibraryGroupHandlesFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDevicePipelinePropertiesFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDevicePipelineProtectedAccessFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDevicePipelineRobustnessFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDevicePipelineRobustnessPropertiesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDevicePointClippingProperties::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDevicePresentBarrierFeaturesNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDevicePresentIdFeaturesKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDevicePresentWaitFeaturesKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDevicePrimitivesGeneratedQueryFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDevicePrimitiveTopologyListRestartFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDevicePrivateDataFeatures::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceProperties2::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceProtectedMemoryFeatures::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceProtectedMemoryProperties::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceProvokingVertexFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceProvokingVertexPropertiesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDevicePushDescriptorPropertiesKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceRasterizationOrderAttachmentAccessFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceRayQueryFeaturesKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceRayTracingInvocationReorderFeaturesNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceRayTracingInvocationReorderPropertiesNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceRayTracingMaintenance1FeaturesKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceRayTracingMotionBlurFeaturesNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceRayTracingPipelineFeaturesKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceRayTracingPipelinePropertiesKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceRayTracingPositionFetchFeaturesKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceRayTracingPropertiesNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceRepresentativeFragmentTestFeaturesNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceRGBA10X6FormatsFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceRobustness2FeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceRobustness2PropertiesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceSampleLocationsPropertiesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceSamplerFilterMinmaxProperties::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceSamplerYcbcrConversionFeatures::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceScalarBlockLayoutFeatures::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceSeparateDepthStencilLayoutsFeatures::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceShaderAtomicFloat2FeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceShaderAtomicFloatFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceShaderAtomicInt64Features::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceShaderClockFeaturesKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceShaderCoreBuiltinsFeaturesARM::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceShaderCoreBuiltinsPropertiesARM::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceShaderCoreProperties2AMD::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceShaderCorePropertiesAMD::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceShaderCorePropertiesARM::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceShaderDemoteToHelperInvocationFeatures::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceShaderDrawParametersFeatures::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceShaderEarlyAndLateFragmentTestsFeaturesAMD::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceShaderFloat16Int8Features::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceShaderImageAtomicInt64FeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceShaderImageFootprintFeaturesNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceShaderIntegerDotProductFeatures::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceShaderIntegerDotProductProperties::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceShaderIntegerFunctions2FeaturesINTEL::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceShaderModuleIdentifierFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceShaderModuleIdentifierPropertiesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceShaderObjectFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceShaderObjectPropertiesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceShaderSMBuiltinsFeaturesNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceShaderSMBuiltinsPropertiesNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceShaderSubgroupExtendedTypesFeatures::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceShaderSubgroupUniformControlFlowFeaturesKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceShaderTerminateInvocationFeatures::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceShaderTileImageFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceShaderTileImagePropertiesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceShadingRateImageFeaturesNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceShadingRateImagePropertiesNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceSparseImageFormatInfo2::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceSubgroupProperties::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceSubgroupSizeControlFeatures::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceSubgroupSizeControlProperties::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceSubpassMergeFeedbackFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceSubpassShadingFeaturesHUAWEI::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceSubpassShadingPropertiesHUAWEI::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceSurfaceInfo2KHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceSynchronization2Features::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceTexelBufferAlignmentFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceTexelBufferAlignmentProperties::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceTextureCompressionASTCHDRFeatures::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceTilePropertiesFeaturesQCOM::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceTimelineSemaphoreFeatures::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceTimelineSemaphoreProperties::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceToolProperties::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceTransformFeedbackFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceTransformFeedbackPropertiesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceUniformBufferStandardLayoutFeatures::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceVariablePointersFeatures::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceVertexAttributeDivisorFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceVertexAttributeDivisorPropertiesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceVertexInputDynamicStateFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceVideoFormatInfoKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceVulkan11Features::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceVulkan11Properties::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceVulkan12Features::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceVulkan12Properties::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceVulkan13Features::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceVulkan13Properties::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceVulkanMemoryModelFeatures::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceWorkgroupMemoryExplicitLayoutFeaturesKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceYcbcr2Plane444FormatsFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceYcbcrImageArraysFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPhysicalDeviceZeroInitializeWorkgroupMemoryFeatures::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPipelineCacheCreateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPipelineColorBlendAdvancedStateCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPipelineColorBlendStateCreateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPipelineColorWriteCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPipelineCompilerControlCreateInfoAMD::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPipelineCoverageModulationStateCreateInfoNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPipelineCoverageReductionStateCreateInfoNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPipelineCoverageToColorStateCreateInfoNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPipelineCreateFlags2CreateInfoKHR::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkPipelineCreationFeedbackCreateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPipelineDepthStencilStateCreateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPipelineDiscardRectangleStateCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPipelineDynamicStateCreateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPipelineExecutableInfoKHR::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkPipelineExecutableInternalRepresentationKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPipelineExecutablePropertiesKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPipelineExecutableStatisticKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPipelineFragmentShadingRateEnumStateCreateInfoNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPipelineFragmentShadingRateStateCreateInfoKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPipelineIndirectDeviceAddressInfoNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPipelineInfoKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPipelineInputAssemblyStateCreateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPipelineLayoutCreateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPipelineLibraryCreateInfoKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPipelineMultisampleStateCreateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPipelinePropertiesIdentifierEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPipelineRasterizationConservativeStateCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPipelineRasterizationDepthClipStateCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPipelineRasterizationLineStateCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPipelineRasterizationProvokingVertexStateCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPipelineRasterizationStateCreateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPipelineRasterizationStateRasterizationOrderAMD::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPipelineRasterizationStateStreamCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPipelineRenderingCreateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPipelineRepresentativeFragmentTestStateCreateInfoNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPipelineRobustnessCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPipelineSampleLocationsStateCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPipelineShaderStageCreateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPipelineShaderStageModuleIdentifierCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPipelineShaderStageRequiredSubgroupSizeCreateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPipelineTessellationDomainOriginStateCreateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPipelineTessellationStateCreateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPipelineVertexInputDivisorStateCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPipelineVertexInputStateCreateInfo::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkPipelineViewportCoarseSampleOrderStateCreateInfoNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPipelineViewportDepthClipControlCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPipelineViewportExclusiveScissorStateCreateInfoNV::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkPipelineViewportShadingRateImageStateCreateInfoNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPipelineViewportStateCreateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPipelineViewportSwizzleStateCreateInfoNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPipelineViewportWScalingStateCreateInfoNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPresentIdKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPresentInfoKHR::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkPresentRegionsKHR::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkPresentTimesInfoGOOGLE::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkPrivateDataSlotCreateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkProtectedSubmitInfo::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkQueryLowLatencySupportNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkQueryPoolCreateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkQueryPoolPerformanceCreateInfoKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkQueryPoolPerformanceQueryCreateInfoINTEL::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkQueueFamilyCheckpointProperties2NV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkQueueFamilyCheckpointPropertiesNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkQueueFamilyGlobalPriorityPropertiesKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkQueueFamilyProperties2::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkQueueFamilyQueryResultStatusPropertiesKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkQueueFamilyVideoPropertiesKHR::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkRayTracingPipelineCreateInfoKHR::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkRayTracingPipelineCreateInfoNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkRayTracingPipelineInterfaceCreateInfoKHR::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkRayTracingShaderGroupCreateInfoKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkRayTracingShaderGroupCreateInfoNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkReleaseSwapchainImagesInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkRenderingAreaInfoKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkRenderingAttachmentInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkRenderingFragmentDensityMapAttachmentInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkRenderingFragmentShadingRateAttachmentInfoKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkRenderingInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkRenderPassAttachmentBeginInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkRenderPassBeginInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkRenderPassCreateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkRenderPassCreateInfo2::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkRenderPassCreationControlEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkRenderPassCreationFeedbackCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkRenderPassFragmentDensityMapCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkRenderPassInputAttachmentAspectCreateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkRenderPassMultiviewCreateInfo::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkRenderPassSampleLocationsBeginInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkRenderPassSubpassFeedbackCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkRenderPassTransformBeginInfoQCOM::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkResolveImageInfo2::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkSampleLocationsInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkSamplerBorderColorComponentMappingCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkSamplerCaptureDescriptorDataInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkSamplerCreateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkSamplerCustomBorderColorCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkSamplerReductionModeCreateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkSamplerYcbcrConversionCreateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkSamplerYcbcrConversionImageFormatProperties::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkSamplerYcbcrConversionInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkSemaphoreCreateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkSemaphoreGetFdInfoKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkSemaphoreSignalInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkSemaphoreSubmitInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkSemaphoreTypeCreateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkSemaphoreWaitInfo::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkShaderCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkShaderModuleCreateInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkShaderModuleIdentifierEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkShaderModuleValidationCacheCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkSharedPresentSurfaceCapabilitiesKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkSparseImageFormatProperties2::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkSparseImageMemoryRequirements2::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkSubmitInfo::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkSubmitInfo2::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkSubpassBeginInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkSubpassDependency2::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkSubpassDescription2::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkSubpassDescriptionDepthStencilResolve::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkSubpassEndInfo::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkSubpassFragmentDensityMapOffsetEndInfoQCOM::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkSubpassResolvePerformanceQueryEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkSubpassShadingPipelineCreateInfoHUAWEI::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkSubresourceHostMemcpySizeEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkSubresourceLayout2KHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkSurfaceCapabilities2EXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkSurfaceCapabilities2KHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkSurfaceCapabilitiesPresentBarrierNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkSurfaceFormat2KHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkSurfacePresentModeCompatibilityEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkSurfacePresentModeEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkSurfacePresentScalingCapabilitiesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkSurfaceProtectedCapabilitiesKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkSwapchainCounterCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkSwapchainCreateInfoKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkSwapchainDisplayNativeHdrCreateInfoAMD::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkSwapchainPresentBarrierCreateInfoNV::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkSwapchainPresentFenceInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkSwapchainPresentModeInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkSwapchainPresentModesCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkSwapchainPresentScalingCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkTextureLODGatherFormatPropertiesAMD::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkTilePropertiesQCOM::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkTimelineSemaphoreSubmitInfo::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkValidationCacheCreateInfoEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkValidationFeaturesEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkValidationFlagsEXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkVertexInputAttributeDescription2EXT::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkVertexInputBindingDescription2EXT::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkVideoBeginCodingInfoKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkVideoCapabilitiesKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkVideoCodingControlInfoKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkVideoDecodeCapabilitiesKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkVideoDecodeH264CapabilitiesKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkVideoDecodeH264DpbSlotInfoKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkVideoDecodeH264PictureInfoKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkVideoDecodeH264ProfileInfoKHR::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkVideoDecodeH264SessionParametersAddInfoKHR::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkVideoDecodeH264SessionParametersCreateInfoKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkVideoDecodeH265CapabilitiesKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkVideoDecodeH265DpbSlotInfoKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkVideoDecodeH265PictureInfoKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkVideoDecodeH265ProfileInfoKHR::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkVideoDecodeH265SessionParametersAddInfoKHR::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkVideoDecodeH265SessionParametersCreateInfoKHR::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkVideoDecodeInfoKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkVideoDecodeUsageInfoKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkVideoEndCodingInfoKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkVideoFormatPropertiesKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkVideoPictureResourceInfoKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkVideoProfileInfoKHR::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkVideoProfileListInfoKHR::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkVideoReferenceSlotInfoKHR::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkVideoSessionCreateInfoKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkVideoSessionMemoryRequirementsKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkVideoSessionParametersCreateInfoKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkVideoSessionParametersUpdateInfoKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkWaylandSurfaceCreateInfoKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkWriteDescriptorSet::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkWriteDescriptorSetAccelerationStructureKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkWriteDescriptorSetAccelerationStructureNV::pNext> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkWriteDescriptorSetInlineUniformBlock::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkXcbSurfaceCreateInfoKHR::pNext> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkXlibSurfaceCreateInfoKHR::pNext> : fexgen::custom_repack {};


template<>
struct fex_gen_config<&VkCommandBufferBeginInfo::pInheritanceInfo> : fexgen::custom_repack {};

template<>
struct fex_gen_config<&VkDeviceCreateInfo::pQueueCreateInfos> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDeviceCreateInfo::ppEnabledLayerNames> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDeviceCreateInfo::ppEnabledExtensionNames> : fexgen::custom_repack {};

template<>
struct fex_gen_config<&VkDependencyInfo::pMemoryBarriers> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDependencyInfo::pBufferMemoryBarriers> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkDependencyInfo::pImageMemoryBarriers> : fexgen::custom_repack {};

template<>
struct fex_gen_config<&VkDescriptorGetInfoEXT::data> : fexgen::custom_repack {};

template<>
struct fex_gen_config<&VkDescriptorSetLayoutCreateInfo::pBindings> : fexgen::custom_repack {};

template<>
struct fex_gen_config<&VkDescriptorUpdateTemplateCreateInfo::pDescriptorUpdateEntries> : fexgen::custom_repack {};

template<>
struct fex_gen_config<&VkGraphicsPipelineCreateInfo::pStages> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkGraphicsPipelineCreateInfo::pVertexInputState> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkGraphicsPipelineCreateInfo::pInputAssemblyState> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkGraphicsPipelineCreateInfo::pTessellationState> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkGraphicsPipelineCreateInfo::pViewportState> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkGraphicsPipelineCreateInfo::pRasterizationState> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkGraphicsPipelineCreateInfo::pMultisampleState> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkGraphicsPipelineCreateInfo::pDepthStencilState> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkGraphicsPipelineCreateInfo::pColorBlendState> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkGraphicsPipelineCreateInfo::pDynamicState> : fexgen::custom_repack {};

template<>
struct fex_gen_config<&VkInstanceCreateInfo::pApplicationInfo> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkInstanceCreateInfo::ppEnabledLayerNames> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkInstanceCreateInfo::ppEnabledExtensionNames> : fexgen::custom_repack {};

template<>
struct fex_gen_config<&VkRenderPassCreateInfo::pSubpasses> : fexgen::custom_repack {};
// NOTE: pDependencies and pAttachments point to ABI-compatible data

template<>
struct fex_gen_config<&VkRenderPassCreateInfo2::pAttachments> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkRenderPassCreateInfo2::pSubpasses> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkRenderPassCreateInfo2::pDependencies> : fexgen::custom_repack {};

template<>
struct fex_gen_config<&VkPipelineShaderStageCreateInfo::pSpecializationInfo> : fexgen::custom_repack {};
// template<> struct fex_gen_config<&VkSpecializationInfo::pMapEntries> : fexgen::custom_repack {};

// TODO: Support annotating as assume_compatible_data_layout instead
template<>
struct fex_gen_config<&VkPipelineCacheCreateInfo::pInitialData> : fexgen::custom_repack {};

// Command buffers are dispatchable handles, so on 32-bit they need to be repacked
template<>
struct fex_gen_config<&VkSubmitInfo::pCommandBuffers> : fexgen::custom_repack {};

template<>
struct fex_gen_config<&VkRenderingInfo::pColorAttachments> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkRenderingInfo::pDepthAttachment> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkRenderingInfo::pStencilAttachment> : fexgen::custom_repack {};

// TODO: Support annotating as assume_compatible_data_layout instead
template<>
struct fex_gen_config<&VkRenderPassBeginInfo::pClearValues> : fexgen::custom_repack {};

template<>
struct fex_gen_config<&VkSubpassDescription2::pInputAttachments> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkSubpassDescription2::pColorAttachments> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkSubpassDescription2::pResolveAttachments> : fexgen::custom_repack {};
template<>
struct fex_gen_config<&VkSubpassDescription2::pDepthStencilAttachment> : fexgen::custom_repack {};

#endif
