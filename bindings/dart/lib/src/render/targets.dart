import 'dart:ffi';
import 'dart:typed_data';

import 'package:ffi/ffi.dart';

import '../internal/c/maplibre_native_c.dart';
import '../internal/c/maplibre_native_c.g.dart' as raw;
import '../internal/status/status.dart';
import 'native_pointer.dart';

final MaplibreNativeCApi _renderTargetApi = MaplibreNativeCApi.open();

/// Physical render target dimensions in device pixels.
final class PhysicalRenderTargetSize {
  /// Creates a physical render target size.
  const PhysicalRenderTargetSize({required this.width, required this.height});

  /// Physical width in device pixels.
  final int width;

  /// Physical height in device pixels.
  final int height;
}

/// Logical render target extent in UI pixels.
final class RenderTargetExtent {
  /// Creates a render target extent.
  const RenderTargetExtent({
    required this.width,
    required this.height,
    this.scaleFactor = 1,
  });

  /// Logical map width in UI pixels.
  final int width;

  /// Logical map height in UI pixels.
  final int height;

  /// UI-to-device pixel scale.
  final double scaleFactor;

  /// Returns the physical size as `ceil(logical * scaleFactor)` per dimension.
  ///
  /// Session-owned texture and surface targets use this size. Borrowed texture
  /// targets state their physical dimensions independently.
  PhysicalRenderTargetSize physicalSize() {
    return using((arena) {
      final nativeExtent = arena<raw.mln_render_target_extent>();
      nativeExtent.ref
        ..size = sizeOf<raw.mln_render_target_extent>()
        ..width = _positiveExtentDimension(width, 'render target width')
        ..height = _positiveExtentDimension(height, 'render target height')
        ..scale_factor = scaleFactor;
      final physicalWidth = arena<Uint32>();
      final physicalHeight = arena<Uint32>();
      final status = _renderTargetApi.raw
          .mln_render_target_extent_physical_size(
            nativeExtent,
            physicalWidth,
            physicalHeight,
          );
      checkNativeStatus(status.value, _renderTargetApi.threadLastErrorMessage);
      return PhysicalRenderTargetSize(
        width: physicalWidth.value,
        height: physicalHeight.value,
      );
    });
  }
}

int _positiveExtentDimension(int value, String name) {
  if (value <= 0 || value > 0xffffffff) {
    throwInvalidArgument('$name must be in 1...4294967295');
  }
  return value;
}

/// Closeable native byte buffer for reusable render readback storage.
final class NativeBuffer implements Finalizable {
  /// Allocates [byteLength] bytes of native memory.
  NativeBuffer(int byteLength)
    : this._(byteLength, _allocateNativeBuffer(byteLength));

  NativeBuffer._(this.byteLength, this._pointer) {
    _finalizer.attach(
      this,
      _pointer.cast<Void>(),
      detach: _finalizerDetachToken,
    );
  }

  static final NativeFinalizer _finalizer = NativeFinalizer(calloc.nativeFree);

  /// Allocated byte length.
  final int byteLength;

  Pointer<Uint8> _pointer;
  final Object _finalizerDetachToken = Object();

  /// Whether this buffer has been freed.
  bool get isClosed => _pointer == nullptr;

  /// Native storage address for FFI integrations.
  ///
  /// The pointer is valid only until [close]. Prefer [copyBytes] and
  /// [writeBytes] when direct FFI access is unnecessary.
  NativePointer get unsafePointer {
    if (_pointer == nullptr) {
      throw StateError('native buffer has been closed');
    }
    return NativePointer(_pointer.address);
  }

  /// Copies bytes from native storage into Dart-owned memory.
  Uint8List copyBytes({int? length}) {
    final viewLength = length ?? byteLength;
    if (viewLength < 0 || viewLength > byteLength) {
      throw RangeError.range(viewLength, 0, byteLength, 'length');
    }
    return Uint8List.fromList(_livePointer.asTypedList(viewLength));
  }

  /// Copies [bytes] into native storage starting at [offset].
  void writeBytes(Uint8List bytes, {int offset = 0}) {
    if (offset < 0 || offset > byteLength) {
      throw RangeError.range(offset, 0, byteLength, 'offset');
    }
    if (bytes.length > byteLength - offset) {
      throw RangeError.range(
        bytes.length,
        0,
        byteLength - offset,
        'bytes.length',
      );
    }
    (_livePointer + offset).asTypedList(bytes.length).setAll(0, bytes);
  }

  /// Frees the native storage. The buffer must not be used afterwards.
  void close() {
    if (_pointer == nullptr) {
      return;
    }
    _finalizer.detach(_finalizerDetachToken);
    calloc.free(_pointer);
    _pointer = nullptr;
  }

