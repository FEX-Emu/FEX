/*
$info$
tags: thunklibs|vulkan
$end_info$
*/

#include "Header.inl"

#include <stdio.h>
#include <cstring>
#include <map>
#include <string>
#include <dlfcn.h>
#include <unordered_map>
#include <vector>

#include "common/Guest.h"
#include <stdarg.h>

#include "thunks.inl"
#include "function_packs.inl"
#include "function_packs_public.inl"

extern "C" {
static bool Setup{};
static std::unordered_map<std::string_view,PFN_vkVoidFunction*> PtrsToLookUp{};

static PFN_vkVoidFunction fexfn_pack_vkGetDeviceProcAddr(VkDevice a_0,const char* a_1);
static PFN_vkVoidFunction fexfn_pack_vkGetInstanceProcAddr(VkInstance a_0,const char* a_1);
static void fexfn_pack_vkCmdSetBlendConstants(VkCommandBuffer a_0,const float a_1[4]);

// Setup can't be done on shared library constructor
// Needs to be deferred until post-constructor phase to remove the chance of crashing
static void DoSetup() {
    const std::vector<std::pair<const char*, PFN_vkVoidFunction*>> Map = {{
#define PAIR(name, ptr) { #name, (PFN_vkVoidFunction*) ptr }
#include "function_pack_pair.inl"
#undef PAIR
    }};
    for (auto &It : Map) {
      PtrsToLookUp[It.first] = It.second;
    }
    Setup = true;
}
static PFN_vkVoidFunction fexfn_pack_vkGetDeviceProcAddr(VkDevice a_0,const char* a_1){
  if (!Setup) {
    DoSetup();
  }

  struct {VkDevice a_0;const char* a_1;PFN_vkVoidFunction rv;} args;
  args.a_0 = a_0;args.a_1 = a_1;
  THUNKFUNC(vkGetDeviceProcAddr)(&args);

  if (args.rv == nullptr) {
    // Early out if our instance doesn't have the pointer
    // Definitely means we don't support it
    return nullptr;
  }

  // Okay, we found a host side function for this
  // Now return our local instance of this function
  auto It = PtrsToLookUp.find(a_1);
  void *ptr{};
  if (It != PtrsToLookUp.end()) {
    ptr = It->second;
  }
  if (ptr == nullptr) {
    fprintf(stderr, "\tvkGetDeviceProcAddr: Couldn't find Guest symbol: '%s'\n", a_1);
    __builtin_trap();
  }
  return (PFN_vkVoidFunction)ptr;
}

static PFN_vkVoidFunction fexfn_pack_vkGetInstanceProcAddr(VkInstance a_0,const char* a_1){
  if (!Setup) {
    DoSetup();
  }

  // Search our host install first to see if the pointer exists
  // This also populates a map on the host facing side
  struct {VkInstance a_0;const char* a_1;PFN_vkVoidFunction rv;} args;
  args.a_0 = a_0;args.a_1 = a_1;
  THUNKFUNC(vkGetInstanceProcAddr)(&args);
  if (args.rv == nullptr) {
    // Early out if our instance doesn't have the pointer
    // Definitely means we don't support it
    return nullptr;
  }

  void *ptr{};

  auto It = PtrsToLookUp.find(a_1);
  if (It != PtrsToLookUp.end()) {
    ptr = (void*)(It->second);
  }
  if (ptr == nullptr) {
    fprintf(stderr, "\tvkGetInstanceProcAddr: Couldn't find Guest symbol: '%s'\n", a_1);
    __builtin_trap();
  }
  return (PFN_vkVoidFunction)ptr;
}

static void fexfn_pack_vkCmdSetBlendConstants(VkCommandBuffer a_0,const float a_1[4]){
  struct {VkCommandBuffer a_0; float a_1[4];} args;
  args.a_0 = a_0;
  memcpy(args.a_1, a_1, sizeof(float) * 4);
  THUNKFUNC(vkCmdSetBlendConstants)(&args);
}

PFN_vkVoidFunction vkGetDeviceProcAddr(VkDevice a_0,const char* a_1) __attribute__((alias("fexfn_pack_vkGetDeviceProcAddr")));
PFN_vkVoidFunction vkGetInstanceProcAddr(VkInstance a_0,const char* a_1) __attribute__((alias("fexfn_pack_vkGetInstanceProcAddr")));
void vkCmdSetBlendConstants(VkCommandBuffer a_0,const float a_1[4]) __attribute__((alias("fexfn_pack_vkCmdSetBlendConstants")));
}

#define DOLOAD(name) LOAD_LIB(name)
DOLOAD(LIBLIB_NAME)
