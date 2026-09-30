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

typedef struct host_state {
  mln_test_graphics* graphics;
  mln_test_graphics_context context;
  mln_test_graphics_texture* texture;
  mln_test_graphics_surface* surface;
} host_state;

static void report(const char* what) {
  fprintf(stderr, "%s: %s\n", what, mln_test_graphics_last_error());
}

// An OpenGL host drives its sessions from its own graphics thread with its
// context current, which is the thread that creates the fixture.
static host_state* host_state_create(void) {
  host_state* state = calloc(1, sizeof(*state));
  if (state == NULL) {
    return NULL;
  }
  state->graphics = mln_test_graphics_create(HOST_BACKEND);
  if (
    state->graphics == NULL ||
    !mln_test_graphics_get_context(state->graphics, &state->context)
  ) {
    report("the render fixture could not create a graphics context");
    mln_test_graphics_destroy(state->graphics);
    free(state);
    return NULL;
  }
#if defined(HOST_OPENGL)
  if (!mln_test_graphics_make_current(state->graphics)) {
    report("the render fixture could not make its context current");
    mln_test_graphics_destroy(state->graphics);
    free(state);
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
  mln_test_graphics_surface_destroy(state->surface);
  mln_test_graphics_texture_destroy(state->texture);
  mln_test_graphics_destroy(state->graphics);
  free(state);
}

uint32_t mln_test_backend_driver(void) {
#if defined(HOST_OPENGL)
  return MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD;
#else
  return MLN_RENDER_DRIVER_CORE_WORKER;
#endif
}

void mln_test_release_thread_gpu_resources(void) {}

static mln_render_target_extent host_extent(void) {
  return (mln_render_target_extent){
    .size = sizeof(mln_render_target_extent),
    .width = MLN_TEST_HOST_TARGET_SIZE,
    .height = MLN_TEST_HOST_TARGET_SIZE,
    .scale_factor = 1.0,
  };
}

#if defined(MLN_FFI_TEST_BACKEND_METAL)
static mln_metal_context_descriptor host_context(const host_state* state) {
  return (mln_metal_context_descriptor){
    .size = sizeof(mln_metal_context_descriptor),
    .device = state->context.metal_device,
  };
}
#elif defined(MLN_FFI_TEST_BACKEND_VULKAN)
static mln_vulkan_context_descriptor host_context(const host_state* state) {
  const mln_test_graphics_context* context = &state->context;
  return (mln_vulkan_context_descriptor){
    .size = sizeof(mln_vulkan_context_descriptor),
    .instance = context->vulkan_instance,
    .physical_device = context->vulkan_physical_device,
    .device = context->vulkan_device,
    .graphics_queue = context->vulkan_queue,
    .graphics_queue_family_index = context->vulkan_queue_family_index,
    .get_instance_proc_addr = context->vulkan_get_instance_proc_addr,
    .get_device_proc_addr = context->vulkan_get_device_proc_addr,
  };
}
#elif defined(MLN_FFI_TEST_OPENGL_WGL)
static mln_opengl_context_descriptor host_context(const host_state* state) {
  const mln_test_graphics_context* context = &state->context;
  return (mln_opengl_context_descriptor){
    .size = sizeof(mln_opengl_context_descriptor),
    .platform = MLN_OPENGL_CONTEXT_PLATFORM_WGL,
    .data = {
      .wgl = {
        .size = sizeof(mln_wgl_context_descriptor),
        .device_context = context->wgl_device_context,
        .share_context = context->wgl_context,
        .get_proc_address = context->get_proc_address,
      }
    },
  };
}
#else
static mln_opengl_context_descriptor host_context(const host_state* state) {
  const mln_test_graphics_context* context = &state->context;
  return (mln_opengl_context_descriptor){
    .size = sizeof(mln_opengl_context_descriptor),
    .platform = MLN_OPENGL_CONTEXT_PLATFORM_EGL,
    .data = {
      .egl = {
        .size = sizeof(mln_egl_context_descriptor),
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
  descriptor.context = host_context(state);
  *out_status = mln_metal_owned_texture_attach(
    map, &descriptor, options, out_session, completion, MLN_TEST_DIAGNOSTIC
  );
#elif defined(MLN_FFI_TEST_BACKEND_VULKAN)
  mln_vulkan_owned_texture_descriptor descriptor =
    mln_vulkan_owned_texture_descriptor_default();
  descriptor.extent = host_extent();
  descriptor.context = host_context(state);
  *out_status = mln_vulkan_owned_texture_attach(
    map, &descriptor, options, out_session, completion, MLN_TEST_DIAGNOSTIC
  );
#else
  mln_opengl_owned_texture_descriptor descriptor =
    mln_opengl_owned_texture_descriptor_default();
  descriptor.extent = host_extent();
  descriptor.context = host_context(state);
  *out_status = mln_opengl_owned_texture_attach(
    map, &descriptor, options, out_session, completion, MLN_TEST_DIAGNOSTIC
  );
#endif
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
  mln_test_graphics_texture_info texture = {0};
  if (
    state->texture == NULL ||
    !mln_test_graphics_texture_get_info(state->texture, &texture)
  ) {
    report("the render fixture could not create a borrowed texture");
    mln_test_backend_destroy(state);
    return false;
  }
  *out_state = state;
#if defined(MLN_FFI_TEST_BACKEND_METAL)
  mln_metal_borrowed_texture_descriptor descriptor =
    mln_metal_borrowed_texture_descriptor_default();
  descriptor.texture = texture.metal_texture;
#elif defined(MLN_FFI_TEST_BACKEND_VULKAN)
  mln_vulkan_borrowed_texture_descriptor descriptor =
    mln_vulkan_borrowed_texture_descriptor_default();
  descriptor.context = host_context(state);
  descriptor.image = texture.vulkan_image;
  descriptor.image_view = texture.vulkan_image_view;
  descriptor.format = texture.format;
  descriptor.initial_layout = texture.vulkan_initial_layout;
  descriptor.final_layout = texture.vulkan_final_layout;
#else
  mln_opengl_borrowed_texture_descriptor descriptor =
    mln_opengl_borrowed_texture_descriptor_default();
  descriptor.context = host_context(state);
  descriptor.texture = texture.opengl_texture;
  descriptor.target = texture.opengl_target;
#endif
  // Physical and logical sizes agree at a scale factor of 1.
  descriptor.extent = host_extent();
  descriptor.physical_width = texture.width;
  descriptor.physical_height = texture.height;
#if defined(MLN_FFI_TEST_BACKEND_METAL)
  *out_status = mln_metal_borrowed_texture_attach(
    map, &descriptor, options, out_session, completion, MLN_TEST_DIAGNOSTIC
  );
#elif defined(MLN_FFI_TEST_BACKEND_VULKAN)
  *out_status = mln_vulkan_borrowed_texture_attach(
    map, &descriptor, options, out_session, completion, MLN_TEST_DIAGNOSTIC
  );
#else
  *out_status = mln_opengl_borrowed_texture_attach(
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
  mln_test_graphics_surface_info surface = {0};
  if (
    state->surface == NULL ||
    !mln_test_graphics_surface_get_info(state->surface, &surface)
  ) {
    report("the render fixture could not create a surface");
    mln_test_backend_destroy(state);
    return false;
  }
  *out_state = state;
#if defined(MLN_FFI_TEST_BACKEND_METAL)
  mln_metal_surface_descriptor descriptor =
    mln_metal_surface_descriptor_default();
  descriptor.extent = host_extent();
  descriptor.context = host_context(state);
  descriptor.layer = surface.metal_layer;
  *out_status = mln_metal_surface_attach(
    map, &descriptor, options, out_session, completion, MLN_TEST_DIAGNOSTIC
  );
#elif defined(MLN_FFI_TEST_BACKEND_VULKAN)
  mln_vulkan_surface_descriptor descriptor =
    mln_vulkan_surface_descriptor_default();
  descriptor.extent = host_extent();
  descriptor.context = host_context(state);
  descriptor.surface = surface.vulkan_surface;
  *out_status = mln_vulkan_surface_attach(
    map, &descriptor, options, out_session, completion, MLN_TEST_DIAGNOSTIC
  );
#else
  mln_opengl_surface_descriptor descriptor =
    mln_opengl_surface_descriptor_default();
  descriptor.extent = host_extent();
  descriptor.context = host_context(state);
  descriptor.surface = surface.opengl_surface;
  *out_status = mln_opengl_surface_attach(
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
