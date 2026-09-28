import CMaplibreNativeC
import Foundation
@testable import MaplibreNativeFFI
import Testing

@Test func renderedQueryGeometryCopiesTaggedArraysAndPreservesUnknownTags(
) throws {
  let copied: RenderedQueryGeometry = try {
    let arena = NativeInputArena()
    defer { withExtendedLifetime(arena) {} }
    let geometry = RenderedQueryGeometry.lineString(ScreenLineString(points: [
      ScreenPoint(x: 1, y: 2), ScreenPoint(x: 3, y: 4),
    ]))
    let raw = try geometry.nativeValue(arena: arena)
    #expect(raw.type == MLN_RENDERED_QUERY_GEOMETRY_TYPE_LINE_STRING.rawValue)
    return try RenderedQueryGeometry(raw: raw)
  }()
  #expect(copied == .lineString(ScreenLineString(points: [
    ScreenPoint(x: 1, y: 2), ScreenPoint(x: 3, y: 4),
  ])))
  var unknown = mln_rendered_query_geometry()
  unknown.type = 9876
  let captured = try RenderedQueryGeometry(raw: unknown)
  #expect(captured.data == .unknown(
    9876,
    Data(repeating: 0, count: MemoryLayout.size(ofValue: unknown.data))
  ))
  #expect(throws: (any Error).self) {
    try captured.nativeValue(arena: NativeInputArena())
  }
}

@Test func queryOptionsMaterializeLayerIdsAndFilters() throws {
  let options = RenderedFeatureQueryOptions(
    layerIds: ["roads", "labels"],
    filter: Data(#"["==","kind","road"]"#.utf8)
  )

  let arena = NativeInputArena()
  defer { withExtendedLifetime(arena) {} }
  try withUnsafePointer(to: options.nativeValue(arena: arena)) { pointer in
    let native: Optional = pointer
    #expect(native != nil)
    #expect(native!.pointee
      .fields == MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS.rawValue)
    #expect(native!.pointee.layer_id_count == 2)
    let firstLayerId = try NativeString.copyUTF8(
      data: native!.pointee.layer_ids![0].data,
      size: native!.pointee.layer_ids![0].size
    )
    #expect(firstLayerId == "roads")
    let filter = try #require(native!.pointee.filter)
    let filterData = try #require(filter.pointee.data)
    #expect(Data(bytes: filterData, count: filter.pointee.size) ==
      Data(#"["==","kind","road"]"#.utf8))
  }
}

@Test func sourceQueryOptionsMaterializeLayerIdsAndFilters() throws {
  let options = SourceFeatureQueryOptions(
    sourceLayerIds: ["transportation"],
    filter: Data(#"["has","class"]"#.utf8)
  )

  let arena = NativeInputArena()
  defer { withExtendedLifetime(arena) {} }
  try withUnsafePointer(to: options.nativeValue(arena: arena)) { pointer in
    let native: Optional = pointer
    #expect(native != nil)
    #expect(native!.pointee
      .fields == MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS.rawValue)
    #expect(native!.pointee.source_layer_id_count == 1)
    let firstLayerId = try NativeString.copyUTF8(
      data: native!.pointee.source_layer_ids![0].data,
      size: native!.pointee.source_layer_ids![0].size
    )
    #expect(firstLayerId == "transportation")
    let filter = try #require(native!.pointee.filter)
    let filterData = try #require(filter.pointee.data)
    #expect(Data(bytes: filterData, count: filter.pointee.size) ==
      Data(#"["has","class"]"#.utf8))
  }
}

@Test func featureStateSelectorMaterializesOptionalFields() throws {
  let selector = FeatureStateSelector(
    sourceId: "source",
    sourceLayerId: "layer",
    featureId: "id",
    stateKey: "hover"
  )

  let arena = NativeInputArena()
  let converted = try selector.nativeValue(arena: arena)
  try withUnsafePointer(to: converted) { native in
    #expect(native.pointee.fields == (
      MLN_FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID.rawValue |
        MLN_FEATURE_STATE_SELECTOR_FEATURE_ID.rawValue |
        MLN_FEATURE_STATE_SELECTOR_STATE_KEY.rawValue
    ))
    let sourceId = try NativeString.copyUTF8(
      data: native.pointee.source_id.data,
      size: native.pointee.source_id.size
    )
    let sourceLayerId = try NativeString.copyUTF8(
      data: native.pointee.source_layer_id.data,
      size: native.pointee.source_layer_id.size
    )
    let featureId = try NativeString.copyUTF8(
      data: native.pointee.feature_id.data,
      size: native.pointee.feature_id.size
    )
    let stateKey = try NativeString.copyUTF8(
      data: native.pointee.state_key.data,
      size: native.pointee.state_key.size
    )
    #expect(sourceId == "source")
    #expect(sourceLayerId == "layer")
    #expect(featureId == "id")
    #expect(stateKey == "hover")
  }
}

private func featureStateObject(
  _ map: MapHandle,
  selector: FeatureStateSelector
) async throws -> [String: Any] {
  let bytes = try await map.getFeatureState(selector: selector)
  return try #require(
    JSONSerialization.jsonObject(with: bytes) as? [String: Any]
  )
}

/// Feature state belongs to the map store: a committed set reads back through
/// an ordered get, a keyed remove drops only that key, and a source-wide
/// remove clears the rest, all without a render session or a loaded source.
@Test func mapFeatureStateRoundTripsWithoutARenderSession() async throws {
  let runtime =
    try Maplibre.runtimeCreate(options: RuntimeOptions(cachePath: ":memory:"))
  defer { try? runtime.closeBlockingForTests() }
  let map = try await runtime
    .mapCreate(options: MapOptions(initialExtent: LogicalExtent(
      width: 64,
      height: 64,
      scaleFactor: 1
    )))
  defer { try? map.closeBlockingForTests() }

  let feature = FeatureStateSelector(sourceId: "geo", featureId: "f1")
  // Missing state reads as an empty JSON object.
  #expect(try await featureStateObject(map, selector: feature).isEmpty)

  try await map.setFeatureState(
    selector: feature,
    state: Data(#"{"hover":true,"rank":2}"#.utf8)
  )
  let stored = try await featureStateObject(map, selector: feature)
  #expect(stored["hover"] as? Bool == true)
  #expect(stored["rank"] as? Int == 2)

  try await map.removeFeatureState(selector: FeatureStateSelector(
    sourceId: "geo", featureId: "f1", stateKey: "hover"
  ))
  let trimmed = try await featureStateObject(map, selector: feature)
  #expect(Set(trimmed.keys) == ["rank"])

  try await map
    .removeFeatureState(selector: FeatureStateSelector(sourceId: "geo"))
  #expect(try await featureStateObject(map, selector: feature).isEmpty)
}
