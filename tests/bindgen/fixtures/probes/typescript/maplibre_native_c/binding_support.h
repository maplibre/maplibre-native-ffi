/*
 * The binding support declarations that the TypeScript addon runtime calls.
 * The real declarations are in include/maplibre_native_c/binding_support.h,
 * and queue_stub.c implements these.
 */
#ifndef MLN_PROBE_BINDING_SUPPORT_H
#define MLN_PROBE_BINDING_SUPPORT_H

#include "api.h"

typedef unsigned long long mln_render_session;
typedef struct mln_render_abandon_result {
  uint32_t size;
} mln_render_abandon_result;

void* mln_binding_owner_token_create(uint64_t handle);
void mln_binding_owner_token_destroy(void* token);
void mln_binding_owner_finalize(void* token);
mln_status mln_binding_render_session_abandon_at_exit(
  mln_render_session session, mln_render_abandon_result* out_result,
  mln_diagnostic* out_diagnostic
);

#endif
