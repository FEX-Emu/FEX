/*
$info$
tags: thunklibs|X11
desc: Handles callbacks and varargs
$end_info$
*/

#include <cstdlib>
#include <stdio.h>

#include <dlfcn.h>
#include <unordered_map>
#include <shared_mutex>

#include "common/CrossArchEvent.h"
#include "common/Host.h"
#include <vector>

#include "libOpenCL_Common.h"

#include "thunkgen_host_libOpenCL.inl"

// OpenCL as an API has multiple functions that have a callback associated with it.
// The lifetime of the callback passed to these functions is associated with the object created, not the library.
//
// Each of the objects that OpenCL creates is refcounted by the driver. Our callback tracking needs to stay aligned with this refcounting.
// Because FEX thunks need to rewrite the callback function and arguments, we also need to to keep the application's data around with refcounting.
//
// We need to be quite careful with these refcounted callbacks since the guest application can be utterly mean to us where it is also tracking
// lifetimes and deleting things as they go away.
//
// Common use case is just a hardcoded handler function in their application that returns results and would still work beyond the object's lifetime.
//
// List of OpenCL functions with callbacks that have lifetimes.
//
// clCreateContext - pfn_notify - Is live until context is destroyed (with refcount)
//   - clCreateContextFromType - Same
//   - clSetContextDestructorCallback - Same
//
// clSetEventCallback - Live until cl_event is destroyed (with refcount)
//
// clSetMemObjectDestructorCallback - Is live until cl_mem is destroyed (with refcount)
//
// clBuildProgram - pfn_notify - Is live until program is built (Stays around until refcount is zero)
//   - clCompileProgram - Same
//   - clLinkProgram - Same
//   - clSetProgramReleaseCallback - Same
//
// clEnqueueNativeKernel - Links a user_func to a command_queue that it can execute.
//   - Live until command queue deleted (with refcount)
//   - clEnqueueSVMFree - Same

