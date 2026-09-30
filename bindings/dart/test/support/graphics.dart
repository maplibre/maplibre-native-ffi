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
      makeCurrent = library
          .lookupFunction<
            Bool Function(Pointer<Void>),
            bool Function(Pointer<Void>)
          >('mln_test_graphics_make_current'),
      lastError = library
          .lookupFunction<Pointer<Utf8> Function(), Pointer<Utf8> Function()>(
            'mln_test_graphics_last_error',
          );

  final Pointer<Void> Function(int) create;
  final void Function(Pointer<Void>) destroy;
  final bool Function(Pointer<Void>, Pointer<_Context>) getContext;
  final bool Function(Pointer<Void>) makeCurrent;
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
/// session shares the host's context, which only the caller driver can
/// drive.
bool get buildHasCoreWorkerTexture => _buildBackend() != _backendWgl;

/// A device or context standing in for the host's.
final class TestGraphics {
  TestGraphics._(this._handle, this._context);

  /// Creates the build's backend context, which [close] destroys. A build
  /// whose backend has no context here fails, rather than skipping.
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

  /// Creates the build's backend context, destroyed by a teardown that runs
  /// after every session the test attaches to it.
  factory TestGraphics.open() {
    final graphics = TestGraphics.create();
    addTearDown(graphics.close);
    return graphics;
  }

  final Pointer<Void> _handle;
  final Pointer<_Context> _context;
  var _closed = false;

  bool get isOpengl =>
      _context.ref.backend == _backendEgl ||
      _context.ref.backend == _backendWgl;

  /// Makes an OpenGL context current on the calling thread, as the host of a
  /// caller-driven session does on its graphics thread.
  void makeCurrent() {
    if (!_library.makeCurrent(_handle)) {
      fail(
        'mln_test_graphics_make_current: '
        '${_library.lastError().toDartString()}',
      );
    }
  }

  /// Attaches a session-owned texture on this context.
  ///
  /// A [shared] OpenGL session joins the host's context and takes the caller
  /// driver; otherwise an EGL session creates a dedicated context on the core
  /// worker. WGL has only the shared form.
  RenderSessionAttachment attachOwnedTexture(
    MapHandle map,
    RenderTargetExtent extent,
    RenderSessionAttachOptions options, {
    required bool shared,
  }) {
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
              ownership: shared
                  ? OpenglContextOwnership.shared
                  : OpenglContextOwnership.dedicated,
              data: OpenglContextDescriptorDataEgl(
                EglContextDescriptor(
                  display: pointer(context.eglDisplay),
                  config: pointer(context.eglConfig),
                  shareContext: shared
                      ? pointer(context.eglContext)
                      : NativePointer.nullPointer,
                  clientApi: OpenglClientApi.gles,
                ),
              ),
            ),
          ),
          options,
        );
      default:
        if (!shared) fail('a WGL session always shares the host context');
        return map.openglOwnedTextureAttach(
          OpenglOwnedTextureDescriptor(
            extent: extent,
            context: OpenglContextDescriptor(
              ownership: OpenglContextOwnership.shared,
              data: OpenglContextDescriptorDataWgl(
                WglContextDescriptor(
                  deviceContext: pointer(context.wglDeviceContext),
                  shareContext: pointer(context.wglContext),
                  getProcAddress: pointer(context.getProcAddress),
                ),
              ),
            ),
          ),
          options,
        );
    }
  }

  /// Destroys the context, on the thread that made it current when the
  /// session was caller-driven. A second close does nothing.
  void close() {
    if (_closed) return;
    _closed = true;
    _library.destroy(_handle);
    calloc.free(_context);
  }
}
