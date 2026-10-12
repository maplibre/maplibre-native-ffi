/// Identity of the native library code asset.
///
/// The generated `@Native` declarations name [nativeAssetId] as their default
/// asset and `hook/build.dart` declares the asset under the same name.
/// A mismatch only surfaces at run time, so both read it from here.
library;

const String nativeAssetName = 'src/internal/c/maplibre_native_c.g.dart';

const String nativeAssetId = 'package:maplibre_native_ffi/$nativeAssetName';
