import Foundation
import MaplibreNativeC
@testable import MaplibreNativeFFI
import Testing

/// Tests that change the process log callback or report observer run in order.
@Suite(.serialized)
struct GlobalStateTests {
  /// Replacing the log callback releases the registration it replaces, and
  /// clearing it releases the last one.
  @Test func replacingTheLogCallbackReleasesThePreviousOne() async throws {
    let records = LockedBox([LogSeverity]())
    let releases = LockedBox(0)
    defer { try? Maplibre.logClearCallback() }

    try Maplibre.logSetCallback(handler: recordingHandler(
      into: records,
      releasing: releases
    ))
    try await withMapFixture { fixture in
      // MapLibre logs the parse failure of a malformed document.
      _ = try? await fixture.map
        .setStyleJson(#"{"version":8,"#)
      await awaitCondition("a logged parse failure") {
        records.value.contains(.error)
      }
    }
    #expect(releases.value == 0)

    try Maplibre.logSetCallback(handler: recordingHandler(
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

  /// A callback's thrown error is reported, native receives the callback's
  /// fallback, and the runtime keeps working.
  @Test func aCallbackThatThrowsIsContainedAndReported() async throws {
    let capture = ReportCapture()
    defer { capture.finish() }
    defer { try? Maplibre.logClearCallback() }
    try Maplibre.logSetCallback(handler: LogHandler { _, _, _, _ in
      throw CallbackFailure()
    })
    let report =
      "mln_log_callback failed and native received its fallback: CallbackFailure()"
    try await withMapFixture { fixture in
      // MapLibre logs the parse failure of a malformed document.
      _ = try? await fixture.map
        .setStyleJson(#"{"version":8,"#)
      await awaitCondition("the thrown error's report") {
        capture.text.contains(report)
      }
      try await fixture.map.setStyleJson(emptyStyle)
    }
  }

  /// A header transform that throws a `MaplibreError` with a failure status
  /// fails on purpose, so nothing is reported. Any other thrown error is a bug,
  /// which is reported to standard error.
  @Test func aHeaderTransformFailsDeliberatelyWithAStatusAndReportsABug(
  ) async throws {
    let capture = ReportCapture()
    defer { capture.finish() }
    // The URLs name a loopback port that nothing serves, so each request fails
    // after the header transform without leaving the machine.
    let refused = "http://127.0.0.1:1/refused.json"
    let broken = "http://127.0.0.1:1/broken.json"
    try await withMapFixture { fixture in
      let deny = denyingProvider()
      try await fixture.runtime.setResourceProvider(ResourceProvider(
        callback: { request, handle in
          guard [refused, broken].contains(request.requestedUrl) else {
            return deny.callback(request, handle)
          }
          try? handle.passThrough()
        }
      ))
      try await fixture.runtime.setHttpHeaderTransform(
        HttpHeaderTransform(callback: { _, url, _ in
          if url == refused {
            throw MaplibreError(
              status: .invalidArgument,
              diagnostic: "the transform refused"
            )
          }
          if url == broken {
            throw MaplibreError(
              status: .ok,
              diagnostic: "the transform failed without a status"
            )
          }
        })
      )
      for url in [refused, broken] {
        try await fixture.map.setStyleUrl(url)
        let failure = try await fixture
          .awaitEvent("the request to fail") { $0.type == .mapLoadingFailed }
        #expect(failure != nil)
      }
    }
    await awaitCondition("the bug's report") {
      capture.text.contains("the transform failed without a status")
    }
    #expect(!capture.text.contains("the transform refused"))
  }

  /// Abandoned handles are disposed on a worker and reported once to standard
  /// error.
  @Test func anAbandonedHandleIsDisposedOffTheCallbackStackAndReported(
  ) async throws {
    let capture = ReportCapture()
    try abandonInsideACallback()
    let failed =
      "Leaked leaky_handle native handle 0x3 (native disposal failed); "
        + "close handles explicitly."
    let disposed =
      "Leaked leaky_handle native handle 0x4; close handles explicitly."
    await awaitCondition("the leak reports") {
      capture.text.contains(failed) && capture.text.contains(disposed)
    }
    capture.finish()
    #expect(capture.text.components(separatedBy: failed).count == 2)
    #expect(capture.text.components(separatedBy: disposed).count == 2)
    let disposals = RecordedHandle.disposals.value
    #expect(Set(disposals) == [
      RecordedHandle.Disposal(raw: 0x3, insideACallback: false),
      RecordedHandle.Disposal(raw: 0x4, insideACallback: false),
    ])
    #expect(disposals.count == 2)
  }
}

private struct CallbackFailure: Error {}

private func recordingHandler(
  into records: LockedBox<[LogSeverity]>,
  releasing releases: LockedBox<Int>
) -> LogHandler {
  let sentinel = ReleaseProbe(releases)
  return LogHandler { severity, _, _, _ in
    withExtendedLifetime(sentinel) {}
    records.update { $0.append(severity) }
    return true
  }
}

/// A handle whose disposal fails for 0x3, as one native refuses would, and
/// succeeds otherwise. It records each disposal and whether it ran inside a
/// callback.
private struct RecordedHandle: NativeHandle {
  struct Disposal: Hashable {
    let raw: UInt64
    let insideACallback: Bool
  }

  static let disposals = LockedBox([Disposal]())

  let raw: UInt64

  func disposeAbandoned() -> Bool {
    let inside = Thread.current
      .threadDictionary["MaplibreNativeFFITests.callback"] != nil
    Self.disposals.update {
      $0.append(Disposal(raw: raw, insideACallback: inside))
    }
    return raw != 0x3
  }
}

/// Drops handles whose disposal fails or succeeds.
private func abandonInsideACallback() throws {
  Thread.current.threadDictionary["MaplibreNativeFFITests.callback"] = true
  defer {
    Thread.current.threadDictionary
      .removeObject(forKey: "MaplibreNativeFFITests.callback")
  }
  for raw: UInt64 in [0x3, 0x4] {
    _ = try NativeHandleState(
      typeName: "leaky_handle",
      handle: RecordedHandle(raw: raw)
    )
  }
}

/// Records the runtime's reports until ``finish()``; they still reach standard
/// error. Redirecting file descriptor 2 instead races the suite's other
/// threads: when `dup2` replaces a descriptor that a thread's pipe write is
/// blocked on, XNU fails that write with EPIPE and the SIGPIPE kills the
/// process.
private final class ReportCapture: Sendable {
  private let captured = LockedBox("")

  init() {
    reportObserver.set { [captured] message in
      captured.update { $0 += message }
    }
  }

  var text: String {
    captured.value
  }

  func finish() {
    reportObserver.set(nil)
  }
}
