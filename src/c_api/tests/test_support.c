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
#include <pthread.h>
#include <time.h>
#endif

#if defined(MLN_TEST_BACKEND_OPENGL) && defined(MLN_TEST_OPENGL_EGL)
#include <EGL/egl.h>
#include <EGL/eglext.h>
#endif

#if defined(MLN_TEST_BACKEND_VULKAN)
#include <vulkan/vulkan.h>
#endif

// Per-thread record of the handles these helpers created. A failing assertion
// longjmps out of the test body, so the test's own teardown never runs and the
// handles it holds would otherwise stay live: the next test on this thread
// would then fail to create a runtime and the single real failure would look
// like a suite-wide outage. Tracking is thread local so the owner thread's
// teardown leaves a worker thread's runtime alone.
#if defined(_MSC_VER) && !defined(__clang__)
#define MLN_TEST_THREAD_LOCAL __declspec(thread)
#else
#define MLN_TEST_THREAD_LOCAL _Thread_local
#endif

#define MLN_TEST_TRACKED_CAPACITY 8

// Sessions are tracked by value. The caller's fixture usually lives on the test
// stack frame, which an aborting assertion unwinds before teardown runs, so a
// pointer to it would dangle.
typedef struct tracked_session {
  mln_render_session* session;
  void* backend_state;
} tracked_session;

static MLN_TEST_THREAD_LOCAL mln_runtime* tracked_runtime;
static MLN_TEST_THREAD_LOCAL mln_map* tracked_maps[MLN_TEST_TRACKED_CAPACITY];
static MLN_TEST_THREAD_LOCAL size_t tracked_map_count;
static MLN_TEST_THREAD_LOCAL tracked_session
  tracked_sessions[MLN_TEST_TRACKED_CAPACITY];
static MLN_TEST_THREAD_LOCAL size_t tracked_session_count;

static void track_map(mln_map* map) {
  // Dropping the overflow silently would leave a live map outside teardown,
  // which keeps the runtime alive and cascades into every later test. Failing
  // here names the real problem instead.
  if (tracked_map_count >= MLN_TEST_TRACKED_CAPACITY) {
    TEST_FAIL_MESSAGE(
      "This test holds more live maps than the suite can track. Destroy maps "
      "as the test finishes with them, or raise MLN_TEST_TRACKED_CAPACITY."
    );
  }
  tracked_maps[tracked_map_count] = map;
  tracked_map_count += 1;
}

static void untrack_map(const mln_map* map) {
  for (size_t index = 0; index < tracked_map_count; index += 1) {
    if (tracked_maps[index] == map) {
      tracked_maps[index] = tracked_maps[tracked_map_count - 1];
      tracked_map_count -= 1;
      return;
    }
  }
}

static void track_session(const mln_test_render_fixture* fixture) {
  if (tracked_session_count >= MLN_TEST_TRACKED_CAPACITY) {
    TEST_FAIL_MESSAGE(
      "This test holds more live render sessions than the suite can track. "
      "Destroy sessions as the test finishes with them, or raise "
      "MLN_TEST_TRACKED_CAPACITY."
    );
  }
  tracked_sessions[tracked_session_count] = (tracked_session){
    .session = fixture->session, .backend_state = fixture->backend_state
  };
  tracked_session_count += 1;
}

static void untrack_session(const mln_render_session* session) {
  for (size_t index = 0; index < tracked_session_count; index += 1) {
    if (tracked_sessions[index].session == session) {
      tracked_sessions[index] = tracked_sessions[tracked_session_count - 1];
      tracked_session_count -= 1;
      return;
    }
  }
}

mln_runtime* mln_test_create_runtime(void) {
  mln_runtime* runtime = NULL;
  const mln_runtime_options options = mln_runtime_options_default();
  const mln_status status = mln_runtime_create(&options, &runtime);
  if (status == MLN_STATUS_INVALID_STATE) {
    TEST_FAIL_MESSAGE(
      "This thread already owns a live runtime, so an earlier test leaked one. "
      "Look for the first failing test above and destroy the runtime it "
      "created."
    );
  }
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, status);
  TEST_ASSERT_NOT_NULL(runtime);
  tracked_runtime = runtime;
  return runtime;
}

