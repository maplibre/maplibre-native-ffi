import CMaplibreNativeC
import Foundation
@testable import MaplibreNativeFFI
import Testing

private func makeRuntime() async throws -> RuntimeHandle {
  try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
}

private func makeMap(_ runtime: RuntimeHandle) async throws -> MapHandle {
  try await MapHandle(
    runtime: runtime,
    options: MapOptions(width: 64, height: 64)
  )
}

// MARK: - Decoding a batch

/// BND-087. The batch reports the record stride, and a later C API version
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
    try NativeRuntimeEventBatch(copying: synthesized.batch)
  }

  #expect(batch.events.map(\.type) == [4, 8, 9])
  #expect(batch.events.map(\.sourceId) == [0x11, 0x22, 0x33])
  #expect(batch.events.map(\.message) == ["first", "second", ""])
  #expect(batch.events.map(\.code) == [0, -3, 0])
}

/// BND-083. An event type, source kind, and payload kind this version does not
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
    let batch = try NativeRuntimeEventBatch(copying: synthesized.batch)
    synthesized.records.copyBytes(
      from: repeatElement(UInt8(0xFF), count: synthesized.records.count)
    )
    synthesized.messages.copyBytes(
      from: repeatElement(UInt8(0xFF), count: synthesized.messages.count)
    )
    return batch
  }

  let decoded = try RuntimeEvent(native: #require(batch.events.first))
  #expect(decoded.type == .unknown(0x4242))
  #expect(decoded.source == .unknown(sourceType: 9, source: 0x77))
  #expect(decoded.code == -12)
  #expect(decoded.message == "opaque")
  #expect(decoded.payload == .unknown(type: 0xBEEF, bytes: window))
}

/// BND-086. A map-sourced event names its map by the id the C API delivered,
/// whether or not this process still holds a ``MapHandle`` for it, so a host
/// keeps the identity it needs to route or forward the event.
@Test func aMapSourcedEventKeepsAnIdNoLiveMapHandleClaims() async throws {
  let runtime = try await makeRuntime()
  defer { try? runtime.closeBlockingForTests() }
  let map = try await makeMap(runtime)
  defer { try? map.closeBlockingForTests() }

  let batch = try withSynthesizedEventBatch(
    events: [rawRuntimeEvent(type: 4, source: 0xDEAD_BEEF)]
  ) { synthesized in
    try NativeRuntimeEventBatch(copying: synthesized.batch)
  }

  let decoded = try RuntimeEvent(native: #require(batch.events.first))
  #expect(decoded.source == .map(MapId(value: 0xDEAD_BEEF)))
  #expect(!map.isSource(of: decoded))
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
    type: 23,
    payloadType: MLN_RUNTIME_EVENT_PAYLOAD_CAMERA_TRANSITION_FINISHED.rawValue
  )
  transition.payload.camera_transition_finished.transition_id = 99

  let batch = try withSynthesizedEventBatch(
    events: [frame, tile, transition]
  ) { synthesized in
    try NativeRuntimeEventBatch(copying: synthesized.batch)
  }
  let payloads = batch.events
    .map { RuntimeEvent(native: $0).payload }

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

/// BND-060, BND-061. The default parameter takes the C API's own default,
/// bits this build does not name included, and a mask the host writes reaches
/// the struct as it stands.
@Test func mapAndRuntimeOptionsAlwaysEncodeAnEventMask() throws {
  #expect(RuntimeEventMask.allRuntimeEvents.contains(.commandFinished))
  try MapOptions(width: 8, height: 8).nativeInput.withNativeOptions { native in
    #expect(native.pointee.event_mask == mln_map_options_default().event_mask)
  }
  let mapMask: RuntimeEventMask = [.mapStyleLoaded, .mapIdle]
  try MapOptions(width: 8, height: 8, eventMask: mapMask).nativeInput
    .withNativeOptions { native in
      #expect(native.pointee.event_mask == mapMask.rawValue)
    }
  try MapOptions(width: 8, height: 8, eventMask: []).nativeInput
    .withNativeOptions { native in
      #expect(native.pointee.event_mask == 0)
    }

  try RuntimeOptions().nativeInput.withNativeOptions(notificationSource: 1) {
    native in
    #expect(
      native.pointee.event_mask == mln_runtime_options_default().event_mask
    )
    #expect(native.pointee.notification_source == 1)
  }
  try RuntimeOptions(eventMask: .allRuntimeEvents).nativeInput
    .withNativeOptions(notificationSource: 1) { native in
      #expect(
        native.pointee.event_mask == RuntimeEventMask.allRuntimeEvents.rawValue
      )
      #expect(native.pointee.notification_source == 1)
    }
}

// MARK: - Draining a live runtime

/// BND-090. One drain reports every event a style load produced, from the map
/// that produced them.
@Test func oneDrainReportsEveryEventAStyleLoadProduced() async throws {
  let runtime = try await makeRuntime()
  defer { try? runtime.closeBlockingForTests() }
  let map = try await makeMap(runtime)
  defer { try? map.closeBlockingForTests() }
  _ = try runtime.drainEvents()

  try map.setStyleJSON(emptyStyleJSON)
  try await runtime.barrier()
  let batch = try runtime.drainEvents()

  #expect(batch.events.count > 1)
  #expect(batch.events.map(\.type).contains(.mapStyleLoaded))
  #expect(batch.events.allSatisfy { map.isSource(of: $0) })
}

