#ifndef MLN_NATIVE_TESTS_ATTACH_TABLE_H
#define MLN_NATIVE_TESTS_ATTACH_TABLE_H

// Validation tables for render target attach and set_target calls. A row edits
// one call's arguments or its backend descriptor, and the runner submits it
// with a completion that no accepted row may reach: every row here expects a
// rejection, and a rejected attach must leave its output session as it was.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "env.h"
#include "maplibre_native_c.h"
#include "tables.h"
#include "unity.h"
#include "wait.h"

// A handle value for descriptor fields that only validation reads.
#define MLN_TEST_FAKE_HANDLE ((void*)(uintptr_t)1)

// One call's arguments around any backend's descriptor.
typedef struct mln_test_target_call {
  mln_map map;
  mln_render_session session;
  mln_render_session_attach_options options;
  bool null_descriptor;
  bool null_options;
  bool null_session;
  bool null_completion;
  union {
    mln_metal_surface_descriptor metal_surface;
    mln_metal_owned_texture_descriptor metal_owned;
    mln_metal_borrowed_texture_descriptor metal_borrowed;
    mln_vulkan_surface_descriptor vulkan_surface;
    mln_vulkan_owned_texture_descriptor vulkan_owned;
    mln_vulkan_borrowed_texture_descriptor vulkan_borrowed;
    mln_opengl_surface_descriptor opengl_surface;
    mln_opengl_owned_texture_descriptor opengl_owned;
    mln_opengl_borrowed_texture_descriptor opengl_borrowed;
    mln_webgpu_surface_descriptor webgpu_surface;
    mln_webgpu_owned_texture_descriptor webgpu_owned;
    mln_webgpu_borrowed_texture_descriptor webgpu_borrowed;
  } descriptor;
} mln_test_target_call;

// A backend's attach function, with its descriptor type erased.
typedef mln_status (*mln_test_attach_fn)(
  mln_map map, const void* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion,
  mln_diagnostic* diagnostic
);

// A backend's set_target function, with its descriptor type erased.
typedef mln_status (*mln_test_set_target_fn)(
  mln_render_session session, const void* descriptor,
  const mln_completion* completion, mln_diagnostic* diagnostic
);

// A call's defaults: `map`, a zeroed out session, the default options with
// `driver`, and the descriptor the caller fills in afterwards.
static inline mln_test_target_call mln_test_target_call_default(
  mln_map map, uint32_t driver
) {
  mln_test_target_call call = {.map = map};
  call.options = mln_render_session_attach_options_default();
  call.options.driver = driver;
  return call;
}

// Submits `call` through `attach` and checks that a rejection left the output
// session as it was.
static inline mln_status mln_test_submit_attach(
  mln_test_attach_fn attach, const mln_test_target_call* call,
  mln_diagnostic* diagnostic
) {
  mln_render_session session = call->session;
  const mln_completion completion = mln_test_discard_completion();
  const mln_status status = attach(
    call->map, call->null_descriptor ? NULL : &call->descriptor,
    call->null_options ? NULL : &call->options,
    call->null_session ? NULL : &session,
    call->null_completion ? NULL : &completion, diagnostic
  );
  if (status != MLN_STATUS_OK) {
    TEST_ASSERT_EQUAL_UINT64_MESSAGE(
      call->session, session, "a rejected attach wrote its session"
    );
  }
  return status;
}

static inline mln_status mln_test_submit_set_target(
  mln_test_set_target_fn set_target, const mln_test_target_call* call,
  mln_diagnostic* diagnostic
) {
  const mln_completion completion = mln_test_discard_completion();
  return set_target(
    call->session, call->null_descriptor ? NULL : &call->descriptor,
    call->null_completion ? NULL : &completion, diagnostic
  );
}

static inline void mln_test_call_without_map(void* call) {
  ((mln_test_target_call*)call)->map = MLN_HANDLE_NULL;
}
static inline void mln_test_call_without_descriptor(void* call) {
  ((mln_test_target_call*)call)->null_descriptor = true;
}
static inline void mln_test_call_without_options(void* call) {
  ((mln_test_target_call*)call)->null_options = true;
}
static inline void mln_test_call_with_undersized_options(void* call) {
  ((mln_test_target_call*)call)->options.size =
    sizeof(mln_render_session_attach_options) - 1;
}
static inline void mln_test_call_without_session(void* call) {
  ((mln_test_target_call*)call)->null_session = true;
}
static inline void mln_test_call_with_occupied_session(void* call) {
  ((mln_test_target_call*)call)->session = 1;
}
static inline void mln_test_call_without_completion(void* call) {
  ((mln_test_target_call*)call)->null_completion = true;
}

