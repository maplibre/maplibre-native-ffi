import MaplibreNativeFFITestCases
import Testing

@main
struct MaplibreNativeFFIIOSSimulatorTestRunner {
  static func main() async {
    // swift test does not run iOS, tvOS simulator, or Mac Catalyst bundles, so
    // this executable delegates to SwiftPM's test entry point.
    await Testing.__swiftPMEntryPoint() as Never
  }
}
