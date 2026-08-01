internal import CMaplibreNativeC
import Foundation

public struct RenderTargetExtent: Equatable, Sendable {
  public var width: UInt32
  public var height: UInt32
  public var scaleFactor: Double

  public init(width: UInt32, height: UInt32, scaleFactor: Double) {
    self.width = width
    self.height = height
    self.scaleFactor = scaleFactor
  }

  var nativeInput: NativeRenderTargetExtent {
    NativeRenderTargetExtent(
      width: width,
      height: height,
      scaleFactor: scaleFactor
    )
  }

  /// Returns the physical device-pixel size as `ceil(logical * scaleFactor)`
  /// per
  /// dimension.
  ///
  /// Session-owned texture targets and surface targets are sized this way.
  /// Borrowed texture targets state their physical size instead, because not
  /// every physical size is reachable from a logical extent.
  public func physicalSize() throws -> (width: UInt32, height: UInt32) {
    try mapNativeFailure {
      var native = nativeInput.native
      var physicalWidth: UInt32 = 0
      var physicalHeight: UInt32 = 0
      try checkStatus(
        mln_render_target_extent_physical_size(
          &native,
          &physicalWidth,
          &physicalHeight
        )
      )
      return (width: physicalWidth, height: physicalHeight)
    }
  }
}

public struct MetalContextDescriptor: Equatable, Sendable {
  public var device: NativePointer

  public init(device: NativePointer = .null) {
    self.device = device
  }

  var nativeInput: NativeMetalContextDescriptor {
    NativeMetalContextDescriptor(deviceAddress: device.addressBitPattern)
  }
}

public struct VulkanContextDescriptor: Equatable, Sendable {
  public var instance: NativePointer
  public var physicalDevice: NativePointer
  public var device: NativePointer
  public var graphicsQueue: NativePointer
  public var graphicsQueueFamilyIndex: UInt32
  public var getInstanceProcAddr: NativePointer
  public var getDeviceProcAddr: NativePointer

  public init(
    instance: NativePointer,
    physicalDevice: NativePointer,
    device: NativePointer,
    graphicsQueue: NativePointer,
    graphicsQueueFamilyIndex: UInt32,
    getInstanceProcAddr: NativePointer = .null,
    getDeviceProcAddr: NativePointer = .null
  ) {
    self.instance = instance
    self.physicalDevice = physicalDevice
    self.device = device
    self.graphicsQueue = graphicsQueue
    self.graphicsQueueFamilyIndex = graphicsQueueFamilyIndex
    self.getInstanceProcAddr = getInstanceProcAddr
    self.getDeviceProcAddr = getDeviceProcAddr
  }

  var nativeInput: NativeVulkanContextDescriptor {
    NativeVulkanContextDescriptor(
      instanceAddress: instance.addressBitPattern,
      physicalDeviceAddress: physicalDevice.addressBitPattern,
      deviceAddress: device.addressBitPattern,
      graphicsQueueAddress: graphicsQueue.addressBitPattern,
      graphicsQueueFamilyIndex: graphicsQueueFamilyIndex,
      getInstanceProcAddrAddress: getInstanceProcAddr.addressBitPattern,
      getDeviceProcAddrAddress: getDeviceProcAddr.addressBitPattern
    )
  }
}

public struct WglContextDescriptor: Equatable, Sendable {
  public var deviceContext: NativePointer
  public var shareContext: NativePointer
  public var getProcAddress: NativePointer

  public init(
    deviceContext: NativePointer,
    shareContext: NativePointer,
    getProcAddress: NativePointer = .null
  ) {
    self.deviceContext = deviceContext
    self.shareContext = shareContext
    self.getProcAddress = getProcAddress
  }

  var nativeInput: NativeWglContextDescriptor {
    NativeWglContextDescriptor(
      deviceContextAddress: deviceContext.addressBitPattern,
      shareContextAddress: shareContext.addressBitPattern,
      getProcAddressAddress: getProcAddress.addressBitPattern
    )
  }
}

public struct EglContextDescriptor: Equatable, Sendable {
  public var display: NativePointer
  public var config: NativePointer
  public var shareContext: NativePointer
  public var getProcAddress: NativePointer

  public init(
    display: NativePointer,
    config: NativePointer,
    shareContext: NativePointer,
    getProcAddress: NativePointer = .null
  ) {
    self.display = display
    self.config = config
    self.shareContext = shareContext
    self.getProcAddress = getProcAddress
  }

  var nativeInput: NativeEglContextDescriptor {
    NativeEglContextDescriptor(
      displayAddress: display.addressBitPattern,
      configAddress: config.addressBitPattern,
      shareContextAddress: shareContext.addressBitPattern,
      getProcAddressAddress: getProcAddress.addressBitPattern
    )
  }
}

