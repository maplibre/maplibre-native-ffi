import Foundation
import MaplibreNativeFFI
import Testing

/// Synchronous close is idempotent; asynchronous release reaches native.
/// Handles compare by id, so distinct handles key a set apart.
@Test func synchronousCloseIsIdempotentAndAsyncReleaseReachesNative(
) async throws {
  try await withMapFixture { fixture in
    try await fixture.map.setStyleJson(emptyStyle)
    let data = try Maplibre.geojsonSourceDataCreate(
      Data(#"{"type":"FeatureCollection","features":[]}"#.utf8)
    )
    let other = try Maplibre
      .geojsonSourceDataCreate(
        Data(#"{"type":"FeatureCollection","features":[]}"#
          .utf8)
      )
    #expect(Set([data, data, other]).count == 2)
    data.close()
    data.close()
    other.close()

    let error = await expectMaplibreError(.invalidState) {
      try await fixture.map.addGeojsonSourceData("late", data: data)
    }
    #expect(error?.status == .invalidState)
    try await fixture.map.close()
    let repeated = await expectMaplibreError(.invalidState) {
      try await fixture.map.close()
    }
    #expect(repeated?.status == .invalidState)
  }
}

/// A handle's id is the source that its events carry, and it stays readable
/// after close, so a host can route events that it drains later. The id also
/// names the handle in its description.
@Test func aHandleIdNamesItsEventsAndOutlivesClose() async throws {
  try await withMapFixture { fixture in
    try await fixture.map.setStyleJson(emptyStyle)
    let loaded = try await fixture.awaitEvent("the style load") {
      $0.type == .mapStyleLoaded
    }
    #expect(loaded?.source == fixture.map.id)
    let hex = String(fixture.map.id, radix: 16)
    #expect(fixture.map.description == "MapHandle(id=0x\(hex), closed=false)")
    try await fixture.map.close()
    #expect(fixture.map.id == loaded?.source)
    #expect(fixture.map.description == "MapHandle(id=0x\(hex), closed=true)")
  }
}

/// Native refuses to release a runtime that still has a map, and the refusal
/// leaves the runtime usable, so the host closes the map and then the runtime.
@Test func aRefusedCloseLeavesTheRuntimeUsableAndALaterCloseSucceeds(
) async throws {
  let fixture = try await MapFixture.make()
  let refusal = await expectMaplibreError(.invalidState) {
    try await fixture.runtime.close()
  }
  #expect(refusal?.status == .invalidState)
  try await fixture.runtime.clearResourceTransform()

  try await fixture.map.close()
  try await fixture.runtime.close()
}

/// A map holds its runtime, so a host may drop its own runtime reference
/// while the map is live. The runtime goes once the map does.
@Test func aMapKeepsItsRuntimeAlive() async throws {
  var runtime: RuntimeHandle? = try MapFixture.makeRuntime()
  weak let weakRuntime = runtime
  var map: MapHandle? = try await runtime?.createMap(options: MapOptions(
    initialExtent: LogicalExtent(width: 8, height: 8, scaleFactor: 1)
  ))
  runtime = nil
  #expect(weakRuntime != nil)
  _ = try await map?.setStyleJson(emptyStyle)

  try await map?.close()
  map = nil
  #expect(weakRuntime == nil)
}
