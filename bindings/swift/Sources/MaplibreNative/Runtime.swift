internal import CMaplibreNativeC
import Foundation

public struct RuntimeOptions: Equatable, Sendable {
  public var assetPath: String?
  public var cachePath: String?
  public var maximumCacheSize: UInt64?

  public init(
    assetPath: String? = nil,
    cachePath: String? = nil,
    maximumCacheSize: UInt64? = nil
  ) {
    self.assetPath = assetPath
    self.cachePath = cachePath
    self.maximumCacheSize = maximumCacheSize
  }

  var nativeInput: NativeRuntimeOptionsInput {
    NativeRuntimeOptionsInput(
      assetPath: assetPath,
      cachePath: cachePath,
      maximumCacheSize: maximumCacheSize
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
    default: .unknown(rawValue)
    }
  }
}

public enum RuntimeEventSource: Equatable, Sendable {
  case runtime
  case map(NativePointer)
  case unknown(sourceType: UInt32, source: NativePointer)

  static func fromNative(sourceType: UInt32, sourceAddress: UInt) -> Self {
    let source = NativePointer(bitPattern: sourceAddress)
    switch sourceType {
    case 0: return .runtime
    case 1: return .map(source)
    default: return .unknown(sourceType: sourceType, source: source)
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
    case let .unknown(type, byteCount):
      self = .unknown(type: type, byteCount: byteCount)
    }
  }
}

public struct RuntimeEvent: Equatable, Sendable {
  public let type: RuntimeEventType
  public let source: RuntimeEventSource
  public let code: Int32
  public let message: String
  public let payload: RuntimeEventPayload

  init(native: NativeRuntimeEvent) {
    type = RuntimeEventType.fromNative(native.type)
    source = RuntimeEventSource.fromNative(
      sourceType: native.sourceType,
      sourceAddress: native.sourceAddress
    )
    code = native.code
    message = native.message
    payload = RuntimeEventPayload(native: native.payload)
  }
}

public final class RuntimeHandle {
  private let handle: NativeHandleBox
  private var resourceTransform: NativeResourceTransformState?
  private var resourceProvider: NativeResourceProviderState?

  public init(options: RuntimeOptions = RuntimeOptions()) throws {
    let pointer = try mapNativeFailure {
      try options.nativeInput.withNativeOptions { nativeOptions in
        try NativeRuntime.create(nativeOptions)
      }
    }
    handle = try NativeHandleBox(typeName: "RuntimeHandle", pointer: pointer)
  }

  public var isClosed: Bool {
    handle.isClosed
  }

  public func close() throws {
    try handle.closeOnce { pointer in
      try checkStatus(mln_runtime_destroy(pointer))
    }
    resourceTransform = nil
    resourceProvider = nil
  }

  func requireLivePointer() throws -> OpaquePointer {
    try handle.requireLive()
  }

  public func runOnce() throws {
    try mapNativeFailure {
      try checkStatus(mln_runtime_run_once(handle.requireLive()))
    }
  }

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
          handle.requireLive(),
          descriptor
        ))
      }
    }
    resourceTransform = replacement
  }

  public func clearResourceTransform() throws {
    try mapNativeFailure {
      try checkStatus(mln_runtime_clear_resource_transform(handle
          .requireLive()))
    }
    resourceTransform = nil
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
          handle.requireLive(),
          descriptor
        ))
      }
    }
    resourceProvider = replacement
  }
}
