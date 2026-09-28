import CMaplibreNativeC
import Foundation
@testable import MaplibreNativeFFI
import Testing

private func makeRuntime() throws -> RuntimeHandle {
  try Maplibre.runtimeCreate(options: RuntimeOptions(cachePath: ":memory:"))
}

private func makeMap(_ runtime: RuntimeHandle) async throws -> MapHandle {
  try await runtime.mapCreate(options: MapOptions(initialExtent: LogicalExtent(
    width: 64,
    height: 64,
    scaleFactor: 1
  )))
}

// MARK: - Decoding a batch

/// The batch reports the record stride, and a later C API version
/// widens it by adding a payload member, so a decoder that stepped by its own
/// event size would misread every event behind the first one.
@Test func batchDecodeStepsByTheStrideTheBatchReports() throws {
  let arena = packMessageArena(["first", "second", ""])
  let events = [
    rawRuntimeEvent(
      type: 4,
      source: 0x11,
      messageOffset: arena.offsets[0],
      messageSize: 5
    ),
    rawRuntimeEvent(
      type: 8,
      source: 0x22,
      code: -3,
      messageOffset: arena.offsets[1],
      messageSize: 6
    ),
    rawRuntimeEvent(type: 9, source: 0x33),
  ]
  let batch = try withSynthesizedEventBatch(
    events: events,
    stride: MemoryLayout<mln_runtime_event>.size + 16,
    messages: arena.bytes
  ) { synthesized in
    try RuntimeEventBatchView(raw: synthesized.batch)
  }

  #expect(batch.events.map { $0.type.rawValue } == [4, 8, 9])
  #expect(batch.events.map(\.source) == [0x11, 0x22, 0x33])
  #expect(batch.events.map(\.message) == ["first", "second", ""])
  #expect(batch.events.map(\.code) == [0, -3, 0])
}

/// An event type, source kind, and payload kind this version does not
/// name reach the host with their raw values, and the payload arrives as the
/// stride's own byte window, copied out of storage the next drain reuses.
@Test func unknownEventSourceAndPayloadKindsSurviveAsRawValues() throws {
  let stride = MemoryLayout<mln_runtime_event>.size + 8
  let payloadOffset = try #require(
    MemoryLayout<mln_runtime_event>.offset(of: \.payload)
  )
  let window = (0 ..< stride - payloadOffset).map { UInt8($0 & 0xFF) }
  let arena = packMessageArena(["opaque"])
  let event = rawRuntimeEvent(
    type: 0x4242,
    sourceType: 9,
    source: 0x77,
    code: -12,
    payloadType: 0xBEEF,
    messageOffset: arena.offsets[0],
    messageSize: 6
  )

  let batch = try withSynthesizedEventBatch(
    events: [event],
    stride: stride,
    messages: arena.bytes,
    payloadWindows: [0: window]
  ) { synthesized in
    let batch = try RuntimeEventBatchView(raw: synthesized.batch)
    synthesized.records.copyBytes(
      from: repeatElement(UInt8(0xFF), count: synthesized.records.count)
    )
    synthesized.messages.copyBytes(
      from: repeatElement(UInt8(0xFF), count: synthesized.messages.count)
    )
    return batch
  }

  let decoded = try #require(batch.events.first)
  #expect(decoded.type.rawValue == 0x4242)
  #expect(decoded.sourceType.rawValue == 9 && decoded.source == 0x77)
  #expect(decoded.code == -12)
  #expect(decoded.message == "opaque")
  #expect(decoded.payload == .unknown(0xBEEF, Data(window)))
}

/// A map-sourced event names its map by the id the C API delivered,
/// whether or not this process still holds a ``MapHandle`` for it, so a host
/// keeps the identity it needs to route or forward the event.
@Test func aMapSourcedEventKeepsAnIdNoLiveMapHandleClaims() async throws {
  let runtime = try makeRuntime()
  defer { try? runtime.closeBlockingForTests() }
  let map = try await makeMap(runtime)
  defer { try? map.closeBlockingForTests() }

  let batch = try withSynthesizedEventBatch(
    events: [rawRuntimeEvent(type: 4, source: 0xDEAD_BEEF)]
  ) { synthesized in
    try RuntimeEventBatchView(raw: synthesized.batch)
  }

  let decoded = try #require(batch.events.first)
  #expect(decoded.sourceType == .map && decoded.source == 0xDEAD_BEEF)
  #expect(decoded.sourceType != .map || decoded.source != map.id)
}

