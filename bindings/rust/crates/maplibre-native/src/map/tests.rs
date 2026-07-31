use super::*;
use std::time::Duration;

use crate::events::{
    RuntimeEventPayload, RuntimeEventSource, RuntimeEventType, empty_runtime_event,
};
use crate::{
    BoundsConstraint, CameraChangeMode, CustomGeometrySourceOptions, EdgeInsets, ErrorKind,
    Feature, JsonMember, MapMode, ResourceKind, ResourceProviderDecision, ResourceResponse,
    TextureImageInfo,
};

const VALID_STYLE_JSON: &str = r#"{"version":8,"sources":{},"layers":[]}"#;
const STYLE_WITH_IDS_JSON: &str = r#"{"version":8,"sources":{"geo":{"type":"geojson","data":{"type":"FeatureCollection","features":[]}}},"layers":[{"id":"background","type":"background"},{"id":"geo-fill","type":"fill","source":"geo"}]}"#;

#[test]
// Spec coverage: BND-105.
fn nine_patch_style_image_round_trips_stretch_content_and_text_fit() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();
    map.set_style_json(VALID_STYLE_JSON).unwrap();

    let image =
        PremultipliedRgba8Image::new(crate::TextureImageInfo::new(2, 2, 8, 16), vec![0u8; 16]);
    let mut options = StyleImageOptions::default();
    options.stretch_x = Some(vec![ImageStretch::new(0.0, 1.0)]);
    options.stretch_y = Some(vec![
        ImageStretch::new(0.0, 1.0),
        ImageStretch::new(1.0, 2.0),
    ]);
    options.content = Some(ImageContent {
        left: 0.5,
        top: 0.5,
        right: 1.5,
        bottom: 1.5,
    });
    options.text_fit_height = Some(StyleImageTextFit::Proportional);
    map.set_style_image("patch", &image, Some(&options))
        .unwrap();

    let info = map.style_image_info("patch").unwrap().unwrap();
    assert_eq!(info.stretch_x_count, 1);
    assert_eq!(info.stretch_y_count, 2);
    assert_eq!(
        info.content,
        Some(ImageContent {
            left: 0.5,
            top: 0.5,
            right: 1.5,
            bottom: 1.5,
        })
    );
    // An absent text fit stays distinguishable from a present default.
    assert_eq!(info.text_fit_width, None);
    assert_eq!(info.text_fit_height, Some(StyleImageTextFit::Proportional));

    let (stretch_x, stretch_y) = map.style_image_stretches("patch").unwrap().unwrap();
    assert_eq!(stretch_x, vec![ImageStretch::new(0.0, 1.0)]);
    assert_eq!(
        stretch_y,
        vec![ImageStretch::new(0.0, 1.0), ImageStretch::new(1.0, 2.0)]
    );
    assert!(map.style_image_stretches("missing").unwrap().is_none());

    // A backwards interval is rejected by C.
    let mut bad = StyleImageOptions::default();
    bad.stretch_x = Some(vec![ImageStretch::new(2.0, 1.0)]);
    let error = map.set_style_image("bad", &image, Some(&bad)).unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);

    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-105.
fn layer_base_accessors_round_trip_through_real_c_abi() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();
    map.set_style_json(STYLE_WITH_IDS_JSON).unwrap();

    // A layer with a source round-trips source-layer and source id.
    assert_eq!(map.layer_source_layer("geo-fill").unwrap(), "");
    map.set_layer_source_layer("geo-fill", "roads").unwrap();
    assert_eq!(map.layer_source_layer("geo-fill").unwrap(), "roads");
    assert_eq!(map.layer_source_id("geo-fill").unwrap(), "geo");

    // A layer type that takes no source is rejected rather than silently
    // ignored, and reads back as empty.
    let error = map
        .set_layer_source_layer("background", "roads")
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    assert!(error.diagnostic().contains("source-layer"));
    assert_eq!(map.layer_source_id("background").unwrap(), "");

    // An unset zoom range crosses the boundary as infinities.
    assert_eq!(map.layer_min_zoom("geo-fill").unwrap(), f64::NEG_INFINITY);
    assert_eq!(map.layer_max_zoom("geo-fill").unwrap(), f64::INFINITY);
    map.set_layer_min_zoom("geo-fill", 4.0).unwrap();
    map.set_layer_max_zoom("geo-fill", 12.5).unwrap();
    assert_eq!(map.layer_min_zoom("geo-fill").unwrap(), 4.0);
    assert_eq!(map.layer_max_zoom("geo-fill").unwrap(), 12.5);

    assert_eq!(
        map.layer_visibility("geo-fill").unwrap(),
        StyleLayerVisibility::Visible
    );
    map.set_layer_visibility("geo-fill", StyleLayerVisibility::None)
        .unwrap();
    assert_eq!(
        map.layer_visibility("geo-fill").unwrap(),
        StyleLayerVisibility::None
    );

    // An unknown raw visibility passes through to C, which rejects it.
    let error = map
        .set_layer_visibility("geo-fill", StyleLayerVisibility::Unknown(900))
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);

    let error = map.layer_min_zoom("missing").unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);

    map.close().unwrap();
    runtime.close().unwrap();
}