public enum OpenGLContextDescriptor: Equatable, Sendable {
  case wgl(WglContextDescriptor)
  case egl(EglContextDescriptor)

  var nativeInput: NativeOpenGLContextDescriptor {
    switch self {
    case let .wgl(descriptor):
      NativeOpenGLContextDescriptor(platform: .wgl(descriptor.nativeInput))
    case let .egl(descriptor):
      NativeOpenGLContextDescriptor(platform: .egl(descriptor.nativeInput))
    }
  }
}

public struct MetalSurfaceDescriptor: Equatable, Sendable {
  public var extent: RenderTargetExtent
  public var context: MetalContextDescriptor
  public var layer: NativePointer

  public init(
    extent: RenderTargetExtent,
    context: MetalContextDescriptor = MetalContextDescriptor(),
    layer: NativePointer
  ) {
    self.extent = extent
    self.context = context
    self.layer = layer
  }

  var nativeInput: NativeMetalSurfaceDescriptorInput {
    NativeMetalSurfaceDescriptorInput(
      extent: extent.nativeInput,
      context: context.nativeInput,
      layerAddress: layer.addressBitPattern
    )
  }
}

public struct VulkanSurfaceDescriptor: Equatable, Sendable {
  public var extent: RenderTargetExtent
  public var context: VulkanContextDescriptor
  public var surface: NativePointer

  public init(
    extent: RenderTargetExtent,
    context: VulkanContextDescriptor,
    surface: NativePointer
  ) {
    self.extent = extent
    self.context = context
    self.surface = surface
  }

  var nativeInput: NativeVulkanSurfaceDescriptorInput {
    NativeVulkanSurfaceDescriptorInput(
      extent: extent.nativeInput,
      context: context.nativeInput,
      surfaceAddress: surface.addressBitPattern
    )
  }
}

public struct OpenGLSurfaceDescriptor: Equatable, Sendable {
  public var extent: RenderTargetExtent
  public var context: OpenGLContextDescriptor
  public var surface: NativePointer

  public init(
    extent: RenderTargetExtent,
    context: OpenGLContextDescriptor,
    surface: NativePointer
  ) {
    self.extent = extent
    self.context = context
    self.surface = surface
  }

  var nativeInput: NativeOpenGLSurfaceDescriptorInput {
    NativeOpenGLSurfaceDescriptorInput(
      extent: extent.nativeInput,
      context: context.nativeInput,
      surfaceAddress: surface.addressBitPattern
    )
  }
}

public struct TextureImageInfo: Equatable, Sendable {
  public let width: UInt32
  public let height: UInt32
  public let stride: UInt32
  public let byteLength: Int

  init(native: NativeTextureImageInfo) {
    width = native.width
    height = native.height
    stride = native.stride
    byteLength = native.byteLength
  }
}

public struct PremultipliedRGBA8Image: Equatable, Sendable {
  public let info: TextureImageInfo
  public let data: Data
}

public struct MetalOwnedTextureDescriptor: Equatable, Sendable {
  public var extent: RenderTargetExtent
  public var context: MetalContextDescriptor

  public init(extent: RenderTargetExtent, context: MetalContextDescriptor) {
    self.extent = extent
    self.context = context
  }

  var nativeInput: NativeMetalOwnedTextureDescriptorInput {
    NativeMetalOwnedTextureDescriptorInput(
      extent: extent.nativeInput,
      context: context.nativeInput
    )
  }
}

public struct MetalBorrowedTextureDescriptor: Equatable, Sendable {
  public var extent: RenderTargetExtent
  /// Physical texture size in device pixels. The texture is sized by its owner,
  /// so this is stated rather than derived from `extent`.
  public var physicalWidth: UInt32
  public var physicalHeight: UInt32
  public var texture: NativePointer

  public init(
    extent: RenderTargetExtent,
    physicalWidth: UInt32,
    physicalHeight: UInt32,
    texture: NativePointer
  ) {
    self.extent = extent
    self.physicalWidth = physicalWidth
    self.physicalHeight = physicalHeight
    self.texture = texture
  }

  var nativeInput: NativeMetalBorrowedTextureDescriptorInput {
    NativeMetalBorrowedTextureDescriptorInput(
      extent: extent.nativeInput,
      physicalWidth: physicalWidth,
      physicalHeight: physicalHeight,
      textureAddress: texture.addressBitPattern
    )
  }
}

public struct VulkanOwnedTextureDescriptor: Equatable, Sendable {
  public var extent: RenderTargetExtent
  public var context: VulkanContextDescriptor

  public init(extent: RenderTargetExtent, context: VulkanContextDescriptor) {
    self.extent = extent
    self.context = context
  }

