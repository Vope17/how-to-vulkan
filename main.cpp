#include "SDL3/SDL.h"
#include "SDL3/SDL_error.h"
#include <SDL3/SDL_vulkan.h>
#include <assert.h>
#include <glm/glm.hpp>
#include <iostream>
#include <stdexcept>
#include <string>
#include <tiny_obj_loader.h>
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
    throw std::runtime_error(SDL_GetError());
  }
}

static inline void tinyobj_chk(bool result) {
  if (result != true) {
    throw std::runtime_error("tinyobj failed");
  }
}

int main(int argc, char **argv) {
  /*
    建立一個vulkan app(Vulkan SDK):
    1.建立VkApplicationInfo
    2.SDL_Vulkan_GetInstanceExtensions獲取擴展
    3.建立VkInstanceCreateInfo(使用到1.)
    4.vkCreateInstance建立instance(使用到3.)
    5.vkEnumeratePhysicalDevices(使用到4.)
    6.
  */

  VkApplicationInfo appInfo{.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                            .pApplicationName = "Learn Vulkan",
                            .apiVersion = VK_API_VERSION_1_3};

  sdl_chk(SDL_Init(SDL_INIT_VIDEO));

  uint32_t instanceExtensionsCount{0};
  char const *const *instanceExtensions{
      SDL_Vulkan_GetInstanceExtensions(&instanceExtensionsCount)};

  for (uint32_t i = 0; i < instanceExtensionsCount; i++) {
    std::cout << " - " << instanceExtensions[i] << "\n";
  }

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

  sdl_chk(SDL_Vulkan_LoadLibrary(nullptr));
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

  VkDevice device(VK_NULL_HANDLE);
  vulkan_chk(vkCreateDevice(devices[deviceIndex], &deviceCI, nullptr, &device));

  VkQueue queue(VK_NULL_HANDLE);
  vkGetDeviceQueue(device, queueFamily, 0, &queue);

  VmaVulkanFunctions vkFunctions{.vkGetInstanceProcAddr = vkGetInstanceProcAddr,
                                 .vkGetDeviceProcAddr = vkGetDeviceProcAddr,
                                 .vkCreateImage = vkCreateImage};
  VmaAllocatorCreateInfo allocatorCI{
      .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
      .physicalDevice = devices[deviceIndex],
      .device = device,
      .pVulkanFunctions = &vkFunctions,
      .instance = instance};

  VmaAllocator allocator(VK_NULL_HANDLE);
  vulkan_chk(vmaCreateAllocator(&allocatorCI, &allocator));

  SDL_Window *window = SDL_CreateWindow(
      "How to Vulkan", 1280u, 720u, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

  VkSurfaceKHR surface(VK_NULL_HANDLE);
  sdl_chk(SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface));

  VkSurfaceCapabilitiesKHR surfaceCaps{};
  vulkan_chk(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(devices[deviceIndex],
                                                       surface, &surfaceCaps));
  int width, height;

  SDL_GetWindowSizeInPixels(window, &width, &height);

  VkExtent2D swapchainExtent{surfaceCaps.currentExtent};
  if (surfaceCaps.currentExtent.width == 0xFFFFFFFF) {

    swapchainExtent = {.width = static_cast<uint32_t>(width),
                       .height = static_cast<uint32_t>(height)};
  }

  const VkFormat imageFormat{VK_FORMAT_B8G8R8A8_SRGB};
  VkSwapchainCreateInfoKHR swapchainCI{
      .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      .surface = surface,
      .minImageCount = surfaceCaps.minImageCount,
      .imageFormat = imageFormat,
      .imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR,
      .imageExtent{.width = swapchainExtent.width,
                   .height = swapchainExtent.height},
      .imageArrayLayers = 1,
      .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
      .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
      .presentMode = VK_PRESENT_MODE_FIFO_KHR};

  VkSwapchainKHR swapchain(VK_NULL_HANDLE);
  vulkan_chk(vkCreateSwapchainKHR(device, &swapchainCI, nullptr, &swapchain));

  uint32_t imageCount{0};
  std::vector<VkImage> swapchainImages;
  vulkan_chk(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr));
  swapchainImages.resize(imageCount);

  vulkan_chk(vkGetSwapchainImagesKHR(device, swapchain, &imageCount,
                                     swapchainImages.data()));
  std::vector<VkImageView> swapchainImageViews;
  swapchainImageViews.resize(imageCount);

  std::vector<VkFormat> depthFormatList{VK_FORMAT_D32_SFLOAT_S8_UINT,
                                        VK_FORMAT_D24_UNORM_S8_UINT};
  VkFormat depthFormat{VK_FORMAT_UNDEFINED};
  for (VkFormat &format : depthFormatList) {
    VkFormatProperties2 formatProperties{
        .sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2};
    vkGetPhysicalDeviceFormatProperties2(devices[deviceIndex], format,
                                         &formatProperties);
    if (formatProperties.formatProperties.optimalTilingFeatures &
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
      depthFormat = format;
      break;
    }
  }

  VkImageCreateInfo depthImageCI{
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .imageType = VK_IMAGE_TYPE_2D,

      .format = depthFormat,
      .extent{.width = static_cast<uint32_t>(width),
              .height = static_cast<uint32_t>(height),
              .depth = 1},
      .mipLevels = 1,
      .arrayLayers = 1,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,

      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
  };

  VmaAllocationCreateInfo allocCI{
      .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,

      .usage = VMA_MEMORY_USAGE_AUTO};

  VkImage depthImage(VK_NULL_HANDLE);
  VmaAllocation depthImageAllocation(VK_NULL_HANDLE);
  vulkan_chk(vmaCreateImage(allocator, &depthImageCI, &allocCI, &depthImage,
                            &depthImageAllocation, nullptr));

  VkImageView depthImageView(VK_NULL_HANDLE);
  VkImageViewCreateInfo depthViewCI{

      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = depthImage,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = depthFormat,
      .subresourceRange{.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                        .levelCount = 1,
                        .layerCount = 1}};
  vulkan_chk(vkCreateImageView(device, &depthViewCI, nullptr, &depthImageView));

  struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv;
  };

  // Mesh data
  tinyobj::attrib_t attrib;

  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  tinyobj_chk(tinyobj::LoadObj(&attrib, &shapes, &materials, nullptr, nullptr,
                               "assets/suzanne.obj"));

  const VkDeviceSize indexCount{shapes[0].mesh.indices.size()};
  std::vector<Vertex> vertices{};
  std::vector<uint16_t> indices{};
  // Load vertex and index data
  for (auto &index : shapes[0].mesh.indices) {
    Vertex v{.pos = {attrib.vertices[index.vertex_index * 3],
                     -attrib.vertices[index.vertex_index * 3 + 1],
                     attrib.vertices[index.vertex_index * 3 + 2]},
             .normal = {attrib.normals[index.normal_index * 3],
                        -attrib.normals[index.normal_index * 3 + 1],
                        attrib.normals[index.normal_index * 3 + 2]},
             .uv = {attrib.texcoords[index.texcoord_index * 2],
                    1.0 - attrib.texcoords[index.texcoord_index * 2 + 1]}};
    vertices.push_back(v);
    indices.push_back(indices.size());
  }

  VkDeviceSize vBufSize{sizeof(Vertex) * vertices.size()};
  VkDeviceSize iBufSize{sizeof(uint16_t) * indices.size()};
  VkBufferCreateInfo bufferCI{.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                              .size = vBufSize + iBufSize,
                              .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                                       VK_BUFFER_USAGE_INDEX_BUFFER_BIT};

  VkBuffer vBuffer(VK_NULL_HANDLE);
  VmaAllocation vBufferAllocation(VK_NULL_HANDLE);
  VmaAllocationCreateInfo vBufferAllocCI{
      .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
               VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
               VMA_ALLOCATION_CREATE_MAPPED_BIT,
      .usage = VMA_MEMORY_USAGE_AUTO};

  VmaAllocationInfo vBufferAllocInfo{};
  vulkan_chk(vmaCreateBuffer(allocator, &bufferCI, &vBufferAllocCI, &vBuffer,
                             &vBufferAllocation, &vBufferAllocInfo));

  memcpy(vBufferAllocInfo.pMappedData, vertices.data(), vBufSize);
  memcpy(((char *)vBufferAllocInfo.pMappedData) + vBufSize, indices.data(),
         iBufSize);

  return 0;
}
