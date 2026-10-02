import Foundation
import MaplibreNativeFFI
import Testing

// Exit tests run their body in a child process, which the simulator and Mac
// Catalyst runners cannot spawn.
#if os(macOS) || os(Linux)
  /// A process exits cleanly with its runtime and map live, its resource
  /// provider and log callback installed, and a style request in flight. The C
  /// API lets a host exit at any point, so nothing is closed first.
  @Test func exitingWithLiveHandlesAndCallbacksIsClean() async {
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
      // Keeps both handles referenced through the exit, so that neither one's
      // deinit retires it first.
      withExtendedLifetime((runtime, map)) { exit(0) }
    }
  }
#endif
