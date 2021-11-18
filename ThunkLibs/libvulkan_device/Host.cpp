/*
$info$
tags: thunklibs|vulkan
$end_info$
*/

#include "Header.inl"

#include <mutex>
#include <stdio.h>
#include <string>
#include <unordered_map>
#include <vector>

#include "common/Host.h"
#include <dlfcn.h>

#include "ldr_ptrs.inl"
#include "function_unpacks.inl"

static bool SetupDev{};
static bool SetupInstance{};
std::mutex SetupMutex{};

static std::unordered_map<std::string,PFN_vkVoidFunction*> PtrsToLookUp{};

const std::vector<std::pair<const char*, PFN_vkVoidFunction*>> Map = {{
  // Our local function
#define PAIR(name, ptr) { #name,  (PFN_vkVoidFunction*) ptr }
#include "ldr_ptrs_pair.inl"
#undef PAIR
}};

static void DoSetupWithDevice(VkDevice dev) {
  std::unique_lock lk {SetupMutex};
    for (auto &It : Map) {
      auto Lookup = PtrsToLookUp.find(It.first);
      if (Lookup != PtrsToLookUp.end() && *Lookup->second == nullptr)
      {
        auto Res = LDR_PTR(vkGetDeviceProcAddr)(dev,It.first);
        if (Res) {
          *PtrsToLookUp[It.first] = Res;
        }
      }
    }
    SetupDev = true;
}

static void DoSetupWithInstance(VkInstance instance) {
  std::unique_lock lk {SetupMutex};

    for (auto &It : Map) {
      auto Lookup = PtrsToLookUp.find(It.first);
      //if (Lookup != PtrsToLookUp.end() && *Lookup->second == nullptr)
      {
        PFN_vkVoidFunction Res = LDR_PTR(vkGetInstanceProcAddr)(instance, It.first);
        if (Res) {
          *Lookup->second = Res;
        }
      }
    }

    // Only do this lookup once.
    // NOTE: If vkGetInstanceProcAddr was called with a null instance, only a few function pointers will be filled with non-null values, so we do repeat the lookup in that case
    if (instance) {
        SetupInstance = true;
    }
}

static void UNPACKFUNC(vkGetDeviceProcAddr)(void *argsv){
  struct arg_t {VkDevice a_0;const char* a_1;PFN_vkVoidFunction rv;};
  auto args = (arg_t*)argsv;

  if (!SetupDev) {
    DoSetupWithDevice(args->a_0);
  }

  // Just return the host facing function pointer
  // The guest will handle mapping if this exists
  args->rv =
    LDR_PTR(vkGetDeviceProcAddr)
    (args->a_0,args->a_1);
}

static void UNPACKFUNC(vkGetInstanceProcAddr)(void *argsv){
  struct arg_t {VkInstance a_0;const char* a_1;PFN_vkVoidFunction rv;};
  auto args = (arg_t*)argsv;

  if (!SetupInstance) {
    DoSetupWithInstance(args->a_0);
  }

  // Just return the host facing function pointer
  // The guest will handle mapping if it exists
  args->rv =
    LDR_PTR(vkGetInstanceProcAddr)
    (args->a_0,args->a_1);
}

static void UNPACKFUNC(vkCreateShaderModule)(void *argsv){
  struct arg_t {VkDevice a_0;const VkShaderModuleCreateInfo* a_1;const VkAllocationCallbacks* a_2;VkShaderModule* a_3;VkResult rv;};
  auto args = (arg_t*)argsv;
  args->rv =
    LDR_PTR(vkCreateShaderModule)
    (args->a_0,args->a_1, nullptr,args->a_3);

}

static void UNPACKFUNC(vkCmdSetBlendConstants)(void *argsv){
  struct arg_t {VkCommandBuffer a_0;const float a_1[4];};
  auto args = (arg_t*)argsv;
  LDR_PTR(vkCmdSetBlendConstants)
    (args->a_0,args->a_1);
}