fn object_member<'a>(value: &'a JsonValue, key: &str) -> Option<&'a JsonValue> {
    let JsonValue::Object(members) = value else {
        return None;
    };
    members
        .iter()
        .find(|member| member.key == key)
        .map(|member| &member.value)
}

fn assert_lat_lng_close(actual: LatLng, expected: LatLng) {
    assert!((actual.latitude - expected.latitude).abs() < 1e-7);
    assert!((actual.longitude - expected.longitude).abs() < 1e-7);
}

#[test]
// Spec coverage: BND-040 and BND-100.
fn map_close_consumes_handle_and_drop_stays_idempotent() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();

    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-042.
fn map_retains_runtime_after_runtime_handle_is_dropped() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();

    drop(runtime);

    map.close().unwrap();
}

#[test]
// Spec coverage: BND-024, BND-101, and BND-105.
fn style_setters_accept_valid_input_and_reject_embedded_nul() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();

    map.set_style_json(VALID_STYLE_JSON).unwrap();
    let _ = map.style_source_ids().unwrap();
    let _ = map.style_layer_ids().unwrap();

    map.set_style_json(STYLE_WITH_IDS_JSON).unwrap();
    let source_ids = map.style_source_ids().unwrap();
    let layer_ids = map.style_layer_ids().unwrap();
    assert!(source_ids.iter().any(|id| id == "geo"));
    assert!(layer_ids.iter().any(|id| id == "background"));
    assert!(layer_ids.iter().any(|id| id == "geo-fill"));

    map.set_style_url("https://example.com/style.json").unwrap();

    let error = map
        .set_style_url("https://example.com/\0style.json")
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    assert_eq!(error.raw_status(), None);
    assert!(error.diagnostic().contains("embedded NUL"));

    let error = map.set_style_json("{").unwrap_err();
    assert_eq!(error.kind(), ErrorKind::NativeError);
    assert_eq!(error.raw_status(), Some(sys::MLN_STATUS_NATIVE_ERROR));
    assert!(!error.diagnostic().trim().is_empty());

    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-105.
fn style_source_exists_and_remove_call_real_c_api() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();
    map.set_style_json(VALID_STYLE_JSON).unwrap();

    let source = JsonValue::Object(vec![
        JsonMember::new("type", JsonValue::String("geojson".to_owned())),
        JsonMember::new(
            "data",
            JsonValue::Object(vec![
                JsonMember::new("type", JsonValue::String("FeatureCollection".to_owned())),
                JsonMember::new("features", JsonValue::Array(Vec::new())),
            ]),
        ),
    ]);

    assert!(!map.style_source_exists("owned-source").unwrap());
    assert!(!map.remove_style_source("owned-source").unwrap());

    map.add_style_source_json("owned-source", &source).unwrap();
    assert!(map.style_source_exists("owned-source").unwrap());
    assert!(map.remove_style_source("owned-source").unwrap());
    assert!(!map.style_source_exists("owned-source").unwrap());
    assert!(!map.remove_style_source("owned-source").unwrap());

    map.close().unwrap();
    runtime.close().unwrap();
}

fn test_style_image(data: Vec<u8>) -> PremultipliedRgba8Image {
    PremultipliedRgba8Image::new(TextureImageInfo::new(2, 2, 8, data.len()), data)
}

#[test]
// Spec coverage: BND-069 and BND-105.
fn style_image_copy_uses_rust_owned_buffer() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();
    map.set_style_json(VALID_STYLE_JSON).unwrap();

    let original_pixels = vec![
        255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255, 255, 255, 255, 255,
    ];
    let mut image = test_style_image(original_pixels.clone());

    map.set_style_image("plain", &image, None).unwrap();
    image.data.fill(0);
    assert!(map.style_image_exists("plain").unwrap());
    let info = map
        .style_image_info("plain")
        .unwrap()
        .expect("added image should have copied metadata");
    assert_eq!(info.width, image.info.width);
    assert_eq!(info.height, image.info.height);
    let mut copied = map
        .copy_style_image_premultiplied_rgba8("plain")
        .unwrap()
        .expect("added Rust image should copy back through C");

    assert_eq!(copied.image.info.width, image.info.width);
    assert_eq!(copied.image.info.height, image.info.height);
    assert_eq!(copied.image.data, original_pixels);
    copied.image.data.fill(1);
    assert_eq!(
        map.copy_style_image_premultiplied_rgba8("plain")
            .unwrap()
            .expect("style image copy should not expose native storage")
            .image
            .data,
        original_pixels
    );
    assert!(map.remove_style_image("plain").unwrap());
    assert!(!map.style_image_exists("plain").unwrap());
    assert!(!map.remove_style_image("plain").unwrap());
}

