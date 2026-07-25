#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <vulkan/vulkan.h>
#ifdef __APPLE__
#include <vulkan/vulkan_metal.h>
#endif

#ifndef VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
#define VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME \
  "VK_KHR_portability_enumeration"
#endif

#ifndef VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR
#define VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR 0x00000001
#endif

typedef struct MlnValaVulkanTestContext {
  void* instance;
  void* physical_device;
  void* device;
  void* graphics_queue;
  uint32_t graphics_queue_family_index;
  void* get_instance_proc_addr;
  void* get_device_proc_addr;
} MlnValaVulkanTestContext;

typedef struct MlnValaVulkanBorrowedImage {
  void* image;
  void* image_view;
  void* memory;
  uint32_t format;
  uint32_t initial_layout;
  uint32_t final_layout;
} MlnValaVulkanBorrowedImage;

static bool has_device_extension(
  VkPhysicalDevice physical_device, const char* name
) {
  uint32_t count = 0;
  if (
    vkEnumerateDeviceExtensionProperties(physical_device, NULL, &count, NULL) !=
    VK_SUCCESS
  ) {
    return false;
  }
  VkExtensionProperties properties[256];
  if (count > 256) {
    count = 256;
  }
  if (
    vkEnumerateDeviceExtensionProperties(
      physical_device, NULL, &count, properties
    ) != VK_SUCCESS
  ) {
    return false;
  }
  for (uint32_t i = 0; i < count; i++) {
    if (strcmp(properties[i].extensionName, name) == 0) {
      return true;
    }
  }
  return false;
}

static bool has_instance_extension(const char* name) {
  uint32_t count = 0;
  if (
    vkEnumerateInstanceExtensionProperties(NULL, &count, NULL) != VK_SUCCESS
  ) {
    return false;
  }
  VkExtensionProperties properties[256];
  if (count > 256) {
    count = 256;
  }
  if (
    vkEnumerateInstanceExtensionProperties(NULL, &count, properties) !=
    VK_SUCCESS
  ) {
    return false;
  }
  for (uint32_t i = 0; i < count; i++) {
    if (strcmp(properties[i].extensionName, name) == 0) {
      return true;
    }
  }
  return false;
}

static bool find_memory_type(
  VkPhysicalDevice physical_device, uint32_t type_filter,
  VkMemoryPropertyFlags properties, uint32_t* out_index
) {
  if (out_index == NULL) {
    return false;
  }
  VkPhysicalDeviceMemoryProperties memory_properties;
  vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

  for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++) {
    if (
      (type_filter & (1U << i)) != 0 &&
      (memory_properties.memoryTypes[i].propertyFlags & properties) ==
        properties
    ) {
      *out_index = i;
      return true;
    }
  }
  return false;
}

