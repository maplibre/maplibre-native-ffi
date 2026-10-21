import MaplibreNativeC
@testable import MaplibreNativeFFI
import Testing

/// Errors preserve the generated status, including unknown values, and the
/// diagnostic.
@Test func aFailedCallMapsItsStatusAndCarriesItsDiagnostic() async throws {
  for status: Int32 in [-1, -10, -99] {
    let error = MaplibreError.fromNativeFailure(NativeStatusFailure(
      rawStatus: status,
      diagnostic: "diagnostic"
    ))
    #expect(error.status.rawValue == status)
    #expect(error.diagnostic == "diagnostic")
  }

  try await withMapFixture { fixture in
    let native = await expectMaplibreError(.invalidArgument) {
      try fixture.runtime
        .setEventMask(mask: RuntimeEventMask(rawValue: 1 << 63))
    }
    #expect(native?.status == .invalidArgument)
    #expect(native?.diagnostic.isEmpty == false)
    #expect(native?.description
      .hasPrefix("INVALID_ARGUMENT: ") == true)

    let binding = await expectMaplibreError(.invalidState) {
      try await fixture.map.close()
      return try fixture.map.getSnapshot()
    }
    #expect(binding?.status == .invalidState)
    #expect(binding?.diagnostic.isEmpty == false)
  }
}

/// A library that reports another C ABI version yields the binding's mismatch
/// error, which carries no native status, and the loaded library's own version
/// passes the check that every receiverless entry point runs first.
@Test func aLibraryWithAnotherCAbiVersionIsRejected() throws {
  let error = try #require(NativeAbi.mismatch(actual: expectedCAbiVersion + 1))
  #expect(error.expected == expectedCAbiVersion)
  #expect(error.actual == expectedCAbiVersion + 1)

  #expect(NativeAbi.mismatch(actual: expectedCAbiVersion) == nil)
  try NativeAbi.ensureCompatible()
}
