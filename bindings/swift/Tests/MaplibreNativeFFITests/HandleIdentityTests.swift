import CMaplibreNativeC
import Foundation
@testable import MaplibreNativeFFI
import Testing

// Handle-identity behaviour the C API owns, reached through the internal
// handle accessors because the safe public API has no way to express these
// calls.

private func makeMap(_ runtime: RuntimeHandle) async throws -> MapHandle {
  try await MapHandle(runtime: runtime,
                      options: MapOptions(
                        width: 64,
                        height: 64,
                        scaleFactor: 1.0,
                        mode: .continuous
                      ))
}

private func mapSize(_ map: NativeMapHandle) throws {
  _ = try NativeMap.snapshot(map)
}

/// BND-045.
@Test func releasedMapIdReplayedAfterANewMapIsReportedStale() async throws {
  let runtime =
    try await RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }

  let first = try await makeMap(runtime)
  let released = try first.requireLiveHandle()
  try first.close()

  // The released slot is the one the next map takes, so the replayed id
  // names a retired generation of a slot that is live again.
  let second = try await makeMap(runtime)
  defer { try? second.closeBlockingForTests() }

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
@Test func mapIdPassedToARuntimeOperationIsRejectedOnItsKind() async throws {
  let runtime =
    try await RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }
  let map = try await makeMap(runtime)
  defer { try? map.closeBlockingForTests() }

  // NativeMapHandle and NativeRuntimeHandle are distinct types, so this call
  // has no expression in the safe API and needs the raw id.
  let wrongKind = try map.requireLiveHandle().raw
  var operation: mln_operation = 0
  do {
    try checkStatus(mln_runtime_barrier_start(wrongKind, &operation))
    Issue.record("a map id should not name a runtime")
  } catch let failure as NativeStatusFailure {
    #expect(failure.rawStatus == MLN_STATUS_INVALID_ARGUMENT.rawValue)
    #expect(failure.diagnostic.contains("map"))
    #expect(failure.diagnostic.contains("runtime"))
  }
}
