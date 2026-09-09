// Regression test for self-lookups in vkGetInstanceProcAddr and vkGetDeviceProcAddr
// XeSS relies on this behavior, and not emulating it correctly caused a crash in Doom: The Dark Ages

#include <dlfcn.h>

#include <catch2/catch_test_macros.hpp>

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

TEST_CASE("Vulkan ProcAddr self-lookups") {
  void* lib = dlopen("libvulkan.so.1", RTLD_NOW);
  if (!lib) {
    WARN("libvulkan.so.1 not available, nothing to test");
    return;
  }
  auto gipa = (PFN_vkGetInstanceProcAddr)dlsym(lib, "vkGetInstanceProcAddr");
  auto gdpa = (PFN_vkGetDeviceProcAddr)dlsym(lib, "vkGetDeviceProcAddr");
  REQUIRE(gipa);
  REQUIRE(gdpa);

  CHECK(gipa(nullptr, "vkGetInstanceProcAddr") != nullptr);

  VkApplicationInfo app {.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO, .apiVersion = VK_API_VERSION_1_2};
  VkInstanceCreateInfo instance_info {.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, .pApplicationInfo = &app};
  VkInstance instance {};
  if (((PFN_vkCreateInstance)gipa(nullptr, "vkCreateInstance"))(&instance_info, nullptr, &instance) != VK_SUCCESS) {
    WARN("vkCreateInstance failed. No Vulkan driver installed?");
    return;
  }

  CHECK(gipa(instance, "vkGetInstanceProcAddr") != nullptr);
  CHECK(gipa(instance, "vkGetDeviceProcAddr") != nullptr);

  uint32_t count = 1;
  VkPhysicalDevice physical {};
  ((PFN_vkEnumeratePhysicalDevices)gipa(instance, "vkEnumeratePhysicalDevices"))(instance, &count, &physical);
  if (count == 0) {
    WARN("no Vulkan physical device. No Vulkan driver installed?");
    ((PFN_vkDestroyInstance)gipa(instance, "vkDestroyInstance"))(instance, nullptr);
    return;
  }

  const float priority = 1.0f;
  VkDeviceQueueCreateInfo queue {.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, .queueCount = 1, .pQueuePriorities = &priority};
  VkDeviceCreateInfo device_info {.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, .queueCreateInfoCount = 1, .pQueueCreateInfos = &queue};
  VkDevice device {};
  REQUIRE(((PFN_vkCreateDevice)gipa(instance, "vkCreateDevice"))(physical, &device_info, nullptr, &device) == VK_SUCCESS);

  // This checks that XeSS will get the answer it expects.
  auto self = (PFN_vkGetDeviceProcAddr)gdpa(device, "vkGetDeviceProcAddr");
  REQUIRE(self != nullptr);
  CHECK(self(device, "vkDestroyDevice") != nullptr);

  // XeSS doesn't seem to rely on this function, but let's make sure it works correctly anyways.
  CHECK(gdpa(device, "vkGetInstanceProcAddr") == nullptr);

  ((PFN_vkDestroyDevice)gdpa(device, "vkDestroyDevice"))(device, nullptr);
  ((PFN_vkDestroyInstance)gipa(instance, "vkDestroyInstance"))(instance, nullptr);
}
