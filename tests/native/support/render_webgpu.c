
#include <emscripten/emscripten.h>
#include <emscripten/eventloop.h>
#include <emscripten/proxying.h>
#include <emscripten/threading.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
  // A non-zero wait timeout requires the TimedWaitAny instance feature. The
  // bound only keeps an unavailable adapter from hanging the runner, so it is
  // generous: browser shards start their GPU processes together, and a 5 s
  // bound expired before a shard's adapter arrived.
  const uint64_t timeout_ns = UINT64_C(30) * 1000U * 1000U * 1000U;
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
  // every non-zero timeout. The instance feature needs Asyncify, which the
  // emdawnwebgpu port enables; without it wgpuCreateInstance returns NULL.
  static const WGPUInstanceFeatureName timed_wait_any[] = {
    WGPUInstanceFeatureName_TimedWaitAny,
  };
  WGPUInstanceDescriptor instance_descriptor = {
    .requiredFeatureCount = 1,
    .requiredFeatures = timed_wait_any,
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

static bool thread_context(mln_webgpu_context_descriptor* out_context) {
  if (!thread_webgpu_attempted) {
    thread_webgpu_attempted = true;
    if (!create_webgpu_device(&thread_webgpu_state)) {
      thread_webgpu_state = (webgpu_state){0};
    }
  }
  if (thread_webgpu_state.device == NULL) {
    return false;
  }

  *out_context = (mln_webgpu_context_descriptor){
    .instance = thread_webgpu_state.instance,
    .device = thread_webgpu_state.device,
    .queue = NULL,
  };
  return true;
}

// The textures of a borrowed ring fixture, in slot order. An owned-texture
// fixture has no state of its own: the device belongs to the thread.
typedef struct webgpu_ring {
  WGPUTexture textures[MLN_TEXTURE_RING_MAX_DEPTH];
  WGPUTextureView views[MLN_TEXTURE_RING_MAX_DEPTH];
  size_t texture_count;
} webgpu_ring;

static void destroy_backend_state(void* opaque_state) {
  webgpu_ring* ring = opaque_state;
  if (ring == NULL) {
    return;
  }
  for (size_t index = 0; index < ring->texture_count; index += 1) {
    wgpuTextureViewRelease(ring->views[index]);
    wgpuTextureDestroy(ring->textures[index]);
    wgpuTextureRelease(ring->textures[index]);
  }
  free(ring);
}

uint32_t mln_test_backend_driver(void) {
  return MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD;
}

bool mln_test_backend_attach(
  mln_map map, const mln_render_session_attach_options* options,
  void** out_state, mln_render_session* out_session,
  const mln_completion* completion, mln_status* out_status
) {
  mln_webgpu_context_descriptor context = {0};
  if (!thread_context(&context)) {
    return false;
  }
  *out_state = NULL;
  mln_webgpu_owned_texture_descriptor descriptor =
    mln_webgpu_owned_texture_descriptor_default();
  descriptor.extent.width = 64;
  descriptor.extent.height = 64;
  descriptor.extent.scale_factor = 1.0;
  descriptor.context = context;
  *out_status = mln_map_attach_webgpu_owned_texture(
    map, &descriptor, options, out_session, completion, MLN_TEST_DIAGNOSTIC
  );
  return true;
}

void mln_test_backend_destroy(void* state) { destroy_backend_state(state); }

// The side, in pixels, of every texture of a borrowed ring fixture. One row of
// RGBA8 pixels is 256 bytes, the row alignment of a texture-to-buffer copy.
enum { ring_texture_size = 64 };

static MLN_TEST_THREAD_LOCAL size_t attach_ring_depth = 1;

static bool attach_borrowed_ring(
  mln_map map, const mln_render_session_attach_options* options,
  void** out_state, mln_render_session* out_session,
  const mln_completion* completion, mln_status* out_status
) {
  mln_webgpu_context_descriptor context = {0};
  if (!thread_context(&context)) {
    return false;
  }
  webgpu_ring* ring = calloc(1, sizeof(webgpu_ring));
  if (ring == NULL) {
    return false;
  }
  mln_webgpu_borrowed_texture entries[MLN_TEXTURE_RING_MAX_DEPTH];
  for (size_t index = 0; index < attach_ring_depth; index += 1) {
    const WGPUTextureDescriptor texture_descriptor = {
      .usage = WGPUTextureUsage_RenderAttachment |
               WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopySrc,
      .dimension = WGPUTextureDimension_2D,
      .size = {ring_texture_size, ring_texture_size, 1},
      .format = WGPUTextureFormat_RGBA8Unorm,
      .mipLevelCount = 1,
      .sampleCount = 1,
    };
    const WGPUTexture texture =
      wgpuDeviceCreateTexture(thread_webgpu_state.device, &texture_descriptor);
    const WGPUTextureView view =
      texture == NULL ? NULL : wgpuTextureCreateView(texture, NULL);
    if (view == NULL) {
      if (texture != NULL) {
        wgpuTextureRelease(texture);
      }
      fprintf(stderr, "creating a WebGPU ring texture failed\n");
      destroy_backend_state(ring);
      return false;
    }
    ring->textures[index] = texture;
    ring->views[index] = view;
    ring->texture_count += 1;
    entries[index] = (mln_webgpu_borrowed_texture){
      .texture = texture,
      .texture_view = view,
    };
  }
  mln_webgpu_borrowed_texture_descriptor descriptor =
    mln_webgpu_borrowed_texture_descriptor_default();
  descriptor.extent.width = ring_texture_size;
  descriptor.extent.height = ring_texture_size;
  descriptor.extent.scale_factor = 1.0;
  descriptor.physical_width = ring_texture_size;
  descriptor.physical_height = ring_texture_size;
  descriptor.context = context;
  descriptor.textures = entries;
  descriptor.texture_count = ring->texture_count;
  descriptor.format = WGPUTextureFormat_RGBA8Unorm;
  *out_state = ring;
  *out_status = mln_map_attach_webgpu_borrowed_texture(
    map, &descriptor, options, out_session, completion, MLN_TEST_DIAGNOSTIC
  );
  return true;
}

bool mln_test_webgpu_borrowed_ring_create(
  mln_map map, mln_test_render_fixture* fixture, size_t depth
) {
  if (depth == 0 || depth > MLN_TEXTURE_RING_MAX_DEPTH) {
    return false;
  }
  attach_ring_depth = depth;
  const bool attached =
    mln_test_render_fixture_create_with(map, fixture, attach_borrowed_ring);
  attach_ring_depth = 1;
  return attached;
}

void* mln_test_webgpu_ring_texture(
  const mln_test_render_fixture* fixture, size_t index
) {
  const webgpu_ring* ring = fixture->backend_state;
  if (ring == NULL || index >= ring->texture_count) {
    return NULL;
  }
  return ring->textures[index];
}

static void on_main_thread_device_lost(
  const WGPUDevice* device, WGPUDeviceLostReason reason, WGPUStringView message,
  void* user_data, void* unused
) {
  (void)device;
  (void)reason;
  (void)message;
  (void)unused;
  mln_test_flag_set(&((mln_test_webgpu_main_thread_device*)user_data)->lost);
}

static void on_main_thread_device(
  WGPURequestDeviceStatus status, WGPUDevice device, WGPUStringView message,
  void* user_data, void* unused
) {
  (void)message;
  (void)unused;
  mln_test_webgpu_main_thread_device* owner = user_data;
  if (status == WGPURequestDeviceStatus_Success) {
    owner->context.device = device;
  }
  mln_test_flag_set(&owner->created);
}

static void on_main_thread_adapter(
  WGPURequestAdapterStatus status, WGPUAdapter adapter, WGPUStringView message,
  void* user_data, void* unused
) {
  (void)message;
  (void)unused;
  mln_test_webgpu_main_thread_device* owner = user_data;
  if (status != WGPURequestAdapterStatus_Success) {
    mln_test_flag_set(&owner->created);
    return;
  }
  const WGPUDeviceDescriptor descriptor = {
    .deviceLostCallbackInfo = {
      .mode = WGPUCallbackMode_AllowSpontaneous,
      .callback = on_main_thread_device_lost,
      .userdata1 = owner,
    },
  };
  const WGPURequestDeviceCallbackInfo device_info = {
    .mode = WGPUCallbackMode_AllowSpontaneous,
    .callback = on_main_thread_device,
    .userdata1 = owner,
  };
  wgpuAdapterRequestDevice(adapter, &descriptor, device_info);
  wgpuAdapterRelease(adapter);
}

// Runs on the main thread, which must not block, so every step answers through
// a spontaneous callback that its event loop runs once this call returns.
static void request_main_thread_device(void* context) {
  mln_test_webgpu_main_thread_device* owner = context;
  owner->context.instance = wgpuCreateInstance(NULL);
  if (owner->context.instance == NULL) {
    mln_test_flag_set(&owner->created);
    return;
  }
  const WGPURequestAdapterOptions options = {0};
  const WGPURequestAdapterCallbackInfo adapter_info = {
    .mode = WGPUCallbackMode_AllowSpontaneous,
    .callback = on_main_thread_adapter,
    .userdata1 = owner,
  };
  wgpuInstanceRequestAdapter(owner->context.instance, &options, adapter_info);
}

static void destroy_main_thread_device(void* context) {
  mln_test_webgpu_main_thread_device* owner = context;
  if (owner->context.device != NULL) {
    wgpuDeviceDestroy(owner->context.device);
    wgpuDeviceRelease(owner->context.device);
  }
  if (owner->context.instance != NULL) {
    wgpuInstanceRelease(owner->context.instance);
  }
}

static bool run_on_main_thread(void (*function)(void*), void* context) {
  return emscripten_proxy_sync(
           emscripten_proxy_get_system_queue(),
           emscripten_main_runtime_thread_id(), function, context
         ) == 1;
}

bool mln_test_webgpu_main_thread_device_create(
  mln_test_webgpu_main_thread_device* device
) {
  *device = (mln_test_webgpu_main_thread_device){0};
  if (
    !run_on_main_thread(request_main_thread_device, device) ||
    !mln_test_wait_for_flag(&device->created)
  ) {
    fprintf(stderr, "the main thread answered no WebGPU device request\n");
    return false;
  }
  if (device->context.device == NULL) {
    fprintf(stderr, "the main thread got no WebGPU device\n");
    (void)run_on_main_thread(destroy_main_thread_device, device);
    *device = (mln_test_webgpu_main_thread_device){0};
    return false;
  }
  return true;
}

void mln_test_webgpu_main_thread_device_destroy(
  mln_test_webgpu_main_thread_device* device
) {
  if (device->context.device == NULL) {
    return;
  }
  TEST_ASSERT_TRUE(run_on_main_thread(destroy_main_thread_device, device));
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_wait_for_flag(&device->lost),
    "the main thread never reported its WebGPU device lost"
  );
  *device = (mln_test_webgpu_main_thread_device){0};
}

