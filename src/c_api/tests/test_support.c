#if !defined(_WIN32)
#define _POSIX_C_SOURCE 200809L
#endif

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "test_support.h"

#include "unity.h"

#if defined(_WIN32)
#include <windows.h>
#else
#include <time.h>
#endif

#if defined(MLN_TEST_BACKEND_OPENGL) && defined(MLN_TEST_OPENGL_EGL)
#include <EGL/egl.h>
#include <EGL/eglext.h>
#endif

#if defined(MLN_TEST_BACKEND_VULKAN)
#include <vulkan/vulkan.h>
#endif

mln_runtime* mln_test_create_runtime(void) {
  mln_runtime* runtime = NULL;
  const mln_runtime_options options = mln_runtime_options_default();
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_runtime_create(&options, &runtime));
  TEST_ASSERT_NOT_NULL(runtime);
  return runtime;
}

mln_map* mln_test_create_map(mln_runtime* runtime) {
  mln_map* map = NULL;
  mln_map_options options = mln_map_options_default();
  options.width = 512;
  options.height = 512;
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_map_create(runtime, &options, &map));
  TEST_ASSERT_NOT_NULL(map);
  return map;
}

void mln_test_destroy_runtime(mln_runtime* runtime) {
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_runtime_destroy(runtime));
}

void mln_test_destroy_map(mln_map* map) {
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_map_destroy(map));
}

void mln_test_sleep_millisecond(void) {
#if defined(_WIN32)
  Sleep(1);
#else
  const struct timespec duration = {.tv_sec = 0, .tv_nsec = 1000000};
  nanosleep(&duration, NULL);
#endif
}

#if defined(MLN_TEST_BACKEND_METAL)

extern void* MTLCreateSystemDefaultDevice(void);

typedef struct metal_state {
  void* device;
} metal_state;

static bool create_backend_state(void** out_state, void* out_context) {
  metal_state* state = calloc(1, sizeof(*state));
  if (state == NULL) {
    return false;
  }
  state->device = MTLCreateSystemDefaultDevice();
  if (state->device == NULL) {
    free(state);
    return false;
  }
  *(mln_metal_context_descriptor*)out_context = (mln_metal_context_descriptor){
    .size = sizeof(mln_metal_context_descriptor), .device = state->device
  };
  *out_state = state;
  return true;
}

static void destroy_backend_state(void* opaque_state) { free(opaque_state); }

#elif defined(MLN_TEST_BACKEND_OPENGL) && defined(MLN_TEST_OPENGL_EGL)

typedef struct egl_state {
  EGLDisplay display;
  EGLConfig config;
  EGLSurface surface;
  EGLContext context;
} egl_state;

static EGLDisplay get_egl_display(void) {
#if defined(__APPLE__)
  const EGLAttrib attributes[] = {
    EGL_PLATFORM_ANGLE_TYPE_ANGLE,
    EGL_PLATFORM_ANGLE_TYPE_METAL_ANGLE,
    EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE,
    EGL_PLATFORM_ANGLE_DEVICE_TYPE_HARDWARE_ANGLE,
    EGL_NONE,
  };
  return eglGetPlatformDisplay(EGL_PLATFORM_ANGLE_ANGLE, NULL, attributes);
#else
  return eglGetDisplay(EGL_DEFAULT_DISPLAY);
#endif
}

