
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "env.h"
#include "host_graphics.h"
#include "mln_test_graphics.h"
#include "render.h"
#include "render_backend.h"
#include "status.h"
#include "unity.h"

// Linux and FreeBSD windows are Wayland surfaces on a compositor in the test
// process, since tests/graphics surfaces there are headless.
#if (defined(__linux__) || defined(__FreeBSD__)) && !defined(__ANDROID__) && \
  !defined(__OHOS__)
#define WAYLAND_SURFACES 1
#include "wayland_compositor.h"
#endif

#if defined(MLN_FFI_TEST_BACKEND_VULKAN) && defined(WAYLAND_SURFACES)
#include <vulkan/vulkan_core.h>
#endif

#if defined(MLN_FFI_TEST_BACKEND_METAL)
#define HOST_BACKEND MLN_TEST_GRAPHICS_BACKEND_METAL
#elif defined(MLN_FFI_TEST_BACKEND_VULKAN)
#define HOST_BACKEND MLN_TEST_GRAPHICS_BACKEND_VULKAN
#elif defined(MLN_FFI_TEST_OPENGL_WGL)
#define HOST_BACKEND MLN_TEST_GRAPHICS_BACKEND_WGL
#define HOST_OPENGL 1
#else
#define HOST_BACKEND MLN_TEST_GRAPHICS_BACKEND_EGL
#define HOST_OPENGL 1
#endif

#define EXTRA_TARGETS 2

struct mln_test_render_surface {
  uint32_t width;
  uint32_t height;
#if defined(WAYLAND_SURFACES)
  void* wayland_surface;
#else
  mln_test_graphics_surface* graphics;
#endif
};

typedef struct host_state {
  mln_test_graphics* graphics;
  // False when the graphics object belongs to another fixture.
  bool owns_graphics;
  mln_test_graphics_context context;
  mln_test_graphics_texture* textures[MLN_TEXTURE_RING_MAX_DEPTH];
  size_t texture_count;
  mln_test_render_surface* surface;
  mln_test_graphics_texture* extra_textures[EXTRA_TARGETS];
  mln_test_render_surface* extra_surfaces[EXTRA_TARGETS];
#if defined(WAYLAND_SURFACES)
  // The compositor of the fixture's surfaces, started with the first one.
  mln_test_wayland* wayland;
#endif
  // The Metal command queue that the session's device wraps, or null.
  void* command_queue;
#if defined(MLN_FFI_TEST_BACKEND_VULKAN)
  // The owner that the session's ring attached with, which a replacement
  // keeps.
  mln_vulkan_image_owner vulkan_owner;
#endif
  bool keep_until_exit;
} host_state;

static void report(const char* what) {
  fprintf(stderr, "%s: %s\n", what, mln_test_graphics_last_error());
}

// Set only while a fixture attaches on a graphics object that it doesn't own.
static mln_test_graphics* shared_graphics = NULL;
// Set only while a fixture attaches on a device of the caller's, in place of
// one that wraps its graphics object.
static mln_gpu_device shared_device = MLN_HANDLE_NULL;
#if defined(MLN_FFI_TEST_BACKEND_VULKAN)
// Set only while mln_test_render_fixture_create_vulkan_handoff_ring()
// attaches.
static mln_vulkan_image_owner attach_vulkan_owner =
  MLN_VULKAN_IMAGE_OWNER_GRAPHICS_QUEUE;
#endif
// Set only while mln_test_render_fixture_create_synced() attaches. A Metal
// fixture that accepts queue order also names the graphics object's queue.
static mln_gpu_sync_kind_flags attach_sync_kinds = 0;

static mln_test_render_surface* surface_create(
  host_state* state, uint32_t width, uint32_t height
) {
  mln_test_render_surface* surface = calloc(1, sizeof(*surface));
  if (surface == NULL) {
    return NULL;
  }
  surface->width = width;
  surface->height = height;
#if defined(WAYLAND_SURFACES)
  if (state->wayland == NULL) {
    mln_test_wayland_start(&state->wayland);
  }
  surface->wayland_surface = mln_test_wayland_create_surface(state->wayland);
  if (surface->wayland_surface == NULL) {
    free(surface);
    return NULL;
  }
#else
  surface->graphics =
    mln_test_graphics_surface_create(state->graphics, width, height);
  if (surface->graphics == NULL) {
    free(surface);
    return NULL;
  }
#endif
  return surface;
}

