/*
 * Native implementations of the protocol groups that executed probes call.
 *
 * Each probe compiles this file with the same MLN_PROTOCOL_* defines as the
 * header it generated bindings from, and links it into the probe program.
 */
#define MLN_PROTOCOL_VALUES
#define MLN_PROTOCOL_KEYWORDS
#include <string.h>

#include "protocols.h"

static mln_status fail(mln_diagnostic* diagnostic, const char* message) {
  if (diagnostic != NULL && diagnostic->size >= sizeof(mln_diagnostic)) {
    strncpy(diagnostic->message, message, sizeof(diagnostic->message) - 1);
    diagnostic->message[sizeof(diagnostic->message) - 1] = 0;
  }
  return MLN_STATUS_INVALID_ARGUMENT;
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
