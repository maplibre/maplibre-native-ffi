// WGL contexts for the render fixture: a hidden window's device context.

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <windows.h>

#include "env.h"
#include "render.h"
#include "render_backend.h"
#include "unity.h"
#include "wait.h"

typedef struct wgl_state {
  HINSTANCE instance;
  HWND window;
  HDC device_context;
  HGLRC context;
} wgl_state;

static bool create_backend_state(void** out_state, void* out_context) {
  static const char class_name[] = "MaplibreNativeCApiTestsWgl";
  wgl_state* state = calloc(1, sizeof(*state));
  if (state == NULL) {
    return false;
  }
  state->instance = GetModuleHandleA(NULL);
  const WNDCLASSA window_class = {
    .style = CS_OWNDC,
    .lpfnWndProc = DefWindowProcA,
    .hInstance = state->instance,
    .lpszClassName = class_name,
  };
  RegisterClassA(&window_class);
  state->window = CreateWindowExA(
    0, class_name, class_name, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
    CW_USEDEFAULT, 8, 8, NULL, NULL, state->instance, NULL
  );
  if (state->window == NULL) {
    free(state);
    return false;
  }
  state->device_context = GetDC(state->window);
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
  const int format = ChoosePixelFormat(state->device_context, &pixel_format);
  if (
    format == 0 ||
    SetPixelFormat(state->device_context, format, &pixel_format) == FALSE
  ) {
    ReleaseDC(state->window, state->device_context);
    DestroyWindow(state->window);
    free(state);
    return false;
  }
  state->context = wglCreateContext(state->device_context);
  if (
    state->context == NULL ||
    wglMakeCurrent(state->device_context, state->context) == FALSE
  ) {
    if (state->context != NULL) {
      wglDeleteContext(state->context);
    }
    ReleaseDC(state->window, state->device_context);
    DestroyWindow(state->window);
    free(state);
    return false;
  }
  *(mln_opengl_context_descriptor*)out_context =
    (mln_opengl_context_descriptor){
      .size = sizeof(mln_opengl_context_descriptor),
      .platform = MLN_OPENGL_CONTEXT_PLATFORM_WGL,
      .data = {
        .wgl = {
          .size = sizeof(mln_wgl_context_descriptor),
          .device_context = state->device_context,
          .share_context = state->context,
          .get_proc_address = (void*)wglGetProcAddress,
        }
      },
    };
  *out_state = state;
  return true;
}

static void destroy_backend_state(void* opaque_state) {
  wgl_state* state = opaque_state;
  if (state == NULL) {
    return;
  }
  wglMakeCurrent(NULL, NULL);
  wglDeleteContext(state->context);
  ReleaseDC(state->window, state->device_context);
  DestroyWindow(state->window);
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
