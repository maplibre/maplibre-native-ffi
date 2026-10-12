import 'dart:convert';
import 'dart:ffi';

import 'package:ffi/ffi.dart';

import '../../error/maplibre_exception.dart';
import '../status/status.dart';
import 'maplibre_native_c.g.dart' as generated;

/// C ABI contract version supported by this generated binding.
const int expectedCAbiVersion = 0;

/// The diagnostic that every status-returning call made from this isolate
/// fills.
///
/// Native code writes the message as the call returns, and the binding reads
/// it before making another call, so one buffer serves each isolate, including
/// calls made from reentrant synchronous callbacks. A finalizer frees it once
/// the isolate is gone.
Pointer<generated.mln_diagnostic> get nativeDiagnostic => _diagnostic.pointer;

final _diagnostic = _NativeDiagnostic();

final class _NativeDiagnostic implements Finalizable {
  _NativeDiagnostic() : pointer = calloc<generated.mln_diagnostic>() {
    pointer.ref.size = sizeOf<generated.mln_diagnostic>();
    _finalizer.attach(this, pointer.cast());
  }

  static final _finalizer = NativeFinalizer(calloc.nativeFree);

  final Pointer<generated.mln_diagnostic> pointer;
}

/// Copies the message of the last status-returning call from this isolate.
String nativeDiagnosticMessage() {
  final message = _diagnostic.pointer.ref.message.elements;
  final bytes = <int>[];
  for (var index = 0; index < message.length; index++) {
    final byte = message[index] & 0xff;
    if (byte == 0) {
      break;
    }
    bytes.add(byte);
  }
  // Truncation can split a multibyte sequence.
  return utf8.decode(bytes, allowMalformed: true);
}

/// Throws the public exception for a failed status-returning call, carrying
/// the message that call wrote to [nativeDiagnostic].
void checkNativeCall(int status) =>
    checkNativeStatus(status, nativeDiagnosticMessage);

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
