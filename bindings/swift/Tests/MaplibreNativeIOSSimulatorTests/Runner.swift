import Testing

@main
struct MaplibreNativeIOSSimulatorTestRunner {
  static func main() async {
    // swift test does not run iOS simulator bundles, so the simulator
    // executable delegates to SwiftPM's test entry point.
    await Testing.__swiftPMEntryPoint() as Never
  }
}
