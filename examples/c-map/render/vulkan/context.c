#include <SDL3/SDL_vulkan.h>
#include <stdlib.h>
#include <string.h>

#include "context.h"

#include "util.h"

static app_error has_instance_extension(const char* name, bool* out_found) {
  *out_found = false;
  uint32_t count = 0;
  MAP_TRY(
    expect_vk(vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr))
  );
  VkExtensionProperties* properties =
    calloc(count, sizeof(VkExtensionProperties));
  if (properties == nullptr) {
    return APP_ERROR_BACKEND_SETUP_FAILED;
  }
  const app_error error = expect_vk(
    vkEnumerateInstanceExtensionProperties(nullptr, &count, properties)
  );
  if (error == APP_OK) {
    for (uint32_t i = 0; i < count; i += 1) {
      if (strcmp(properties[i].extensionName, name) == 0) {
        *out_found = true;
        break;
      }
    }
  }
  free(properties);
  return error;
}

static app_error create_instance(vulkan_context* context) {
  uint32_t sdl_extension_count = 0;
  const char* const* sdl_extensions =
    SDL_Vulkan_GetInstanceExtensions(&sdl_extension_count);
  if (sdl_extensions == nullptr || sdl_extension_count == 0) {
    return APP_ERROR_BACKEND_SETUP_FAILED;
  }
  bool needs_portability = false;
  MAP_TRY(has_instance_extension(
    VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME, &needs_portability
  ));

  const uint32_t extension_count =
    sdl_extension_count + (needs_portability ? 1 : 0);
  const char** extensions = calloc(extension_count, sizeof(const char*));
  if (extensions == nullptr) {
    return APP_ERROR_BACKEND_SETUP_FAILED;
  }
  for (uint32_t i = 0; i < sdl_extension_count; i += 1) {
    extensions[i] = sdl_extensions[i];
  }
  if (needs_portability) {
    extensions[sdl_extension_count] =
      VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
  }

  const VkApplicationInfo app_info = {
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pApplicationName = "c-map",
    .applicationVersion = 1,
    .pEngineName = "c-map",
    .engineVersion = 1,
    .apiVersion = VK_API_VERSION_1_0,
  };
  const VkInstanceCreateInfo create_info = {
    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .flags =
      needs_portability ? VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR : 0,
    .pApplicationInfo = &app_info,
    .enabledExtensionCount = extension_count,
    .ppEnabledExtensionNames = extensions,
  };
  const app_error error =
    expect_vk(vkCreateInstance(&create_info, nullptr, &context->instance));
  free(extensions);
  return error;
}

static app_error pick_device(vulkan_context* context) {
  uint32_t count = 0;
  MAP_TRY(
    expect_vk(vkEnumeratePhysicalDevices(context->instance, &count, nullptr))
  );
  if (count == 0) {
    return APP_ERROR_BACKEND_SETUP_FAILED;
  }
  VkPhysicalDevice* devices = calloc(count, sizeof(VkPhysicalDevice));
  if (devices == nullptr) {
    return APP_ERROR_BACKEND_SETUP_FAILED;
  }
  app_error error =
    expect_vk(vkEnumeratePhysicalDevices(context->instance, &count, devices));
  if (error != APP_OK) {
    free(devices);
    return error;
  }

  error = APP_ERROR_BACKEND_SETUP_FAILED;
  for (uint32_t device_index = 0; device_index < count && error != APP_OK;
       device_index += 1) {
    VkPhysicalDevice device = devices[device_index];
    uint32_t family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &family_count, nullptr);
    VkQueueFamilyProperties* families =
      calloc(family_count, sizeof(VkQueueFamilyProperties));
    if (families == nullptr) {
      break;
    }
    vkGetPhysicalDeviceQueueFamilyProperties(device, &family_count, families);
    for (uint32_t family_index = 0; family_index < family_count;
         family_index += 1) {
      if ((families[family_index].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) {
        continue;
      }
      if (!SDL_Vulkan_GetPresentationSupport(
            context->instance, device, family_index
          )) {
        continue;
      }
      context->physical_device = device;
      context->queue_family_index = family_index;
      error = APP_OK;
      break;
    }
    free(families);
  }
  free(devices);
  return error;
}

static app_error has_device_extension(
  VkPhysicalDevice device, const char* name, bool* out_found
) {
  *out_found = false;
  uint32_t count = 0;
  MAP_TRY(expect_vk(
    vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr)
  ));
  VkExtensionProperties* properties =
    calloc(count, sizeof(VkExtensionProperties));
  if (properties == nullptr) {
    return APP_ERROR_BACKEND_SETUP_FAILED;
  }
  const app_error error = expect_vk(
    vkEnumerateDeviceExtensionProperties(device, nullptr, &count, properties)
  );
  if (error == APP_OK) {
    for (uint32_t i = 0; i < count; i += 1) {
      if (strcmp(properties[i].extensionName, name) == 0) {
        *out_found = true;
        break;
      }
    }
  }
  free(properties);
  return error;
}

static app_error create_device(vulkan_context* context) {
  const float priority = 1.0f;
  const VkDeviceQueueCreateInfo queue_info = {
    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
    .queueFamilyIndex = context->queue_family_index,
    .queueCount = 1,
    .pQueuePriorities = &priority,
  };
  // A device that exposes the portability subset requires enabling it. The
  // name is spelled out because its declaration lives behind the provisional
  // vulkan_beta.h header.
  bool needs_portability = false;
  MAP_TRY(has_device_extension(
    context->physical_device, "VK_KHR_portability_subset", &needs_portability
  ));
  const char* extensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    "VK_KHR_portability_subset",
  };
  VkPhysicalDeviceFeatures features;
  vkGetPhysicalDeviceFeatures(context->physical_device, &features);
  const VkDeviceCreateInfo create_info = {
    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .queueCreateInfoCount = 1,
    .pQueueCreateInfos = &queue_info,
    .enabledExtensionCount = needs_portability ? 2 : 1,
    .ppEnabledExtensionNames = extensions,
    .pEnabledFeatures = &features,
  };
  MAP_TRY(expect_vk(vkCreateDevice(
    context->physical_device, &create_info, nullptr, &context->device
  )));
  vkGetDeviceQueue(
    context->device, context->queue_family_index, 0, &context->queue
  );
  return APP_OK;
}

static app_error context_create(vulkan_context* context, SDL_Window* window) {
  MAP_TRY(create_instance(context));
  MAP_TRY(expect_sdl(SDL_Vulkan_CreateSurface(
    window, context->instance, nullptr, &context->surface
  )));
  MAP_TRY(pick_device(context));
  return create_device(context);
}

app_error vulkan_context_init(vulkan_context* context, SDL_Window* window) {
  *context = (vulkan_context){};
  const app_error error = context_create(context, window);
  if (error != APP_OK) {
    vulkan_context_deinit(context);
  }
  return error;
}

void vulkan_context_deinit(vulkan_context* context) {
  if (context->device != VK_NULL_HANDLE) {
    vkDestroyDevice(context->device, nullptr);
  }
  if (context->surface != VK_NULL_HANDLE) {
    SDL_Vulkan_DestroySurface(context->instance, context->surface, nullptr);
  }
  if (context->instance != VK_NULL_HANDLE) {
    vkDestroyInstance(context->instance, nullptr);
  }
  *context = (vulkan_context){};
}

void vulkan_context_wait_idle(vulkan_context* context) {
  if (context->device != VK_NULL_HANDLE) {
    vkDeviceWaitIdle(context->device);
  }
}
