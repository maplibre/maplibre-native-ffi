import 'dart:ffi';

import 'package:ffi/ffi.dart';

import '../../error/maplibre_exception.dart';
import '../loader/native_library.dart';
import 'maplibre_native_c.g.dart' as generated;

/// C ABI contract version supported by this generated binding.
const int expectedCAbiVersion = 0;

/// Native library handle plus the callback adapter entry points this
/// binding registers with native code.
final class MaplibreNativeCApi {
  MaplibreNativeCApi._(this.library)
    : raw = generated.MaplibreNativeC(library) {
    validateCAbiVersion(raw.mln_c_version());
  }

  /// Opens the native library and resolves generated C symbols lazily.
  factory MaplibreNativeCApi.open({String? path}) =>
      MaplibreNativeCApi._(openMaplibreNativeCLibrary(path: path));

  /// Native library that owns the resolved C symbols.
  final DynamicLibrary library;

  /// Generated MapLibre Native C declarations.
  final generated.MaplibreNativeC raw;

  /// Copies the current thread-local native diagnostic message.
  String threadLastErrorMessage() {
    final pointer = raw.mln_thread_last_error_message();
    if (pointer == nullptr) {
      return '';
    }
    return pointer.cast<Utf8>().toDartString();
  }

  /// Native adapter callback for queued log records.
  Pointer<NativeFunction<generated.mln_log_callbackFunction>>
  adapterLogCallback() => library.lookup('mln_adapter_log_callback');

  /// Destroys a copied adapter log record.
  void adapterLogRecordDestroy(Pointer<Void> record) {
    library.lookupFunction<
      Void Function(Pointer<Void>),
      void Function(Pointer<Void>)
    >('mln_adapter_log_record_destroy')(record);
  }

  /// Native adapter callback for exact URL resource providers.
  Pointer<NativeFunction<generated.mln_resource_provider_callbackFunction>>
  adapterResourceProviderRulesCallback() =>
      library.lookup('mln_adapter_resource_provider_rules_callback');

  /// Native adapter callback for queued resource providers.
  Pointer<NativeFunction<generated.mln_resource_provider_callbackFunction>>
  adapterQueuedResourceProviderCallback() =>
      library.lookup('mln_adapter_queued_resource_provider_callback');

  /// Native adapter callback for exact URL resource transforms.
  Pointer<NativeFunction<generated.mln_resource_transform_callbackFunction>>
  adapterResourceTransformRewriteCallback() =>
      library.lookup('mln_adapter_resource_transform_rewrite_callback');

  /// Destroys a copied adapter resource request record.
  void adapterResourceProviderRequestDestroy(Pointer<Void> request) {
    library.lookupFunction<
      Void Function(Pointer<Void>),
      void Function(Pointer<Void>)
    >('mln_adapter_resource_provider_request_destroy')(request);
  }
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
