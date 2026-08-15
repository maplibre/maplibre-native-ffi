#ifndef MLN_GO_INTERNAL_CGO_SHIM_H
#define MLN_GO_INTERNAL_CGO_SHIM_H

#include <stdint.h>

#include "maplibre_native_c.h"

static inline void* mln_go_handle_to_pointer(uintptr_t handle) {
  return (void*)handle;
}

static inline void mln_go_opengl_context_set_wgl(
  mln_opengl_context_descriptor* descriptor, void* device_context,
  void* share_context, void* get_proc_address
) {
  descriptor->platform = MLN_OPENGL_CONTEXT_PLATFORM_WGL;
  descriptor->data.wgl.size = sizeof(mln_wgl_context_descriptor);
  descriptor->data.wgl.device_context = device_context;
  descriptor->data.wgl.share_context = share_context;
  descriptor->data.wgl.get_proc_address = get_proc_address;
}

static inline void mln_go_opengl_context_set_egl(
  mln_opengl_context_descriptor* descriptor, void* display, void* config,
  void* share_context, mln_opengl_client_api client_api, void* get_proc_address
) {
  descriptor->platform = MLN_OPENGL_CONTEXT_PLATFORM_EGL;
  descriptor->data.egl.size = sizeof(mln_egl_context_descriptor);
  descriptor->data.egl.display = display;
  descriptor->data.egl.config = config;
  descriptor->data.egl.share_context = share_context;
  descriptor->data.egl.client_api = client_api;
  descriptor->data.egl.get_proc_address = get_proc_address;
}
static inline void mln_go_opengl_context_set_webgl(
  mln_opengl_context_descriptor* descriptor, mln_webgl_context_kind kind,
  int32_t context, const char* canvas_selector, size_t canvas_selector_size
) {
  descriptor->platform = MLN_OPENGL_CONTEXT_PLATFORM_WEBGL;
  descriptor->data.webgl.size = sizeof(mln_webgl_context_descriptor);
  descriptor->data.webgl.kind = kind;
  descriptor->data.webgl.context = context;
  descriptor->data.webgl.canvas_selector.data = canvas_selector;
  descriptor->data.webgl.canvas_selector.size = canvas_selector_size;
}

#endif