// Runs the rows every attach shares, then the backend's `rows`, from the
// defaults in `call`, which must name a live map.
static inline void mln_test_run_attach_table(
  mln_test_validation_call submit, const mln_test_target_call* call,
  const mln_test_validation_case* rows, size_t row_count
) {
  const mln_test_validation_case shared[] = {
    {"null map", mln_test_call_without_map, MLN_STATUS_INVALID_ARGUMENT, NULL},
    {"null descriptor", mln_test_call_without_descriptor,
     MLN_STATUS_INVALID_ARGUMENT, NULL},
    {"null options", mln_test_call_without_options, MLN_STATUS_INVALID_ARGUMENT,
     NULL},
    {"undersized options", mln_test_call_with_undersized_options,
     MLN_STATUS_INVALID_ARGUMENT, NULL},
    {"null session output", mln_test_call_without_session,
     MLN_STATUS_INVALID_ARGUMENT, NULL},
    {"occupied session output", mln_test_call_with_occupied_session,
     MLN_STATUS_INVALID_ARGUMENT, NULL},
    {"null completion", mln_test_call_without_completion,
     MLN_STATUS_INVALID_ARGUMENT, NULL},
  };
  mln_test_run_validation_table(
    shared, sizeof(shared) / sizeof(shared[0]), call, sizeof(*call), submit,
    NULL
  );
  mln_test_run_validation_table(
    rows, row_count, call, sizeof(*call), submit, NULL
  );
}

// Defines the edit every descriptor shares, for the union member `member` of
// type `type`: undersized. MLN_TEST_DESCRIPTOR_CASES(member) names its row.
// The descriptor versions its embedded extent and context, so their sizes are
// not checked.
#define MLN_TEST_DESCRIPTOR_EDITS(member, type)                               \
  static void member##_undersized(void* call) {                               \
    ((mln_test_target_call*)call)->descriptor.member.size = sizeof(type) - 1; \
  }

#define MLN_TEST_DESCRIPTOR_CASES(member)                                     \
  {"undersized descriptor", member##_undersized, MLN_STATUS_INVALID_ARGUMENT, \
   "size is too small"}

// For a target whose physical size follows from its extent: an extent that
// overflows 32 bits once scaled. MLN_TEST_OVERFLOW_CASE(member) names its row.
#define MLN_TEST_OVERFLOW_EDIT(member)                          \
  static void member##_overflowing_extent(void* call) {         \
    mln_render_target_extent* extent =                          \
      &((mln_test_target_call*)call)->descriptor.member.extent; \
    extent->width = UINT32_MAX;                                 \
    extent->scale_factor = 2.0;                                 \
  }

#define MLN_TEST_OVERFLOW_CASE(member)                               \
  {"extent that overflows once scaled", member##_overflowing_extent, \
   MLN_STATUS_INVALID_ARGUMENT, "too large"}

// Defines `name`, a validation-table call that submits through the backend
// function `function`, an attach or a set_target.
#define MLN_TEST_ATTACH_SUBMITTER(name, function)                      \
  static mln_status name##_call(                                       \
    mln_map map, const void* descriptor,                               \
    const mln_render_session_attach_options* options,                  \
    mln_render_session* out_session, const mln_completion* completion, \
    mln_diagnostic* diagnostic                                         \
  ) {                                                                  \
    return function(                                                   \
      map, descriptor, options, out_session, completion, diagnostic    \
    );                                                                 \
  }                                                                    \
  static mln_status name(                                              \
    void* context, const void* call, mln_diagnostic* diagnostic        \
  ) {                                                                  \
    (void)context;                                                     \
    return mln_test_submit_attach(name##_call, call, diagnostic);      \
  }

#define MLN_TEST_SET_TARGET_SUBMITTER(name, function)                 \
  static mln_status name##_call(                                      \
    mln_render_session session, const void* descriptor,               \
    const mln_completion* completion, mln_diagnostic* diagnostic      \
  ) {                                                                 \
    return function(session, descriptor, completion, diagnostic);     \
  }                                                                   \
  static mln_status name(                                             \
    void* context, const void* call, mln_diagnostic* diagnostic       \
  ) {                                                                 \
    (void)context;                                                    \
    return mln_test_submit_set_target(name##_call, call, diagnostic); \
  }

// A logical extent every descriptor default can take.
static inline mln_render_target_extent mln_test_target_extent(void) {
  return (mln_render_target_extent){
    .size = sizeof(mln_render_target_extent),
    .width = 64,
    .height = 64,
    .scale_factor = 1.0,
  };
}

#endif
