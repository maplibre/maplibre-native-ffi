import Foundation
@testable import MaplibreNativeFFI
import Testing

/// A style with no sources, so a load finishes without a request.
let emptyStyle = Data(#"{"version":8,"sources":{},"layers":[]}"#.utf8)

/// A style that paints every pixel red, so a readback shows that a frame
/// rendered.
let redStyle = Data(##"""
{"version":8,"sources":{},"layers":[{"id":"background",
"type":"background","paint":{"background-color":"#ff0000"}}]}
"""##.utf8)

/// A resource provider that answers the URLs in `routes` and fails every other
/// request as not found, so no test reaches the network.
func denyingProvider(routes: [String: Data] = [:]) -> ResourceProvider {
  ResourceProvider { request, handle in
    let url = request.requestedUrl ?? ""
    if let bytes = routes[url] {
      try handle.complete(response: ResourceResponse(status: .ok, bytes: bytes))
    } else {
      try handle.complete(response: ResourceResponse(
        status: .error,
        errorReason: .notFound,
        errorMessage: "the test fixture serves no \(url)"
      ))
    }
    try handle.close()
    return .handle
  }
}

/// One runtime and one map, the fixture every test that needs native objects
/// starts from. The runtime's event wake pulses, so event waits wake as soon as
/// the runtime queues an event, and its resource provider denies every request
/// it has no route for.
final class MapFixture: Sendable {
  let runtime: RuntimeHandle
  let map: MapHandle

  private init(runtime: RuntimeHandle, map: MapHandle) {
    self.runtime = runtime
    self.map = map
  }

  static func make(
    extent: LogicalExtent = LogicalExtent(
      width: 32,
      height: 32,
      scaleFactor: 1
    ),
    routes: [String: Data] = [:]
  ) async throws -> MapFixture {
    let runtime = try makeRuntime()
    do {
      try await runtime
        .setResourceProvider(provider: denyingProvider(routes: routes))
      let map = try await runtime
        .createMap(options: MapOptions(initialExtent: extent))
      return MapFixture(runtime: runtime, map: map)
    } catch {
      try? await runtime.close()
      throw error
    }
  }

  /// A runtime whose event wake pulses, for a test that manages its own maps.
  static func makeRuntime() throws -> RuntimeHandle {
    try Maplibre.runtimeCreate(options: RuntimeOptions(
      cachePath: ":memory:",
      eventWake: Wake(callback: { Pulse.shared.signal() })
    ))
  }

  /// Closes the map, if the test left it open, and then the runtime, waiting
  /// for native teardown so no native thread outlives the test.
  func close() async {
    do { try await map.close() } catch {
      Issue.record("closing the fixture map failed: \(error)")
    }
    do { try await runtime.close() } catch {
      Issue.record("closing the fixture runtime failed: \(error)")
    }
  }

  /// Drains events until one that `isMatch` accepts arrives, and returns it.
  /// Records an issue naming `subject` and returns nil at the deadline.
  func awaitEvent(
    _ subject: String,
    sourceLocation: SourceLocation = #_sourceLocation,
    where isMatch: (RuntimeEvent) -> Bool
  ) async throws -> RuntimeEvent? {
    var found: RuntimeEvent?
    try await awaitCondition(subject, sourceLocation: sourceLocation) {
      found = try runtime.drainEventCopies().first(where: isMatch)
      return found != nil
    }
    return found
  }
}

/// Runs `body` with a fresh fixture and closes it afterwards, whether or not
/// `body` throws.
func withMapFixture<Result>(
  extent: LogicalExtent = LogicalExtent(width: 32, height: 32, scaleFactor: 1),
  routes: [String: Data] = [:],
  _ body: (MapFixture) async throws -> Result
) async throws -> Result {
  let fixture = try await MapFixture.make(extent: extent, routes: routes)
  do {
    let result = try await body(fixture)
    await fixture.close()
    return result
  } catch {
    await fixture.close()
    throw error
  }
}

extension RuntimeHandle {
  /// Drains the queued events into copies that outlive the batch.
  func drainEventCopies() throws -> [RuntimeEvent] {
    guard let batch = try drainEvents() else { return [] }
    defer { try? batch.close() }
    return try batch.get().events
  }
}

/// Expects `body` to throw a ``MaplibreError`` of `kind`, and returns it.
@discardableResult
func expectMaplibreError(
  _ kind: MaplibreErrorKind,
  sourceLocation: SourceLocation = #_sourceLocation,
  _ body: () async throws -> some Any
) async -> MaplibreError? {
  do {
    _ = try await body()
    Issue.record("expected a \(kind) error", sourceLocation: sourceLocation)
  } catch let error as MaplibreError {
    #expect(error.kind == kind, sourceLocation: sourceLocation)
    return error
  } catch {
    Issue.record("expected a MaplibreError, got \(error)",
                 sourceLocation: sourceLocation)
  }
  return nil
}
