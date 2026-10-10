/// GPU contexts from `mln_test_graphics`, the shared test library in
/// `tests/graphics`, loaded from the install prefix the suite runs against.
library;

import 'dart:ffi';
import 'dart:io';

import 'package:ffi/ffi.dart';
import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';
import 'package:test/test.dart';

const _backendMetal = 1;
const _backendVulkan = 2;
const _backendEgl = 3;
const _backendWgl = 4;

/// Mirrors `mln_test_graphics_context`.
final class _Context extends Struct {
  @Uint32()
  external int backend;
  @Uint32()
  external int vulkanQueueFamilyIndex;
  external Pointer<Void> metalDevice;
  external Pointer<Void> vulkanInstance;
  external Pointer<Void> vulkanPhysicalDevice;
  external Pointer<Void> vulkanDevice;
  external Pointer<Void> vulkanQueue;
  external Pointer<Void> vulkanGetInstanceProcAddr;
  external Pointer<Void> vulkanGetDeviceProcAddr;
  external Pointer<Void> eglDisplay;
  external Pointer<Void> eglConfig;
  external Pointer<Void> eglContext;
  external Pointer<Void> wglDeviceContext;
  external Pointer<Void> wglContext;
  external Pointer<Void> getProcAddress;
}

final class _Library {
  _Library(DynamicLibrary library)
    : create = library
          .lookupFunction<
            Pointer<Void> Function(Uint32),
            Pointer<Void> Function(int)
          >('mln_test_graphics_create'),
      destroy = library
          .lookupFunction<
            Void Function(Pointer<Void>),
            void Function(Pointer<Void>)
          >('mln_test_graphics_destroy'),
      getContext = library
          .lookupFunction<
            Bool Function(Pointer<Void>, Pointer<_Context>),
            bool Function(Pointer<Void>, Pointer<_Context>)
          >('mln_test_graphics_get_context'),
      lastError = library
          .lookupFunction<Pointer<Utf8> Function(), Pointer<Utf8> Function()>(
            'mln_test_graphics_last_error',
          );

  final Pointer<Void> Function(int) create;
  final void Function(Pointer<Void>) destroy;
  final bool Function(Pointer<Void>, Pointer<_Context>) getContext;
  final Pointer<Utf8> Function() lastError;
}

final _library = _Library(DynamicLibrary.open(_libraryPath()));

String _libraryPath() {
  final prefix = File(
    '.dart_tool/maplibre_native_install_dir',
  ).readAsStringSync().trim();
  if (Platform.isWindows) return '$prefix/bin/mln_test_graphics.dll';
  if (Platform.isMacOS) return '$prefix/lib/libmln_test_graphics.dylib';
  return '$prefix/lib/libmln_test_graphics.so';
}

/// The backend the native library was built for, which is the one backend
/// every render test runs on.
int _buildBackend() {
  final backends = supportedRenderBackendMask();
  if (backends.contains(RenderBackendFlag.metal)) return _backendMetal;
  if (backends.contains(RenderBackendFlag.vulkan)) return _backendVulkan;
  final providers = openglSupportedContextProviderMask();
  if (providers.contains(OpenglContextProviderFlag.egl)) return _backendEgl;
  if (providers.contains(OpenglContextProviderFlag.wgl)) return _backendWgl;
  throw StateError('no test graphics backend for $backends');
}

/// Whether this build can attach an owned texture to a core worker. A WGL
/// session shares the host's context, which only a caller-driven session can
/// drive.
bool get buildHasCoreWorkerTexture => _buildBackend() != _backendWgl;

/// Whether a core-worker session on this build exposes its frames to the host.
/// Metal and Vulkan sessions do. An OpenGL session exposes frames only from a
/// context that it shares with the host, which only a caller-driven session
/// can drive.
bool get buildExposesCoreWorkerFrames => switch (_buildBackend()) {
  _backendMetal || _backendVulkan => true,
  _ => false,
};

/// A device or context standing in for the host's.
final class TestGraphics {
  TestGraphics._(this._handle, this._context);

  /// Creates the build's backend context, destroyed by a teardown that runs
  /// after every session the test attaches to it. A build whose backend has
  /// no context here fails, rather than skipping.
  factory TestGraphics.open() {
    final graphics = TestGraphics.create();
    addTearDown(graphics.close);
    return graphics;
  }

  /// Creates the build's backend context, which the caller closes, for code
  /// that runs outside a test, such as a fixture process.
  factory TestGraphics.create() {
    final handle = _library.create(_buildBackend());
    if (handle == nullptr) {
      fail('mln_test_graphics_create: ${_library.lastError().toDartString()}');
    }
    final context = calloc<_Context>();
    if (!_library.getContext(handle, context)) {
      final error = _library.lastError().toDartString();
      calloc.free(context);
      _library.destroy(handle);
      fail('mln_test_graphics_get_context: $error');
    }
    return TestGraphics._(handle, context);
  }

  final Pointer<Void> _handle;
  final Pointer<_Context> _context;
  var _closed = false;

  /// Attaches a session-owned texture on this device for a core worker. On
  /// EGL, the session creates a dedicated context that shares nothing with
  /// the host's.
  RenderSessionAttachment attachOwnedTexture(
    MapHandle map,
    RenderTargetExtent extent,
    RenderSessionAttachOptions options,
  ) {
    final context = _context.ref;
    NativePointer pointer(Pointer<Void> value) => NativePointer(value.address);
    switch (context.backend) {
      case _backendMetal:
        return map.metalOwnedTextureAttach(
          MetalOwnedTextureDescriptor(
            extent: extent,
            context: MetalContextDescriptor(
              device: pointer(context.metalDevice),
            ),
          ),
          options,
        );
      case _backendVulkan:
        return map.vulkanOwnedTextureAttach(
          VulkanOwnedTextureDescriptor(
            extent: extent,
            context: VulkanContextDescriptor(
              instance: pointer(context.vulkanInstance),
              physicalDevice: pointer(context.vulkanPhysicalDevice),
              device: pointer(context.vulkanDevice),
              graphicsQueue: pointer(context.vulkanQueue),
              graphicsQueueFamilyIndex: context.vulkanQueueFamilyIndex,
              getInstanceProcAddr: pointer(context.vulkanGetInstanceProcAddr),
              getDeviceProcAddr: pointer(context.vulkanGetDeviceProcAddr),
            ),
          ),
          options,
        );
      case _backendEgl:
        return map.openglOwnedTextureAttach(
          OpenglOwnedTextureDescriptor(
            extent: extent,
            context: OpenglContextDescriptor(
              ownership: OpenglContextOwnership.dedicated,
              data: OpenglContextDescriptorDataEgl(
                EglContextDescriptor(
                  display: pointer(context.eglDisplay),
                  config: pointer(context.eglConfig),
                  shareContext: NativePointer.nullPointer,
                  clientApi: OpenglClientApi.gles,
                ),
              ),
            ),
          ),
          options,
        );
      default:
        fail('a WGL texture has no core-worker session');
    }
  }

  /// Destroys the context. A second close does nothing.
  void close() {
    if (_closed) return;
    _closed = true;
    _library.destroy(_handle);
    calloc.free(_context);
  }
}
