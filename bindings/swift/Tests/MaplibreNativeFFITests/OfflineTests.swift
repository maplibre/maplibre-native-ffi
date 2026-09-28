import CMaplibreNativeC
import Foundation
@testable import MaplibreNativeFFI
import Testing

@Test func offlineRegionDefinitionsMaterializeTileAndGeometryDescriptors(
) throws {
  let tileDefinition = OfflineRegionDefinition
    .tilePyramid(OfflineTilePyramidRegionDefinition(
      styleUrl: "https://example.com/style.json",
      bounds: LatLngBounds(
        southwest: LatLng(latitude: -1, longitude: -2),
        northeast: LatLng(latitude: 3, longitude: 4)
      ),
      minZoom: 1,
      maxZoom: 5,
      pixelRatio: 2,
      includeIdeographs: true
    ))

  let tileDefinitionArena = NativeInputArena()
  defer { withExtendedLifetime(tileDefinitionArena) {} }
  try withUnsafePointer(to: tileDefinition
    .nativeValue(arena: tileDefinitionArena))
  { native in
    #expect(native.pointee.type == MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID
      .rawValue)
    #expect(String(cString: native.pointee.data.tile_pyramid.style_url) ==
      "https://example.com/style.json")
    #expect(native.pointee.data.tile_pyramid.bounds.northeast.longitude == 4)
    #expect(native.pointee.data.tile_pyramid.pixel_ratio == 2)
    #expect(native.pointee.data.tile_pyramid.include_ideographs)
  }

  let geometryDefinition = OfflineRegionDefinition
    .geometry(OfflineGeometryRegionDefinition(
      styleUrl: "asset://style.json",
      geometry: Data(
        #"{"type":"LineString","coordinates":[[2,1],[4,3]]}"#.utf8
      ),
      minZoom: 0,
      maxZoom: .infinity,
      pixelRatio: 1,
      includeIdeographs: false
    ))

  let geometryDefinitionArena = NativeInputArena()
  defer { withExtendedLifetime(geometryDefinitionArena) {} }
  try withUnsafePointer(to: geometryDefinition
    .nativeValue(arena: geometryDefinitionArena))
  { native in
    #expect(native.pointee.type == MLN_OFFLINE_REGION_DEFINITION_GEOMETRY
      .rawValue)
    #expect(String(cString: native.pointee.data.geometry.style_url) ==
      "asset://style.json")
    let geometry = native.pointee.data.geometry.geometry
    let geometryData = try #require(geometry.data)
    #expect(Data(bytes: geometryData, count: geometry.size) ==
      Data(#"{"type":"LineString","coordinates":[[2,1],[4,3]]}"#.utf8))
  }
}

@Test func offlineRegionInfoCopiesDefinitionAndMetadata() throws {
  let metadata = [UInt8]("metadata".utf8)
  let definition = OfflineRegionDefinition
    .tilePyramid(OfflineTilePyramidRegionDefinition(
      styleUrl: "asset://style.json",
      bounds: LatLngBounds(
        southwest: LatLng(latitude: 0, longitude: 1),
        northeast: LatLng(latitude: 2, longitude: 3)
      ),
      minZoom: 2,
      maxZoom: 6,
      pixelRatio: 1,
      includeIdeographs: false
    ))

  let arena = NativeInputArena()
  defer { withExtendedLifetime(arena) {} }
  let copied = try withUnsafePointer(to: definition
    .nativeValue(arena: arena))
  { definition in
    try metadata.withUnsafeBufferPointer { metadata in
      var raw = mln_offline_region_info()
      raw.size = UInt32(MemoryLayout<mln_offline_region_info>.size)
      raw.id = 42
      raw.definition = definition.pointee
      raw.metadata = metadata.baseAddress
      raw.metadata_size = metadata.count
      return try OfflineRegionInfo(raw: raw)
    }
  }

  #expect(copied.id == 42)
  #expect(copied.definition == definition)
  #expect(copied.metadata == Data(metadata))
}