static void UNPACKFUNC(vkCreateInstance)(void *argsv){
  struct arg_t {const VkInstanceCreateInfo* a_0;const VkAllocationCallbacks* a_1;VkInstance* a_2;VkResult rv;};
  auto args = (arg_t*)argsv;
  args->rv =
    LDR_PTR(vkCreateInstance)
    (args->a_0, nullptr,args->a_2);
}

static void UNPACKFUNC(vkCreateDevice)(void *argsv){
  struct arg_t {VkPhysicalDevice a_0;const VkDeviceCreateInfo* a_1;const VkAllocationCallbacks* a_2;VkDevice* a_3;VkResult rv;};
  auto args = (arg_t*)argsv;
  args->rv =
    LDR_PTR(vkCreateDevice)
    (args->a_0,args->a_1, nullptr,args->a_3);
}

static void UNPACKFUNC(vkAllocateMemory)(void *argsv){
  struct arg_t {VkDevice a_0;const VkMemoryAllocateInfo* a_1;const VkAllocationCallbacks* a_2;VkDeviceMemory* a_3;VkResult rv;};
  auto args = (arg_t*)argsv;
  args->rv =
    LDR_PTR(vkAllocateMemory)
    (args->a_0,args->a_1,nullptr,args->a_3);
}
static void UNPACKFUNC(vkFreeMemory)(void *argsv){
  struct arg_t {VkDevice a_0;VkDeviceMemory a_1;const VkAllocationCallbacks* a_2;};
  auto args = (arg_t*)argsv;
  LDR_PTR(vkFreeMemory)
    (args->a_0,args->a_1,nullptr);
}

static void UNPACKFUNC(vkGetPhysicalDeviceSurfaceSupportKHR)(void *argsv){
  struct arg_t {VkPhysicalDevice a_0;uint32_t a_1;VkSurfaceKHR a_2;VkBool32* a_3;VkResult rv;};
  auto args = (arg_t*)argsv;
  args->rv = LDR_PTR(vkGetPhysicalDeviceSurfaceSupportKHR)
    (args->a_0,args->a_1,args->a_2,args->a_3);
}

static ExportEntry exports[] = {
    #include "tab_function_unpacks.inl"
    { nullptr, nullptr }
};

#include "ldr.inl"

static void DoSetup() {
  for (auto &It : Map) {
    PtrsToLookUp[It.first] = It.second;
  }
}

static void init_func() {
  // Initialize some initial pointers that we can get while loading
  (void*&)LDR_PTR(vkGetInstanceProcAddr) = dlsym(LDR_HANDLE, "vkGetInstanceProcAddr");
  if (LDR_PTR(vkGetInstanceProcAddr) == nullptr) {
    (void*&)LDR_PTR(vkGetInstanceProcAddr) = dlsym(LDR_HANDLE, "vk_icdGetInstanceProcAddr");
  }

  (void*&)LDR_PTR(vkEnumerateInstanceVersion) = (void *) LDR_PTR(vkGetInstanceProcAddr)(nullptr, "vkEnumerateInstanceVersion");
  (void*&)LDR_PTR(vkEnumerateInstanceExtensionProperties) = (void *) LDR_PTR(vkGetInstanceProcAddr)(nullptr, "vkEnumerateInstanceExtensionProperties");
  (void*&)LDR_PTR(vkEnumerateInstanceLayerProperties) = (void*) LDR_PTR(vkGetInstanceProcAddr)(nullptr, "vkEnumerateInstanceLayerProperties");
  (void*&)LDR_PTR(vkCreateInstance) = (void*) LDR_PTR(vkGetInstanceProcAddr)(nullptr, "vkCreateInstance");

  DoSetup();
}

#define DOEXPORT_INIT(name, init) EXPORTS_INIT(name, init)
DOEXPORT_INIT(LIBLIB_NAME, init_func)
