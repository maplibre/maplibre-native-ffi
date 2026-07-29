#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct MlnValaOpenGLTestContext {
  uint32_t platform;
  void* display;
  void* config;
  void* context;
  void* surface;
  void* get_proc_address;
  void* window;
} MlnValaOpenGLTestContext;

#if defined(MLN_VALA_TEST_EGL)

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>

bool mln_vala_opengl_test_context_supported(void) { return true; }

static EGLDisplay test_display(void) {
#if defined(__APPLE__)
  const EGLAttrib attributes[] = {
    EGL_PLATFORM_ANGLE_TYPE_ANGLE,
    EGL_PLATFORM_ANGLE_TYPE_METAL_ANGLE,
    EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE,
    EGL_PLATFORM_ANGLE_DEVICE_TYPE_HARDWARE_ANGLE,
    EGL_NONE,
  };
  return eglGetPlatformDisplay(EGL_PLATFORM_ANGLE_ANGLE, NULL, attributes);
#elif defined(EGL_PLATFORM_SURFACELESS_MESA)
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
  out_context->platform = 2;
  out_context->get_proc_address = (void*)eglGetProcAddress;
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

uint32_t mln_vala_opengl_test_texture_create(uint32_t width, uint32_t height) {
  GLuint texture = 0;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(
    GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)width, (GLsizei)height, 0, GL_RGBA,
    GL_UNSIGNED_BYTE, NULL
  );
  glBindTexture(GL_TEXTURE_2D, 0);
  return texture;
}

void mln_vala_opengl_test_texture_destroy(uint32_t texture) {
  GLuint value = texture;
  glDeleteTextures(1, &value);
}

#elif defined(MLN_VALA_TEST_WGL)

#include <GL/gl.h>
#include <windows.h>

bool mln_vala_opengl_test_context_supported(void) { return true; }

bool mln_vala_opengl_test_context_create(
  uint32_t width, uint32_t height, MlnValaOpenGLTestContext* out_context
) {
  if (out_context == NULL) {
    return false;
  }
  *out_context = (MlnValaOpenGLTestContext){0};

  static const char class_name[] = "MaplibreNativeValaWgl";
  HINSTANCE instance = GetModuleHandleA(NULL);
  const WNDCLASSA window_class = {
    .style = CS_OWNDC,
    .lpfnWndProc = DefWindowProcA,
    .hInstance = instance,
    .lpszClassName = class_name,
  };
  RegisterClassA(&window_class);
  HWND window = CreateWindowExA(
    0, class_name, class_name, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
    CW_USEDEFAULT, (int)width, (int)height, NULL, NULL, instance, NULL
  );
  if (window == NULL) {
    return false;
  }
  HDC device_context = GetDC(window);
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
  const int format = ChoosePixelFormat(device_context, &pixel_format);
  if (
    format == 0 ||
    SetPixelFormat(device_context, format, &pixel_format) == FALSE
  ) {
    ReleaseDC(window, device_context);
    DestroyWindow(window);
    return false;
  }
  HGLRC context = wglCreateContext(device_context);
  if (context == NULL || wglMakeCurrent(device_context, context) == FALSE) {
    if (context != NULL) {
      wglDeleteContext(context);
    }
    ReleaseDC(window, device_context);
    DestroyWindow(window);
    return false;
  }

  out_context->platform = 1;
  out_context->display = device_context;
  out_context->context = context;
  out_context->surface = device_context;
  out_context->get_proc_address = (void*)wglGetProcAddress;
  out_context->window = window;
  return true;
}

void mln_vala_opengl_test_context_destroy(MlnValaOpenGLTestContext* context) {
  if (context == NULL || context->window == NULL) {
    return;
  }
  HWND window = (HWND)context->window;
  wglMakeCurrent(NULL, NULL);
  if (context->context != NULL) {
    wglDeleteContext((HGLRC)context->context);
  }
  if (context->display != NULL) {
    ReleaseDC(window, (HDC)context->display);
  }
  DestroyWindow(window);
  *context = (MlnValaOpenGLTestContext){0};
}

uint32_t mln_vala_opengl_test_texture_create(uint32_t width, uint32_t height) {
  GLuint texture = 0;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(
    GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)width, (GLsizei)height, 0, GL_RGBA,
    GL_UNSIGNED_BYTE, NULL
  );
  glBindTexture(GL_TEXTURE_2D, 0);
  return texture;
}

void mln_vala_opengl_test_texture_destroy(uint32_t texture) {
  GLuint value = texture;
  glDeleteTextures(1, &value);
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

uint32_t mln_vala_opengl_test_texture_create(uint32_t width, uint32_t height) {
  (void)width;
  (void)height;
  return 0;
}

void mln_vala_opengl_test_texture_destroy(uint32_t texture) { (void)texture; }

#endif
