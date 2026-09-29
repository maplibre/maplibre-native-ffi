import 'dart:ffi';

import 'package:ffi/ffi.dart';

import '../../error/maplibre_exception.dart';
import 'maplibre_native_c.g.dart' as generated;

/// C ABI contract version supported by this generated binding.
const int expectedCAbiVersion = 0;

/// Copies the current thread-local native diagnostic message.
///
/// Native code sets it on every non-OK synchronous return, and leaves it empty
/// when the call reported no diagnostic.
String threadLastErrorMessage() {
  final pointer = generated.mln_thread_last_error_message();
  if (pointer == nullptr) {
    return '';
  }
  return pointer.cast<Utf8>().toDartString();
}

bool _abiVersionChecked = false;

/// Validates the native C ABI once, before the binding relies on it.
///
/// Call this as the first statement of any public entry point that can reach C
/// without a handle this isolate already created: the statics on [Maplibre],
/// and the types built to cross isolates. Everything else is reachable only
/// through such an entry point. The status checkers call it as a backstop.
void ensureAbiVersion() {
  if (_abiVersionChecked) {
    return;
  }
  validateCAbiVersion(generated.mln_c_version());
  _abiVersionChecked = true;
}

/// Validates a reported C ABI version before public handles are created.
void validateCAbiVersion(int actualVersion) {
  if (actualVersion == expectedCAbiVersion) {
    return;
  }
  throw MaplibreException.abiVersionMismatch(
    'MapLibre Native C ABI version $actualVersion is incompatible with this '
    'binding; expected $expectedCAbiVersion.',
  );
}
