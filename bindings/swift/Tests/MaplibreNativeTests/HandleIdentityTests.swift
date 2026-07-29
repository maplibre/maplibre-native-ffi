import CMaplibreNativeC
import Foundation
@testable import MaplibreNative
import Testing

private final class CapturedFailure: @unchecked Sendable {
  private let lock = NSLock()
  private var failure: NativeStatusFailure?

  func store(_ value: NativeStatusFailure) {
    lock.withLock { failure = value }
  }

  func value() -> NativeStatusFailure? {
    lock.withLock { failure }
  }
}

// Handle-identity behaviour the C API owns, reached through the internal
// handle accessors because the safe public API has no way to express these
// calls.

private func makeMap(_ runtime: RuntimeHandle) throws -> MapHandle {
  try MapHandle(
    runtime: runtime,
    options: MapOptions(
      width: 64,
      height: 64,
      scaleFactor: 1.0,
      mode: .continuous
    )
  )
}

private func mapSize(_ map: NativeMapHandle) throws {
  var width: UInt32 = 0
  var height: UInt32 = 0
  var scaleFactor: Double = 0
  try checkStatus(
    mln_map_get_size(map.raw, &width, &height, &scaleFactor)
  )
}

/// BND-045.
@Test func releasedMapIdReplayedAfterANewMapIsReportedStale() throws {
  let runtime =
    try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.close() }

  let first = try makeMap(runtime)
  let released = try first.requireLiveHandle()
  try first.close()

  // The released slot is the one the next map takes, so this is the case a
  // pointer handle could not tell apart from a live map.
  let second = try makeMap(runtime)
  defer { try? second.close() }

  #expect(throws: NativeStatusFailure.self) { try mapSize(released) }
  do {
    try mapSize(released)
  } catch let failure as NativeStatusFailure {
    #expect(failure.rawStatus == MLN_STATUS_INVALID_ARGUMENT.rawValue)
    #expect(failure.diagnostic.contains("stale"))
  }

  // The live map is unaffected by the replay.
  try mapSize(second.requireLiveHandle())
}

/// BND-047.
@Test func mapIdPassedToARuntimeOperationIsRejectedOnItsKind() throws {
  let runtime =
    try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.close() }
  let map = try makeMap(runtime)
  defer { try? map.close() }

  // NativeMapHandle and NativeRuntimeHandle are distinct types, so this call
  // has no expression in the safe API and needs the raw id.
  let wrongKind = try map.requireLiveHandle().raw
  do {
    try checkStatus(mln_runtime_pump(wrongKind, 0))
    Issue.record("a map id should not name a runtime")
  } catch let failure as NativeStatusFailure {
    #expect(failure.rawStatus == MLN_STATUS_INVALID_ARGUMENT.rawValue)
    #expect(failure.diagnostic.contains("map"))
    #expect(failure.diagnostic.contains("runtime"))
  }
}

/// BND-049.
@Test func liveMapIdCalledFromAnotherThreadReportsWrongThread() throws {
  let runtime =
    try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.close() }
  let map = try makeMap(runtime)
  defer { try? map.close() }

  let live = try map.requireLiveHandle()
  let captured = CapturedFailure()
  let thread = Thread {
    do {
      try mapSize(live)
    } catch let failure as NativeStatusFailure {
      captured.store(failure)
    } catch {}
  }
  thread.start()
  while !thread.isFinished {
    usleep(1000)
  }

  // The id is live, so the owner-thread rule decides rather than identity.
  let failure = try #require(captured.value())
  #expect(failure.rawStatus == MLN_STATUS_WRONG_THREAD.rawValue)
  #expect(!failure.diagnostic.contains("stale"))
}