static void surface_destroy(
  host_state* state, mln_test_render_surface* surface
) {
  if (surface == NULL) {
    return;
  }
#if defined(WAYLAND_SURFACES)
  mln_test_wayland_destroy_surface(state->wayland, surface->wayland_surface);
#else
  (void)state;
  mln_test_graphics_surface_destroy(surface->graphics);
#endif
  free(surface);
}

static void host_state_free(host_state* state) {
#if defined(WAYLAND_SURFACES)
  mln_test_wayland_stop(state->wayland);
#endif
  if (state->owns_graphics) {
    mln_test_graphics_destroy(state->graphics);
  }
  free(state);
}

// An OpenGL host drives its sessions from its own graphics thread with its
// context current, which is the thread that creates the fixture.
static host_state* host_state_create(void) {
  host_state* state = calloc(1, sizeof(*state));
  if (state == NULL) {
    return NULL;
  }
  state->owns_graphics = shared_graphics == NULL;
  state->graphics = state->owns_graphics
                      ? mln_test_graphics_create(HOST_BACKEND)
                      : shared_graphics;
  if (
    state->graphics == NULL ||
    !mln_test_graphics_get_context(state->graphics, &state->context)
  ) {
    report("the render fixture could not create a graphics context");
    host_state_free(state);
    return NULL;
  }
#if defined(HOST_OPENGL)
  if (!mln_test_graphics_make_current(state->graphics)) {
    report("the render fixture could not make its context current");
    host_state_free(state);
    return NULL;
  }
#endif
  return state;
}

void mln_test_backend_destroy(void* opaque_state) {
  host_state* state = opaque_state;
  if (state == NULL || state->keep_until_exit) {
    return;
  }
  for (size_t index = 0; index < EXTRA_TARGETS; index += 1) {
    surface_destroy(state, state->extra_surfaces[index]);
    mln_test_graphics_texture_destroy(state->extra_textures[index]);
  }
  surface_destroy(state, state->surface);
  for (size_t index = 0; index < state->texture_count; index += 1) {
    mln_test_graphics_texture_destroy(state->textures[index]);
  }
  host_state_free(state);
}

uint32_t mln_test_backend_driver(void) {
#if defined(HOST_OPENGL)
  return MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD;
#else
  return MLN_RENDER_DRIVER_CORE_WORKER;
#endif
}

void mln_test_release_thread_gpu_resources(void) {}

#if defined(MLN_FFI_TEST_BACKEND_VULKAN)
// Set only while one of the mln_test_render_fixture_create_vulkan_*()
// functions attaches, to replace the context's PFN_vkGetDeviceProcAddr and to
// give the device a host queue lock.
static mln_test_vulkan_device_proc_addr_wrap wrap_device_proc_addr = NULL;
static const mln_queue_lock* attach_queue_lock = NULL;
#endif

#if defined(MLN_FFI_TEST_BACKEND_VULKAN)
// The device extensions that the fixture's device enabled, as a wrap lists
// them.
static size_t vulkan_extension_views(
  const mln_test_graphics_context* context, mln_buffer_view out[8]
) {
  size_t count = 0;
  for (; count < context->vulkan_device_extension_count && count < 8; ++count) {
    out[count] =
      mln_buffer_view_from_cstr(context->vulkan_device_extensions[count]);
  }
  return count;
}
#endif