fn image_source_coordinates() -> [LatLng; 4] {
    [
        LatLng::new(0.0, 0.0),
        LatLng::new(0.0, 1.0),
        LatLng::new(1.0, 1.0),
        LatLng::new(1.0, 0.0),
    ]
}

#[test]
// Spec coverage: BND-105.
fn image_source_helpers_accept_url_and_inline_images() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();
    map.set_style_json(VALID_STYLE_JSON).unwrap();

    let coordinates = image_source_coordinates();
    map.add_image_source_url("url-image", &coordinates, "https://example.com/image.png")
        .unwrap();
    assert_eq!(
        map.image_source_coordinates("url-image").unwrap(),
        Some(coordinates)
    );

    let image = test_style_image(vec![1; 16]);
    map.add_image_source_image("inline-image", &coordinates, &image)
        .unwrap();
    assert_eq!(
        map.style_source_type("inline-image").unwrap(),
        Some(SourceType::Image)
    );
}

#[test]
// Spec coverage: BND-105.
fn tile_source_helpers_call_real_c_api() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();
    map.set_style_json(VALID_STYLE_JSON).unwrap();

    map.add_vector_source_url("vector-url", "https://example.com/vector.json", None)
        .unwrap();
    assert_eq!(
        map.style_source_type("vector-url").unwrap(),
        Some(SourceType::Vector)
    );

    let mut dem_options = TileSourceOptions::default();
    dem_options.raster_dem_encoding = Some(RasterDemEncoding::Terrarium);
    map.add_raster_dem_source_tiles(
        "dem-tiles",
        &["https://example.com/dem/{z}/{x}/{y}.png"],
        Some(&dem_options),
    )
    .unwrap();
    assert_eq!(
        map.style_source_type("dem-tiles").unwrap(),
        Some(SourceType::RasterDem)
    );
}

#[test]
// Spec coverage: BND-060, BND-061, and BND-105.
fn geojson_source_helpers_accept_options_and_keep_them_across_updates() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();
    map.set_style_json(VALID_STYLE_JSON).unwrap();

    let mut options = GeoJsonSourceOptions::default();
    options.cluster = Some(true);
    options.cluster_radius = Some(40);
    // A present zero must reach native as an explicit buffer of zero.
    options.buffer = Some(0);
    options.cluster_properties = Some(JsonValue::Object(vec![JsonMember::new(
        "weight_sum",
        JsonValue::Array(vec![
            JsonValue::String("+".to_owned()),
            JsonValue::Array(vec![
                JsonValue::String("get".to_owned()),
                JsonValue::String("weight".to_owned()),
            ]),
        ]),
    )]));

    map.add_geojson_source_url(
        "geojson-url",
        "https://example.com/points.geojson",
        Some(&options),
    )
    .unwrap();
    assert_eq!(
        map.style_source_type("geojson-url").unwrap(),
        Some(SourceType::GeoJson)
    );

    let data = GeoJson::FeatureCollection(vec![Feature::new(
        Geometry::Point(LatLng::new(0.0, 0.0)),
        vec![JsonMember::new("weight", JsonValue::UInt(1))],
    )]);
    map.add_geojson_source_data("geojson-data", &data, None)
        .unwrap();
    assert_eq!(
        map.style_source_type("geojson-data").unwrap(),
        Some(SourceType::GeoJson)
    );

    // Updates carry no options of their own; the source keeps what it was added
    // with.
    map.set_geojson_source_data("geojson-url", &data).unwrap();
    map.set_geojson_source_url("geojson-data", "https://example.com/points.geojson")
        .unwrap();

    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-060 and BND-061.
fn clustered_geojson_source_requires_a_feature_collection() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();
    map.set_style_json(VALID_STYLE_JSON).unwrap();

    let mut options = GeoJsonSourceOptions::default();
    options.cluster = Some(true);

    // MapLibre Native engages clustering for feature collections only, so these
    // used to tile unclustered instead of honouring the requested option.
    let bare = GeoJson::Geometry(Geometry::Point(LatLng::new(0.0, 0.0)));
    let error = map
        .add_geojson_source_data("quakes", &bare, Some(&options))
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    let message = error.to_string();
    assert!(message.contains("quakes"), "{message}");
    assert!(
        message.contains("requires a feature collection"),
        "{message}"
    );
    assert!(message.contains("a bare geometry"), "{message}");

    let single = GeoJson::Feature(Feature::new(
        Geometry::Point(LatLng::new(0.0, 0.0)),
        Vec::new(),
    ));
    let error = map
        .add_geojson_source_data("quakes", &single, Some(&options))
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    assert!(error.to_string().contains("a single feature"), "{error}");

    // The constraint belongs to clustering alone, and the rejected ID stays free.
    map.add_geojson_source_data("quakes", &bare, None).unwrap();

    // An empty feature collection carries nothing to cluster, so it stays
    // accepted and a later update supplies the features to cluster.
    let empty = GeoJson::FeatureCollection(Vec::new());
    map.add_geojson_source_data("pending", &empty, Some(&options))
        .unwrap();

    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-060 and BND-061.
