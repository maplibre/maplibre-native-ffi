import Foundation
import MaplibreNativeSupport

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
    NativeRenderTargetExtent(width: width, height: height, scaleFactor: scaleFactor)
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

  public init(
    instance: NativePointer,
    physicalDevice: NativePointer,
    device: NativePointer,
    graphicsQueue: NativePointer,
    graphicsQueueFamilyIndex: UInt32
  ) {
    self.instance = instance
    self.physicalDevice = physicalDevice
    self.device = device
    self.graphicsQueue = graphicsQueue
    self.graphicsQueueFamilyIndex = graphicsQueueFamilyIndex
  }

  var nativeInput: NativeVulkanContextDescriptor {
    NativeVulkanContextDescriptor(
      instanceAddress: instance.addressBitPattern,
      physicalDeviceAddress: physicalDevice.addressBitPattern,
      deviceAddress: device.addressBitPattern,
      graphicsQueueAddress: graphicsQueue.addressBitPattern,
      graphicsQueueFamilyIndex: graphicsQueueFamilyIndex
    )
  }
}

public struct MetalSurfaceDescriptor: Equatable, Sendable {
  public var extent: RenderTargetExtent
  public var context: MetalContextDescriptor
  public var layer: NativePointer

  public init(extent: RenderTargetExtent, context: MetalContextDescriptor = MetalContextDescriptor(), layer: NativePointer) {
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

  public init(extent: RenderTargetExtent, context: VulkanContextDescriptor, surface: NativePointer) {
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
    NativeMetalOwnedTextureDescriptorInput(extent: extent.nativeInput, context: context.nativeInput)
  }
}

public struct MetalBorrowedTextureDescriptor: Equatable, Sendable {
  public var extent: RenderTargetExtent
  public var texture: NativePointer

  public init(extent: RenderTargetExtent, texture: NativePointer) {
    self.extent = extent
    self.texture = texture
  }

  var nativeInput: NativeMetalBorrowedTextureDescriptorInput {
    NativeMetalBorrowedTextureDescriptorInput(extent: extent.nativeInput, textureAddress: texture.addressBitPattern)
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
    NativeVulkanOwnedTextureDescriptorInput(extent: extent.nativeInput, context: context.nativeInput)
  }
}

public struct VulkanBorrowedTextureDescriptor: Equatable, Sendable {
  public var extent: RenderTargetExtent
  public var context: VulkanContextDescriptor
  public var image: NativePointer
  public var imageView: NativePointer
  public var format: UInt32
  public var initialLayout: UInt32
  public var finalLayout: UInt32

  public init(
    extent: RenderTargetExtent,
    context: VulkanContextDescriptor,
    image: NativePointer,
    imageView: NativePointer,
    format: UInt32,
    initialLayout: UInt32,
    finalLayout: UInt32
  ) {
    self.extent = extent
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
      context: context.nativeInput,
      imageAddress: image.addressBitPattern,
      imageViewAddress: imageView.addressBitPattern,
      format: format,
      initialLayout: initialLayout,
      finalLayout: finalLayout
    )
  }
}

public final class RenderSessionHandle {
  private let map: MapHandle
  private let handle: NativeHandleBox

  init(map: MapHandle, pointer: OpaquePointer) throws {
    self.map = map
    handle = try NativeHandleBox(typeName: "RenderSessionHandle", pointer: pointer)
  }

  public var isClosed: Bool {
    handle.isClosed
  }

  func requireLivePointer() throws -> OpaquePointer {
    try handle.requireLive()
  }

  public func close() throws {
    try handle.closeOnce { pointer in
      try CAPI.renderSessionDestroy(pointer)
    }
  }

  public func resize(width: UInt32, height: UInt32, scaleFactor: Double) throws {
    try mapNativeFailure {
      try CAPI.renderSessionResize(try handle.requireLive(), width: width, height: height, scaleFactor: scaleFactor)
    }
  }

  public func renderUpdate() throws {
    try mapNativeFailure {
      try CAPI.renderSessionRenderUpdate(try handle.requireLive())
    }
  }

  public func detach() throws {
    try mapNativeFailure {
      try CAPI.renderSessionDetach(try handle.requireLive())
    }
  }

  public func reduceMemoryUse() throws {
    try mapNativeFailure {
      try CAPI.renderSessionReduceMemoryUse(try handle.requireLive())
    }
  }

  public func clearData() throws {
    try mapNativeFailure {
      try CAPI.renderSessionClearData(try handle.requireLive())
    }
  }

