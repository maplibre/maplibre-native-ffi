import CMaplibreNativeC
import Foundation
@testable import MaplibreNativeFFI
import Testing

/// Drains until an event `isMatch` accepts arrives, and returns it.
/// Records an issue naming `subject` and returns `nil` at the deadline.
func drainUntilEvent(
  _ runtime: RuntimeHandle,
  waitingFor subject: String,
  timeout: TimeInterval = 10,
  where isMatch: (RuntimeEvent) -> Bool
) async throws -> RuntimeEvent? {
  let deadline = Date().addingTimeInterval(timeout)
  while Date() < deadline {
    try await runtime.barrier()
    for event in try runtime.drainEvents().events where isMatch(event) {
      return event
    }
    try await Task<Never, Never>.sleep(nanoseconds: 1_000_000)
  }
  Issue.record("timed out waiting for \(subject)")
  return nil
}

func commandDisposition(
  _ commandId: UInt64,
  runtime: RuntimeHandle
) async throws -> UInt32? {
  try await drainUntilEvent(
    runtime,
    waitingFor: "command \(commandId) to finish"
  ) { event in
    guard case let .commandFinished(finished) = event.payload else {
      return false
    }
    return finished.commandId == commandId
  }.flatMap { event in
    guard case let .commandFinished(finished) = event.payload else {
      return nil
    }
    return finished.disposition
  }
}

/// Drains until autonomous execution stops producing events.
func drainUntilQuiet(
  _ runtime: RuntimeHandle,
  iterations: Int = 100
) async throws {
  for _ in 0 ..< iterations {
    try await runtime.barrier()
    if try runtime.drainEvents().events.isEmpty { return }
  }
  Issue.record("the runtime kept producing events while idle")
}

/// A style with no sources, so a load finishes without reaching the network.
let emptyStyleJSON = Data(#"{"version":8,"sources":{},"layers":[]}"#.utf8)

private final class CapturedFailure: @unchecked Sendable {
  private let lock = NSLock()
  private var failure: NativeStatusFailure?

  func store(_ failure: NativeStatusFailure) {
    lock.withLock { self.failure = failure }
  }

  func value() -> NativeStatusFailure? {
    lock.withLock { failure }
  }
}

/// Runs `body` on another thread and returns the native failure it threw.
func failureFromAnotherThread(
  _ body: @escaping @Sendable () throws -> Void
) -> NativeStatusFailure? {
  let captured = CapturedFailure()
  let thread = Thread {
    do {
      try body()
    } catch let failure as NativeStatusFailure {
      captured.store(failure)
    } catch {}
  }
  thread.start()
  while !thread.isFinished {
    usleep(1000)
  }
  return captured.value()
}

/// Builds one raw event record. Every field defaults to what the C API writes
/// for a map event that carries no detail.
func rawRuntimeEvent(
  type: UInt32,
  sourceType: UInt32 = MLN_RUNTIME_EVENT_SOURCE_MAP.rawValue,
  source: UInt64 = 1,
  code: Int32 = 0,
  payloadType: UInt32 = MLN_RUNTIME_EVENT_PAYLOAD_NONE.rawValue,
  messageOffset: UInt32 = 0,
  messageSize: UInt32 = 0
) -> mln_runtime_event {
  var event = mln_runtime_event()
  event.type = type
  event.source_type = sourceType
  event.source = source
  event.code = code
  event.payload_type = payloadType
  event.message_offset = messageOffset
  event.message_size = messageSize
  return event
}

/// Packs each message and its terminator into one arena the way the C API does,
/// and reports where each message starts.
func packMessageArena(
  _ messages: [String]
) -> (bytes: [UInt8], offsets: [UInt32]) {
  var bytes: [UInt8] = []
  var offsets: [UInt32] = []
  for message in messages {
    offsets.append(UInt32(bytes.count))
    bytes.append(contentsOf: Array(message.utf8))
    bytes.append(0)
  }
  return (bytes, offsets)
}

/// One batch a test built over storage it owns.
struct SynthesizedEventBatch {
  let batch: mln_runtime_event_batch_view
  /// The event records, for a test that overwrites them after a decode.
  let records: UnsafeMutableRawBufferPointer
  /// The message arena, for the same reason.
  let messages: UnsafeMutableRawBufferPointer
}

/// Runs `body` with a batch whose records step by `stride`, so a decode test
/// drives strides, message offsets, and payload kinds the C API does not yet
/// produce. Bytes past each record's known fields stay zero unless
/// `payloadWindows` names them, keyed by event index.
func withSynthesizedEventBatch<Result>(
  events: [mln_runtime_event],
  stride: Int = MemoryLayout<mln_runtime_event>.size,
  messages: [UInt8] = [],
  remainingCount: Int = 0,
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
  var messageStorage = messages + [0]
  return try recordStorage.withUnsafeMutableBytes { records in
    try messageStorage.withUnsafeMutableBytes { arena in
      let recordBase = try #require(records.baseAddress)
      for (index, event) in events.enumerated() {
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

      var batch = mln_runtime_event_batch_view()
      batch.size = UInt32(MemoryLayout<mln_runtime_event_batch_view>.size)
      batch.event_size = UInt32(stride)
      batch.events = events.isEmpty
        ? nil
        : UnsafeRawPointer(recordBase)
        .assumingMemoryBound(to: mln_runtime_event.self)
      batch.event_count = events.count
      batch.messages = try messages.isEmpty
        ? nil
        : UnsafeRawPointer(#require(arena.baseAddress))
        .assumingMemoryBound(to: CChar.self)
      batch.messages_size = messages.count
      batch.remaining_count = remainingCount
      return try body(SynthesizedEventBatch(
        batch: batch,
        records: records,
        messages: arena
      ))
    }
  }
}
