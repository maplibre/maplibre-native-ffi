/*
 * Native implementations of the protocol groups that executed probes call.
 *
 * Each probe compiles this file with the same MLN_PROTOCOL_* defines as the
 * header it generated bindings from, and links it into the probe program.
 */
#define MLN_PROTOCOL_VALUES
#define MLN_PROTOCOL_KEYWORDS
#define MLN_PROTOCOL_DEFAULTS
#include <stdlib.h>
#include <string.h>

#include "protocols.h"

static mln_status fail(mln_diagnostic* diagnostic, const char* message) {
  if (diagnostic != NULL && diagnostic->size >= sizeof(mln_diagnostic)) {
    strncpy(diagnostic->message, message, sizeof(diagnostic->message) - 1);
    diagnostic->message[sizeof(diagnostic->message) - 1] = 0;
  }
  return MLN_STATUS_INVALID_ARGUMENT;
}

// Reports the C ABI version that every binding's handwritten runtime expects,
// or MLN_PROBE_C_VERSION when it is set, for a probe that checks a mismatch.
uint32_t mln_c_version(void) {
  const char* version = getenv("MLN_PROBE_C_VERSION");
  return version == NULL ? 0 : (uint32_t)strtoul(version, NULL, 10);
}

// Returns its input, so a probe sees what its binding encoded, as decoded.
mln_status mln_probe_roundtrip(
  mln_probe_options input, mln_probe_options* out_options,
  mln_diagnostic* out_diagnostic
) {
  if (input.right_count > 8) {
    return fail(out_diagnostic, "right holds more than 8 points");
  }
  *out_options = input;
  return MLN_STATUS_OK;
}

mln_status mln_probe_nullable_text(
  const char* text, uint16_t text_size, mln_probe_text_result* out_result,
  mln_diagnostic* out_diagnostic
) {
  (void)out_diagnostic;
  out_result->text = (mln_buffer_view){text, text_size};
  return MLN_STATUS_OK;
}

// Moves each argument into a distinct field, so a probe can tell them apart.
mln_status mln_keyword_combine(
  double defer, double self, double raw, double bindingArg0,
  mln_keyword_entry* out_entry, mln_diagnostic* out_diagnostic
) {
  (void)out_diagnostic;
  out_entry->type = defer - self;
  out_entry->defer = raw;
  out_entry->raw = bindingArg0;
  return MLN_STATUS_OK;
}

mln_probe_settings mln_probe_settings_default(void) {
  return (mln_probe_settings){
    .size = sizeof(mln_probe_settings),
    .extent = {.width = 256, .scale = 1.5},
    .mode = MLN_PROBE_MODE_SECOND,
    .flags = MLN_PROBE_FLAG_ALL,
    .heading = MLN_PROBE_FLAG_SOUTH,
    .ratio = 0.25F,
    .offset = -3,
    .enabled = true,
  };
}

mln_status mln_probe_settings_check(
  mln_probe_settings settings, mln_diagnostic* out_diagnostic
) {
  const mln_probe_settings expected = mln_probe_settings_default();
  if (settings.size != expected.size) return fail(out_diagnostic, "size");
  if (settings.extent.width != expected.extent.width) {
    return fail(out_diagnostic, "extent.width");
  }
  if (settings.extent.scale != expected.extent.scale) {
    return fail(out_diagnostic, "extent.scale");
  }
  if (settings.mode != expected.mode) return fail(out_diagnostic, "mode");
  if (settings.flags != expected.flags) return fail(out_diagnostic, "flags");
  if (settings.heading != expected.heading) {
    return fail(out_diagnostic, "heading");
  }
  if (settings.ratio != expected.ratio) return fail(out_diagnostic, "ratio");
  if (settings.offset != expected.offset) return fail(out_diagnostic, "offset");
  if (settings.enabled != expected.enabled) {
    return fail(out_diagnostic, "enabled");
  }
  if (settings.count != expected.count) return fail(out_diagnostic, "count");
  return MLN_STATUS_OK;
}
