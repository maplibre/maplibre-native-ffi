internal import CMaplibreNativeC
import Foundation

public struct RuntimeOptions: Equatable, Sendable {
  public var assetPath: String?
  public var cachePath: String?
  /// Runtime-originated event types this runtime queues, every type the
  /// library reports unless the host narrows it.
  public var eventMask: RuntimeEventMask

  public init(
    assetPath: String? = nil,
    cachePath: String? = nil,
    eventMask: RuntimeEventMask = .runtimeOptionsDefault
  ) {
    self.assetPath = assetPath
    self.cachePath = cachePath
    self.eventMask = eventMask
  }

  var nativeInput: NativeRuntimeOptionsInput {
    NativeRuntimeOptionsInput(
      assetPath: assetPath,
      cachePath: cachePath,
      eventMask: eventMask.rawValue
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
    case 23: .mapCameraTransitionFinished
    default: .unknown(rawValue)
    }
  }
}

/// The event types a map or a runtime queues.
///
/// One bit per ``RuntimeEventType``, taken from the C API's own mask constants
/// so the two cannot drift apart. ``MapHandle/setEventMask(_:)`` reads the
/// map-originated bits and ``RuntimeHandle/setEventMask(_:)`` reads the
/// runtime-originated ones, ignoring the rest, so ``all`` is a value both
/// accept and a handle reports every bit last set.
///
/// An unselected event type is never built and never queued.
public struct RuntimeEventMask: OptionSet, Sendable, Hashable {
  public let rawValue: UInt64

  public init(rawValue: UInt64) {
    self.rawValue = rawValue
  }

  private static func native(
    _ mask: mln_runtime_event_mask
  ) -> RuntimeEventMask {
    RuntimeEventMask(rawValue: mask.rawValue)
  }

  /// Selecting no event type is spelled `[]`, the OptionSet empty literal.
  public static let mapCameraWillChange =
    native(MLN_RUNTIME_EVENT_MASK_MAP_CAMERA_WILL_CHANGE)
  public static let mapCameraIsChanging =
    native(MLN_RUNTIME_EVENT_MASK_MAP_CAMERA_IS_CHANGING)
  public static let mapCameraDidChange =
    native(MLN_RUNTIME_EVENT_MASK_MAP_CAMERA_DID_CHANGE)
  public static let mapStyleLoaded =
    native(MLN_RUNTIME_EVENT_MASK_MAP_STYLE_LOADED)
  public static let mapLoadingStarted =
    native(MLN_RUNTIME_EVENT_MASK_MAP_LOADING_STARTED)
  public static let mapLoadingFinished =
    native(MLN_RUNTIME_EVENT_MASK_MAP_LOADING_FINISHED)
  public static let mapLoadingFailed =
    native(MLN_RUNTIME_EVENT_MASK_MAP_LOADING_FAILED)
  public static let mapIdle = native(MLN_RUNTIME_EVENT_MASK_MAP_IDLE)
  public static let mapRenderUpdateAvailable =
    native(MLN_RUNTIME_EVENT_MASK_MAP_RENDER_UPDATE_AVAILABLE)
  public static let mapRenderError =
    native(MLN_RUNTIME_EVENT_MASK_MAP_RENDER_ERROR)
  public static let mapStillImageFinished =
    native(MLN_RUNTIME_EVENT_MASK_MAP_STILL_IMAGE_FINISHED)
  public static let mapStillImageFailed =
    native(MLN_RUNTIME_EVENT_MASK_MAP_STILL_IMAGE_FAILED)
  public static let mapRenderFrameStarted =
    native(MLN_RUNTIME_EVENT_MASK_MAP_RENDER_FRAME_STARTED)
  public static let mapRenderFrameFinished =
    native(MLN_RUNTIME_EVENT_MASK_MAP_RENDER_FRAME_FINISHED)
  public static let mapRenderMapStarted =
    native(MLN_RUNTIME_EVENT_MASK_MAP_RENDER_MAP_STARTED)
  public static let mapRenderMapFinished =
    native(MLN_RUNTIME_EVENT_MASK_MAP_RENDER_MAP_FINISHED)
  public static let mapStyleImageMissing =
    native(MLN_RUNTIME_EVENT_MASK_MAP_STYLE_IMAGE_MISSING)
  public static let mapTileAction =
    native(MLN_RUNTIME_EVENT_MASK_MAP_TILE_ACTION)
  public static let mapCameraTransitionFinished =
    native(MLN_RUNTIME_EVENT_MASK_MAP_CAMERA_TRANSITION_FINISHED)
  public static let offlineRegionStatusChanged =
    native(MLN_RUNTIME_EVENT_MASK_OFFLINE_REGION_STATUS_CHANGED)
  public static let offlineRegionResponseError =
    native(MLN_RUNTIME_EVENT_MASK_OFFLINE_REGION_RESPONSE_ERROR)
  public static let offlineRegionTileCountLimitExceeded =
    native(MLN_RUNTIME_EVENT_MASK_OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED)