  var nativeInput: NativeVulkanOwnedTextureDescriptorInput {
    NativeVulkanOwnedTextureDescriptorInput(
      extent: extent.nativeInput,
      context: context.nativeInput
    )
  }
}

public struct VulkanBorrowedTextureDescriptor: Equatable, Sendable {
  public var extent: RenderTargetExtent
  /// Physical image size in device pixels. The image is sized by its owner,
  /// so this is stated rather than derived from `extent`.
  public var physicalWidth: UInt32
  public var physicalHeight: UInt32
  public var context: VulkanContextDescriptor
  public var image: NativePointer
  public var imageView: NativePointer
  public var format: UInt32
  public var initialLayout: UInt32
  public var finalLayout: UInt32

  public init(
    extent: RenderTargetExtent,
    physicalWidth: UInt32,
    physicalHeight: UInt32,
    context: VulkanContextDescriptor,
    image: NativePointer,
    imageView: NativePointer,
    format: UInt32,
    initialLayout: UInt32,
    finalLayout: UInt32
  ) {
    self.extent = extent
    self.physicalWidth = physicalWidth
    self.physicalHeight = physicalHeight
    self.context = context
    self.image = image
    self.imageView = imageView
    self.format = format
    self.initialLayout = initialLayout
    self.finalLayout = finalLayout
  }

  var nativeInput: NativeVulkanBorrowedTextureDescriptorInput {
    NativeVulkanBorrowedTextureDescriptorInput(
      extent: extent.nativeInput,
      physicalWidth: physicalWidth,
      physicalHeight: physicalHeight,
      context: context.nativeInput,
      imageAddress: image.addressBitPattern,
      imageViewAddress: imageView.addressBitPattern,
      format: format,
      initialLayout: initialLayout,
      finalLayout: finalLayout
    )
  }
}

public struct OpenGLOwnedTextureDescriptor: Equatable, Sendable {
  public var extent: RenderTargetExtent
  public var context: OpenGLContextDescriptor

  public init(extent: RenderTargetExtent, context: OpenGLContextDescriptor) {
    self.extent = extent
    self.context = context
  }

  var nativeInput: NativeOpenGLOwnedTextureDescriptorInput {
    NativeOpenGLOwnedTextureDescriptorInput(
      extent: extent.nativeInput,
      context: context.nativeInput
    )
  }
}

public struct OpenGLBorrowedTextureDescriptor: Equatable, Sendable {
  public var extent: RenderTargetExtent
  /// Physical texture size in device pixels. The texture is sized by its owner,
  /// so this is stated rather than derived from `extent`.
  public var physicalWidth: UInt32
  public var physicalHeight: UInt32
  public var context: OpenGLContextDescriptor
  public var texture: UInt32
  public var target: UInt32

  public init(
    extent: RenderTargetExtent,
    physicalWidth: UInt32,
    physicalHeight: UInt32,
    context: OpenGLContextDescriptor,
    texture: UInt32,
    target: UInt32
  ) {
    self.extent = extent
    self.physicalWidth = physicalWidth
    self.physicalHeight = physicalHeight
    self.context = context
    self.texture = texture
    self.target = target
  }

  var nativeInput: NativeOpenGLBorrowedTextureDescriptorInput {
    NativeOpenGLBorrowedTextureDescriptorInput(
      extent: extent.nativeInput,
      physicalWidth: physicalWidth,
      physicalHeight: physicalHeight,
      context: context.nativeInput,
      texture: texture,
      target: target
    )
  }
}

/// A render session, affine to the thread that attached it.
///
/// The session holds no Swift-level retention of its map, because a session may
/// be attached on a thread that cannot hold a non-`Sendable` ``MapHandle``.
/// Native keeps the map alive instead: destroying a map fails while a session
/// is attached to it.
public final class RenderSessionHandle {
  private let handle: NativeHandleBox<NativeRenderSessionHandle>

  init(handle session: NativeRenderSessionHandle) throws {
    handle = try NativeHandleBox(
      typeName: "RenderSessionHandle",
      handle: session
    )
  }

  public var isClosed: Bool {
    handle.isClosed
  }

  func requireLiveHandle() throws -> NativeRenderSessionHandle {
    try handle.requireLive()
  }

  public func close() throws {
    try handle.closeOnce { session in
      try checkStatus(mln_render_session_destroy(session.raw))
    }
  }

