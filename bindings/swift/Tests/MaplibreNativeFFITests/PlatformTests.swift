import Foundation
import MaplibreNativeFFI
import Testing

// Exit tests run their body in a child process, which the simulator and Mac
// Catalyst runners cannot spawn.
#if os(macOS) || os(Linux)
  /// A process exits cleanly once it has waited for its runtime's teardown,
  /// with its log callback still installed, its resource provider never
  /// cleared, and its map closed with a style request in flight. The C API
  /// names that wait as the point after which a host may exit.
  @Test func exitingAfterRuntimeTeardownIsClean() async {
    await #expect(processExitsWith: .success) {
      try Maplibre.logSetCallback { _, _, _, _ in 1 }
      let runtime = try MapFixture.makeRuntime()
      try await runtime.setResourceProvider(provider: denyingProvider(
        routes: ["custom://exit.json": emptyStyle]
      ))
      let map = try await runtime.mapCreate(options: MapOptions(
        initialExtent: LogicalExtent(width: 8, height: 8, scaleFactor: 1)
      ))
      try await map.setStyleUrl(url: "custom://exit.json")
      try await map.close()
      try await runtime.close()
      exit(0)
    }
  }
#endif
