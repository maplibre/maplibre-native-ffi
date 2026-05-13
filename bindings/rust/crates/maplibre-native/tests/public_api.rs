use maplibre_native::{
    BoundOptions, CameraFitOptions, CameraOptions, Feature, FeatureIdentifier,
    FeatureStateSelector, GeoJson, Geometry, JsonMember, JsonValue, LatLng, MapOptions,
    MapTileOptions, MapViewportOptions, OfflineRegionDefinition, OfflineRegionInfo,
    RenderedFeatureQueryOptions, RenderedQueryGeometry, RuntimeOptions, SourceFeatureQueryOptions,
    SourceInfo, StyleImage, StyleImageOptions, TileSourceOptions,
};

#[test]
fn moved_public_types_remain_importable_from_crate_root() {
    let _camera = CameraOptions::new().with_center(LatLng::new(1.0, 2.0));
    let _fit = CameraFitOptions::new();
    let _bounds = BoundOptions::new();
    let _map = MapOptions::new(256, 256, 1.0);
    let _viewport = MapViewportOptions::new();
    let _tile = MapTileOptions::new();
    let _json = JsonValue::object(vec![JsonMember::new("answer", JsonValue::Int(42))]);
    let geometry = Geometry::Point(LatLng::new(1.0, 2.0));
    let feature = Feature::new(geometry.clone(), Vec::new())
        .with_identifier(FeatureIdentifier::String("id".into()));
    let _geojson = GeoJson::Feature(feature);
    let _selector = FeatureStateSelector::new("source").with_feature_id("feature");
    let _rendered_geometry =
        RenderedQueryGeometry::point(maplibre_native::ScreenPoint::new(1.0, 2.0));
    let _rendered_query = RenderedFeatureQueryOptions::new();
    let _source_query = SourceFeatureQueryOptions::new();
    let _runtime = RuntimeOptions::new();
    let _offline = OfflineRegionDefinition::TilePyramid {
        style_url: "maplibre://style".to_string(),
        bounds: maplibre_native::LatLngBounds::new(LatLng::new(0.0, 0.0), LatLng::new(1.0, 1.0)),
        min_zoom: 0.0,
        max_zoom: 1.0,
        pixel_ratio: 1.0,
        include_ideographs: false,
    };
    let _offline_info_type: Option<OfflineRegionInfo> = None;
    let _source_info_type: Option<SourceInfo> = None;
    let _style_image_type: Option<StyleImage> = None;
    let _style_image_options = StyleImageOptions::new();
    let _tile_source_options = TileSourceOptions::new();
}
