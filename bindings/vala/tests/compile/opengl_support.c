#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct MlnValaOpenGLTestContext {
  void* display;
  void* config;
  void* context;
  void* surface;
} MlnValaOpenGLTestContext;

#if defined(__linux__)

#include <EGL/egl.h>
#include <EGL/eglext.h>

bool mln_vala_opengl_test_context_supported(void) { return true; }

static EGLDisplay test_display(void) {
#if defined(EGL_PLATFORM_SURFACELESS_MESA)
  EGLDisplay display = eglGetPlatformDisplay(
    EGL_PLATFORM_SURFACELESS_MESA, EGL_DEFAULT_DISPLAY, NULL
  );
  if (display != EGL_NO_DISPLAY) {
    return display;
  }
#endif
  return eglGetDisplay(EGL_DEFAULT_DISPLAY);
}

bool mln_vala_opengl_test_context_create(
  uint32_t width, uint32_t height, MlnValaOpenGLTestContext* out_context
) {
  if (out_context == NULL) {
    return false;
  }
  *out_context = (MlnValaOpenGLTestContext){0};

  EGLDisplay display = test_display();
  if (
    display == EGL_NO_DISPLAY ||
    eglInitialize(display, NULL, NULL) == EGL_FALSE ||
    eglBindAPI(EGL_OPENGL_ES_API) == EGL_FALSE
  ) {
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
  EGLConfig config = NULL;
  EGLint config_count = 0;
  if (
    eglChooseConfig(display, config_attributes, &config, 1, &config_count) ==
      EGL_FALSE ||
    config_count == 0 || config == NULL
  ) {
    eglTerminate(display);
    return false;
  }

  const EGLint context_attributes[] = {
    EGL_CONTEXT_CLIENT_VERSION,
    3,
    EGL_NONE,
  };
  EGLContext context =
    eglCreateContext(display, config, EGL_NO_CONTEXT, context_attributes);
  const EGLint surface_attributes[] = {
    EGL_WIDTH, (EGLint)width, EGL_HEIGHT, (EGLint)height, EGL_NONE,
  };
  EGLSurface surface =
    eglCreatePbufferSurface(display, config, surface_attributes);
  if (
    context == EGL_NO_CONTEXT || surface == EGL_NO_SURFACE ||
    eglMakeCurrent(display, surface, surface, context) == EGL_FALSE
  ) {
    if (surface != EGL_NO_SURFACE) {
      eglDestroySurface(display, surface);
    }
    if (context != EGL_NO_CONTEXT) {
      eglDestroyContext(display, context);
    }
    eglTerminate(display);
    return false;
  }

  out_context->display = display;
  out_context->config = config;
  out_context->context = context;
  out_context->surface = surface;
  return true;
}

void mln_vala_opengl_test_context_destroy(MlnValaOpenGLTestContext* context) {
  if (context == NULL || context->display == NULL) {
    return;
  }
  EGLDisplay display = context->display;
  eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  if (context->surface != NULL) {
    eglDestroySurface(display, context->surface);
  }
  if (context->context != NULL) {
    eglDestroyContext(display, context->context);
  }
  eglTerminate(display);
  *context = (MlnValaOpenGLTestContext){0};
}

#else

bool mln_vala_opengl_test_context_supported(void) { return false; }

bool mln_vala_opengl_test_context_create(
  uint32_t width, uint32_t height, MlnValaOpenGLTestContext* out_context
) {
  (void)width;
  (void)height;
  if (out_context != NULL) {
    *out_context = (MlnValaOpenGLTestContext){0};
  }
  return false;
}

void mln_vala_opengl_test_context_destroy(MlnValaOpenGLTestContext* context) {
  (void)context;
}

#endif
