// EGL contexts for the render fixture, and the dedicated EGL fixtures whose
// context the session owns.

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "env.h"
#include "render.h"
#include "render_backend.h"
#include "unity.h"
#include "wait.h"

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
static egl_state* create_dedicated_egl_state(void) {
  egl_state* state = calloc(1, sizeof(egl_state));
  if (state == NULL) {
    return NULL;
  }
  state->display = get_egl_display();
  if (
    state->display == EGL_NO_DISPLAY ||
    eglInitialize(state->display, NULL, NULL) == EGL_FALSE
  ) {
    free(state);
    return NULL;
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
    return NULL;
  }
  return state;
}

mln_test_fixture_result mln_test_dedicated_egl_surface_create(
  mln_map map, mln_test_render_fixture* fixture
) {
  egl_state* state = create_dedicated_egl_state();
  if (state == NULL) {
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

  fixture->driver = MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD;
  fixture->backend_state = state;
  mln_render_session_attach_options options =
    mln_render_session_attach_options_default();
  options.driver = fixture->driver;
  mln_test_completion attach = mln_test_completion_default(0);
  const mln_status attach_status = mln_opengl_surface_attach(
    map, &descriptor, &options, &fixture->session, &attach.descriptor,
    MLN_TEST_DIAGNOSTIC
  );
  if (
    attach_status != MLN_STATUS_OK ||
    mln_test_render_fixture_finish_operation(fixture, &attach) != MLN_STATUS_OK
  ) {
    if (attach_status != MLN_STATUS_OK) {
      attach.descriptor.release_user_data(attach.descriptor.user_data);
    }
    mln_test_completion_destroy(&attach);
    if (fixture->session != MLN_HANDLE_NULL) {
      mln_render_abandon_result abandoned = {
        .size = sizeof(mln_render_abandon_result)
      };
      (void)mln_render_session_abandon(
        fixture->session, &abandoned, MLN_TEST_DIAGNOSTIC
      );
      (void)mln_render_session_destroy(fixture->session, MLN_TEST_DIAGNOSTIC);
    }
    eglDestroySurface(state->display, state->surface);
    free(state);
    *fixture = (mln_test_render_fixture){0};
    return MLN_TEST_FIXTURE_ATTACH_FAILED;
  }
  mln_test_completion_destroy(&attach);
  return MLN_TEST_FIXTURE_OK;
}

void mln_test_dedicated_egl_surface_destroy(mln_test_render_fixture* fixture) {
  if (fixture == NULL) {
    return;
  }
  if (fixture->session != 0) {
    mln_test_completion detach = mln_test_completion_default(0);
    if (
      mln_render_session_detach(
        fixture->session, &detach.descriptor, MLN_TEST_DIAGNOSTIC
      ) == MLN_STATUS_OK
    ) {
      (void)mln_test_render_fixture_finish_operation(fixture, &detach);
    } else {
      detach.descriptor.release_user_data(detach.descriptor.user_data);
    }
    mln_test_completion_destroy(&detach);
    (void)mln_render_session_destroy(fixture->session, MLN_TEST_DIAGNOSTIC);
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

mln_test_fixture_result mln_test_dedicated_egl_texture_create(
  mln_map map, mln_test_render_fixture* fixture
) {
  egl_state* state = create_dedicated_egl_state();
  if (state == NULL) {
    return MLN_TEST_FIXTURE_UNAVAILABLE;
  }
  mln_opengl_owned_texture_descriptor descriptor =
    mln_opengl_owned_texture_descriptor_default();
  descriptor.extent.width = 64;
  descriptor.extent.height = 64;
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
  fixture->driver = MLN_RENDER_DRIVER_CORE_WORKER;
  fixture->backend_state = state;
  mln_render_session_attach_options options =
    mln_render_session_attach_options_default();
  options.driver = fixture->driver;
  options.requested_texture_ring_depth = 3;
  mln_test_completion attach = mln_test_completion_default(0);
  const mln_status attach_status = mln_opengl_owned_texture_attach(
    map, &descriptor, &options, &fixture->session, &attach.descriptor,
    MLN_TEST_DIAGNOSTIC
  );
  const mln_status finish_status =
    attach_status == MLN_STATUS_OK
      ? mln_test_render_fixture_finish_operation(fixture, &attach)
      : attach_status;
  if (attach_status != MLN_STATUS_OK) {
    attach.descriptor.release_user_data(attach.descriptor.user_data);
  }
  mln_test_completion_destroy(&attach);
  if (finish_status != MLN_STATUS_OK) {
    if (fixture->session != MLN_HANDLE_NULL) {
      mln_render_abandon_result abandoned = {
        .size = sizeof(mln_render_abandon_result)
      };
      (void)mln_render_session_abandon(
        fixture->session, &abandoned, MLN_TEST_DIAGNOSTIC
      );
      (void)mln_render_session_destroy(fixture->session, MLN_TEST_DIAGNOSTIC);
    }
    free(state);
    *fixture = (mln_test_render_fixture){0};
    return MLN_TEST_FIXTURE_ATTACH_FAILED;
  }
  return MLN_TEST_FIXTURE_OK;
}

void mln_test_dedicated_egl_texture_destroy(mln_test_render_fixture* fixture) {
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
  free(fixture->backend_state);
  *fixture = (mln_test_render_fixture){0};
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

uint32_t mln_test_backend_driver(void) {
  return MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD;
}

bool mln_test_backend_attach(
  mln_map map, const mln_render_session_attach_options* options,
  void** out_state, mln_render_session* out_session,
  const mln_completion* completion, mln_status* out_status
) {
  mln_opengl_context_descriptor context = {0};
  if (!create_backend_state(out_state, &context)) {
    return false;
  }
  mln_opengl_owned_texture_descriptor descriptor =
    mln_opengl_owned_texture_descriptor_default();
  descriptor.extent.width = 64;
  descriptor.extent.height = 64;
  descriptor.context = context;
  *out_status = mln_opengl_owned_texture_attach(
    map, &descriptor, options, out_session, completion, MLN_TEST_DIAGNOSTIC
  );
  return true;
}

void mln_test_backend_destroy(void* state) { destroy_backend_state(state); }

void mln_test_release_thread_gpu_resources(void) {}
