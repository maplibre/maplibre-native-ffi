// The dedicated OpenGL fixtures, whose sessions own their EGL or WGL context.
// The EGL display, config, and pbuffer, or the WGL device context, come from
// tests/graphics; the session creates the context itself.

#if defined(MLN_FFI_TEST_OPENGL_EGL)
#include <EGL/egl.h>
#endif
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "env.h"
#include "mln_test_graphics.h"
#include "render.h"
#include "render_backend.h"
#include "unity.h"
#include "wait.h"

typedef struct dedicated_state {
  mln_test_graphics* graphics;
  mln_test_graphics_context context;
  mln_test_graphics_surface* surface;
} dedicated_state;

static void destroy_dedicated_state(dedicated_state* state) {
  if (state == NULL) {
    return;
  }
  mln_test_graphics_surface_destroy(state->surface);
  mln_test_graphics_destroy(state->graphics);
  free(state);
}

#if defined(MLN_FFI_TEST_OPENGL_EGL)
#define DEDICATED_BACKEND MLN_TEST_GRAPHICS_BACKEND_EGL
#define DEDICATED_PROVIDER "EGL"
#else
#define DEDICATED_BACKEND MLN_TEST_GRAPHICS_BACKEND_WGL
#define DEDICATED_PROVIDER "WGL"
#endif

// Nothing becomes current here: naming dedicated ownership is what asks the
// session to create its own context and keep it current.
static dedicated_state* create_dedicated_state(bool with_surface) {
  dedicated_state* state = calloc(1, sizeof(*state));
  if (state == NULL) {
    return NULL;
  }
  state->graphics = mln_test_graphics_create(DEDICATED_BACKEND);
  if (
    state->graphics != NULL &&
    mln_test_graphics_get_context(state->graphics, &state->context) &&
    (!with_surface ||
     (state->surface =
        mln_test_graphics_surface_create(state->graphics, 64, 64)) != NULL)
  ) {
    return state;
  }
  fprintf(
    stderr, "the dedicated " DEDICATED_PROVIDER " fixture has no objects: %s\n",
    mln_test_graphics_last_error()
  );
  destroy_dedicated_state(state);
  return NULL;
}

static mln_open_gl_context_descriptor dedicated_context(
  const dedicated_state* state
) {
#if defined(MLN_FFI_TEST_OPENGL_WGL)
  return (mln_open_gl_context_descriptor){
    .platform = MLN_OPEN_GL_CONTEXT_PLATFORM_WGL,
    .ownership = MLN_OPEN_GL_CONTEXT_OWNERSHIP_DEDICATED,
    .data = {
      .wgl = {
        .device_context = state->context.wgl_device_context,
        .share_context = NULL,
        .get_proc_address = NULL,
      }
    },
  };
#else
  return (mln_open_gl_context_descriptor){
    .platform = MLN_OPEN_GL_CONTEXT_PLATFORM_EGL,
    .ownership = MLN_OPEN_GL_CONTEXT_OWNERSHIP_DEDICATED,
    .data = {
      .egl = {
        .display = state->context.egl_display,
        .config = state->context.egl_config,
        .share_context = NULL,
        .client_api = MLN_OPEN_GL_CLIENT_API_GLES,
        .get_proc_address = NULL,
      }
    },
  };
#endif
}

// Leaves `fixture` zeroed after a failed attach.
static bool finish_dedicated_attach(
  mln_test_render_fixture* fixture, dedicated_state* state,
  mln_status attach_status, mln_test_completion* attach
) {
  const mln_status finish_status =
    attach_status == MLN_STATUS_OK
      ? mln_test_render_fixture_finish_operation(fixture, attach)
      : attach_status;
  if (attach_status != MLN_STATUS_OK) {
    attach->descriptor.release_user_data(attach->descriptor.user_data);
  }
  mln_test_completion_destroy(attach);
  if (finish_status == MLN_STATUS_OK) {
    return true;
  }
  if (fixture->session != MLN_HANDLE_NULL) {
    mln_render_abandon_result abandoned = {
      .size = sizeof(mln_render_abandon_result)
    };
    (void)mln_render_session_abandon(
      fixture->session, &abandoned, MLN_TEST_DIAGNOSTIC
    );
    (void)mln_test_render_session_dispose(fixture->session);
  }
  destroy_dedicated_state(state);
  *fixture = (mln_test_render_fixture){0};
  return false;
}