  Pointer<Uint8> get _livePointer {
    if (_pointer == nullptr) {
      throw StateError('native buffer has been closed');
    }
    return _pointer;
  }
}

Pointer<Uint8> _allocateNativeBuffer(int byteLength) {
  if (byteLength <= 0) {
    throw ArgumentError.value(byteLength, 'byteLength', 'must be positive');
  }
  return calloc<Uint8>(byteLength);
}

/// Metal backend context fields shared by Metal render targets.
final class MetalContextDescriptor {
  /// Creates a Metal context descriptor.
  const MetalContextDescriptor({required this.device});

  /// Borrowed or retained native Metal device pointer, depending on target kind.
  final NativePointer device;
}

/// Vulkan backend context fields shared by Vulkan render targets.
final class VulkanContextDescriptor {
  /// Creates a Vulkan context descriptor.
  const VulkanContextDescriptor({
    required this.instance,
    required this.physicalDevice,
    required this.device,
    required this.graphicsQueue,
    required this.graphicsQueueFamilyIndex,
    this.getInstanceProcAddr = NativePointer.nullPointer,
    this.getDeviceProcAddr = NativePointer.nullPointer,
  });

  /// Borrowed VkInstance.
  final NativePointer instance;

  /// Borrowed VkPhysicalDevice.
  final NativePointer physicalDevice;

  /// Borrowed VkDevice.
  final NativePointer device;

  /// Borrowed graphics VkQueue.
  final NativePointer graphicsQueue;

  /// Queue family index for [graphicsQueue].
  final int graphicsQueueFamilyIndex;

  /// Optional `PFN_vkGetInstanceProcAddr` for the host Vulkan loader.
  final NativePointer getInstanceProcAddr;

  /// Optional `PFN_vkGetDeviceProcAddr` for the host Vulkan loader.
  final NativePointer getDeviceProcAddr;
}

/// OpenGL context provider support flag reported by the native library build.
final class OpenGLContextProviderMask {
  /// Creates an OpenGL context provider mask from raw C flag bits.
  const OpenGLContextProviderMask(this.bits);

  /// WGL context provider support bit.
  static const wgl = OpenGLContextProviderMask(1 << 0);

  /// EGL context provider support bit.
  static const egl = OpenGLContextProviderMask(1 << 1);

  /// Raw provider mask bits.
  final int bits;

  /// Returns true when all [provider] bits are present in this mask.
  bool contains(OpenGLContextProviderMask provider) =>
      (bits & provider.bits) == provider.bits;

  @override
  String toString() =>
      'OpenGLContextProviderMask[bits=0x${bits.toRadixString(16)}]';
}

/// OpenGL backend context fields shared by OpenGL render targets.
sealed class OpenGLContextDescriptor {
  const OpenGLContextDescriptor();
}

/// WGL context fields shared by OpenGL render targets on Windows.
final class WglContextDescriptor extends OpenGLContextDescriptor {
  /// Creates a WGL context descriptor.
  const WglContextDescriptor({
    required this.deviceContext,
    required this.shareContext,
    this.getProcAddress = NativePointer.nullPointer,
  });

  /// Borrowed HDC used to create a shared session context.
  final NativePointer deviceContext;

  /// Borrowed HGLRC whose share group the session context joins.
  final NativePointer shareContext;

  /// Optional `wglGetProcAddress`-compatible function for the host loader.
  final NativePointer getProcAddress;
}

/// EGL context fields shared by OpenGL render targets on Linux.
final class EglContextDescriptor extends OpenGLContextDescriptor {
  /// Creates an EGL context descriptor.
  const EglContextDescriptor({
    required this.display,
    required this.config,
    required this.shareContext,
    this.getProcAddress = NativePointer.nullPointer,
  });

  /// Borrowed EGLDisplay.
  final NativePointer display;

  /// Borrowed EGLConfig used to create a shared session context.
  final NativePointer config;

  /// Borrowed EGLContext whose share group the session context joins.
  final NativePointer shareContext;

  /// Optional `eglGetProcAddress`-compatible function for the host loader.
  final NativePointer getProcAddress;
}

/// Metal native surface session attachment options.
final class MetalSurfaceDescriptor {
  /// Creates a Metal surface descriptor.
  const MetalSurfaceDescriptor({
    required this.extent,
    required this.context,
    required this.layer,
  });

  /// Logical surface extent.
  final RenderTargetExtent extent;

  /// Metal backend context.
  final MetalContextDescriptor context;

  /// Borrowed `CAMetalLayer*` / `CA::MetalLayer*`.
  final NativePointer layer;
}

