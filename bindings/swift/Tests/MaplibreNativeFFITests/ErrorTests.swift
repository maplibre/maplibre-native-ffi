import CMaplibreNativeC
@testable import MaplibreNativeFFI
import Testing

/// Every C status maps to its error kind with its raw code and diagnostic, a
/// status this version does not name keeps its code, and a failure the
/// binding raises itself carries no native status.
@Test func aFailedCallMapsItsStatusAndCarriesItsDiagnostic() async throws {
  let kinds: [(Int32, MaplibreErrorKind)] = [
    (MLN_STATUS_INVALID_ARGUMENT.rawValue, .invalidArgument),
    (MLN_STATUS_INVALID_STATE.rawValue, .invalidState),
    (MLN_STATUS_WRONG_THREAD.rawValue, .wrongThread),
    (MLN_STATUS_UNSUPPORTED.rawValue, .unsupported),
    (MLN_STATUS_NATIVE_ERROR.rawValue, .nativeError),
    (MLN_STATUS_CANCELLED.rawValue, .cancelled),
    (MLN_STATUS_BUSY.rawValue, .busy),
    (MLN_STATUS_TARGET_LOST.rawValue, .targetLost),
    (MLN_STATUS_NOT_READY.rawValue, .notReady),
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

    // The next call on the thread succeeds and carries nothing over.
    let mask = try fixture.runtime.getEventMask()
    #expect(mask == .all)

    let binding = await expectMaplibreError(.invalidState) {
      try await fixture.map.close()
      return try fixture.map.snapshotGet()
    }
    #expect(binding?.rawStatus == nil)
    #expect(binding?.diagnostic == "MapHandle is closed")
  }
}