static void destroy_dedicated_fixture(mln_test_render_fixture* fixture) {
  if (fixture == NULL) {
    return;
  }
  if (fixture->session != MLN_HANDLE_NULL) {
    mln_test_completion detach = mln_test_completion_default(0);
    mln_status detach_status = mln_render_session_detach(
      fixture->session, &detach.descriptor, MLN_TEST_DIAGNOSTIC
    );
    if (detach_status == MLN_STATUS_OK) {
      detach_status =
        mln_test_render_fixture_finish_operation(fixture, &detach);
    } else {
      detach.descriptor.release_user_data(detach.descriptor.user_data);
    }
    mln_test_completion_destroy(&detach);
    if (detach_status != MLN_STATUS_OK) {
      mln_render_abandon_result abandoned = {
        .size = sizeof(mln_render_abandon_result)
      };
      (void)mln_render_session_abandon(
        fixture->session, &abandoned, MLN_TEST_DIAGNOSTIC
      );
    }
    (void)mln_test_render_session_dispose(fixture->session);
  }
  destroy_dedicated_state(fixture->backend_state);
  *fixture = (mln_test_render_fixture){0};
}

#if defined(MLN_FFI_TEST_OPENGL_EGL)
bool mln_test_dedicated_egl_surface_create(
  mln_map map, mln_test_render_fixture* fixture
) {
  dedicated_state* state = create_dedicated_state(true);
  if (state == NULL) {
    return false;
  }
  mln_test_graphics_surface_info surface = {0};
  (void)mln_test_graphics_surface_get_info(state->surface, &surface);
  mln_open_gl_surface_descriptor descriptor = {
    .size = sizeof(mln_open_gl_surface_descriptor)
  };
  descriptor.extent = (mln_logical_extent){
    .width = surface.width,
    .height = surface.height,
    .scale_factor = 1.0,
  };
  descriptor.context = dedicated_context(state);
  descriptor.surface = surface.opengl_surface;

  fixture->driver = MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD;
  fixture->backend_state = state;
  mln_render_session_attach_options options = {
    .size = sizeof(mln_render_session_attach_options)
  };
  options.driver = fixture->driver;
  mln_test_completion attach = mln_test_completion_default(0);
  const mln_status attach_status = mln_map_attach_open_gl_surface(
    map, &descriptor, &options, &fixture->session, &attach.descriptor,
    MLN_TEST_DIAGNOSTIC
  );
  return finish_dedicated_attach(fixture, state, attach_status, &attach);
}

void mln_test_dedicated_egl_surface_destroy(mln_test_render_fixture* fixture) {
  destroy_dedicated_fixture(fixture);
}

bool mln_test_egl_context_is_current(void) {
  return eglGetCurrentContext() != EGL_NO_CONTEXT;
}

bool mln_test_dedicated_egl_surface_is_current(
  const mln_test_render_fixture* fixture
) {
  const dedicated_state* state = fixture->backend_state;
  mln_test_graphics_surface_info surface = {0};
  return mln_test_graphics_surface_get_info(state->surface, &surface) &&
         eglGetCurrentContext() != EGL_NO_CONTEXT &&
         eglGetCurrentSurface(EGL_DRAW) == surface.opengl_surface;
}

bool mln_test_dedicated_egl_make_host_context_current(
  mln_test_render_fixture* fixture
) {
  dedicated_state* state = fixture->backend_state;
  return mln_test_graphics_make_current(state->graphics);
}

#endif

bool mln_test_dedicated_texture_create(
  mln_map map, mln_test_render_fixture* fixture
) {
  dedicated_state* state = create_dedicated_state(false);
  if (state == NULL) {
    return false;
  }
  mln_open_gl_owned_texture_descriptor descriptor = {
    .size = sizeof(mln_open_gl_owned_texture_descriptor)
  };
  descriptor.extent.width = 64;
  descriptor.extent.height = 64;
  descriptor.extent.scale_factor = 1.0;
  descriptor.context = dedicated_context(state);
  fixture->driver = MLN_RENDER_DRIVER_CORE_WORKER;
  fixture->backend_state = state;
  mln_render_session_attach_options options = {
    .size = sizeof(mln_render_session_attach_options)
  };
  options.driver = fixture->driver;
  options.requested_texture_ring_depth = 3;
  mln_test_completion attach = mln_test_completion_default(0);
  const mln_status attach_status = mln_map_attach_open_gl_owned_texture(
    map, &descriptor, &options, &fixture->session, &attach.descriptor,
    MLN_TEST_DIAGNOSTIC
  );
  return finish_dedicated_attach(fixture, state, attach_status, &attach);
}

void mln_test_dedicated_texture_destroy(mln_test_render_fixture* fixture) {
  destroy_dedicated_fixture(fixture);
}
