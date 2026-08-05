internal import CMaplibreNativeC
import Foundation

public struct RuntimeOptions: Equatable, Sendable {
  public var assetPath: String?
  public var cachePath: String?

  public init(
    assetPath: String? = nil,
    cachePath: String? = nil
  ) {
    self.assetPath = assetPath
    self.cachePath = cachePath
  }

  var nativeInput: NativeRuntimeOptionsInput {
    NativeRuntimeOptionsInput(
      assetPath: assetPath,
      cachePath: cachePath
    )
  }
}

public enum RuntimeEventType: Sendable, Hashable {
  case mapCameraWillChange
  case mapCameraIsChanging
  case mapCameraDidChange
  case mapStyleLoaded
  case mapLoadingStarted
  case mapLoadingFinished
  case mapLoadingFailed
  case mapIdle
  case mapRenderUpdateAvailable
  case mapRenderError
  case mapStillImageFinished
  case mapStillImageFailed
  case mapRenderFrameStarted
  case mapRenderFrameFinished
  case mapRenderMapStarted
  case mapRenderMapFinished
  case mapStyleImageMissing
  case mapTileAction
  case offlineRegionStatusChanged
  case offlineRegionResponseError
  case offlineRegionTileCountLimitExceeded
  case offlineOperationCompleted
  case mapCameraTransitionFinished
  case unknown(UInt32)

  public static func fromNative(_ rawValue: UInt32) -> Self {
    switch rawValue {
    case 1: .mapCameraWillChange
    case 2: .mapCameraIsChanging
    case 3: .mapCameraDidChange
    case 4: .mapStyleLoaded
    case 5: .mapLoadingStarted
    case 6: .mapLoadingFinished
    case 7: .mapLoadingFailed
    case 8: .mapIdle
    case 9: .mapRenderUpdateAvailable
    case 10: .mapRenderError
    case 11: .mapStillImageFinished
    case 12: .mapStillImageFailed
    case 13: .mapRenderFrameStarted
    case 14: .mapRenderFrameFinished
    case 15: .mapRenderMapStarted
    case 16: .mapRenderMapFinished
    case 17: .mapStyleImageMissing
    case 18: .mapTileAction
    case 19: .offlineRegionStatusChanged
    case 20: .offlineRegionResponseError
    case 21: .offlineRegionTileCountLimitExceeded
    case 22: .offlineOperationCompleted
    case 23: .mapCameraTransitionFinished
    default: .unknown(rawValue)
    }
  }
}

/// Identifies one map for the life of the process. This is an identity value,
/// not a handle: it grants no access to the map.
public struct MapId: Hashable, Sendable {
  /// The handle id the C API issued for this map.
  public let value: UInt64
}

public enum RuntimeEventSource: Equatable, Sendable {
  case runtime
  case map(MapId)
  case unknown(sourceType: UInt32, source: UInt64)

  static func fromNative(sourceType: UInt32, sourceId: UInt64) -> Self {
    switch sourceType {
    case 0: return .runtime
    case 1: return .map(MapId(value: sourceId))
    default: return .unknown(sourceType: sourceType, source: sourceId)
    }
  }
}

/// Camera change kinds reported by camera will-change and did-change events.
///
/// `RuntimeEvent.code` carries this value for `.mapCameraWillChange` and
/// `.mapCameraDidChange`. Convert it with
/// `CameraChangeMode.fromNative(UInt32(bitPattern: event.code))`.
public enum CameraChangeMode: Sendable, Hashable {
  /// The camera reached its new value without an animated transition.
  case immediate
  /// The camera moved as part of an animated transition.
  case animated
  case unknown(UInt32)

  public static func fromNative(_ rawValue: UInt32) -> Self {
    switch rawValue {
    case 0: .immediate
    case 1: .animated
    default: .unknown(rawValue)
    }
  }
}

public enum RenderMode: Sendable, Hashable {
  case partial
  case full
  case unknown(UInt32)

  public static func fromNative(_ rawValue: UInt32) -> Self {
    switch rawValue {
    case 0: .partial
    case 1: .full
    default: .unknown(rawValue)
    }
  }
}

public struct RenderingStats: Equatable, Sendable {
  public let encodingTime: Double
  public let renderingTime: Double
  public let frameCount: Int64
  public let drawCallCount: Int64
  public let totalDrawCallCount: Int64

  init(native: NativeRenderingStats) {
    encodingTime = native.encodingTime
    renderingTime = native.renderingTime
    frameCount = native.frameCount
    drawCallCount = native.drawCallCount
    totalDrawCallCount = native.totalDrawCallCount
  }
}