// Wraps the device or context of the graphics object with `context` as a
// device for the fixture's session, or returns the null handle.
static mln_gpu_device host_device(
  const mln_test_graphics_context* context, host_state* state
) {
  mln_gpu_device device = MLN_HANDLE_NULL;
  mln_status status = MLN_STATUS_UNSUPPORTED;
#if defined(MLN_FFI_TEST_BACKEND_METAL)
  const mln_metal_device_descriptor descriptor = {
    .size = sizeof(mln_metal_device_descriptor),
    .device = context->metal_device,
    .command_queue = state->command_queue,
  };
  status = mln_gpu_device_wrap_metal(&descriptor, &device, MLN_TEST_DIAGNOSTIC);
#elif defined(MLN_FFI_TEST_BACKEND_VULKAN)
  (void)state;
  mln_buffer_view extensions[8];
  const size_t extension_count = vulkan_extension_views(context, extensions);
  mln_vulkan_device_descriptor descriptor = {
    .size = sizeof(mln_vulkan_device_descriptor),
    .context =
      {
        .instance = context->vulkan_instance,
        .physical_device = context->vulkan_physical_device,
        .device = context->vulkan_device,
        .graphics_queue = context->vulkan_queue,
        .graphics_queue_family_index = context->vulkan_queue_family_index,
        .get_instance_proc_addr = context->vulkan_get_instance_proc_addr,
        .get_device_proc_addr =
          wrap_device_proc_addr == NULL
            ? context->vulkan_get_device_proc_addr
            : wrap_device_proc_addr(context->vulkan_get_device_proc_addr),
      },
    .enabled_extensions = extensions,
    .enabled_extension_count = extension_count,
  };
  if (attach_queue_lock != NULL) descriptor.queue_lock = *attach_queue_lock;
  status =
    mln_gpu_device_wrap_vulkan(&descriptor, &device, MLN_TEST_DIAGNOSTIC);
#elif defined(MLN_FFI_TEST_OPENGL_WGL)
  (void)state;
  const mln_open_gl_context_descriptor descriptor = {
    .platform = MLN_OPEN_GL_CONTEXT_PLATFORM_WGL,
    .data = {
      .wgl = {
        .device_context = context->wgl_device_context,
        .share_context = context->wgl_context,
        .get_proc_address = context->get_proc_address,
      }
    },
  };
  status =
    mln_gpu_device_wrap_open_gl(&descriptor, &device, MLN_TEST_DIAGNOSTIC);
#else
  (void)state;
  const mln_open_gl_context_descriptor descriptor = {
    .platform = MLN_OPEN_GL_CONTEXT_PLATFORM_EGL,
    .data = {
      .egl = {
        .display = context->egl_display,
        .config = context->egl_config,
        .share_context = context->egl_context,
        .get_proc_address = context->get_proc_address,
      }
    },
  };
  status =
    mln_gpu_device_wrap_open_gl(&descriptor, &device, MLN_TEST_DIAGNOSTIC);
#endif
  if (status != MLN_STATUS_OK) {
    fprintf(
      stderr, "the render fixture could not wrap its device: %s\n",
      mln_test_last_error()
    );
    return MLN_HANDLE_NULL;
  }
  return device;
}

#if defined(MLN_FFI_TEST_BACKEND_METAL)
typedef mln_metal_borrowed_texture borrowed_entry;
#elif defined(MLN_FFI_TEST_BACKEND_VULKAN)
typedef mln_vulkan_borrowed_texture borrowed_entry;
#else
typedef mln_open_gl_borrowed_texture borrowed_entry;
#endif

