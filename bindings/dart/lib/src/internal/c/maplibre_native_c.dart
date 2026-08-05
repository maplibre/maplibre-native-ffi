import 'dart:ffi';

import 'package:ffi/ffi.dart';

import '../../error/maplibre_exception.dart';
import 'maplibre_native_c.g.dart' as generated;

/// C ABI contract version supported by this generated binding.
const int expectedCAbiVersion = 0;

/// Callback adapter entry points this binding registers with native code, plus
/// the ABI check that gates every use of them.
final class MaplibreNativeCApi {
  MaplibreNativeCApi._() {
    ensureAbiVersion();
  }

  /// Checks the native ABI version before any public handle is created.
  factory MaplibreNativeCApi.open() => MaplibreNativeCApi._();

  /// Copies the current thread-local native diagnostic message.
  String threadLastErrorMessage() {
    final pointer = generated.mln_thread_last_error_message();
    if (pointer == nullptr) {
      return '';
    }
    return pointer.cast<Utf8>().toDartString();
  }

  /// Native adapter callback for queued log records.
  Pointer<NativeFunction<generated.mln_log_callbackFunction>>
  adapterLogCallback() => Native.addressOf(generated.mln_adapter_log_callback);

  /// Destroys a copied adapter log record.
  void adapterLogRecordDestroy(Pointer<Void> record) =>
      generated.mln_adapter_log_record_destroy(record);

  /// Native adapter callback for exact URL resource providers.
  Pointer<NativeFunction<generated.mln_resource_provider_callbackFunction>>
  adapterResourceProviderRulesCallback() =>
      Native.addressOf(generated.mln_adapter_resource_provider_rules_callback);

  /// Native adapter callback for queued resource providers.
  Pointer<NativeFunction<generated.mln_resource_provider_callbackFunction>>
  adapterQueuedResourceProviderCallback() =>
      Native.addressOf(generated.mln_adapter_queued_resource_provider_callback);

  /// Native adapter callback for exact URL resource transforms.
  Pointer<NativeFunction<generated.mln_resource_transform_callbackFunction>>
  adapterResourceTransformRewriteCallback() => Native.addressOf(
    generated.mln_adapter_resource_transform_rewrite_callback,
  );

  /// Native adapter callback for routed HTTP header transforms.
  Pointer<NativeFunction<generated.mln_http_header_transform_callbackFunction>>
  adapterHttpHeaderTransformCallback() =>
      Native.addressOf(generated.mln_adapter_http_header_transform_callback);

  /// Destroys a copied adapter resource request record.
  void adapterResourceProviderRequestDestroy(Pointer<Void> request) =>
      generated.mln_adapter_resource_provider_request_destroy(request);
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
