/// Identity of the native library code asset.
///
/// `tool/ffigen.dart` stamps [nativeAssetId] into the generated `@Native`
/// annotations and `hook/build.dart` declares the asset under the same name.
/// A mismatch between them surfaces only at run time, as a failure to resolve
/// the asset, so both read it from here.
library;

/// Asset name within this package, as `hook/build.dart` declares it.
const String nativeAssetName = 'src/internal/c/maplibre_native_c.g.dart';

/// Fully qualified asset id, as the generated annotations reference it.
const String nativeAssetId = 'package:maplibre_native_ffi/$nativeAssetName';
