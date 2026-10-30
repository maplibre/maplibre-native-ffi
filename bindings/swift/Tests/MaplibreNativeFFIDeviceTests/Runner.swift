import MaplibreNativeFFITestCases
import Testing
#if os(Windows)
  import Foundation
#endif

@main
struct MaplibreNativeFFIDeviceTestRunner {
  #if os(Windows)
    static func main() {
      // Swift's async main exits its initial thread through libdispatch. Wine
      // deadlocks when that thread's FLS cleanup starts a replacement worker.
      // Keep it alive while servicing the main actor instead.
      Task { await Testing.__swiftPMEntryPoint() as Never }
      while true {
        RunLoop.current.run(until: Date(timeIntervalSinceNow: 0.1))
      }
    }
  #else
    static func main() async {
      // swift test does not run cross-compiled device bundles, so
      // this executable delegates to SwiftPM's test entry point.
      await Testing.__swiftPMEntryPoint() as Never
    }
  #endif
}