  /// Resizes this attached render session.
  ///
  /// Surface and owned-texture sessions resize in place. Borrowed texture
  /// targets are sized by their owner and throw an unsupported-feature error:
  /// allocate a texture at the new size and hand it over with the backend's
  /// borrowed-texture target setter, such as
  /// ``setMetalBorrowedTextureTarget(_:)``, which keeps this session.
  ///
  /// The session keeps its renderer across a resize, so renderer-held state
  /// such as feature state carries over. A scale factor change is the
  /// exception: a renderer compiles its shaders for one pixel ratio, so that
  /// resize starts a new one with renderer-held state empty. Map state such as
  /// camera, style, and sources lives on the map and survives either way.
  public func resize(width: UInt32, height: UInt32,
                     scaleFactor: Double) throws
  {
    try mapNativeFailure {
      try checkStatus(mln_render_session_resize(
        handle.requireLive().raw,
        width,
        height,
        scaleFactor
      ))
    }
  }

  /// Presents this attached surface session through a new surface.
  ///
  /// A host surface can be destroyed and recreated while the map goes on
  /// living, which is what Android rotation, a Flutter `SurfaceProducer`
  /// lifecycle change, and a window resize that reallocates all look like from
  /// here. Replacing the surface in place keeps this session's renderer, and
  /// with it the tile pyramid, glyph and image atlases, symbol placement, and
  /// feature state.
  ///
  /// The descriptor names the same graphics context this session attached
  /// with, and its extent applies as a resize does. A layer on a different
  /// device throws an invalid-argument error and leaves this session rendering
  /// into the surface it has, so close it and attach again to take that one.
  /// The session sets the layer's pixel format itself, so there is nothing else
  /// here for a replacement to mismatch.
  public func setMetalSurfaceTarget(
    _ descriptor: MetalSurfaceDescriptor
  ) throws {
    try mapNativeFailure {
      try descriptor.nativeInput.withNativeDescriptor { nativeDescriptor in
        try checkStatus(mln_metal_surface_set_target(
          handle.requireLive().raw,
          nativeDescriptor
        ))
      }
    }
  }

  /// Presents this attached surface session through a new surface.
  ///
  /// See ``setMetalSurfaceTarget(_:)`` for what replacing a surface preserves.
  /// The outgoing `VkSurfaceKHR` must still be valid: this session holds a
  /// swapchain built from it, and Vulkan destroys every swapchain before its
  /// surface.
  public func setVulkanSurfaceTarget(
    _ descriptor: VulkanSurfaceDescriptor
  ) throws {
    try mapNativeFailure {
      try descriptor.nativeInput.withNativeDescriptor { nativeDescriptor in
        try checkStatus(mln_vulkan_surface_set_target(
          handle.requireLive().raw,
          nativeDescriptor
        ))
      }
    }
  }

  /// Presents this attached surface session through a new surface.
  ///
  /// See ``setMetalSurfaceTarget(_:)`` for what replacing a surface preserves.
  /// The new surface is made current on the next render, so a host may hand
  /// over a replacement for one it has already destroyed. A surface accepted
  /// here can still prove unusable, which the next ``renderUpdate()`` reports
  /// rather than this call.
  public func setOpenGLSurfaceTarget(
    _ descriptor: OpenGLSurfaceDescriptor
  ) throws {
    try mapNativeFailure {
      try descriptor.nativeInput.withNativeDescriptor { nativeDescriptor in
        try checkStatus(mln_opengl_surface_set_target(
          handle.requireLive().raw,
          nativeDescriptor
        ))
      }
    }
  }

  /// Renders this attached texture session into a new caller-owned texture.
  ///
  /// A caller-owned texture is sized by its owner, so a host that follows a
  /// resize reallocates rather than resizing and
  /// ``resize(width:height:scaleFactor:)`` throws an unsupported-feature error.
  /// Handing the replacement over here keeps this session's renderer instead,
  /// so the map does not go cold on every resize.
  ///
  /// The replacement belongs to the device this session attached with, which
  /// throws an invalid-argument error otherwise, and carries the pixel format
  /// it attached with, which throws an unsupported-feature error otherwise.
  /// Both leave this session rendering into the texture it has. The caller owns
  /// the replacement and keeps it valid until the next replacement, detach, or
  /// close. This session never retained the outgoing texture and never releases
  /// it, but reads from it during this call, so keep that texture valid until
  /// the call returns.
  public func setMetalBorrowedTextureTarget(
    _ descriptor: MetalBorrowedTextureDescriptor
  ) throws {
    try mapNativeFailure {
      try descriptor.nativeInput.withNativeDescriptor { nativeDescriptor in
        try checkStatus(mln_metal_borrowed_texture_set_target(
          handle.requireLive().raw,
          nativeDescriptor
        ))
      }
    }
  }

  /// Renders this attached texture session into a new caller-owned image.
  ///
  /// See ``setMetalBorrowedTextureTarget(_:)`` for what replacing a target
  /// preserves. The replacement carries the format and both layouts this
  /// session attached with, since its render pass was built around them.
  public func setVulkanBorrowedTextureTarget(
    _ descriptor: VulkanBorrowedTextureDescriptor
  ) throws {
    try mapNativeFailure {
      try descriptor.nativeInput.withNativeDescriptor { nativeDescriptor in
        try checkStatus(mln_vulkan_borrowed_texture_set_target(
          handle.requireLive().raw,
          nativeDescriptor
        ))
      }
    }
  }