static bool create_backend_state(void** out_state, void* out_context) {
  egl_state* state = calloc(1, sizeof(*state));
  if (state == NULL) {
    return false;
  }
  state->display = get_egl_display();
  if (
    state->display == EGL_NO_DISPLAY ||
    eglInitialize(state->display, NULL, NULL) == EGL_FALSE
  ) {
    free(state);
    return false;
  }
  if (eglBindAPI(EGL_OPENGL_ES_API) == EGL_FALSE) {
    eglTerminate(state->display);
    free(state);
    return false;
  }

  const EGLint config_attributes[] = {
    EGL_SURFACE_TYPE,
    EGL_PBUFFER_BIT,
    EGL_RENDERABLE_TYPE,
    EGL_OPENGL_ES3_BIT,
    EGL_RED_SIZE,
    8,
    EGL_GREEN_SIZE,
    8,
    EGL_BLUE_SIZE,
    8,
    EGL_ALPHA_SIZE,
    8,
    EGL_DEPTH_SIZE,
    24,
    EGL_STENCIL_SIZE,
    8,
    EGL_NONE,
  };
  EGLint config_count = 0;
  if (
    eglChooseConfig(
      state->display, config_attributes, &state->config, 1, &config_count
    ) == EGL_FALSE ||
    config_count == 0 || state->config == NULL
  ) {
    eglTerminate(state->display);
    free(state);
    return false;
  }

  const EGLint context_attributes[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
  state->context = eglCreateContext(
    state->display, state->config, EGL_NO_CONTEXT, context_attributes
  );
  const EGLint surface_attributes[] = {EGL_WIDTH, 8, EGL_HEIGHT, 8, EGL_NONE};
  state->surface =
    eglCreatePbufferSurface(state->display, state->config, surface_attributes);
  if (
    state->context == EGL_NO_CONTEXT || state->surface == EGL_NO_SURFACE ||
    eglMakeCurrent(
      state->display, state->surface, state->surface, state->context
    ) == EGL_FALSE
  ) {
    if (state->surface != EGL_NO_SURFACE) {
      eglDestroySurface(state->display, state->surface);
    }
    if (state->context != EGL_NO_CONTEXT) {
      eglDestroyContext(state->display, state->context);
    }
    eglTerminate(state->display);
    free(state);
    return false;
  }

  *(mln_opengl_context_descriptor*)out_context =
    (mln_opengl_context_descriptor){
      .size = sizeof(mln_opengl_context_descriptor),
      .platform = MLN_OPENGL_CONTEXT_PLATFORM_EGL,
      .data = {
        .egl = {
          .size = sizeof(mln_egl_context_descriptor),
          .display = state->display,
          .config = state->config,
          .share_context = state->context,
          .get_proc_address = NULL,
        }
      },
    };
  *out_state = state;
  return true;
}

static void destroy_backend_state(void* opaque_state) {
  egl_state* state = opaque_state;
  if (state == NULL) {
    return;
  }
  eglMakeCurrent(
    state->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT
  );
  eglDestroySurface(state->display, state->surface);
  eglDestroyContext(state->display, state->context);
  eglTerminate(state->display);
  free(state);
}

#elif defined(MLN_TEST_BACKEND_OPENGL) && defined(MLN_TEST_OPENGL_WGL)

typedef struct wgl_state {
  HINSTANCE instance;
  HWND window;
  HDC device_context;
  HGLRC context;
} wgl_state;

static bool create_backend_state(void** out_state, void* out_context) {
  static const char class_name[] = "MaplibreNativeCApiTestsWgl";
  wgl_state* state = calloc(1, sizeof(*state));
  if (state == NULL) {
    return false;
  }
  state->instance = GetModuleHandleA(NULL);
  const WNDCLASSA window_class = {
    .style = CS_OWNDC,
    .lpfnWndProc = DefWindowProcA,
    .hInstance = state->instance,
    .lpszClassName = class_name,
  };
  RegisterClassA(&window_class);
  state->window = CreateWindowExA(
    0, class_name, class_name, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
    CW_USEDEFAULT, 8, 8, NULL, NULL, state->instance, NULL
  );
  if (state->window == NULL) {
    free(state);
    return false;
  }
  state->device_context = GetDC(state->window);
  const PIXELFORMATDESCRIPTOR pixel_format = {
    .nSize = sizeof(PIXELFORMATDESCRIPTOR),
    .nVersion = 1,
    .dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
    .iPixelType = PFD_TYPE_RGBA,
    .cColorBits = 32,
    .cDepthBits = 24,
    .cStencilBits = 8,
    .iLayerType = PFD_MAIN_PLANE,
  };
  const int format = ChoosePixelFormat(state->device_context, &pixel_format);
  if (
    format == 0 ||
    SetPixelFormat(state->device_context, format, &pixel_format) == FALSE
  ) {
    ReleaseDC(state->window, state->device_context);
    DestroyWindow(state->window);
    free(state);
    return false;
  }
  state->context = wglCreateContext(state->device_context);
  if (
    state->context == NULL ||
    wglMakeCurrent(state->device_context, state->context) == FALSE
  ) {
    if (state->context != NULL) {
      wglDeleteContext(state->context);
    }
    ReleaseDC(state->window, state->device_context);
    DestroyWindow(state->window);
    free(state);
    return false;
  }
  *(mln_opengl_context_descriptor*)out_context =
    (mln_opengl_context_descriptor){
      .size = sizeof(mln_opengl_context_descriptor),
      .platform = MLN_OPENGL_CONTEXT_PLATFORM_WGL,
      .data = {
        .wgl = {
          .size = sizeof(mln_wgl_context_descriptor),
          .device_context = state->device_context,
          .share_context = state->context,
          .get_proc_address = (void*)wglGetProcAddress,
        }
      },
    };
  *out_state = state;
  return true;
}

static void destroy_backend_state(void* opaque_state) {
  wgl_state* state = opaque_state;
  if (state == NULL) {
    return;
  }
  wglMakeCurrent(NULL, NULL);
  wglDeleteContext(state->context);
  ReleaseDC(state->window, state->device_context);
  DestroyWindow(state->window);
  free(state);
}

#elif defined(MLN_TEST_BACKEND_VULKAN)

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

#endif

bool mln_test_render_fixture_create(
  mln_map* map, mln_test_render_fixture* fixture
) {
  if (map == NULL || fixture == NULL) {
    return false;
  }
  *fixture = (mln_test_render_fixture){0};
#if defined(MLN_TEST_BACKEND_METAL)
  mln_metal_context_descriptor context = {0};
  if (!create_backend_state(&fixture->backend_state, &context)) {
    return false;
  }
  mln_metal_owned_texture_descriptor descriptor =
    mln_metal_owned_texture_descriptor_default();
#elif defined(MLN_TEST_BACKEND_OPENGL)
  mln_opengl_context_descriptor context = {0};
  if (!create_backend_state(&fixture->backend_state, &context)) {
    return false;
  }
  mln_opengl_owned_texture_descriptor descriptor =
    mln_opengl_owned_texture_descriptor_default();
#elif defined(MLN_TEST_BACKEND_VULKAN)
  mln_vulkan_context_descriptor context = {0};
  if (!create_backend_state(&fixture->backend_state, &context)) {
    return false;
  }
  mln_vulkan_owned_texture_descriptor descriptor =
    mln_vulkan_owned_texture_descriptor_default();
#endif
  descriptor.extent.width = 64;
  descriptor.extent.height = 64;
  descriptor.context = context;
#if defined(MLN_TEST_BACKEND_METAL)
  const mln_status status =
    mln_metal_owned_texture_attach(map, &descriptor, &fixture->session);
#elif defined(MLN_TEST_BACKEND_OPENGL)
  const mln_status status =
    mln_opengl_owned_texture_attach(map, &descriptor, &fixture->session);
#elif defined(MLN_TEST_BACKEND_VULKAN)
  const mln_status status =
    mln_vulkan_owned_texture_attach(map, &descriptor, &fixture->session);
#endif
  if (status != MLN_STATUS_OK || fixture->session == NULL) {
    destroy_backend_state(fixture->backend_state);
    *fixture = (mln_test_render_fixture){0};
    return false;
  }
  return true;
}

void mln_test_render_fixture_destroy(mln_test_render_fixture* fixture) {
  if (fixture == NULL) {
    return;
  }
  if (fixture->session != NULL) {
    TEST_ASSERT_EQUAL_INT(
      MLN_STATUS_OK, mln_render_session_destroy(fixture->session)
    );
  }
  destroy_backend_state(fixture->backend_state);
  *fixture = (mln_test_render_fixture){0};
}