/// Every typed payload reads its own union member, so a member's bytes cannot
/// be attributed to the wrong payload kind.
@Test func typedPayloadsDecodeTheUnionMemberTheirKindNames() throws {
  var frame = rawRuntimeEvent(
    type: 14,
    payloadType: MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME.rawValue
  )
  frame.payload.render_frame.mode = 1
  frame.payload.render_frame.needs_repaint = true
  frame.payload.render_frame.stats.frame_count = 21
  frame.payload.render_frame.stats.encoding_time = 0.5

  var tile = rawRuntimeEvent(
    type: 18,
    payloadType: MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION.rawValue
  )
  tile.payload.tile_action.operation = 5
  tile.payload.tile_action.tile_id.canonical_x = 7
  tile.payload.tile_action.tile_id.wrap = -1

  var transition = rawRuntimeEvent(
    type: MLN_RUNTIME_EVENT_MAP_CAMERA_TRANSITION_FINISHED.rawValue,
    payloadType: MLN_RUNTIME_EVENT_PAYLOAD_CAMERA_TRANSITION_FINISHED.rawValue
  )
  transition.payload.camera_transition_finished.transition_id = 99

  let batch = try withSynthesizedEventBatch(
    events: [frame, tile, transition]
  ) { synthesized in
    try RuntimeEventBatchView(raw: synthesized.batch)
  }
  let decoded = batch.events
  let payloads = decoded.map(\.payload)
  #expect(decoded[2].type == .mapCameraTransitionFinished)

  guard case let .renderFrame(decodedFrame) = payloads[0] else {
    Issue.record("expected a render frame payload, got \(payloads[0])")
    return
  }
  #expect(decodedFrame.mode == .full)
  #expect(decodedFrame.needsRepaint)
  #expect(!decodedFrame.placementChanged)
  #expect(decodedFrame.stats.frameCount == 21)
  #expect(decodedFrame.stats.encodingTime == 0.5)

  guard case let .tileAction(decodedTile) = payloads[1] else {
    Issue.record("expected a tile action payload, got \(payloads[1])")
    return
  }
  #expect(decodedTile.operation == .endParse)
  #expect(decodedTile.tileId.canonicalX == 7)
  #expect(decodedTile.tileId.wrap == -1)

  guard case let .cameraTransitionFinished(finished) = payloads[2] else {
    Issue.record("expected a transition payload, got \(payloads[2])")
    return
  }
  #expect(finished.transitionId == 99)
}

// MARK: - The mask type

/// The default parameter takes the C API's own default,
/// bits this build does not name included, and a mask the host writes reaches
/// the struct as it stands.
@Test func mapAndRuntimeOptionsAlwaysEncodeAnEventMask() throws {
  let mapMask: RuntimeEventMask = [.mapStyleLoaded, .mapIdle]
  try withUnsafePointer(to: MapOptions(
    initialExtent: LogicalExtent(width: 8, height: 8, scaleFactor: 1),
    eventMask: mapMask
  ).nativeValue()) { native in
    #expect(native.pointee.event_mask == mapMask.rawValue)
  }
  try withUnsafePointer(to: MapOptions(
    initialExtent: LogicalExtent(width: 8, height: 8, scaleFactor: 1),
    eventMask: []
  ).nativeValue()) { native in
    #expect(native.pointee.event_mask == 0)
  }

  let arena = NativeInputArena()
  let native = try RuntimeOptions(eventMask: .allRuntimeEvents)
    .nativeValue(arena: arena)
  #expect(native.event_mask == RuntimeEventMask.allRuntimeEvents.rawValue)
  #expect(native.event_wake.callback == nil)
}

// MARK: - Draining a live runtime

/// One drain reports every event a style load produced, from the map
/// that produced them.
@Test func oneDrainReportsEveryEventAStyleLoadProduced() async throws {
  let runtime = try makeRuntime()
  defer { try? runtime.closeBlockingForTests() }
  let map = try await makeMap(runtime)
  defer { try? map.closeBlockingForTests() }
  _ = try runtime.drainEventCopies()

  try await map.setStyleJson(json: emptyStyleJSON)
  try await runtime.barrier()
  let batch = try runtime.drainEventCopies()

  #expect(batch.count > 1)
  #expect(batch.map(\.type).contains(.mapStyleLoaded))
  #expect(batch.allSatisfy { $0.sourceType == .map && $0.source == map.id })
}