public struct RenderFrameEvent: Equatable, Sendable {
  public let mode: RenderMode
  public let needsRepaint: Bool
  public let placementChanged: Bool
  public let stats: RenderingStats

  init(native: NativeRenderFrameEvent) {
    mode = RenderMode.fromNative(native.mode)
    needsRepaint = native.needsRepaint
    placementChanged = native.placementChanged
    stats = RenderingStats(native: native.stats)
  }
}

public struct RenderMapEvent: Equatable, Sendable {
  public let mode: RenderMode

  init(native: NativeRenderMapEvent) {
    mode = RenderMode.fromNative(native.mode)
  }
}

public enum TileOperation: Sendable, Hashable {
  case requestedFromCache
  case requestedFromNetwork
  case loadFromNetwork
  case loadFromCache
  case startParse
  case endParse
  case error
  case cancelled
  case null
  case unknown(UInt32)

  public static func fromNative(_ rawValue: UInt32) -> Self {
    switch rawValue {
    case 0: .requestedFromCache
    case 1: .requestedFromNetwork
    case 2: .loadFromNetwork
    case 3: .loadFromCache
    case 4: .startParse
    case 5: .endParse
    case 6: .error
    case 7: .cancelled
    case 8: .null
    default: .unknown(rawValue)
    }
  }
}

public struct TileId: Equatable, Sendable {
  public let overscaledZ: UInt32
  public let wrap: Int32
  public let canonicalZ: UInt32
  public let canonicalX: UInt32
  public let canonicalY: UInt32

  init(native: NativeTileId) {
    overscaledZ = native.overscaledZ
    wrap = native.wrap
    canonicalZ = native.canonicalZ
    canonicalX = native.canonicalX
    canonicalY = native.canonicalY
  }
}

public struct TileActionEvent: Equatable, Sendable {
  public let operation: TileOperation
  public let tileId: TileId
  public let sourceId: String

  init(native: NativeTileActionEvent) {
    operation = TileOperation.fromNative(native.operation)
    tileId = TileId(native: native.tileId)
    sourceId = native.sourceId
  }
}

public struct OfflineRegionStatus: Equatable, Sendable {
  public let downloadState: UInt32
  public let completedResourceCount: UInt64
  public let completedResourceSize: UInt64
  public let completedTileCount: UInt64
  public let requiredTileCount: UInt64
  public let completedTileSize: UInt64
  public let requiredResourceCount: UInt64
  public let requiredResourceCountIsPrecise: Bool
  public let complete: Bool

  init(native: NativeOfflineRegionStatus) {
    downloadState = native.downloadState
    completedResourceCount = native.completedResourceCount
    completedResourceSize = native.completedResourceSize
    completedTileCount = native.completedTileCount
    requiredTileCount = native.requiredTileCount
    completedTileSize = native.completedTileSize
    requiredResourceCount = native.requiredResourceCount
    requiredResourceCountIsPrecise = native.requiredResourceCountIsPrecise
    complete = native.complete
  }
}

public struct OfflineRegionStatusEvent: Equatable, Sendable {
  public let regionId: Int64
  public let status: OfflineRegionStatus

  init(native: NativeOfflineRegionStatusEvent) {
    regionId = native.regionId
    status = OfflineRegionStatus(native: native.status)
  }
}

public struct OfflineRegionResponseErrorEvent: Equatable, Sendable {
  public let regionId: Int64
  public let reason: ResourceErrorReason

  init(native: NativeOfflineRegionResponseErrorEvent) {
    regionId = native.regionId
    reason = ResourceErrorReason.fromNative(native.reason)
  }
}

public struct OfflineRegionTileCountLimitEvent: Equatable, Sendable {
  public let regionId: Int64
  public let limit: UInt64

  init(native: NativeOfflineRegionTileCountLimitEvent) {
    regionId = native.regionId
    limit = native.limit
  }
}

public struct OfflineOperationCompletedEvent: Equatable, Sendable {
  public let operationId: UInt64
  public let operationKind: UInt32
  public let resultKind: UInt32
  public let resultStatus: Int32
  public let found: Bool

  init(native: NativeOfflineOperationCompletedEvent) {
    operationId = native.operationId
    operationKind = native.operationKind
    resultKind = native.resultKind
    resultStatus = native.resultStatus
    found = native.found
  }
}

/// Payload of a `.mapCameraTransitionFinished` event.
public struct CameraTransitionFinishedEvent: Equatable, Sendable {
  /// The `AnimationOptions.transitionId` that started the finished transition.
  public let transitionId: UInt64

