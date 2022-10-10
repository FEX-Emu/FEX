/*
$info$
tags: thunklibs|OpenCL
desc: Handles callbacks and varargs
$end_info$
*/

#include <cstdint>
#include <stdio.h>
#include <cstring>
#include <map>
#include <string>

#include <dlfcn.h>

#include <thread>
#include <functional>
#include <unordered_map>

#include "libOpenCL_Common.h"

#include "common/Guest.h"
#include "common/CrossArchEvent.h"
#include <stdarg.h>

#include "thunkgen_guest_libOpenCL.inl"

static std::thread CBThread{};
static std::atomic<bool> CBDone{false};

static CrossArchEvent WaitForWork{};
static CrossArchEvent WorkDone{};
static void *CBWorkData{};

void FEX_Helper_GiveEvents() {
  struct {
    CrossArchEvent *WaitForWork;
    CrossArchEvent *WorkDone;
    void **Work;
  } args;

  args.WaitForWork = &WaitForWork;
  args.WorkDone = &WorkDone;
  args.Work = &CBWorkData;

  fexthunks_libOpenCL_FEX_GiveEventHandlers(&args);
}

static void CallbackThreadFunc() {
  // Set the thread name to make it easy to know what thread this is
  pthread_setname_np(pthread_self(), "cl:async_cb");

  // Hand the host our helpers
  FEX_Helper_GiveEvents();
  while (!CBDone) {
    WaitForWorkFunc(&WaitForWork);
    if (CBDone) {
      return;
    }

    auto Data = reinterpret_cast<FEX::CL::GuestCallbackData*>(CBWorkData);
    switch (Data->Header.Type) {
      case FEX::CL::AsyncCallbackType::TYPE_CLCREATECONTEXT:
      case FEX::CL::AsyncCallbackType::TYPE_CLCREATECONTEXTFROMTYPE:
      {
        using clCreateContextCBType = void (*) (const char *errinfo, const void *private_info, size_t cb, void *user_data);
        auto CB = reinterpret_cast<clCreateContextCBType>(Data->Header.cb);

        CB(Data->clCreateContext.Args.a_0,
           Data->clCreateContext.Args.a_1,
           Data->clCreateContext.Args.a_2,
           Data->clCreateContext.Args.a_3
           );

        break;
      }
      case FEX::CL::AsyncCallbackType::TYPE_CLSETCONTEXTDESTRUCTORCALLBACK: {
        using clCBType = void (*) (cl_context context, void* user_data);
        auto CB = reinterpret_cast<clCBType>(Data->Header.cb);

        CB(Data->clSetContextDestructorCallback.Args.a_0,
           Data->clSetContextDestructorCallback.Args.a_1
           );
        break;
      }
      case FEX::CL::AsyncCallbackType::TYPE_CLSETMEMOBJECTDESTRUCTORCALLBACK: {
        using clCBType = void (*) (cl_mem mem, void* user_data);
        auto CB = reinterpret_cast<clCBType>(Data->Header.cb);

        CB(Data->clMem.Args.a_0,
           Data->clMem.Args.a_1
           );
        break;
      }
      case FEX::CL::AsyncCallbackType::TYPE_CLBUILDPROGRAM:
      case FEX::CL::AsyncCallbackType::TYPE_CLCOMPILEPROGRAM:
      case FEX::CL::AsyncCallbackType::TYPE_CLLINKPROGRAM:
      case FEX::CL::AsyncCallbackType::TYPE_CLSETPROGRAMRELEASECALLBACK:
      {
        using clProgramCBType = void (*) (cl_program program, void * user_data);
        auto CB = reinterpret_cast<clProgramCBType>(Data->Header.cb);

        CB(Data->clProgram.Args.a_0, Data->clProgram.Args.a_1);
        break;
      }
      case FEX::CL::AsyncCallbackType::TYPE_CLENQUEUENATIVEKERNEL:
      {
        using clCBType = void (*) (void* user_data);
        auto CB = reinterpret_cast<clCBType>(Data->Header.cb);
        CB(Data->clUser.Args.a_0);
        break;
      }
      case FEX::CL::AsyncCallbackType::TYPE_CLENQUEUESVMFREE:
      {
        using clCBType = void (*) (cl_command_queue queue, cl_uint num_svm_pointers, void *svm_pointers[], void* user_data);
        auto CB = reinterpret_cast<clCBType>(Data->Header.cb);

        CB(Data->clSVMFree.Args.a_0,
           Data->clSVMFree.Args.a_1,
           Data->clSVMFree.Args.a_2,
           Data->clSVMFree.Args.a_3
           );
        break;
      }
      case FEX::CL::AsyncCallbackType::TYPE_CLSETEVENTCALLBACK:
      {
        using clCBType = void (*) (cl_event event, cl_int event_command_exec_status, void *user_data);
        auto CB = reinterpret_cast<clCBType>(Data->Header.cb);

        CB(Data->clSetEventCallback.Args.a_0,
           Data->clSetEventCallback.Args.a_1,
           Data->clSetEventCallback.Args.a_2
           );
        break;
      }
    }
    NotifyWorkFunc(&WorkDone);
  }
}

extern "C" {
  static void init_lib() {
    // Start a guest side thread that allows us to do callbacks from xcb safely
    if (!CBThread.joinable()) {
      CBThread = std::thread(CallbackThreadFunc);
    }
  }
  __attribute__((destructor)) static void close_lib() {
    if (CBThread.joinable()) {
      CBDone = true;
      NotifyWorkFunc(&WaitForWork);
      NotifyWorkFunc(&WorkDone);
      CBThread.join();
    }
  }
}

typedef void voidFunc();

// Maps OpenGL API function names to the address of a guest function which is
// linked to the corresponding host function pointer
const std::unordered_map<std::string_view, uintptr_t /* guest function address */> HostPtrInvokers =
    std::invoke([]() {
#define PAIR(name, unused) Ret[#name] = reinterpret_cast<uintptr_t>(GetCallerForHostFunction(name));
        std::unordered_map<std::string_view, uintptr_t> Ret;
        FOREACH_internal_SYMBOL(PAIR);
        return Ret;
#undef PAIR
    });

extern "C" {
  CL_API_ENTRY CL_API_PREFIX__VERSION_1_1_DEPRECATED void * CL_API_CALL
    clGetExtensionFunctionAddress(const char *procname) CL_API_SUFFIX__VERSION_1_1_DEPRECATED {
    auto Ret = fexfn_pack_clGetExtensionFunctionAddress(procname);
    if (!Ret) {
      return nullptr;
    }

    auto TargetFuncIt = HostPtrInvokers.find(reinterpret_cast<const char*>(procname));
    if (TargetFuncIt == HostPtrInvokers.end()) {
      std::string_view procname_s { reinterpret_cast<const char*>(procname) };
      fprintf(stderr, "clGetExtensionFunctionAddress: not found %s\n", procname);
      return nullptr;
    }

    LinkAddressToFunction((uintptr_t)Ret, TargetFuncIt->second);
    return Ret;
  }
}

LOAD_LIB_INIT(libOpenCL, init_lib)
