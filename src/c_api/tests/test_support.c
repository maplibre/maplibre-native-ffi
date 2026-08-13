#if !defined(_WIN32)
#define _POSIX_C_SOURCE 200809L
#endif

#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
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

#if defined(MLN_FFI_TEST_BACKEND_OPENGL) && defined(MLN_FFI_TEST_OPENGL_WEBGL)
#include <emscripten.h>
#include <emscripten/html5.h>

// A fixture needs a GL context but no on-page canvas, and an OffscreenCanvas
// belongs to a single thread, so each fixture gets its own on the calling
// thread. The registry must be GL.offscreenCanvases, where
// findCanvasEventTarget() resolves the selector under
// -sOFFSCREENCANVAS_SUPPORT.
// The entry carries the canvas under both names its consumers unwrap:
// `offscreenCanvas` for WebGL and emdawnwebgpu, `canvas` for the transfer
// path.
EM_JS(
  void, mln_test_register_offscreen_canvas,
  (const char* name, int width, int height), {
    const id = UTF8ToString(name);
    const canvas = new OffscreenCanvas(width, height);
    Module["GL"].offscreenCanvases[id] = {
      canvas : canvas,
      offscreenCanvas : canvas,
      id : id,
    };
  }
);
EM_JS(void, mln_test_unregister_offscreen_canvas, (const char* name), {
  delete Module["GL"].offscreenCanvases[UTF8ToString(name)];
});

#endif

#if defined(MLN_FFI_TEST_BACKEND_OPENGL) && defined(MLN_FFI_TEST_OPENGL_EGL)
#include <EGL/egl.h>
#include <EGL/eglext.h>
#endif

#if defined(MLN_FFI_TEST_BACKEND_VULKAN)
#include <vulkan/vulkan.h>
#endif

#if defined(MLN_FFI_TEST_BACKEND_WEBGPU)
#include <webgpu/webgpu.h>
#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#include <emscripten/eventloop.h>
#endif
#endif

// Per-thread record of the handles these helpers created. A failing assertion
// longjmps out of the test body, so suite teardown reclaims what the test still
// holds. Tracking is thread local so one thread's teardown leaves another
// thread's runtime alone.
#if defined(_MSC_VER) && !defined(__clang__)
#define MLN_FFI_TEST_THREAD_LOCAL __declspec(thread)
#else
#define MLN_FFI_TEST_THREAD_LOCAL _Thread_local
#endif

#define MLN_FFI_TEST_TRACKED_CAPACITY 8

// Tracked by value: the caller's fixture usually lives on a test stack frame an
// aborting assertion unwinds before teardown runs.
typedef struct tracked_session {
  mln_render_session session;
  void* backend_state;
} tracked_session;

static MLN_FFI_TEST_THREAD_LOCAL mln_runtime tracked_runtime;
static MLN_FFI_TEST_THREAD_LOCAL mln_notification_source
  tracked_notification_source;
static MLN_FFI_TEST_THREAD_LOCAL mln_event_batch compatibility_batch_handle;
static MLN_FFI_TEST_THREAD_LOCAL mln_map
  tracked_maps[MLN_FFI_TEST_TRACKED_CAPACITY];
static MLN_FFI_TEST_THREAD_LOCAL size_t tracked_map_count;
static MLN_FFI_TEST_THREAD_LOCAL tracked_session
  tracked_sessions[MLN_FFI_TEST_TRACKED_CAPACITY];
static MLN_FFI_TEST_THREAD_LOCAL size_t tracked_session_count;

// Fails before the handle exists: failing after creation would strand the very
// handle that overflowed.
static void reserve_map_slot(void) {
  if (tracked_map_count >= MLN_FFI_TEST_TRACKED_CAPACITY) {
    TEST_FAIL_MESSAGE(
      "This test holds more live maps than the suite can track. Destroy maps "
      "as the test finishes with them, or raise MLN_FFI_TEST_TRACKED_CAPACITY."
    );
  }
}

static void track_map(mln_map map) {
  tracked_maps[tracked_map_count] = map;
  tracked_map_count += 1;
}

static void untrack_map(const mln_map map) {
  for (size_t index = 0; index < tracked_map_count; index += 1) {
    if (tracked_maps[index] == map) {
      tracked_maps[index] = tracked_maps[tracked_map_count - 1];
      tracked_map_count -= 1;
      return;
    }
  }
}

