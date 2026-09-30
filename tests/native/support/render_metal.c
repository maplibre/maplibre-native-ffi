// Metal contexts for the render fixture: the system default device.

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "env.h"
#include "render.h"
#include "render_backend.h"
#include "unity.h"
#include "wait.h"

extern void* MTLCreateSystemDefaultDevice(void);

typedef struct metal_state {
  void* device;
} metal_state;

static bool create_backend_state(void** out_state, void* out_context) {
  metal_state* state = calloc(1, sizeof(*state));
  if (state == NULL) {
    return false;
  }
  state->device = MTLCreateSystemDefaultDevice();
  if (state->device == NULL) {
    free(state);
    return false;
  }
  *(mln_metal_context_descriptor*)out_context = (mln_metal_context_descriptor){
    .size = sizeof(mln_metal_context_descriptor), .device = state->device
  };
  *out_state = state;
  return true;
}

static void destroy_backend_state(void* opaque_state) { free(opaque_state); }

uint32_t mln_test_backend_driver(void) { return MLN_RENDER_DRIVER_CORE_WORKER; }

bool mln_test_backend_attach(
  mln_map map, const mln_render_session_attach_options* options,
  void** out_state, mln_render_session* out_session,
  const mln_completion* completion, mln_status* out_status
) {
  mln_metal_context_descriptor context = {0};
  if (!create_backend_state(out_state, &context)) {
    return false;
  }
  mln_metal_owned_texture_descriptor descriptor =
    mln_metal_owned_texture_descriptor_default();
  descriptor.extent.width = 64;
  descriptor.extent.height = 64;
  descriptor.context = context;
  *out_status = mln_metal_owned_texture_attach(
    map, &descriptor, options, out_session, completion, MLN_TEST_DIAGNOSTIC
  );
  return true;
}

void mln_test_backend_destroy(void* state) { destroy_backend_state(state); }

void mln_test_release_thread_gpu_resources(void) {}
