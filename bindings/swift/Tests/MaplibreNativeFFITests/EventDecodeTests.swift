import CMaplibreNativeC
import Foundation
@testable import MaplibreNativeFFI
import Testing

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
    generation: 0x1_0000_0005,
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
  #expect(decoded.generation == 0x1_0000_0005)
  #expect(decoded.code == -12)
  #expect(decoded.message == "opaque")
  #expect(decoded.payload == .unknown(0xBEEF, Data(window)))
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