// Fails before the caller attaches, for the same reason as reserve_map_slot().
static void reserve_session_slot(void) {
  if (tracked_session_count >= MLN_FFI_TEST_TRACKED_CAPACITY) {
    TEST_FAIL_MESSAGE(
      "This test holds more live render sessions than the suite can track. "
      "Destroy sessions as the test finishes with them, or raise "
      "MLN_FFI_TEST_TRACKED_CAPACITY."
    );
  }
}

static void track_session(const mln_test_render_fixture* fixture) {
  tracked_sessions[tracked_session_count] = (tracked_session){
    .session = fixture->session, .backend_state = fixture->backend_state
  };
  tracked_session_count += 1;
}

static void untrack_session(mln_render_session session) {
  for (size_t index = 0; index < tracked_session_count; index += 1) {
    if (tracked_sessions[index].session == session) {
      tracked_sessions[index] = tracked_sessions[tracked_session_count - 1];
      tracked_session_count -= 1;
      return;
    }
  }
}

mln_runtime mln_test_create_runtime(void) {
  mln_runtime runtime = MLN_HANDLE_NULL;
  mln_notification_source source = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_notification_source_create(&source));
  mln_runtime_options options = mln_runtime_options_default();
  options.notification_source = source;
  const mln_status status = mln_runtime_create(&options, &runtime);
  if (status == MLN_STATUS_INVALID_STATE) {
    mln_notification_source_close(source);
    TEST_FAIL_MESSAGE(
      "This thread already owns a live runtime, so an earlier test leaked one. "
      "Look for the first failing test above and destroy the runtime it "
      "created."
    );
  }
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, status);
  TEST_ASSERT_NOT_EQUAL_UINT64(MLN_HANDLE_NULL, runtime);
  tracked_runtime = runtime;
  tracked_notification_source = source;
  return runtime;
}

mln_map mln_test_create_map_with_options(
  mln_runtime runtime, const mln_map_options* options
) {
  mln_map map = MLN_HANDLE_NULL;
  reserve_map_slot();
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_map_create(runtime, options, &map));
  TEST_ASSERT_NOT_EQUAL_UINT64(MLN_HANDLE_NULL, map);
  track_map(map);
  return map;
}

mln_map mln_test_create_map(mln_runtime runtime) {
  mln_map_options options = mln_map_options_default();
  options.width = 512;
  options.height = 512;
  return mln_test_create_map_with_options(runtime, &options);
}

// Untracking happens only after the destroy succeeds, so a handle whose destroy
// is rejected for now stays tracked for teardown to reclaim.
void mln_test_destroy_runtime(mln_runtime runtime) {
  mln_event_batch_release(compatibility_batch_handle);
  compatibility_batch_handle = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_runtime_destroy(runtime));
  if (tracked_runtime == runtime) {
    tracked_runtime = MLN_HANDLE_NULL;
  }
  if (tracked_notification_source != MLN_HANDLE_NULL) {
    TEST_ASSERT_EQUAL_INT(
      MLN_STATUS_OK, mln_notification_source_close(tracked_notification_source)
    );
    tracked_notification_source = MLN_HANDLE_NULL;
  }
}

void mln_test_destroy_map(mln_map map) {
  mln_event_batch_release(compatibility_batch_handle);
  compatibility_batch_handle = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_map_destroy(map));
  untrack_map(map);
}

