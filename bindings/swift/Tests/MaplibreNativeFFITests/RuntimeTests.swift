import CMaplibreNativeC
import Foundation
@testable import MaplibreNativeFFI
import Testing

@Test func runtimeCreateRunDrainAndClose() async throws {
  let runtime =
    try Maplibre.runtimeCreate(options: RuntimeOptions(cachePath: ":memory:"))
  try await runtime.barrier()
  _ = try runtime.drainEventCopies()
  try await runtime.close()

  #expect(runtime.isClosed)
}

@Test func abandonedMapResultReleasesNativeOwnership() async throws {
  let runtime =
    try Maplibre.runtimeCreate(options: RuntimeOptions(cachePath: ":memory:"))
  _ = try await runtime
    .mapCreate(options: MapOptions(initialExtent: LogicalExtent(
      width: 64,
      height: 64,
      scaleFactor: 1
    )))
  try await runtime.barrier()
  try await runtime.close()
  #expect(runtime.isClosed)
}

@Test func runtimeResourceTransformCanInstallAndClear() async throws {
  let runtime =
    try Maplibre.runtimeCreate(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }

  try await runtime
    .setResourceTransform(
      transform: ResourceTransform(callback: { _, url, response in
        try response.setUrl(url: url.replacingOccurrences(
          of: "example.test",
          with: "example.invalid"
        ))
      })
    )
  try await runtime.clearResourceTransform()
}

/// Requests a style URL whose scheme no file source serves, drains until the
/// matching loading failure arrives, and returns its message. The failure
/// proves the request reached the network file source, where the
/// runtime-scoped resource provider applies.
private func loadProbeStyle(
  runtime: RuntimeHandle,
  map: MapHandle,
  styleURL: String
) async throws -> String? {
  _ = try await map.setStyleUrl(url: styleURL)
  return try await drainUntilEvent(
    runtime,
    waitingFor: "a loading failure for \(styleURL)"
  ) { $0.type == .mapLoadingFailed && $0.message.contains(styleURL) }?.message
}

@Test func runtimeResourceProviderIsConsultedUntilReplacedAndCleared(
) async throws {
  let runtime =
    try Maplibre.runtimeCreate(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }
  let map = try await runtime
    .mapCreate(options: MapOptions(initialExtent: LogicalExtent(
      width: 64,
      height: 64,
      scaleFactor: 1
    )))
  defer { try? map.closeBlockingForTests() }

  let firstCalls = LockedBox(0)
  try await runtime
    .setResourceProvider(provider: ResourceProvider(callback: { _, _ in
      firstCalls.update { $0 += 1 }
      return .passThrough
    }))

  let firstFailure = try await loadProbeStyle(
    runtime: runtime,
    map: map,
    styleURL: "jar:file:/packaged/first.json"
  )
  #expect(firstFailure?.contains("\"jar\"") == true)
  #expect(firstCalls.value > 0)

  // The previous provider stops being consulted once the call returns.
  let secondCalls = LockedBox(0)
  try await runtime
    .setResourceProvider(provider: ResourceProvider(callback: { _, _ in
      secondCalls.update { $0 += 1 }
      return .passThrough
    }))
  let firstCallsAfterReplace = firstCalls.value

  let secondFailure = try await loadProbeStyle(
    runtime: runtime,
    map: map,
    styleURL: "jar:file:/packaged/second.json"
  )
  #expect(secondFailure?.contains("\"jar\"") == true)
  #expect(secondCalls.value > 0)
  #expect(firstCalls.value == firstCallsAfterReplace)

  try await runtime.clearResourceProvider()
  let secondCallsAfterClear = secondCalls.value

  let clearedFailure = try await loadProbeStyle(
    runtime: runtime,
    map: map,
    styleURL: "jar:file:/packaged/third.json"
  )
  #expect(clearedFailure?.contains("\"jar\"") == true)
  #expect(firstCalls.value == firstCallsAfterReplace)
  #expect(secondCalls.value == secondCallsAfterClear)

  // Clearing an already cleared provider stays a successful no-op.
  try await runtime.clearResourceProvider()
}

private let providerStyleJSON = #"{"version":8,"sources":{},"layers":[]}"#

/// the default tile server's `maplibre:` scheme alias reaches the
/// provider as the alias, alongside the URL the built-in network path would
/// have fetched.
@Test func resourceProviderSeesSchemeAliasAndItsResolvedURL() async throws {
  let runtime =
    try Maplibre.runtimeCreate(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }

  let resolved = LockedBox<String?>(nil)
  try await runtime
    .setResourceProvider(
      provider: ResourceProvider(callback: { request, handle in
        guard request.requestedUrl == "maplibre://maps/style" else {
          return .passThrough
        }
        resolved.update { $0 = request.resolvedUrl }
        try? handle.complete(response: ResourceResponse(
          status: .ok,
          bytes: Data(providerStyleJSON.utf8)
        ))
        return .handle
      })
    )

  let map = try await runtime
    .mapCreate(options: MapOptions(initialExtent: LogicalExtent(
      width: 64,
      height: 64,
      scaleFactor: 1
    )))
  defer { try? map.closeBlockingForTests() }

  try await map.setStyleUrl(url: "maplibre://maps/style")
  let loaded = try await drainUntilEvent(
    runtime,
    waitingFor: "the provider-served style to load"
  ) { $0.type == .mapStyleLoaded }

  #expect(loaded != nil)
  #expect(resolved.value == "https://demotiles.maplibre.org/style.json")
}