  /// Renders this attached texture session into a new caller-owned texture.
  ///
  /// See ``setMetalBorrowedTextureTarget(_:)`` for what replacing a target
  /// preserves. The replacement belongs to the context this session attached
  /// with, or one in its share group, and the host context must be current on
  /// this thread.
  public func setOpenGLBorrowedTextureTarget(
    _ descriptor: OpenGLBorrowedTextureDescriptor
  ) throws {
    try mapNativeFailure {
      try descriptor.nativeInput.withNativeDescriptor { nativeDescriptor in
        try checkStatus(mln_opengl_borrowed_texture_set_target(
          handle.requireLive().raw,
          nativeDescriptor
        ))
      }
    }
  }

  /// Renders the latest available map render update.
  ///
  /// The map retains its latest update, so repeated calls re-render it and
  /// return true again; use this to redraw on demand after resize or surface
  /// expose, and gate frame loops on render-update-available events instead
  /// of the return value. Returns false when no frame was rendered,
  /// because the map has not published an update yet or the renderer skipped
  /// the frame; both are normal during startup, so keep pumping the runtime
  /// until an update is reported.
  @discardableResult
  public func renderUpdate() throws -> Bool {
    try mapNativeFailure {
      var rendered = false
      try checkStatus(mln_render_session_render_update(
        handle.requireLive().raw,
        &rendered
      ))
      return rendered
    }
  }

  public func detach() throws {
    try mapNativeFailure {
      try checkStatus(mln_render_session_detach(handle.requireLive().raw))
    }
  }

  public func reduceMemoryUse() throws {
    try mapNativeFailure {
      try checkStatus(mln_render_session_reduce_memory_use(handle
          .requireLive().raw))
    }
  }

  public func clearData() throws {
    try mapNativeFailure {
      try checkStatus(mln_render_session_clear_data(handle.requireLive().raw))
    }
  }

  public func dumpDebugLogs() throws {
    try mapNativeFailure {
      try checkStatus(mln_render_session_dump_debug_logs(handle
          .requireLive().raw))
    }
  }

  public func readPremultipliedRGBA8(into data: inout [UInt8]) throws
    -> TextureImageInfo
  {
    try mapNativeFailure {
      try data.withUnsafeMutableBufferPointer { buffer in
        let rawInfo = try NativeRender.textureReadPremultipliedRGBA8(
          session: handle.requireLive(),
          data: buffer.baseAddress,
          capacity: buffer.count
        )
        // An empty destination reaches native code as the null pointer and
        // zero capacity that mean a size probe, which succeeds without
        // copying. Report the buffer as too small unless the frame really
        // carries no bytes.
        if buffer.isEmpty, rawInfo.byte_length > 0 {
          throw MaplibreError.invalidArgument(
            "buffer length 0 is smaller than the required \(rawInfo.byte_length) bytes"
          )
        }
        return TextureImageInfo(native: NativeTextureImageInfo(rawInfo))
      }
    }
  }

  public func acquireMetalOwnedTextureFrame() throws
    -> MetalOwnedTextureFrameHandle
  {
    try mapNativeFailure {
      try MetalOwnedTextureFrameHandle(
        session: self,
        frame: NativeMetalOwnedTextureFrame(NativeRender
          .metalOwnedTextureAcquireFrame(handle.requireLive()))
      )
    }
  }

  public func acquireVulkanOwnedTextureFrame() throws
    -> VulkanOwnedTextureFrameHandle
  {
    try mapNativeFailure {
      try VulkanOwnedTextureFrameHandle(
        session: self,
        frame: NativeVulkanOwnedTextureFrame(NativeRender
          .vulkanOwnedTextureAcquireFrame(handle.requireLive()))
      )
    }
  }

  public func acquireOpenGLOwnedTextureFrame() throws
    -> OpenGLOwnedTextureFrameHandle
  {
    try mapNativeFailure {
      try OpenGLOwnedTextureFrameHandle(
        session: self,
        frame: NativeOpenGLOwnedTextureFrame(NativeRender
          .openGLOwnedTextureAcquireFrame(handle.requireLive()))
      )
    }
  }
}

public final class MetalOwnedTextureFrameView {
  private let texturePointer: FrameNativePointer
  private let devicePointer: FrameNativePointer

  fileprivate init(texture: FrameNativePointer, device: FrameNativePointer) {
    texturePointer = texture
    devicePointer = device
  }

  public var texture: FrameNativePointer {
    get throws {
      _ = try texturePointer.addressBitPattern
      return texturePointer
    }
  }