extern "C" {
  struct ContextCallbacks {
    std::atomic<size_t> RefCount{};
    // CreateContext and CreateContextFromType share this callback
    FEX::CL::Type_SingleUserData CreateContextCallback{};
    FEX::CL::Type_SingleUserData SetContextDestructorCallback{};
  };

  struct ProgramCallbacks {
    std::atomic<size_t> RefCount{};
    FEX::CL::Type_SingleUserData BuildCallback{};
    FEX::CL::Type_SingleUserData CompileCallback{};
    FEX::CL::Type_SingleUserData LinkCallback{};
    FEX::CL::Type_SingleUserData SetProgramReleaseCallback{};
  };

  struct CommandQueueCallbacks {
    std::atomic<size_t> RefCount{};
    std::vector<std::unique_ptr<FEX::CL::Type_SingleUserDataCopy>> UserFunc{};
    FEX::CL::Type_SingleUserData SVMFreeCallback{};
  };

  struct EventCallbacks {
    std::atomic<size_t> RefCount{};
    std::vector<FEX::CL::Type_SingleUserData> EventCallbacks{};
  };

  struct MemoryObjectCallbacks {
    std::atomic<size_t> RefCount{};
    FEX::CL::Type_SingleUserData MemObjectDestructorCallback{};
  };

  // Delete map members with clReleaseContext once refcount is zero
  std::shared_mutex ContextCBsMutex{};
  std::unordered_map<cl_context, ContextCallbacks> ContextCBs{};
  // Delete map members with clReleaseProgram once refcount is zero
  std::shared_mutex ProgramCBsMutex{};
  std::unordered_map<cl_program, ProgramCallbacks> ProgramCBs{};
  // Delete map members with clReleaseCommandQueue once refcount is zero
  std::shared_mutex CommandCBsMutex{};
  std::unordered_map<cl_command_queue, CommandQueueCallbacks> CommandQueueCBs{};
  // Delete map members with clReleaseMemObject once refcount is zero
  std::shared_mutex MemCBsMutex{};
  std::unordered_map<cl_mem, MemoryObjectCallbacks> MemCBs{};
  // Delete map members with clReleaseEvent once refcount is zero
  std::shared_mutex EventCBsMutex{};
  std::unordered_map<cl_event, EventCallbacks> EventCBs{};

  CrossArchEvent *WaitForWork{};
  CrossArchEvent *WorkDone{};
  void **Work{};

  static void fexfn_impl_libOpenCL_FEX_GiveEventHandlers(CrossArchEvent* a_0, CrossArchEvent* a_1, void** a_2){
    WaitForWork = a_0;
    WorkDone = a_1;
    Work = a_2;
  }

  static void clCreateContext_async_callback(const char * errinfo, const void * private_info, size_t cb, void *user_data) {
    auto FEX_user_data = reinterpret_cast<FEX::CL::Type_SingleUserData*>(user_data);
    auto CBData = FEX::CL::GuestCallbackData::Create<FEX::CL::GuestCallbackData::Type_clCreateContext>(FEX_user_data, errinfo, private_info, cb);
    *Work = &CBData;

    // Tell the thread it has work
    NotifyWorkFunc(WaitForWork);
    // Wait for the work to be done
    WaitForWorkFunc(WorkDone);
  }

  static void clProgram_async_callback(cl_program a_0, void *a_1) {
    auto FEX_user_data = reinterpret_cast<FEX::CL::Type_SingleUserData*>(a_1);
    auto CBData = FEX::CL::GuestCallbackData::Create<FEX::CL::GuestCallbackData::Type_clProgram>(FEX_user_data, a_0);
    *Work = &CBData;

    // Tell the thread it has work
    NotifyWorkFunc(WaitForWork);
    // Wait for the work to be done
    WaitForWorkFunc(WorkDone);
  }

  static void clSetMemObjectDestructorCallback_async_callback(cl_mem a_0, void *a_1) {
    auto FEX_user_data = reinterpret_cast<FEX::CL::Type_SingleUserData*>(a_1);
    auto CBData = FEX::CL::GuestCallbackData::Create<FEX::CL::GuestCallbackData::Type_clMem>(FEX_user_data, a_0);
    *Work = &CBData;

    // Tell the thread it has work
    NotifyWorkFunc(WaitForWork);
    // Wait for the work to be done
    WaitForWorkFunc(WorkDone);
  }

  static void clSetContextDestructorCallback_async_callback(cl_context a_0, void *a_1) {
    auto FEX_user_data = reinterpret_cast<FEX::CL::Type_SingleUserData*>(a_1);
    auto CBData = FEX::CL::GuestCallbackData::Create<FEX::CL::GuestCallbackData::Type_clSetContextDestructorCallback>(FEX_user_data, a_0);
    *Work = &CBData;

    // Tell the thread it has work
    NotifyWorkFunc(WaitForWork);
    // Wait for the work to be done
    WaitForWorkFunc(WorkDone);
  }

  static void clUser_async_callback(void *a_0) {
    auto FEX_user_data = reinterpret_cast<FEX::CL::Type_SingleUserDataCopy*>(a_0);
    // Unlike other callbacks, the a_0
    auto CBData = FEX::CL::GuestCallbackData::Create<FEX::CL::GuestCallbackData::Type_clUser>(FEX_user_data);
    *Work = &CBData;

    // Tell the thread it has work
    NotifyWorkFunc(WaitForWork);
    // Wait for the work to be done
    WaitForWorkFunc(WorkDone);
  }

  static void clSVMFree_async_callback(cl_command_queue a_0, cl_uint a_1, void *a_2[], void *a_3) {
    auto FEX_user_data = reinterpret_cast<FEX::CL::Type_SingleUserData*>(a_3);
    auto CBData = FEX::CL::GuestCallbackData::Create<FEX::CL::GuestCallbackData::Type_clSVMFree>(FEX_user_data, a_0, a_1, a_2);
    *Work = &CBData;

    // Tell the thread it has work
    NotifyWorkFunc(WaitForWork);
    // Wait for the work to be done
    WaitForWorkFunc(WorkDone);
  }

  static void clSetEventCallback_async_callback(cl_event a_0, cl_int a_1, void *a_2) {
    auto FEX_user_data = reinterpret_cast<FEX::CL::Type_SingleUserData*>(a_2);
    auto CBData = FEX::CL::GuestCallbackData::Create<FEX::CL::GuestCallbackData::Type_clSetEventCallback>(FEX_user_data, a_0, a_1);
    *Work = &CBData;

    // Tell the thread it has work
    NotifyWorkFunc(WaitForWork);
    // Wait for the work to be done
    WaitForWorkFunc(WorkDone);
  }
}

