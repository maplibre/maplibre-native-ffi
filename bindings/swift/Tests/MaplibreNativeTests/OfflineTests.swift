import CMaplibreNativeC
import Foundation
@testable import MaplibreNative
import Testing

@Test func offlineRegionDefinitionsMaterializeTileAndGeometryDescriptors(
) throws {
  let tileDefinition = OfflineRegionDefinition.tilePyramid(
    styleURL: "https://example.com/style.json",
    bounds: LatLngBounds(
      southwest: LatLng(latitude: -1, longitude: -2),
      northeast: LatLng(latitude: 3, longitude: 4)
    ),
    minZoom: 1,
    maxZoom: 5,
    pixelRatio: 2,
    includeIdeographs: true
  )

  try tileDefinition.nativeDefinition.withNativeDefinition { native in
    #expect(native.pointee.type == MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID
      .rawValue)
    #expect(String(cString: native.pointee.data.tile_pyramid.style_url) ==
      "https://example.com/style.json")
    #expect(native.pointee.data.tile_pyramid.bounds.northeast.longitude == 4)
    #expect(native.pointee.data.tile_pyramid.pixel_ratio == 2)
    #expect(native.pointee.data.tile_pyramid.include_ideographs)
  }

  let geometryDefinition = OfflineRegionDefinition.geometry(
    styleURL: "asset://style.json",
    geometry: .lineString([
      LatLng(latitude: 1, longitude: 2),
      LatLng(latitude: 3, longitude: 4),
    ]),
    minZoom: 0,
    maxZoom: .infinity,
    pixelRatio: 1,
    includeIdeographs: false
  )

  try geometryDefinition.nativeDefinition.withNativeDefinition { native in
    #expect(native.pointee.type == MLN_OFFLINE_REGION_DEFINITION_GEOMETRY
      .rawValue)
    #expect(String(cString: native.pointee.data.geometry.style_url) ==
      "asset://style.json")
    #expect(native.pointee.data.geometry.geometry.pointee
      .type == MLN_GEOMETRY_TYPE_LINE_STRING.rawValue)
    #expect(native.pointee.data.geometry.geometry.pointee.data.line_string
      .coordinate_count == 2)
  }
}

@Test func offlineRegionInfoCopiesDefinitionAndMetadata() throws {
  let metadata = [UInt8]("metadata".utf8)
  let definition = OfflineRegionDefinition.tilePyramid(
    styleURL: "asset://style.json",
    bounds: LatLngBounds(
      southwest: LatLng(latitude: 0, longitude: 1),
      northeast: LatLng(latitude: 2, longitude: 3)
    ),
    minZoom: 2,
    maxZoom: 6,
    pixelRatio: 1,
    includeIdeographs: false
  )

  let copied = try definition.nativeDefinition
    .withNativeDefinition { definition in
      try metadata.withUnsafeBufferPointer { metadata in
        var raw = mln_offline_region_info()
        raw.size = UInt32(MemoryLayout<mln_offline_region_info>.size)
        raw.id = 42
        raw.definition = definition.pointee
        raw.metadata = metadata.baseAddress
        raw.metadata_size = metadata.count
        return try NativeOfflineRegionInfo(copying: raw)
      }
    }

  #expect(copied.id == 42)
  #expect(copied.definition == definition.nativeDefinition)
  #expect(copied.metadata == Data(metadata))
}

@Test func offlineRegionCopyRejectsMissingGeometryAndMetadataPointers() throws {
  var definition = mln_offline_region_definition()
  definition.type = MLN_OFFLINE_REGION_DEFINITION_GEOMETRY.rawValue
  definition.data.geometry = mln_offline_geometry_region_definition()
  definition.data.geometry.geometry = nil

  do {
    _ = try NativeOfflineRegionDefinition(copying: definition)
    Issue.record("missing geometry should throw")
  } catch let failure as NativeStatusFailure {
    #expect(!failure.isNativeStatus)
    #expect(failure.rawStatus == MLN_STATUS_NATIVE_ERROR.rawValue)
    #expect(failure
      .diagnostic == "offline geometry region definition geometry is null")
  } catch {
    Issue.record("unexpected error: \(error)")
  }

  var info = mln_offline_region_info()
  info.definition.type = MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID.rawValue
  info.metadata = nil
  info.metadata_size = 1

  do {
    _ = try NativeOfflineRegionInfo(copying: info)
    Issue.record("missing metadata should throw")
  } catch let failure as NativeStatusFailure {
    #expect(!failure.isNativeStatus)
    #expect(failure.rawStatus == MLN_STATUS_NATIVE_ERROR.rawValue)
    #expect(failure.diagnostic == "offline region metadata is null")
  } catch {
    Issue.record("unexpected error: \(error)")
  }
}

@Test func closedRuntimeRejectsOfflineCallsThroughSwiftHandleState() throws {
  let runtime =
    try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  try runtime.close()

  do {
    _ = try runtime.offlineRegionsListStart()
    Issue.record("closed runtime should throw")
  } catch let error as MaplibreError {
    #expect(error.kind == .invalidState)
    #expect(error.rawStatus == nil)
  } catch {
    Issue.record("unexpected error: \(error)")
  }
}