  public var device: FrameNativePointer {
    get throws {
      _ = try devicePointer.addressBitPattern
      return devicePointer
    }
  }
}

public final class VulkanOwnedTextureFrameView {
  private let imagePointer: FrameNativePointer
  private let imageViewPointer: FrameNativePointer

  fileprivate init(image: FrameNativePointer, imageView: FrameNativePointer) {
    imagePointer = image
    imageViewPointer = imageView
  }

  public var image: FrameNativePointer {
    get throws {
      _ = try imagePointer.addressBitPattern
      return imagePointer
    }
  }

  public var imageView: FrameNativePointer {
    get throws {
      _ = try imageViewPointer.addressBitPattern
      return imageViewPointer
    }
  }
}

public final class OpenGLOwnedTextureFrameView {
  private let textureName: FrameOpenGLTextureName
  private let textureTarget: UInt32
  private let scope: NativeFrameScope

  fileprivate init(
    texture: FrameOpenGLTextureName,
    target: UInt32,
    scope: NativeFrameScope
  ) {
    textureName = texture
    textureTarget = target
    self.scope = scope
  }

  public var texture: FrameOpenGLTextureName {
    get throws {
      _ = try textureName.value
      return textureName
    }
  }

  public var target: UInt32 {
    get throws {
      try scope.requireActive("OpenGL texture")
      return textureTarget
    }
  }
}

public final class MetalOwnedTextureFrameHandle {
  private let releaseFrame: (inout NativeMetalOwnedTextureFrame) throws -> Void
  private var frame: NativeMetalOwnedTextureFrame?

  init(session: RenderSessionHandle, frame: NativeMetalOwnedTextureFrame) {
    releaseFrame = { frame in
      try withUnsafePointer(to: &frame.raw) { rawFrame in
        try checkStatus(mln_metal_owned_texture_release_frame(
          session.requireLiveHandle().raw,
          rawFrame
        ))
      }
    }
    self.frame = frame
  }

  init(
    frame: NativeMetalOwnedTextureFrame,
    releaseFrame: @escaping (inout NativeMetalOwnedTextureFrame) throws -> Void
  ) {
    self.releaseFrame = releaseFrame
    self.frame = frame
  }

  deinit {
    if let frame {
      NativeHandleLeakReporter.report(
        NativeHandleLeak(
          typeName: "MetalOwnedTextureFrameHandle",
          handle: 0,
          detail: "texture 0x\(String(UInt(bitPattern: frame.raw.texture), radix: 16))"
        )
      )
    }
  }

  public var isClosed: Bool {
    frame == nil
  }

  public func withBackendPointers(_ body: (MetalOwnedTextureFrameView) throws
    -> Void) throws
  {
    guard let frame else {
      throw MaplibreError(
        kind: .invalidState,
        rawStatus: nil,
        diagnostic: "Metal texture frame is closed"
      )
    }
    let scope = NativeFrameScope(isFrameLive: { [weak self] in
      self?.frame != nil
    })
    let view = MetalOwnedTextureFrameView(
      texture: FrameNativePointer(
        bitPattern: UInt(bitPattern: frame.raw.texture),
        scope: scope,
        diagnosticName: "Metal texture"
      ),
      device: FrameNativePointer(
        bitPattern: UInt(bitPattern: frame.raw.device),
        scope: scope,
        diagnosticName: "Metal device"
      )
    )
    defer { scope.close() }
    try body(view)
  }

  public func close() throws {
    guard var frame else { return }
    try mapNativeFailure {
      try releaseFrame(&frame)
    }
    self.frame = nil
  }
}

public final class VulkanOwnedTextureFrameHandle {
  private let releaseFrame: (inout NativeVulkanOwnedTextureFrame) throws -> Void
  private var frame: NativeVulkanOwnedTextureFrame?

  init(session: RenderSessionHandle, frame: NativeVulkanOwnedTextureFrame) {
    releaseFrame = { frame in
      try withUnsafePointer(to: &frame.raw) { rawFrame in
        try checkStatus(mln_vulkan_owned_texture_release_frame(
          session.requireLiveHandle().raw,
          rawFrame
        ))
      }
    }
    self.frame = frame
  }

  init(
    frame: NativeVulkanOwnedTextureFrame,
    releaseFrame: @escaping (inout NativeVulkanOwnedTextureFrame) throws -> Void
  ) {
    self.releaseFrame = releaseFrame
    self.frame = frame
  }

  deinit {
    if let frame {
      NativeHandleLeakReporter.report(
        NativeHandleLeak(
          typeName: "VulkanOwnedTextureFrameHandle",
          handle: 0,
          detail: "image 0x\(String(UInt(bitPattern: frame.raw.image), radix: 16))"
        )
      )
    }
  }

