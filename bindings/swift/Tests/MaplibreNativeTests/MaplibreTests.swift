@testable import MaplibreNative
import Testing

@Test func cVersionIsReadable() {
  #expect(Maplibre.cVersion() == 0)
}

@Test func supportedBackendsPreserveMaskBits() {
  let backends = Maplibre.supportedRenderBackends()
  #expect(!backends.isEmpty)
}

@Test func supportedOpenGLContextProvidersPreserveMaskBits() {
  let providers = Maplibre.supportedOpenGLContextProviders()
  #expect(providers.isSubset(of: [.wgl, .egl]))
}

@Test func unknownNetworkStatusIsRejectedBeforeCallingC() {
  do {
    try Maplibre.setNetworkStatus(.unknown(999_999))
    Issue.record("unknown status should throw")
  } catch let error as MaplibreError {
    #expect(error.kind == .invalidArgument)
    #expect(error.rawStatus == nil)
  } catch {
    Issue.record("unexpected error: \(error)")
  }
}

@Test func networkStatusRoundTripsThroughNative() throws {
  let original = try Maplibre.networkStatus()
  try Maplibre.setNetworkStatus(.offline)
  #expect(try Maplibre.networkStatus() == .offline)
  try Maplibre.setNetworkStatus(original)
}
