import Foundation
import MaplibreNativeC
@testable import MaplibreNativeFFI
import Testing

/// The batch reports the record stride, which a later C API version widens
/// by adding a payload member, so the decoder steps by it rather than by its
/// own event size. An event type, source kind, and payload kind this version
/// does not name reach the host with their raw values, and the payload arrives
/// as the stride's own byte window, copied out of storage the next drain
/// reuses.
@Test func unknownEventSourceAndPayloadKindsSurviveAsRawValues() throws {
  let stride = MemoryLayout<mln_runtime_event>.size + 8
  let payloadOffset = try #require(
    MemoryLayout<mln_runtime_event>.offset(of: \.payload)
  )
  let window = (0 ..< stride - payloadOffset).map { UInt8($0 & 0xFF) }
  let event = rawRuntimeEvent(
    type: 0x4242,
    sourceType: 9,
    source: 0x77,
    generation: 0x1_0000_0005,
    code: 0xFFFF_FFF4,
    payloadType: 0xBEEF
  )

  // The second event decodes only when the walk steps by the reported stride.
  let known = rawRuntimeEvent(
    type: MLN_RUNTIME_EVENT_MAP_IDLE.rawValue,
    source: 0x22
  )

  let events = try withSynthesizedEventBatch(
    events: [event, known],
    stride: stride,
    messages: ["opaque", "second"],
    payloadWindows: [0: window]
  ) { synthesized in
    let events = try deliveredEvents(synthesized.batch)
    synthesized.records.copyBytes(
      from: repeatElement(UInt8(0xFF), count: synthesized.records.count)
    )
    synthesized.messages.copyBytes(
      from: repeatElement(UInt8(0xFF), count: synthesized.messages.count)
    )
    return events
  }

  let decoded = try #require(events.first)
  #expect(decoded.type.rawValue == 0x4242)
  #expect(decoded.sourceType.rawValue == 9 && decoded.source == 0x77)
  #expect(decoded.generation == 0x1_0000_0005)
  #expect(decoded.code == 0xFFFF_FFF4)
  #expect(decoded.message == "opaque")
  #expect(decoded.payload == .unknown(0xBEEF, Data(window)))
  #expect(events.count == 2)
  #expect(events.last?.type == .mapIdle)
  #expect(events.last?.source == 0x22)
  #expect(events.last?.message == "second")
}

/// A typed payload reads the union member its kind names.
@Test func typedPayloadsDecodeTheUnionMemberTheirKindNames() throws {
  var frame = rawRuntimeEvent(
    type: MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED.rawValue,
    payloadType: MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME.rawValue
  )
  frame.payload.render_frame.mode = MLN_RENDER_MODE_FULL
  frame.payload.render_frame.needs_repaint = true
  frame.payload.render_frame.stats.frame_count = 21
  frame.payload.render_frame.stats.encoding_time = 0.5

  let events = try withSynthesizedEventBatch(
    events: [frame]
  ) { synthesized in
    try deliveredEvents(synthesized.batch)
  }
  let payloads = events.map(\.payload)

  guard case let .renderFrame(decodedFrame) = payloads[0] else {
    Issue.record("expected a render frame payload, got \(payloads[0])")
    return
  }
  #expect(decodedFrame.mode == .full)
  #expect(decodedFrame.needsRepaint)
  #expect(!decodedFrame.placementChanged)
  #expect(decodedFrame.stats.frameCount == 21)
  #expect(decodedFrame.stats.encodingTime == 0.5)
}