  public var isClosed: Bool {
    frame == nil
  }

  public func withBackendPointers(_ body: (VulkanOwnedTextureFrameView) throws
    -> Void) throws
  {
    guard let frame else {
      throw MaplibreError(
        kind: .invalidState,
        rawStatus: nil,
        diagnostic: "Vulkan texture frame is closed"
      )
    }
    let scope = NativeFrameScope(isFrameLive: { [weak self] in
      self?.frame != nil
    })
    let view = VulkanOwnedTextureFrameView(
      image: FrameNativePointer(
        bitPattern: UInt(bitPattern: frame.raw.image),
        scope: scope,
        diagnosticName: "Vulkan image"
      ),
      imageView: FrameNativePointer(
        bitPattern: UInt(bitPattern: frame.raw.image_view),
        scope: scope,
        diagnosticName: "Vulkan image view"
      )
    )
    defer { scope.close() }
    try body(view)
  }

  public func close() throws {
    guard var frame else { return }
    try mapNativeFailure {
      try releaseFrame(&frame)
    }
    self.frame = nil
  }
}

public final class OpenGLOwnedTextureFrameHandle {
  private let releaseFrame: (inout NativeOpenGLOwnedTextureFrame) throws -> Void
  private var frame: NativeOpenGLOwnedTextureFrame?

  init(session: RenderSessionHandle, frame: NativeOpenGLOwnedTextureFrame) {
    releaseFrame = { frame in
      try withUnsafePointer(to: &frame.raw) { rawFrame in
        try checkStatus(mln_opengl_owned_texture_release_frame(
          session.requireLiveHandle().raw,
          rawFrame
        ))
      }
    }
    self.frame = frame
  }

  init(
    frame: NativeOpenGLOwnedTextureFrame,
    releaseFrame: @escaping (inout NativeOpenGLOwnedTextureFrame) throws -> Void
  ) {
    self.releaseFrame = releaseFrame
    self.frame = frame
  }

  deinit {
    if let frame {
      NativeHandleLeakReporter.report(
        NativeHandleLeak(
          typeName: "OpenGLOwnedTextureFrameHandle",
          handle: 0,
          detail: "texture 0x\(String(UInt(frame.raw.texture), radix: 16))"
        )
      )
    }
  }

  public var isClosed: Bool {
    frame == nil
  }

  public func withBackendPointers(_ body: (OpenGLOwnedTextureFrameView) throws
    -> Void) throws
  {
    guard let frame else {
      throw MaplibreError(
        kind: .invalidState,
        rawStatus: nil,
        diagnostic: "OpenGL texture frame is closed"
      )
    }
    let scope = NativeFrameScope(isFrameLive: { [weak self] in
      self?.frame != nil
    })
    let view = OpenGLOwnedTextureFrameView(
      texture: FrameOpenGLTextureName(frame.raw.texture, scope: scope),
      target: frame.raw.target,
      scope: scope
    )
    defer { scope.close() }
    try body(view)
  }

  public func close() throws {
    guard var frame else { return }
    try mapNativeFailure {
      try releaseFrame(&frame)
    }
    self.frame = nil
  }
}

/// A reference to a map for the sole purpose of attaching a render session.
///
/// Produced by ``MapHandle/attachRef()``. Every attach function lives here
/// rather than on ``MapHandle``, because attaching is the one map operation
/// that runs on the render session's thread instead of the map's.
///
/// This carries no Swift retention of the map, because ``MapHandle`` is not
/// `Sendable`. Native keeps the map alive instead: destroying a map fails while
/// a render session is attached to it. Closing the map marks the shared handle
/// state closed, so a reference that outlives its map throws rather than
/// binding
/// a session to whatever the allocator put at that address next.
///
/// Dropping the last reference to a ``MapHandle`` instead of closing it, while
/// a
/// session is still attached on another thread, leaks the native map: the
/// destroy fails and `deinit` can only report it. Close the session first, then
/// the map.
///
/// This is plainly `Sendable`: it carries a copied handle id rather than a
/// pointer, and the attach it performs reaches no thread-affine map state — the
/// C API claims the map's render-session slot under its registry lock and posts
/// the new size to the map's own owner thread.
public struct MapAttachRef: Sendable {
  private let handle: NativeHandleBox<NativeMapHandle>

  init(handle: NativeHandleBox<NativeMapHandle>) {
    self.handle = handle
  }

  /// Whether the map this reference names has been closed.
  ///
  /// A reference can outlive its ``MapHandle``, so a host that keeps one across
  /// a map's lifetime can check here instead of relying on the error from a
  /// failed attach.
  public var isMapClosed: Bool {
    handle.isClosed
  }

