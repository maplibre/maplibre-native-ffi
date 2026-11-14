/// The render backend under test, which a device that a test creates renders
/// with. A library may carry several backends, and its own default device
/// would always take the platform's default one.
library;

import 'dart:io';

import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';

const _backends = {
  'metal': (RenderBackend.metal, RenderBackendFlag.metal),
  'vulkan': (RenderBackend.vulkan, RenderBackendFlag.vulkan),
  'opengl': (RenderBackend.openGl, RenderBackendFlag.openGl),
};

/// The backend that the test task names: vulkan, opengl, or metal, in
/// MLN_FFI_TEST_BACKEND, or in the define of that name for a suite on a
/// device, which runs without the task's environment.
String? _testBackendName() {
  final environment = Platform.environment['MLN_FFI_TEST_BACKEND'];
  if (environment != null && environment.isNotEmpty) return environment;
  const defined = String.fromEnvironment('MLN_FFI_TEST_BACKEND');
  return defined.isEmpty ? null : defined;
}

/// The backend under test: the one that the test task names, which the loaded
/// library must carry, or where nothing names one, the one backend that the
/// library carries.
final RenderBackend backendUnderTest = () {
  final carried = supportedRenderBackendMask();
  final name = _testBackendName();
  if (name == null) {
    final backends = [
      for (final (backend, flag) in _backends.values)
        if (carried.contains(flag)) backend,
    ];
    if (backends.length != 1) {
      throw StateError(
        'the library carries $carried; MLN_FFI_TEST_BACKEND must name the '
        'backend under test',
      );
    }
    return backends.single;
  }
  final (backend, flag) =
      _backends[name] ?? (throw StateError('no tests drive the $name backend'));
  if (!carried.contains(flag)) {
    throw StateError(
      'the library lacks the backend under test, $name: $carried',
    );
  }
  return backend;
}();

/// Creates a GPU device of the backend under test, which the caller releases.
GpuDeviceHandle createDeviceUnderTest() => GpuDeviceHandle.create(
  options: GpuDeviceOptions(backend: backendUnderTest),
);