fn clustered_geojson_source_reports_non_point_geometry() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();
    map.set_style_json(VALID_STYLE_JSON).unwrap();

    let mut options = GeoJsonSourceOptions::default();
    options.cluster = Some(true);

    let mixed = GeoJson::FeatureCollection(vec![
        Feature::new(Geometry::Point(LatLng::new(0.0, 0.0)), Vec::new()),
        Feature::new(
            Geometry::GeometryCollection(vec![Geometry::Point(LatLng::new(1.0, 1.0))]),
            Vec::new(),
        ),
    ]);

    // Supercluster reads every feature geometry as a point, so this used to
    // surface a bare variant access message from inside MapLibre Native.
    let error = map
        .add_geojson_source_data("quakes", &mixed, Some(&options))
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    let message = error.to_string();
    assert!(message.contains("quakes"), "{message}");
    assert!(
        message.contains("point geometry on every feature"),
        "{message}"
    );
    assert!(message.contains("geometry collection"), "{message}");

    // The constraint belongs to clustering alone, and the rejected ID stays free.
    map.add_geojson_source_data("quakes", &mixed, None).unwrap();

    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-105.
fn style_source_type_and_info_call_real_c_api() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();
    map.set_style_json(VALID_STYLE_JSON).unwrap();

    let geojson_source = JsonValue::Object(vec![
        JsonMember::new("type", JsonValue::String("geojson".to_owned())),
        JsonMember::new(
            "data",
            JsonValue::Object(vec![
                JsonMember::new("type", JsonValue::String("FeatureCollection".to_owned())),
                JsonMember::new("features", JsonValue::Array(Vec::new())),
            ]),
        ),
    ]);
    let vector_source = JsonValue::Object(vec![
        JsonMember::new("type", JsonValue::String("vector".to_owned())),
        JsonMember::new(
            "tiles",
            JsonValue::Array(vec![JsonValue::String(
                "https://example.com/{z}/{x}/{y}.pbf".to_owned(),
            )]),
        ),
        JsonMember::new(
            "attribution",
            JsonValue::String("Example attribution".to_owned()),
        ),
    ]);

    assert_eq!(map.style_source_type("missing-source").unwrap(), None);
    assert_eq!(map.style_source_info("missing-source").unwrap(), None);

    map.add_style_source_json("empty", &geojson_source).unwrap();
    assert_eq!(
        map.style_source_type("empty").unwrap(),
        Some(SourceType::GeoJson)
    );
    let info = map.style_source_info("empty").unwrap().unwrap();
    assert_eq!(info.source_type, SourceType::GeoJson);
    assert_eq!(info.raw_source_type, sys::MLN_STYLE_SOURCE_TYPE_GEOJSON);
    assert!(!info.is_volatile);
    assert_eq!(info.attribution, None);

    map.add_style_source_json("vector-meta", &vector_source)
        .unwrap();
    assert_eq!(
        map.style_source_type("vector-meta").unwrap(),
        Some(SourceType::Vector)
    );
    let info = map.style_source_info("vector-meta").unwrap().unwrap();
    assert_eq!(info.source_type, SourceType::Vector);
    assert_eq!(info.raw_source_type, sys::MLN_STYLE_SOURCE_TYPE_VECTOR);
    assert_eq!(info.attribution.as_deref(), Some("Example attribution"));

    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-124.
fn custom_geometry_source_apis_call_real_c_api_and_style_replacement_releases_state() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();
    map.set_style_json(VALID_STYLE_JSON).unwrap();

    let mut custom_options = CustomGeometrySourceOptions::new(|_| {}).with_cancel_tile(|_| {});
    custom_options.min_zoom = Some(0.0);
    custom_options.max_zoom = Some(2.0);
    custom_options.tolerance = Some(0.375);
    custom_options.tile_size = Some(512);
    custom_options.buffer = Some(64);
    custom_options.clip = Some(true);
    custom_options.wrap = Some(false);
    map.add_custom_geometry_source("custom", custom_options)
        .unwrap();
    assert_eq!(map.custom_geometry_source_count_for_testing(), 1);

    let tile_id = CanonicalTileId::new(0, 0, 0);
    map.set_custom_geometry_source_tile_data(
        "custom",
        tile_id,
        &GeoJson::FeatureCollection(Vec::new()),
    )
    .unwrap();
    map.invalidate_custom_geometry_source_tile("custom", tile_id)
        .unwrap();
    map.invalidate_custom_geometry_source_region(
        "custom",
        LatLngBounds::new(LatLng::new(-1.0, -1.0), LatLng::new(1.0, 1.0)),
    )
    .unwrap();

    assert!(map.remove_style_source("custom").unwrap());
    assert_eq!(map.custom_geometry_source_count_for_testing(), 0);
    assert!(!map.style_source_exists("custom").unwrap());

    map.add_custom_geometry_source("custom", CustomGeometrySourceOptions::new(|_| {}))
        .unwrap();
    assert_eq!(map.custom_geometry_source_count_for_testing(), 1);

    map.set_style_json(VALID_STYLE_JSON).unwrap();
    assert_eq!(map.custom_geometry_source_count_for_testing(), 0);

    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-124.