  /// The map's handle id, which the C API validates on every attach.
  ///
  /// A released id is rejected rather than binding the session to a later map,
  /// so no lock is held across the attach.
  func mapHandle() throws -> NativeMapHandle {
    try handle.requireLive()
  }
}

public extension MapAttachRef {
  func attachMetalSurface(_ descriptor: MetalSurfaceDescriptor) throws
    -> RenderSessionHandle
  {
    let session = try mapNativeFailure {
      let map = try mapHandle()
      return try descriptor.nativeInput
        .withNativeDescriptor { nativeDescriptor in
          try NativeRender.metalSurfaceAttach(
            map: map,
            descriptor: nativeDescriptor
          )
        }
    }
    return try RenderSessionHandle(handle: session)
  }

  func attachVulkanSurface(_ descriptor: VulkanSurfaceDescriptor) throws
    -> RenderSessionHandle
  {
    let session = try mapNativeFailure {
      let map = try mapHandle()
      return try descriptor.nativeInput
        .withNativeDescriptor { nativeDescriptor in
          try NativeRender.vulkanSurfaceAttach(
            map: map,
            descriptor: nativeDescriptor
          )
        }
    }
    return try RenderSessionHandle(handle: session)
  }

  func attachOpenGLSurface(_ descriptor: OpenGLSurfaceDescriptor) throws
    -> RenderSessionHandle
  {
    let session = try mapNativeFailure {
      let map = try mapHandle()
      return try descriptor.nativeInput
        .withNativeDescriptor { nativeDescriptor in
          try NativeRender.openGLSurfaceAttach(
            map: map,
            descriptor: nativeDescriptor
          )
        }
    }
    return try RenderSessionHandle(handle: session)
  }

  func attachMetalOwnedTexture(_ descriptor: MetalOwnedTextureDescriptor) throws
    -> RenderSessionHandle
  {
    let session = try mapNativeFailure {
      let map = try mapHandle()
      return try descriptor.nativeInput
        .withNativeDescriptor { nativeDescriptor in
          try NativeRender.metalOwnedTextureAttach(
            map: map,
            descriptor: nativeDescriptor
          )
        }
    }
    return try RenderSessionHandle(handle: session)
  }

  func attachMetalBorrowedTexture(
    _ descriptor: MetalBorrowedTextureDescriptor
  ) throws
    -> RenderSessionHandle
  {
    let session = try mapNativeFailure {
      let map = try mapHandle()
      return try descriptor.nativeInput
        .withNativeDescriptor { nativeDescriptor in
          try NativeRender.metalBorrowedTextureAttach(
            map: map,
            descriptor: nativeDescriptor
          )
        }
    }
    return try RenderSessionHandle(handle: session)
  }

  func attachVulkanOwnedTexture(
    _ descriptor: VulkanOwnedTextureDescriptor
  ) throws
    -> RenderSessionHandle
  {
    let session = try mapNativeFailure {
      let map = try mapHandle()
      return try descriptor.nativeInput
        .withNativeDescriptor { nativeDescriptor in
          try NativeRender.vulkanOwnedTextureAttach(
            map: map,
            descriptor: nativeDescriptor
          )
        }
    }
    return try RenderSessionHandle(handle: session)
  }

  func attachVulkanBorrowedTexture(
    _ descriptor: VulkanBorrowedTextureDescriptor
  ) throws
    -> RenderSessionHandle
  {
    let session = try mapNativeFailure {
      let map = try mapHandle()
      return try descriptor.nativeInput
        .withNativeDescriptor { nativeDescriptor in
          try NativeRender.vulkanBorrowedTextureAttach(
            map: map,
            descriptor: nativeDescriptor
          )
        }
    }
    return try RenderSessionHandle(handle: session)
  }

  func attachOpenGLOwnedTexture(
    _ descriptor: OpenGLOwnedTextureDescriptor
  ) throws
    -> RenderSessionHandle
  {
    let session = try mapNativeFailure {
      let map = try mapHandle()
      return try descriptor.nativeInput
        .withNativeDescriptor { nativeDescriptor in
          try NativeRender.openGLOwnedTextureAttach(
            map: map,
            descriptor: nativeDescriptor
          )
        }
    }
    return try RenderSessionHandle(handle: session)
  }

  func attachOpenGLBorrowedTexture(
    _ descriptor: OpenGLBorrowedTextureDescriptor
  ) throws
    -> RenderSessionHandle
  {
    let session = try mapNativeFailure {
      let map = try mapHandle()
      return try descriptor.nativeInput
        .withNativeDescriptor { nativeDescriptor in
          try NativeRender.openGLBorrowedTextureAttach(
            map: map,
            descriptor: nativeDescriptor
          )
        }
    }
    return try RenderSessionHandle(handle: session)
  }
}