bool mln_vala_vulkan_test_context_create(
  MlnValaVulkanTestContext* out_context
) {
  if (out_context == NULL) {
    return false;
  }
  memset(out_context, 0, sizeof(*out_context));

  VkApplicationInfo app_info;
  memset(&app_info, 0, sizeof(app_info));
  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pApplicationName = "maplibre-native-vala-tests";
  app_info.applicationVersion = 1;
  app_info.pEngineName = "maplibre-native-vala-tests";
  app_info.engineVersion = 1;
  app_info.apiVersion = VK_API_VERSION_1_1;

  const char* instance_extensions[4];
  uint32_t instance_extension_count = 0;
#ifdef __APPLE__
  instance_extensions[instance_extension_count++] =
    VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
  if (has_instance_extension(VK_KHR_SURFACE_EXTENSION_NAME)) {
    instance_extensions[instance_extension_count++] =
      VK_KHR_SURFACE_EXTENSION_NAME;
  }
#ifdef VK_EXT_METAL_SURFACE_EXTENSION_NAME
  if (has_instance_extension(VK_EXT_METAL_SURFACE_EXTENSION_NAME)) {
    instance_extensions[instance_extension_count++] =
      VK_EXT_METAL_SURFACE_EXTENSION_NAME;
  }
#endif
#endif

  VkInstanceCreateInfo instance_info;
  memset(&instance_info, 0, sizeof(instance_info));
  instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instance_info.pApplicationInfo = &app_info;
#ifdef __APPLE__
  instance_info.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
  instance_info.enabledExtensionCount = instance_extension_count;
  instance_info.ppEnabledExtensionNames = instance_extensions;
#endif

  VkInstance instance = VK_NULL_HANDLE;
  if (vkCreateInstance(&instance_info, NULL, &instance) != VK_SUCCESS) {
    return false;
  }

  uint32_t physical_device_count = 0;
  if (
    vkEnumeratePhysicalDevices(instance, &physical_device_count, NULL) !=
      VK_SUCCESS ||
    physical_device_count == 0
  ) {
    vkDestroyInstance(instance, NULL);
    return false;
  }
  VkPhysicalDevice physical_devices[16];
  if (physical_device_count > 16) {
    physical_device_count = 16;
  }
  if (
    vkEnumeratePhysicalDevices(
      instance, &physical_device_count, physical_devices
    ) != VK_SUCCESS
  ) {
    vkDestroyInstance(instance, NULL);
    return false;
  }

  for (uint32_t device_index = 0; device_index < physical_device_count;
       device_index++) {
    VkPhysicalDevice physical_device = physical_devices[device_index];
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(
      physical_device, &queue_family_count, NULL
    );
    if (queue_family_count == 0) {
      continue;
    }
    VkQueueFamilyProperties queue_families[64];
    if (queue_family_count > 64) {
      queue_family_count = 64;
    }
    vkGetPhysicalDeviceQueueFamilyProperties(
      physical_device, &queue_family_count, queue_families
    );

    for (uint32_t queue_family_index = 0;
         queue_family_index < queue_family_count; queue_family_index++) {
      if (
        (queue_families[queue_family_index].queueFlags &
         VK_QUEUE_GRAPHICS_BIT) == 0 ||
        queue_families[queue_family_index].queueCount == 0
      ) {
        continue;
      }

      float priority = 1.0f;
      VkDeviceQueueCreateInfo queue_info;
      memset(&queue_info, 0, sizeof(queue_info));
      queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queue_info.queueFamilyIndex = queue_family_index;
      queue_info.queueCount = 1;
      queue_info.pQueuePriorities = &priority;

      VkPhysicalDeviceFeatures supported_features;
      memset(&supported_features, 0, sizeof(supported_features));
      vkGetPhysicalDeviceFeatures(physical_device, &supported_features);
      VkPhysicalDeviceFeatures features;
      memset(&features, 0, sizeof(features));
      features.samplerAnisotropy = supported_features.samplerAnisotropy;
      features.wideLines = supported_features.wideLines;

      const char* device_extensions[4];
      uint32_t device_extension_count = 0;
      if (has_device_extension(physical_device, "VK_KHR_portability_subset")) {
        device_extensions[device_extension_count++] =
          "VK_KHR_portability_subset";
      }
      if (
        has_device_extension(physical_device, VK_KHR_SWAPCHAIN_EXTENSION_NAME)
      ) {
        device_extensions[device_extension_count++] =
          VK_KHR_SWAPCHAIN_EXTENSION_NAME;
      }

      VkDeviceCreateInfo device_info;
      memset(&device_info, 0, sizeof(device_info));
      device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
      device_info.queueCreateInfoCount = 1;
      device_info.pQueueCreateInfos = &queue_info;
      device_info.enabledExtensionCount = device_extension_count;
      device_info.ppEnabledExtensionNames = device_extensions;
      device_info.pEnabledFeatures = &features;

      VkDevice device = VK_NULL_HANDLE;
      if (
        vkCreateDevice(physical_device, &device_info, NULL, &device) !=
        VK_SUCCESS
      ) {
        continue;
      }

      VkQueue queue = VK_NULL_HANDLE;
      vkGetDeviceQueue(device, queue_family_index, 0, &queue);
      if (queue == VK_NULL_HANDLE) {
        vkDestroyDevice(device, NULL);
        continue;
      }

      out_context->instance = instance;
      out_context->physical_device = physical_device;
      out_context->device = device;
      out_context->graphics_queue = queue;
      out_context->graphics_queue_family_index = queue_family_index;
      out_context->get_instance_proc_addr = (void*)vkGetInstanceProcAddr;
      out_context->get_device_proc_addr = (void*)vkGetDeviceProcAddr;
      return true;
    }
  }

  vkDestroyInstance(instance, NULL);
  return false;
}