  public func dumpDebugLogs() throws {
    try mapNativeFailure {
      try CAPI.renderSessionDumpDebugLogs(try handle.requireLive())
    }
  }

  public func readPremultipliedRGBA8(into data: inout [UInt8]) throws -> TextureImageInfo {
    try mapNativeFailure {
      try data.withUnsafeMutableBufferPointer { buffer in
        let rawInfo = try CAPI.textureReadPremultipliedRGBA8(
          session: try handle.requireLive(),
          data: buffer.baseAddress,
          capacity: buffer.count
        )
        return TextureImageInfo(native: NativeTextureImageInfo(rawInfo))
      }
    }
  }

  public func acquireMetalOwnedTextureFrame() throws -> MetalOwnedTextureFrameHandle {
    try mapNativeFailure {
      MetalOwnedTextureFrameHandle(
        session: self,
        frame: NativeMetalOwnedTextureFrame(try CAPI.metalOwnedTextureAcquireFrame(try handle.requireLive()))
      )
    }
  }

  public func acquireVulkanOwnedTextureFrame() throws -> VulkanOwnedTextureFrameHandle {
    try mapNativeFailure {
      VulkanOwnedTextureFrameHandle(
        session: self,
        frame: NativeVulkanOwnedTextureFrame(try CAPI.vulkanOwnedTextureAcquireFrame(try handle.requireLive()))
      )
    }
  }
}

public final class MetalOwnedTextureFrameView {
  private let texturePointer: NativePointer
  private let devicePointer: NativePointer
  private let isFrameLive: () -> Bool
  private var active = true

  fileprivate init(texture: NativePointer, device: NativePointer, isFrameLive: @escaping () -> Bool) {
    texturePointer = texture
    devicePointer = device
    self.isFrameLive = isFrameLive
  }

  fileprivate func invalidate() {
    active = false
  }

  public var texture: NativePointer {
    get throws {
      guard active else {
        throw MaplibreError(kind: .invalidState, rawStatus: nil, diagnostic: "Metal texture frame scope has ended")
      }
      guard isFrameLive() else {
        throw MaplibreError(kind: .invalidState, rawStatus: nil, diagnostic: "Metal texture frame is closed")
      }
      return texturePointer
    }
  }

  public var device: NativePointer {
    get throws {
      guard active else {
        throw MaplibreError(kind: .invalidState, rawStatus: nil, diagnostic: "Metal texture frame scope has ended")
      }
      guard isFrameLive() else {
        throw MaplibreError(kind: .invalidState, rawStatus: nil, diagnostic: "Metal texture frame is closed")
      }
      return devicePointer
    }
  }
}

public final class VulkanOwnedTextureFrameView {
  private let imagePointer: NativePointer
  private let imageViewPointer: NativePointer
  private let isFrameLive: () -> Bool
  private var active = true

  fileprivate init(image: NativePointer, imageView: NativePointer, isFrameLive: @escaping () -> Bool) {
    imagePointer = image
    imageViewPointer = imageView
    self.isFrameLive = isFrameLive
  }

  fileprivate func invalidate() {
    active = false
  }

  public var image: NativePointer {
    get throws {
      guard active else {
        throw MaplibreError(kind: .invalidState, rawStatus: nil, diagnostic: "Vulkan texture frame scope has ended")
      }
      guard isFrameLive() else {
        throw MaplibreError(kind: .invalidState, rawStatus: nil, diagnostic: "Vulkan texture frame is closed")
      }
      return imagePointer
    }
  }

  public var imageView: NativePointer {
    get throws {
      guard active else {
        throw MaplibreError(kind: .invalidState, rawStatus: nil, diagnostic: "Vulkan texture frame scope has ended")
      }
      guard isFrameLive() else {
        throw MaplibreError(kind: .invalidState, rawStatus: nil, diagnostic: "Vulkan texture frame is closed")
      }
      return imageViewPointer
    }
  }
}

public final class MetalOwnedTextureFrameHandle {
  private let releaseFrame: (inout NativeMetalOwnedTextureFrame) throws -> Void
  private var frame: NativeMetalOwnedTextureFrame?

  init(session: RenderSessionHandle, frame: NativeMetalOwnedTextureFrame) {
    self.releaseFrame = { frame in
      try withUnsafePointer(to: &frame.raw) { rawFrame in
        try CAPI.metalOwnedTextureReleaseFrame(try session.requireLivePointer(), frame: rawFrame)
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
        NativeHandleLeak(typeName: "MetalOwnedTextureFrameHandle", address: UInt(bitPattern: frame.raw.texture))
      )
    }
  }

