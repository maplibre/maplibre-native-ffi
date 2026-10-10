import CMaplibreNativeC
import Foundation
@testable import MaplibreNativeFFI
import Testing

/// Adds a custom geometry source whose callback state reports its release
/// through `releases`.
private func addCustomSource(
  _ map: MapHandle,
  id: String,
  releases: LockedBox<Int>
) async throws -> CommandCompletion {
  let sentinel = ReleaseProbe(releases)
  return try await map.addCustomGeometrySource(
    sourceId: id,
    options: CustomGeometrySourceOptions(fetchTile: { _ in
      withExtendedLifetime(sentinel) {}
    })
  )
}

/// An accepted registration stays rooted while native holds it and is freed
/// when native releases it; a registration native rejects is freed by the
/// time a later barrier completes.
@Test func aRegistrationIsRootedUntilNativeReleasesIt() async throws {
  try await withMapFixture { fixture in
    try await fixture.map.setStyleJson(json: emptyStyle)
    let accepted = LockedBox(0)
    let added = try await addCustomSource(
      fixture.map,
      id: "custom",
      releases: accepted
    )
    #expect(added.disposition == .committed)

    // A second source under the same ID is rejected on the map thread.
    let rejected = LockedBox(0)
    let duplicate = try await addCustomSource(
      fixture.map,
      id: "custom",
      releases: rejected
    )
    #expect(duplicate.disposition == .failed)
    try await fixture.runtime.barrier()
    #expect(rejected.value == 1)
    #expect(accepted.value == 0)

    let removed = try await fixture.map.removeStyleSource(sourceId: "custom")
    #expect(removed.disposition == .committed)
    #expect(accepted.value == 1)
  }
}

/// Installs a provider for `url` that hands each of its requests to `body`
/// and returns `body`'s decision, and denies every other request.
func installProvider(
  on runtime: RuntimeHandle,
  for url: String,
  _ body: @escaping @Sendable (ResourceRequestHandle) throws
    -> ResourceProviderDecision
) async throws {
  let deny = denyingProvider()
  try await runtime.setResourceProvider(provider: ResourceProvider(
    callback: { request, handle in
      guard request.requestedUrl == url else {
        return try deny.callback!(request, handle)
      }
      return try body(handle)
    }
  ))
}

/// A cancel callback holds its request weakly, so a provider that drops the
/// request lets it go: the binding releases the request, and native then
/// releases the cancel registration.
@Test func aCancelCallbackDoesNotKeepItsRequestAlive() async throws {
  try await withMapFixture { fixture in
    let releases = LockedBox(0)
    let request = LockedBox<ResourceRequestHandle?>(nil)
    weak var weakRequest: ResourceRequestHandle?
    try await installProvider(
      on: fixture.runtime,
      for: "custom://dropped.json"
    ) { handle in
      _ = try handle.setCancelCallback(handler: ResourceRequestCancelHandler {
        [sentinel = ReleaseProbe(releases)] in withExtendedLifetime(sentinel) {}
      })
      request.update { $0 = handle }
      return .handle
    }
    try await fixture.map.setStyleUrl(url: "custom://dropped.json")
    await awaitCondition("the provider call") { request.value != nil }
    weakRequest = request.value
    request.update { $0 = nil }

    await awaitCondition("the cancel registration's release") {
      releases.value == 1
    }
    #expect(weakRequest == nil)
  }
}

/// Inside a provider callback, a call on another owner is forbidden and the
/// request's own calls are admitted.
@Test func aProviderCallbackAdmitsOnlyItsRequestsCalls() async throws {
  try await withMapFixture { fixture in
    let runtime = fixture.runtime
    let outcome = LockedBox<(forbidden: MaplibreError?, cancelled: Bool?)?>(nil)
    try await installProvider(
      on: runtime,
      for: "custom://admission.json"
    ) { handle in
      var forbidden: MaplibreError?
      do { _ = try runtime.getEventMask() }
      catch let error as MaplibreError { forbidden = error }
      let cancelled = try? handle.isCancelled()
      try handle.complete(response: ResourceResponse(
        status: .ok,
        bytes: emptyStyle
      ))
      outcome.update { $0 = (forbidden, cancelled) }
      return .handle
    }
    try await fixture.map.setStyleUrl(url: "custom://admission.json")
    #expect(try await fixture.awaitEvent("the provider-served style") {
      $0.type == .mapStyleLoaded
    } != nil)

    let result = try #require(outcome.value)
    #expect(result.forbidden?.kind == .invalidState)
    #expect(result.forbidden?.rawStatus == nil)
    #expect(result.cancelled == false)
    // Outside the callback, the same call is admitted again.
    #expect(try runtime.getEventMask() == .all)
  }
}

