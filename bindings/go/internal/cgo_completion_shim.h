#ifndef MAPLIBRE_NATIVE_FFI_GO_COMPLETION_SHIM_H
#define MAPLIBRE_NATIVE_FFI_GO_COMPLETION_SHIM_H

#include "maplibre_native_c.h"

extern void mln_go_completion_callback(
  void* user_data, mln_completion_result* result
);
extern void mln_go_completion_release(void* user_data);

static inline void mln_go_completion_trampoline(
  void* user_data, const mln_completion_result* result
) {
  mln_go_completion_callback(user_data, (mln_completion_result*)result);
}

// user_data is a cell from binding_handle_cell() that holds the completion's
// cgo.Handle.
static inline mln_completion mln_go_make_completion(void* user_data) {
  mln_completion completion = {
    .size = sizeof(mln_completion),
    .callback = mln_go_completion_trampoline,
    .user_data = user_data,
    .release_user_data = mln_go_completion_release,
  };
  return completion;
}

#endif