static auto fexfn_impl_libOpenCL_clCreateContext (const cl_context_properties * a_0, cl_uint a_1, const cl_device_id * a_2, fex_guest_function_ptr a_3, void * a_4, cl_int * a_5) -> cl_context {
  auto CBData = FEX::CL::Type_SingleUserData::Create<FEX::CL::AsyncCallbackType::TYPE_CLCREATECONTEXT>(a_3, a_4);

  using CallbackType = decltype(&clCreateContext_async_callback);
  CallbackType CB{};
  FEX::CL::Type_SingleUserData *CBDataPtr{};

  if (a_3) {
    CB = clCreateContext_async_callback;
    CBDataPtr = &CBData;
  }

  auto Result = fexldr_ptr_libOpenCL_clCreateContext(a_0, a_1, a_2, CB, CBDataPtr, a_5);

  if (Result && a_3) {
    std::unique_lock lk{ContextCBsMutex};

    auto ContextCB = ContextCBs.try_emplace(Result);

    // Refcount increase
    ContextCB.first->second.RefCount++;

    // Add the callback to the tracking map
    ContextCB.first->second.CreateContextCallback = CBData;
  }
  return Result;
}

static auto fexfn_impl_libOpenCL_clCreateContextFromType(const cl_context_properties * a_0, cl_device_type a_1, fex_guest_function_ptr a_2, void * a_3, cl_int * a_4) -> cl_context {
  auto CBData = FEX::CL::Type_SingleUserData::Create<FEX::CL::AsyncCallbackType::TYPE_CLCREATECONTEXTFROMTYPE>(a_2, a_3);

  using CallbackType = decltype(&clCreateContext_async_callback);
  CallbackType CB{};
  FEX::CL::Type_SingleUserData *CBDataPtr{};

  if (a_3) {
    CB = clCreateContext_async_callback;
    CBDataPtr = &CBData;
  }

  auto Result = fexldr_ptr_libOpenCL_clCreateContextFromType(a_0, a_1, CB, CBDataPtr, a_4);
  if (Result && a_2) {
    std::unique_lock lk{ContextCBsMutex};

    auto ContextCB = ContextCBs.try_emplace(Result);

    // Refcount increase
    ContextCB.first->second.RefCount++;

    // Add the callback to the tracking map
    ContextCB.first->second.CreateContextCallback = CBData;
  }

  return Result;
}

static auto fexfn_impl_libOpenCL_clSetContextDestructorCallback(cl_context a_0, fex_guest_function_ptr a_1, void * a_2) -> cl_int {
  using CallbackType = decltype(&clSetContextDestructorCallback_async_callback);
  CallbackType CB{};
  FEX::CL::Type_SingleUserData *CBDataPtr{};

  if (a_1) {
    CB = clSetContextDestructorCallback_async_callback;

    std::unique_lock lk{ContextCBsMutex};

    // Add the callback to the tracking map
    auto CBData = ContextCBs.try_emplace(a_0);

    CBData.first->second.SetContextDestructorCallback = FEX::CL::Type_SingleUserData::Create<FEX::CL::AsyncCallbackType::TYPE_CLSETCONTEXTDESTRUCTORCALLBACK>(a_1, a_2);
    CBDataPtr = &CBData.first->second.SetContextDestructorCallback;
  }

  return fexldr_ptr_libOpenCL_clSetContextDestructorCallback(a_0, CB, CBDataPtr);
}

