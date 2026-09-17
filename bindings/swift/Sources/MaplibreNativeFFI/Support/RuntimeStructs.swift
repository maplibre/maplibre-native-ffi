internal import CMaplibreNativeC

struct NativeRuntimeOptionsInput: Equatable {
  var assetPath: String?
  var cachePath: String?
  var eventMask: UInt64

  init(
    assetPath: String? = nil,
    cachePath: String? = nil,
    eventMask: UInt64
  ) {
    self.assetPath = assetPath
    self.cachePath = cachePath
    self.eventMask = eventMask
  }

  func withNativeOptions<Result>(
    eventWake: mln_wake,
    _ body: (UnsafePointer<mln_runtime_options>) throws -> Result
  ) throws -> Result {
    try NativeString.withOptionalCString(assetPath) { assetPath in
      try NativeString.withOptionalCString(cachePath) { cachePath in
        var options = mln_runtime_options_default()
        options.asset_path = assetPath
        options.cache_path = cachePath
        options.event_mask = eventMask
        options.event_wake = eventWake
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

struct NativeCameraTransitionFinishedEvent: Equatable {
  let transitionId: UInt64

  init(_ raw: mln_runtime_event_camera_transition_finished) {
    transitionId = raw.transition_id
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

  init(_ raw: mln_runtime_event_tile_action) {
    operation = raw.operation
    tileId = NativeTileId(raw.tile_id)
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

enum NativeRuntimeEventPayload: Equatable {
  case none
  case renderFrame(NativeRenderFrameEvent)
  case renderMap(NativeRenderMapEvent)
  case tileAction(NativeTileActionEvent)
  case offlineRegionStatus(NativeOfflineRegionStatusEvent)
  case offlineRegionResponseError(NativeOfflineRegionResponseErrorEvent)
  case offlineRegionTileCountLimit(NativeOfflineRegionTileCountLimitEvent)
  case cameraTransitionFinished(NativeCameraTransitionFinishedEvent)
  /// A payload kind this binding does not name, carrying the payload union's
  /// fixed byte window copied out of the batch.
  case unknown(type: UInt32, bytes: [UInt8])
}

struct NativeRuntimeEvent: Equatable {
  let type: UInt32
  let sourceType: UInt32
  let sourceId: UInt64
  let code: Int32
  let message: String
  let payload: NativeRuntimeEventPayload
}

/// One drained batch of runtime events copied from an owned native batch.
struct NativeRuntimeEventBatch: Equatable {
  let events: [NativeRuntimeEvent]

  init(copying raw: mln_runtime_event_batch_view) throws {
    events = try Self.copyEvents(raw)
  }

  private static func copyEvents(_ raw: mln_runtime_event_batch_view) throws
    -> [NativeRuntimeEvent]
  {
    guard raw.event_count > 0 else { return [] }
    guard let events = raw.events else {
      throw NativeStatusFailure
        .swiftNativeError("runtime event batch has no event array")
    }
    // The batch reports the record stride, which a later C API version widens
    // by adding a payload member. Stepping by this binding's own event size
    // would misread every event behind the first one.
    let stride = Int(raw.event_size)
    let base = UnsafeRawPointer(events)
    return try (0 ..< raw.event_count).map { index in
      try copyEvent(
        at: base.advanced(by: index * stride),
        stride: stride,
        messages: raw.messages
      )
    }
  }

  private static func copyEvent(
    at record: UnsafeRawPointer,
    stride: Int,
    messages: UnsafePointer<CChar>?
  ) throws -> NativeRuntimeEvent {
    let raw = record.load(as: mln_runtime_event.self)
    return try NativeRuntimeEvent(
      type: raw.type,
      sourceType: raw.source_type,
      sourceId: raw.source,
      code: raw.code,
      message: copyMessage(raw, messages: messages),
      payload: copyPayload(raw, at: record, stride: stride)
    )
  }

  private static func copyMessage(
    _ raw: mln_runtime_event,
    messages: UnsafePointer<CChar>?
  ) throws -> String {
    guard raw.message_size > 0 else { return "" }
    guard let messages else {
      throw NativeStatusFailure
        .swiftNativeError("runtime event batch has no message arena")
    }
    return try NativeString.copyUTF8(
      data: messages.advanced(by: Int(raw.message_offset)),
      size: Int(raw.message_size)
    )
  }

  private static func copyPayload(
    _ raw: mln_runtime_event,
    at record: UnsafeRawPointer,
    stride: Int
  ) throws -> NativeRuntimeEventPayload {
    switch raw.payload_type {
    case MLN_RUNTIME_EVENT_PAYLOAD_NONE.rawValue:
      return .none
    case MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME.rawValue:
      return .renderFrame(NativeRenderFrameEvent(raw.payload.render_frame))
    case MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP.rawValue:
      return .renderMap(NativeRenderMapEvent(raw.payload.render_map))
    case MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION.rawValue:
      return .tileAction(NativeTileActionEvent(raw.payload.tile_action))
    case MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS.rawValue:
      return .offlineRegionStatus(
        NativeOfflineRegionStatusEvent(raw.payload.offline_region_status)
      )
    case MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR.rawValue:
      return .offlineRegionResponseError(
        NativeOfflineRegionResponseErrorEvent(
          raw.payload.offline_region_response_error
        )
      )
    case MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT.rawValue:
      return .offlineRegionTileCountLimit(
        NativeOfflineRegionTileCountLimitEvent(
          raw.payload.offline_region_tile_count_limit
        )
      )
    case MLN_RUNTIME_EVENT_PAYLOAD_CAMERA_TRANSITION_FINISHED.rawValue:
      return .cameraTransitionFinished(
        NativeCameraTransitionFinishedEvent(
          raw.payload.camera_transition_finished
        )
      )
    default:
      return try .unknown(
        type: raw.payload_type,
        bytes: copyUnknownPayload(at: record, stride: stride)
      )
    }
  }

  /// Copies the payload window of a payload kind this binding does not name, so
  /// a host forwards it unchanged. The window is the record stride minus the
  /// payload's offset, an offset the C API keeps across versions.
  private static func copyUnknownPayload(
    at record: UnsafeRawPointer,
    stride: Int
  ) throws -> [UInt8] {
    guard let offset = MemoryLayout<mln_runtime_event>.offset(of: \.payload)
    else {
      throw NativeStatusFailure
        .swiftNativeError("runtime event payload has no fixed offset")
    }
    return Array(UnsafeRawBufferPointer(
      start: record.advanced(by: offset),
      count: max(stride - offset, 0)
    ))
  }
}