/// Vulkan native surface session attachment options.
final class VulkanSurfaceDescriptor {
  /// Creates a Vulkan surface descriptor.
  const VulkanSurfaceDescriptor({
    required this.extent,
    required this.context,
    required this.surface,
  });

  /// Logical surface extent.
  final RenderTargetExtent extent;

  /// Borrowed Vulkan context.
  final VulkanContextDescriptor context;

  /// Borrowed `VkSurfaceKHR`.
  final NativePointer surface;
}

/// OpenGL native surface session attachment options.
final class OpenGLSurfaceDescriptor {
  /// Creates an OpenGL surface descriptor.
  const OpenGLSurfaceDescriptor({
    required this.extent,
    required this.context,
    required this.surface,
  });

  /// Logical surface extent.
  final RenderTargetExtent extent;

  /// Borrowed OpenGL context.
  final OpenGLContextDescriptor context;

  /// Borrowed platform surface handle: HDC for WGL, EGLSurface for EGL.
  final NativePointer surface;
}

/// Metal texture session attachment options for a session-owned target.
final class MetalOwnedTextureDescriptor {
  /// Creates a Metal owned-texture descriptor.
  const MetalOwnedTextureDescriptor({
    required this.extent,
    required this.context,
  });

  /// Logical texture extent.
  final RenderTargetExtent extent;

  /// Metal backend context.
  final MetalContextDescriptor context;
}

/// Metal caller-owned texture session attachment options.
final class MetalBorrowedTextureDescriptor {
  /// Creates a Metal borrowed-texture descriptor.
  const MetalBorrowedTextureDescriptor({
    required this.extent,
    required this.physicalWidth,
    required this.physicalHeight,
    required this.texture,
  });

  /// Logical texture extent.
  final RenderTargetExtent extent;

  /// Physical texture width in device pixels.
  final int physicalWidth;

  /// Physical texture height in device pixels.
  final int physicalHeight;

  /// Borrowed `id<MTLTexture>` / `MTL::Texture*`.
  final NativePointer texture;
}

/// Vulkan texture session attachment options for a session-owned target.
final class VulkanOwnedTextureDescriptor {
  /// Creates a Vulkan owned-texture descriptor.
  const VulkanOwnedTextureDescriptor({
    required this.extent,
    required this.context,
  });

  /// Logical texture extent.
  final RenderTargetExtent extent;

  /// Borrowed Vulkan context.
  final VulkanContextDescriptor context;
}

/// Vulkan caller-owned texture session attachment options.
final class VulkanBorrowedTextureDescriptor {
  /// Creates a Vulkan borrowed-texture descriptor.
  const VulkanBorrowedTextureDescriptor({
    required this.extent,
    required this.physicalWidth,
    required this.physicalHeight,
    required this.context,
    required this.image,
    required this.imageView,
    required this.format,
    required this.initialLayout,
    required this.finalLayout,
  });

  /// Logical texture extent.
  final RenderTargetExtent extent;

  /// Physical image width in device pixels.
  final int physicalWidth;

  /// Physical image height in device pixels.
  final int physicalHeight;

  /// Borrowed Vulkan context.
  final VulkanContextDescriptor context;

  /// Borrowed VkImage.
  final NativePointer image;

  /// Borrowed VkImageView.
  final NativePointer imageView;

  /// Backend-native VkFormat value.
  final int format;

  /// Backend-native VkImageLayout at render-pass begin.
  final int initialLayout;

  /// Backend-native VkImageLayout left after rendering succeeds.
  final int finalLayout;
}

/// OpenGL texture session attachment options for a session-owned target.
final class OpenGLOwnedTextureDescriptor {
  /// Creates an OpenGL owned-texture descriptor.
  const OpenGLOwnedTextureDescriptor({
    required this.extent,
    required this.context,
  });

  /// Logical texture extent.
  final RenderTargetExtent extent;

  /// Borrowed OpenGL context.
  final OpenGLContextDescriptor context;
}

/// OpenGL caller-owned texture session attachment options.
final class OpenGLBorrowedTextureDescriptor {
  /// Creates an OpenGL borrowed-texture descriptor.
  const OpenGLBorrowedTextureDescriptor({
    required this.extent,
    required this.physicalWidth,
    required this.physicalHeight,
    required this.context,
    required this.texture,
    required this.target,
  });

  /// Logical texture extent.
  final RenderTargetExtent extent;

  /// Physical texture width in device pixels.
  final int physicalWidth;

  /// Physical texture height in device pixels.
  final int physicalHeight;

  /// Borrowed OpenGL context.
  final OpenGLContextDescriptor context;

  /// Borrowed OpenGL texture name.
  final int texture;

  /// Backend-native OpenGL texture target, such as `GL_TEXTURE_2D`.
  final int target;
}
