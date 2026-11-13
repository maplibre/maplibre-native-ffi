/// GPU contexts from `mln_test_graphics`, the shared test library in
/// `tests/graphics`, loaded from the install prefix the suite runs against.
///
/// A test attaches a borrowed texture in such a context only for what an
/// offscreen session lacks: acquired frames, and a device that wraps the host's
/// Vulkan device with a queue lock. Both exist only on Metal and Vulkan builds,
/// so an OpenGL build opens no context here.
library;

import 'dart:ffi';
import 'dart:io';

import 'package:ffi/ffi.dart';
import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';
import 'package:test/test.dart';

const _backendMetal = 1;
const _backendVulkan = 2;

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
  external Pointer<Pointer<Utf8>> vulkanDeviceExtensions;
  @Uint32()
  external int vulkanDeviceExtensionCount;
  external Pointer<Void> eglDisplay;
  external Pointer<Void> eglConfig;
  external Pointer<Void> eglContext;
  external Pointer<Void> wglDeviceContext;
  external Pointer<Void> wglContext;
  external Pointer<Void> getProcAddress;
}

/// Mirrors `mln_test_graphics_texture_info`.
final class _TextureInfo extends Struct {
  @Uint64()
  external int vulkanImage;
  @Uint64()
  external int vulkanImageView;
  external Pointer<Void> metalTexture;
  @Uint32()
  external int width;
  @Uint32()
  external int height;
  @Uint32()
  external int format;
  @Uint32()
  external int vulkanFinalLayout;
  @Uint32()
  external int openGlTexture;
  @Uint32()
  external int openGlTarget;
}

/// The fields of a texture's info that a borrowed descriptor names.
typedef _Texture = ({
  NativePointer metalTexture,
  int vulkanImage,
  int vulkanImageView,
  int format,
  int vulkanFinalLayout,
});

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
      textureCreate = library
          .lookupFunction<
            Pointer<Void> Function(Pointer<Void>, Uint32, Uint32),
            Pointer<Void> Function(Pointer<Void>, int, int)
          >('mln_test_graphics_texture_create'),
      textureDestroy = library
          .lookupFunction<
            Void Function(Pointer<Void>),
            void Function(Pointer<Void>)
          >('mln_test_graphics_texture_destroy'),
      textureGetInfo = library
          .lookupFunction<
            Bool Function(Pointer<Void>, Pointer<_TextureInfo>),
            bool Function(Pointer<Void>, Pointer<_TextureInfo>)
          >('mln_test_graphics_texture_get_info'),
      lastError = library
          .lookupFunction<Pointer<Utf8> Function(), Pointer<Utf8> Function()>(
            'mln_test_graphics_last_error',
          );

  final Pointer<Void> Function(int) create;
  final void Function(Pointer<Void>) destroy;
  final bool Function(Pointer<Void>, Pointer<_Context>) getContext;
  final Pointer<Void> Function(Pointer<Void>, int, int) textureCreate;
  final void Function(Pointer<Void>) textureDestroy;
  final bool Function(Pointer<Void>, Pointer<_TextureInfo>) textureGetInfo;
  final Pointer<Utf8> Function() lastError;
}

final _library = _Library(DynamicLibrary.open(_libraryPath()));

String _libraryPath() {
  // The install prefix that `mise run //bindings/dart:test` links. A hardened
  // Dart VM refuses to load a library by relative path.
  if (Platform.isAndroid) return 'libmln_test_graphics.so';
  if (Platform.isIOS) {
    final app = File(Platform.resolvedExecutable).parent.path;
    return '$app/Frameworks/mln_test_graphics.framework/mln_test_graphics';
  }
  final prefix = Directory('.dart_tool/native').absolute.path;
  if (Platform.isWindows) return '$prefix/bin/mln_test_graphics.dll';
  if (Platform.isMacOS) return '$prefix/lib/libmln_test_graphics.dylib';
  return '$prefix/lib/libmln_test_graphics.so';
}

/// The backend the native library was built for, which is the one backend
/// every render test runs on, or null on an OpenGL build.
int? _buildBackend() {
  final backends = supportedRenderBackendMask();
  if (backends.contains(RenderBackendFlag.metal)) return _backendMetal;
  if (backends.contains(RenderBackendFlag.vulkan)) return _backendVulkan;
  return null;
}

/// Whether this build attaches sessions to Vulkan, the one backend whose
/// devices take a queue lock.
bool get buildAttachesVulkan => _buildBackend() == _backendVulkan;

/// Whether [TestGraphics.open] opens a context on this build.
bool get buildOpensTestGraphics => _buildBackend() != null;

/// Whether a core-worker session on this build exposes its frames to the host.
/// Metal and Vulkan sessions do. An OpenGL session exposes frames only from a
/// context that it shares with the host, which only a caller-driven session
/// can drive.
bool get buildExposesCoreWorkerFrames => _buildBackend() != null;