  public var isClosed: Bool {
    frame == nil
  }

  public func withBackendPointers(_ body: (MetalOwnedTextureFrameView) throws -> Void) throws {
    guard let frame else {
      throw MaplibreError(kind: .invalidState, rawStatus: nil, diagnostic: "Metal texture frame is closed")
    }
    let view = MetalOwnedTextureFrameView(
      texture: NativePointer(bitPattern: UInt(bitPattern: frame.raw.texture)),
      device: NativePointer(bitPattern: UInt(bitPattern: frame.raw.device)),
      isFrameLive: { [weak self] in self?.frame != nil }
    )
    defer { view.invalidate() }
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
    self.releaseFrame = { frame in
      try withUnsafePointer(to: &frame.raw) { rawFrame in
        try CAPI.vulkanOwnedTextureReleaseFrame(try session.requireLivePointer(), frame: rawFrame)
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
        NativeHandleLeak(typeName: "VulkanOwnedTextureFrameHandle", address: UInt(bitPattern: frame.raw.image))
      )
    }
  }

  public var isClosed: Bool {
    frame == nil
  }

  public func withBackendPointers(_ body: (VulkanOwnedTextureFrameView) throws -> Void) throws {
    guard let frame else {
      throw MaplibreError(kind: .invalidState, rawStatus: nil, diagnostic: "Vulkan texture frame is closed")
    }
    let view = VulkanOwnedTextureFrameView(
      image: NativePointer(bitPattern: UInt(bitPattern: frame.raw.image)),
      imageView: NativePointer(bitPattern: UInt(bitPattern: frame.raw.image_view)),
      isFrameLive: { [weak self] in self?.frame != nil }
    )
    defer { view.invalidate() }
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

extension MapHandle {
  public func attachMetalSurface(_ descriptor: MetalSurfaceDescriptor) throws -> RenderSessionHandle {
    let pointer = try mapNativeFailure {
      try descriptor.nativeInput.withNativeDescriptor { nativeDescriptor in
        try CAPI.metalSurfaceAttach(map: try requireLivePointer(), descriptor: nativeDescriptor)
      }
    }
    return try RenderSessionHandle(map: self, pointer: pointer)
  }

  public func attachVulkanSurface(_ descriptor: VulkanSurfaceDescriptor) throws -> RenderSessionHandle {
    let pointer = try mapNativeFailure {
      try descriptor.nativeInput.withNativeDescriptor { nativeDescriptor in
        try CAPI.vulkanSurfaceAttach(map: try requireLivePointer(), descriptor: nativeDescriptor)
      }
    }
    return try RenderSessionHandle(map: self, pointer: pointer)
  }

  public func attachMetalOwnedTexture(_ descriptor: MetalOwnedTextureDescriptor) throws -> RenderSessionHandle {
    let pointer = try mapNativeFailure {
      try descriptor.nativeInput.withNativeDescriptor { nativeDescriptor in
        try CAPI.metalOwnedTextureAttach(map: try requireLivePointer(), descriptor: nativeDescriptor)
      }
    }
    return try RenderSessionHandle(map: self, pointer: pointer)
  }

  public func attachMetalBorrowedTexture(_ descriptor: MetalBorrowedTextureDescriptor) throws -> RenderSessionHandle {
    let pointer = try mapNativeFailure {
      try descriptor.nativeInput.withNativeDescriptor { nativeDescriptor in
        try CAPI.metalBorrowedTextureAttach(map: try requireLivePointer(), descriptor: nativeDescriptor)
      }
    }
    return try RenderSessionHandle(map: self, pointer: pointer)
  }

  public func attachVulkanOwnedTexture(_ descriptor: VulkanOwnedTextureDescriptor) throws -> RenderSessionHandle {
    let pointer = try mapNativeFailure {
      try descriptor.nativeInput.withNativeDescriptor { nativeDescriptor in
        try CAPI.vulkanOwnedTextureAttach(map: try requireLivePointer(), descriptor: nativeDescriptor)
      }
    }
    return try RenderSessionHandle(map: self, pointer: pointer)
  }

  public func attachVulkanBorrowedTexture(_ descriptor: VulkanBorrowedTextureDescriptor) throws -> RenderSessionHandle {
    let pointer = try mapNativeFailure {
      try descriptor.nativeInput.withNativeDescriptor { nativeDescriptor in
        try CAPI.vulkanBorrowedTextureAttach(map: try requireLivePointer(), descriptor: nativeDescriptor)
      }
    }
    return try RenderSessionHandle(map: self, pointer: pointer)
  }
}