uint8_t* mln_test_read_fixture(const char* relative_path, size_t* out_size) {
  if (out_size != NULL) {
    *out_size = 0;
  }
#if defined(__EMSCRIPTEN__)
  // The browser suite embeds its fixtures in the module at a fixed virtual
  // path.
  const char* fixture_dir = "/fixtures";
#else
  const char* fixture_dir = getenv("MLN_FFI_TEST_FIXTURE_DIR");
  TEST_ASSERT_TRUE_MESSAGE(
    fixture_dir != NULL && fixture_dir[0] != '\0',
    "MLN_FFI_TEST_FIXTURE_DIR is unset; run the suite through ctest"
  );
#endif

  char path[1024];
  const int written =
    snprintf(path, sizeof(path), "%s/%s", fixture_dir, relative_path);
  if (written < 0 || (size_t)written >= sizeof(path)) {
    return NULL;
  }

  FILE* file = fopen(path, "rb");
  if (file == NULL) {
    return NULL;
  }
  if (fseek(file, 0, SEEK_END) != 0) {
    fclose(file);
    return NULL;
  }
  const long length = ftell(file);
  if (length < 0 || fseek(file, 0, SEEK_SET) != 0) {
    fclose(file);
    return NULL;
  }

  uint8_t* bytes = malloc((size_t)length == 0 ? 1 : (size_t)length);
  if (bytes == NULL) {
    fclose(file);
    return NULL;
  }
  const size_t read_count = fread(bytes, 1, (size_t)length, file);
  fclose(file);
  if (read_count != (size_t)length) {
    free(bytes);
    return NULL;
  }
  if (out_size != NULL) {
    *out_size = read_count;
  }
  return bytes;
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

uint64_t mln_test_monotonic_milliseconds(void) {
#if defined(_WIN32)
  return (uint64_t)GetTickCount64();
#else
  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  return (uint64_t)now.tv_sec * 1000u + (uint64_t)(now.tv_nsec / 1000000L);
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

// Every thread the suite starts releases its graphics device here: on browser
// WebGPU a thread that returns still holding one is never reported as exited.
#if defined(_WIN32)
static DWORD WINAPI thread_trampoline(LPVOID argument) {
  mln_test_thread* thread = argument;
  thread->entry(thread->argument);
  mln_test_release_thread_gpu_resources();
  return 0;
}
#else
static void* thread_trampoline(void* argument) {
  mln_test_thread* thread = argument;
  thread->entry(thread->argument);
  mln_test_release_thread_gpu_resources();
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

#if defined(MLN_FFI_TEST_BACKEND_METAL)

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

#elif defined(MLN_FFI_TEST_BACKEND_OPENGL) && defined(MLN_FFI_TEST_OPENGL_EGL)

typedef struct egl_state {
  EGLDisplay display;
  EGLConfig config;
  EGLSurface surface;
  EGLContext context;
} egl_state;

// These fixtures render into pbuffers and never present, so they name the
// surfaceless platform: EGL_DEFAULT_DISPLAY resolves to whatever libEGL was
// built for, commonly x11, which fails eglInitialize() without a display
// server. Android and OpenHarmony keep the default display, whose EGL serves
// their own window systems.
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
#elif defined(__OHOS__) || defined(__ANDROID__)
  return eglGetDisplay(EGL_DEFAULT_DISPLAY);
#else
  return eglGetPlatformDisplay(
    EGL_PLATFORM_SURFACELESS_MESA, EGL_DEFAULT_DISPLAY, NULL
  );
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

// These fixtures never call eglTerminate. Android and OpenHarmony resolve
// EGL_DEFAULT_DISPLAY, so the display is shared with everything else in the
// process, and terminating it retires EGL objects the C API still holds. A
// display stays initialized for the process either way, so leaving it is free.
mln_test_fixture_result mln_test_dedicated_egl_surface_create(
  mln_map map, mln_test_render_fixture* fixture
) {
  egl_state* state = calloc(1, sizeof(egl_state));
  if (state == NULL) {
    return MLN_TEST_FIXTURE_UNAVAILABLE;
  }
  state->display = get_egl_display();
  if (
    state->display == EGL_NO_DISPLAY ||
    eglInitialize(state->display, NULL, NULL) == EGL_FALSE
  ) {
    free(state);
    return MLN_TEST_FIXTURE_UNAVAILABLE;
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
    free(state);
    return MLN_TEST_FIXTURE_UNAVAILABLE;
  }
  const EGLint surface_attributes[] = {EGL_WIDTH, 64, EGL_HEIGHT, 64, EGL_NONE};
  state->surface =
    eglCreatePbufferSurface(state->display, state->config, surface_attributes);
  if (state->surface == EGL_NO_SURFACE) {
    free(state);
    return MLN_TEST_FIXTURE_UNAVAILABLE;
  }

  // No context and no eglMakeCurrent here: naming dedicated ownership is what
  // asks the session to create its own and keep it current.
  mln_opengl_surface_descriptor descriptor =
    mln_opengl_surface_descriptor_default();
  descriptor.extent = (mln_render_target_extent){
    .size = sizeof(mln_render_target_extent),
    .width = 64,
    .height = 64,
    .scale_factor = 1.0,
  };
  descriptor.context = (mln_opengl_context_descriptor){
    .size = sizeof(mln_opengl_context_descriptor),
    .platform = MLN_OPENGL_CONTEXT_PLATFORM_EGL,
    .ownership = MLN_OPENGL_CONTEXT_OWNERSHIP_DEDICATED,
    .data = {
      .egl = {
        .size = sizeof(mln_egl_context_descriptor),
        .display = state->display,
        .config = state->config,
        .share_context = NULL,
        .client_api = MLN_OPENGL_CLIENT_API_GLES,
        .get_proc_address = NULL,
      }
    },
  };
  descriptor.surface = state->surface;

  mln_render_session session = 0;
  if (mln_opengl_surface_attach(map, &descriptor, &session) != MLN_STATUS_OK) {
    eglDestroySurface(state->display, state->surface);
    free(state);
    return MLN_TEST_FIXTURE_ATTACH_FAILED;
  }
  fixture->session = session;
  fixture->backend_state = state;
  return MLN_TEST_FIXTURE_OK;
}

void mln_test_dedicated_egl_surface_destroy(mln_test_render_fixture* fixture) {
  if (fixture == NULL) {
    return;
  }
  if (fixture->session != 0) {
    mln_render_session_destroy(fixture->session);
    fixture->session = 0;
  }
  egl_state* state = fixture->backend_state;
  if (state != NULL) {
    eglDestroySurface(state->display, state->surface);
    free(state);
    fixture->backend_state = NULL;
  }
}

bool mln_test_egl_context_is_current(void) {
  return eglGetCurrentContext() != EGL_NO_CONTEXT;
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

#elif defined(MLN_FFI_TEST_BACKEND_WEBGPU)

typedef struct webgpu_state {
  WGPUInstance instance;
  WGPUAdapter adapter;
  WGPUDevice device;
} webgpu_state;

// The suite stands in for the host that owns the WebGPU device. Adapter and
// device requests are futures, and the suite runs on a worker where blocking on
// them is legal.
static bool await_future(WGPUInstance instance, WGPUFuture future) {
  // Bounded so a browser without a WebGPU adapter fails the fixture rather
  // than hanging until the runner's timeout; software device creation is well
  // under a second. A non-zero timeout is legal only on an instance that asked
  // for timed waits.
  const uint64_t timeout_ns = UINT64_C(5) * 1000U * 1000U * 1000U;
  WGPUFutureWaitInfo wait = {.future = future, .completed = false};
  const WGPUWaitStatus status =
    wgpuInstanceWaitAny(instance, 1, &wait, timeout_ns);
  if (status != WGPUWaitStatus_Success || !wait.completed) {
    // emdawnwebgpu reports a refused wait through DEBUG_PRINTF, which an
    // optimised build compiles out, so the status is all that separates a
    // timeout from a rejected wait.
    fprintf(
      stderr, "waiting on a WebGPU future failed (status %d, completed %d)\n",
      (int)status, (int)wait.completed
    );
    return false;
  }
  return true;
}

static void on_adapter(
  WGPURequestAdapterStatus status, WGPUAdapter adapter, WGPUStringView message,
  void* user_data, void* unused
) {
  (void)message;
  (void)unused;
  if (status == WGPURequestAdapterStatus_Success) {
    *(WGPUAdapter*)user_data = adapter;
  }
}

static void on_device(
  WGPURequestDeviceStatus status, WGPUDevice device, WGPUStringView message,
  void* user_data, void* unused
) {
  (void)message;
  (void)unused;
  if (status == WGPURequestDeviceStatus_Success) {
    *(WGPUDevice*)user_data = device;
  }
}

// One device per thread, not per process: emdawnwebgpu keeps WebGPU objects in
// the JS realm of the worker that created them. The device outlives every
// fixture on its thread, so destroy_backend_state() releases nothing.
static _Thread_local webgpu_state thread_webgpu_state;
static _Thread_local bool thread_webgpu_attempted;

static bool create_webgpu_device(webgpu_state* state) {
  // Timed waits have to be asked for up front, or wgpuInstanceWaitAny rejects
  // every non-zero timeout. The capability needs Asyncify, which the
  // emdawnwebgpu port enables; without it wgpuCreateInstance returns NULL.
  WGPUInstanceDescriptor instance_descriptor = {
    .capabilities = {.timedWaitAnyEnable = true},
  };
  state->instance = wgpuCreateInstance(&instance_descriptor);
  if (state->instance == NULL) {
    fprintf(
      stderr, "creating a WebGPU instance with timed waits enabled failed\n"
    );
    return false;
  }

  WGPURequestAdapterOptions adapter_options = {0};
  WGPURequestAdapterCallbackInfo adapter_info = {
    .mode = WGPUCallbackMode_AllowProcessEvents,
    .callback = on_adapter,
    .userdata1 = &state->adapter,
  };
  if (
    !await_future(
      state->instance, wgpuInstanceRequestAdapter(
                         state->instance, &adapter_options, adapter_info
                       )
    ) ||
    state->adapter == NULL
  ) {
    fprintf(stderr, "requesting a WebGPU adapter failed\n");
    wgpuInstanceRelease(state->instance);
    return false;
  }

  WGPUDeviceDescriptor device_descriptor = {0};
  WGPURequestDeviceCallbackInfo device_info = {
    .mode = WGPUCallbackMode_AllowProcessEvents,
    .callback = on_device,
    .userdata1 = &state->device,
  };
  if (
    !await_future(
      state->instance,
      wgpuAdapterRequestDevice(state->adapter, &device_descriptor, device_info)
    ) ||
    state->device == NULL
  ) {
    fprintf(stderr, "requesting a WebGPU device failed\n");
    wgpuAdapterRelease(state->adapter);
    wgpuInstanceRelease(state->instance);
    return false;
  }

  return true;
}

static bool create_backend_state(void** out_state, void* out_context) {
  if (!thread_webgpu_attempted) {
    thread_webgpu_attempted = true;
    if (!create_webgpu_device(&thread_webgpu_state)) {
      thread_webgpu_state = (webgpu_state){0};
    }
  }
  if (thread_webgpu_state.device == NULL) {
    return false;
  }

  *(mln_webgpu_context_descriptor*)out_context =
    (mln_webgpu_context_descriptor){
      .size = sizeof(mln_webgpu_context_descriptor),
      .instance = thread_webgpu_state.instance,
      .device = thread_webgpu_state.device,
      .queue = NULL,
    };
  // Borrowed from the thread's cache, so it outlives the fixture.
  *out_state = &thread_webgpu_state;
  return true;
}

static void destroy_backend_state(void* opaque_state) { (void)opaque_state; }

#elif defined(MLN_FFI_TEST_BACKEND_OPENGL) && defined(MLN_FFI_TEST_OPENGL_WEBGL)

typedef struct webgl_state {
  EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context;
  char id[32];
} webgl_state;

static atomic_uint webgl_canvas_counter;

// The fixture creates a real WebGL2 context and hands it over, the way a
// browser host would.
static bool create_backend_state(void** out_state, void* out_context) {
  webgl_state* state = calloc(1, sizeof(*state));
  if (state == NULL) {
    return false;
  }

  EmscriptenWebGLContextAttributes attributes;
  emscripten_webgl_init_context_attributes(&attributes);
  // WebGL2 is the GLES 3.0 the OpenGL backend targets.
  attributes.majorVersion = 2;
  attributes.minorVersion = 0;
  attributes.depth = EM_TRUE;
  attributes.stencil = EM_TRUE;
  attributes.antialias = EM_FALSE;
  // The suite renders into its own texture rather than presenting.
  attributes.preserveDrawingBuffer = EM_FALSE;
  // The context stays on the thread that created it, and nothing renders to
  // the page, so there is no swap to proxy.
  attributes.explicitSwapControl = EM_FALSE;
  attributes.proxyContextToMainThread = EMSCRIPTEN_WEBGL_CONTEXT_PROXY_DISALLOW;

  const unsigned int serial = atomic_fetch_add(&webgl_canvas_counter, 1U) + 1U;
  (void)snprintf(state->id, sizeof(state->id), "mln-test-%u", serial);
  mln_test_register_offscreen_canvas(state->id, 64, 64);

  char target[sizeof(state->id) + 1];
  (void)snprintf(target, sizeof(target), "#%s", state->id);
  state->context = emscripten_webgl_create_context(target, &attributes);
  if (state->context <= 0) {
    fprintf(
      stderr, "creating the fixture's WebGL context failed: %ld\n",
      (long)state->context
    );
    mln_test_unregister_offscreen_canvas(state->id);
    free(state);
    return false;
  }
  const EMSCRIPTEN_RESULT current_result =
    emscripten_webgl_make_context_current(state->context);
  if (current_result != EMSCRIPTEN_RESULT_SUCCESS) {
    fprintf(
      stderr, "making the fixture's WebGL context current failed: %d\n",
      current_result
    );
    emscripten_webgl_destroy_context(state->context);
    mln_test_unregister_offscreen_canvas(state->id);
    free(state);
    return false;
  }

  *(mln_opengl_context_descriptor*)out_context =
    (mln_opengl_context_descriptor){
      .size = sizeof(mln_opengl_context_descriptor),
      .platform = MLN_OPENGL_CONTEXT_PLATFORM_WEBGL,
      .data = {
        .webgl = {
          .size = sizeof(mln_webgl_context_descriptor),
          .context = state->context,
        }
      },
    };
  *out_state = state;
  return true;
}

static void destroy_backend_state(void* opaque_state) {
  webgl_state* state = opaque_state;
  if (state == NULL) {
    return;
  }
  emscripten_webgl_destroy_context(state->context);
  mln_test_unregister_offscreen_canvas(state->id);
  free(state);
}

#elif defined(MLN_FFI_TEST_BACKEND_OPENGL) && defined(MLN_FFI_TEST_OPENGL_WGL)

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

#elif defined(MLN_FFI_TEST_BACKEND_VULKAN)

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

#if !(defined(MLN_FFI_TEST_BACKEND_OPENGL) && defined(MLN_FFI_TEST_OPENGL_EGL))
// Builds without an EGL provider report the dedicated fixture unavailable, so
// its test skips rather than fails.
mln_test_fixture_result mln_test_dedicated_egl_surface_create(
  mln_map map, mln_test_render_fixture* fixture
) {
  (void)map;
  (void)fixture;
  return MLN_TEST_FIXTURE_UNAVAILABLE;
}

void mln_test_dedicated_egl_surface_destroy(mln_test_render_fixture* fixture) {
  (void)fixture;
}

bool mln_test_egl_context_is_current(void) { return false; }
#endif

#if defined(MLN_FFI_TEST_BACKEND_WEBGPU)

void mln_test_release_thread_gpu_resources(void) {
  if (thread_webgpu_state.device != NULL) {
    // Destroy rather than only release: emdawnwebgpu returns the device's
    // runtime keepalive when device.lost settles, which only destroying
    // resolves.
    wgpuDeviceDestroy(thread_webgpu_state.device);
    wgpuDeviceRelease(thread_webgpu_state.device);
  }
  if (thread_webgpu_state.adapter != NULL) {
    wgpuAdapterRelease(thread_webgpu_state.adapter);
  }
  if (thread_webgpu_state.instance != NULL) {
    wgpuInstanceRelease(thread_webgpu_state.instance);
  }
  thread_webgpu_state = (webgpu_state){0};
  thread_webgpu_attempted = false;
#if defined(__EMSCRIPTEN__)
  // device.lost resolves through the JS job queue, so the thread has to reach
  // its event loop before the keepalive count settles. emscripten_sleep()
  // suspends and resumes through a task, which lets those jobs run; the
  // blocking sleep helper uses Atomics.wait and never would. The wait is on the
  // count itself, bounded, and aborts on expiry, because a thread that keeps a
  // keepalive is never reported as exited and blocks its joiner for good.
  for (unsigned int attempt = 0;
       attempt < 1000 && emscripten_runtime_keepalive_check(); attempt += 1) {
    emscripten_sleep(1);
  }
  if (emscripten_runtime_keepalive_check()) {
    fprintf(
      stderr,
      "a runtime keepalive outlived this thread's graphics device; the thread "
      "would never be reported as exited, so the run fails here rather than "
      "blocking whoever joins it\n"
    );
    abort();
  }
#endif
}

#else

void mln_test_release_thread_gpu_resources(void) {}

#endif

bool mln_test_render_fixture_create(
  mln_map map, mln_test_render_fixture* fixture
) {
  if (map == MLN_HANDLE_NULL || fixture == NULL) {
    return false;
  }
  reserve_session_slot();
  *fixture = (mln_test_render_fixture){0};
#if defined(MLN_FFI_TEST_BACKEND_METAL)
  mln_metal_context_descriptor context = {0};
  if (!create_backend_state(&fixture->backend_state, &context)) {
    return false;
  }
  mln_metal_owned_texture_descriptor descriptor =
    mln_metal_owned_texture_descriptor_default();
#elif defined(MLN_FFI_TEST_BACKEND_OPENGL)
  mln_opengl_context_descriptor context = {0};
  if (!create_backend_state(&fixture->backend_state, &context)) {
    return false;
  }
  mln_opengl_owned_texture_descriptor descriptor =
    mln_opengl_owned_texture_descriptor_default();
#elif defined(MLN_FFI_TEST_BACKEND_VULKAN)
  mln_vulkan_context_descriptor context = {0};
  if (!create_backend_state(&fixture->backend_state, &context)) {
    return false;
  }
  mln_vulkan_owned_texture_descriptor descriptor =
    mln_vulkan_owned_texture_descriptor_default();
#elif defined(MLN_FFI_TEST_BACKEND_WEBGPU)
  mln_webgpu_context_descriptor context = {0};
  if (!create_backend_state(&fixture->backend_state, &context)) {
    return false;
  }
  mln_webgpu_owned_texture_descriptor descriptor =
    mln_webgpu_owned_texture_descriptor_default();
#endif
  descriptor.extent.width = 64;
  descriptor.extent.height = 64;
  descriptor.context = context;
#if defined(MLN_FFI_TEST_BACKEND_METAL)
  const mln_status status =
    mln_metal_owned_texture_attach(map, &descriptor, &fixture->session);
#elif defined(MLN_FFI_TEST_BACKEND_OPENGL)
  const mln_status status =
    mln_opengl_owned_texture_attach(map, &descriptor, &fixture->session);
#elif defined(MLN_FFI_TEST_BACKEND_VULKAN)
  const mln_status status =
    mln_vulkan_owned_texture_attach(map, &descriptor, &fixture->session);
#elif defined(MLN_FFI_TEST_BACKEND_WEBGPU)
  const mln_status status =
    mln_webgpu_owned_texture_attach(map, &descriptor, &fixture->session);
#endif
  if (status != MLN_STATUS_OK || fixture->session == MLN_HANDLE_NULL) {
    destroy_backend_state(fixture->backend_state);
    *fixture = (mln_test_render_fixture){0};
    return false;
  }
  track_session(fixture);
  return true;
}

mln_test_event_batch mln_test_event_batch_default(void) {
  mln_event_batch_release(compatibility_batch_handle);
  compatibility_batch_handle = MLN_HANDLE_NULL;
  return (mln_test_event_batch){.size = sizeof(mln_test_event_batch)};
}

mln_status mln_test_drain_events(
  mln_runtime runtime, size_t max_events, mln_test_event_batch* out_batch
) {
  if (out_batch == NULL || out_batch->size < sizeof(mln_test_event_batch)) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  mln_event_batch_release(compatibility_batch_handle);
  compatibility_batch_handle = MLN_HANDLE_NULL;
  mln_event_batch batch = MLN_HANDLE_NULL;
  mln_status status = mln_runtime_drain_events(runtime, max_events, &batch);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  mln_runtime_event_batch_view view = {
    .size = sizeof(mln_runtime_event_batch_view)
  };
  status = mln_event_batch_get(batch, &view);
  if (status != MLN_STATUS_OK) {
    mln_event_batch_release(batch);
    return status;
  }
  compatibility_batch_handle = batch;
  *out_batch = (mln_test_event_batch){
    .size = sizeof(mln_test_event_batch),
    .event_size = view.event_size,
    .events = view.events,
    .event_count = view.event_count,
    .messages = view.messages,
    .messages_size = view.messages_size,
    .remaining_count = view.remaining_count,
  };
  return MLN_STATUS_OK;
}

size_t mln_test_drain_all(mln_runtime runtime) {
  return mln_test_drain_counting(runtime, 0);
}

size_t mln_test_drain_counting(mln_runtime runtime, uint32_t type) {
  size_t total = 0;
  for (;;) {
    mln_test_event_batch batch = mln_test_event_batch_default();
    if (mln_test_drain_events(runtime, 0, &batch) != MLN_STATUS_OK) {
      return total;
    }
    if (batch.event_count == 0) {
      return total;
    }
    for (size_t index = 0; index < batch.event_count; index += 1) {
      const mln_runtime_event* event =
        (const mln_runtime_event*)((const char*)batch.events +
                                   (index * batch.event_size));
      // Type 0 is not an event type, so it counts every event.
      if (type == 0 || event->type == type) {
        total += 1;
      }
    }
  }
}

bool mln_test_drain_find(
  mln_runtime runtime, uint32_t type, mln_map source,
  mln_runtime_event* out_event, char* out_message, size_t message_capacity
) {
  bool found = false;
  for (;;) {
    mln_test_event_batch batch = mln_test_event_batch_default();
    if (mln_test_drain_events(runtime, 0, &batch) != MLN_STATUS_OK) {
      return found;
    }
    if (batch.event_count == 0) {
      return found;
    }
    for (size_t index = 0; index < batch.event_count; index += 1) {
      const mln_runtime_event* event =
        (const mln_runtime_event*)((const char*)batch.events +
                                   (index * batch.event_size));
      if (found || event->type != type) {
        continue;
      }
      if (source != MLN_HANDLE_NULL && event->source != source) {
        continue;
      }
      found = true;
      if (out_event != NULL) {
        *out_event = *event;
      }
      if (out_message != NULL && message_capacity > 0) {
        size_t copied = event->message_size;
        if (copied > message_capacity - 1) {
          copied = message_capacity - 1;
        }
        if (copied > 0) {
          memcpy(out_message, batch.messages + event->message_offset, copied);
        }
        out_message[copied] = '\0';
      }
    }
  }
}

bool mln_test_pump_until(mln_runtime runtime, atomic_bool* flag) {
  for (unsigned int attempt = 0; attempt < 500; attempt += 1) {
    if (atomic_load(flag)) {
      return true;
    }
    // A short park rather than zero: this waits on another thread's flag, so
    // spinning would burn the whole loop budget before that thread ran.
    if (mln_runtime_pump(runtime, 2) != MLN_STATUS_OK) {
      return false;
    }
    // Drain so the queue does not grow without bound while we wait.
    mln_test_drain_all(runtime);
    if (atomic_load(flag)) {
      return true;
    }
    mln_test_sleep_millisecond();
  }
  return atomic_load(flag);
}

void mln_test_render_fixture_destroy(mln_test_render_fixture* fixture) {
  mln_event_batch_release(compatibility_batch_handle);
  compatibility_batch_handle = MLN_HANDLE_NULL;
  if (fixture == NULL) {
    return;
  }
  if (fixture->session != MLN_HANDLE_NULL) {
    TEST_ASSERT_EQUAL_INT(
      MLN_STATUS_OK, mln_render_session_destroy(fixture->session)
    );
  }
  untrack_session(fixture->session);
  destroy_backend_state(fixture->backend_state);
  *fixture = (mln_test_render_fixture){0};
}

bool mln_test_reclaim_thread_resources(void) {
  mln_event_batch_release(compatibility_batch_handle);
  compatibility_batch_handle = MLN_HANDLE_NULL;
  bool reclaimed = false;
  // Render session before map before runtime: the C API keeps a map with a live
  // session and a runtime with live maps alive on purpose.
  while (tracked_session_count > 0) {
    tracked_session_count -= 1;
    const tracked_session entry = tracked_sessions[tracked_session_count];
    if (entry.session != MLN_HANDLE_NULL) {
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
  if (tracked_runtime != MLN_HANDLE_NULL) {
    mln_runtime_destroy(tracked_runtime);
    tracked_runtime = MLN_HANDLE_NULL;
    reclaimed = true;
  }
  if (tracked_notification_source != MLN_HANDLE_NULL) {
    mln_notification_source_close(tracked_notification_source);
    tracked_notification_source = MLN_HANDLE_NULL;
    reclaimed = true;
  }
  return reclaimed;
}
