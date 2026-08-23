internal import CMaplibreNativeC
import Foundation

public struct RenderTargetExtent: Hashable, Sendable {
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

/// How a session's OpenGL context relates to its driver and host graphics
/// state.
///
/// A shared session leaves the thread as it found it: every render makes the
/// session context current and restores whatever was current before. The
/// session context joins the host share group named by the descriptor, so a
/// host may hand the session a texture and sample it from its own context.
///
/// A dedicated session owns its driver thread's context. It keeps the context
/// current between renders and joins no share group. The driver may be a native
/// core worker or a dedicated host thread, such as an Android host that renders
/// into a SurfaceView.
public enum OpenGLContextOwnership: Sendable, Hashable {
  /// The session shares its thread with host graphics work.
  case shared
  /// The session owns its thread's OpenGL context.
  case dedicated
  case unknown(UInt32)

  public static func fromNative(_ rawValue: UInt32) -> Self {
    switch rawValue {
    case 0: .shared
    case 1: .dedicated
    default: .unknown(rawValue)
    }
  }

  public var rawValue: UInt32 {
    switch self {
    case .shared: 0
    case .dedicated: 1
    case let .unknown(raw): raw
    }
  }
}

/// OpenGL client API a dedicated EGL session creates its context for.
public enum OpenGLClientAPI: Sendable, Hashable {
  /// No client API is named.
  case unspecified
  /// Desktop OpenGL, as `EGL_OPENGL_API` names it.
  case gl
  /// OpenGL ES, as `EGL_OPENGL_ES_API` names it.
  case gles
  case unknown(UInt32)

  public static func fromNative(_ rawValue: UInt32) -> Self {
    switch rawValue {
    case 0: .unspecified
    case 1: .gl
    case 2: .gles
    default: .unknown(rawValue)
    }
  }

  public var rawValue: UInt32 {
    switch self {
    case .unspecified: 0
    case .gl: 1
    case .gles: 2
    case let .unknown(raw): raw
    }
  }
}

public struct WglContextDescriptor: Equatable, Sendable {
  public var deviceContext: NativePointer
  /// The context whose share group the session context joins. Required under
  /// shared ownership. A dedicated session joins no share group, so it must be
  /// ``NativePointer/null`` there.
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
  /// The context whose share group the session context joins. Required under
  /// shared ownership, where the session also takes its client API from this
  /// context. A dedicated session joins no share group, so it must be
  /// ``NativePointer/null`` there and names ``clientAPI`` instead.
  public var shareContext: NativePointer
  /// The client API the session creates its context for. Required under
  /// dedicated ownership. A shared session queries ``shareContext`` for it, so
  /// this is ignored there.
  public var clientAPI: OpenGLClientAPI
  public var getProcAddress: NativePointer

  public init(
    display: NativePointer,
    config: NativePointer,
    shareContext: NativePointer,
    clientAPI: OpenGLClientAPI = .unspecified,
    getProcAddress: NativePointer = .null
  ) {
    self.display = display
    self.config = config
    self.shareContext = shareContext
    self.clientAPI = clientAPI
    self.getProcAddress = getProcAddress
  }

  var nativeInput: NativeEglContextDescriptor {
    NativeEglContextDescriptor(
      displayAddress: display.addressBitPattern,
      configAddress: config.addressBitPattern,
      shareContextAddress: shareContext.addressBitPattern,
      clientAPIRawValue: clientAPI.rawValue,
      getProcAddressAddress: getProcAddress.addressBitPattern
    )
  }
}

public enum WebGLContextDescriptor: Equatable, Sendable {
  case existing(context: Int32)
  case transferredCanvas(selector: String)

  var nativeInput: NativeWebGLContextDescriptor {
    switch self {
    case let .existing(context):
      NativeWebGLContextDescriptor(
        kind: .existing,
        context: context,
        canvasSelector: ""
      )
    case let .transferredCanvas(selector):
      NativeWebGLContextDescriptor(
        kind: .transferredCanvas,
        context: 0,
        canvasSelector: selector
      )
    }
  }
}

public struct OpenGLContextDescriptor: Equatable, Sendable {
  public enum Platform: Equatable, Sendable {
    case wgl(WglContextDescriptor)
    case egl(EglContextDescriptor)
    case webGL(WebGLContextDescriptor)
  }

  public var platform: Platform
  public var ownership: OpenGLContextOwnership

  public init(
    platform: Platform,
    ownership: OpenGLContextOwnership = .shared
  ) {
    self.platform = platform
    self.ownership = ownership
  }

  public static func wgl(
    _ context: WglContextDescriptor,
    ownership: OpenGLContextOwnership = .shared
  ) -> Self {
    Self(platform: .wgl(context), ownership: ownership)
  }

  public static func egl(
    _ context: EglContextDescriptor,
    ownership: OpenGLContextOwnership = .shared
  ) -> Self {
    Self(platform: .egl(context), ownership: ownership)
  }

  public static func webGL(
    _ context: WebGLContextDescriptor,
    ownership: OpenGLContextOwnership = .shared
  ) -> Self {
    Self(platform: .webGL(context), ownership: ownership)
  }