// Describes `textures` as a borrowed ring target of their size, for the
// fixture with `state`. The target's array points at `entries`, which has
// room for `count` entries.
static bool describe_ring(
  const host_state* state, mln_test_graphics_texture* const* textures,
  size_t count, borrowed_entry* entries, mln_render_target* out
) {
  mln_test_graphics_texture_info info = {0};
  for (size_t index = 0; index < count; index += 1) {
    if (!mln_test_graphics_texture_get_info(textures[index], &info)) {
      report("the render fixture could not describe a texture");
      return false;
    }
#if defined(MLN_FFI_TEST_BACKEND_METAL)
    entries[index] = (borrowed_entry){.texture = info.metal_texture};
#elif defined(MLN_FFI_TEST_BACKEND_VULKAN)
    entries[index] = (borrowed_entry){
      .image = info.vulkan_image,
      .image_view = info.vulkan_image_view,
    };
#else
    entries[index] = (borrowed_entry){.texture = info.opengl_texture};
#endif
  }
  // Every texture of the ring shares the last one's format and size.
  const mln_logical_size logical_size = {
    .width = info.width, .height = info.height
  };
  *out = (mln_render_target){.size = sizeof(mln_render_target)};
#if defined(MLN_FFI_TEST_BACKEND_METAL)
  (void)state;
  out->kind = MLN_RENDER_TARGET_KIND_METAL_BORROWED_TEXTURES;
  out->data.metal_borrowed_textures = (mln_metal_borrowed_textures_target){
    .logical_size = logical_size,
    .physical_width = info.width,
    .physical_height = info.height,
    .textures = entries,
    .texture_count = count,
  };
#elif defined(MLN_FFI_TEST_BACKEND_VULKAN)
  out->kind = MLN_RENDER_TARGET_KIND_VULKAN_BORROWED_TEXTURES;
  out->data.vulkan_borrowed_textures = (mln_vulkan_borrowed_textures_target){
    .logical_size = logical_size,
    .physical_width = info.width,
    .physical_height = info.height,
    .textures = entries,
    .texture_count = count,
    .format = info.format,
    .final_layout = info.vulkan_final_layout,
    .owner = state->vulkan_owner,
  };
#else
  (void)state;
  out->kind = MLN_RENDER_TARGET_KIND_OPEN_GL_BORROWED_TEXTURES;
  out->data.open_gl_borrowed_textures = (mln_open_gl_borrowed_textures_target){
    .logical_size = logical_size,
    .physical_width = info.width,
    .physical_height = info.height,
    .textures = entries,
    .texture_count = count,
    .target = info.opengl_target,
  };
#endif
  return true;
}

// Describes `surface`, which the fixture with `state` created, as a target of
// the surface's size.
static bool describe_surface(
  const host_state* state, const mln_test_render_surface* surface,
  mln_render_target* out
) {
  *out = (mln_render_target){.size = sizeof(mln_render_target)};
  const mln_logical_size logical_size = {
    .width = surface->width, .height = surface->height
  };
#if defined(WAYLAND_SURFACES)
  out->kind = MLN_RENDER_TARGET_KIND_WAYLAND_SURFACE;
  out->data.wayland_surface = (mln_wayland_surface_target){
    .logical_size = logical_size,
    .display = mln_test_wayland_display(state->wayland),
    .surface = surface->wayland_surface,
  };
  return true;
#else
  (void)state;
  mln_test_graphics_surface_info info = {0};
  if (!mln_test_graphics_surface_get_info(surface->graphics, &info)) {
    report("the render fixture could not describe a surface");
    return false;
  }
  if (info.metal_layer != NULL) {
    out->kind = MLN_RENDER_TARGET_KIND_METAL_LAYER;
    out->data.metal_layer = (mln_metal_layer_target){
      .logical_size = logical_size, .layer = info.metal_layer
    };
  } else if (info.win32_window != NULL) {
    out->kind = MLN_RENDER_TARGET_KIND_WIN32_WINDOW;
    out->data.win32_window = (mln_win32_window_target){
      .logical_size = logical_size, .window = info.win32_window
    };
  } else if (info.android_window != NULL) {
    out->kind = MLN_RENDER_TARGET_KIND_ANDROID_WINDOW;
    out->data.android_window = (mln_android_window_target){
      .logical_size = logical_size, .window = info.android_window
    };
  } else {
    report("the render fixture's surface has no window or layer");
    return false;
  }
  return true;
#endif
}