static auto fexfn_impl_libOpenCL_clRetainContext(cl_context a_0) -> cl_int {
  {
    std::shared_lock lk{ContextCBsMutex};
    auto CB = ContextCBs.find(a_0);
    if (CB != ContextCBs.end()) {
      CB->second.RefCount++;
    }
  }
  return fexldr_ptr_libOpenCL_clRetainContext(a_0);
}

static auto fexfn_impl_libOpenCL_clReleaseContext(cl_context a_0) -> cl_int {
  auto Result = fexldr_ptr_libOpenCL_clReleaseContext(a_0);

  std::unique_lock lk{ContextCBsMutex};
  auto CB = ContextCBs.find(a_0);
  if (CB != ContextCBs.end()) {
    auto RefCount = CB->second.RefCount.fetch_sub(1);;
    if (RefCount == 1) {
      // Remove the tracking once this has been refcount to zero
      ContextCBs.erase(CB);
    }
  }

  return Result;
}

static auto fexfn_impl_libOpenCL_clBuildProgram(cl_program a_0, cl_uint a_1, const cl_device_id * a_2, const char * a_3, fex_guest_function_ptr a_4, void * a_5) -> cl_int {
  FEX::CL::Type_SingleUserData *CBDataPtr{};

  using CallbackType = decltype(&clProgram_async_callback);
  CallbackType CB{};

  if (a_4) {
    CB = clProgram_async_callback;

    std::unique_lock lk{ProgramCBsMutex};

    // Add the callback to the tracking map
    auto CBData = ProgramCBs.try_emplace(a_0);

    // Refcount increase
    CBData.first->second.RefCount++;

    CBData.first->second.BuildCallback = FEX::CL::Type_SingleUserData::Create<FEX::CL::AsyncCallbackType::TYPE_CLBUILDPROGRAM>(a_4, a_5);
    CBDataPtr = &CBData.first->second.BuildCallback;
  }
  return fexldr_ptr_libOpenCL_clBuildProgram(a_0, a_1, a_2, a_3, CB, CBDataPtr);
}

static auto fexfn_impl_libOpenCL_clCompileProgram(cl_program a_0, cl_uint a_1, const cl_device_id * a_2, const char * a_3, cl_uint a_4, const cl_program * a_5, const char ** a_6, fex_guest_function_ptr a_7, void * a_8) -> cl_int {
  FEX::CL::Type_SingleUserData *CBDataPtr{};

  using CallbackType = decltype(&clProgram_async_callback);
  CallbackType CB{};

  if (a_7) {
    CB = clProgram_async_callback;

    std::unique_lock lk{ProgramCBsMutex};

    // Add the callback to the tracking map
    auto CBData = ProgramCBs.try_emplace(a_0);

    // Refcount increase
    CBData.first->second.RefCount++;

    CBData.first->second.CompileCallback = FEX::CL::Type_SingleUserData::Create<FEX::CL::AsyncCallbackType::TYPE_CLCOMPILEPROGRAM>(a_7, a_8);
    CBDataPtr = &CBData.first->second.CompileCallback;
  }

  return fexldr_ptr_libOpenCL_clCompileProgram(a_0, a_1, a_2, a_3, a_4, a_5, a_6, CB, CBDataPtr);
}

