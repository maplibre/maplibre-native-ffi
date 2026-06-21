internal import CMaplibreNativeC

struct NativeRuntimeOptionsInput: Equatable {
  var assetPath: String?
  var cachePath: String?
  var maximumCacheSize: UInt64?

  init(
    assetPath: String? = nil,
    cachePath: String? = nil,
    maximumCacheSize: UInt64? = nil
  ) {
    self.assetPath = assetPath
    self.cachePath = cachePath
    self.maximumCacheSize = maximumCacheSize
  }

  func withNativeOptions<Result>(
    _ body: (UnsafePointer<mln_runtime_options>) throws -> Result
  ) throws -> Result {
    try NativeString.withOptionalCString(assetPath) { assetPath in
      try NativeString.withOptionalCString(cachePath) { cachePath in
        var options = mln_runtime_options_default()
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

struct NativeRenderingStats: Equatable {
  let encodingTime: Double
  let renderingTime: Double
  let frameCount: Int64
  let drawCallCount: Int64
  let totalDrawCallCount: Int64

  init(_ raw: mln_rendering_stats) {
    encodingTime = raw.encoding_time
    renderingTime = raw.rendering_time
    frameCount = raw.frame_count
    drawCallCount = raw.draw_call_count
    totalDrawCallCount = raw.total_draw_call_count
  }
}

struct NativeRenderFrameEvent: Equatable {
  let mode: UInt32
  let needsRepaint: Bool
  let placementChanged: Bool
  let stats: NativeRenderingStats

  init(_ raw: mln_runtime_event_render_frame) {
    mode = raw.mode
    needsRepaint = raw.needs_repaint
    placementChanged = raw.placement_changed
    stats = NativeRenderingStats(raw.stats)
  }
}

struct NativeRenderMapEvent: Equatable {
  let mode: UInt32

  init(_ raw: mln_runtime_event_render_map) {
    mode = raw.mode
  }
}

struct NativeTileId: Equatable {
  let overscaledZ: UInt32
  let wrap: Int32
  let canonicalZ: UInt32
  let canonicalX: UInt32
  let canonicalY: UInt32

  init(_ raw: mln_tile_id) {
    overscaledZ = raw.overscaled_z
    wrap = raw.wrap
    canonicalZ = raw.canonical_z
    canonicalX = raw.canonical_x
    canonicalY = raw.canonical_y
  }
}

struct NativeTileActionEvent: Equatable {
  let operation: UInt32
  let tileId: NativeTileId
  let sourceId: String

  init(_ raw: mln_runtime_event_tile_action) throws {
    operation = raw.operation
    tileId = NativeTileId(raw.tile_id)
    sourceId = try NativeString.copyUTF8(
      data: raw.source_id,
      size: raw.source_id_size
    )
  }
}

struct NativeOfflineRegionStatus: Equatable {
  let downloadState: UInt32
  let completedResourceCount: UInt64
  let completedResourceSize: UInt64
  let completedTileCount: UInt64
  let requiredTileCount: UInt64
  let completedTileSize: UInt64
  let requiredResourceCount: UInt64
  let requiredResourceCountIsPrecise: Bool
  let complete: Bool

  init(_ raw: mln_offline_region_status) {
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

struct NativeOfflineRegionStatusEvent: Equatable {
  let regionId: Int64
  let status: NativeOfflineRegionStatus

  init(_ raw: mln_runtime_event_offline_region_status) {
    regionId = raw.region_id
    status = NativeOfflineRegionStatus(raw.status)
  }
}

struct NativeOfflineRegionResponseErrorEvent: Equatable {
  let regionId: Int64
  let reason: UInt32

  init(_ raw: mln_runtime_event_offline_region_response_error) {
    regionId = raw.region_id
    reason = raw.reason
  }
}

struct NativeOfflineRegionTileCountLimitEvent: Equatable {
  let regionId: Int64
  let limit: UInt64

  init(_ raw: mln_runtime_event_offline_region_tile_count_limit) {
    regionId = raw.region_id
    limit = raw.limit
  }
}

struct NativeOfflineOperationCompletedEvent: Equatable {
  let operationId: UInt64
  let operationKind: UInt32
  let resultKind: UInt32
  let resultStatus: Int32
  let found: Bool

  init(_ raw: mln_runtime_event_offline_operation_completed) {
    operationId = raw.operation_id
    operationKind = raw.operation_kind
    resultKind = raw.result_kind
    resultStatus = raw.result_status
    found = raw.found
  }
}

enum NativeRuntimeEventPayload: Equatable {
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

struct NativeRuntimeEvent: Equatable {
  let type: UInt32
  let sourceType: UInt32
  let sourceAddress: UInt
  let code: Int32
  let message: String
  let payload: NativeRuntimeEventPayload

  init(_ raw: mln_runtime_event) throws {
    type = raw.type
    sourceType = raw.source_type
    sourceAddress = UInt(bitPattern: raw.source)
    code = raw.code
    message = try NativeString.copyUTF8(
      data: raw.message,
      size: raw.message_size
    )
    payload = try Self.copyPayload(raw)
  }

  private static func copyPayload(_ raw: mln_runtime_event) throws
    -> NativeRuntimeEventPayload
  {
    switch raw.payload_type {
    case MLN_RUNTIME_EVENT_PAYLOAD_NONE.rawValue:
      return .none
    case MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME.rawValue:
      return try withPayload(raw, as: mln_runtime_event_render_frame.self) {
        .renderFrame(NativeRenderFrameEvent($0))
      }
    case MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP.rawValue:
      return try withPayload(raw, as: mln_runtime_event_render_map.self) {
        .renderMap(NativeRenderMapEvent($0))
      }
    case MLN_RUNTIME_EVENT_PAYLOAD_STYLE_IMAGE_MISSING.rawValue:
      return try withPayload(
        raw,
        as: mln_runtime_event_style_image_missing.self
      ) {
        try .styleImageMissing(NativeString.copyUTF8(
          data: $0.image_id,
          size: $0.image_id_size
        ))
      }
    case MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION.rawValue:
      return try withPayload(raw, as: mln_runtime_event_tile_action.self) {
        try .tileAction(NativeTileActionEvent($0))
      }
    case MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS.rawValue:
      return try withPayload(
        raw,
        as: mln_runtime_event_offline_region_status.self
      ) {
        .offlineRegionStatus(NativeOfflineRegionStatusEvent($0))
      }
    case MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR.rawValue:
      return try withPayload(
        raw,
        as: mln_runtime_event_offline_region_response_error.self
      ) {
        .offlineRegionResponseError(NativeOfflineRegionResponseErrorEvent($0))
      }
    case MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT.rawValue:
      return try withPayload(
        raw,
        as: mln_runtime_event_offline_region_tile_count_limit.self
      ) {
        .offlineRegionTileCountLimit(NativeOfflineRegionTileCountLimitEvent($0))
      }
    case MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_OPERATION_COMPLETED.rawValue:
      return try withPayload(
        raw,
        as: mln_runtime_event_offline_operation_completed.self
      ) {
        .offlineOperationCompleted(NativeOfflineOperationCompletedEvent($0))
      }
    default:
      return .unknown(type: raw.payload_type, byteCount: raw.payload_size)
    }
  }

  private static func withPayload<Payload, Result>(
    _ raw: mln_runtime_event,
    as _: Payload.Type,
    _ body: (Payload) throws -> Result
  ) throws -> Result {
    guard raw.payload_size >= MemoryLayout<Payload>.size,
          let payload = raw.payload
    else {
      throw NativeStatusFailure(
        rawStatus: 0,
        diagnostic: "runtime event payload is missing or too small"
      )
    }
    return try body(payload.assumingMemoryBound(to: Payload.self).pointee)
  }
}