private final class CancelProbe: @unchecked Sendable {
  private let lock = NSLock()
  private var storedHandle: ResourceRequestHandle?
  private var cancelCount = 0
  private var providerCalled = false

  func store(_ handle: ResourceRequestHandle) {
    lock.withLock { storedHandle = handle }
  }

  /// Marks the provider callback finished, after `configure` has run, so a
  /// test that waits on this flag also sees everything `configure` recorded.
  func finishProviderCall() {
    lock.withLock { providerCalled = true }
  }

  var handle: ResourceRequestHandle? {
    lock.withLock { storedHandle }
  }

  var wasProviderCalled: Bool {
    lock.withLock { providerCalled }
  }

  func recordCancel() {
    lock.withLock { cancelCount += 1 }
  }

  var cancels: Int {
    lock.withLock { cancelCount }
  }
}

/// Installs a provider that takes every request and keeps the handle, then
/// requests a style through it. `configure` runs inside the provider callback
/// with the handle, before the provider returns. The request stays open unless
/// `configure` completes it.
private func startCancelProbeRequest(
  runtime: RuntimeHandle,
  map: MapHandle,
  probe: CancelProbe,
  configure: @escaping @Sendable (ResourceRequestHandle) -> Void = { _ in }
) async throws -> Bool {
  try await runtime
    .setResourceProvider(provider: ResourceProvider(callback: { _, handle in
      probe.store(handle)
      configure(handle)
      probe.finishProviderCall()
      return .handle
    }))
  try await map.setStyleUrl(url: "custom://cancel-style.json")
  return try await waitUntilTrue(
    "the provider to take the request"
  ) { probe.wasProviderCalled }
}

/// closing a map discards its pending style request, and MapLibre
/// runs the registered cancel callback once for the request the provider still
/// holds. A second registration reports invalid state and leaves the first in
/// place, and a closed request rejects a registration.
@Test func resourceRequestCancelCallbackRunsWhenTheMapDiscardsTheRequest(
) async throws {
  let runtime =
    try Maplibre.runtimeCreate(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }
  let map = try await runtime
    .mapCreate(options: MapOptions(initialExtent: LogicalExtent(
      width: 64,
      height: 64,
      scaleFactor: 1
    )))
  defer { try? map.closeBlockingForTests() }

  let probe = CancelProbe()
  let secondCalls = LockedBox(0)
  let secondRegistration = LockedBox<Result<Bool, Error>?>(nil)
  let configure: @Sendable (ResourceRequestHandle) -> Void = { handle in
    try? handle.setCancelCallback { probe.recordCancel() }
    secondRegistration.update {
      $0 = Result {
        try handle.setCancelCallback { secondCalls.update { $0 += 1 } }
        return true
      }
    }
  }
  #expect(try await startCancelProbeRequest(
    runtime: runtime,
    map: map,
    probe: probe,
    configure: configure
  ))
  #expect(probe.cancels == 0)
  switch secondRegistration.value {
  case let .failure(error as MaplibreError):
    #expect(error.kind == .invalidState)
  case let other:
    Issue
      .record(
        "a second registration should report invalid state: \(other.debugDescription)"
      )
  }

  // Closing the map waits for its native teardown, and that teardown is what
  // discards the request and runs the cancel callback.
  try await map.close()
  #expect(try await waitUntilTrue("the cancel callback") {
    probe.cancels == 1
  })
  try await runtime.barrier()
  #expect(probe.cancels == 1)
  #expect(secondCalls.value == 0)

  let handle = try #require(probe.handle)
  #expect(try handle.cancelled())
  #expect(throws: MaplibreError.self) {
    try handle.complete(response: ResourceResponse(
      status: .ok,
      bytes: emptyStyleJSON
    ))
  }

  try handle.close()
  do {
    try handle.setCancelCallback { probe.recordCancel() }
    Issue.record("a closed request should reject a cancel callback")
  } catch let error as MaplibreError {
    #expect(error.diagnostic.contains("closed"))
  }
  #expect(probe.cancels == 1)
}

/// the cancel callback may close its own request. Native release
/// returns at once from inside the callback instead of waiting for it.
@Test func resourceRequestCancelCallbackMayCloseTheRequest() async throws {
  let runtime =
    try Maplibre.runtimeCreate(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }
  let map = try await runtime
    .mapCreate(options: MapOptions(initialExtent: LogicalExtent(
      width: 64,
      height: 64,
      scaleFactor: 1
    )))
  defer { try? map.closeBlockingForTests() }

  let probe = CancelProbe()
  #expect(try await startCancelProbeRequest(
    runtime: runtime,
    map: map,
    probe: probe
  ) { handle in
    try? handle.setCancelCallback {
      probe.recordCancel()
      try handle.close()
    }
  })

  try await map.close()
  #expect(try await waitUntilTrue("the cancel callback") {
    probe.cancels == 1
  })

  let handle = try #require(probe.handle)
  do {
    try handle.setCancelCallback { probe.recordCancel() }
    Issue.record("a closed request should reject a cancel callback")
  } catch let error as MaplibreError {
    #expect(error.diagnostic.contains("closed"))
  }
  #expect(probe.cancels == 1)
}