fn custom_geometry_source_state_is_released_on_map_close() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();
    map.set_style_json(VALID_STYLE_JSON).unwrap();
    map.add_custom_geometry_source("custom", CustomGeometrySourceOptions::new(|_| {}))
        .unwrap();
    assert_eq!(map.custom_geometry_source_count_for_testing(), 1);

    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-124.
fn custom_geometry_source_state_ignores_stale_style_loaded_events() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();
    map.set_style_json(VALID_STYLE_JSON).unwrap();
    map.add_custom_geometry_source("custom", CustomGeometrySourceOptions::new(|_| {}))
        .unwrap();

    let mut event = empty_runtime_event();
    event.type_ = sys::MLN_RUNTIME_EVENT_MAP_STYLE_LOADED;
    event.source_type = sys::MLN_RUNTIME_EVENT_SOURCE_MAP;
    event.source = map.inner.handle.handle().0;
    runtime.inner.apply_event_side_effects_for_testing(&event);

    assert_eq!(map.custom_geometry_source_count_for_testing(), 1);
    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-124.
fn custom_geometry_source_state_releases_detached_sources_on_style_loaded_event() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();
    map.set_style_json(VALID_STYLE_JSON).unwrap();
    map.add_custom_geometry_source("custom", CustomGeometrySourceOptions::new(|_| {}))
        .unwrap();

    let source_id = maplibre_native_core::string::string_view("custom");
    let mut removed = false;
    // SAFETY: map is live, source_id is valid for this call, and removed
    // points to writable storage. This bypasses the binding cleanup path to
    // model native style replacement detaching the source.
    let status = unsafe {
        sys::mln_map_remove_style_source(map.inner.handle.handle(), source_id.raw(), &mut removed)
    };
    assert_eq!(status, sys::MLN_STATUS_OK);
    assert!(removed);
    assert_eq!(map.custom_geometry_source_count_for_testing(), 1);

    let mut event = empty_runtime_event();
    event.type_ = sys::MLN_RUNTIME_EVENT_MAP_STYLE_LOADED;
    event.source_type = sys::MLN_RUNTIME_EVENT_SOURCE_MAP;
    event.source = map.inner.handle.handle().0;
    runtime.inner.apply_event_side_effects_for_testing(&event);

    assert_eq!(map.custom_geometry_source_count_for_testing(), 0);
    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-124.
fn custom_geometry_source_adds_to_current_style_after_url_style_request() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();
    map.set_style_json(VALID_STYLE_JSON).unwrap();
    map.set_style_url("unsupported://style.json").unwrap();

    map.add_custom_geometry_source("custom", CustomGeometrySourceOptions::new(|_| {}))
        .unwrap();

    assert_eq!(map.custom_geometry_source_count_for_testing(), 1);
    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-124.
fn custom_geometry_source_state_releases_after_url_style_replacement() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    runtime
        .set_resource_provider(|request, handle| {
            if request.url == "custom://style.json" {
                assert_eq!(request.kind, ResourceKind::Style);
                handle
                    .complete(ResourceResponse::ok(VALID_STYLE_JSON.as_bytes().to_vec()))
                    .unwrap();
            }
            ResourceProviderDecision::PassThrough
        })
        .unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();
    map.set_style_json(VALID_STYLE_JSON).unwrap();
    map.add_custom_geometry_source("custom", CustomGeometrySourceOptions::new(|_| {}))
        .unwrap();
    assert_eq!(map.custom_geometry_source_count_for_testing(), 1);
    drain_runtime_events(&runtime);

    map.set_style_url("custom://style.json").unwrap();
    wait_for_map_event(&runtime, &map, RuntimeEventType::MapStyleLoaded);

    assert_eq!(map.custom_geometry_source_count_for_testing(), 0);
    map.close().unwrap();
    runtime.close().unwrap();
}

fn drain_runtime_events(runtime: &RuntimeHandle) {
    for _ in 0..20 {
        runtime.pump(Some(Duration::ZERO)).unwrap();
        while runtime.poll_event().unwrap().is_some() {}
    }
}

