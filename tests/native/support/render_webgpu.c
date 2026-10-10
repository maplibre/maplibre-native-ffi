// WebGPU contexts for the render fixture: one device per thread, created the
// way a browser host would.

#include <emscripten/emscripten.h>
#include <emscripten/eventloop.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <webgpu/webgpu.h>

#include "env.h"
#include "render.h"
#include "render_backend.h"
#include "unity.h"
#include "wait.h"

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
static MLN_TEST_THREAD_LOCAL webgpu_state thread_webgpu_state;
static MLN_TEST_THREAD_LOCAL bool thread_webgpu_attempted;

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
      .instance = thread_webgpu_state.instance,
      .device = thread_webgpu_state.device,
      .queue = NULL,
    };
  // Borrowed from the thread's cache, so it outlives the fixture.
  *out_state = &thread_webgpu_state;
  return true;
}

static void destroy_backend_state(void* opaque_state) { (void)opaque_state; }

uint32_t mln_test_backend_driver(void) {
  return MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD;
}

bool mln_test_backend_attach(
  mln_map map, const mln_render_session_attach_options* options,
  void** out_state, mln_render_session* out_session,
  const mln_completion* completion, mln_status* out_status
) {
  mln_webgpu_context_descriptor context = {0};
  if (!create_backend_state(out_state, &context)) {
    return false;
  }
  mln_webgpu_owned_texture_descriptor descriptor =
    mln_webgpu_owned_texture_descriptor_default();
  descriptor.extent.width = 64;
  descriptor.extent.height = 64;
  descriptor.context = context;
  *out_status = mln_map_attach_webgpu_owned_texture(
    map, &descriptor, options, out_session, completion, MLN_TEST_DIAGNOSTIC
  );
  return true;
}

void mln_test_backend_destroy(void* state) { destroy_backend_state(state); }

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