static auto fexfn_impl_libOpenCL_clLinkProgram(cl_context a_0, cl_uint a_1, const cl_device_id * a_2, const char * a_3, cl_uint a_4, const cl_program * a_5, fex_guest_function_ptr a_6, void * a_7, cl_int * a_8) -> cl_program {
  FEX::CL::Type_SingleUserData *CBDataPtr{};

  using CallbackType = decltype(&clProgram_async_callback);
  CallbackType CB{};

  if (a_7) {
    CB = clProgram_async_callback;

    std::unique_lock lk{ProgramCBsMutex};

    for (size_t i = 0; i < a_4; ++i) {
      const cl_program program = a_5[i];

      // Add the callback to the tracking map
      auto CBData = ProgramCBs.try_emplace(program);

      // Refcount increase
      CBData.first->second.RefCount++;

      CBData.first->second.LinkCallback = FEX::CL::Type_SingleUserData::Create<FEX::CL::AsyncCallbackType::TYPE_CLLINKPROGRAM>(a_6, a_7);
      CBDataPtr = &CBData.first->second.CompileCallback;
    }
  }

  return fexldr_ptr_libOpenCL_clLinkProgram(a_0, a_1, a_2, a_3, a_4, a_5, CB, CBDataPtr, a_8);
}

static auto fexfn_impl_libOpenCL_clSetProgramReleaseCallback(cl_program a_0, fex_guest_function_ptr a_1, void * a_2) -> cl_int {
  FEX::CL::Type_SingleUserData *CBDataPtr{};

  using CallbackType = decltype(&clProgram_async_callback);
  CallbackType CB{};

  if (a_1) {
    CB = clProgram_async_callback;

    std::unique_lock lk{ProgramCBsMutex};

    // Add the callback to the tracking map
    auto CBData = ProgramCBs.try_emplace(a_0);

    CBData.first->second.CompileCallback = FEX::CL::Type_SingleUserData::Create<FEX::CL::AsyncCallbackType::TYPE_CLSETPROGRAMRELEASECALLBACK>(a_1, a_2);
    CBDataPtr = &CBData.first->second.CompileCallback;
  }

  return fexldr_ptr_libOpenCL_clSetProgramReleaseCallback(a_0, CB, CBDataPtr);
}

static auto fexfn_impl_libOpenCL_clRetainProgram(cl_program a_0) -> cl_int {
  {
    std::shared_lock lk{ContextCBsMutex};
    auto CB = ProgramCBs.find(a_0);
    if (CB != ProgramCBs.end()) {
      CB->second.RefCount++;
    }
  }
  return fexldr_ptr_libOpenCL_clRetainProgram(a_0);
}
static auto fexfn_impl_libOpenCL_clReleaseProgram(cl_program a_0) -> cl_int {
  auto Result = fexldr_ptr_libOpenCL_clReleaseProgram(a_0);

  std::unique_lock lk{ContextCBsMutex};
  auto CB = ProgramCBs.find(a_0);
  if (CB != ProgramCBs.end()) {
    auto RefCount = CB->second.RefCount.fetch_sub(1);;
    if (RefCount == 1) {
      // Remove the tracking once this has been refcount to zero
      ProgramCBs.erase(CB);
    }
  }

  return Result;
}

static auto fexfn_impl_libOpenCL_clEnqueueNativeKernel(cl_command_queue a_0, fex_guest_function_ptr a_1, void * a_2, size_t a_3, cl_uint a_4, const cl_mem * a_5, const void ** a_6, cl_uint a_7, const cl_event * a_8, cl_event * a_9) -> cl_int {
  using CallbackType = decltype(&clUser_async_callback);
  CallbackType CB{};
  void *MemoryPtr {a_2};
  size_t Size = a_3;

  if (a_1) {
    CB = clUser_async_callback;

    std::unique_lock lk{CommandCBsMutex};

    // Add the callback to the tracking map
    auto CBData = CommandQueueCBs.try_emplace(a_0);
    auto UserPtr = CBData.first->second.UserFunc.emplace_back(FEX::CL::Type_SingleUserDataCopy::Create<FEX::CL::AsyncCallbackType::TYPE_CLENQUEUENATIVEKERNEL>(a_1, a_2, a_3)).get();

    // Change memory pointer over to our tracking data
    MemoryPtr = UserPtr;
    Size = sizeof(*UserPtr);
  }

  return fexldr_ptr_libOpenCL_clEnqueueNativeKernel(a_0, CB, MemoryPtr, Size, a_4, a_5, a_6, a_7, a_8, a_9);
}

