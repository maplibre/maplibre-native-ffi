@testable import MaplibreNativeFFI
import Testing

@Test func loadingMissingPluginLibraryThrowsNativeError() {
  do {
    try Maplibre.loadPlugin(
      path: "missing/maplibre-plugin.dylib",
      entryPoint: "missing_plugin_register"
    )
    Issue.record("loading a missing plugin library should throw")
  } catch let error as MaplibreError {
    #expect(error.kind == .nativeError)
    #expect(!error.diagnostic.isEmpty)
  } catch {
    Issue.record("unexpected error: \(error)")
  }
}