#if defined(MLN_FFI_TEST_BACKEND_VULKAN) && defined(WAYLAND_SURFACES)
// Finds the CPU's Vulkan device, such as lavapipe, through the instance of a
// tests/graphics device.
static bool find_cpu_device(uint8_t out_uuid[VK_UUID_SIZE]) {
  mln_test_graphics* graphics =
    mln_test_graphics_create(MLN_TEST_GRAPHICS_BACKEND_VULKAN);
  mln_test_graphics_context context = {0};
  TEST_ASSERT_NOT_NULL_MESSAGE(graphics, mln_test_graphics_last_error());
  TEST_ASSERT_TRUE(mln_test_graphics_get_context(graphics, &context));
  const PFN_vkGetInstanceProcAddr get_instance_proc_addr =
    (PFN_vkGetInstanceProcAddr)(uintptr_t)context.vulkan_get_instance_proc_addr;
  VkInstance instance = (VkInstance)context.vulkan_instance;
  const PFN_vkEnumeratePhysicalDevices enumerate =
    (PFN_vkEnumeratePhysicalDevices)get_instance_proc_addr(
      instance, "vkEnumeratePhysicalDevices"
    );
  const PFN_vkGetPhysicalDeviceProperties2 get_properties =
    (PFN_vkGetPhysicalDeviceProperties2)get_instance_proc_addr(
      instance, "vkGetPhysicalDeviceProperties2"
    );
  VkPhysicalDevice devices[16];
  uint32_t count = 16;
  const VkResult listed = enumerate(instance, &count, devices);
  bool found = false;
  for (uint32_t index = 0; index < count && !found; ++index) {
    VkPhysicalDeviceIDProperties ids = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES,
    };
    VkPhysicalDeviceProperties2 properties = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
      .pNext = &ids,
    };
    get_properties(devices[index], &properties);
    if (properties.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU) {
      memcpy(out_uuid, ids.deviceUUID, VK_UUID_SIZE);
      found = true;
    }
  }
  mln_test_graphics_destroy(graphics);
  TEST_ASSERT_TRUE(listed == VK_SUCCESS || listed == VK_INCOMPLETE);
  TEST_ASSERT_TRUE_MESSAGE(found, "no CPU Vulkan device, such as lavapipe");
  return found;
}
#endif

mln_gpu_device mln_test_create_presenting_device(void) {
  mln_gpu_device_options options = {.size = sizeof(options)};
  options.fields = MLN_GPU_DEVICE_OPTION_BACKEND;
#if defined(MLN_FFI_TEST_BACKEND_METAL)
  options.backend = MLN_RENDER_BACKEND_METAL;
#elif defined(MLN_FFI_TEST_BACKEND_VULKAN)
  options.backend = MLN_RENDER_BACKEND_VULKAN;
#if defined(WAYLAND_SURFACES)
  static uint8_t uuid[VK_UUID_SIZE];
  if (find_cpu_device(uuid)) {
    options.fields |= MLN_GPU_DEVICE_OPTION_ADAPTER_UUID;
    options.adapter_uuid =
      (mln_buffer_view){.data = uuid, .size = sizeof(uuid)};
  }
#endif
#else
  options.backend = MLN_RENDER_BACKEND_OPEN_GL;
#endif
  mln_gpu_device device = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_gpu_device_create(&options, &device, MLN_TEST_DIAGNOSTIC));
  return device;
}

bool mln_test_backend_attach_borrowed(
  mln_map map, const mln_render_session_attach_options* options,
  void** out_state, mln_render_session* out_session,
  const mln_completion* completion, mln_status* out_status
) {
  const size_t depth = mln_test_render_ring_depth();
  host_state* state = host_state_create();
  if (state == NULL) {
    return false;
  }
  for (size_t index = 0; index < depth; index += 1) {
    state->textures[index] = mln_test_graphics_texture_create(
      state->graphics, MLN_TEST_HOST_TARGET_SIZE, MLN_TEST_HOST_TARGET_SIZE
    );
    state->texture_count += state->textures[index] != NULL;
  }
#if defined(MLN_FFI_TEST_BACKEND_VULKAN)
  state->vulkan_owner = attach_vulkan_owner;
#endif
#if defined(MLN_FFI_TEST_BACKEND_METAL)
  if (attach_sync_kinds & MLN_GPU_SYNC_KIND_FLAG_QUEUE_ORDER)
    state->command_queue =
      mln_test_graphics_metal_command_queue(state->graphics);
#endif
  borrowed_entry entries[MLN_TEXTURE_RING_MAX_DEPTH];
  mln_render_target target;
  if (
    state->texture_count != depth ||
    !describe_ring(
      state, state->textures, state->texture_count, entries, &target
    )
  ) {
    report("the render fixture could not create a borrowed texture");
    mln_test_backend_destroy(state);
    return false;
  }
  const mln_gpu_device device = shared_device != MLN_HANDLE_NULL
                                  ? shared_device
                                  : host_device(&state->context, state);
  if (device == MLN_HANDLE_NULL) {
    mln_test_backend_destroy(state);
    return false;
  }
  *out_state = state;
  mln_render_session_attach_options attach = *options;
  attach.producer_sync_kinds |= attach_sync_kinds;
  *out_status = mln_map_attach(
    map, &target, device, &attach, out_session, completion, MLN_TEST_DIAGNOSTIC
  );
  // The session holds the device.
  if (device != shared_device) mln_gpu_device_release(device);
  return true;
}