static MLN_TEST_THREAD_LOCAL const mln_webgpu_context_descriptor*
  attach_context;

// Attach validation makes no WebGPU call, so the attach can run here. The
// fixture services on the main thread, where the backend initializes.
static bool attach_main_thread_texture(
  mln_map map, const mln_render_session_attach_options* options,
  void** out_state, mln_render_session* out_session,
  const mln_completion* completion, mln_status* out_status
) {
  *out_state = NULL;
  mln_webgpu_owned_texture_descriptor descriptor =
    mln_webgpu_owned_texture_descriptor_default();
  descriptor.extent.width = 64;
  descriptor.extent.height = 64;
  descriptor.extent.scale_factor = 1.0;
  descriptor.context = *attach_context;
  *out_status = mln_map_attach_webgpu_owned_texture(
    map, &descriptor, options, out_session, completion, MLN_TEST_DIAGNOSTIC
  );
  return true;
}

bool mln_test_webgpu_main_thread_fixture_create(
  mln_map map, const mln_test_webgpu_main_thread_device* device,
  mln_test_render_fixture* fixture
) {
  attach_context = &device->context;
  const bool attached = mln_test_render_fixture_create_serviced_on_main_thread(
    map, fixture, attach_main_thread_texture
  );
  attach_context = NULL;
  return attached;
}

