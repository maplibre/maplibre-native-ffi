// Host graphics for the render fixtures on every native backend. The context,
// and any borrowed texture or surface, come from tests/graphics, which the
// binding suites load for the same purpose.

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "env.h"
#include "host_graphics.h"
#include "mln_test_graphics.h"
#include "render.h"
#include "render_backend.h"

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

// How many more targets a case can make on one fixture's graphics object.
#define EXTRA_TARGETS 2

typedef struct host_state {
  mln_test_graphics* graphics;
  // False when the graphics object belongs to another fixture.
  bool owns_graphics;
  mln_test_graphics_context context;
  mln_test_graphics_texture* texture;
  mln_test_graphics_surface* surface;
  mln_test_graphics_texture* extra_textures[EXTRA_TARGETS];
  mln_test_graphics_surface* extra_surfaces[EXTRA_TARGETS];
} host_state;

static void report(const char* what) {
  fprintf(stderr, "%s: %s\n", what, mln_test_graphics_last_error());
}

// Set only while mln_test_render_fixture_create_vulkan_borrowed_texture()
// attaches a fixture on the graphics object of another.
static mln_test_graphics* shared_graphics = NULL;

static void host_state_free(host_state* state) {
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
  if (state == NULL) {
    return;
  }
  for (size_t index = 0; index < EXTRA_TARGETS; index += 1) {
    mln_test_graphics_surface_destroy(state->extra_surfaces[index]);
    mln_test_graphics_texture_destroy(state->extra_textures[index]);
  }
  mln_test_graphics_surface_destroy(state->surface);
  mln_test_graphics_texture_destroy(state->texture);
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

static mln_logical_extent host_extent(void) {
  return (mln_logical_extent){
    .width = MLN_TEST_HOST_TARGET_SIZE,
    .height = MLN_TEST_HOST_TARGET_SIZE,
    .scale_factor = 1.0,
  };
}

#if defined(MLN_FFI_TEST_BACKEND_METAL)
static mln_metal_context_descriptor host_context(
  const mln_test_graphics_context* context
) {
  return (mln_metal_context_descriptor){
    .device = context->metal_device,
  };
}
#elif defined(MLN_FFI_TEST_BACKEND_VULKAN)
// Set only while mln_test_render_fixture_create_vulkan_borrowed_texture() or
// mln_test_render_fixture_create_vulkan_owned_texture() attaches, to replace
// the context's PFN_vkGetDeviceProcAddr and to give the session a host queue
// lock.
static mln_test_vulkan_device_proc_addr_wrap wrap_device_proc_addr = NULL;
static const mln_queue_lock* attach_queue_lock = NULL;

static mln_vulkan_context_descriptor host_context(
  const mln_test_graphics_context* context
) {
  return (mln_vulkan_context_descriptor){
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
  };
}
#elif defined(MLN_FFI_TEST_OPENGL_WGL)
static mln_opengl_context_descriptor host_context(
  const mln_test_graphics_context* context
) {
  return (mln_opengl_context_descriptor){
    .platform = MLN_OPENGL_CONTEXT_PLATFORM_WGL,
    .data = {
      .wgl = {
        .device_context = context->wgl_device_context,
        .share_context = context->wgl_context,
        .get_proc_address = context->get_proc_address,
      }
    },
  };
}
#else
static mln_opengl_context_descriptor host_context(
  const mln_test_graphics_context* context
) {
  return (mln_opengl_context_descriptor){
    .platform = MLN_OPENGL_CONTEXT_PLATFORM_EGL,
    .data = {
      .egl = {
        .display = context->egl_display,
        .config = context->egl_config,
        .share_context = context->egl_context,
      }
    },
  };
}
#endif

bool mln_test_backend_attach(
  mln_map map, const mln_render_session_attach_options* options,
  void** out_state, mln_render_session* out_session,
  const mln_completion* completion, mln_status* out_status
) {
  host_state* state = host_state_create();
  if (state == NULL) {
    return false;
  }
  *out_state = state;
#if defined(MLN_FFI_TEST_BACKEND_METAL)
  mln_metal_owned_texture_descriptor descriptor =
    mln_metal_owned_texture_descriptor_default();
  descriptor.extent = host_extent();
  descriptor.context = host_context(&state->context);
  *out_status = mln_map_attach_metal_owned_texture(
    map, &descriptor, options, out_session, completion, MLN_TEST_DIAGNOSTIC
  );
#elif defined(MLN_FFI_TEST_BACKEND_VULKAN)
  mln_vulkan_owned_texture_descriptor descriptor =
    mln_vulkan_owned_texture_descriptor_default();
  descriptor.extent = host_extent();
  descriptor.context = host_context(&state->context);
  mln_render_session_attach_options attach = *options;
  if (attach_queue_lock != NULL) {
    attach.queue_lock = *attach_queue_lock;
  }
  *out_status = mln_map_attach_vulkan_owned_texture(
    map, &descriptor, &attach, out_session, completion, MLN_TEST_DIAGNOSTIC
  );
#else
  mln_opengl_owned_texture_descriptor descriptor =
    mln_opengl_owned_texture_descriptor_default();
  descriptor.extent = host_extent();
  descriptor.context = host_context(&state->context);
  *out_status = mln_map_attach_opengl_owned_texture(
    map, &descriptor, options, out_session, completion, MLN_TEST_DIAGNOSTIC
  );
#endif
  return true;
}

#if defined(MLN_FFI_TEST_BACKEND_METAL)
typedef mln_metal_borrowed_texture_descriptor borrowed_descriptor;
typedef mln_metal_surface_descriptor surface_descriptor;
#elif defined(MLN_FFI_TEST_BACKEND_VULKAN)
typedef mln_vulkan_borrowed_texture_descriptor borrowed_descriptor;
typedef mln_vulkan_surface_descriptor surface_descriptor;
#else
typedef mln_opengl_borrowed_texture_descriptor borrowed_descriptor;
typedef mln_opengl_surface_descriptor surface_descriptor;
#endif

// Describes `texture`, which the graphics object with `context` created, as a
// borrowed target of the host size.
static bool describe_texture(
  const mln_test_graphics_context* context,
  const mln_test_graphics_texture* texture, borrowed_descriptor* out
) {
  mln_test_graphics_texture_info info = {0};
  if (!mln_test_graphics_texture_get_info(texture, &info)) {
    report("the render fixture could not describe a texture");
    return false;
  }
#if defined(MLN_FFI_TEST_BACKEND_METAL)
  (void)context;
  *out = mln_metal_borrowed_texture_descriptor_default();
  out->texture = info.metal_texture;
#elif defined(MLN_FFI_TEST_BACKEND_VULKAN)
  *out = mln_vulkan_borrowed_texture_descriptor_default();
  out->context = host_context(context);
  out->image = info.vulkan_image;
  out->image_view = info.vulkan_image_view;
  out->format = info.format;
  out->initial_layout = info.vulkan_initial_layout;
  out->final_layout = info.vulkan_final_layout;
#else
  *out = mln_opengl_borrowed_texture_descriptor_default();
  out->context = host_context(context);
  out->texture = info.opengl_texture;
  out->target = info.opengl_target;
#endif
  // Physical and logical sizes agree at a scale factor of 1.
  out->extent = host_extent();
  out->physical_width = info.width;
  out->physical_height = info.height;
  return true;
}

static bool describe_surface(
  const mln_test_graphics_context* context,
  const mln_test_graphics_surface* surface, surface_descriptor* out
) {
  mln_test_graphics_surface_info info = {0};
  if (!mln_test_graphics_surface_get_info(surface, &info)) {
    report("the render fixture could not describe a surface");
    return false;
  }
#if defined(MLN_FFI_TEST_BACKEND_METAL)
  *out = mln_metal_surface_descriptor_default();
  out->layer = info.metal_layer;
#elif defined(MLN_FFI_TEST_BACKEND_VULKAN)
  *out = mln_vulkan_surface_descriptor_default();
  out->surface = info.vulkan_surface;
#else
  *out = mln_opengl_surface_descriptor_default();
  out->surface = info.opengl_surface;
#endif
  out->extent = host_extent();
  out->context = host_context(context);
  return true;
}

static bool attach_borrowed_texture(
  mln_map map, const mln_render_session_attach_options* options,
  void** out_state, mln_render_session* out_session,
  const mln_completion* completion, mln_status* out_status
) {
  host_state* state = host_state_create();
  if (state == NULL) {
    return false;
  }
  state->texture = mln_test_graphics_texture_create(
    state->graphics, MLN_TEST_HOST_TARGET_SIZE, MLN_TEST_HOST_TARGET_SIZE
  );
  borrowed_descriptor descriptor;
  if (
    state->texture == NULL ||
    !describe_texture(&state->context, state->texture, &descriptor)
  ) {
    report("the render fixture could not create a borrowed texture");
    mln_test_backend_destroy(state);
    return false;
  }
  *out_state = state;
#if defined(MLN_FFI_TEST_BACKEND_METAL)
  *out_status = mln_map_attach_metal_borrowed_texture(
    map, &descriptor, options, out_session, completion, MLN_TEST_DIAGNOSTIC
  );
#elif defined(MLN_FFI_TEST_BACKEND_VULKAN)
  mln_render_session_attach_options attach = *options;
  if (attach_queue_lock != NULL) {
    attach.queue_lock = *attach_queue_lock;
  }
  *out_status = mln_map_attach_vulkan_borrowed_texture(
    map, &descriptor, &attach, out_session, completion, MLN_TEST_DIAGNOSTIC
  );
#else
  *out_status = mln_map_attach_opengl_borrowed_texture(
    map, &descriptor, options, out_session, completion, MLN_TEST_DIAGNOSTIC
  );
#endif
  return true;
}

static bool attach_surface(
  mln_map map, const mln_render_session_attach_options* options,
  void** out_state, mln_render_session* out_session,
  const mln_completion* completion, mln_status* out_status
) {
  host_state* state = host_state_create();
  if (state == NULL) {
    return false;
  }
  state->surface = mln_test_graphics_surface_create(
    state->graphics, MLN_TEST_HOST_TARGET_SIZE, MLN_TEST_HOST_TARGET_SIZE
  );
  surface_descriptor descriptor;
  if (
    state->surface == NULL ||
    !describe_surface(&state->context, state->surface, &descriptor)
  ) {
    report("the render fixture could not create a surface");
    mln_test_backend_destroy(state);
    return false;
  }
  *out_state = state;
#if defined(MLN_FFI_TEST_BACKEND_METAL)
  *out_status = mln_map_attach_metal_surface(
    map, &descriptor, options, out_session, completion, MLN_TEST_DIAGNOSTIC
  );
#elif defined(MLN_FFI_TEST_BACKEND_VULKAN)
  *out_status = mln_map_attach_vulkan_surface(
    map, &descriptor, options, out_session, completion, MLN_TEST_DIAGNOSTIC
  );
#else
  *out_status = mln_map_attach_opengl_surface(
    map, &descriptor, options, out_session, completion, MLN_TEST_DIAGNOSTIC
  );
#endif
  return true;
}

bool mln_test_render_fixture_create_borrowed_texture(
  mln_map map, mln_test_render_fixture* fixture
) {
  return mln_test_render_fixture_create_with(
    map, fixture, attach_borrowed_texture
  );
}

#if defined(MLN_FFI_TEST_BACKEND_VULKAN)
bool mln_test_render_fixture_create_vulkan_borrowed_texture(
  mln_map map, mln_test_render_fixture* fixture,
  mln_test_vulkan_device_proc_addr_wrap wrap,
  const mln_test_render_fixture* share, const mln_queue_lock* queue_lock
) {
  wrap_device_proc_addr = wrap;
  attach_queue_lock = queue_lock;
  shared_graphics =
    share == NULL ? NULL : mln_test_render_fixture_graphics(share);
  const bool attached =
    mln_test_render_fixture_create_with(map, fixture, attach_borrowed_texture);
  wrap_device_proc_addr = NULL;
  attach_queue_lock = NULL;
  shared_graphics = NULL;
  return attached;
}

bool mln_test_render_fixture_create_vulkan_owned_texture(
  mln_map map, mln_test_render_fixture* fixture,
  const mln_queue_lock* queue_lock
) {
  attach_queue_lock = queue_lock;
  const bool attached =
    mln_test_render_fixture_create_with(map, fixture, mln_test_backend_attach);
  attach_queue_lock = NULL;
  return attached;
}
#endif

bool mln_test_render_fixture_create_surface(
  mln_map map, mln_test_render_fixture* fixture
) {
  return mln_test_render_fixture_create_with(map, fixture, attach_surface);
}

bool mln_test_render_fixture_read_texture(
  const mln_test_render_fixture* fixture, uint8_t* pixels, size_t size
) {
  const host_state* state = fixture->backend_state;
  if (state == NULL || state->texture == NULL) {
    return false;
  }
  if (!mln_test_graphics_texture_read_rgba8(state->texture, pixels, size)) {
    report("the render fixture could not read its borrowed texture");
    return false;
  }
  return true;
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
  host_state* state = fixture->backend_state;
  for (size_t index = 0; state != NULL && index < EXTRA_TARGETS; index += 1) {
    if (state->extra_textures[index] == NULL) {
      state->extra_textures[index] = mln_test_graphics_texture_create(
        state->graphics, MLN_TEST_HOST_TARGET_SIZE, MLN_TEST_HOST_TARGET_SIZE
      );
      if (state->extra_textures[index] == NULL) {
        report("the render fixture could not create another texture");
      }
      return state->extra_textures[index];
    }
  }
  return NULL;
}

mln_test_graphics_surface* mln_test_render_fixture_new_surface(
  const mln_test_render_fixture* fixture
) {
  host_state* state = fixture->backend_state;
  for (size_t index = 0; state != NULL && index < EXTRA_TARGETS; index += 1) {
    if (state->extra_surfaces[index] == NULL) {
      state->extra_surfaces[index] = mln_test_graphics_surface_create(
        state->graphics, MLN_TEST_HOST_TARGET_SIZE, MLN_TEST_HOST_TARGET_SIZE
      );
      if (state->extra_surfaces[index] == NULL) {
        report("the render fixture could not create another surface");
      }
      return state->extra_surfaces[index];
    }
  }
  return NULL;
}

mln_status mln_test_render_fixture_set_texture(
  const mln_test_render_fixture* fixture, mln_test_graphics* graphics,
  const mln_test_graphics_texture* texture, const mln_completion* completion
) {
  mln_test_graphics_context context = {0};
  borrowed_descriptor descriptor;
  if (
    !mln_test_graphics_get_context(graphics, &context) ||
    !describe_texture(&context, texture, &descriptor)
  ) {
    return MLN_STATUS_NATIVE_ERROR;
  }
#if defined(MLN_FFI_TEST_BACKEND_METAL)
  return mln_render_session_set_metal_borrowed_texture_target(
    fixture->session, &descriptor, completion, MLN_TEST_DIAGNOSTIC
  );
#elif defined(MLN_FFI_TEST_BACKEND_VULKAN)
  return mln_render_session_set_vulkan_borrowed_texture_target(
    fixture->session, &descriptor, completion, MLN_TEST_DIAGNOSTIC
  );
#else
  return mln_render_session_set_opengl_borrowed_texture_target(
    fixture->session, &descriptor, completion, MLN_TEST_DIAGNOSTIC
  );
#endif
}

mln_status mln_test_render_fixture_set_surface(
  const mln_test_render_fixture* fixture, mln_test_graphics* graphics,
  const mln_test_graphics_surface* surface, const mln_completion* completion
) {
  mln_test_graphics_context context = {0};
  surface_descriptor descriptor;
  if (
    !mln_test_graphics_get_context(graphics, &context) ||
    !describe_surface(&context, surface, &descriptor)
  ) {
    return MLN_STATUS_NATIVE_ERROR;
  }
#if defined(MLN_FFI_TEST_BACKEND_METAL)
  return mln_render_session_set_metal_surface_target(
    fixture->session, &descriptor, completion, MLN_TEST_DIAGNOSTIC
  );
#elif defined(MLN_FFI_TEST_BACKEND_VULKAN)
  return mln_render_session_set_vulkan_surface_target(
    fixture->session, &descriptor, completion, MLN_TEST_DIAGNOSTIC
  );
#else
  return mln_render_session_set_opengl_surface_target(
    fixture->session, &descriptor, completion, MLN_TEST_DIAGNOSTIC
  );
#endif
}
