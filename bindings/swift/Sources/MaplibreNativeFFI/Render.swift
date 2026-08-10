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
  /// per dimension. Borrowed texture targets state their physical size instead.
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

/// The outcome of a ``RenderSessionHandle/renderUpdate()`` call.
public enum RenderResult: Sendable, Hashable {
  /// The call rendered a frame into the render target.
  case rendered
  /// The map has no render update yet.
  case noUpdate
  /// The map has not applied the session's current size yet.
  case sizePending
  /// The render target had no frame to draw into.
  case targetNotReady
  case unknown(UInt32)

  public static func fromNative(_ rawValue: UInt32) -> Self {
    switch rawValue {
    case 0: .rendered
    case 1: .noUpdate
    case 2: .sizePending
    case 3: .targetNotReady
    default: .unknown(rawValue)
    }
  }
}

/// A render session, affine to the thread that attached it.
///
/// The session holds no Swift-level retention of its map. Native keeps the map
/// alive instead: destroying a map fails while a session is attached to it.
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
  /// targets throw an unsupported-feature error; hand over a new texture with
  /// the backend's borrowed-texture target setter instead.
  ///
  /// The session keeps its renderer, and renderer-held state such as feature
  /// state, across a resize. A scale factor change starts a new renderer with
  /// that state empty.
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

  /// Presents this attached surface session through a new surface, keeping the
  /// session's renderer and its cached state.
  ///
  /// The descriptor's extent applies as a resize does. A `context.device` that
  /// is neither nil nor this session's device throws an invalid-argument error
  /// and leaves this session rendering into the surface it has.
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
  /// The new surface is made current on the next render, so a host may hand
  /// over a replacement for one it has already destroyed. The next
  /// ``renderUpdate()`` reports an unusable surface, not this call.
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

  /// Renders this attached texture session into a new caller-owned texture,
  /// keeping the session's renderer. A scale factor change starts a new one.
  ///
  /// The replacement must belong to the device and carry the pixel format this
  /// session attached with; a mismatch throws and leaves this session rendering
  /// into the texture it has. The caller keeps the replacement valid until the
  /// next replacement, detach, or close. The outgoing texture is neither read
  /// nor released here.
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
  /// The replacement must carry the format and both layouts this session
  /// attached with, since its render pass was built around them.
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
  /// The replacement must belong to the context this session attached with, or
  /// one in its share group, and the host context must be current on this
  /// thread.
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

  /// Renders the latest available map render update, reporting the outcome.
  ///
  /// Each result names the wake that a host waits for before it calls again:
  ///
  /// - ``RenderResult/rendered``: the target holds a new frame. The map retains
  ///   its latest update, so a host redraws on demand after a resize or a
  ///   surface expose, and paces a frame loop on the map
  ///   render-update-available event.
  /// - ``RenderResult/noUpdate``: the map has produced no render update so far,
  ///   because no style is loaded or the first pass is still running. Wait for
  ///   the map render-update-available event.
  /// - ``RenderResult/sizePending``: the session resized and the map, which
  ///   applies its size on its own thread, is still behind. The map publishes
  ///   an update for the new size on its own, so wait for the next map
  ///   render-update-available event.
  /// - ``RenderResult/targetNotReady``: the render target had no frame
  ///   available, such as a Metal surface whose next drawable is nil or a
  ///   texture target that holds no texture yet. No map update resolves this,
  ///   so wait for a host event that changes the target, or retry after a
  ///   delay.
  @discardableResult
  public func renderUpdate() throws -> RenderResult {
    try mapNativeFailure {
      var result = MLN_RENDER_RESULT_RENDERED
      try checkStatus(mln_render_session_render_update(
        handle.requireLive().raw,
        &result
      ))
      return RenderResult.fromNative(result.rawValue)
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
        // zero capacity of a size probe, which succeeds without copying.
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
/// Produced by ``MapHandle/attachRef()``. Attaching runs on the render
/// session's thread rather than the map's, so it lives here instead of on the
/// non-`Sendable` ``MapHandle``.
///
/// This carries no Swift retention of the map. A reference that outlives its
/// map throws rather than attaching. Close the session before the map:
/// dropping the last ``MapHandle`` reference while a session is still attached
/// leaks the native map, since the destroy fails and `deinit` can only report
/// it.
public struct MapAttachRef: Sendable {
  private let handle: NativeHandleBox<NativeMapHandle>

  init(handle: NativeHandleBox<NativeMapHandle>) {
    self.handle = handle
  }

  /// Whether the map this reference names has been closed.
  public var isMapClosed: Bool {
    handle.isClosed
  }

  /// The map's handle id, which the C API validates on every attach. A released
  /// id is rejected rather than binding the session to a later map.
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
