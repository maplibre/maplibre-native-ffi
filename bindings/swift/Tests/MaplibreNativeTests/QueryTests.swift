import CMaplibreNativeC
@testable import MaplibreNative
import Testing

@Test func renderedQueryGeometryMaterializesNativeShapes() throws {
  try RenderedQueryGeometry.point(ScreenPoint(x: 1, y: 2)).nativeGeometry
    .withNativeGeometry { geometry in
      #expect(geometry.pointee.type == MLN_RENDERED_QUERY_GEOMETRY_TYPE_POINT
        .rawValue)
      #expect(geometry.pointee.data.point.x == 1)
      #expect(geometry.pointee.data.point.y == 2)
    }

  try RenderedQueryGeometry.lineString([
    ScreenPoint(x: 1, y: 2),
    ScreenPoint(x: 3, y: 4),
  ]).nativeGeometry
    .withNativeGeometry { geometry in
      #expect(geometry.pointee
        .type == MLN_RENDERED_QUERY_GEOMETRY_TYPE_LINE_STRING.rawValue)
      #expect(geometry.pointee.data.line_string.point_count == 2)
      #expect(geometry.pointee.data.line_string.points![1].x == 3)
    }
}

@Test func renderedQueryLineStringRejectsEmptyInputBeforeCallingC() throws {
  do {
    try RenderedQueryGeometry.lineString([]).nativeGeometry
      .withNativeGeometry { _ in }
    Issue.record("empty line string should throw")
  } catch let failure as NativeStatusFailure {
    #expect(!failure.isNativeStatus)
    #expect(failure.rawStatus == MLN_STATUS_INVALID_ARGUMENT.rawValue)
    #expect(failure
      .diagnostic ==
      "rendered query line string geometry must contain at least one point")
  } catch {
    Issue.record("unexpected error: \(error)")
  }
}

@Test func queryOptionsMaterializeLayerIdsAndFilters() throws {
  let options = RenderedFeatureQueryOptions(
    layerIds: ["roads", "labels"],
    filter: .array([.string("=="), .string("kind"), .string("road")])
  )

  try options.nativeOptions.withNativeOptions { native in
    #expect(native != nil)
    #expect(native!.pointee
      .fields == MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS.rawValue)
    #expect(native!.pointee.layer_id_count == 2)
    let firstLayerId = try NativeString.copyUTF8(
      data: native!.pointee.layer_ids![0].data,
      size: native!.pointee.layer_ids![0].size
    )
    #expect(firstLayerId == "roads")
    #expect(native!.pointee.filter?.pointee.type == MLN_JSON_VALUE_TYPE_ARRAY
      .rawValue)
  }
}

@Test func sourceQueryOptionsMaterializeLayerIdsAndFilters() throws {
  let options = SourceFeatureQueryOptions(
    sourceLayerIds: ["transportation"],
    filter: .array([.string("has"), .string("class")])
  )

  try options.nativeOptions.withNativeOptions { native in
    #expect(native != nil)
    #expect(native!.pointee
      .fields == MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS.rawValue)
    #expect(native!.pointee.source_layer_id_count == 1)
    let firstLayerId = try NativeString.copyUTF8(
      data: native!.pointee.source_layer_ids![0].data,
      size: native!.pointee.source_layer_ids![0].size
    )
    #expect(firstLayerId == "transportation")
    #expect(native!.pointee.filter?.pointee.type == MLN_JSON_VALUE_TYPE_ARRAY
      .rawValue)
  }
}

@Test func featureStateSelectorMaterializesOptionalFields() throws {
  let selector = FeatureStateSelector(
    sourceId: "source",
    sourceLayerId: "layer",
    featureId: "id",
    stateKey: "hover"
  )

  try selector.nativeSelector.withNativeSelector { native in
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

@Test func queriedFeatureCopiesNestedFeatureStateAndSourceMetadata() throws {
  let arena = NativeInputArena()
  var geometry = mln_geometry()
  geometry.size = UInt32(MemoryLayout<mln_geometry>.size)
  geometry.type = MLN_GEOMETRY_TYPE_POINT.rawValue
  geometry.data.point = mln_lat_lng(latitude: 1, longitude: 2)

  var property = mln_json_member(
    key: arena.view("name"),
    value: arena.allocate(.string("feature"))
  )
  var state = arena.nativeValue(.object([NativeJSONMember(
    key: "selected",
    value: .bool(true)
  )]))
  var feature = mln_feature()
  feature.size = UInt32(MemoryLayout<mln_feature>.size)
  feature.identifier_type = MLN_FEATURE_IDENTIFIER_TYPE_STRING.rawValue
  feature.identifier.string_value = arena.view("feature-id")

  let copied = try withUnsafePointer(to: &geometry) { geometryPointer in
    try withUnsafePointer(to: &property) { propertyPointer in
      try withUnsafePointer(to: &state) { statePointer in
        feature.geometry = geometryPointer
        feature.properties = propertyPointer
        feature.property_count = 1

        let raw = mln_queried_feature(
          size: UInt32(MemoryLayout<mln_queried_feature>.size),
          fields: MLN_QUERIED_FEATURE_SOURCE_ID
            .rawValue | MLN_QUERIED_FEATURE_SOURCE_LAYER_ID
            .rawValue | MLN_QUERIED_FEATURE_STATE.rawValue,
          feature: feature,
          source_id: arena.view("source"),
          source_layer_id: arena.view("layer"),
          state: statePointer
        )
        return try NativeQueriedFeature(copying: raw)
      }
    }
  }

  #expect(copied.sourceId == "source")
  #expect(copied.sourceLayerId == "layer")
  #expect(copied.feature.identifier == .string("feature-id"))
  #expect(copied.feature.geometry == .point(NativeLatLng(
    latitude: 1,
    longitude: 2
  )))
  #expect(copied.feature.properties == [NativeJSONMember(
    key: "name",
    value: .string("feature")
  )])
  #expect(copied.state == .object([NativeJSONMember(
    key: "selected",
    value: .bool(true)
  )]))
}