fn wait_for_map_event(runtime: &RuntimeHandle, map: &MapHandle, event_type: RuntimeEventType) {
    for _ in 0..1000 {
        runtime.pump(Some(Duration::ZERO)).unwrap();
        while let Some(event) = runtime.poll_event().unwrap() {
            if event.event_type == event_type && event.source == RuntimeEventSource::Map(map.id()) {
                return;
            }
        }
        std::thread::sleep(Duration::from_millis(1));
    }
    panic!("timed out waiting for {event_type:?}");
}

#[test]
// Spec coverage: BND-063, BND-064, and BND-105.
fn style_json_descriptors_copy_owned_rust_values() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();
    map.set_style_json(STYLE_WITH_IDS_JSON).unwrap();

    let layer = JsonValue::Object(vec![
        JsonMember::new("id", JsonValue::String("owned-background".to_owned())),
        JsonMember::new("type", JsonValue::String("background".to_owned())),
        JsonMember::new(
            "paint",
            JsonValue::Object(vec![JsonMember::new(
                "background-opacity",
                JsonValue::Double(0.5),
            )]),
        ),
    ]);
    map.add_style_layer_json(&layer, None).unwrap();
    let copied_layer = map
        .style_layer_json("owned-background")
        .unwrap()
        .expect("added layer should have a JSON snapshot");

    assert_eq!(
        object_member(&copied_layer, "id"),
        Some(&JsonValue::String("owned-background".to_owned()))
    );
    let paint = object_member(&copied_layer, "paint").expect("layer paint should be copied");
    assert_eq!(
        object_member(paint, "background-opacity"),
        Some(&JsonValue::Double(0.5))
    );

    map.set_layer_property(
        "owned-background",
        "background-opacity",
        &JsonValue::Double(0.75),
    )
    .unwrap();
    assert_eq!(
        map.layer_property("owned-background", "background-opacity")
            .unwrap(),
        Some(JsonValue::Double(0.75))
    );

    let filter = JsonValue::Array(vec![
        JsonValue::String("==".to_owned()),
        JsonValue::Array(vec![
            JsonValue::String("get".to_owned()),
            JsonValue::String("kind".to_owned()),
        ]),
        JsonValue::String("park".to_owned()),
    ]);
    map.set_layer_filter("geo-fill", Some(&filter)).unwrap();
    assert_eq!(map.layer_filter("geo-fill").unwrap(), Some(filter.clone()));
    map.set_layer_filter("geo-fill", None).unwrap();
    assert_eq!(map.layer_filter("geo-fill").unwrap(), None);

    let error = map
        .set_layer_filter("owned-background", Some(&JsonValue::Double(f64::NAN)))
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    assert_eq!(error.raw_status(), None);

    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-102 and BND-103.
fn camera_jump_and_coordinate_conversions_round_trip() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let mut options = MapOptions::new(512, 512, 1.0);
    options.mode = MapMode::Continuous;
    let map = MapHandle::with_options(&runtime, &options).unwrap();
    let center = LatLng::new(45.0, -122.0);
    let eased_center = LatLng::new(46.0, -123.0);
    let flown_center = LatLng::new(47.0, -124.0);

    let mut jump_camera = CameraOptions::default();
    jump_camera.center = Some(center);
    jump_camera.zoom = Some(4.0);
    map.jump_to(&jump_camera).unwrap();
    let camera = map.camera().unwrap();
    assert_eq!(camera.center, Some(center));
    assert_eq!(camera.zoom, Some(4.0));

    let mut immediate = AnimationOptions::default();
    immediate.duration_ms = Some(0.0);
    let mut ease_camera = CameraOptions::default();
    ease_camera.center = Some(eased_center);
    ease_camera.zoom = Some(5.0);
    map.ease_to(&ease_camera, Some(&immediate)).unwrap();
    let camera = map.camera().unwrap();
    assert_lat_lng_close(camera.center.unwrap(), eased_center);
    assert_eq!(camera.zoom, Some(5.0));

    let mut fly_camera = CameraOptions::default();
    fly_camera.center = Some(flown_center);
    fly_camera.zoom = Some(6.0);
    map.fly_to(&fly_camera, Some(&immediate)).unwrap();
    let camera = map.camera().unwrap();
    assert_lat_lng_close(camera.center.unwrap(), flown_center);
    assert_eq!(camera.zoom, Some(6.0));

    let mut cancel_camera = CameraOptions::default();
    cancel_camera.center = Some(center);
    let mut cancel_animation = AnimationOptions::default();
    cancel_animation.duration_ms = Some(1000.0);
    map.ease_to(&cancel_camera, Some(&cancel_animation))
        .unwrap();
    map.cancel_transitions().unwrap();

    assert!(!map.is_gesture_in_progress().unwrap());
    map.set_gesture_in_progress(true).unwrap();
    map.move_by(8.0, -4.0).unwrap();
    assert!(map.is_gesture_in_progress().unwrap());
    map.set_gesture_in_progress(false).unwrap();
    assert!(!map.is_gesture_in_progress().unwrap());

    let point = map.pixel_for_lat_lng(center).unwrap();
    let round_tripped = map.lat_lng_for_pixel(point).unwrap();
    assert!((round_tripped.latitude - center.latitude).abs() < 1e-7);
    assert!((round_tripped.longitude - center.longitude).abs() < 1e-7);

    let points = map.pixels_for_lat_lngs(&[center]).unwrap();
    let coordinates = map.lat_lngs_for_pixels(&points).unwrap();
    assert_eq!(points.len(), 1);
    assert!((coordinates[0].latitude - center.latitude).abs() < 1e-7);
    assert!((coordinates[0].longitude - center.longitude).abs() < 1e-7);

    map.close().unwrap();
    runtime.close().unwrap();
}