/// A provider that takes a request answers it later, from outside the
/// callback, and the request retires once the host releases it.
@Test func aProviderAnswersATakenRequestLater() async throws {
  try await withMapFixture { fixture in
    let taken = LockedBox<ResourceRequestHandle?>(nil)
    try await installProvider(
      on: fixture.runtime,
      for: "custom://later.json"
    ) { handle in
      taken.update { $0 = handle }
      return .handle
    }
    try await fixture.map.setStyleUrl(url: "custom://later.json")
    await awaitCondition("the provider to take the request") {
      taken.value != nil
    }
    let request = try #require(taken.value)
    #expect(!request.isClosed)

    try request.complete(response: ResourceResponse(
      status: .ok,
      bytes: emptyStyle
    ))
    #expect(try await fixture.awaitEvent("the style answered later") {
      $0.type == .mapStyleLoaded
    } != nil)
    try request.close()
    try request.waitUntilRetired()
    #expect(request.isClosed)
  }
}

/// A resource transform's response object works only on the callback's
/// thread and only until the callback returns.
@Test func aTransformResponseExpiresWithItsCallback() async throws {
  try await withMapFixture { fixture in
    // A transform applies to requests that reach the network file source, so
    // the provider passes this one through. The transform rewrites it to a
    // scheme no transport serves, so it fails without a connection.
    let url = "http://127.0.0.1:1/transformed.json"
    try await installProvider(on: fixture.runtime, for: url) { _ in
      .passThrough
    }
    let escaped = LockedBox<ResourceTransformResponse?>(nil)
    let otherThread = LockedBox<MaplibreError?>(nil)
    try await fixture.runtime.setResourceTransform(
      transform: ResourceTransform(callback: { _, requested, response in
        guard requested == url else { return }
        let done = DispatchSemaphore(value: 0)
        Thread {
          do { try response.setUrl(url: "custom://other-thread.json") }
          catch let error as MaplibreError {
            otherThread.update { $0 = error }
          } catch {}
          done.signal()
        }.start()
        _ = isSignalled(done)
        try response.setUrl(url: "custom://rewritten.json")
        escaped.update { $0 = response }
      })
    )
    try await fixture.map.setStyleUrl(url: url)
    let failure = try await fixture
      .awaitEvent("the rewritten request to fail") {
        $0.type == .mapLoadingFailed
      }
    #expect(failure != nil)

    #expect(otherThread.value?.kind == .invalidState)
    let response = try #require(escaped.value)
    await expectMaplibreError(.invalidState) {
      try response.setUrl(url: "custom://late.json")
    }
  }
}

/// A cancel callback registered on a request native already cancelled is
/// rejected through the registration's cancelled output, and the binding
/// frees it at once rather than rooting it.
@Test func aRegistrationOnACancelledRequestIsNotRooted() async throws {
  try await withMapFixture { fixture in
    let taken = LockedBox<ResourceRequestHandle?>(nil)
    try await installProvider(
      on: fixture.runtime,
      for: "custom://cancelled.json"
    ) { handle in
      taken.update { $0 = handle }
      return .handle
    }
    try await fixture.map.setStyleUrl(url: "custom://cancelled.json")
    await awaitCondition("the provider to take the request") {
      taken.value != nil
    }
    let request = try #require(taken.value)
    // Closing the map discards its pending style request.
    try await fixture.map.close()
    try await awaitCondition("the request's cancellation") {
      try request.isCancelled()
    }

    let releases = LockedBox(0)
    let cancelled = try request.setCancelCallback(
      handler: ResourceRequestCancelHandler {
        [sentinel = ReleaseProbe(releases)] in withExtendedLifetime(sentinel) {}
      }
    )
    #expect(cancelled)
    #expect(releases.value == 1)
    try request.close()
    #expect(releases.value == 1)
  }
}
