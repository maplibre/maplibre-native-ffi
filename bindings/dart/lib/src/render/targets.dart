import 'dart:ffi';

import 'package:ffi/ffi.dart';

import '../internal/c/maplibre_native_c.dart';
import '../internal/c/maplibre_native_c.g.dart' as raw;
import '../internal/status/status.dart';
import 'native_pointer.dart';

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
      ensureAbiVersion();
      final status = raw.mln_render_target_extent_physical_size(
        nativeExtent,
        physicalWidth,
        physicalHeight,
      );
      checkNativeStatus(status, threadLastErrorMessage);
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

  /// WebGL context provider support bit.
  static const webgl = OpenGLContextProviderMask(1 << 2);

  /// Raw provider mask bits.
  final int bits;

  /// Returns true when all [provider] bits are present in this mask.
  bool contains(OpenGLContextProviderMask provider) =>
      (bits & provider.bits) == provider.bits;

  @override
  String toString() =>
      'OpenGLContextProviderMask[bits=0x${bits.toRadixString(16)}]';
}

/// How a session's OpenGL context relates to its driver thread and host
/// graphics state.
///
/// Known values use the named constants. Values added by a newer compatible
/// native library retain their raw integer through [fromRawValue].
final class OpenGLContextOwnership {
  const OpenGLContextOwnership._(this.rawValue, this.name);

  /// The session shares its thread with host graphics work.
  ///
  /// Every render makes the session context current and restores whatever was
  /// current before, and the session context joins the share group named by
  /// the context descriptor.
  static const shared = OpenGLContextOwnership._(0, 'shared');

  /// The session owns its thread's OpenGL context.
  ///
  /// The session makes its context current once and keeps it current between
  /// renders, and it joins no share group. Use this when a thread exists to
  /// drive one render session and runs no other graphics work.
  static const dedicated = OpenGLContextOwnership._(1, 'dedicated');

  /// Creates a context ownership from a raw native value.
  factory OpenGLContextOwnership.fromRawValue(int rawValue) =>
      switch (rawValue) {
        0 => shared,
        1 => dedicated,
        _ => OpenGLContextOwnership._(rawValue, 'unknown($rawValue)'),
      };

  /// Raw native value.
  final int rawValue;

  /// Human-readable name.
  final String name;

  @override
  bool operator ==(Object other) =>
      other is OpenGLContextOwnership && other.rawValue == rawValue;

  @override
  int get hashCode => rawValue.hashCode;
}

/// OpenGL client API a dedicated EGL session creates its context for.
///
/// Known values use the named constants. Values added by a newer compatible
/// native library retain their raw integer through [fromRawValue].
final class OpenGLClientApi {
  const OpenGLClientApi._(this.rawValue, this.name);

  /// No client API is named.
  static const unspecified = OpenGLClientApi._(0, 'unspecified');

  /// Desktop OpenGL, as `EGL_OPENGL_API` names it.
  static const gl = OpenGLClientApi._(1, 'gl');

  /// OpenGL ES, as `EGL_OPENGL_ES_API` names it.
  static const gles = OpenGLClientApi._(2, 'gles');

  /// Creates a client API from a raw native value.
  factory OpenGLClientApi.fromRawValue(int rawValue) => switch (rawValue) {
    0 => unspecified,
    1 => gl,
    2 => gles,
    _ => OpenGLClientApi._(rawValue, 'unknown($rawValue)'),
  };

  /// Raw native value.
  final int rawValue;

  /// Human-readable name.
  final String name;

  @override
  bool operator ==(Object other) =>
      other is OpenGLClientApi && other.rawValue == rawValue;

  @override
  int get hashCode => rawValue.hashCode;
}

/// OpenGL backend context fields shared by OpenGL render targets.
sealed class OpenGLContextDescriptor {
  const OpenGLContextDescriptor({
    this.ownership = OpenGLContextOwnership.shared,
  });

  /// Whether the session shares its driver thread and graphics objects with
  /// the host.
  ///
  /// A private EGL owned texture is dedicated to its core worker; a target
  /// that renders into host-visible graphics objects is shared.
  final OpenGLContextOwnership ownership;
}

/// WGL context fields shared by OpenGL render targets on Windows.
final class WglContextDescriptor extends OpenGLContextDescriptor {
  /// Creates a WGL context descriptor.
  const WglContextDescriptor({
    required this.deviceContext,
    required this.shareContext,
    this.getProcAddress = NativePointer.nullPointer,
    super.ownership,
  });

