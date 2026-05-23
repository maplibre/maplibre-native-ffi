import CMaplibreNativeC

public struct NativeRuntimeOptionsInput: Sendable, Equatable {
  public var assetPath: String?
  public var cachePath: String?
  public var maximumCacheSize: UInt64?

  public init(assetPath: String? = nil, cachePath: String? = nil, maximumCacheSize: UInt64? = nil) {
    self.assetPath = assetPath
    self.cachePath = cachePath
    self.maximumCacheSize = maximumCacheSize
  }

  public func withNativeOptions<Result>(
    _ body: (UnsafePointer<mln_runtime_options>) throws -> Result
  ) throws -> Result {
    try NativeString.withOptionalCString(assetPath) { assetPath in
      try NativeString.withOptionalCString(cachePath) { cachePath in
        var options = CAPI.runtimeOptionsDefault()
        options.asset_path = assetPath
        options.cache_path = cachePath
        if let maximumCacheSize {
          options.flags |= MLN_RUNTIME_OPTION_MAXIMUM_CACHE_SIZE.rawValue
          options.maximum_cache_size = maximumCacheSize
        }
        return try withUnsafePointer(to: &options, body)
      }
    }
  }
}

public struct NativeRenderingStats: Equatable, Sendable {
  public let encodingTime: Double
  public let renderingTime: Double
  public let frameCount: Int64
  public let drawCallCount: Int64
  public let totalDrawCallCount: Int64

  public init(_ raw: mln_rendering_stats) {
    encodingTime = raw.encoding_time
    renderingTime = raw.rendering_time
    frameCount = raw.frame_count
    drawCallCount = raw.draw_call_count
    totalDrawCallCount = raw.total_draw_call_count
  }
}

public struct NativeRenderFrameEvent: Equatable, Sendable {
  public let mode: UInt32
  public let needsRepaint: Bool
  public let placementChanged: Bool
  public let stats: NativeRenderingStats

  public init(_ raw: mln_runtime_event_render_frame) {
    mode = raw.mode
    needsRepaint = raw.needs_repaint
    placementChanged = raw.placement_changed
    stats = NativeRenderingStats(raw.stats)
  }
}

public struct NativeRenderMapEvent: Equatable, Sendable {
  public let mode: UInt32

  public init(_ raw: mln_runtime_event_render_map) {
    mode = raw.mode
  }
}

public struct NativeTileId: Equatable, Sendable {
  public let overscaledZ: UInt32
  public let wrap: Int32
  public let canonicalZ: UInt32
  public let canonicalX: UInt32
  public let canonicalY: UInt32

  public init(_ raw: mln_tile_id) {
    overscaledZ = raw.overscaled_z
    wrap = raw.wrap
    canonicalZ = raw.canonical_z
    canonicalX = raw.canonical_x
    canonicalY = raw.canonical_y
  }
}

public struct NativeTileActionEvent: Equatable, Sendable {
  public let operation: UInt32
  public let tileId: NativeTileId
  public let sourceId: String

  public init(_ raw: mln_runtime_event_tile_action) throws {
    operation = raw.operation
    tileId = NativeTileId(raw.tile_id)
    sourceId = try NativeString.copyUTF8(data: raw.source_id, size: raw.source_id_size)
  }
}

public struct NativeOfflineRegionStatus: Equatable, Sendable {
  public let downloadState: UInt32
  public let completedResourceCount: UInt64
  public let completedResourceSize: UInt64
  public let completedTileCount: UInt64
  public let requiredTileCount: UInt64
  public let completedTileSize: UInt64
  public let requiredResourceCount: UInt64
  public let requiredResourceCountIsPrecise: Bool
  public let complete: Bool

  public init(_ raw: mln_offline_region_status) {
    downloadState = raw.download_state
    completedResourceCount = raw.completed_resource_count
    completedResourceSize = raw.completed_resource_size
    completedTileCount = raw.completed_tile_count
    requiredTileCount = raw.required_tile_count
    completedTileSize = raw.completed_tile_size
    requiredResourceCount = raw.required_resource_count
    requiredResourceCountIsPrecise = raw.required_resource_count_is_precise
    complete = raw.complete
  }
}

public struct NativeOfflineRegionStatusEvent: Equatable, Sendable {
  public let regionId: Int64
  public let status: NativeOfflineRegionStatus

  public init(_ raw: mln_runtime_event_offline_region_status) {
    regionId = raw.region_id
    status = NativeOfflineRegionStatus(raw.status)
  }
}

public struct NativeOfflineRegionResponseErrorEvent: Equatable, Sendable {
  public let regionId: Int64
  public let reason: UInt32