static void on_buffer_mapped(
  WGPUMapAsyncStatus status, WGPUStringView message, void* user_data,
  void* unused
) {
  (void)message;
  (void)unused;
  *(WGPUMapAsyncStatus*)user_data = status;
}

bool mln_test_webgpu_read_ring_texture(
  const mln_test_render_fixture* fixture, size_t index, uint8_t* pixels,
  size_t size
) {
  const WGPUTexture texture = mln_test_webgpu_ring_texture(fixture, index);
  const size_t row_size = (size_t)ring_texture_size * 4U;
  const size_t image_size = row_size * ring_texture_size;
  if (texture == NULL || size < image_size) {
    return false;
  }
  const WGPUDevice device = thread_webgpu_state.device;
  const WGPUBufferDescriptor buffer_descriptor = {
    .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead,
    .size = image_size,
  };
  const WGPUBuffer buffer = wgpuDeviceCreateBuffer(device, &buffer_descriptor);
  if (buffer == NULL) {
    return false;
  }
  // The copy goes on the device's queue with no fence of its own, which is how
  // a host reads a frame whose producer writes the queue orders before it.
  const WGPUCommandEncoder encoder =
    wgpuDeviceCreateCommandEncoder(device, NULL);
  const WGPUTexelCopyTextureInfo source = {
    .texture = texture,
    .aspect = WGPUTextureAspect_All,
  };
  const WGPUTexelCopyBufferInfo destination = {
    .layout =
      {
        .bytesPerRow = (uint32_t)row_size,
        .rowsPerImage = ring_texture_size,
      },
    .buffer = buffer,
  };
  const WGPUExtent3D extent = {ring_texture_size, ring_texture_size, 1};
  wgpuCommandEncoderCopyTextureToBuffer(
    encoder, &source, &destination, &extent
  );
  const WGPUCommandBuffer commands = wgpuCommandEncoderFinish(encoder, NULL);
  wgpuCommandEncoderRelease(encoder);
  const WGPUQueue queue = wgpuDeviceGetQueue(device);
  wgpuQueueSubmit(queue, 1, &commands);
  wgpuQueueRelease(queue);
  wgpuCommandBufferRelease(commands);

  WGPUMapAsyncStatus status = WGPUMapAsyncStatus_Error;
  const WGPUBufferMapCallbackInfo map_info = {
    .mode = WGPUCallbackMode_AllowProcessEvents,
    .callback = on_buffer_mapped,
    .userdata1 = &status,
  };
  const bool mapped =
    await_future(
      thread_webgpu_state.instance,
      wgpuBufferMapAsync(buffer, WGPUMapMode_Read, 0, image_size, map_info)
    ) &&
    status == WGPUMapAsyncStatus_Success;
  const void* data =
    mapped ? wgpuBufferGetConstMappedRange(buffer, 0, image_size) : NULL;
  if (data != NULL) {
    memcpy(pixels, data, image_size);
    wgpuBufferUnmap(buffer);
  }
  wgpuBufferRelease(buffer);
  return data != NULL;
}

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
