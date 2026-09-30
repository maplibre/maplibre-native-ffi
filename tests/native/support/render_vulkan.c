// Vulkan contexts for the render fixture: a device on the first physical
// device with a graphics queue.

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>

#include "env.h"
#include "render.h"
#include "render_backend.h"
#include "unity.h"
#include "wait.h"

typedef struct vulkan_state {
  VkInstance instance;
  VkPhysicalDevice physical_device;
  VkDevice device;
  VkQueue queue;
  uint32_t queue_family_index;
} vulkan_state;

static bool has_device_extension(VkPhysicalDevice device, const char* name) {
  uint32_t count = 0;
  if (
    vkEnumerateDeviceExtensionProperties(device, NULL, &count, NULL) !=
    VK_SUCCESS
  ) {
    return false;
  }
  VkExtensionProperties* properties = calloc(count, sizeof(*properties));
  if (properties == NULL) {
    return false;
  }
  const VkResult status =
    vkEnumerateDeviceExtensionProperties(device, NULL, &count, properties);
  bool found = false;
  if (status == VK_SUCCESS) {
    for (uint32_t index = 0; index < count; index += 1) {
      if (strcmp(properties[index].extensionName, name) == 0) {
        found = true;
        break;
      }
    }
  }
  free(properties);
  return found;
}

static bool create_backend_state(void** out_state, void* out_context) {
  vulkan_state* state = calloc(1, sizeof(*state));
  if (state == NULL) {
    return false;
  }
  const VkApplicationInfo application_info = {
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pApplicationName = "maplibre-native-c-api-tests",
    .applicationVersion = 1,
    .pEngineName = "maplibre-native-c-api-tests",
    .engineVersion = 1,
    .apiVersion = VK_API_VERSION_1_1,
  };
  VkInstanceCreateInfo instance_info = {
    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pApplicationInfo = &application_info,
  };
#if defined(__APPLE__)
  const char* instance_extensions[] = {
    VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
  };
  instance_info.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
  instance_info.enabledExtensionCount = 1;
  instance_info.ppEnabledExtensionNames = instance_extensions;
#endif
  if (vkCreateInstance(&instance_info, NULL, &state->instance) != VK_SUCCESS) {
    free(state);
    return false;
  }

  uint32_t physical_device_count = 0;
  if (
    vkEnumeratePhysicalDevices(state->instance, &physical_device_count, NULL) !=
      VK_SUCCESS ||
    physical_device_count == 0
  ) {
    vkDestroyInstance(state->instance, NULL);
    free(state);
    return false;
  }
  VkPhysicalDevice* physical_devices =
    calloc(physical_device_count, sizeof(*physical_devices));
  if (
    physical_devices == NULL ||
    vkEnumeratePhysicalDevices(
      state->instance, &physical_device_count, physical_devices
    ) != VK_SUCCESS
  ) {
    free(physical_devices);
    vkDestroyInstance(state->instance, NULL);
    free(state);
    return false;
  }

  bool created = false;
  for (uint32_t device_index = 0;
       device_index < physical_device_count && !created; device_index += 1) {
    uint32_t queue_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(
      physical_devices[device_index], &queue_count, NULL
    );
    VkQueueFamilyProperties* queues = calloc(queue_count, sizeof(*queues));
    if (queues == NULL) {
      continue;
    }
    vkGetPhysicalDeviceQueueFamilyProperties(
      physical_devices[device_index], &queue_count, queues
    );
    for (uint32_t queue_index = 0; queue_index < queue_count;
         queue_index += 1) {
      if (
        (queues[queue_index].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0 ||
        queues[queue_index].queueCount == 0
      ) {
        continue;
      }
      const float priority = 1.0F;
      const VkDeviceQueueCreateInfo queue_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = queue_index,
        .queueCount = 1,
        .pQueuePriorities = &priority,
      };
      VkPhysicalDeviceFeatures supported_features = {0};
      vkGetPhysicalDeviceFeatures(
        physical_devices[device_index], &supported_features
      );
      const VkPhysicalDeviceFeatures features = {
        .samplerAnisotropy = supported_features.samplerAnisotropy,
        .wideLines = supported_features.wideLines,
      };
      const char* portability_extensions[] = {"VK_KHR_portability_subset"};
      const bool portability = has_device_extension(
        physical_devices[device_index], portability_extensions[0]
      );
      const VkDeviceCreateInfo device_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_info,
        .enabledExtensionCount = portability ? 1U : 0U,
        .ppEnabledExtensionNames = portability ? portability_extensions : NULL,
        .pEnabledFeatures = &features,
      };
      if (
        vkCreateDevice(
          physical_devices[device_index], &device_info, NULL, &state->device
        ) == VK_SUCCESS
      ) {
        state->physical_device = physical_devices[device_index];
        state->queue_family_index = queue_index;
        vkGetDeviceQueue(state->device, queue_index, 0, &state->queue);
        created = true;
        break;
      }
    }
    free(queues);
  }
  free(physical_devices);
  if (!created) {
    vkDestroyInstance(state->instance, NULL);
    free(state);
    return false;
  }

  *(mln_vulkan_context_descriptor*)out_context =
    (mln_vulkan_context_descriptor){
      .size = sizeof(mln_vulkan_context_descriptor),
      .instance = (void*)(uintptr_t)state->instance,
      .physical_device = (void*)(uintptr_t)state->physical_device,
      .device = (void*)(uintptr_t)state->device,
      .graphics_queue = (void*)(uintptr_t)state->queue,
      .graphics_queue_family_index = state->queue_family_index,
      .get_instance_proc_addr = (void*)vkGetInstanceProcAddr,
      .get_device_proc_addr = (void*)vkGetDeviceProcAddr,
    };
  *out_state = state;
  return true;
}

static void destroy_backend_state(void* opaque_state) {
  vulkan_state* state = opaque_state;
  if (state == NULL) {
    return;
  }
  vkDeviceWaitIdle(state->device);
  vkDestroyDevice(state->device, NULL);
  vkDestroyInstance(state->instance, NULL);
  free(state);
}

uint32_t mln_test_backend_driver(void) { return MLN_RENDER_DRIVER_CORE_WORKER; }

bool mln_test_backend_attach(
  mln_map map, const mln_render_session_attach_options* options,
  void** out_state, mln_render_session* out_session,
  const mln_completion* completion, mln_status* out_status
) {
  mln_vulkan_context_descriptor context = {0};
  if (!create_backend_state(out_state, &context)) {
    return false;
  }
  mln_vulkan_owned_texture_descriptor descriptor =
    mln_vulkan_owned_texture_descriptor_default();
  descriptor.extent.width = 64;
  descriptor.extent.height = 64;
  descriptor.context = context;
  *out_status = mln_vulkan_owned_texture_attach(
    map, &descriptor, options, out_session, completion, MLN_TEST_DIAGNOSTIC
  );
  return true;
}

void mln_test_backend_destroy(void* state) { destroy_backend_state(state); }

void mln_test_release_thread_gpu_resources(void) {}