// The session holds the device, so the fixture releases it once the attach
// call returns.
static bool attach_surface(
  mln_map map, const mln_render_session_attach_options* options,
  void** out_state, mln_render_session* out_session,
  const mln_completion* completion, mln_status* out_status
) {
  host_state* state = host_state_create();
  if (state == NULL) {
    return false;
  }
  state->surface =
    surface_create(state, MLN_TEST_HOST_TARGET_SIZE, MLN_TEST_HOST_TARGET_SIZE);
  mln_render_target target;
  if (
    state->surface == NULL || !describe_surface(state, state->surface, &target)
  ) {
    report("the render fixture could not create a surface");
    mln_test_backend_destroy(state);
    return false;
  }
  *out_state = state;
  const mln_gpu_device device = mln_test_create_presenting_device();
  *out_status = mln_map_attach(
    map, &target, device, options, out_session, completion, MLN_TEST_DIAGNOSTIC
  );
  mln_gpu_device_release(device);
  return true;
}

bool mln_test_render_fixture_create_synced(
  mln_map map, mln_test_render_fixture* fixture,
  mln_gpu_sync_kind_flags producer_sync_kinds
) {
  attach_sync_kinds = producer_sync_kinds;
  const bool attached = mln_test_render_fixture_create_borrowed_ring(
    map, fixture, 1, MLN_TEST_PRESET_DRIVER
  );
  attach_sync_kinds = 0;
  return attached;
}

#if defined(MLN_FFI_TEST_BACKEND_VULKAN)
bool mln_test_render_fixture_create_vulkan_borrowed_texture(
  mln_map map, mln_test_render_fixture* fixture, size_t depth,
  mln_test_vulkan_device_proc_addr_wrap wrap,
  const mln_test_render_fixture* share, const mln_queue_lock* queue_lock
) {
  wrap_device_proc_addr = wrap;
  attach_queue_lock = queue_lock;
  shared_graphics =
    share == NULL ? NULL : mln_test_render_fixture_graphics(share);
  const bool attached = mln_test_render_fixture_create_borrowed_ring(
    map, fixture, depth, MLN_TEST_PRESET_DRIVER
  );
  wrap_device_proc_addr = NULL;
  attach_queue_lock = NULL;
  shared_graphics = NULL;
  return attached;
}

bool mln_test_render_fixture_create_vulkan_handoff_ring(
  mln_map map, mln_test_render_fixture* fixture, mln_vulkan_image_owner owner
) {
  attach_vulkan_owner = owner;
  attach_sync_kinds = MLN_GPU_SYNC_KIND_FLAG_CPU_COMPLETE |
                      MLN_GPU_SYNC_KIND_FLAG_VULKAN_TIMELINE_SEMAPHORE;
  const bool attached = mln_test_render_fixture_create_borrowed_ring(
    map, fixture, 1, MLN_TEST_PRESET_DRIVER
  );
  attach_vulkan_owner = MLN_VULKAN_IMAGE_OWNER_GRAPHICS_QUEUE;
  attach_sync_kinds = 0;
  return attached;
}

bool mln_test_render_fixture_create_vulkan_on(
  mln_map map, mln_test_render_fixture* fixture, mln_test_graphics* graphics,
  mln_gpu_device device
) {
  shared_graphics = graphics;
  shared_device = device;
  const bool attached = mln_test_render_fixture_create_borrowed_ring(
    map, fixture, 1, MLN_TEST_PRESET_DRIVER
  );
  shared_graphics = NULL;
  shared_device = MLN_HANDLE_NULL;
  return attached;
}
#endif

