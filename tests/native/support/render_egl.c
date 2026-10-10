// The dedicated EGL fixtures, whose sessions own their EGL context. The
// display, config, and pbuffer come from tests/graphics; the session creates
// the context itself.

#include <EGL/egl.h>
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

// Nothing becomes current here: naming dedicated ownership is what asks the
// session to create its own context and keep it current.
static dedicated_state* create_dedicated_state(bool with_surface) {
  dedicated_state* state = calloc(1, sizeof(*state));
  if (state == NULL) {
    return NULL;
  }
  state->graphics = mln_test_graphics_create(MLN_TEST_GRAPHICS_BACKEND_EGL);
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
    stderr, "the dedicated EGL fixture has no EGL objects: %s\n",
    mln_test_graphics_last_error()
  );
  destroy_dedicated_state(state);
  return NULL;
}

static mln_opengl_context_descriptor dedicated_context(
  const dedicated_state* state
) {
  return (mln_opengl_context_descriptor){
    .platform = MLN_OPENGL_CONTEXT_PLATFORM_EGL,
    .ownership = MLN_OPENGL_CONTEXT_OWNERSHIP_DEDICATED,
    .data = {
      .egl = {
        .display = state->context.egl_display,
        .config = state->context.egl_config,
        .share_context = NULL,
        .client_api = MLN_OPENGL_CLIENT_API_GLES,
        .get_proc_address = NULL,
      }
    },
  };
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
    (void)mln_render_session_destroy(fixture->session, MLN_TEST_DIAGNOSTIC);
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
    (void)mln_render_session_destroy(fixture->session, MLN_TEST_DIAGNOSTIC);
  }
  destroy_dedicated_state(fixture->backend_state);
  *fixture = (mln_test_render_fixture){0};
}

bool mln_test_dedicated_egl_surface_create(
  mln_map map, mln_test_render_fixture* fixture
) {
  dedicated_state* state = create_dedicated_state(true);
  if (state == NULL) {
    return false;
  }
  mln_test_graphics_surface_info surface = {0};
  (void)mln_test_graphics_surface_get_info(state->surface, &surface);
  mln_opengl_surface_descriptor descriptor =
    mln_opengl_surface_descriptor_default();
  descriptor.extent = (mln_render_target_extent){
    .size = sizeof(mln_render_target_extent),
    .width = surface.width,
    .height = surface.height,
    .scale_factor = 1.0,
  };
  descriptor.context = dedicated_context(state);
  descriptor.surface = surface.opengl_surface;

  fixture->driver = MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD;
  fixture->backend_state = state;
  mln_render_session_attach_options options =
    mln_render_session_attach_options_default();
  options.driver = fixture->driver;
  mln_test_completion attach = mln_test_completion_default(0);
  const mln_status attach_status = mln_map_attach_opengl_surface(
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

bool mln_test_dedicated_egl_texture_create(
  mln_map map, mln_test_render_fixture* fixture
) {
  dedicated_state* state = create_dedicated_state(false);
  if (state == NULL) {
    return false;
  }
  mln_opengl_owned_texture_descriptor descriptor =
    mln_opengl_owned_texture_descriptor_default();
  descriptor.extent.width = 64;
  descriptor.extent.height = 64;
  descriptor.context = dedicated_context(state);
  fixture->driver = MLN_RENDER_DRIVER_CORE_WORKER;
  fixture->backend_state = state;
  mln_render_session_attach_options options =
    mln_render_session_attach_options_default();
  options.driver = fixture->driver;
  options.requested_texture_ring_depth = 3;
  mln_test_completion attach = mln_test_completion_default(0);
  const mln_status attach_status = mln_map_attach_opengl_owned_texture(
    map, &descriptor, &options, &fixture->session, &attach.descriptor,
    MLN_TEST_DIAGNOSTIC
  );
  return finish_dedicated_attach(fixture, state, attach_status, &attach);
}

void mln_test_dedicated_egl_texture_destroy(mln_test_render_fixture* fixture) {
  destroy_dedicated_fixture(fixture);
}
