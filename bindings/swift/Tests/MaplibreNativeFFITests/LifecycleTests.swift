import Foundation
@testable import MaplibreNativeFFI
import Testing

/// A handle describes itself by its type, id, and whether it is closed.
private func expectDescription(
  of description: String,
  typeName: String,
  id: UInt64,
  closed: Bool,
  sourceLocation: SourceLocation = #_sourceLocation
) {
  #expect(
    description ==
      "\(typeName)(id=0x\(String(id, radix: 16)), closed=\(closed))",
    sourceLocation: sourceLocation
  )
}

/// A close that is the handle's release is idempotent and marks it closed.
/// Handles compare by id, so distinct handles key a set apart.
@Test func aSecondCloseIsANoOpAndTheHandleReportsItIsClosed() async throws {
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
    #expect(data.id != 0)
    #expect(!data.isClosed)
    expectDescription(
      of: data.description,
      typeName: "GeojsonSourceDataHandle",
      id: data.id,
      closed: false
    )
    data.close()
    #expect(data.isClosed)
    data.close()
    #expect(data.isClosed)
    other.close()
    expectDescription(
      of: String(describing: data),
      typeName: "GeojsonSourceDataHandle",
      id: data.id,
      closed: true
    )

    let error = await expectMaplibreError(.invalidState) {
      try await fixture.map.addGeojsonSourceData("late", data: data)
    }
    #expect(error?.status == .invalidState)
  }
}

/// A dispose is idempotent and marks the handle closed, while an asynchronous
/// release after it reaches native, which reports the stale handle.
@Test func synchronousCloseIsIdempotentAndAsyncReleaseReachesNative(
) async throws {
  let runtime = try MapFixture.makeRuntime()
  let map = try await runtime.createMap(options: MapOptions(
    initialExtent: LogicalExtent(width: 8, height: 8, scaleFactor: 1)
  ))
  #expect(!map.isClosed)
  expectDescription(
    of: "\(map)",
    typeName: "MapHandle",
    id: map.id,
    closed: false
  )
  try map.dispose()
  #expect(map.isClosed)
  try map.dispose()
  expectDescription(
    of: "\(map)",
    typeName: "MapHandle",
    id: map.id,
    closed: true
  )
  let repeated = await expectMaplibreError(.invalidState) {
    try await map.close()
  }
  #expect(repeated?.status == .invalidState)

  // The runtime refuses its release until the disposed map retires.
  var teardown: NativeFuture<Void>?
  try await awaitCondition("the disposed map to retire") {
    do {
      teardown = try runtime.startClose()
      return true
    } catch let error as MaplibreError where error.status == .invalidState {
      return false
    }
  }
  #expect(runtime.isClosed)
  try await teardown?.value()
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