/// BND-082, BND-092. An event copied out of a batch keeps its message and
/// payload after the drain that ends the batch's window.
@Test func anEventTakenOutOfABatchOutlivesTheNextDrain() async throws {
  let runtime = try await makeRuntime()
  defer { try? runtime.closeBlockingForTests() }
  let map = try await makeMap(runtime)
  defer { try? map.closeBlockingForTests() }
  _ = try runtime.drainEvents()

  try? map.setStyleJSON(Data(#"{"version":8,"#.utf8))
  try await runtime.barrier()
  let failure = try #require(
    try runtime.drainEvents().events.first { $0.type == .mapLoadingFailed }
  )
  #expect(!failure.message.isEmpty)
  let copied = failure

  try map.setStyleJSON(emptyStyleJSON)
  try await runtime.barrier()
  let second = try runtime.drainEvents()

  #expect(second.events.map(\.type).contains(.mapStyleLoaded))
  #expect(failure == copied)
  #expect(!failure.message.isEmpty)
}

/// BND-091. A map and a runtime built from the default event mask select every
/// event type and deliver each type the test drives, while a bit outside `all`
/// names no event type and is rejected rather than silently kept.
@Test func theDefaultMaskSelectsEveryEventTypeAndAnUnknownBitIsRejected(
) async throws {
  let runtime = try await makeRuntime()
  defer { try? runtime.closeBlockingForTests() }
  let map = try await makeMap(runtime)
  defer { try? map.closeBlockingForTests() }

  #expect(try runtime.eventMask == .all)
  #expect(try map.eventMask == .all)

  _ = try runtime.drainEvents()
  try map.setStyleJSON(emptyStyleJSON)
  _ = try map.updateCamera(CameraUpdate(camera: CameraOptions(zoom: 4)))
  try await runtime.barrier()
  let types = try Set(runtime.drainEvents().events.map(\.type))

  #expect(types.contains(.mapStyleLoaded))
  #expect(types.contains(.mapCameraDidChange))
  #expect(types.contains(.mapRenderUpdateAvailable))

  let unnamed = RuntimeEventMask(rawValue: 1 << 63)
  for setMask in [runtime.setEventMask, map.setEventMask] {
    do {
      try setMask(unnamed)
      Issue.record("a mask bit outside all should be rejected")
    } catch let error as MaplibreError {
      #expect(error.kind == .invalidArgument)
    }
  }
  do {
    _ = try await MapHandle(runtime: runtime,
                            options: MapOptions(
                              width: 64,
                              height: 64,
                              eventMask: unnamed
                            ))
    Issue.record("a mask bit outside all should be rejected on creation")
  } catch let error as MaplibreError {
    #expect(error.kind == .invalidArgument)
  }
}

/// Once an event is queued, a later mask command does not remove it.
@Test func aQueuedEventSurvivesANarrowedMask() async throws {
  let runtime = try await makeRuntime()
  defer { try? runtime.closeBlockingForTests() }
  let map = try await makeMap(runtime)
  defer { try? map.closeBlockingForTests() }
  _ = try runtime.drainEvents()

  _ = try map.setStyleJSON(emptyStyleJSON)
  try await runtime.barrier()
  _ = try map.setEventMask(RuntimeEventMask.all.subtracting(.mapStyleLoaded))
  try await runtime.barrier()
  #expect(try runtime.drainEvents().events.map(\.type)
    .contains(.mapStyleLoaded))
}

/// A read-modify-write cycle keeps every bit it did not touch, on both handles.
@Test func anEventMaskRoundTripsThroughBothHandles() async throws {
  let runtime = try await makeRuntime()
  defer { try? runtime.closeBlockingForTests() }
  let map = try await makeMap(runtime)
  defer { try? map.closeBlockingForTests() }

  try runtime.setEventMask(.all)
  _ = try map.setEventMask(.all)
  try await runtime.barrier()
  #expect(try runtime.eventMask == .all)
  #expect(try map.eventMask == .all)

  var mapMask = try map.eventMask
  mapMask.remove(.mapRenderUpdateAvailable)
  _ = try map.setEventMask(mapMask)
  try await runtime.barrier()
  let readBackMapMask = try map.eventMask
  #expect(readBackMapMask == mapMask)
  #expect(readBackMapMask.contains(.mapStyleLoaded))
  #expect(!readBackMapMask.contains(.mapRenderUpdateAvailable))

  var runtimeMask = try runtime.eventMask
  runtimeMask.remove(.offlineRegionStatusChanged)
  try runtime.setEventMask(runtimeMask)
  let readBackRuntimeMask = try runtime.eventMask
  #expect(readBackRuntimeMask == runtimeMask)
  #expect(!readBackRuntimeMask.contains(.offlineRegionStatusChanged))
}

/// Runtime and map operations remain valid after Swift resumes on another task.
@Test func runtimeAndMapAreUsableAfterTaskResumption() async throws {
  let runtime = try await makeRuntime()
  defer { try? runtime.closeBlockingForTests() }
  let map = try await makeMap(runtime)
  defer { try? map.closeBlockingForTests() }

  await Task.yield()
  try runtime.setEventMask(.all)
  try map.setEventMask(.all)
  _ = try map.updateCamera(CameraUpdate(camera: CameraOptions(zoom: 3)))
  let camera = try await map.queryCamera()
  #expect(camera.camera.zoom == 3)
}
