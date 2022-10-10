#include <common/CrossArchEvent.h>
#include <common/GeneratorInterface.h>

#define CL_VERSION_3_0
#define CL_USE_DEPRECATED_OPENCL_1_0_APIS
#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#define CL_USE_DEPRECATED_OPENCL_2_0_APIS
#define CL_USE_DEPRECATED_OPENCL_2_1_APIS
#define CL_USE_DEPRECATED_OPENCL_2_2_APIS
#include <CL/cl.h>

template<auto>
struct fex_gen_config {
    unsigned version = 1;
};


void FEX_GiveEventHandlers(CrossArchEvent*, CrossArchEvent*, void**);
template<> struct fex_gen_config<FEX_GiveEventHandlers> : fexgen::custom_host_impl, fexgen::custom_guest_entrypoint {};

template<> struct fex_gen_config<clGetExtensionFunctionAddress> : fexgen::custom_guest_entrypoint, fexgen::returns_guest_pointer {};

// Symbols queryable through clGetExtensionFunctionAddress
namespace internal {
template<auto>
struct fex_gen_config : fexgen::generate_guest_symtable, fexgen::indirect_guest_calls {
};

template<> struct fex_gen_config<clGetPlatformIDs> {};
template<> struct fex_gen_config<clGetPlatformInfo> {};
template<> struct fex_gen_config<clGetDeviceIDs> {};
template<> struct fex_gen_config<clGetDeviceInfo> {};
template<> struct fex_gen_config<clCreateSubDevices> {};
template<> struct fex_gen_config<clRetainDevice> {};
template<> struct fex_gen_config<clReleaseDevice> {};
template<> struct fex_gen_config<clSetDefaultDeviceCommandQueue> {};
template<> struct fex_gen_config<clGetDeviceAndHostTimer> {};
template<> struct fex_gen_config<clGetHostTimer> {};
template<> struct fex_gen_config<clCreateContext> : fexgen::custom_host_impl, fexgen::callback_guest {};
template<> struct fex_gen_config<clCreateContextFromType> : fexgen::custom_host_impl, fexgen::callback_guest {};
template<> struct fex_gen_config<clRetainContext> : fexgen::custom_host_impl {};
template<> struct fex_gen_config<clReleaseContext> : fexgen::custom_host_impl {};
template<> struct fex_gen_config<clGetContextInfo> {};
template<> struct fex_gen_config<clSetContextDestructorCallback> : fexgen::custom_host_impl, fexgen::callback_guest {};
template<> struct fex_gen_config<clCreateCommandQueueWithProperties> : fexgen::custom_host_impl {};
template<> struct fex_gen_config<clRetainCommandQueue> : fexgen::custom_host_impl {};
template<> struct fex_gen_config<clReleaseCommandQueue> : fexgen::custom_host_impl {};
template<> struct fex_gen_config<clGetCommandQueueInfo> {};
template<> struct fex_gen_config<clCreateBuffer> {};
template<> struct fex_gen_config<clCreateSubBuffer> {};
template<> struct fex_gen_config<clCreateImage> {};
template<> struct fex_gen_config<clCreatePipe> {};
template<> struct fex_gen_config<clCreateBufferWithProperties> {};
template<> struct fex_gen_config<clCreateImageWithProperties> {};
template<> struct fex_gen_config<clRetainMemObject> : fexgen::custom_host_impl {};
template<> struct fex_gen_config<clReleaseMemObject> : fexgen::custom_host_impl {};
template<> struct fex_gen_config<clGetSupportedImageFormats> {};
template<> struct fex_gen_config<clGetMemObjectInfo> {};
template<> struct fex_gen_config<clGetImageInfo> {};
template<> struct fex_gen_config<clGetPipeInfo> {};
template<> struct fex_gen_config<clSetMemObjectDestructorCallback> : fexgen::custom_host_impl, fexgen::callback_guest {};
template<> struct fex_gen_config<clSVMAlloc> {};
template<> struct fex_gen_config<clSVMFree> {};
template<> struct fex_gen_config<clCreateSamplerWithProperties> {};
template<> struct fex_gen_config<clRetainSampler> {};
template<> struct fex_gen_config<clReleaseSampler> {};
template<> struct fex_gen_config<clGetSamplerInfo> {};
template<> struct fex_gen_config<clCreateProgramWithSource> {};
template<> struct fex_gen_config<clCreateProgramWithBinary> {};
template<> struct fex_gen_config<clCreateProgramWithBuiltInKernels> {};
template<> struct fex_gen_config<clCreateProgramWithIL> {};
template<> struct fex_gen_config<clRetainProgram> : fexgen::custom_host_impl {};
template<> struct fex_gen_config<clReleaseProgram> : fexgen::custom_host_impl {};
template<> struct fex_gen_config<clBuildProgram> : fexgen::custom_host_impl, fexgen::callback_guest {};
template<> struct fex_gen_config<clCompileProgram> : fexgen::custom_host_impl, fexgen::callback_guest {};
template<> struct fex_gen_config<clLinkProgram> : fexgen::custom_host_impl, fexgen::callback_guest {};
template<> struct fex_gen_config<clSetProgramReleaseCallback> : fexgen::custom_host_impl, fexgen::callback_guest {};
template<> struct fex_gen_config<clSetProgramSpecializationConstant> {};
template<> struct fex_gen_config<clUnloadPlatformCompiler> {};
template<> struct fex_gen_config<clGetProgramInfo> {};
template<> struct fex_gen_config<clGetProgramBuildInfo> {};
template<> struct fex_gen_config<clCreateKernel> {};
template<> struct fex_gen_config<clCreateKernelsInProgram> {};
template<> struct fex_gen_config<clCloneKernel> {};
template<> struct fex_gen_config<clRetainKernel> {};
template<> struct fex_gen_config<clReleaseKernel> {};
template<> struct fex_gen_config<clSetKernelArg> {};
template<> struct fex_gen_config<clSetKernelArgSVMPointer> {};
template<> struct fex_gen_config<clSetKernelExecInfo> {};
template<> struct fex_gen_config<clGetKernelInfo> {};
template<> struct fex_gen_config<clGetKernelArgInfo> {};
template<> struct fex_gen_config<clGetKernelWorkGroupInfo> {};
template<> struct fex_gen_config<clGetKernelSubGroupInfo> {};
template<> struct fex_gen_config<clWaitForEvents> {};
template<> struct fex_gen_config<clGetEventInfo> {};
template<> struct fex_gen_config<clCreateUserEvent> {};
template<> struct fex_gen_config<clRetainEvent> : fexgen::custom_host_impl {};
template<> struct fex_gen_config<clReleaseEvent> : fexgen::custom_host_impl {};
template<> struct fex_gen_config<clSetUserEventStatus> {};
template<> struct fex_gen_config<clSetEventCallback> : fexgen::custom_host_impl, fexgen::callback_guest {};
template<> struct fex_gen_config<clGetEventProfilingInfo> {};
template<> struct fex_gen_config<clFlush> {};
template<> struct fex_gen_config<clFinish> {};
template<> struct fex_gen_config<clEnqueueReadBuffer> {};
template<> struct fex_gen_config<clEnqueueReadBufferRect> {};
template<> struct fex_gen_config<clEnqueueWriteBuffer> {};
template<> struct fex_gen_config<clEnqueueWriteBufferRect> {};
template<> struct fex_gen_config<clEnqueueFillBuffer> {};
template<> struct fex_gen_config<clEnqueueCopyBuffer> {};
template<> struct fex_gen_config<clEnqueueCopyBufferRect> {};
template<> struct fex_gen_config<clEnqueueReadImage> {};
template<> struct fex_gen_config<clEnqueueWriteImage> {};
template<> struct fex_gen_config<clEnqueueFillImage> {};
template<> struct fex_gen_config<clEnqueueCopyImage> {};
template<> struct fex_gen_config<clEnqueueCopyImageToBuffer> {};
template<> struct fex_gen_config<clEnqueueCopyBufferToImage> {};
template<> struct fex_gen_config<clEnqueueMapBuffer> {};
template<> struct fex_gen_config<clEnqueueMapImage> {};
template<> struct fex_gen_config<clEnqueueUnmapMemObject> {};
template<> struct fex_gen_config<clEnqueueMigrateMemObjects> {};
template<> struct fex_gen_config<clEnqueueNDRangeKernel> {};
template<> struct fex_gen_config<clEnqueueNativeKernel> : fexgen::custom_host_impl, fexgen::callback_guest {};
template<> struct fex_gen_config<clEnqueueMarkerWithWaitList> {};
template<> struct fex_gen_config<clEnqueueBarrierWithWaitList> {};
template<> struct fex_gen_config<clEnqueueSVMFree> : fexgen::custom_host_impl, fexgen::callback_guest {};
template<> struct fex_gen_config<clEnqueueSVMMemcpy> {};
template<> struct fex_gen_config<clEnqueueSVMMemFill> {};
template<> struct fex_gen_config<clEnqueueSVMMap> {};
template<> struct fex_gen_config<clEnqueueSVMUnmap> {};
template<> struct fex_gen_config<clEnqueueSVMMigrateMem> {};
template<> struct fex_gen_config<clGetExtensionFunctionAddressForPlatform> {};
template<> struct fex_gen_config<clSetCommandQueueProperty> {};
template<> struct fex_gen_config<clCreateImage2D> {};
template<> struct fex_gen_config<clCreateImage3D> {};
template<> struct fex_gen_config<clEnqueueMarker> {};
template<> struct fex_gen_config<clEnqueueWaitForEvents> {};
template<> struct fex_gen_config<clEnqueueBarrier> {};
template<> struct fex_gen_config<clUnloadCompiler> {};
template<> struct fex_gen_config<clCreateCommandQueue> : fexgen::custom_host_impl {};
template<> struct fex_gen_config<clCreateSampler> {};
template<> struct fex_gen_config<clEnqueueTask> {};

// Special undocumented `clGetICDLoaderInfoOCLICD` function from the ICD. Only documented in ICD source.
cl_int clGetICDLoaderInfoOCLICD(uint32_t, size_t, void*, size_t*);

template<> struct fex_gen_config<clGetICDLoaderInfoOCLICD> {};

}