static auto fexfn_impl_libOpenCL_clCreateCommandQueue(cl_context a_0, cl_device_id a_1, cl_command_queue_properties a_2, cl_int * a_3) -> cl_command_queue {
  auto Result = fexldr_ptr_libOpenCL_clCreateCommandQueue(a_0, a_1, a_2, a_3);

  if (Result && a_3) {
    std::unique_lock lk{CommandCBsMutex};

    auto ContextCB = CommandQueueCBs.try_emplace(Result);

    // Refcount increase
    ContextCB.first->second.RefCount++;
  }
  return Result;
}

static auto fexfn_impl_libOpenCL_clCreateCommandQueueWithProperties(cl_context a_0, cl_device_id a_1, const cl_queue_properties * a_2, cl_int * a_3) -> cl_command_queue {
  auto Result = fexldr_ptr_libOpenCL_clCreateCommandQueueWithProperties(a_0, a_1, a_2, a_3);

  if (Result && a_3) {
    std::unique_lock lk{CommandCBsMutex};

    auto ContextCB = CommandQueueCBs.try_emplace(Result);

    // Refcount increase
    ContextCB.first->second.RefCount++;
  }
  return Result;
}

static auto fexfn_impl_libOpenCL_clEnqueueSVMFree(cl_command_queue a_0, cl_uint a_1, void ** a_2, fex_guest_function_ptr a_3, void * a_4, cl_uint a_5, const cl_event * a_6, cl_event * a_7) -> cl_int {
  FEX::CL::Type_SingleUserData *CBDataPtr{};

  using CallbackType = decltype(&clSVMFree_async_callback);
  CallbackType CB{};

  if (a_7) {
    CB = clSVMFree_async_callback;

    std::unique_lock lk{ContextCBsMutex};

    // Add the callback to the tracking map
    auto CBData = CommandQueueCBs.try_emplace(a_0);

    CBData.first->second.SVMFreeCallback = FEX::CL::Type_SingleUserData::Create<FEX::CL::AsyncCallbackType::TYPE_CLENQUEUESVMFREE>(a_3, a_4);
    CBDataPtr = &CBData.first->second.SVMFreeCallback;
  }

  return fexldr_ptr_libOpenCL_clEnqueueSVMFree(a_0, a_1, a_2, CB, CBDataPtr, a_5, a_6, a_7);
}

static auto fexfn_impl_libOpenCL_clRetainCommandQueue(cl_command_queue a_0) -> cl_int {
  {
    std::shared_lock lk{CommandCBsMutex};
    auto CB = CommandQueueCBs.find(a_0);
    if (CB != CommandQueueCBs.end()) {
      CB->second.RefCount++;
    }
  }
  return fexldr_ptr_libOpenCL_clRetainCommandQueue(a_0);
}

static auto fexfn_impl_libOpenCL_clReleaseCommandQueue(cl_command_queue a_0) -> cl_int {
  auto Result = fexldr_ptr_libOpenCL_clReleaseCommandQueue(a_0);

  std::unique_lock lk{CommandCBsMutex};
  auto CB = CommandQueueCBs.find(a_0);
  if (CB != CommandQueueCBs.end()) {
    auto RefCount = CB->second.RefCount.fetch_sub(1);;
    if (RefCount == 1) {
      // Remove the tracking once this has been refcount to zero
      CommandQueueCBs.erase(CB);
    }
  }

  return Result;
}

