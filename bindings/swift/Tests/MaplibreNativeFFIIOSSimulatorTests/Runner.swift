import MaplibreNativeFFITestCases
import Testing

@main
struct MaplibreNativeFFIIOSSimulatorTestRunner {
  static func main() async {
    // swift test does not run iOS or tvOS simulator bundles, nor Mac Catalyst
    // ones, so this executable delegates to SwiftPM's test entry point.
    await Testing.__swiftPMEntryPoint() as Never
  }
}
