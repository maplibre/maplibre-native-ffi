#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "GraphicsSupport.h"
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan_core.h>

static bool create_egl(mln_test_graphics* state) {
#ifdef __APPLE__
  state->loader = dlopen("libEGL.dylib", RTLD_NOW | RTLD_LOCAL);
#elif defined(__ANDROID__)
  state->loader = dlopen("libEGL.so", RTLD_NOW | RTLD_LOCAL);
#else
  state->loader = dlopen("libEGL.so.1", RTLD_NOW | RTLD_LOCAL);
#endif
  if (!state->loader) return false;
  void* (*get_proc)(const char*) = dlsym(state->loader, "eglGetProcAddress");
  unsigned (*initialize)(void*, int*, int*) =
    dlsym(state->loader, "eglInitialize");
  unsigned (*choose)(void*, const int*, void**, int, int*) =
    dlsym(state->loader, "eglChooseConfig");
  if (!get_proc || !initialize || !choose) return false;
#ifdef __ANDROID__
  void* (*get_display)(void*) = dlsym(state->loader, "eglGetDisplay");
  if (!get_display) return false;
  state->display = get_display(NULL);
#else
  void* (*get_display)(unsigned, void*, const int*) =
    get_proc("eglGetPlatformDisplayEXT");
  if (!get_display) return false;
#if defined(__APPLE__)
  const int display_attributes[] = {0x3203, 0x3489, 0x3209, 0x320A, 0x3038};
  state->display = get_display(0x3202, NULL, display_attributes);
#else
  state->display = get_display(0x31DD, NULL, NULL);
#endif
#endif
  if (!state->display || !initialize(state->display, NULL, NULL)) {
    state->display = NULL;
    return false;
  }
  const int config_attributes[] = {0x3033, 1,  0x3040, 0x40, 0x3024, 8,
                                   0x3023, 8,  0x3022, 8,    0x3021, 8,
                                   0x3025, 24, 0x3026, 8,    0x3038};
  int count = 0;
  return choose(state->display, config_attributes, &state->config, 1, &count) &&
         count > 0;
}

static bool create_vulkan(mln_test_graphics* state) {
#ifdef __APPLE__
  const char* directory = getenv("MLN_FFI_VULKAN_LOADER_DIR");
  if (directory) {
    char path[4096];
    int length =
      snprintf(path, sizeof(path), "%s/libvulkan.1.dylib", directory);
    if (length > 0 && (size_t)length < sizeof(path))
      state->loader = dlopen(path, RTLD_NOW | RTLD_LOCAL);
  } else {
    state->loader = dlopen("libvulkan.1.dylib", RTLD_NOW | RTLD_LOCAL);
  }
#elif defined(__ANDROID__)
  state->loader = dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
#else
  state->loader = dlopen("libvulkan.so.1", RTLD_NOW | RTLD_LOCAL);
#endif
  if (!state->loader) return false;
#define LOAD(name)                                           \
  PFN_##name name = (PFN_##name)dlsym(state->loader, #name); \
  if (!name) return false
  LOAD(vkCreateInstance);
  LOAD(vkEnumeratePhysicalDevices);
  LOAD(vkGetPhysicalDeviceQueueFamilyProperties);
  LOAD(vkGetPhysicalDeviceFeatures);
  LOAD(vkEnumerateDeviceExtensionProperties);
  LOAD(vkCreateDevice);
  LOAD(vkGetDeviceQueue);
#undef LOAD
  const VkApplicationInfo application = {
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pApplicationName = "Binding tests",
    .apiVersion = VK_API_VERSION_1_1,
  };
  VkInstanceCreateInfo instance_info = {
    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pApplicationInfo = &application,
  };
#ifdef __APPLE__
  const char* instance_extensions[] = {
    VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
  };
  instance_info.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
  instance_info.enabledExtensionCount = 1;
  instance_info.ppEnabledExtensionNames = instance_extensions;
#endif
  VkInstance instance = VK_NULL_HANDLE;
  if (vkCreateInstance(&instance_info, NULL, &instance) != VK_SUCCESS)
    return false;
  state->instance = instance;
  uint32_t count = 0;
  if (
    vkEnumeratePhysicalDevices(instance, &count, NULL) != VK_SUCCESS || !count
  )
    return false;
  VkPhysicalDevice* devices = calloc(count, sizeof(*devices));
  if (!devices) return false;
  if (vkEnumeratePhysicalDevices(instance, &count, devices) != VK_SUCCESS) {
    free(devices);
    return false;
  }
  for (uint32_t d = 0; d < count && !state->device; ++d) {
    uint32_t queue_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(devices[d], &queue_count, NULL);
    VkQueueFamilyProperties* queues = calloc(queue_count, sizeof(*queues));
    if (!queues) continue;
    vkGetPhysicalDeviceQueueFamilyProperties(devices[d], &queue_count, queues);
    for (uint32_t q = 0; q < queue_count; ++q) {
      if (
        !(queues[q].queueFlags & VK_QUEUE_GRAPHICS_BIT) || !queues[q].queueCount
      )
        continue;
      uint32_t extension_count = 0;
      vkEnumerateDeviceExtensionProperties(
        devices[d], NULL, &extension_count, NULL
      );
      VkExtensionProperties* extensions =
        calloc(extension_count, sizeof(*extensions));
      bool portability = false;
      if (extensions) {
        if (
          vkEnumerateDeviceExtensionProperties(
            devices[d], NULL, &extension_count, extensions
          ) == VK_SUCCESS
        ) {
          for (uint32_t e = 0; e < extension_count; ++e) {
            portability |=
              strcmp(
                extensions[e].extensionName, "VK_KHR_portability_subset"
              ) == 0;
          }
        }
        free(extensions);
      }
      const float priority = 1;
      const VkDeviceQueueCreateInfo queue_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = q,
        .queueCount = 1,
        .pQueuePriorities = &priority,
      };
      VkPhysicalDeviceFeatures supported = {0};
      vkGetPhysicalDeviceFeatures(devices[d], &supported);
      const VkPhysicalDeviceFeatures features = {
        .samplerAnisotropy = supported.samplerAnisotropy,
        .wideLines = supported.wideLines,
      };
      const char* device_extensions[] = {"VK_KHR_portability_subset"};
      const VkDeviceCreateInfo device_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_info,
        .enabledExtensionCount = portability ? 1 : 0,
        .ppEnabledExtensionNames = portability ? device_extensions : NULL,
        .pEnabledFeatures = &features,
      };
      VkDevice device = VK_NULL_HANDLE;
      if (
        vkCreateDevice(devices[d], &device_info, NULL, &device) == VK_SUCCESS
      ) {
        VkQueue queue = VK_NULL_HANDLE;
        vkGetDeviceQueue(device, q, 0, &queue);
        state->device = device;
        state->physical_device = devices[d];
        state->queue = queue;
        state->queue_family = q;
        break;
      }
    }
    free(queues);
  }
  free(devices);
  state->get_instance_proc_addr = dlsym(state->loader, "vkGetInstanceProcAddr");
  state->get_device_proc_addr = dlsym(state->loader, "vkGetDeviceProcAddr");
  return state->device && state->get_instance_proc_addr &&
         state->get_device_proc_addr;
}

