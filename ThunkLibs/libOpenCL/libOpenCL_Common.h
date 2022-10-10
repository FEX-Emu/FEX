#pragma once

#include <cstring>
#include <memory>
#include <new>

#define CL_VERSION_3_0
#define CL_USE_DEPRECATED_OPENCL_1_0_APIS
#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#define CL_USE_DEPRECATED_OPENCL_2_0_APIS
#define CL_USE_DEPRECATED_OPENCL_2_1_APIS
#define CL_USE_DEPRECATED_OPENCL_2_2_APIS
#define CL_TARGET_OPENCL_VERSION 300
#include <CL/cl.h>

namespace FEX::CL {
  enum class AsyncCallbackType {
    // Tied to context
    TYPE_CLCREATECONTEXT,
    TYPE_CLCREATECONTEXTFROMTYPE,
    TYPE_CLSETCONTEXTDESTRUCTORCALLBACK,
    TYPE_CLSETMEMOBJECTDESTRUCTORCALLBACK,
    TYPE_CLSETPROGRAMRELEASECALLBACK,
    TYPE_CLSETEVENTCALLBACK,
    // Tied to cl_progam
    TYPE_CLBUILDPROGRAM,
    TYPE_CLCOMPILEPROGRAM,
    TYPE_CLLINKPROGRAM,
    // Tied to command_queue
    TYPE_CLENQUEUENATIVEKERNEL,
    TYPE_CLENQUEUESVMFREE,
  };

  union CallbackData {
    struct _Header {
      AsyncCallbackType Type;
#ifdef GUEST_THUNK_LIBRARY
      uintptr_t cb;
#else
      fex_guest_function_ptr cb;
#endif
    };
  };

  struct Type_SingleUserData {
    CallbackData::_Header Header;

    // User provided data that we need to keep around
    void *user_data;

    template <AsyncCallbackType Type>
    static Type_SingleUserData Create(decltype(CallbackData::_Header::cb) a_0, void * a_1) {
      static_assert(Type == AsyncCallbackType::TYPE_CLCREATECONTEXT ||
        Type == AsyncCallbackType::TYPE_CLCREATECONTEXTFROMTYPE ||
        Type == AsyncCallbackType::TYPE_CLSETCONTEXTDESTRUCTORCALLBACK ||
        Type == AsyncCallbackType::TYPE_CLSETMEMOBJECTDESTRUCTORCALLBACK ||
        Type == AsyncCallbackType::TYPE_CLSETPROGRAMRELEASECALLBACK ||
        Type == AsyncCallbackType::TYPE_CLSETEVENTCALLBACK ||
        Type == AsyncCallbackType::TYPE_CLBUILDPROGRAM ||
        Type == AsyncCallbackType::TYPE_CLCOMPILEPROGRAM ||
        Type == AsyncCallbackType::TYPE_CLLINKPROGRAM ||
        Type == AsyncCallbackType::TYPE_CLENQUEUESVMFREE
        ,
        "Mismatch type for handling clProgram");

      FEX::CL::Type_SingleUserData Data {
        .Header {
          .Type = Type,
          .cb = a_0,
        },
        .user_data = a_1,
      };

      return Data;
    };
  };

  struct Type_SingleUserDataCopy {
    CallbackData::_Header Header;

    // User provided data that we need to keep around
    void *user_data;
    size_t size;
    ~Type_SingleUserDataCopy() {
      operator delete(user_data);
    }

    template <AsyncCallbackType Type>
    static std::unique_ptr<Type_SingleUserDataCopy> Create(decltype(CallbackData::_Header::cb) a_0, void * a_1, size_t size) {
      static_assert(Type == AsyncCallbackType::TYPE_CLENQUEUENATIVEKERNEL,
        "Mismatch type for handling clProgram");

      void *Memory = ::operator new (size);
      memcpy(Memory, a_1, size);
      return std::make_unique<FEX::CL::Type_SingleUserDataCopy>(FEX::CL::Type_SingleUserDataCopy{
        .Header {
          .Type = Type,
          .cb = a_0,
        },
        .user_data = Memory,
        .size = size,
      });
    };
  };

  union GuestCallbackData {
    CallbackData::_Header Header;

    template<typename ...>
    struct PackedArgs;

    template<typename A0>
    struct PackedArgs<A0> {
      A0 a_0;
    };

    template<typename A0, typename A1>
    struct PackedArgs<A0, A1> {
      A0 a_0;
      A1 a_1;
    };

    template<typename A0, typename A1, typename A2>
    struct PackedArgs<A0, A1, A2> {
      A0 a_0;
      A1 a_1;
      A2 a_2;
    };

    template<typename A0, typename A1, typename A2, typename A3>
    struct PackedArgs<A0, A1, A2, A3> {
      A0 a_0;
      A1 a_1;
      A2 a_2;
      A3 a_3;
    };

    struct Type_clCreateContext {
      CallbackData::_Header Header;
      PackedArgs<const char*, const void*, size_t, void*> Args;
    } clCreateContext;

    struct Type_clSetContextDestructorCallback {
      CallbackData::_Header Header;
      PackedArgs<cl_context, void*> Args;
    } clSetContextDestructorCallback;

    struct Type_clProgram {
      // Callback for this job that is necessary
      // This gets passed from Host->Guest for the callback arguments
      CallbackData::_Header Header;
      PackedArgs<cl_program, void*> Args;
    } clProgram;

    struct Type_clMem {
      // Callback for this job that is necessary
      // This gets passed from Host->Guest for the callback arguments
      CallbackData::_Header Header;
      PackedArgs<cl_mem, void*> Args;
    } clMem;

    struct Type_clUser {
      // Callback for this job that is necessary
      // This gets passed from Host->Guest for the callback arguments
      CallbackData::_Header Header;
      PackedArgs<void*> Args;
    } clUser;

    struct Type_clSetEventCallback {
      CallbackData::_Header Header;
      PackedArgs<cl_event, cl_int, void*> Args;
    } clSetEventCallback;

    struct Type_clSVMFree {
      CallbackData::_Header Header;
      PackedArgs<cl_command_queue, cl_uint, void**, void*> Args;
    } clSVMFree;

    template<typename DataType, typename OriginalDataType, typename... T>
    static DataType Create(OriginalDataType* OriginalData, T... args) {
      DataType Data;
      Data.Header.Type = OriginalData->Header.Type;
      Data.Header.cb = OriginalData->Header.cb;
      Data.Args = { args..., OriginalData->user_data };
      return Data;
    }
  };
}