/// A device or context standing in for the host's.
final class TestGraphics {
  TestGraphics._(this._handle, this._context);

  /// Creates the build's backend context, destroyed by a teardown that runs
  /// after every session the test attaches to it. A build whose backend has
  /// no context here fails, rather than skipping.
  factory TestGraphics.open() {
    final backend = _buildBackend();
    if (backend == null) fail('an OpenGL build has no test graphics context');
    final handle = _library.create(backend);
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
    final graphics = TestGraphics._(handle, context);
    addTearDown(graphics.close);
    return graphics;
  }

  final Pointer<Void> _handle;
  final Pointer<_Context> _context;
  final _textures = <Pointer<Void>>[];
  var _closed = false;

  /// Wraps this context's device in a GPU device, which a teardown releases.
  /// A Vulkan device takes [queueLock], the lock on the queue that the host
  /// submits to.
  GpuDeviceHandle wrapDevice({QueueLock? queueLock}) {
    final context = _context.ref;
    NativePointer pointer(Pointer<Void> value) => NativePointer(value.address);
    final GpuDeviceHandle device;
    switch (context.backend) {
      case _backendMetal:
        device = GpuDeviceHandle.wrapMetal(
          MetalDeviceDescriptor(device: pointer(context.metalDevice)),
        );
      case _backendVulkan:
        device = GpuDeviceHandle.wrapVulkan(
          VulkanDeviceDescriptor(
            context: VulkanContextDescriptor(
              instance: pointer(context.vulkanInstance),
              physicalDevice: pointer(context.vulkanPhysicalDevice),
              device: pointer(context.vulkanDevice),
              graphicsQueue: pointer(context.vulkanQueue),
              graphicsQueueFamilyIndex: context.vulkanQueueFamilyIndex,
              getInstanceProcAddr: pointer(context.vulkanGetInstanceProcAddr),
              getDeviceProcAddr: pointer(context.vulkanGetDeviceProcAddr),
            ),
            queueLock: queueLock,
            enabledExtensions: [
              for (var i = 0; i < context.vulkanDeviceExtensionCount; i++)
                context.vulkanDeviceExtensions[i].toDartString(),
            ],
          ),
        );
      default:
        fail('no GPU device for backend ${context.backend}');
    }
    addTearDown(device.release);
    return device;
  }

  /// Attaches a ring of one texture on this context for a core worker, at
  /// [size] on a map of scale factor 1, on [device] or else on a device that
  /// wraps this context's. [close] destroys the texture, so the session must
  /// retire first.
  RenderSessionHandle attachBorrowedTexture(
    MapHandle map,
    LogicalSize size,
    RenderSessionAttachOptions options, {
    GpuDeviceHandle? device,
  }) {
    final width = size.width;
    final height = size.height;
    final texture = _createTexture(width, height);
    final backend = _context.ref.backend;
    final target = switch (backend) {
      _backendMetal => RenderTarget.metalBorrowedTextures(
        MetalBorrowedTexturesTarget(
          logicalSize: size,
          physicalWidth: width,
          physicalHeight: height,
          textures: [MetalBorrowedTexture(texture: texture.metalTexture)],
        ),
      ),
      _backendVulkan => RenderTarget.vulkanBorrowedTextures(
        VulkanBorrowedTexturesTarget(
          logicalSize: size,
          physicalWidth: width,
          physicalHeight: height,
          textures: [
            VulkanBorrowedTexture(
              image: texture.vulkanImage,
              imageView: texture.vulkanImageView,
            ),
          ],
          format: texture.format,
          finalLayout: texture.vulkanFinalLayout,
        ),
      ),
      _ => fail('no borrowed texture for backend $backend'),
    };
    return map.attach(target, options, device: device ?? wrapDevice());
  }

  /// Creates a texture that [close] destroys, and returns its info.
  _Texture _createTexture(int width, int height) {
    final texture = _library.textureCreate(_handle, width, height);
    if (texture == nullptr) {
      fail(
        'mln_test_graphics_texture_create: '
        '${_library.lastError().toDartString()}',
      );
    }
    _textures.add(texture);
    return using((arena) {
      final info = arena<_TextureInfo>();
      if (!_library.textureGetInfo(texture, info)) {
        fail(
          'mln_test_graphics_texture_get_info: '
          '${_library.lastError().toDartString()}',
        );
      }
      return (
        metalTexture: NativePointer(info.ref.metalTexture.address),
        vulkanImage: info.ref.vulkanImage,
        vulkanImageView: info.ref.vulkanImageView,
        format: info.ref.format,
        vulkanFinalLayout: info.ref.vulkanFinalLayout,
      );
    });
  }

  /// Destroys the textures and then the context. A second close does nothing.
  void close() {
    if (_closed) return;
    _closed = true;
    _textures.forEach(_library.textureDestroy);
    _library.destroy(_handle);
    calloc.free(_context);
  }
}
