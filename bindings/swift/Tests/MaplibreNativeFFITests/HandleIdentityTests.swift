import CMaplibreNativeC
import Foundation
@testable import MaplibreNativeFFI
import Testing

// Handle-identity behaviour the C API owns, reached through the internal
// handle accessors because the safe public API has no way to express these
// calls.

private func makeMap(_ runtime: RuntimeHandle) async throws -> MapHandle {
  try await MapHandle(runtime: runtime,
                      options: MapOptions(width: 64, height: 64))
}

private func readSnapshot(_ map: NativeMapHandle) throws {
  _ = try NativeMap.snapshot(map)
}

/// BND-045.
@Test func releasedMapIdReplayedAfterANewMapIsReportedStale() async throws {
  let runtime =
    try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }

  let first = try await makeMap(runtime)
  let released = try first.requireLiveHandle()
  try await first.close()

  // The released slot is the one the next map takes, so the replayed id
  // names a retired generation of a slot that is live again.
  let second = try await makeMap(runtime)
  defer { try? second.closeBlockingForTests() }

  do {
    try readSnapshot(released)
    Issue.record("a replayed map id should be rejected as stale")
  } catch let failure as NativeStatusFailure {
    #expect(failure.rawStatus == MLN_STATUS_INVALID_ARGUMENT.rawValue)
    #expect(failure.diagnostic.contains("stale"))
  }

  // The live map is unaffected by the replay.
  try readSnapshot(second.requireLiveHandle())
}

/// BND-047.
@Test func mapIdPassedToARuntimeOperationIsRejectedOnItsKind() async throws {
  let runtime =
    try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }
  let map = try await makeMap(runtime)
  defer { try? map.closeBlockingForTests() }

  // NativeMapHandle and NativeRuntimeHandle are distinct types, so this call
  // has no expression in the safe API and needs the raw id.
  let wrongKind = try map.requireLiveHandle().raw
  var completion = mln_completion(
    size: UInt32(MemoryLayout<mln_completion>.size),
    callback: { _, _ in },
    user_data: nil,
    release_user_data: nil
  )
  do {
    try checkStatus(mln_runtime_barrier(wrongKind, &completion))
    Issue.record("a map id should not name a runtime")
  } catch let failure as NativeStatusFailure {
    #expect(failure.rawStatus == MLN_STATUS_INVALID_ARGUMENT.rawValue)
    #expect(failure.diagnostic.contains("map"))
    #expect(failure.diagnostic.contains("runtime"))
  }
}