  public init(_ raw: mln_runtime_event_offline_region_response_error) {
    regionId = raw.region_id
    reason = raw.reason
  }
}

public struct NativeOfflineRegionTileCountLimitEvent: Equatable, Sendable {
  public let regionId: Int64
  public let limit: UInt64

  public init(_ raw: mln_runtime_event_offline_region_tile_count_limit) {
    regionId = raw.region_id
    limit = raw.limit
  }
}

public struct NativeOfflineOperationCompletedEvent: Equatable, Sendable {
  public let operationId: UInt64
  public let operationKind: UInt32
  public let resultKind: UInt32
  public let resultStatus: Int32
  public let found: Bool

  public init(_ raw: mln_runtime_event_offline_operation_completed) {
    operationId = raw.operation_id
    operationKind = raw.operation_kind
    resultKind = raw.result_kind
    resultStatus = raw.result_status
    found = raw.found
  }
}

public enum NativeRuntimeEventPayload: Equatable, Sendable {
  case none
  case renderFrame(NativeRenderFrameEvent)
  case renderMap(NativeRenderMapEvent)
  case styleImageMissing(String)
  case tileAction(NativeTileActionEvent)
  case offlineRegionStatus(NativeOfflineRegionStatusEvent)
  case offlineRegionResponseError(NativeOfflineRegionResponseErrorEvent)
  case offlineRegionTileCountLimit(NativeOfflineRegionTileCountLimitEvent)
  case offlineOperationCompleted(NativeOfflineOperationCompletedEvent)
  case unknown(type: UInt32, byteCount: Int)
}

public struct NativeRuntimeEvent: Equatable, Sendable {
  public let type: UInt32
  public let sourceType: UInt32
  public let sourceAddress: UInt
  public let code: Int32
  public let message: String
  public let payload: NativeRuntimeEventPayload

  public init(_ raw: mln_runtime_event) throws {
    type = raw.type
    sourceType = raw.source_type
    sourceAddress = UInt(bitPattern: raw.source)
    code = raw.code
    message = try NativeString.copyUTF8(data: raw.message, size: raw.message_size)
    payload = try Self.copyPayload(raw)
  }

  private static func copyPayload(_ raw: mln_runtime_event) throws -> NativeRuntimeEventPayload {
    switch raw.payload_type {
    case MLN_RUNTIME_EVENT_PAYLOAD_NONE.rawValue:
      return .none
    case MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME.rawValue:
      return try withPayload(raw, as: mln_runtime_event_render_frame.self) { .renderFrame(NativeRenderFrameEvent($0)) }
    case MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP.rawValue:
      return try withPayload(raw, as: mln_runtime_event_render_map.self) { .renderMap(NativeRenderMapEvent($0)) }
    case MLN_RUNTIME_EVENT_PAYLOAD_STYLE_IMAGE_MISSING.rawValue:
      return try withPayload(raw, as: mln_runtime_event_style_image_missing.self) {
        .styleImageMissing(try NativeString.copyUTF8(data: $0.image_id, size: $0.image_id_size))
      }
    case MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION.rawValue:
      return try withPayload(raw, as: mln_runtime_event_tile_action.self) { .tileAction(try NativeTileActionEvent($0)) }
    case MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS.rawValue:
      return try withPayload(raw, as: mln_runtime_event_offline_region_status.self) {
        .offlineRegionStatus(NativeOfflineRegionStatusEvent($0))
      }
    case MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR.rawValue:
      return try withPayload(raw, as: mln_runtime_event_offline_region_response_error.self) {
        .offlineRegionResponseError(NativeOfflineRegionResponseErrorEvent($0))
      }
    case MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT.rawValue:
      return try withPayload(raw, as: mln_runtime_event_offline_region_tile_count_limit.self) {
        .offlineRegionTileCountLimit(NativeOfflineRegionTileCountLimitEvent($0))
      }
    case MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_OPERATION_COMPLETED.rawValue:
      return try withPayload(raw, as: mln_runtime_event_offline_operation_completed.self) {
        .offlineOperationCompleted(NativeOfflineOperationCompletedEvent($0))
      }
    default:
      return .unknown(type: raw.payload_type, byteCount: raw.payload_size)
    }
  }

  private static func withPayload<Payload, Result>(
    _ raw: mln_runtime_event,
    as type: Payload.Type,
    _ body: (Payload) throws -> Result
  ) throws -> Result {
    guard raw.payload_size >= MemoryLayout<Payload>.size, let payload = raw.payload else {
      throw NativeStatusFailure(rawStatus: 0, diagnostic: "runtime event payload is missing or too small")
    }
    return try body(payload.assumingMemoryBound(to: Payload.self).pointee)
  }
}
