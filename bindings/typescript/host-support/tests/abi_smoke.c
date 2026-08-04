/**
 * Exercises the normalized ABI against a real library, with no host runtime.
 *
 * The conformance suite proves this through a transport, which needs a
 * JavaScript engine. A target where no engine runs yet — a freshly cross-built
 * OpenHarmony device, say — still has to prove that the generated dispatch, the
 * layouts, and the diagnostics agree with the library it was built against.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "maplibre_native_c.h"
#include "maplibre_native_c/callback_adapter.h"
#include "mln_abi.h"

static uint64_t slots[8];

static int ep(const char* name) {
  for (uint32_t i = 0; i < mln_abi_entrypoint_count(); ++i)
    if (strcmp(mln_abi_entrypoint_name(i), name) == 0) return (int)i;
  return -1;
}

int main(void) {
  char diag[512];
  uint32_t diag_len = 0;
  int failures = 0;

  memset(slots, 0, sizeof slots);
  mln_abi_call(
    (uint32_t)ep("mln_supported_render_backend_mask"), slots, diag, sizeof diag,
    &diag_len
  );
  uint32_t mask = (uint32_t)slots[0];
  printf(
    "backend mask: %u (direct %u)\n", mask, mln_supported_render_backend_mask()
  );
  if (mask != mln_supported_render_backend_mask() || mask == 0) failures++;

  mln_runtime_options options;
  memset(&options, 0, sizeof options);
  memset(slots, 0, sizeof slots);
  slots[0] = (uint64_t)(uintptr_t)&options;
  mln_abi_call(
    (uint32_t)ep("mln_runtime_options_default"), slots, diag, sizeof diag,
    &diag_len
  );
  printf(
    "options.size: %u (direct %u)\n", options.size,
    mln_runtime_options_default().size
  );
  if (options.size != mln_runtime_options_default().size || options.size == 0)
    failures++;

  mln_lat_lng coordinate = {.latitude = 45.0, .longitude = -122.0};
  mln_projected_meters meters;
  memset(&meters, 0, sizeof meters);
  memset(slots, 0, sizeof slots);
  slots[1] = (uint64_t)(uintptr_t)&coordinate;
  slots[2] = (uint64_t)(uintptr_t)&meters;
  mln_abi_call(
    (uint32_t)ep("mln_projected_meters_for_lat_lng"), slots, diag, sizeof diag,
    &diag_len
  );
  mln_projected_meters expected;
  mln_projected_meters_for_lat_lng(coordinate, &expected);
  printf(
    "by-value struct: status=%d northing=%.3f (direct %.3f)\n",
    (int32_t)(uint32_t)slots[0], meters.northing, expected.northing
  );
  if (meters.northing != expected.northing || meters.northing == 0.0)
    failures++;

  memset(slots, 0, sizeof slots);
  diag[0] = '\0';
  mln_abi_call(
    (uint32_t)ep("mln_runtime_create"), slots, diag, sizeof diag, &diag_len
  );
  printf(
    "failing call: status=%d diagnostic=\"%s\" (%u bytes)\n",
    (int32_t)(uint32_t)slots[0], diag, diag_len
  );
  if (
    (int32_t)(uint32_t)slots[0] != MLN_STATUS_INVALID_ARGUMENT || diag_len == 0
  )
    failures++;

  int32_t abi = mln_abi_call(9999, slots, diag, sizeof diag, &diag_len);
  int32_t misaligned =
    mln_abi_call(0, (char*)slots + 1, diag, sizeof diag, &diag_len);
  int32_t null_slots = mln_abi_call(0, NULL, diag, sizeof diag, &diag_len);
  printf(
    "guards: unknown=%d misaligned=%d null=%d\n", abi, misaligned, null_slots
  );
  if (abi != 1 || misaligned != 3 || null_slots != 2) failures++;

  void* symbol = mln_abi_symbol((uint32_t)ep("mln_adapter_log_callback"));
  printf("symbol identity: %d\n", symbol == (void*)&mln_adapter_log_callback);
  if (symbol != (void*)&mln_adapter_log_callback) failures++;

  uint64_t token = mln_abi_transfer_issue(0x1234abcd);
  uint64_t first = mln_abi_transfer_claim(token);
  uint64_t second = mln_abi_transfer_claim(token);
  printf(
    "transfer: token=%llu first=%llu second=%llu\n", (unsigned long long)token,
    (unsigned long long)first, (unsigned long long)second
  );
  if (first != 0x1234abcd || second != 0) failures++;

  printf(
    failures == 0 ? "ALL CHECKS PASSED\n" : "%d CHECKS FAILED\n", failures
  );
  return failures;
}
