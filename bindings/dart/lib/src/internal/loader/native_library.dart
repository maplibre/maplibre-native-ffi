import 'dart:ffi';
import 'dart:io';

/// Opens the MapLibre Native C library for the low-level binding.
DynamicLibrary openMaplibreNativeCLibrary({String? path}) {
  final explicitPath = path ?? Platform.environment['MLN_FFI_NATIVE_LIBRARY'];
  if (explicitPath != null && explicitPath.isNotEmpty) {
    return DynamicLibrary.open(explicitPath);
  }

  final buildDir = Platform.environment['MLN_FFI_BUILD_DIR'];
  if (buildDir != null && buildDir.isNotEmpty) {
    return DynamicLibrary.open(_join(buildDir, _platformLibraryFileName()));
  }

  return DynamicLibrary.open(_platformLibraryFileName());
}

String _platformLibraryFileName() {
  if (Platform.isMacOS || Platform.isIOS) {
    return 'libmaplibre-native-c.dylib';
  }
  if (Platform.isWindows) {
    return 'maplibre-native-c.dll';
  }
  return 'libmaplibre-native-c.so';
}

String _join(String directory, String fileName) {
  if (directory.endsWith(Platform.pathSeparator)) {
    return '$directory$fileName';
  }
  return '$directory${Platform.pathSeparator}$fileName';
}