  /// Every map-originated event type, which is what
  /// ``MapHandle/setEventMask(_:)`` reads.
  public static let allMapEvents =
    native(MLN_RUNTIME_EVENT_MASK_ALL_MAP_EVENTS)
  /// Every runtime-originated event type, which is what
  /// ``RuntimeHandle/setEventMask(_:)`` reads.
  public static let allRuntimeEvents =
    native(MLN_RUNTIME_EVENT_MASK_ALL_RUNTIME_EVENTS)
  /// Every event type this build of the binding names.
  public static let all = native(MLN_RUNTIME_EVENT_MASK_ALL)

  /// The mask ``RuntimeOptions`` starts a runtime with: the C API's own
  /// default, kept raw rather than replaced by ``all``, so a newer native
  /// library's default keeps selecting event types this build does not name.
  /// Those reach a host as unknown event and payload domains.
  public static var runtimeOptionsDefault: RuntimeEventMask {
    RuntimeEventMask(rawValue: mln_runtime_options_default().event_mask)
  }

  /// The mask ``MapOptions`` starts a map with, taken from the C API's own
  /// default the way ``runtimeOptionsDefault`` is.
  public static var mapOptionsDefault: RuntimeEventMask {
    RuntimeEventMask(rawValue: mln_map_options_default().event_mask)
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

/// Payload of a `.mapTileAction` event. The event message carries the source
/// ID.
public struct TileActionEvent: Equatable, Sendable {
  public let operation: TileOperation
  public let tileId: TileId

  init(native: NativeTileActionEvent) {
    operation = TileOperation.fromNative(native.operation)
    tileId = TileId(native: native.tileId)
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
  case tileAction(TileActionEvent)
  case offlineRegionStatus(OfflineRegionStatusEvent)
  case offlineRegionResponseError(OfflineRegionResponseErrorEvent)
  case offlineRegionTileCountLimit(OfflineRegionTileCountLimitEvent)
  case cameraTransitionFinished(CameraTransitionFinishedEvent)
  /// A payload kind this version of the binding does not name, carrying the
  /// payload's fixed byte window so a host forwards it unchanged.
  case unknown(type: UInt32, bytes: [UInt8])

  init(native: NativeRuntimeEventPayload) {
    switch native {
    case .none:
      self = .none
    case let .renderFrame(event):
      self = .renderFrame(RenderFrameEvent(native: event))
    case let .renderMap(event):
      self = .renderMap(RenderMapEvent(native: event))
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
    case let .cameraTransitionFinished(event):
      self =
        .cameraTransitionFinished(
          CameraTransitionFinishedEvent(native: event)
        )
    case let .unknown(type, bytes):
      self = .unknown(type: type, bytes: bytes)
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
  /// - Every other event type leaves it 0.
  public let code: Int32
  /// Text this event carries, empty when it carries none.
  ///
  /// It is the failure text of `.mapLoadingFailed`, `.mapRenderError`,
  /// `.mapStillImageFailed`, and `.offlineRegionResponseError`, the image ID of
  /// `.mapStyleImageMissing`, and the source ID of `.mapTileAction`.
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

/// One drained batch of runtime events.
///
/// This is a copy of an owned C event batch, so a host keeps events, their
/// messages, and their payloads for as long as it likes.
public struct RuntimeEventBatch: Equatable, Sendable {
  /// The drained events, in queue order.
  public let events: [RuntimeEvent]
}

public final class RuntimeHandle: @unchecked Sendable {
  private let handle: NativeHandleBox<NativeRuntimeHandle>
  private let eventWake: NativeWakeState

  public init(options: RuntimeOptions = RuntimeOptions()) throws {
    let eventWake = NativeWakeState()
    let wake = eventWake.makeDescriptor()
    do {
      let runtime = try mapNativeFailure {
        try options.nativeInput.withNativeOptions(
          eventWake: wake
        ) { nativeOptions in
          try NativeRuntime.create(nativeOptions)
        }
      }
      handle = try NativeHandleBox(
        typeName: "RuntimeHandle",
        handle: runtime
      )
      self.eventWake = eventWake
    } catch {
      eventWake.releaseRejectedDescriptor()
      throw error
    }
  }

  public var isClosed: Bool {
    handle.isClosed
  }

  public func close() throws {
    try mapNativeFailure {
      try handle.closeOnce { try NativeRuntime.release($0) }
    }
  }

  func closeBlockingForTests() throws {
    try close()
  }

  /// Waits until every command accepted before this call has committed.
  public func barrier() async throws {
    try await mapNativeFailure {
      let runtime = try requireLiveHandle()
      try await NativeCompletion.unit { completion in
        mln_runtime_barrier(runtime.raw, completion)
      }
    }
  }

  func requireLiveHandle() throws -> NativeRuntimeHandle {
    try handle.requireLive()
  }

  /// Drains this runtime's queued events, copying every event out of the owned
  /// native batch before the call returns.
  ///
  /// Events arrive in queue order, from this runtime and from every map it
  /// owns. One drain takes the whole queue.
  public func drainEvents() throws -> RuntimeEventBatch {
    try mapNativeFailure {
      let batch = try NativeRuntime.drainEvents(handle.requireLive())
      return RuntimeEventBatch(
        events: batch.events.map { RuntimeEvent(native: $0) }
      )
    }
  }

  /// Schedules the receiver when runtime events become ready to drain.
  ///
  /// The callback may coalesce and carries no event payload. It should schedule
  /// ``drainEvents()`` on the host execution context that consumes
  /// runtime events.
  public func setEventReadyHandler(
    _ handler: (@Sendable () -> Void)?
  ) {
    eventWake.setHandler(handler)
  }

  /// Selects which runtime-originated event types this runtime queues.
  ///
  public func setEventMask(_ mask: RuntimeEventMask) throws {
    try mapNativeFailure { try NativeRuntime.setEventMask(
      handle.requireLive(),
      mask: mask.rawValue
    ) }
  }

  public var eventMask: RuntimeEventMask {
    get throws {
      try mapNativeFailure {
        try RuntimeEventMask(rawValue: NativeRuntime
          .eventMask(handle.requireLive()))
      }
    }
  }

  @discardableResult
  public func setResourceTransform(_ callback: @escaping @Sendable (
    ResourceTransformRequest
  )
    -> String?) async throws -> CommandCompletion
  {
    let replacement = NativeResourceTransformState {
      callback(ResourceTransformRequest(native: $0))
    }
    let future = try replacement.withDescriptor { try startCallbackSet(
      $0,
      mln_runtime_set_resource_transform
    ) }
    return try await mapNativeFailure { try await future.value() }
  }

  @discardableResult
  public func clearResourceTransform() async throws -> CommandCompletion {
    try await submitCallbackClear(mln_runtime_clear_resource_transform)
  }

  @discardableResult
  public func setHttpHeaderTransform(_ callback: @escaping @Sendable (
    HttpHeaderTransformRequest
  )
    -> [HttpHeader]) async throws -> CommandCompletion
  {
    let replacement = NativeHttpHeaderTransformState(callback)
    let future = try replacement.withDescriptor { try startCallbackSet(
      $0,
      mln_runtime_set_http_header_transform
    ) }
    return try await mapNativeFailure { try await future.value() }
  }

  @discardableResult
  public func clearHttpHeaderTransform() async throws -> CommandCompletion {
    try await submitCallbackClear(mln_runtime_clear_http_header_transform)
  }

  @discardableResult
  public func setResourceProvider(_ callback: @escaping @Sendable (
    ResourceRequest,
    ResourceRequestHandle
  ) -> ResourceProviderDecision) async throws -> CommandCompletion {
    let replacement = NativeResourceProviderState { request, handle in
      switch callback(
        ResourceRequest(native: request),
        ResourceRequestHandle(state: handle)
      ) {
      case .passThrough: return 0
      case .handle: return 1
      }
    }
    let future = try replacement.withDescriptor { try startCallbackSet(
      $0,
      mln_runtime_set_resource_provider
    ) }
    return try await mapNativeFailure { try await future.value() }
  }

  @discardableResult
  public func clearResourceProvider() async throws -> CommandCompletion {
    try await submitCallbackClear(mln_runtime_clear_resource_provider)
  }

  private func startCallbackSet<Descriptor>(
    _ descriptor: UnsafePointer<Descriptor>,
    _ submit: (
      mln_runtime,
      UnsafePointer<Descriptor>,
      UnsafePointer<mln_completion>
    ) -> mln_status
  ) throws -> NativeFuture<CommandCompletion> {
    try mapNativeFailure {
      let runtime = try handle.requireLive()
      return try NativeCompletion.startCommand { completion in
        submit(runtime.raw, descriptor, completion)
      }
    }
  }

  private func submitCallbackClear(_ submit: (
    mln_runtime,
    UnsafePointer<mln_completion>
  ) -> mln_status) async throws -> CommandCompletion {
    try await mapNativeFailure {
      let runtime = try handle.requireLive()
      return try await NativeCompletion.command { completion in
        submit(runtime.raw, completion)
      }
    }
  }
}

public enum CommandDisposition: Sendable, Hashable {
  case committed
  case superseded
  case failed
  case cancelled
  case unknown(UInt32)

  static func fromNative(_ raw: UInt32) -> Self {
    switch raw {
    case MLN_COMMAND_DISPOSITION_COMMITTED.rawValue: .committed
    case MLN_COMMAND_DISPOSITION_SUPERSEDED.rawValue: .superseded
    case MLN_COMMAND_DISPOSITION_FAILED.rawValue: .failed
    case MLN_COMMAND_DISPOSITION_CANCELLED.rawValue: .cancelled
    default: .unknown(raw)
    }
  }
}

public struct CommandCompletion: Sendable, Hashable {
  public let disposition: CommandDisposition
  public let generation: UInt64
  public let rawStatus: Int32
  public let diagnostic: String
}