mln_map* mln_test_create_map_with_options(
  mln_runtime* runtime, const mln_map_options* options
) {
  mln_map* map = NULL;
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_map_create(runtime, options, &map));
  TEST_ASSERT_NOT_NULL(map);
  track_map(map);
  return map;
}

mln_map* mln_test_create_map(mln_runtime* runtime) {
  mln_map_options options = mln_map_options_default();
  options.width = 512;
  options.height = 512;
  return mln_test_create_map_with_options(runtime, &options);
}

// Untracking happens only after the destroy succeeds. A destroy that is
// temporarily invalid -- destroying a map that still has a render session
// attached -- longjmps out of the assertion below, and the handle has to stay
// tracked so teardown can still reclaim it.
void mln_test_destroy_runtime(mln_runtime* runtime) {
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_runtime_destroy(runtime));
  if (tracked_runtime == runtime) {
    tracked_runtime = NULL;
  }
}

void mln_test_destroy_map(mln_map* map) {
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_map_destroy(map));
  untrack_map(map);
}

void mln_test_sleep_millisecond(void) { mln_test_sleep_milliseconds(1); }

void mln_test_sleep_milliseconds(unsigned int milliseconds) {
#if defined(_WIN32)
  Sleep(milliseconds);
#else
  const struct timespec duration = {
    .tv_sec = (time_t)(milliseconds / 1000u),
    .tv_nsec = (long)(milliseconds % 1000u) * 1000000L,
  };
  nanosleep(&duration, NULL);
#endif
}

struct mln_test_thread {
  void (*entry)(void*);
  void* argument;
#if defined(_WIN32)
  HANDLE handle;
#else
  pthread_t handle;
#endif
};

#if defined(_WIN32)
static DWORD WINAPI thread_trampoline(LPVOID argument) {
  mln_test_thread* thread = argument;
  thread->entry(thread->argument);
  return 0;
}
#else
static void* thread_trampoline(void* argument) {
  mln_test_thread* thread = argument;
  thread->entry(thread->argument);
  return NULL;
}
#endif

mln_test_thread* mln_test_thread_start(void (*entry)(void*), void* argument) {
  mln_test_thread* thread = calloc(1, sizeof(*thread));
  TEST_ASSERT_NOT_NULL(thread);
  thread->entry = entry;
  thread->argument = argument;
#if defined(_WIN32)
  thread->handle = CreateThread(NULL, 0, thread_trampoline, thread, 0, NULL);
  TEST_ASSERT_NOT_NULL(thread->handle);
#else
  TEST_ASSERT_EQUAL_INT(
    0, pthread_create(&thread->handle, NULL, thread_trampoline, thread)
  );
#endif
  return thread;
}

void mln_test_thread_join(mln_test_thread* thread) {
  if (thread == NULL) {
    return;
  }
#if defined(_WIN32)
  WaitForSingleObject(thread->handle, INFINITE);
  CloseHandle(thread->handle);
#else
  pthread_join(thread->handle, NULL);
#endif
  free(thread);
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
  track_session(fixture);
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
  untrack_session(fixture->session);
  destroy_backend_state(fixture->backend_state);
  *fixture = (mln_test_render_fixture){0};
}

bool mln_test_reclaim_thread_resources(void) {
  bool reclaimed = false;
  // Render session before map before runtime: the C API keeps a map with a live
  // session and a runtime with live maps alive on purpose.
  while (tracked_session_count > 0) {
    tracked_session_count -= 1;
    const tracked_session entry = tracked_sessions[tracked_session_count];
    if (entry.session != NULL) {
      mln_render_session_destroy(entry.session);
    }
    destroy_backend_state(entry.backend_state);
    reclaimed = true;
  }
  while (tracked_map_count > 0) {
    tracked_map_count -= 1;
    mln_map_destroy(tracked_maps[tracked_map_count]);
    reclaimed = true;
  }
  if (tracked_runtime != NULL) {
    mln_runtime_destroy(tracked_runtime);
    tracked_runtime = NULL;
    reclaimed = true;
  }
  return reclaimed;
}
