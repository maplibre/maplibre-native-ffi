import Foundation
import MaplibreNativeC
@testable import MaplibreNativeFFI
import Testing

/// A camera command's end handler sits two records deep in its input, and
/// once the command is gone only native holds it. Native runs it once when
/// its identity is cancelled.
@Test func aCameraEndHandlerRunsAfterItsCommandIsGone() async throws {
  try await withMapFixture { fixture in
    let ends = LockedBox<[CameraTransitionEnd]>([])
    do {
      let update = CameraUpdate(
        mode: .ease,
        camera: CameraOptions(zoom: 4),
        animation: AnimationOptions(
          durationMs: 60000,
          transitionId: UInt64.max,
          endHandler: CameraTransitionHandler { end in
            ends.update { $0.append(end) }
          }
        )
      )
      try await fixture.map.updateCamera(update)
    }
    _ = try await fixture.map
      .cancelCameraTransitionsWithId(transitionId: UInt64.max)
    await awaitCondition("the end handler") { !ends.value.isEmpty }
    #expect(ends.value.count == 1)
    #expect(ends.value.first?.outcome == .cancelled)
    #expect(ends.value.first?.generation != 0)
  }
}

/// Installs a provider for `url` that hands each of its requests to `body`,
/// and denies every other request.
func installProvider(
  on runtime: RuntimeHandle,
  for url: String,
  _ body: @escaping @Sendable (ResourceRequestHandle) throws -> Void
) async throws {
  let deny = denyingProvider()
  try await runtime.setResourceProvider(ResourceProvider(
    callback: { request, handle in
      guard request.requestedUrl == url else {
        return deny.callback(request, handle)
      }
      do { try body(handle) }
      catch {
        Issue.record("provider failed: \(error)")
        handle.close()
      }
    }
  ))
}

/// A provider that takes a request answers it later, from outside the
/// callback, and the answer consumes the request's handle.
@Test func aProviderAnswersATakenRequestLater() async throws {
  try await withMapFixture { fixture in
    let taken = LockedBox<ResourceRequestHandle?>(nil)
    try await installProvider(
      on: fixture.runtime,
      for: "custom://later.json"
    ) { handle in
      taken.update { $0 = handle }
    }
    try await fixture.map.setStyleUrl("custom://later.json")
    await awaitCondition("the provider to take the request") {
      taken.value != nil
    }
    let request = try #require(taken.value)

    try request.complete(response: ResourceResponse(
      status: .ok,
      bytes: Data(emptyStyle.utf8)
    ))
    #expect(try await fixture.awaitEvent("the style answered later") {
      $0.type == .mapStyleLoaded
    } != nil)
    #expect(request.isClosed)
    await expectMaplibreError(.invalidState) {
      try request.complete(response: ResourceResponse(status: .ok))
    }
  }
}

/// A header transform's response object works only on the callback's thread
/// and only until the callback returns.
@Test func aHeaderResponseExpiresWithItsCallback() async throws {
  try await withMapFixture { fixture in
    // The provider passes this request to the HTTP client, which runs the
    // header transform before it tries a port that nothing serves.
    let url = "http://127.0.0.1:1/headers.json"
    try await installProvider(on: fixture.runtime, for: url) {
      try $0.passThrough()
    }
    let escaped = LockedBox<HttpHeaderTransformResponse?>(nil)
    let otherThread = LockedBox<MaplibreError?>(nil)
    try await fixture.runtime.setHttpHeaderTransform(
      HttpHeaderTransform(callback: { _, requested, response in
        guard requested == url else { return }
        let done = DispatchSemaphore(value: 0)
        Thread {
          do { try response.setHeader(name: "X-Test", value: "other") }
          catch let error as MaplibreError {
            otherThread.update { $0 = error }
          } catch {}
          done.signal()
        }.start()
        _ = isSignalled(done)
        try response.setHeader(name: "X-Test", value: "inside")
        escaped.update { $0 = response }
      })
    )
    try await fixture.map.setStyleUrl(url)
    let failure = try await fixture
      .awaitEvent("the request to fail") {
        $0.type == .mapLoadingFailed
      }
    #expect(failure != nil)

    #expect(otherThread.value?.status == .invalidState)
    let response = try #require(escaped.value)
    await expectMaplibreError(.invalidState) {
      try response.setHeader(name: "X-Test", value: "late")
    }
  }
}

/// A transform owns the request its callback receives, and another thread
/// answers it after the callback returns, which consumes the request.
@Test func aTransformRequestIsAnsweredLater() async throws {
  try await withMapFixture { fixture in
    // The provider passes the style through to the online source, which
    // waits for the transform's answer.
    let url = "custom://transformed.json"
    try await installProvider(on: fixture.runtime, for: url) {
      try $0.passThrough()
    }
    let taken = LockedBox<ResourceTransformRequestHandle?>(nil)
    try await fixture.runtime.setResourceTransform(
      ResourceTransform(callback: { _, requested, request in
        guard requested == url else { return request.close() }
        taken.update { $0 = request }
      })
    )
    try await fixture.map.setStyleUrl(url)
    await awaitCondition("the transform to take the request") {
      taken.value != nil
    }
    let request = try #require(taken.value)
    let answered = LockedBox<Error?>(nil)
    let done = DispatchSemaphore(value: 0)
    Thread {
      do { try request.complete(url: "unsupported://rewritten.json") }
      catch { answered.update { $0 = error } }
      done.signal()
    }.start()
    #expect(isSignalled(done))
    #expect(answered.value == nil)
    #expect(request.isClosed)
    await expectMaplibreError(.invalidState) {
      try request.complete(url: "unsupported://again.json")
    }
    // The online source cannot load the rewritten scheme.
    #expect(try await fixture.awaitEvent("the rewritten request to fail") {
      $0.type == .mapLoadingFailed
    } != nil)
  }
}

/// A provider's routes reach native as a copied array: the provider sees only
/// the requests they match, and a route that names only a URL pattern
/// matches every resource kind. A request outside the routes loads natively.
@Test func aProviderReceivesOnlyTheRequestsItsRoutesMatch() async throws {
  try await withMapFixture { fixture in
    // The sprite adds a sprite JSON and a sprite image request under the
    // routed prefix.
    let style = Data(#"""
    {"version":8,"sources":{},"layers":[],"sprite":"custom://routed/sprite"}
    """#.utf8)
    let seen = LockedBox<[ResourceRequestInfo]>([])
    try await fixture.runtime.setResourceProvider(ResourceProvider(
      callback: { request, handle in
        seen.update { $0.append(request) }
        do {
          try handle.complete(response: request.kind == .style
            ? ResourceResponse(status: .ok, bytes: style)
            : ResourceResponse(
              status: .error,
              errorReason: .notFound,
              errorMessage: "the test serves no sprite"
            ))
        } catch {
          Issue.record("provider failed: \(error)")
          handle.close()
        }
      },
      routes: [ResourceRoute(flags: .glob, url: "custom://routed/**")]
    ))

    try await fixture.map.setStyleUrl("custom://routed/style.json")
    await awaitCondition("the routed style and sprite requests") {
      Set(seen.value.map(\.kind))
        .isSuperset(of: [.style, .spriteJson, .spriteImage])
    }

    // Outside the routes, native loading has no transport for the scheme.
    try await fixture.map.setStyleUrl("custom://elsewhere/style.json")
    #expect(try await fixture.awaitEvent("the unrouted style to fail") {
      $0.type == .mapLoadingFailed && $0.message.contains("elsewhere")
    } != nil)
    #expect(seen.value.allSatisfy {
      $0.requestedUrl.hasPrefix("custom://routed/")
    })
  }
}