mln_test_graphics* mln_test_graphics_create(bool vulkan) {
  mln_test_graphics* state = calloc(1, sizeof(*state));
  if (!state) return NULL;
  state->vulkan = vulkan;
  if (!(vulkan ? create_vulkan(state) : create_egl(state))) {
    const char* error = dlerror();
    fprintf(
      stderr, "Could not initialize %s test driver: %s\n",
      vulkan ? "Vulkan" : "EGL", error ? error : "driver initialization failed"
    );
    mln_test_graphics_destroy(state);
    return NULL;
  }
  return state;
}

bool mln_test_graphics_make_current(mln_test_graphics* state) {
  if (!state || state->vulkan || !state->display || !state->config)
    return false;
  unsigned (*bind_api)(unsigned) = dlsym(state->loader, "eglBindAPI");
  void* (*create_context)(void*, void*, void*, const int*) =
    dlsym(state->loader, "eglCreateContext");
  void* (*create_surface)(void*, void*, const int*) =
    dlsym(state->loader, "eglCreatePbufferSurface");
  unsigned (*make_current)(void*, void*, void*, void*) =
    dlsym(state->loader, "eglMakeCurrent");
  if (!bind_api || !create_context || !create_surface || !make_current)
    return false;
  if (!bind_api(0x30A0)) return false;
  if (!state->context) {
    const int attributes[] = {0x3098, 3, 0x3038};
    state->context =
      create_context(state->display, state->config, NULL, attributes);
    if (!state->context) return false;
  }
  if (!state->surface) {
    const int attributes[] = {0x3057, 1, 0x3056, 1, 0x3038};
    state->surface = create_surface(state->display, state->config, attributes);
    if (!state->surface) return false;
  }
  return make_current(
    state->display, state->surface, state->surface, state->context
  );
}

void mln_test_graphics_destroy(mln_test_graphics* state) {
  if (!state) return;
  if (state->vulkan) {
    if (state->device) {
      PFN_vkDeviceWaitIdle wait =
        (PFN_vkDeviceWaitIdle)dlsym(state->loader, "vkDeviceWaitIdle");
      PFN_vkDestroyDevice destroy =
        (PFN_vkDestroyDevice)dlsym(state->loader, "vkDestroyDevice");
      wait(state->device);
      destroy(state->device, NULL);
    }
    if (state->instance) {
      PFN_vkDestroyInstance destroy =
        (PFN_vkDestroyInstance)dlsym(state->loader, "vkDestroyInstance");
      destroy(state->instance, NULL);
    }
  } else if (state->display) {
    if (state->context || state->surface) {
      unsigned (*make_current)(void*, void*, void*, void*) =
        dlsym(state->loader, "eglMakeCurrent");
      unsigned (*destroy_surface)(void*, void*) =
        dlsym(state->loader, "eglDestroySurface");
      unsigned (*destroy_context)(void*, void*) =
        dlsym(state->loader, "eglDestroyContext");
      make_current(state->display, NULL, NULL, NULL);
      if (state->surface) destroy_surface(state->display, state->surface);
      if (state->context) destroy_context(state->display, state->context);
    }
    unsigned (*terminate)(void*) = dlsym(state->loader, "eglTerminate");
    terminate(state->display);
  }
  if (state->loader) dlclose(state->loader);
  free(state);
}