/// Camera events drained from one runtime queue, in arrival order.
#[derive(Default)]
struct CameraEventTally {
    finished_transition_ids: Vec<u64>,
    did_change_modes: Vec<CameraChangeMode>,
    did_change_followed_finish: bool,
}

/// Drains the queued runtime events and tallies the camera events among them.
///
/// The transition-finished event is queued while the camera command that ends
/// the transition runs, so polling alone observes it.
fn drain_camera_events(runtime: &RuntimeHandle) -> CameraEventTally {
    let mut tally = CameraEventTally::default();
    while let Some(event) = runtime.poll_event().unwrap() {
        match event.event_type {
            RuntimeEventType::MapCameraTransitionFinished => {
                let RuntimeEventPayload::CameraTransitionFinished(payload) = event.payload else {
                    panic!("transition-finished event should carry its typed payload");
                };
                tally.finished_transition_ids.push(payload.transition_id);
            }
            RuntimeEventType::MapCameraDidChange => {
                tally
                    .did_change_modes
                    .push(CameraChangeMode::from_raw(event.code as u32));
                tally.did_change_followed_finish |= !tally.finished_transition_ids.is_empty();
            }
            _ => {}
        }
    }
    tally
}

fn identified_ease(transition_id: u64, duration_ms: f64) -> AnimationOptions {
    let mut animation = AnimationOptions::default();
    animation.transition_id = Some(transition_id);
    animation.duration_ms = Some(duration_ms);
    animation
}

#[test]
// Spec coverage: BND-061, BND-102, and BND-087.
fn identified_camera_transitions_report_each_terminal_outcome_once() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let mut options = MapOptions::new(512, 512, 1.0);
    options.mode = MapMode::Continuous;
    let map = MapHandle::with_options(&runtime, &options).unwrap();
    let mut camera = CameraOptions::default();
    camera.center = Some(LatLng::new(45.0, -122.0));
    camera.zoom = Some(4.0);
    // Map construction queues its own camera events, so start from an empty
    // queue.
    let _ = drain_camera_events(&runtime);

    // A zero-duration ease resolves inside the call, so its end is reported
    // ahead of the did-change event for the same instant jump.
    map.ease_to(&camera, Some(&identified_ease(0, 0.0)))
        .unwrap();
    let tally = drain_camera_events(&runtime);
    assert_eq!(tally.finished_transition_ids, vec![0]);
    assert!(tally.did_change_followed_finish);
    assert_eq!(tally.did_change_modes, vec![CameraChangeMode::Immediate]);

    // A running transition stays silent until something ends it.
    camera.zoom = Some(12.0);
    map.ease_to(&camera, Some(&identified_ease(11, 5_000.0)))
        .unwrap();
    let tally = drain_camera_events(&runtime);
    assert!(tally.finished_transition_ids.is_empty());

    // A later camera command supersedes the running transition.
    camera.zoom = Some(13.0);
    map.ease_to(&camera, Some(&identified_ease(12, 5_000.0)))
        .unwrap();
    let tally = drain_camera_events(&runtime);
    assert_eq!(tally.finished_transition_ids, vec![11]);
    assert_eq!(tally.did_change_modes, vec![CameraChangeMode::Animated]);

    // Cancelling ends the superseding transition.
    map.cancel_transitions().unwrap();
    let tally = drain_camera_events(&runtime);
    assert_eq!(tally.finished_transition_ids, vec![12]);

    // Leaving the identity absent keeps the transition silent, so the present
    // zero ID above stayed distinguishable from an omitted one.
    camera.zoom = Some(14.0);
    map.ease_to(&camera, Some(&AnimationOptions::default()))
        .unwrap();
    let tally = drain_camera_events(&runtime);
    assert!(tally.finished_transition_ids.is_empty());

    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-104.
fn empty_coordinate_slice_is_rejected_before_calling_c() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();
    let mut fit = CameraFitOptions::default();
    fit.padding = Some(EdgeInsets::new(1.0, 1.0, 1.0, 1.0));

    let error = map.camera_for_lat_lngs(&[], Some(&fit)).unwrap_err();

    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    assert_eq!(error.raw_status(), None);
    assert!(error.diagnostic().contains("at least one coordinate"));

    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-102, BND-103.
