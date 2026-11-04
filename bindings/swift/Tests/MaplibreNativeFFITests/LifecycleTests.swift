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

/// A synchronous release is idempotent and marks the handle closed.
/// Handles compare by id, so distinct handles key a set apart.
@Test func aSecondReleaseIsANoOpAndTheHandleReportsItIsClosed() async throws {
  try await withMapFixture { fixture in
    try await fixture.map.setStyleJson(emptyStyle)
    let data = try GeoJsonSourceDataHandle.create(
      data: #"{"type":"FeatureCollection","features":[]}"#
    )
    let other = try GeoJsonSourceDataHandle
      .create(
        data: #"{"type":"FeatureCollection","features":[]}"#
      )
    #expect(Set([data, data, other]).count == 2)
    #expect(data.id != 0)
    #expect(!data.isClosed)
    expectDescription(
      of: data.description,
      typeName: "GeoJsonSourceDataHandle",
      id: data.id,
      closed: false
    )
    data.release()
    #expect(data.isClosed)
    data.release()
    #expect(data.isClosed)
    other.release()
    expectDescription(
      of: String(describing: data),
      typeName: "GeoJsonSourceDataHandle",
      id: data.id,
      closed: true
    )

    await expectMaplibreError(.invalidState) {
      try await fixture.map.addGeoJsonSourceData("late", data: data)
    }
  }
}

/// A dispose is idempotent and marks the handle closed, while an asynchronous
/// release after it reaches native, which reports the stale handle.
@Test func aDisposeIsIdempotentAndAsyncReleaseReachesNative(
) async throws {
  let runtime = try MapFixture.makeRuntime()
  let map = try await MapFixture.makeMap(on: runtime)
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
  await expectMaplibreError(.invalidState) {
    try await map.release()
  }

  // The runtime finishes its teardown once the disposed map retires.
  try runtime.dispose()
  #expect(runtime.isClosed)
}

/// A handle's id is the source that its events carry, and it stays readable
/// after its release, so a host can route events that it drains later. The id
/// also
/// names the handle in its description.
@Test func aHandleIdNamesItsEventsAndOutlivesRelease() async throws {
  try await withMapFixture { fixture in
    try await fixture.map.setStyleJson(emptyStyle)
    let loaded = try await fixture.awaitEvent("the style load") {
      $0.type == .mapStyleLoaded
    }
    #expect(loaded?.source == fixture.map.id)
    let hex = String(fixture.map.id, radix: 16)
    #expect(fixture.map.description == "MapHandle(id=0x\(hex), closed=false)")
    try await fixture.map.release()
    #expect(fixture.map.id == loaded?.source)
    #expect(fixture.map.description == "MapHandle(id=0x\(hex), closed=true)")
  }
}

/// Native refuses to release a runtime that still has a map, and the refusal
/// leaves the runtime usable, so the host releases the map and then the
/// runtime.
@Test func aRefusedReleaseLeavesTheRuntimeUsableAndALaterReleaseSucceeds(
) async throws {
  let fixture = try await MapFixture.make()
  await expectMaplibreError(.invalidState) {
    try await fixture.runtime.release()
  }
  try await fixture.runtime.clearResourceTransform()

  try await fixture.map.release()
  try await fixture.runtime.release()
}

/// A map holds its runtime, so a host may drop its own runtime reference
/// while the map is live, and the map keeps working.
@Test func aMapKeepsItsRuntimeAlive() async throws {
  var runtime: RuntimeHandle? = try MapFixture.makeRuntime()
  // Kept only to release the runtime after its map.
  weak let parent = runtime
  let map = try await MapFixture.makeMap(on: #require(runtime))
  runtime = nil

  try await map.setStyleJson(emptyStyle)
  try await map.release()
  try await #require(parent).release()
}