/// An event copied out of a batch keeps its message and
/// payload after the drain that ends the batch's window.
@Test func anEventTakenOutOfABatchOutlivesTheNextDrain() async throws {
  let runtime = try makeRuntime()
  defer { try? runtime.closeBlockingForTests() }
  let map = try await makeMap(runtime)
  defer { try? map.closeBlockingForTests() }
  _ = try runtime.drainEventCopies()

  _ = try? await map.setStyleJson(json: Data(#"{"version":8,"#.utf8))
  try await runtime.barrier()
  let failure = try #require(
    try runtime.drainEventCopies().first { $0.type == .mapLoadingFailed }
  )
  let message = failure.message
  #expect(!message.isEmpty)

  try await map.setStyleJson(json: emptyStyleJSON)
  try await runtime.barrier()
  let second = try runtime.drainEventCopies()

  #expect(second.map(\.type).contains(.mapStyleLoaded))
  // The drain that ends the first batch's window reuses its arena, so the
  // message has to have been copied out rather than borrowed.
  #expect(failure.message == message)
  #expect(failure.payload == .none)
}

/// A map and a runtime built from the default event mask select every
/// event type and deliver each type the test drives, while a bit outside `all`
/// names no event type and is rejected rather than silently kept.
@Test func theDefaultMaskSelectsEveryEventTypeAndAnUnknownBitIsRejected(
) async throws {
  let runtime = try makeRuntime()
  defer { try? runtime.closeBlockingForTests() }
  let map = try await makeMap(runtime)
  defer { try? map.closeBlockingForTests() }

  #expect(try runtime.getEventMask() == .all)
  #expect(try map.snapshotGet().eventMask == .all)

  _ = try runtime.drainEventCopies()
  try await map.setStyleJson(json: emptyStyleJSON)
  _ = try await map
    .updateCamera(update: CameraUpdate(camera: CameraOptions(zoom: 4)))
  try await runtime.barrier()
  let types = try Set(runtime.drainEventCopies().map(\.type))

  #expect(types.contains(.mapStyleLoaded))
  #expect(types.contains(.mapCameraDidChange))
  #expect(types.contains(.mapRenderUpdateAvailable))

  let unnamed = RuntimeEventMask(rawValue: 1 << 63)
  do {
    try runtime.setEventMask(mask: unnamed)
    Issue.record("a runtime mask bit outside all should be rejected")
  } catch let error as MaplibreError {
    #expect(error.kind == .invalidArgument)
  }
  do {
    _ = try await map.setEventMask(mask: unnamed)
    Issue.record("a map mask bit outside all should be rejected")
  } catch let error as MaplibreError {
    #expect(error.kind == .invalidArgument)
  }
  do {
    _ = try await runtime.mapCreate(options: MapOptions(
      initialExtent: LogicalExtent(width: 64, height: 64, scaleFactor: 1),
      eventMask: unnamed
    ))
    Issue.record("a mask bit outside all should be rejected on creation")
  } catch let error as MaplibreError {
    #expect(error.kind == .invalidArgument)
  }
}

/// Once an event is queued, a later mask command does not remove it.
@Test func aQueuedEventSurvivesANarrowedMask() async throws {
  let runtime = try makeRuntime()
  defer { try? runtime.closeBlockingForTests() }
  let map = try await makeMap(runtime)
  defer { try? map.closeBlockingForTests() }
  _ = try runtime.drainEventCopies()

  _ = try await map.setStyleJson(json: emptyStyleJSON)
  try await runtime.barrier()
  _ = try await map
    .setEventMask(mask: RuntimeEventMask.all.subtracting(.mapStyleLoaded))
  try await runtime.barrier()
  #expect(try runtime.drainEventCopies().map(\.type).contains(.mapStyleLoaded))
}

/// A read-modify-write cycle keeps every bit it did not touch, on both handles.
@Test func anEventMaskRoundTripsThroughBothHandles() async throws {
  let runtime = try makeRuntime()
  defer { try? runtime.closeBlockingForTests() }
  let map = try await makeMap(runtime)
  defer { try? map.closeBlockingForTests() }

  try runtime.setEventMask(mask: .all)
  _ = try await map.setEventMask(mask: .all)
  try await runtime.barrier()
  #expect(try runtime.getEventMask() == .all)
  #expect(try map.snapshotGet().eventMask == .all)

  var mapMask = try map.snapshotGet().eventMask
  mapMask.remove(.mapRenderUpdateAvailable)
  _ = try await map.setEventMask(mask: mapMask)
  try await runtime.barrier()
  let readBackMapMask = try map.snapshotGet().eventMask
  #expect(readBackMapMask == mapMask)
  #expect(readBackMapMask.contains(.mapStyleLoaded))
  #expect(!readBackMapMask.contains(.mapRenderUpdateAvailable))

  var runtimeMask = try runtime.getEventMask()
  runtimeMask.remove(.offlineRegionStatusChanged)
  try runtime.setEventMask(mask: runtimeMask)
  let readBackRuntimeMask = try runtime.getEventMask()
  #expect(readBackRuntimeMask == runtimeMask)
  #expect(!readBackRuntimeMask.contains(.offlineRegionStatusChanged))
}

/// A runtime and a map are usable from a thread other than the one that
/// created them, so a host may hand either between execution contexts.
@Test func runtimeAndMapAreUsableFromAnotherThread() async throws {
  let runtime = try makeRuntime()
  defer { try? runtime.closeBlockingForTests() }
  let map = try await makeMap(runtime)
  defer { try? map.closeBlockingForTests() }

  let zoom = try await Task.detached {
    try runtime.setEventMask(mask: .all)
    _ = try await map.setEventMask(mask: .all)
    _ = try await map
      .updateCamera(update: CameraUpdate(camera: CameraOptions(zoom: 3)))
    return try await map.cameraQuery().camera.zoom
  }.value
  #expect(zoom == 3)
  #expect(try map.snapshotGet().eventMask == .all)
}