  var nativeInput: NativeOpenGLContextDescriptor {
    switch platform {
    case let .wgl(descriptor):
      NativeOpenGLContextDescriptor(
        platform: .wgl(descriptor.nativeInput),
        ownershipRawValue: ownership.rawValue
      )
    case let .egl(descriptor):
      NativeOpenGLContextDescriptor(
        platform: .egl(descriptor.nativeInput),
        ownershipRawValue: ownership.rawValue
      )
    case let .webGL(descriptor):
      NativeOpenGLContextDescriptor(
        platform: .webGL(descriptor.nativeInput),
        ownershipRawValue: ownership.rawValue
      )
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

public struct WebGPUContextDescriptor: Equatable, Sendable {
  public var instance: NativePointer
  public var device: NativePointer
  public var queue: NativePointer

  public init(
    instance: NativePointer = .null,
    device: NativePointer,
    queue: NativePointer = .null
  ) {
    self.instance = instance
    self.device = device
    self.queue = queue
  }

  var nativeInput: NativeWebGPUContextDescriptor {
    NativeWebGPUContextDescriptor(
      instanceAddress: instance.addressBitPattern,
      deviceAddress: device.addressBitPattern,
      queueAddress: queue.addressBitPattern
    )
  }
}

public struct WebGPUSurfaceDescriptor: Equatable, Sendable {
  public var extent: RenderTargetExtent
  public var context: WebGPUContextDescriptor
  public var surface: NativePointer
  public var format: UInt32

  public init(
    extent: RenderTargetExtent,
    context: WebGPUContextDescriptor,
    surface: NativePointer,
    format: UInt32
  ) {
    self.extent = extent
    self.context = context
    self.surface = surface
    self.format = format
  }

  var nativeInput: NativeWebGPUSurfaceDescriptorInput {
    .init(
      extent: extent.nativeInput,
      context: context.nativeInput,
      surfaceAddress: surface.addressBitPattern,
      format: format
    )
  }
}

public struct WebGPUOwnedTextureDescriptor: Equatable, Sendable {
  public var extent: RenderTargetExtent
  public var context: WebGPUContextDescriptor

  public init(extent: RenderTargetExtent, context: WebGPUContextDescriptor) {
    self.extent = extent
    self.context = context
  }

  var nativeInput: NativeWebGPUOwnedTextureDescriptorInput {
    .init(extent: extent.nativeInput, context: context.nativeInput)
  }
}

public struct WebGPUBorrowedTextureDescriptor: Equatable, Sendable {
  public var extent: RenderTargetExtent
  public var physicalWidth: UInt32
  public var physicalHeight: UInt32
  public var context: WebGPUContextDescriptor
  public var texture: NativePointer
  public var textureView: NativePointer
  public var format: UInt32

  public init(
    extent: RenderTargetExtent,
    physicalWidth: UInt32,
    physicalHeight: UInt32,
    context: WebGPUContextDescriptor,
    texture: NativePointer,
    textureView: NativePointer,
    format: UInt32
  ) {
    self.extent = extent
    self.physicalWidth = physicalWidth
    self.physicalHeight = physicalHeight
    self.context = context
    self.texture = texture
    self.textureView = textureView
    self.format = format
  }

  var nativeInput: NativeWebGPUBorrowedTextureDescriptorInput {
    .init(
      extent: extent.nativeInput,
      physicalWidth: physicalWidth,
      physicalHeight: physicalHeight,
      context: context.nativeInput,
      textureAddress: texture.addressBitPattern,
      textureViewAddress: textureView.addressBitPattern,
      format: format
    )
  }
}

public enum RenderDriver: UInt32, Sendable, Hashable {
  case coreWorker = 1
  case callerGraphicsThread = 2
}

public struct RenderSessionAttachOptions: Sendable, Hashable {
  public var driver: RenderDriver
  public var requestedTextureRingDepth: UInt32

  public init(
    driver: RenderDriver,
    requestedTextureRingDepth: UInt32 = 0
  ) {
    self.driver = driver
    self.requestedTextureRingDepth = requestedTextureRingDepth
  }

  func withNative<Result>(
    frameWake: mln_wake,
    driverWorkWake: mln_wake,
    _ body: (UnsafePointer<mln_render_session_attach_options>) throws -> Result
  ) rethrows -> Result {
    var value = mln_render_session_attach_options_default()
    value.driver = driver.rawValue
    value.requested_texture_ring_depth = requestedTextureRingDepth
    value.frame_wake = frameWake
    value.driver_work_wake = driverWorkWake
    return try withUnsafePointer(to: &value, body)
  }
}

/// A render session that native code is still attaching and the task that
/// reports the attachment result.
///
/// A caller-graphics-thread host services driver work through ``session``
/// while ``completion`` is pending.
public struct RenderSessionAttachment: Sendable {
  public let session: RenderSessionHandle
  public let completion: Task<Void, Error>

  fileprivate init(
    session: RenderSessionHandle,
    completion: Task<Void, Error>
  ) {
    self.session = session
    self.completion = completion
  }
}

public struct RenderSessionCapabilities: Sendable, Hashable {
  public struct Features: OptionSet, Sendable, Hashable {
    public let rawValue: UInt32
    public init(rawValue: UInt32) {
      self.rawValue = rawValue
    }

    public static let frameAcquisition = Self(rawValue: 1 << 0)
    public static let readback = Self(rawValue: 1 << 1)
    public static let consumerSync = Self(rawValue: 1 << 2)
    public static let presentation = Self(rawValue: 1 << 3)
  }

  public let driver: RenderDriver
  public let textureRingDepth: UInt32
  public let features: Features
}

public enum RenderResult: Sendable, Hashable {
  case rendered
  case noUpdate
  case sizePending
  case targetNotReady
  case superseded
  case deadlineMissed
  case unknown(UInt32)

  public static func fromNative(_ value: UInt32) -> Self {
    switch value {
    case 0: .rendered
    case 1: .noUpdate
    case 2: .sizePending
    case 3: .targetNotReady
    case 4: .superseded
    case 5: .deadlineMissed
    default: .unknown(value)
    }
  }
}

public enum RenderSessionState: UInt32, Sendable, Hashable {
  case attaching = 1
  case attached = 2
  case detaching = 3
  case detached = 4
  case targetLost = 5
  case abandoned = 6
}

public struct FrameDemand: Sendable, Hashable {
  public struct Options: OptionSet, Sendable, Hashable {
    public let rawValue: UInt32
    public init(rawValue: UInt32) {
      self.rawValue = rawValue
    }

    public static let ifNeeded = Self(rawValue: 1 << 0)
    public static let present = Self(rawValue: 1 << 1)
  }

  public var options: Options
  public var token: UInt64
  public var coalescingBoundary: UInt64
  public var timeoutNanoseconds: UInt64

  public init(
    options: Options = .ifNeeded,
    token: UInt64 = 0,
    coalescingBoundary: UInt64 = 0,
    timeoutNanoseconds: UInt64 = 0
  ) {
    self.options = options
    self.token = token
    self.coalescingBoundary = coalescingBoundary
    self.timeoutNanoseconds = timeoutNanoseconds
  }

  var native: mln_frame_demand {
    var value = mln_frame_demand_default()
    value.flags = options.rawValue
    value.token = token
    value.coalescing_boundary = coalescingBoundary
    value.timeout_ns = timeoutNanoseconds
    return value
  }
}

public struct RenderFrameResult: Sendable, Hashable {
  public let result: RenderResult
  public let token: UInt64
  public let mapUpdateGeneration: UInt64
  public let extentGeneration: UInt64
  public let frameGeneration: UInt64
  /// Whether the map asked for another frame while it rendered this one, as
  /// during an ongoing camera transition. Set only when ``result`` is
  /// ``RenderResult/rendered``, and false for every other outcome. This is
  /// the same signal the map render-frame-finished event carries in
  /// ``RenderFrameEvent/needsRepaint``, delivered with the frame result so a
  /// host can re-arm its frame loop without the runtime event round trip.
  public let needsRepaint: Bool

  fileprivate init(_ value: mln_render_frame_result) {
    result = .fromNative(value.disposition)
    token = value.token
    mapUpdateGeneration = value.map_update_generation
    extentGeneration = value.extent_generation
    frameGeneration = value.frame_generation
    needsRepaint = value.needs_repaint
  }
}

public struct RenderSessionSnapshot: Sendable, Hashable {
  public let state: RenderSessionState?
  public let driver: RenderDriver?
  public let latestResult: RenderResult
  public let extent: RenderTargetExtent
  public let generation: UInt64
  public let mapUpdateGeneration: UInt64
  public let renderedUpdateGeneration: UInt64
  public let extentGeneration: UInt64
  public let frameGeneration: UInt64
  public let latestDemandToken: UInt64
  public let pendingDemandCount: UInt32
  public let acquiredFrameCount: UInt32
  public let targetReady: Bool
  public let pendingChanges: Bool
}

public enum GPUSynchronization: Sendable, Hashable {
  case cpuComplete
  case metalSharedEvent(NativePointer, value: UInt64)
  case vulkanTimelineSemaphore(NativePointer, value: UInt64)
  case openGLFence(NativePointer)
  case webGPUToken(NativePointer, value: UInt64)

  var native: mln_gpu_sync {
    var value = mln_gpu_sync_default()
    switch self {
    case .cpuComplete:
      break
    case let .metalSharedEvent(object, signal):
      value.kind = MLN_GPU_SYNC_METAL_SHARED_EVENT.rawValue
      value.object = object.unsafeMutableRawPointer
      value.value = signal
    case let .vulkanTimelineSemaphore(object, signal):
      value.kind = MLN_GPU_SYNC_VULKAN_TIMELINE_SEMAPHORE.rawValue
      value.object = object.unsafeMutableRawPointer
      value.value = signal
    case let .openGLFence(object):
      value.kind = MLN_GPU_SYNC_OPENGL_FENCE.rawValue
      value.object = object.unsafeMutableRawPointer
    case let .webGPUToken(object, signal):
      value.kind = MLN_GPU_SYNC_WEBGPU_TOKEN.rawValue
      value.object = object.unsafeMutableRawPointer
      value.value = signal
    }
    return value
  }
}

public struct RenderAbandonResult: Sendable, Hashable {
  public enum Disposition: UInt32, Sendable, Hashable {
    case clean = 0
    case quarantined = 1
  }

  public let disposition: Disposition
  public let quarantinedResourceCount: UInt32
}

public struct MetalOwnedTextureFrame: Sendable, Hashable {
  public let generation: UInt64
  public let width: UInt32
  public let height: UInt32
  public let scaleFactor: Double
  public let frameID: UInt64
  public let texture: NativePointer
  public let device: NativePointer
  public let pixelFormat: UInt64
}

public struct VulkanOwnedTextureFrame: Sendable, Hashable {
  public let generation: UInt64
  public let width: UInt32
  public let height: UInt32
  public let scaleFactor: Double
  public let frameID: UInt64
  public let image: NativePointer
  public let imageView: NativePointer
  public let device: NativePointer
  public let format: UInt32
  public let layout: UInt32
}

public struct OpenGLOwnedTextureFrame: Sendable, Hashable {
  public let generation: UInt64
  public let width: UInt32
  public let height: UInt32
  public let scaleFactor: Double
  public let frameID: UInt64
  public let texture: UInt32
  public let target: UInt32
  public let internalFormat: UInt32
  public let format: UInt32
  public let type: UInt32
}

public struct WebGPUOwnedTextureFrame: Sendable, Hashable {
  public let generation: UInt64
  public let width: UInt32
  public let height: UInt32
  public let scaleFactor: Double
  public let frameID: UInt64
  public let texture: NativePointer
  public let textureView: NativePointer
  public let device: NativePointer
  public let format: UInt32
}

public final class AcquiredFrameHandle: @unchecked Sendable {
  private let lock = NSLock()
  private var handle: mln_acquired_frame

  fileprivate init(handle: mln_acquired_frame) {
    self.handle = handle
  }

  deinit {
    let leaked = lock.withLock { handle }
    if leaked != 0 {
      NativeHandleLeakReporter.report(NativeHandleLeak(
        typeName: "AcquiredFrameHandle",
        handle: leaked,
        detail: "release(consumerCompletion:) was not called"
      ))
    }
  }

  public var isReleased: Bool {
    lock.withLock { handle == 0 }
  }

  public func result() throws -> RenderFrameResult {
    try mapNativeFailure {
      try lock.withLock {
        var value = mln_render_frame_result()
        value.size = UInt32(MemoryLayout<mln_render_frame_result>.size)
        try checkStatus(mln_acquired_frame_get_result(
          requireLiveLocked(),
          &value
        ))
        return RenderFrameResult(value)
      }
    }
  }

  public func producerSynchronization() throws -> GPUSynchronization {
    try mapNativeFailure {
      try lock.withLock {
        var value = mln_gpu_sync_default()
        try checkStatus(mln_acquired_frame_get_producer_sync(
          requireLiveLocked(), &value
        ))
        let object = NativePointer(bitPattern: UInt(bitPattern: value.object))
        switch value.kind {
        case MLN_GPU_SYNC_CPU_COMPLETE.rawValue:
          return .cpuComplete
        case MLN_GPU_SYNC_METAL_SHARED_EVENT.rawValue:
          return .metalSharedEvent(object, value: value.value)
        case MLN_GPU_SYNC_VULKAN_TIMELINE_SEMAPHORE.rawValue:
          return .vulkanTimelineSemaphore(object, value: value.value)
        case MLN_GPU_SYNC_OPENGL_FENCE.rawValue:
          return .openGLFence(object)
        case MLN_GPU_SYNC_WEBGPU_TOKEN.rawValue:
          return .webGPUToken(object, value: value.value)
        default:
          throw NativeStatusFailure.swiftNativeError(
            "Unknown producer synchronization kind"
          )
        }
      }
    }
  }

  public func withMetalTexture<Result>(
    _ body: (MetalOwnedTextureFrame) throws -> Result
  ) throws -> Result {
    try mapNativeFailure {
      try lock.withLock {
        var value = mln_metal_owned_texture_frame()
        value.size = UInt32(MemoryLayout<mln_metal_owned_texture_frame>.size)
        try checkStatus(mln_acquired_frame_get_metal_texture(
          requireLiveLocked(), &value
        ))
        return try body(MetalOwnedTextureFrame(
          generation: value.generation,
          width: value.width,
          height: value.height,
          scaleFactor: value.scale_factor,
          frameID: value.frame_id,
          texture: NativePointer(bitPattern: UInt(bitPattern: value.texture)),
          device: NativePointer(bitPattern: UInt(bitPattern: value.device)),
          pixelFormat: value.pixel_format
        ))
      }
    }
  }

  public func withVulkanTexture<Result>(
    _ body: (VulkanOwnedTextureFrame) throws -> Result
  ) throws -> Result {
    try mapNativeFailure {
      try lock.withLock {
        var value = mln_vulkan_owned_texture_frame()
        value.size = UInt32(MemoryLayout<mln_vulkan_owned_texture_frame>.size)
        try checkStatus(mln_acquired_frame_get_vulkan_texture(
          requireLiveLocked(), &value
        ))
        return try body(VulkanOwnedTextureFrame(
          generation: value.generation,
          width: value.width,
          height: value.height,
          scaleFactor: value.scale_factor,
          frameID: value.frame_id,
          image: NativePointer(bitPattern: UInt(bitPattern: value.image)),
          imageView: NativePointer(bitPattern: UInt(bitPattern: value
              .image_view)),
          device: NativePointer(bitPattern: UInt(bitPattern: value.device)),
          format: value.format,
          layout: value.layout
        ))
      }
    }
  }

  public func withOpenGLTexture<Result>(
    _ body: (OpenGLOwnedTextureFrame) throws -> Result
  ) throws -> Result {
    try mapNativeFailure {
      try lock.withLock {
        var value = mln_opengl_owned_texture_frame()
        value.size = UInt32(MemoryLayout<mln_opengl_owned_texture_frame>.size)
        try checkStatus(mln_acquired_frame_get_opengl_texture(
          requireLiveLocked(), &value
        ))
        return try body(OpenGLOwnedTextureFrame(
          generation: value.generation,
          width: value.width,
          height: value.height,
          scaleFactor: value.scale_factor,
          frameID: value.frame_id,
          texture: value.texture,
          target: value.target,
          internalFormat: value.internal_format,
          format: value.format,
          type: value.type
        ))
      }
    }
  }

  public func withWebGPUTexture<Result>(
    _ body: (WebGPUOwnedTextureFrame) throws -> Result
  ) throws -> Result {
    try mapNativeFailure {
      try lock.withLock {
        var value = mln_webgpu_owned_texture_frame()
        value.size = UInt32(MemoryLayout<mln_webgpu_owned_texture_frame>.size)
        try checkStatus(mln_acquired_frame_get_webgpu_texture(
          requireLiveLocked(), &value
        ))
        return try body(WebGPUOwnedTextureFrame(
          generation: value.generation,
          width: value.width,
          height: value.height,
          scaleFactor: value.scale_factor,
          frameID: value.frame_id,
          texture: NativePointer(bitPattern: UInt(bitPattern: value.texture)),
          textureView: NativePointer(
            bitPattern: UInt(bitPattern: value.texture_view)
          ),
          device: NativePointer(bitPattern: UInt(bitPattern: value.device)),
          format: value.format
        ))
      }
    }
  }

  public func release(
    consumerCompletion: GPUSynchronization = .cpuComplete
  ) throws {
    try mapNativeFailure {
      try lock.withLock {
        guard handle != 0 else { return }
        var completion = consumerCompletion.native
        try checkStatus(mln_acquired_frame_release(&handle, &completion))
      }
    }
  }

  private func requireLiveLocked() throws -> mln_acquired_frame {
    guard handle != 0 else {
      throw NativeStatusFailure.swiftNativeError("Acquired frame is released")
    }
    return handle
  }
}

public final class RenderSessionHandle: @unchecked Sendable {
  private let handle: NativeHandleBox<NativeRenderSessionHandle>
  private let frameWake: NativeWakeState
  private let driverWorkWake: NativeWakeState

  fileprivate init(
    session: NativeRenderSessionHandle,
    frameWake: NativeWakeState,
    driverWorkWake: NativeWakeState
  ) throws {
    self.frameWake = frameWake
    self.driverWorkWake = driverWorkWake
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

  public func setFrameReadyHandler(_ handler: (@Sendable () -> Void)?) throws {
    try handle.withLive { _ in frameWake.setHandler(handler) }
  }

  public func setDriverWorkReadyHandler(
    _ handler: (@Sendable () -> Void)?
  ) throws {
    try handle.withLive { _ in driverWorkWake.setHandler(handler) }
  }

  public func capabilities() throws -> RenderSessionCapabilities {
    try mapNativeFailure {
      try handle.withLive { session in
        var value = mln_render_session_capabilities()
        value.size = UInt32(MemoryLayout<mln_render_session_capabilities>.size)
        try checkStatus(mln_render_session_get_capabilities(
          session.raw, &value
        ))
        guard let driver = RenderDriver(rawValue: value.driver) else {
          throw NativeStatusFailure.swiftNativeError("Unknown render driver")
        }
        return RenderSessionCapabilities(
          driver: driver,
          textureRingDepth: value.texture_ring_depth,
          features: .init(rawValue: value.flags)
        )
      }
    }
  }

  public func snapshot() throws -> RenderSessionSnapshot {
    try mapNativeFailure {
      try handle.withLive { session in
        var value = mln_render_session_snapshot()
        value.size = UInt32(MemoryLayout<mln_render_session_snapshot>.size)
        try checkStatus(mln_render_session_get_snapshot(session.raw, &value))
        return RenderSessionSnapshot(
          state: RenderSessionState(rawValue: value.state),
          driver: RenderDriver(rawValue: value.driver),
          latestResult: .fromNative(value.latest_result),
          extent: RenderTargetExtent(
            width: value.extent.width,
            height: value.extent.height,
            scaleFactor: value.extent.scale_factor
          ),
          generation: value.generation,
          mapUpdateGeneration: value.map_update_generation,
          renderedUpdateGeneration: value.rendered_update_generation,
          extentGeneration: value.extent_generation,
          frameGeneration: value.frame_generation,
          latestDemandToken: value.latest_demand_token,
          pendingDemandCount: value.pending_demand_count,
          acquiredFrameCount: value.acquired_frame_count,
          targetReady: value.target_ready,
          pendingChanges: value.pending_changes
        )
      }
    }
  }

  public func requestFrame(_ demand: FrameDemand = FrameDemand()) throws {
    try mapNativeFailure {
      try handle.withLive { session in
        var value = demand.native
        try checkStatus(mln_render_session_request_frame(session.raw, &value))
      }
    }
  }

  public func drainFrameResults() throws -> [RenderFrameResult] {
    return try mapNativeFailure {
      try handle.withLive { session in
        var batch: mln_render_frame_batch = 0
        try checkStatus(mln_render_session_drain_frame_results(
          session.raw, &batch
        ))
        guard batch != 0 else { return [] }
        defer { mln_render_frame_batch_release(batch) }
        var count = 0
        try checkStatus(mln_render_frame_batch_count(batch, &count))
        return try (0 ..< count).map { index in
          var value = mln_render_frame_result()
          value.size = UInt32(MemoryLayout<mln_render_frame_result>.size)
          try checkStatus(mln_render_frame_batch_get(batch, index, &value))
          return RenderFrameResult(value)
        }
      }
    }
  }

  public func acquireFrame() throws -> AcquiredFrameHandle? {
    try mapNativeFailure {
      try handle.withLive { session in
        var frame: mln_acquired_frame = 0
        let status = mln_render_session_acquire_frame(session.raw, &frame)
        if status == MLN_STATUS_NOT_READY { return nil }
        try checkStatus(status)
        return AcquiredFrameHandle(handle: frame)
      }
    }
  }

  @discardableResult
  public func serviceDriverWork(maxWork: Int = 64) throws -> Int {
    guard maxWork >= 0 else {
      throw NativeStatusFailure.swiftInvalidArgument(
        "maxWork cannot be negative"
      )
    }
    return try mapNativeFailure {
      try handle.withLive { session in
        var serviced = 0
        try checkStatus(mln_render_session_service_driver_work(
          session.raw, maxWork, &serviced
        ))
        return serviced
      }
    }
  }

  public func resize(_ extent: RenderTargetExtent) async throws {
    try await performCompletion { session, completion in
      var value = extent.nativeInput.native
      return mln_render_session_resize(session, &value, completion)
    }
  }

  public func setMetalBorrowedTextureTarget(
    _ descriptor: MetalBorrowedTextureDescriptor
  ) async throws {
    let future = try mapNativeFailure {
      try descriptor.nativeInput.withNativeDescriptor { descriptor in
        try startCompletion { session, completion in
          mln_metal_borrowed_texture_set_target(
            session, descriptor, completion
          )
        }
      }
    }
    try await future.value()
  }

  public func setMetalSurfaceTarget(
    _ descriptor: MetalSurfaceDescriptor
  ) async throws {
    let future = try mapNativeFailure {
      try descriptor.nativeInput.withNativeDescriptor { descriptor in
        try startCompletion { session, completion in
          mln_metal_surface_set_target(session, descriptor, completion)
        }
      }
    }
    try await future.value()
  }

  public func setVulkanSurfaceTarget(
    _ descriptor: VulkanSurfaceDescriptor
  ) async throws {
    let future = try mapNativeFailure {
      try descriptor.nativeInput.withNativeDescriptor { descriptor in
        try startCompletion { session, completion in
          mln_vulkan_surface_set_target(session, descriptor, completion)
        }
      }
    }
    try await future.value()
  }

  public func setOpenGLSurfaceTarget(
    _ descriptor: OpenGLSurfaceDescriptor
  ) async throws {
    let future = try mapNativeFailure {
      try descriptor.nativeInput.withNativeDescriptor { descriptor in
        try startCompletion { session, completion in
          mln_opengl_surface_set_target(session, descriptor, completion)
        }
      }
    }
    try await future.value()
  }

  public func setWebGPUSurfaceTarget(
    _ descriptor: WebGPUSurfaceDescriptor
  ) async throws {
    let future = try mapNativeFailure {
      try descriptor.nativeInput.withNativeDescriptor { descriptor in
        try startCompletion { session, completion in
          mln_webgpu_surface_set_target(session, descriptor, completion)
        }
      }
    }
    try await future.value()
  }

  public func barrier() async throws {
    try await performCompletion {
      mln_render_session_barrier($0, $1)
    }
  }

  public func reduceMemoryUse() async throws {
    try await performCompletion(mln_render_session_reduce_memory_use)
  }

  public func clearData() async throws {
    try await performCompletion(mln_render_session_clear_data)
  }

  public func dumpDebugLogs() async throws {
    try await performCompletion(mln_render_session_dump_debug_logs)
  }

  public func readPremultipliedRGBA8() async throws
    -> PremultipliedRGBA8Image
  {
    let future = try startCompletion(
      mln_texture_read_premultiplied_rgba8
    ) { result in
      let value: mln_texture_readback_result = try NativeCompletion
        .value(result)
      return try PremultipliedRGBA8Image(
        info: TextureImageInfo(native: NativeTextureImageInfo(value.info)),
        data: NativeCompletion.dataView(value.data)
      )
    }
    return try await future.value()
  }

  public func detach() async throws {
    try await performCompletion(mln_render_session_detach)
    frameWake.setHandler(nil)
    driverWorkWake.setHandler(nil)
  }

  /// Irreversibly closes control and mailboxes without a graphics call.
  ///
  /// The call waits for the map's in-flight tile work before returning, so no
  /// library thread touches the session's target or device afterward and the
  /// host may destroy its graphics objects immediately. Do not call it from a
  /// MapLibre worker callback.
  public func abandon() throws -> RenderAbandonResult {
    try mapNativeFailure {
      try handle.withLive { session in
        var value = mln_render_abandon_result()
        value.size = UInt32(MemoryLayout<mln_render_abandon_result>.size)
        try checkStatus(mln_render_session_abandon(session.raw, &value))
        frameWake.setHandler(nil)
        driverWorkWake.setHandler(nil)
        guard let disposition = RenderAbandonResult.Disposition(
          rawValue: value.disposition
        ) else {
          throw NativeStatusFailure.swiftNativeError(
            "Unknown render abandonment disposition"
          )
        }
        return RenderAbandonResult(
          disposition: disposition,
          quarantinedResourceCount: value.quarantined_resource_count
        )
      }
    }
  }

  public func close() throws {
    try mapNativeFailure {
      try handle.closeOnce { session in
        frameWake.setHandler(nil)
        driverWorkWake.setHandler(nil)
        try checkStatus(mln_render_session_destroy(session.raw))
      }
    }
  }

  func startCompletion<Value: Sendable>(
    _ body: (mln_render_session, UnsafePointer<mln_completion>) -> mln_status,
    convert: @escaping (UnsafePointer<mln_completion_result>) throws -> Value
  ) throws -> NativeFuture<Value> {
    try handle.withLive { session in
      try NativeCompletion.start(
        { completion in body(session.raw, completion) },
        convert: convert
      )
    }
  }

  func startCompletion(
    _ body: (mln_render_session, UnsafePointer<mln_completion>) -> mln_status
  ) throws -> NativeFuture<Void> {
    try startCompletion(body) { _ in () }
  }

  func performCompletion(
    _ body: (mln_render_session, UnsafePointer<mln_completion>) -> mln_status
  ) async throws {
    try await mapNativeFailure { try startCompletion(body) }.value()
  }
}

public extension MapHandle {
  private typealias StartedRenderSession = (
    attachment: NativeRender.Attachment,
    frameWake: NativeWakeState,
    driverWorkWake: NativeWakeState
  )

  private func startRenderSession(
    options: RenderSessionAttachOptions,
    _ start: (UnsafePointer<mln_render_session_attach_options>) throws
      -> NativeRender.Attachment
  ) throws -> StartedRenderSession {
    let frameWake = NativeWakeState()
    let driverWorkWake = NativeWakeState()
    let frameDescriptor = frameWake.makeDescriptor()
    let driverDescriptor = driverWorkWake.makeDescriptor()
    do {
      let attachment = try mapNativeFailure {
        try options.withNative(
          frameWake: frameDescriptor,
          driverWorkWake: driverDescriptor,
          start
        )
      }
      return (attachment, frameWake, driverWorkWake)
    } catch {
      frameWake.releaseRejectedDescriptor()
      driverWorkWake.releaseRejectedDescriptor()
      throw error
    }
  }

  private func finishRenderSession(
    _ started: StartedRenderSession
  ) throws -> RenderSessionAttachment {
    let session = try RenderSessionHandle(
      session: started.attachment.session,
      frameWake: started.frameWake,
      driverWorkWake: started.driverWorkWake
    )
    let completed = Task { [session] in
      try await mapNativeFailure {
        try await started.attachment.completion.value()
      }
      withExtendedLifetime(session) {}
    }
    return RenderSessionAttachment(session: session, completion: completed)
  }

  func attachMetalSurface(
    _ descriptor: MetalSurfaceDescriptor,
    options: RenderSessionAttachOptions
  ) throws -> RenderSessionAttachment {
    let attachment = try descriptor.nativeInput.withNativeDescriptor { value in
      try startRenderSession(options: options) { options in
        try NativeRender.metalSurfaceAttachStart(
          map: requireLiveHandle(),
          descriptor: value,
          options: options
        )
      }
    }
    return try finishRenderSession(attachment)
  }

  func attachVulkanSurface(
    _ descriptor: VulkanSurfaceDescriptor,
    options: RenderSessionAttachOptions
  ) throws -> RenderSessionAttachment {
    let attachment = try descriptor.nativeInput.withNativeDescriptor { value in
      try startRenderSession(options: options) { options in
        try NativeRender.vulkanSurfaceAttachStart(
          map: requireLiveHandle(),
          descriptor: value,
          options: options
        )
      }
    }
    return try finishRenderSession(attachment)
  }

  func attachOpenGLSurface(
    _ descriptor: OpenGLSurfaceDescriptor,
    options: RenderSessionAttachOptions = .init(driver: .callerGraphicsThread)
  ) throws -> RenderSessionAttachment {
    let attachment = try descriptor.nativeInput.withNativeDescriptor { value in
      try startRenderSession(options: options) { options in
        try NativeRender.openGLSurfaceAttachStart(
          map: requireLiveHandle(),
          descriptor: value,
          options: options
        )
      }
    }
    return try finishRenderSession(attachment)
  }

  func attachMetalOwnedTexture(
    _ descriptor: MetalOwnedTextureDescriptor,
    options: RenderSessionAttachOptions
  ) throws -> RenderSessionAttachment {
    let attachment = try descriptor.nativeInput.withNativeDescriptor { value in
      try startRenderSession(options: options) { options in
        try NativeRender.metalOwnedTextureAttachStart(
          map: requireLiveHandle(),
          descriptor: value,
          options: options
        )
      }
    }
    return try finishRenderSession(attachment)
  }

  func attachMetalBorrowedTexture(
    _ descriptor: MetalBorrowedTextureDescriptor,
    options: RenderSessionAttachOptions
  ) throws -> RenderSessionAttachment {
    let attachment = try descriptor.nativeInput.withNativeDescriptor { value in
      try startRenderSession(options: options) { options in
        try NativeRender.metalBorrowedTextureAttachStart(
          map: requireLiveHandle(),
          descriptor: value,
          options: options
        )
      }
    }
    return try finishRenderSession(attachment)
  }

  func attachVulkanOwnedTexture(
    _ descriptor: VulkanOwnedTextureDescriptor,
    options: RenderSessionAttachOptions
  ) throws -> RenderSessionAttachment {
    let attachment = try descriptor.nativeInput.withNativeDescriptor { value in
      try startRenderSession(options: options) { options in
        try NativeRender.vulkanOwnedTextureAttachStart(
          map: requireLiveHandle(),
          descriptor: value,
          options: options
        )
      }
    }
    return try finishRenderSession(attachment)
  }

  func attachVulkanBorrowedTexture(
    _ descriptor: VulkanBorrowedTextureDescriptor,
    options: RenderSessionAttachOptions
  ) throws -> RenderSessionAttachment {
    let attachment = try descriptor.nativeInput.withNativeDescriptor { value in
      try startRenderSession(options: options) { options in
        try NativeRender.vulkanBorrowedTextureAttachStart(
          map: requireLiveHandle(),
          descriptor: value,
          options: options
        )
      }
    }
    return try finishRenderSession(attachment)
  }

  func attachOpenGLOwnedTexture(
    _ descriptor: OpenGLOwnedTextureDescriptor,
    options: RenderSessionAttachOptions = .init(driver: .callerGraphicsThread)
  ) throws -> RenderSessionAttachment {
    let attachment = try descriptor.nativeInput.withNativeDescriptor { value in
      try startRenderSession(options: options) { options in
        try NativeRender.openGLOwnedTextureAttachStart(
          map: requireLiveHandle(),
          descriptor: value,
          options: options
        )
      }
    }
    return try finishRenderSession(attachment)
  }

  func attachOpenGLBorrowedTexture(
    _ descriptor: OpenGLBorrowedTextureDescriptor,
    options: RenderSessionAttachOptions = .init(driver: .callerGraphicsThread)
  ) throws -> RenderSessionAttachment {
    let attachment = try descriptor.nativeInput.withNativeDescriptor { value in
      try startRenderSession(options: options) { options in
        try NativeRender.openGLBorrowedTextureAttachStart(
          map: requireLiveHandle(),
          descriptor: value,
          options: options
        )
      }
    }
    return try finishRenderSession(attachment)
  }

  func attachWebGPUSurface(
    _ descriptor: WebGPUSurfaceDescriptor,
    options: RenderSessionAttachOptions = .init(driver: .callerGraphicsThread)
  ) throws -> RenderSessionAttachment {
    let attachment = try descriptor.nativeInput.withNativeDescriptor { value in
      try startRenderSession(options: options) { options in
        try NativeRender.webGPUSurfaceAttachStart(
          map: requireLiveHandle(),
          descriptor: value,
          options: options
        )
      }
    }
    return try finishRenderSession(attachment)
  }

  func attachWebGPUOwnedTexture(
    _ descriptor: WebGPUOwnedTextureDescriptor,
    options: RenderSessionAttachOptions = .init(driver: .callerGraphicsThread)
  ) throws -> RenderSessionAttachment {
    let attachment = try descriptor.nativeInput.withNativeDescriptor { value in
      try startRenderSession(options: options) { options in
        try NativeRender.webGPUOwnedTextureAttachStart(
          map: requireLiveHandle(),
          descriptor: value,
          options: options
        )
      }
    }
    return try finishRenderSession(attachment)
  }

  func attachWebGPUBorrowedTexture(
    _ descriptor: WebGPUBorrowedTextureDescriptor,
    options: RenderSessionAttachOptions = .init(driver: .callerGraphicsThread)
  ) throws -> RenderSessionAttachment {
    let attachment = try descriptor.nativeInput.withNativeDescriptor { value in
      try startRenderSession(options: options) { options in
        try NativeRender.webGPUBorrowedTextureAttachStart(
          map: requireLiveHandle(),
          descriptor: value,
          options: options
        )
      }
    }
    return try finishRenderSession(attachment)
  }
}
