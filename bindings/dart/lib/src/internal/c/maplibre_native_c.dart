import 'dart:ffi';

import 'package:ffi/ffi.dart';

import '../../error/maplibre_exception.dart';
import '../loader/native_library.dart';
import 'maplibre_native_c.g.dart' as generated;

/// C ABI contract version supported by this generated binding.
const int expectedCAbiVersion = 0;

/// Native library handle plus binding-private Dart shim entry points.
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

  /// Native Dart-shim callback for queued log records.
  Pointer<NativeFunction<generated.mln_log_callbackFunction>>
  dartLogCallback() => library.lookup('mln_dart_log_callback');

  /// Destroys a copied Dart-shim log record.
  void dartLogRecordDestroy(Pointer<Void> record) {
    library.lookupFunction<
      Void Function(Pointer<Void>),
      void Function(Pointer<Void>)
    >('mln_dart_log_record_destroy')(record);
  }

  /// Native Dart-shim callback for exact URL resource providers.
  Pointer<NativeFunction<generated.mln_resource_provider_callbackFunction>>
  dartResourceProviderRulesCallback() =>
      library.lookup('mln_dart_resource_provider_rules_callback');

  /// Native Dart-shim callback for queued resource providers.
  Pointer<NativeFunction<generated.mln_resource_provider_callbackFunction>>
  dartQueuedResourceProviderCallback() =>
      library.lookup('mln_dart_queued_resource_provider_callback');

  /// Native Dart-shim callback for exact URL resource transforms.
  Pointer<NativeFunction<generated.mln_resource_transform_callbackFunction>>
  dartResourceTransformRewriteCallback() =>
      library.lookup('mln_dart_resource_transform_rewrite_callback');

  /// Destroys a copied Dart-shim resource request record.
  void dartResourceProviderRequestDestroy(Pointer<Void> request) {
    library.lookupFunction<
      Void Function(Pointer<Void>),
      void Function(Pointer<Void>)
    >('mln_dart_resource_provider_request_destroy')(request);
  }

  /// Invokes a custom-geometry tile callback through the native Dart shim.
  void dartTestInvokeCustomGeometryTileCallback(
    Pointer<
      NativeFunction<generated.mln_custom_geometry_source_tile_callbackFunction>
    >
    callback,
    Pointer<Void> userData,
    generated.mln_canonical_tile_id tileId,
  ) {
    library.lookupFunction<
      Void Function(
        Pointer<
          NativeFunction<
            generated.mln_custom_geometry_source_tile_callbackFunction
          >
        >,
        Pointer<Void>,
        generated.mln_canonical_tile_id,
      ),
      void Function(
        Pointer<
          NativeFunction<
            generated.mln_custom_geometry_source_tile_callbackFunction
          >
        >,
        Pointer<Void>,
        generated.mln_canonical_tile_id,
      )
    >('mln_dart_test_invoke_custom_geometry_tile_callback')(
      callback,
      userData,
      tileId,
    );
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