  init(native: NativeCameraTransitionFinishedEvent) {
    transitionId = native.transitionId
  }
}

public enum RuntimeEventPayload: Equatable, Sendable {
  case none
  case renderFrame(RenderFrameEvent)
  case renderMap(RenderMapEvent)
  case styleImageMissing(String)
  case tileAction(TileActionEvent)
  case offlineRegionStatus(OfflineRegionStatusEvent)
  case offlineRegionResponseError(OfflineRegionResponseErrorEvent)
  case offlineRegionTileCountLimit(OfflineRegionTileCountLimitEvent)
  case offlineOperationCompleted(OfflineOperationCompletedEvent)
  case cameraTransitionFinished(CameraTransitionFinishedEvent)
  case unknown(type: UInt32, byteCount: Int)

  init(native: NativeRuntimeEventPayload) {
    switch native {
    case .none:
      self = .none
    case let .renderFrame(event):
      self = .renderFrame(RenderFrameEvent(native: event))
    case let .renderMap(event):
      self = .renderMap(RenderMapEvent(native: event))
    case let .styleImageMissing(imageId):
      self = .styleImageMissing(imageId)
    case let .tileAction(event):
      self = .tileAction(TileActionEvent(native: event))
    case let .offlineRegionStatus(event):
      self = .offlineRegionStatus(OfflineRegionStatusEvent(native: event))
    case let .offlineRegionResponseError(event):
      self =
        .offlineRegionResponseError(
          OfflineRegionResponseErrorEvent(native: event)
        )
    case let .offlineRegionTileCountLimit(event):
      self =
        .offlineRegionTileCountLimit(
          OfflineRegionTileCountLimitEvent(native: event)
        )
    case let .offlineOperationCompleted(event):
      self =
        .offlineOperationCompleted(
          OfflineOperationCompletedEvent(native: event)
        )
    case let .cameraTransitionFinished(event):
      self =
        .cameraTransitionFinished(
          CameraTransitionFinishedEvent(native: event)
        )
    case let .unknown(type, byteCount):
      self = .unknown(type: type, byteCount: byteCount)
    }
  }
}

public struct RuntimeEvent: Equatable, Sendable {
  public let type: RuntimeEventType
  public let source: RuntimeEventSource
  /// Secondary event detail whose meaning `type` selects.
  ///
  /// - `.mapCameraWillChange` and `.mapCameraDidChange` carry a
  ///   `CameraChangeMode` raw value, read with
  ///   `CameraChangeMode.fromNative(UInt32(bitPattern: code))`.
  /// - `.mapLoadingFailed` carries the ordinal of MapLibre Native's internal
  ///   map load error kind, which this API leaves unnamed; `message` holds the
  ///   failure text.
  /// - `.offlineOperationCompleted` carries the operation result as a native
  ///   status value, the same value the payload reports in `resultStatus`.
  /// - Every other event type leaves it 0.
  public let code: Int32
  public let message: String
  public let payload: RuntimeEventPayload

  init(native: NativeRuntimeEvent) {
    type = RuntimeEventType.fromNative(native.type)
    source = RuntimeEventSource.fromNative(
      sourceType: native.sourceType,
      sourceId: native.sourceId
    )
    code = native.code
    message = native.message
    payload = RuntimeEventPayload(native: native.payload)
  }
}

public final class RuntimeHandle {
  private let handle: NativeHandleBox<NativeRuntimeHandle>
  private var resourceTransform: NativeResourceTransformState?
  private var httpHeaderTransform: NativeHttpHeaderTransformState?
  private var resourceProvider: NativeResourceProviderState?

  public init(options: RuntimeOptions = RuntimeOptions()) throws {
    let runtime = try mapNativeFailure {
      try options.nativeInput.withNativeOptions { nativeOptions in
        try NativeRuntime.create(nativeOptions)
      }
    }
    handle = try NativeHandleBox(typeName: "RuntimeHandle", handle: runtime)
  }

  public var isClosed: Bool {
    handle.isClosed
  }

  public func close() throws {
    try handle.closeOnce { handle in
      try checkStatus(mln_runtime_destroy(handle.raw))
    }
    resourceTransform = nil
    httpHeaderTransform = nil
    resourceProvider = nil
  }

  func requireLiveHandle() throws -> NativeRuntimeHandle {
    try handle.requireLive()
  }