@Test func offlineRegionCopyRejectsMissingGeometryAndMetadataPointers() throws {
  var definition = mln_offline_region_definition()
  definition.type = MLN_OFFLINE_REGION_DEFINITION_GEOMETRY.rawValue
  definition.data.geometry = mln_offline_geometry_region_definition()
  definition.data.geometry.geometry = mln_buffer_view(data: nil, size: 1)

  #expect(throws: NativeStringError.self) {
    try OfflineRegionDefinition(raw: definition)
  }

  var info = mln_offline_region_info()
  info.definition.type = MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID.rawValue
  info.metadata = nil
  info.metadata_size = 1

  #expect(throws: NativeStringError.self) { try OfflineRegionInfo(raw: info) }
}

/// An offline region's whole lifecycle over the runtime's own database, and
/// the not-found status every mutation reports for an id no region carries.
@Test func offlineRegionLifecycleReportsNotFoundForAMissingId() async throws {
  let runtime =
    try Maplibre.runtimeCreate(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }

  try await runtime.setMaximumAmbientCacheSize(size: 8 << 20)
  try await runtime.runAmbientCacheOperation(operation: .clear)

  let definition = OfflineRegionDefinition
    .tilePyramid(OfflineTilePyramidRegionDefinition(
      styleUrl: "asset://style.json",
      bounds: LatLngBounds(
        southwest: LatLng(latitude: 0, longitude: 0),
        northeast: LatLng(latitude: 1, longitude: 1)
      ),
      minZoom: 0,
      maxZoom: 2,
      pixelRatio: 1,
      includeIdeographs: false
    ))
  let created = try await runtime.offlineRegionCreate(
    definition: definition,
    metadata: Data("first".utf8)
  )
  #expect(created.definition == definition)
  #expect(created.metadata == Data("first".utf8))

  let updated = try await runtime.offlineRegionUpdateMetadata(
    regionId: created.id,
    metadata: Data("second".utf8)
  )
  #expect(updated.id == created.id)
  #expect(updated.metadata == Data("second".utf8))
  #expect(try await runtime.offlineRegionGet(regionId: created.id) == updated)
  #expect(try await runtime.offlineRegionsList().contains(updated))

  let status = try await runtime.offlineRegionGetStatus(regionId: created.id)
  #expect(status.downloadState == .inactive)

  try await runtime.offlineRegionSetObserved(
    regionId: created.id,
    observed: true
  )
  try await runtime.offlineRegionInvalidate(regionId: created.id)
  try await runtime.offlineRegionDelete(regionId: created.id)
  #expect(try await runtime.offlineRegionGet(regionId: created.id) == nil)

  // Every mutation reports the missing region through its completion.
  let missing = created.id
  await expectNotFound { try await runtime.offlineRegionUpdateMetadata(
    regionId: missing, metadata: Data()
  ) }
  await expectNotFound {
    try await runtime.offlineRegionGetStatus(regionId: missing)
  }
  await expectNotFound {
    try await runtime.offlineRegionSetObserved(
      regionId: missing,
      observed: false
    )
  }
  await expectNotFound {
    try await runtime.offlineRegionSetDownloadState(
      regionId: missing, state: .inactive
    )
  }
  await expectNotFound {
    try await runtime.offlineRegionInvalidate(regionId: missing)
  }
  await expectNotFound {
    try await runtime.offlineRegionDelete(regionId: missing)
  }
}

private func expectNotFound(
  _ body: () async throws -> some Any,
  sourceLocation: SourceLocation = #_sourceLocation
) async {
  do {
    _ = try await body()
    Issue.record(
      "a missing region should report not found",
      sourceLocation: sourceLocation
    )
  } catch let error as MaplibreError {
    #expect(error.kind == .notFound, sourceLocation: sourceLocation)
    #expect(
      error.rawStatus == MLN_STATUS_NOT_FOUND.rawValue,
      sourceLocation: sourceLocation
    )
  } catch {
    Issue.record(
      "unexpected error: \(error)", sourceLocation: sourceLocation
    )
  }
}

@Test func closedRuntimeRejectsOfflineCallsThroughSwiftHandleState(
) async throws {
  let runtime =
    try Maplibre.runtimeCreate(options: RuntimeOptions(cachePath: ":memory:"))
  try await runtime.close()

  do {
    _ = try await runtime.offlineRegionsList()
    Issue.record("closed runtime should throw")
  } catch let error as MaplibreError {
    #expect(error.kind == .invalidState)
    #expect(error.rawStatus == nil)
  } catch {
    Issue.record("unexpected error: \(error)")
  }
}