void mln_test_render_fixture_destroy_textures(
  const mln_test_render_fixture* fixture
) {
  host_state* state = fixture->backend_state;
  if (state == NULL) {
    return;
  }
  for (size_t index = 0; index < state->texture_count; index += 1) {
    mln_test_graphics_texture_destroy(state->textures[index]);
    state->textures[index] = NULL;
  }
  state->texture_count = 0;
}

mln_status mln_test_render_fixture_start_surface_attach(
  mln_map map, const mln_render_session_attach_options* options,
  const mln_completion* completion, mln_test_render_fixture* fixture
) {
  return mln_test_render_fixture_start_attach_with(
    map, options, completion, fixture, attach_surface,
    MLN_RENDER_DRIVER_CORE_WORKER
  );
}

bool mln_test_render_fixture_create_surface(
  mln_map map, mln_test_render_fixture* fixture
) {
  return mln_test_render_fixture_create_on(
    map, fixture, attach_surface, MLN_RENDER_DRIVER_CORE_WORKER
  );
}

bool mln_test_render_fixture_read_texture(
  const mln_test_render_fixture* fixture, uint8_t* pixels, size_t size
) {
  mln_test_graphics_texture* texture =
    mln_test_render_fixture_texture(fixture, 0);
  if (texture == NULL) {
    return false;
  }
  if (!mln_test_graphics_texture_read_rgba8(texture, pixels, size)) {
    report("the render fixture could not read its borrowed texture");
    return false;
  }
  return true;
}

mln_test_graphics_texture* mln_test_render_fixture_texture(
  const mln_test_render_fixture* fixture, size_t index
) {
  const host_state* state = fixture->backend_state;
  if (state == NULL || index >= state->texture_count) {
    return NULL;
  }
  return state->textures[index];
}

uint64_t mln_test_texture_handle(const mln_test_graphics_texture* texture) {
  mln_test_graphics_texture_info info = {0};
  if (!mln_test_graphics_texture_get_info(texture, &info)) {
    report("the render fixture could not describe a texture");
    return 0;
  }
#if defined(MLN_FFI_TEST_BACKEND_METAL)
  return (uint64_t)(uintptr_t)info.metal_texture;
#elif defined(MLN_FFI_TEST_BACKEND_VULKAN)
  return info.vulkan_image;
#else
  return info.opengl_texture;
#endif
}

uint64_t mln_test_frame_texture_handle(
  mln_acquired_frame frame, uint32_t* out_slot
) {
#if defined(MLN_FFI_TEST_BACKEND_METAL)
  mln_metal_texture_frame record = {.size = sizeof(record)};
  if (
    mln_acquired_frame_get_metal_texture(frame, &record, NULL) != MLN_STATUS_OK
  ) {
    return 0;
  }
  *out_slot = record.slot;
  return (uint64_t)(uintptr_t)record.texture;
#elif defined(MLN_FFI_TEST_BACKEND_VULKAN)
  mln_vulkan_texture_frame record = {.size = sizeof(record)};
  if (
    mln_acquired_frame_get_vulkan_texture(frame, &record, NULL) != MLN_STATUS_OK
  ) {
    return 0;
  }
  *out_slot = record.slot;
  return record.image;
#else
  mln_open_gl_texture_frame record = {.size = sizeof(record)};
  if (
    mln_acquired_frame_get_open_gl_texture(frame, &record, NULL) !=
    MLN_STATUS_OK
  ) {
    return 0;
  }
  *out_slot = record.slot;
  return record.texture;
#endif
}

void mln_test_render_fixture_keep_graphics_until_exit(
  const mln_test_render_fixture* fixture
) {
  host_state* state = fixture->backend_state;
  if (state != NULL) {
    state->keep_until_exit = true;
  }
}

mln_test_graphics* mln_test_render_fixture_graphics(
  const mln_test_render_fixture* fixture
) {
  const host_state* state = fixture->backend_state;
  return state == NULL ? NULL : state->graphics;
}

mln_test_graphics_texture* mln_test_render_fixture_new_texture(
  const mln_test_render_fixture* fixture
) {
  return mln_test_render_fixture_new_sized_texture(
    fixture, MLN_TEST_HOST_TARGET_SIZE, MLN_TEST_HOST_TARGET_SIZE
  );
}

