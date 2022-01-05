/*
$info$
tags: thunklibs|Vulkan
$end_info$
*/

#define VK_USE_PLATFORM_XLIB_XRANDR_EXT
#define VK_USE_PLATFORM_XLIB_KHR
#define VK_USE_PLATFORM_XCB_KHR
#define VK_USE_PLATFORM_WAYLAND_KHR
#include <vulkan/vulkan.h>

#include "common/Guest.h"

#include <cstdio>
#include <dlfcn.h>
#include <string_view>
#include <unordered_map>

#include "thunks.inl"
#include "function_packs.inl"
#include "function_packs_public.inl"
#include "symbol_list.inl"

extern "C" {
static bool Setup{};
static std::unordered_map<std::string_view,PFN_vkVoidFunction*> PtrsToLookUp{};

// Setup can't be done on shared library constructor
// Needs to be deferred until post-constructor phase to remove the chance of crashing
static void DoSetup() {
    // Initialize unordered_map from generated initializer-list
    PtrsToLookUp = {
#define PAIR(name, unused) { #name, (PFN_vkVoidFunction*)name },
FOREACH_SYMBOL(PAIR)
#undef PAIR
    };

    Setup = true;
}

PFN_vkVoidFunction vkGetDeviceProcAddr(VkDevice a_0,const char* a_1){
  if (!Setup) {
    DoSetup();
  }

  auto ret = fexfn_pack_vkGetDeviceProcAddr(a_0, a_1);

  if (ret == nullptr) {
    // Early out if our instance doesn't have the pointer
    // Definitely means we don't support it
    return nullptr;
  }

  // Okay, we found a host side function for this
  // Now return our local instance of this function
  auto It = PtrsToLookUp.find(a_1);
  if (It == PtrsToLookUp.end() || !It->second) {
    fprintf(stderr, "\tvkGetDeviceProcAddr: Couldn't find Guest symbol: '%s'\n", a_1);
    __builtin_trap();
  }
  return (PFN_vkVoidFunction)It->second;
}

PFN_vkVoidFunction vkGetInstanceProcAddr(VkInstance a_0,const char* a_1){
  if (!Setup) {
    DoSetup();
  }

  // Search our host install first to see if the pointer exists
  // This also populates a map on the host facing side
  auto ret = fexfn_pack_vkGetInstanceProcAddr(a_0, a_1);
  if (ret == nullptr) {
    // Early out if our instance doesn't have the pointer
    // Definitely means we don't support it
    return nullptr;
  }

  auto It = PtrsToLookUp.find(a_1);
  if (It == PtrsToLookUp.end() || !It->second) {
    fprintf(stderr, "\tvkGetInstanceProcAddr: Couldn't find Guest symbol: '%s'\n", a_1);
    __builtin_trap();
  }
  return (PFN_vkVoidFunction)It->second;
}

}

#define DOLOAD(name) LOAD_LIB(name)
DOLOAD(LIBLIB_NAME)
