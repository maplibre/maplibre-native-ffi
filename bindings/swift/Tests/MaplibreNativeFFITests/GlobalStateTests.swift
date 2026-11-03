import Foundation
@testable import MaplibreNativeFFI
import Testing

/// Tests that change the process report observer run in order.
@Suite(.serialized)
struct GlobalStateTests {
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
        try fixture.map.discarding.setStyleUrl(url)
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
