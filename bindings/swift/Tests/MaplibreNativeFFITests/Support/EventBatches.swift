import Foundation
import MaplibreNativeC
@testable import MaplibreNativeFFI
import Testing

/// Builds one raw event record. Every field defaults to what the C API writes
/// for a map event that carries no detail.
func rawRuntimeEvent(
  type: UInt32,
  sourceType: UInt32 = MLN_RUNTIME_EVENT_SOURCE_MAP.rawValue,
  source: UInt64 = 1,
  generation: UInt64 = 0,
  code: UInt32 = 0,
  payloadType: UInt32 = MLN_RUNTIME_EVENT_PAYLOAD_NONE.rawValue
) -> mln_runtime_event {
  var event = mln_runtime_event()
  event.type = mln_runtime_event_type(rawValue: type)
  event.source_type = mln_runtime_event_source_type(rawValue: sourceType)
  event.source = source
  event.generation = generation
  event.code = code
  event.payload_type = mln_runtime_event_payload_type(rawValue: payloadType)
  return event
}

/// One batch a test built over storage it owns.
struct SynthesizedEventBatch {
  let batch: mln_event_batch_view
  /// The event records, for a test that overwrites them after a decode.
  let records: UnsafeMutableRawBufferPointer
  /// The message arena, for the same reason.
  let messages: UnsafeMutableRawBufferPointer
}

/// Runs `body` with a batch whose records step by `stride`, so a decode test
/// drives strides, message views, and payload kinds the C API does not yet
/// produce. Bytes past each record's known fields stay zero unless
/// `payloadWindows` names them, keyed by event index.
func withSynthesizedEventBatch<Result>(
  events: [mln_runtime_event],
  stride: Int = MemoryLayout<mln_runtime_event>.size,
  messages: [String] = [],
  payloadWindows: [Int: [UInt8]] = [:],
  _ body: (SynthesizedEventBatch) throws -> Result
) throws -> Result {
  let payloadOffset = try #require(
    MemoryLayout<mln_runtime_event>.offset(of: \.payload)
  )
  // The view borrows an array of event records, so the storage behind one has
  // to carry the record alignment; a `UInt64` element buffer does. An empty
  // allocation has no base address, and the C API spells an empty array or
  // arena as a null pointer, so one spare element keeps both in one code path.
  var recordStorage = [UInt64](
    repeating: 0,
    count: stride * events.count / 8 + 1
  )
  var messageStorage: [UInt8] = messages
    .flatMap { Array($0.utf8) + [UInt8(0)] } + [0]
  return try recordStorage.withUnsafeMutableBytes { records in
    try messageStorage.withUnsafeMutableBytes { arena in
      let recordBase = try #require(records.baseAddress)
      var messageOffset = 0
      for (index, original) in events.enumerated() {
        var event = original
        if index < messages.count {
          let length = messages[index].utf8.count
          event.message = mln_buffer_view(
            data: arena.baseAddress?.advanced(by: messageOffset),
            size: length
          )
          messageOffset += length + 1
        }
        let record = recordBase.advanced(by: index * stride)
        record.storeBytes(of: event, as: mln_runtime_event.self)
        guard let window = payloadWindows[index] else { continue }
        window.withUnsafeBytes { bytes in
          UnsafeMutableRawBufferPointer(
            start: record.advanced(by: payloadOffset),
            count: bytes.count
          ).copyMemory(from: bytes)
        }
      }

      var batch = mln_event_batch_view()
      batch.size = UInt32(MemoryLayout<mln_event_batch_view>.size)
      batch.event_size = UInt32(stride)
      batch.events = events.isEmpty
        ? nil
        : UnsafeRawPointer(recordBase)
        .assumingMemoryBound(to: mln_runtime_event.self)
      batch.event_count = events.count
      return try body(SynthesizedEventBatch(
        batch: batch,
        records: records,
        messages: arena
      ))
    }
  }
}

/// Delivers `batch` through a runtime event handler's native callback, as
/// native does, and returns the events that the handler received.
func deliveredEvents(_ batch: mln_event_batch_view) throws -> [RuntimeEvent] {
  final class Received: @unchecked Sendable {
    var events: [RuntimeEvent]?
  }
  let received = Received()
  let arena = NativeInputArena()
  let handler = try RuntimeEventHandler { received.events = $0 }
    .nativeValue(arena: arena)
  var batch = batch
  let callback = try #require(handler.callback)
  callback(handler.user_data, &batch)
  return try #require(received.events)
}