mln_test_graphics_texture* mln_test_render_fixture_new_sized_texture(
  const mln_test_render_fixture* fixture, uint32_t width, uint32_t height
) {
  host_state* state = fixture->backend_state;
  for (size_t index = 0; state != NULL && index < EXTRA_TARGETS; index += 1) {
    if (state->extra_textures[index] == NULL) {
      state->extra_textures[index] =
        mln_test_graphics_texture_create(state->graphics, width, height);
      if (state->extra_textures[index] == NULL) {
        report("the render fixture could not create another texture");
      }
      return state->extra_textures[index];
    }
  }
  return NULL;
}

mln_test_render_surface* mln_test_render_fixture_new_surface(
  const mln_test_render_fixture* fixture
) {
  return mln_test_render_fixture_new_sized_surface(
    fixture, MLN_TEST_HOST_TARGET_SIZE, MLN_TEST_HOST_TARGET_SIZE
  );
}

mln_test_render_surface* mln_test_render_fixture_new_sized_surface(
  const mln_test_render_fixture* fixture, uint32_t width, uint32_t height
) {
  host_state* state = fixture->backend_state;
  for (size_t index = 0; state != NULL && index < EXTRA_TARGETS; index += 1) {
    if (state->extra_surfaces[index] == NULL) {
      state->extra_surfaces[index] = surface_create(state, width, height);
      if (state->extra_surfaces[index] == NULL) {
        report("the render fixture could not create another surface");
      }
      return state->extra_surfaces[index];
    }
  }
  return NULL;
}

mln_test_render_surface* mln_test_render_surface_create(
  const mln_test_render_fixture* fixture
) {
  host_state* state = fixture->backend_state;
  if (state == NULL) {
    return NULL;
  }
  mln_test_render_surface* surface =
    surface_create(state, MLN_TEST_HOST_TARGET_SIZE, MLN_TEST_HOST_TARGET_SIZE);
  if (surface == NULL) {
    report("the render fixture could not create a surface");
  }
  return surface;
}

void mln_test_render_surface_destroy(
  const mln_test_render_fixture* fixture, mln_test_render_surface* surface
) {
  host_state* state = fixture->backend_state;
  if (state != NULL) {
    surface_destroy(state, surface);
  }
}

#if defined(MLN_FFI_TEST_BACKEND_METAL)
void* mln_test_render_surface_metal_layer(
  const mln_test_render_surface* surface
) {
#if defined(WAYLAND_SURFACES)
  (void)surface;
  return NULL;
#else
  mln_test_graphics_surface_info info = {0};
  if (!mln_test_graphics_surface_get_info(surface->graphics, &info)) {
    return NULL;
  }
  return info.metal_layer;
#endif
}
#endif

mln_status mln_test_render_fixture_set_textures(
  const mln_test_render_fixture* fixture,
  mln_test_graphics_texture* const* textures, size_t count,
  const mln_completion* completion
) {
  // A fixture without a ring, such as an offscreen one, describes the
  // textures with the defaults.
  const host_state defaults = {0};
  const host_state* state =
    fixture->backend_state != NULL ? fixture->backend_state : &defaults;
  borrowed_entry entries[MLN_TEXTURE_RING_MAX_DEPTH];
  mln_render_target target;
  if (
    count > MLN_TEXTURE_RING_MAX_DEPTH ||
    !describe_ring(state, textures, count, entries, &target)
  ) {
    return MLN_STATUS_NATIVE_ERROR;
  }
  return mln_render_session_set_target(
    fixture->session, &target, completion, MLN_TEST_DIAGNOSTIC
  );
}

mln_status mln_test_render_fixture_set_surface(
  const mln_test_render_fixture* fixture,
  const mln_test_render_surface* surface, const mln_completion* completion
) {
  mln_render_target target;
  if (!describe_surface(fixture->backend_state, surface, &target)) {
    return MLN_STATUS_NATIVE_ERROR;
  }
  return mln_render_session_set_target(
    fixture->session, &target, completion, MLN_TEST_DIAGNOSTIC
  );
}
