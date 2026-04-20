#include "SDL3/SDL.h"
#include <SDL3/SDL_vulkan.h>
#include <assert.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

static inline void vulkan_chk(VkResult result) {
  if (result != VK_SUCCESS) {
    throw std::runtime_error("Vulkan error code: " + std::to_string(result));
  }
}

static inline void sdl_chk(bool result) {
  if (result != true) {
    throw std::runtime_error("sdk error");
  }
}

int main(int argc, char **argv) {
  VkApplicationInfo appInfo{.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                            .pApplicationName = "Learn Vulkan",
                            .apiVersion = VK_API_VERSION_1_3};

  if (SDL_Init(SDL_INIT_VIDEO) != true) {
    std::cerr << "SDL failed: " << SDL_GetError() << std::endl;
    return 1;
  }
  uint32_t instanceExtensionsCount{0};
  char const *const *instanceExtensions{
      SDL_Vulkan_GetInstanceExtensions(&instanceExtensionsCount)};
  VkInstanceCreateInfo instanceCI{
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pApplicationInfo = &appInfo,
      .enabledExtensionCount = instanceExtensionsCount,
      .ppEnabledExtensionNames = instanceExtensions,
  };
  VkInstance instance{VK_NULL_HANDLE};
  vulkan_chk(vkCreateInstance(&instanceCI, nullptr, &instance));

  uint32_t deviceCount{0};
  vulkan_chk(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr));
  std::vector<VkPhysicalDevice> devices(deviceCount);
  vulkan_chk(
      vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()));

  uint32_t deviceIndex{0};
  if (argc > 1) {

    deviceIndex = std::stoi(argv[1]);
    assert(deviceIndex < deviceCount);
  }

  VkPhysicalDeviceProperties2 deviceProperties{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
  vkGetPhysicalDeviceProperties2(devices[deviceIndex], &deviceProperties);
  std::cout << "Selected device: " << deviceProperties.properties.deviceName
            << "\n";

  uint32_t queueFamilyCount{0};

  vkGetPhysicalDeviceQueueFamilyProperties(devices[deviceIndex],
                                           &queueFamilyCount, nullptr);
  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(
      devices[deviceIndex], &queueFamilyCount, queueFamilies.data());
  uint32_t queueFamily{0};
  for (size_t i = 0; i < queueFamilies.size(); i++) {
    if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      queueFamily = i;
      break;
    }
  }

  sdl_chk(SDL_Vulkan_GetPresentationSupport(instance, devices[deviceIndex],
                                            queueFamily));

  const float qfpriorities{1.0f};
  VkDeviceQueueCreateInfo queueCI{
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,

      .queueFamilyIndex = queueFamily,
      .queueCount = 1,
      .pQueuePriorities = &qfpriorities};

  const std::vector<const char *> deviceExtensions{
      VK_KHR_SWAPCHAIN_EXTENSION_NAME};

  VkPhysicalDeviceVulkan12Features enabledVk12Features{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
      .descriptorIndexing = true,
      .shaderSampledImageArrayNonUniformIndexing = true,
      .descriptorBindingVariableDescriptorCount = true,
      .runtimeDescriptorArray = true,
      .bufferDeviceAddress = true};
  VkPhysicalDeviceVulkan13Features enabledVk13Features{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
      .pNext = &enabledVk12Features,
      .synchronization2 = true,
      .dynamicRendering = true,
  };
  VkPhysicalDeviceFeatures enabledVk10Features{.samplerAnisotropy = VK_TRUE};

  VkDeviceCreateInfo deviceCI{
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .pNext = &enabledVk13Features,
      .queueCreateInfoCount = 1,
      .pQueueCreateInfos = &queueCI,
      .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
      .ppEnabledExtensionNames = deviceExtensions.data(),
      .pEnabledFeatures = &enabledVk10Features};

  VkDevice device{VK_NULL_HANDLE};

  VkQueue queue{VK_NULL_HANDLE};

  vulkan_chk(vkCreateDevice(devices[deviceIndex], &deviceCI, nullptr, &device));

  vkGetDeviceQueue(device, queueFamily, 0, &queue);

  VmaVulkanFunctions vkFunctions{

      .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
      .vkGetDeviceProcAddr = vkGetDeviceProcAddr,

      .vkCreateImage = vkCreateImage};
  VmaAllocatorCreateInfo allocatorCI{
      .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
      .physicalDevice = devices[deviceIndex],
      .device = device,
      .pVulkanFunctions = &vkFunctions,
      .instance = instance};

  VmaAllocator allocator{VK_NULL_HANDLE};
  vulkan_chk(vmaCreateAllocator(&allocatorCI, &allocator));

  SDL_Window *window = SDL_CreateWindow(
      "How to Vulkan", 1280u, 720u, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

  VkSurfaceKHR surface{VK_NULL_HANDLE};
  sdl_chk(SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface));

  VkSurfaceCapabilitiesKHR surfaceCaps{};
  sdl_chk(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(devices[deviceIndex],
                                                    surface, &surfaceCaps));

  VkExtent2D swapchainExtent{surfaceCaps.currentExtent};
  if (surfaceCaps.currentExtent.width == 0xFFFFFFFF) {

    swapchainExtent = {.width = static_cast<uint32_t>(windowSize.x),
                       .height = static_cast<uint32_t>(windowSize.y)};
  }
  return 0;
}
