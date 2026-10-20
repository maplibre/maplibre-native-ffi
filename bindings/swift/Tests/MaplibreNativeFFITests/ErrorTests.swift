import MaplibreNativeC
@testable import MaplibreNativeFFI
import Testing

/// A C status maps to its error kind with its raw code and diagnostic, a
/// status this version does not name keeps its code, and a failure the
/// binding raises itself carries no native status.
@Test func aFailedCallMapsItsStatusAndCarriesItsDiagnostic() async throws {
  let kinds: [(Int32, MaplibreErrorKind)] = [
    (MLN_STATUS_INVALID_ARGUMENT.rawValue, .invalidArgument),
    (MLN_STATUS_NOT_FOUND.rawValue, .notFound),
    (-99, .unknownStatus),
  ]
  for (status, kind) in kinds {
    let error = MaplibreError.fromNativeFailure(NativeStatusFailure(
      rawStatus: status,
      diagnostic: "status \(status)"
    ))
    #expect(error.kind == kind)
    #expect(error.rawStatus == status)
    #expect(error.diagnostic == "status \(status)")
  }

  try await withMapFixture { fixture in
    let native = await expectMaplibreError(.invalidArgument) {
      try fixture.runtime
        .setEventMask(mask: RuntimeEventMask(rawValue: 1 << 63))
    }
    #expect(native?.rawStatus == MLN_STATUS_INVALID_ARGUMENT.rawValue)
    #expect(native?.diagnostic.isEmpty == false)
    #expect(native?.description
      .hasPrefix("MapLibre Native status -1: ") == true)

    let binding = await expectMaplibreError(.invalidState) {
      try await fixture.map.close()
      return try fixture.map.getSnapshot()
    }
    #expect(binding?.rawStatus == nil)
    #expect(binding?.diagnostic == "MapHandle is closed")
  }
}

/// A library that reports another C ABI version yields the binding's mismatch
/// error, which carries no native status, and the loaded library's own version
/// passes the check that every receiverless entry point runs first.
@Test func aLibraryWithAnotherCAbiVersionIsRejected() throws {
  let error = try #require(NativeAbi.mismatch(actual: expectedCAbiVersion + 1))
  #expect(error.kind == .abiVersionMismatch)
  #expect(error.rawStatus == nil)
  #expect(error.diagnostic.contains("expected \(expectedCAbiVersion)"))

  #expect(NativeAbi.mismatch(actual: expectedCAbiVersion) == nil)
  try NativeAbi.ensureCompatible()
}
