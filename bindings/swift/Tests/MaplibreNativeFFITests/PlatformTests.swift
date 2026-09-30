import Foundation
import MaplibreNativeFFI
import Testing

// Exit tests run their body in a child process, which the simulator and Mac
// Catalyst runners cannot spawn.
#if os(macOS) || os(Linux)
  /// A process that exits while its runtime, map, resource provider, and log
  /// callback are all live, and a style request is in flight, exits cleanly.
  @Test func exitingWithLiveHandlesAndCallbacksIsClean() async {
    await #expect(processExitsWith: .success) {
      let runtime = try MapFixture.makeRuntime()
      try Maplibre.logSetCallback { _, _, _, _ in 1 }
      try await runtime.setResourceProvider(provider: denyingProvider(
        routes: ["custom://exit.json": emptyStyle]
      ))
      let map = try await runtime.mapCreate(options: MapOptions(
        initialExtent: LogicalExtent(width: 8, height: 8, scaleFactor: 1)
      ))
      try await map.setStyleUrl(url: "custom://exit.json")
      withExtendedLifetime((runtime, map)) {}
      exit(0)
    }
  }
#endif
