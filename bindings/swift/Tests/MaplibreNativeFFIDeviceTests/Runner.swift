import MaplibreNativeFFITestCases
import Testing

@main
struct MaplibreNativeFFIDeviceTestRunner {
  static func main() async {
    // swift test does not run cross-compiled device bundles, so
    // this executable delegates to SwiftPM's test entry point.
    await Testing.__swiftPMEntryPoint() as Never
  }
}