static auto fexfn_impl_libOpenCL_clSetMemObjectDestructorCallback(cl_mem a_0, fex_guest_function_ptr a_1, void * a_2) -> cl_int {
  using CallbackType = decltype(&clSetMemObjectDestructorCallback_async_callback);
  CallbackType CB{};
  FEX::CL::Type_SingleUserData *CBDataPtr{};

  if (a_1) {
    CB = clSetMemObjectDestructorCallback_async_callback;

    std::unique_lock lk{ContextCBsMutex};

    // Add the callback to the tracking map
    auto CBData = MemCBs.try_emplace(a_0);

    CBData.first->second.MemObjectDestructorCallback = FEX::CL::Type_SingleUserData::Create<FEX::CL::AsyncCallbackType::TYPE_CLSETMEMOBJECTDESTRUCTORCALLBACK>(a_1, a_2);
    CBDataPtr = &CBData.first->second.MemObjectDestructorCallback;
  }

  return fexldr_ptr_libOpenCL_clSetMemObjectDestructorCallback(a_0, CB, CBDataPtr);
}

static auto fexfn_impl_libOpenCL_clSetEventCallback(cl_event a_0, cl_int a_1, fex_guest_function_ptr a_2, void * a_3) -> cl_int {
  using CallbackType = decltype(&clSetEventCallback_async_callback);
  CallbackType CB{};
  FEX::CL::Type_SingleUserData *CBDataPtr{};

  if (a_1) {
    CB = clSetEventCallback_async_callback;

    std::unique_lock lk{EventCBsMutex};

    // Add the callback to the tracking map
    auto CBData = EventCBs.try_emplace(a_0);

    auto Ptr = CBData.first->second.EventCallbacks.emplace_back(FEX::CL::Type_SingleUserData::Create<FEX::CL::AsyncCallbackType::TYPE_CLSETEVENTCALLBACK>(a_2, a_3));
    CBDataPtr = &Ptr;
  }

  return fexldr_ptr_libOpenCL_clSetEventCallback(a_0, a_1, CB, CBDataPtr);

}

static auto fexfn_impl_libOpenCL_clRetainMemObject(cl_mem a_0) -> cl_int {
  {
    std::shared_lock lk{ContextCBsMutex};
    auto CB = MemCBs.find(a_0);
    if (CB != MemCBs.end()) {
      CB->second.RefCount++;
    }
  }
  return fexldr_ptr_libOpenCL_clRetainMemObject(a_0);
}

static auto fexfn_impl_libOpenCL_clReleaseMemObject(cl_mem a_0) -> cl_int {
  auto Result = fexldr_ptr_libOpenCL_clReleaseMemObject(a_0);

  std::unique_lock lk{ContextCBsMutex};
  auto CB = MemCBs.find(a_0);
  if (CB != MemCBs.end()) {
    auto RefCount = CB->second.RefCount.fetch_sub(1);;
    if (RefCount == 1) {
      // Remove the tracking once this has been refcount to zero
      MemCBs.erase(CB);
    }
  }

  return Result;
}

static auto fexfn_impl_libOpenCL_clRetainEvent(cl_event a_0) -> cl_int {
  {
    std::shared_lock lk{EventCBsMutex};
    auto CB = EventCBs.find(a_0);
    if (CB != EventCBs.end()) {
      CB->second.RefCount++;
    }
  }
  return fexldr_ptr_libOpenCL_clRetainEvent(a_0);
}
static auto fexfn_impl_libOpenCL_clReleaseEvent(cl_event a_0) -> cl_int {
  auto Result = fexldr_ptr_libOpenCL_clReleaseEvent(a_0);

  std::unique_lock lk{EventCBsMutex};
  auto CB = EventCBs.find(a_0);
  if (CB != EventCBs.end()) {
    auto RefCount = CB->second.RefCount.fetch_sub(1);;
    if (RefCount == 1) {
      // Remove the tracking once this has been refcount to zero
      EventCBs.erase(CB);
    }
  }

  return Result;
}

EXPORTS(libOpenCL)
