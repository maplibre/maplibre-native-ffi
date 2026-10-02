/// The parts of the C ABI that the generated declarations name but the header
/// model does not describe: the diagnostic record that every status-returning
/// call takes last, and the macros that the binding reads.
///
/// `maplibre_native_c.g.dart` re-exports this library.
// ignore_for_file: camel_case_types, constant_identifier_names
library;

import 'dart:ffi';

/// `MLN_DIAGNOSTIC_MESSAGE_CAPACITY` from `base.h`.
const int MLN_DIAGNOSTIC_MESSAGE_CAPACITY = 4096;

/// `MLN_ADAPTER_RESOURCE_KIND_ANY` from `callback_adapter.h`.
const int MLN_ADAPTER_RESOURCE_KIND_ANY = 0xffffffff;

/// `mln_diagnostic` from `base.h`.
final class mln_diagnostic extends Struct {
  @Uint32()
  external int size;

  @Array(MLN_DIAGNOSTIC_MESSAGE_CAPACITY)
  external Array<Char> message;
}