  /// Borrowed HDC used to create the session context.
  final NativePointer deviceContext;

  /// Borrowed HGLRC whose share group the session context joins.
  ///
  /// Required under shared ownership. A dedicated session joins no share
  /// group, so it must be null there.
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
    this.clientApi = OpenGLClientApi.unspecified,
    this.getProcAddress = NativePointer.nullPointer,
    super.ownership,
  });

  /// Borrowed EGLDisplay.
  final NativePointer display;

  /// Borrowed EGLConfig used to create the session context.
  ///
  /// OpenGL texture sessions require `EGL_SURFACE_TYPE` to include
  /// `EGL_PBUFFER_BIT`.
  final NativePointer config;

  /// Borrowed EGLContext whose share group the session context joins.
  ///
  /// Required under shared ownership, where the session also takes its client
  /// API from this context. A dedicated session joins no share group, so it
  /// must be null there and names [clientApi] instead.
  final NativePointer shareContext;

  /// Client API the session creates its context for.
  ///
  /// Required under dedicated ownership. A shared session queries
  /// [shareContext] for it, so this is ignored there.
  final OpenGLClientApi clientApi;

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
  final VulkanHandle surface;
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
  final VulkanHandle image;

  /// Borrowed VkImageView.
  final VulkanHandle imageView;

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

/// WebGPU device, instance, and queue used by a render target.
final class WebGPUContextDescriptor {
  /// Creates a WebGPU context descriptor.
  const WebGPUContextDescriptor({
    this.instance = NativePointer.nullPointer,
    required this.device,
    this.queue = NativePointer.nullPointer,
  });

  /// Borrowed `WGPUInstance`, which a texture session may leave null.
  final NativePointer instance;

  /// Borrowed `WGPUDevice`, which every WebGPU target requires.
  final NativePointer device;

  /// Borrowed `WGPUQueue` belonging to [device]. Null uses the device's
  /// default queue.
  final NativePointer queue;
}

/// WebGPU native-surface attachment options.
final class WebGPUSurfaceDescriptor {
  /// Creates a WebGPU surface descriptor.
  const WebGPUSurfaceDescriptor({
    required this.extent,
    required this.context,
    required this.surface,
    required this.format,
  });

  /// Logical surface extent.
  final RenderTargetExtent extent;

  /// WebGPU backend context.
  final WebGPUContextDescriptor context;

  /// Borrowed `WGPUSurface`, which stays alive for the session. The session
  /// configures it for this device and extent, and unconfigures it at the end.
  final NativePointer surface;

  /// `WGPUTextureFormat` to configure the surface with. A browser host takes
  /// it from `navigator.gpu.getPreferredCanvasFormat()`.
  final int format;
}

/// WebGPU session-owned texture-ring attachment options.
final class WebGPUOwnedTextureDescriptor {
  /// Creates a WebGPU session-owned texture descriptor.
  const WebGPUOwnedTextureDescriptor({
    required this.extent,
    required this.context,
  });

  /// Logical texture extent.
  final RenderTargetExtent extent;

  /// WebGPU backend context.
  final WebGPUContextDescriptor context;
}

/// WebGPU caller-owned texture attachment options.
final class WebGPUBorrowedTextureDescriptor {
  /// Creates a WebGPU caller-owned texture descriptor.
  const WebGPUBorrowedTextureDescriptor({
    required this.extent,
    required this.physicalWidth,
    required this.physicalHeight,
    required this.context,
    required this.texture,
    required this.textureView,
    required this.format,
  });

  /// Logical texture extent.
  final RenderTargetExtent extent;

  /// Physical texture width in device pixels.
  final int physicalWidth;

  /// Physical texture height in device pixels.
  final int physicalHeight;

  /// WebGPU backend context that created [texture] and [textureView].
  final WebGPUContextDescriptor context;

  /// Borrowed `WGPUTexture`. It must be 2D, single-sample, and
  /// render-attachment capable, and its physical size and format must match
  /// this descriptor.
  final NativePointer texture;

  /// Borrowed 2D color `WGPUTextureView` of [texture].
  final NativePointer textureView;

  /// Backend-native `WGPUTextureFormat` value.
  final int format;
}