@Test func resourceRequestRegistrationReportsAlreadyCancelled() async throws {
  let runtime = try Maplibre
    .runtimeCreate(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }
  let map = try await runtime
    .mapCreate(options: MapOptions(initialExtent: LogicalExtent(
      width: 64,
      height: 64,
      scaleFactor: 1
    )))
  let probe = CancelProbe()
  #expect(try await startCancelProbeRequest(
    runtime: runtime,
    map: map,
    probe: probe
  ))
  let handle = try #require(probe.handle)
  defer { try? handle.close() }
  try await map.close()
  #expect(try await waitUntilTrue("request cancellation") {
    try handle.cancelled()
  })
  #expect(try handle.setCancelCallback { probe.recordCancel() })
  #expect(probe.cancels == 0)
}

/// MapLibre retires a request the provider answered, and that
/// teardown leaves the cancel callback alone.
@Test func resourceRequestCancelCallbackSkipsACompletedRequest() async throws {
  let runtime =
    try Maplibre.runtimeCreate(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }
  let map = try await runtime
    .mapCreate(options: MapOptions(initialExtent: LogicalExtent(
      width: 64,
      height: 64,
      scaleFactor: 1
    )))
  defer { try? map.closeBlockingForTests() }

  let probe = CancelProbe()
  #expect(try await startCancelProbeRequest(
    runtime: runtime,
    map: map,
    probe: probe
  ) { handle in
    try? handle.setCancelCallback { probe.recordCancel() }
    try? handle.complete(response: ResourceResponse(
      status: .ok,
      bytes: Data(providerStyleJSON.utf8)
    ))
  })
  #expect(try await drainUntilEvent(
    runtime,
    waitingFor: "the provider-served style"
  ) { $0.type == .mapStyleLoaded } != nil)

  try await map.close()
  try await runtime.barrier()

  #expect(probe.cancels == 0)
  // Completion keeps ownership until explicit close.
  let handle = try #require(probe.handle)
  #expect(!handle.isClosed)
  try handle.close()
  try handle.waitUntilRetired()
}

@Test func providerRejectedConversionCanRetryAndAcceptedCompletionClaimsOwnership(
) async throws {
  let runtime = try Maplibre
    .runtimeCreate(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }
  let map = try await runtime
    .mapCreate(options: MapOptions(initialExtent: LogicalExtent(
      width: 8,
      height: 8,
      scaleFactor: 1
    )))
  defer { try? map.closeBlockingForTests() }
  let escaped = LockedBox<ResourceRequestHandle?>(nil)
  try await runtime
    .setResourceProvider(
      provider: ResourceProvider(callback: { request, handle in
        #expect(request.requestedUrl == "custom://retry.json")
        #expect(throws: MaplibreError.self) { try runtime.getEventMask() }
        #expect(throws: MaplibreError.self) { try handle.waitUntilRetired() }
        #expect(throws: MaplibreError.self) {
          try handle.complete(response: ResourceResponse(
            status: .ok,
            etag: "invalid\0etag"
          ))
        }
        #expect(!handle.isClosed)
        try handle.complete(response: ResourceResponse(
          status: .ok,
          bytes: emptyStyleJSON
        ))
        escaped.update { $0 = handle }
        return .passThrough
      })
    )
  try await map.setStyleUrl(url: "custom://retry.json")
  #expect(try await drainUntilEvent(
    runtime,
    waitingFor: "retried provider response"
  ) { $0.type == .mapStyleLoaded } != nil)
  let handle = try #require(escaped.value)
  #expect(!handle.isClosed)
  try handle.close()
  try handle.waitUntilRetired()
  #expect(handle.isClosed)
}

@Test func providerPassThroughInvalidatesAnEscapedOwner() async throws {
  let runtime = try Maplibre
    .runtimeCreate(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }
  let map = try await runtime
    .mapCreate(options: MapOptions(initialExtent: LogicalExtent(
      width: 8,
      height: 8,
      scaleFactor: 1
    )))
  defer { try? map.closeBlockingForTests() }
  let escaped = LockedBox<ResourceRequestHandle?>(nil)
  try await runtime
    .setResourceProvider(provider: ResourceProvider(callback: { _, handle in
      escaped.update { $0 = handle }
      return .passThrough
    }))
  #expect(try await loadProbeStyle(
    runtime: runtime,
    map: map,
    styleURL: "jar:file:/passthrough.json"
  ) != nil)
  let handle = try #require(escaped.value)
  #expect(handle.isClosed)
  #expect(throws: MaplibreError.self) { try handle.cancelled() }
}
