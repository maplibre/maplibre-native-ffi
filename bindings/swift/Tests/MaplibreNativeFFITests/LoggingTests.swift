import Foundation
@testable import MaplibreNativeFFI
import Testing

/// Reports when the closure that captured it is released, so a test observes
/// that installing or clearing a log callback drops the previous one.
private final class ReleaseSentinel: @unchecked Sendable {
  private let onRelease: @Sendable () -> Void

  init(_ onRelease: @escaping @Sendable () -> Void) {
    self.onRelease = onRelease
  }

  deinit {
    onRelease()
  }
}

/// Builds a callback that appends every record it receives, and whose only
/// strong reference to its sentinel is the callback itself, so the sentinel
/// reports when the registration behind it is dropped.
private func recordingCallback(
  into records: LockedBox<[LogRecord]>,
  releasing releases: LockedBox<Int>
) -> LogCallback {
  let sentinel = ReleaseSentinel { releases.update { $0 += 1 } }
  return { record in
    withExtendedLifetime(sentinel) {}
    records.update { $0.append(record) }
    return true
  }
}

/// Loads a style document MapLibre cannot parse, so it logs a parse failure
/// through whatever callback is installed.
private func logOneParseFailure() async throws {
  let runtime = try RuntimeHandle(
    options: RuntimeOptions(cachePath: ":memory:")
  )
  defer { try? runtime.closeBlockingForTests() }
  let map = try await MapHandle(
    runtime: runtime,
    options: MapOptions(width: 32, height: 32)
  )
  defer { try? map.closeBlockingForTests() }
  _ = try? await map.setStyleJSON(Data(#"{"version":8,"#.utf8))
  try await runtime.barrier()
}

/// The installed callback receives MapLibre's own records, a replacement takes
/// over from it, clearing stops delivery, and each install releases the
/// registration it replaced.
@Test func logCallbackReceivesRecordsUntilReplacedAndCleared() async throws {
  let first = LockedBox([LogRecord]())
  let second = LockedBox([LogRecord]())
  let releases = LockedBox(0)
  defer { try? Maplibre.clearLogCallback() }

  try Maplibre.setLogCallback(
    recordingCallback(into: first, releasing: releases)
  )
  try await logOneParseFailure()
  #expect(first.value.contains { $0.severity == .error })
  #expect(releases.value == 0)

  let firstCount = first.value.count
  try Maplibre.setLogCallback(
    recordingCallback(into: second, releasing: releases)
  )
  #expect(releases.value == 1)

  try await logOneParseFailure()
  #expect(second.value.contains { $0.severity == .error })
  #expect(first.value.count == firstCount)

  try Maplibre.clearLogCallback()
  #expect(releases.value == 2)
  let secondCount = second.value.count
  try await logOneParseFailure()
  #expect(second.value.count == secondCount)
}

@Test func asyncLogSeverityMaskRejectsUnknownBitsBeforeCallingC() throws {
  do {
    try Maplibre.setAsyncLogSeverityMask(LogSeverityMask(rawValue: 1 << 30))
    Issue.record("unknown mask bits should throw")
  } catch let error as MaplibreError {
    #expect(error.kind == .invalidArgument)
    #expect(error.rawStatus == nil)
  } catch {
    Issue.record("unexpected error: \(error)")
  }
}

@Test func asyncLogSeverityMaskAcceptsKnownBits() throws {
  try Maplibre.setAsyncLogSeverityMask([.info, .error])
  try Maplibre.restoreDefaultAsyncLogSeverityMask()
}
