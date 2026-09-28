@testable import MaplibreNativeFFI
import Testing

@Test func unknownNetworkStatusIsRejectedByNative() {
  do {
    try Maplibre.networkStatusSet(status: .init(rawValue: 999_999))
    Issue.record("unknown status should throw")
  } catch let error as MaplibreError {
    #expect(error.kind == .invalidArgument)
    #expect(error.rawStatus != nil)
  } catch {
    Issue.record("unexpected error: \(error)")
  }
}

@Test func networkStatusRoundTripsThroughNative() throws {
  let original = try Maplibre.networkStatusGet()
  try Maplibre.networkStatusSet(status: .offline)
  #expect(try Maplibre.networkStatusGet() == .offline)
  try Maplibre.networkStatusSet(status: original)
}