fn unbounded_and_world_bounds_constrain_the_camera_differently() {
    fn jumped_longitude(map: &MapHandle, longitude: f64) -> f64 {
        let mut camera = CameraOptions::default();
        camera.center = Some(LatLng::new(0.0, longitude));
        camera.zoom = Some(2.0);
        map.jump_to(&camera).unwrap();
        map.camera().unwrap().center.unwrap().longitude
    }

    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();

    // A pristine map reports the unbounded constraint, not world bounds.
    assert_eq!(
        map.bounds().unwrap().bounds,
        Some(BoundsConstraint::Unbounded)
    );
    assert!((jumped_longitude(&map, 200.0) - -160.0).abs() < 1e-6);

    let world = LatLngBounds::new(LatLng::new(-90.0, -180.0), LatLng::new(90.0, 180.0));
    let mut options = BoundOptions::default();
    options.bounds = Some(BoundsConstraint::Bounded(world));
    map.set_bounds(&options).unwrap();
    assert_eq!(
        map.bounds().unwrap().bounds,
        Some(BoundsConstraint::Bounded(world))
    );
    // World bounds clamp at the antimeridian instead of wrapping.
    assert!((jumped_longitude(&map, 200.0) - 180.0).abs() < 1e-6);

    let mut options = BoundOptions::default();
    options.bounds = Some(BoundsConstraint::Unbounded);
    map.set_bounds(&options).unwrap();
    assert_eq!(
        map.bounds().unwrap().bounds,
        Some(BoundsConstraint::Unbounded)
    );
    // Releasing the constraint restores antimeridian wrapping.
    assert!((jumped_longitude(&map, 200.0) - -160.0).abs() < 1e-6);

    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-102.
fn projection_mode_round_trips_through_real_c_api() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();

    let mut projection_mode = ProjectionMode::default();
    projection_mode.axonometric = Some(false);
    projection_mode.x_skew = Some(0.0);
    projection_mode.y_skew = Some(0.0);
    map.set_projection_mode(&projection_mode).unwrap();
    let copied_projection_mode = map.projection_mode().unwrap();

    assert_eq!(copied_projection_mode.axonometric, Some(false));

    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-045.
fn a_released_map_id_replayed_after_a_new_map_reports_it_stale() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();

    let first = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();
    let released = first.inner.native().unwrap();
    first.close().unwrap();

    // The released slot is the one the next map takes, so the replayed id
    // names a retired generation of a slot that is live again.
    let second = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();

    let mut width = 0;
    let mut height = 0;
    let mut scale = 0.0;
    // SAFETY: the id is well-formed and the out-pointers are live locals; the
    // C API resolves the id rather than dereferencing it.
    let status = unsafe { sys::mln_map_get_size(released, &mut width, &mut height, &mut scale) };
    assert_eq!(status, sys::MLN_STATUS_INVALID_ARGUMENT);
    assert!(maplibre_native_core::error::capture_thread_diagnostic().contains("stale"));

    // The live map is unaffected by the replay.
    assert_eq!(second.size().unwrap().0, MapOptions::default().width);
    second.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-047.
fn a_map_id_passed_to_a_runtime_operation_reports_invalid_argument() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();

    // `mln_map` and `mln_runtime` are distinct newtypes, so this call has no
    // expression in the safe API and needs the raw id.
    let wrong_kind = sys::mln_runtime(map.inner.native().unwrap().0);
    // SAFETY: the value is well-formed; the C API rejects it on its kind tag.
    let status = unsafe { sys::mln_runtime_pump(wrong_kind, 0) };

    assert_eq!(status, sys::MLN_STATUS_INVALID_ARGUMENT);
    let message = maplibre_native_core::error::capture_thread_diagnostic();
    assert!(message.contains("map"), "{message}");
    assert!(message.contains("runtime"), "{message}");

    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-049.
fn a_live_map_id_called_from_another_thread_reports_wrong_thread() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();

    let live = map.inner.native().unwrap();
    let (status, message) = std::thread::scope(|scope| {
        scope
            .spawn(|| {
                let mut width = 0;
                let mut height = 0;
                let mut scale = 0.0;
                // SAFETY: the id is live; only the calling thread is wrong.
                let status =
                    unsafe { sys::mln_map_get_size(live, &mut width, &mut height, &mut scale) };
                (
                    status,
                    maplibre_native_core::error::capture_thread_diagnostic(),
                )
            })
            .join()
            .unwrap()
    });

    // The id is live, so the owner-thread rule decides rather than identity.
    assert_eq!(status, sys::MLN_STATUS_WRONG_THREAD);
    assert!(!message.contains("stale"), "{message}");

    map.close().unwrap();
    runtime.close().unwrap();
}