  /// Advances this runtime: parks the owner thread when `timeout` allows it,
  /// then drains the owner-thread task queues. Drain queued runtime events with
  /// ``pollEvent()`` afterwards.
  ///
  /// `timeout` is in seconds. Zero drains and returns, a positive value parks
  /// for up to that long, and `nil` parks until a wake arrives. Timers and
  /// ready file descriptors wake the runtime only when they queue owner-thread
  /// work, so pass a bounded timeout to cap how long a call waits.
  ///
  /// A non-zero timeout blocks the calling thread. Call it outside any lock
  /// that a thread signalling a `WakeSource` takes.
  public func pump(timeout: TimeInterval? = 0) throws {
    try mapNativeFailure {
      let timeoutMilliseconds: Int64
      if let timeout {
        // A negative or non-finite timeout collapses to no wait; `nil` is the
        // spelling for an unbounded park. The upper clamp keeps the conversion
        // inside Int64.
        let milliseconds = timeout.isFinite ? (timeout * 1000).rounded() : 0
        timeoutMilliseconds = Int64(min(max(milliseconds, 0), 9.0e18))
      } else {
        timeoutMilliseconds = -1
      }
      try checkStatus(
        mln_runtime_pump(handle.requireLive().raw, timeoutMilliseconds)
      )
    }
  }

  /// Acquires a wake source that releases this runtime's parked owner thread.
  /// The returned source is usable from any thread, and the caller closes it.
  public func wakeSource() throws -> WakeSource {
    try mapNativeFailure {
      var source = mln_wake_source()
      try checkStatus(mln_runtime_wake_source_acquire(
        handle.requireLive().raw,
        &source
      ))
      return try WakeSource(handle: NativeWakeSourceHandle(raw: source))
    }
  }

  /// Polls and copies the next queued runtime event, returning `nil` when the
  /// queue is empty.
  ///
  /// A polled map-style-loaded event releases the map's detached custom
  /// geometry source callbacks, so drain the queue to keep sources the new
  /// style dropped from lingering.
  public func pollEvent() throws -> RuntimeEvent? {
    try mapNativeFailure {
      guard let event = try NativeRuntime.pollEvent(handle.requireLive()) else {
        return nil
      }
      let runtimeEvent = try RuntimeEvent(native: NativeRuntimeEvent(event))
      MapHandle.handleRuntimeEvent(runtimeEvent)
      return runtimeEvent
    }
  }

  public func setResourceTransform(
    _ callback: @escaping @Sendable (ResourceTransformRequest) -> String?
  ) throws {
    let replacement = NativeResourceTransformState { nativeRequest in
      callback(ResourceTransformRequest(native: nativeRequest))
    }
    try mapNativeFailure {
      try replacement.withDescriptor { descriptor in
        try checkStatus(mln_runtime_set_resource_transform(
          handle.requireLive().raw,
          descriptor
        ))
      }
    }
    resourceTransform = replacement
  }

  public func clearResourceTransform() throws {
    try mapNativeFailure {
      try checkStatus(mln_runtime_clear_resource_transform(handle
          .requireLive().raw))
    }
    resourceTransform = nil
  }

  public func setHttpHeaderTransform(
    _ callback: @escaping @Sendable (HttpHeaderTransformRequest) -> [HttpHeader]
  ) throws {
    let replacement = NativeHttpHeaderTransformState(callback)
    try mapNativeFailure {
      try replacement.withDescriptor { descriptor in
        try checkStatus(mln_runtime_set_http_header_transform(
          handle.requireLive().raw,
          descriptor
        ))
      }
    }
    httpHeaderTransform = replacement
  }

  public func clearHttpHeaderTransform() throws {
    try mapNativeFailure {
      try checkStatus(mln_runtime_clear_http_header_transform(handle
          .requireLive().raw))
    }
    httpHeaderTransform = nil
  }

  public func setResourceProvider(
    _ callback: @escaping @Sendable (ResourceRequest, ResourceRequestHandle)
      -> ResourceProviderDecision
  ) throws {
    let replacement =
      NativeResourceProviderState { nativeRequest, nativeHandle in
        let request = ResourceRequest(native: nativeRequest)
        let handle = ResourceRequestHandle(state: nativeHandle)
        switch callback(request, handle) {
        case .passThrough:
          return 0
        case .handle:
          return 1
        }
      }
    try mapNativeFailure {
      try replacement.withDescriptor { descriptor in
        try checkStatus(mln_runtime_set_resource_provider(
          handle.requireLive().raw,
          descriptor
        ))
      }
    }
    resourceProvider = replacement
  }

  public func clearResourceProvider() throws {
    try mapNativeFailure {
      try checkStatus(mln_runtime_clear_resource_provider(handle
          .requireLive().raw))
    }
    resourceProvider = nil
  }
}