bool mln_vala_vulkan_test_borrowed_image_create(
  MlnValaVulkanTestContext* context, uint32_t width, uint32_t height,
  MlnValaVulkanBorrowedImage* out_image
) {
  if (
    context == NULL || context->device == NULL ||
    context->physical_device == NULL || width == 0 || height == 0 ||
    out_image == NULL
  ) {
    return false;
  }
  memset(out_image, 0, sizeof(*out_image));

  VkDevice device = (VkDevice)context->device;
  VkImageCreateInfo image_info;
  memset(&image_info, 0, sizeof(image_info));
  image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  image_info.imageType = VK_IMAGE_TYPE_2D;
  image_info.format = VK_FORMAT_R8G8B8A8_UNORM;
  image_info.extent.width = width;
  image_info.extent.height = height;
  image_info.extent.depth = 1;
  image_info.mipLevels = 1;
  image_info.arrayLayers = 1;
  image_info.samples = VK_SAMPLE_COUNT_1_BIT;
  image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
  image_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                     VK_IMAGE_USAGE_SAMPLED_BIT |
                     VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

  VkImage image = VK_NULL_HANDLE;
  if (vkCreateImage(device, &image_info, NULL, &image) != VK_SUCCESS) {
    return false;
  }

  VkMemoryRequirements requirements;
  vkGetImageMemoryRequirements(device, image, &requirements);

  uint32_t memory_type_index = 0;
  if (!find_memory_type(
        (VkPhysicalDevice)context->physical_device, requirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memory_type_index
      )) {
    vkDestroyImage(device, image, NULL);
    return false;
  }

  VkMemoryAllocateInfo allocate_info;
  memset(&allocate_info, 0, sizeof(allocate_info));
  allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocate_info.allocationSize = requirements.size;
  allocate_info.memoryTypeIndex = memory_type_index;

  VkDeviceMemory memory = VK_NULL_HANDLE;
  if (vkAllocateMemory(device, &allocate_info, NULL, &memory) != VK_SUCCESS) {
    vkDestroyImage(device, image, NULL);
    return false;
  }
  if (vkBindImageMemory(device, image, memory, 0) != VK_SUCCESS) {
    vkFreeMemory(device, memory, NULL);
    vkDestroyImage(device, image, NULL);
    return false;
  }

  VkImageViewCreateInfo view_info;
  memset(&view_info, 0, sizeof(view_info));
  view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  view_info.image = image;
  view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  view_info.format = VK_FORMAT_R8G8B8A8_UNORM;
  view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  view_info.subresourceRange.baseMipLevel = 0;
  view_info.subresourceRange.levelCount = 1;
  view_info.subresourceRange.baseArrayLayer = 0;
  view_info.subresourceRange.layerCount = 1;

  VkImageView image_view = VK_NULL_HANDLE;
  if (vkCreateImageView(device, &view_info, NULL, &image_view) != VK_SUCCESS) {
    vkFreeMemory(device, memory, NULL);
    vkDestroyImage(device, image, NULL);
    return false;
  }

  out_image->image = image;
  out_image->image_view = image_view;
  out_image->memory = memory;
  out_image->format = VK_FORMAT_R8G8B8A8_UNORM;
  out_image->initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
  out_image->final_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  return true;
}

void mln_vala_vulkan_test_borrowed_image_destroy(
  MlnValaVulkanTestContext* context, MlnValaVulkanBorrowedImage* image
) {
  if (context == NULL || image == NULL || context->device == NULL) {
    return;
  }
  VkDevice device = (VkDevice)context->device;
  vkDeviceWaitIdle(device);
  if (image->image_view != NULL) {
    vkDestroyImageView(device, (VkImageView)image->image_view, NULL);
    image->image_view = NULL;
  }
  if (image->image != NULL) {
    vkDestroyImage(device, (VkImage)image->image, NULL);
    image->image = NULL;
  }
  if (image->memory != NULL) {
    vkFreeMemory(device, (VkDeviceMemory)image->memory, NULL);
    image->memory = NULL;
  }
}

bool mln_vala_vulkan_test_surface_supported(void) {
#ifdef __APPLE__
#ifdef VK_EXT_METAL_SURFACE_EXTENSION_NAME
  return has_instance_extension(VK_KHR_SURFACE_EXTENSION_NAME) &&
         has_instance_extension(VK_EXT_METAL_SURFACE_EXTENSION_NAME);
#else
  return false;
#endif
#else
  return false;
#endif
}

bool mln_vala_vulkan_test_surface_create(
  MlnValaVulkanTestContext* context, void* metal_layer, void** out_surface
) {
  if (
    context == NULL || context->instance == NULL || metal_layer == NULL ||
    out_surface == NULL
  ) {
    return false;
  }
  *out_surface = NULL;
#ifdef __APPLE__
  PFN_vkCreateMetalSurfaceEXT create_surface =
    (PFN_vkCreateMetalSurfaceEXT)vkGetInstanceProcAddr(
      (VkInstance)context->instance, "vkCreateMetalSurfaceEXT"
    );
  if (create_surface == NULL) {
    return false;
  }
  VkMetalSurfaceCreateInfoEXT surface_info;
  memset(&surface_info, 0, sizeof(surface_info));
  surface_info.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
  surface_info.pLayer = metal_layer;
  VkSurfaceKHR surface = VK_NULL_HANDLE;
  if (
    create_surface(
      (VkInstance)context->instance, &surface_info, NULL, &surface
    ) != VK_SUCCESS
  ) {
    return false;
  }
  *out_surface = surface;
  return true;
#else
  (void)context;
  (void)metal_layer;
  (void)out_surface;
  return false;
#endif
}

void mln_vala_vulkan_test_surface_destroy(
  MlnValaVulkanTestContext* context, void* surface
) {
  if (context == NULL || context->instance == NULL || surface == NULL) {
    return;
  }
  vkDestroySurfaceKHR(
    (VkInstance)context->instance, (VkSurfaceKHR)surface, NULL
  );
}

void mln_vala_vulkan_test_context_destroy(MlnValaVulkanTestContext* context) {
  if (context == NULL) {
    return;
  }
  if (context->device != NULL) {
    vkDeviceWaitIdle((VkDevice)context->device);
    vkDestroyDevice((VkDevice)context->device, NULL);
    context->device = NULL;
  }
  if (context->instance != NULL) {
    vkDestroyInstance((VkInstance)context->instance, NULL);
    context->instance = NULL;
  }
  context->physical_device = NULL;
  context->graphics_queue = NULL;
  context->graphics_queue_family_index = 0;
  context->get_instance_proc_addr = NULL;
  context->get_device_proc_addr = NULL;
}
