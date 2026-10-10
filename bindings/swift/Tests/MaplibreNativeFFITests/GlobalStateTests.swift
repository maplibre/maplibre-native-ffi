#if canImport(Darwin)
  import Darwin
#elseif canImport(Glibc)
  import Glibc
#endif
import Foundation
@testable import MaplibreNativeFFI
import Testing

/// The log callback, the diagnostic handler, and the leak report's standard
/// error belong to the process, so the tests that touch them run one at a
/// time.
@Suite(.serialized)
struct GlobalStateTests {
  /// Replacing the log callback releases the registration it replaces, and
  /// clearing it releases the last one.
  @Test func replacingTheLogCallbackReleasesThePreviousOne() async throws {
    let records = LockedBox([LogSeverity]())
    let releases = LockedBox(0)
    defer { try? Maplibre.logClearCallback() }

    try Maplibre.logSetCallback(recordingCallback(
      into: records,
      releasing: releases
    ))
    try await withMapFixture { fixture in
      // MapLibre logs the parse failure of a malformed document.
      _ = try? await fixture.map
        .setStyleJson(json: Data(#"{"version":8,"#.utf8))
      await awaitCondition("a logged parse failure") {
        records.value.contains(.error)
      }
    }
    #expect(releases.value == 0)

    try Maplibre.logSetCallback(recordingCallback(
      into: LockedBox([]),
      releasing: releases
    ))
    await awaitCondition("the replaced callback's release") {
      releases.value == 1
    }
    try Maplibre.logClearCallback()
    await awaitCondition("the cleared callback's release") {
      releases.value == 2
    }
  }

  /// A handle abandoned inside a callback is disposed after the callback, off
  /// its stack, and a disposal that fails reports the leak on standard error.
  @Test func anAbandonedHandleIsDisposedOffTheCallbackStackAndReported(
  ) async throws {
    let capture = try StandardErrorCapture()
    try abandonInsideACallback()
    await awaitCondition("the leak report") {
      capture.text.contains(
        "Leaked leaky_handle native handle 0x3; close handles explicitly."
      )
    }
    capture.finish()
    #expect(UnretirableHandle.disposedInsideACallback.value == [false])
  }

  /// A provider that throws is contained: the binding passes the request
  /// through, so the map reports the load failure it would have without a
  /// provider, and the request the callback saw is closed. The diagnostic
  /// handler receives the error.
  @Test func aThrowingProviderPassesTheRequestThroughAndIsReported(
  ) async throws {
    let reports = LockedBox([String]())
    Maplibre.setDiagnosticHandler { diagnostic in
      // Tests in other suites run concurrently, so only this error counts.
      if case let .callbackError(callback, error) = diagnostic,
         error is ProviderFailure
      {
        reports.update { $0.append(callback) }
      }
    }
    defer { Maplibre.setDiagnosticHandler(nil) }
    try await withMapFixture { fixture in
      let seen = LockedBox<ResourceRequestHandle?>(nil)
      try await installProvider(
        on: fixture.runtime,
        for: "custom://throwing.json"
      ) { handle in
        seen.update { $0 = handle }
        throw ProviderFailure()
      }
      try await fixture.map.setStyleUrl(url: "custom://throwing.json")
      let failure = try await fixture.awaitEvent("the pass-through failure") {
        $0.type == .mapLoadingFailed && $0.message.contains("custom")
      }
      #expect(failure != nil)
      #expect(try #require(seen.value).isClosed)
    }
    #expect(reports.value == ["mln_resource_provider_callback"])
  }
}

private struct ProviderFailure: Error {}

private func recordingCallback(
  into records: LockedBox<[LogSeverity]>,
  releasing releases: LockedBox<Int>
) -> @Sendable (LogSeverity, LogEvent, Int64, String) throws -> UInt32 {
  let sentinel = ReleaseProbe(releases)
  return { severity, _, _, _ in
    withExtendedLifetime(sentinel) {}
    records.update { $0.append(severity) }
    return 1
  }
}

/// A handle whose disposal fails, as one native refuses would, and records
/// whether it ran inside a callback.
private struct UnretirableHandle: NativeHandle {
  static let disposedInsideACallback = LockedBox([Bool]())

  let raw: UInt64

  func disposeAbandoned() -> Bool {
    let inside = NativeCallbackGuard.isActive
    Self.disposedInsideACallback.update { $0.append(inside) }
    return false
  }
}

/// Drops the last reference to a live handle state while a callback guard is
/// active, as a callback that abandons a handle does.
private func abandonInsideACallback() throws {
  let scope = NativeCallbackGuard.enter(owner: nil, operations: [])
  defer { scope.end() }
  _ = try NativeHandleState(
    typeName: "leaky_handle",
    handle: UnretirableHandle(raw: 0x3)
  )
}

/// Redirects the process's standard error into a pipe until ``finish()``,
/// which restores it and writes everything captured back to it.
private final class StandardErrorCapture: @unchecked Sendable {
  private let saved: Int32
  private let pipe = Pipe()
  private let captured = LockedBox(Data())
  private let drained = DispatchSemaphore(value: 0)

  init() throws {
    // Flushing every stream reaches standard error without naming Glibc's
    // global `stderr`, which strict concurrency rejects as shared state.
    fflush(nil)
    saved = dup(STDERR_FILENO)
    guard saved >= 0,
          dup2(pipe.fileHandleForWriting.fileDescriptor, STDERR_FILENO) >= 0
    else { throw FixtureError("standard error could not be redirected") }
    let reader = pipe.fileHandleForReading
    Thread { [captured, drained] in
      while true {
        let chunk = reader.availableData
        if chunk.isEmpty { break }
        captured.update { $0.append(chunk) }
      }
      drained.signal()
    }.start()
  }

  var text: String {
    String(decoding: captured.value, as: UTF8.self)
  }

  func finish() {
    fflush(nil)
    dup2(saved, STDERR_FILENO)
    close(saved)
    try? pipe.fileHandleForWriting.close()
    _ = isSignalled(drained)
    FileHandle.standardError.write(captured.value)
  }
}
