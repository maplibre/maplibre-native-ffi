use super::*;
use serde_json::{Value as JsonValue, json};
use std::sync::Arc;
use std::sync::atomic::{AtomicUsize, Ordering};
use std::time::Duration;

use crate::events::{RuntimeEventPayload, RuntimeEventSource, RuntimeEventType};
use crate::{
    BoundsConstraint, CameraChangeMode, CustomGeometrySourceOptions, CustomMvtVectorSourceOptions,
    EdgeInsets, ErrorKind, MapMode, ResourceKind, ResourceProviderDecision, ResourceResponse,
    RuntimeEventMask, TextureImageInfo,
};

const VALID_STYLE_JSON: &str = r#"{"version":8,"sources":{},"layers":[]}"#;
const STYLE_WITH_IDS_JSON: &str = r#"{"version":8,"sources":{"geo":{"type":"geojson","data":{"type":"FeatureCollection","features":[]}}},"layers":[{"id":"background","type":"background"},{"id":"geo-fill","type":"fill","source":"geo"}]}"#;
const STYLE_WITH_TRANSITION_JSON: &str =
    r#"{"version":8,"transition":{"duration":750,"delay":100},"sources":{},"layers":[]}"#;
const STYLE_WITH_DELAY_ONLY_TRANSITION_JSON: &str =
    r#"{"version":8,"transition":{"delay":100},"sources":{},"layers":[]}"#;

#[test]
// Spec coverage: BND-105.
fn nine_patch_style_image_round_trips_stretch_content_and_text_fit() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();
    map.set_style_json(VALID_STYLE_JSON.as_bytes()).unwrap();

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
    map.set_style_json(STYLE_WITH_IDS_JSON.as_bytes()).unwrap();

    assert_eq!(map.layer_source_layer("geo-fill").unwrap(), "");
    map.set_layer_source_layer("geo-fill", "roads").unwrap();
    assert_eq!(map.layer_source_layer("geo-fill").unwrap(), "roads");
    assert_eq!(map.layer_source_id("geo-fill").unwrap(), "geo");

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
    value.as_object()?.get(key)
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

    map.set_style_json(VALID_STYLE_JSON.as_bytes()).unwrap();
    let _ = map.style_source_ids().unwrap();
    let _ = map.style_layer_ids().unwrap();

    map.set_style_json(STYLE_WITH_IDS_JSON.as_bytes()).unwrap();
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

    let error = map.set_style_json(b"{").unwrap_err();
    assert_eq!(error.kind(), ErrorKind::NativeError);
    assert_eq!(error.raw_status(), Some(sys::MLN_STATUS_NATIVE_ERROR));
    assert!(!error.diagnostic().trim().is_empty());

    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-101.
fn loaded_style_document_and_url_read_back_what_was_loaded() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();

    assert!(map.loaded_style_json().unwrap().is_empty());
    assert_eq!(map.style_url().unwrap(), "");

    // The document reads back byte-for-byte, so it can be reloaded unchanged.
    map.set_style_json(VALID_STYLE_JSON.as_bytes()).unwrap();
    assert_eq!(
        map.loaded_style_json().unwrap(),
        VALID_STYLE_JSON.as_bytes()
    );
    // Inline JSON clears the URL.
    assert_eq!(map.style_url().unwrap(), "");

    // The URL records the request before the load succeeds; the document still
    // reports the style that last parsed.
    map.set_style_url("https://example.com/style.json").unwrap();
    assert_eq!(map.style_url().unwrap(), "https://example.com/style.json");
    assert_eq!(
        map.loaded_style_json().unwrap(),
        VALID_STYLE_JSON.as_bytes()
    );

    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-105.
fn style_source_exists_and_remove_call_real_c_api() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();
    map.set_style_json(VALID_STYLE_JSON.as_bytes()).unwrap();

    let source = serde_json::to_vec(&json!({
        "type": "geojson",
        "data": {"type": "FeatureCollection", "features": []},
    }))
    .unwrap();

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
    map.set_style_json(VALID_STYLE_JSON.as_bytes()).unwrap();

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
    map.set_style_json(VALID_STYLE_JSON.as_bytes()).unwrap();

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
    map.set_style_json(VALID_STYLE_JSON.as_bytes()).unwrap();

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
fn geojson_source_helpers_accept_prepared_data_and_keep_options_across_updates() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();
    map.set_style_json(VALID_STYLE_JSON.as_bytes()).unwrap();

    let mut options = GeoJsonSourceOptions::default();
    options.cluster = Some(true);
    options.cluster_radius = Some(40);
    // A present zero must reach native as an explicit buffer of zero.
    options.buffer = Some(0);
    options.cluster_properties =
        Some(serde_json::to_vec(&json!({"weight_sum": ["+", ["get", "weight"]]})).unwrap());

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

    let bytes = serde_json::to_vec(&json!({
        "type": "FeatureCollection",
        "features": [{
            "type": "Feature",
            "geometry": {"type": "Point", "coordinates": [0.0, 0.0]},
            "properties": {"weight": 1},
        }],
    }))
    .unwrap();
    let data = crate::GeoJsonSourceDataHandle::new(&bytes, Some(&options)).unwrap();

    // One prepared value installs on any number of sources.
    map.add_geojson_source_data("geojson-data", &data).unwrap();
    map.add_geojson_source_data("geojson-data-2", &data)
        .unwrap();
    assert_eq!(
        map.style_source_type("geojson-data").unwrap(),
        Some(SourceType::GeoJson)
    );
    assert_eq!(
        map.style_source_type("geojson-data-2").unwrap(),
        Some(SourceType::GeoJson)
    );

    // Updates install prepared data; the source keeps the options the data it
    // was added with carried.
    map.set_geojson_source_data("geojson-data", &data).unwrap();
    map.set_geojson_source_data("geojson-url", &data).unwrap();
    map.set_geojson_source_url("geojson-data-2", "https://example.com/points.geojson")
        .unwrap();

    // Sources keep their own reference, so releasing the prepared data never
    // invalidates them.
    data.close();
    assert_eq!(
        map.style_source_type("geojson-data").unwrap(),
        Some(SourceType::GeoJson)
    );

    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-060 and BND-061.
fn set_geojson_source_data_rejects_mismatched_prepared_options() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();
    map.set_style_json(VALID_STYLE_JSON.as_bytes()).unwrap();

    let bytes = br#"{"type":"FeatureCollection","features":[]}"#;
    let mut options = GeoJsonSourceOptions::default();
    options.cluster = Some(true);
    options.cluster_radius = Some(40);
    let added = crate::GeoJsonSourceDataHandle::new(bytes, Some(&options)).unwrap();
    map.add_geojson_source_data("points", &added).unwrap();

    // Data prepared under different options would tile inconsistently with
    // the source's fixed options, so the install is rejected.
    let mut mismatched = options.clone();
    mismatched.cluster_radius = Some(80);
    let mismatched = crate::GeoJsonSourceDataHandle::new(bytes, Some(&mismatched)).unwrap();
    let error = map
        .set_geojson_source_data("points", &mismatched)
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);

    // Cluster aggregations are part of the comparison, so data prepared with
    // different cluster_properties is rejected too.
    let mut properties_only = options.clone();
    properties_only.cluster_properties =
        Some(serde_json::to_vec(&json!({"weight_sum": ["+", ["get", "weight"]]})).unwrap());
    let properties_only =
        crate::GeoJsonSourceDataHandle::new(bytes, Some(&properties_only)).unwrap();
    let error = map
        .set_geojson_source_data("points", &properties_only)
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);

    // The rejected install left the source usable with matching data.
    let matching = crate::GeoJsonSourceDataHandle::new(bytes, Some(&options)).unwrap();
    map.set_geojson_source_data("points", &matching).unwrap();

    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-060 and BND-061.
fn clustered_geojson_data_requires_a_feature_collection() {
    let mut options = GeoJsonSourceOptions::default();
    options.cluster = Some(true);

    // MapLibre Native clusters feature collections only; anything else would
    // tile unclustered instead of honouring the requested option, so
    // preparation rejects it. No runtime or map is required to observe this.
    let bare = br#"{"type":"Point","coordinates":[0.0,0.0]}"#;
    let error = crate::GeoJsonSourceDataHandle::new(bare, Some(&options)).unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    let message = error.to_string();
    assert!(
        message.contains("requires a feature collection"),
        "{message}"
    );
    assert!(message.contains("a bare geometry"), "{message}");

    let single = br#"{"type":"Feature","geometry":{"type":"Point","coordinates":[0.0,0.0]},"properties":{}}"#;
    let error = crate::GeoJsonSourceDataHandle::new(single, Some(&options)).unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    assert!(error.to_string().contains("a single feature"), "{error}");

    // The constraint belongs to clustering alone.
    crate::GeoJsonSourceDataHandle::new(bare, None).unwrap();

    // An empty feature collection is accepted; a later update supplies the
    // features to cluster.
    let empty = br#"{"type":"FeatureCollection","features":[]}"#;
    crate::GeoJsonSourceDataHandle::new(empty, Some(&options)).unwrap();
}

#[test]
// Spec coverage: BND-060 and BND-061.
fn clustered_geojson_data_reports_non_point_geometry() {
    let mut options = GeoJsonSourceOptions::default();
    options.cluster = Some(true);

    let mixed = br#"{"type":"FeatureCollection","features":[{"type":"Feature","geometry":{"type":"Point","coordinates":[0.0,0.0]},"properties":{}},{"type":"Feature","geometry":{"type":"GeometryCollection","geometries":[{"type":"Point","coordinates":[1.0,1.0]}]},"properties":{}}]}"#;

    // Supercluster reads every feature geometry as a point.
    let error = crate::GeoJsonSourceDataHandle::new(mixed, Some(&options)).unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    let message = error.to_string();
    assert!(
        message.contains("point geometry on every feature"),
        "{message}"
    );
    assert!(message.contains("geometry collection"), "{message}");

    // The constraint belongs to clustering alone.
    crate::GeoJsonSourceDataHandle::new(mixed, None).unwrap();
}

#[test]
// Rust regression: preparation is free of any runtime or map and the prepared
// value is immutable, so the handle transfers and shares across threads.
fn geojson_source_data_handle_is_send_and_sync() {
    static_assertions::assert_impl_all!(crate::GeoJsonSourceDataHandle: Send, Sync);
}

#[test]
// Spec coverage: BND-060 and BND-061.
fn geojson_data_prepared_off_thread_installs_on_the_map_thread() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();
    map.set_style_json(VALID_STYLE_JSON.as_bytes()).unwrap();

    let data = std::thread::spawn(|| {
        let bytes = br#"{"type":"FeatureCollection","features":[{"type":"Feature","geometry":{"type":"Point","coordinates":[0.0,0.0]},"properties":{}}]}"#;
        crate::GeoJsonSourceDataHandle::new(bytes, None).unwrap()
    })
    .join()
    .unwrap();

    map.add_geojson_source_data("worker-prepared", &data)
        .unwrap();
    assert_eq!(
        map.style_source_type("worker-prepared").unwrap(),
        Some(SourceType::GeoJson)
    );

    // The prepared value releases off the map thread as well.
    std::thread::spawn(move || drop(data)).join().unwrap();

    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-060 and BND-061.
fn synchronous_tiling_override_targets_live_geojson_sources_only() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();
    map.set_style_json(STYLE_WITH_IDS_JSON.as_bytes()).unwrap();

    map.set_geojson_source_synchronous_tiling("geo", true)
        .unwrap();
    map.set_geojson_source_synchronous_tiling("geo", false)
        .unwrap();

    let error = map
        .set_geojson_source_synchronous_tiling("missing", true)
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);

    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-105.
fn style_source_type_and_info_call_real_c_api() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();
    map.set_style_json(VALID_STYLE_JSON.as_bytes()).unwrap();

    let geojson_source = serde_json::to_vec(&json!({
        "type": "geojson",
        "data": {"type": "FeatureCollection", "features": []},
    }))
    .unwrap();
    let vector_source = serde_json::to_vec(&json!({
        "type": "vector",
        "tiles": ["https://example.com/{z}/{x}/{y}.pbf"],
        "attribution": "Example attribution",
    }))
    .unwrap();

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
// Spec coverage: BND-105.
fn style_source_volatility_round_trips_through_public_api() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();
    map.set_style_json(VALID_STYLE_JSON.as_bytes()).unwrap();
    map.add_vector_source_url("source", "https://example.com/source.json", None)
        .unwrap();

    assert!(
        !map.style_source_info("source")
            .unwrap()
            .unwrap()
            .is_volatile
    );

    map.set_style_source_volatile("source", true).unwrap();
    assert!(
        map.style_source_info("source")
            .unwrap()
            .unwrap()
            .is_volatile
    );

    map.set_style_source_volatile("source", false).unwrap();
    assert!(
        !map.style_source_info("source")
            .unwrap()
            .unwrap()
            .is_volatile
    );

    let error = map.set_style_source_volatile("missing", true).unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);

    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-066, BND-109.
fn style_source_info_copies_reconstructible_source_state() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();
    map.set_style_json(VALID_STYLE_JSON.as_bytes()).unwrap();

    map.add_vector_source_url("remote", "https://example.com/source.json", None)
        .unwrap();
    let remote = map.style_source_info("remote").unwrap().unwrap();
    assert_eq!(
        remote.url.as_deref(),
        Some("https://example.com/source.json")
    );
    assert_eq!(remote.tile_json, None);

    let mut options = TileSourceOptions::default();
    options.min_zoom = Some(2.0);
    options.max_zoom = Some(7.0);
    options.attribution = Some("Example attribution".to_owned());
    options.scheme = Some(TileScheme::Tms);
    options.bounds = Some(LatLngBounds::new(
        LatLng::new(-5.0, -10.0),
        LatLng::new(15.0, 20.0),
    ));
    options.vector_encoding = Some(VectorTileEncoding::Mlt);
    let tiles = [
        "https://a.example/{z}/{x}/{y}.mlt",
        "https://b.example/{z}/{x}/{y}.mlt",
    ];
    map.add_vector_source_tiles("inline", &tiles, Some(&options))
        .unwrap();
    let copied = map.style_source_info("inline").unwrap().unwrap();
    assert_eq!(copied.url, None);
    assert_eq!(copied.attribution.as_deref(), Some("Example attribution"));
    assert_eq!(copied.tile_size, Some(512));
    assert_eq!(copied.vector_encoding, Some(VectorTileEncoding::Mlt));
    let tile_json = copied.tile_json.as_ref().unwrap();
    assert_eq!(tile_json.tiles, tiles);
    assert_eq!(tile_json.min_zoom, 2.0);
    assert_eq!(tile_json.max_zoom, 7.0);
    assert_eq!(tile_json.scheme, TileScheme::Tms);
    assert_eq!(tile_json.bounds, options.bounds);

    assert!(map.remove_style_source("inline").unwrap());
    map.close().unwrap();
    runtime.close().unwrap();

    assert_eq!(
        copied.tile_json.unwrap().tiles,
        [
            "https://a.example/{z}/{x}/{y}.mlt".to_owned(),
            "https://b.example/{z}/{x}/{y}.mlt".to_owned(),
        ]
    );
}

/// Counts how often the C API released a custom geometry source's callback
/// state, by living inside the source's fetch callback: the release frees the
/// callback, which drops this probe.
struct ReleaseProbe {
    releases: Arc<AtomicUsize>,
}

impl Drop for ReleaseProbe {
    fn drop(&mut self) {
        self.releases.fetch_add(1, Ordering::SeqCst);
    }
}

/// Builds source options whose callback state increments `releases` when the C
/// API releases it.
fn options_counting_releases(releases: &Arc<AtomicUsize>) -> CustomGeometrySourceOptions {
    let probe = ReleaseProbe {
        releases: Arc::clone(releases),
    };
    CustomGeometrySourceOptions::new(move |_| {
        let _ = &probe;
    })
}

#[test]
// Spec coverage: BND-124.
fn custom_geometry_source_apis_call_real_c_api_and_style_replacement_releases_state() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();
    map.set_style_json(VALID_STYLE_JSON.as_bytes()).unwrap();
    let releases = Arc::new(AtomicUsize::new(0));

    let mut custom_options = options_counting_releases(&releases).with_cancel_tile(|_| {});
    custom_options.min_zoom = Some(0.0);
    custom_options.max_zoom = Some(2.0);
    custom_options.tolerance = Some(0.375);
    custom_options.tile_size = Some(512);
    custom_options.buffer = Some(64);
    custom_options.clip = Some(true);
    custom_options.wrap = Some(false);
    map.add_custom_geometry_source("custom", custom_options)
        .unwrap();

    let tile_id = CanonicalTileId::new(0, 0, 0);
    map.set_custom_geometry_source_tile_data(
        "custom",
        tile_id,
        br#"{"type":"FeatureCollection","features":[]}"#,
    )
    .unwrap();
    map.invalidate_custom_geometry_source_tile("custom", tile_id)
        .unwrap();
    map.invalidate_custom_geometry_source_region(
        "custom",
        LatLngBounds::new(LatLng::new(-1.0, -1.0), LatLng::new(1.0, 1.0)),
    )
    .unwrap();
    assert_eq!(releases.load(Ordering::SeqCst), 0);

    // An explicit removal releases the callback state once.
    assert!(map.remove_style_source("custom").unwrap());
    assert_eq!(releases.load(Ordering::SeqCst), 1);
    assert!(!map.style_source_exists("custom").unwrap());

    // A duplicate source ID is rejected, and a rejected add releases nothing
    // this call handed over, so the state it built is freed here instead.
    map.add_custom_geometry_source("custom", options_counting_releases(&releases))
        .unwrap();
    let error = map
        .add_custom_geometry_source("custom", options_counting_releases(&releases))
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    assert_eq!(releases.load(Ordering::SeqCst), 2);

    // The inline style load replaces the style before it returns, so the source
    // it dropped is released by then.
    map.set_style_json(VALID_STYLE_JSON.as_bytes()).unwrap();
    assert_eq!(releases.load(Ordering::SeqCst), 3);

    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-124.
fn custom_geometry_source_state_is_released_on_map_close() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();
    map.set_style_json(VALID_STYLE_JSON.as_bytes()).unwrap();
    let releases = Arc::new(AtomicUsize::new(0));
    map.add_custom_geometry_source("custom", options_counting_releases(&releases))
        .unwrap();

    map.close().unwrap();

    assert_eq!(releases.load(Ordering::SeqCst), 1);
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-124.
fn custom_geometry_source_adds_to_current_style_after_url_style_request() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();
    map.set_style_json(VALID_STYLE_JSON.as_bytes()).unwrap();
    map.set_style_url("unsupported://style.json").unwrap();
    let releases = Arc::new(AtomicUsize::new(0));

    map.add_custom_geometry_source("custom", options_counting_releases(&releases))
        .unwrap();

    assert!(map.style_source_exists("custom").unwrap());
    assert_eq!(releases.load(Ordering::SeqCst), 0);
    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-093 and BND-124.
fn custom_geometry_source_state_releases_after_url_style_replacement() {
    let mut runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    runtime
        .set_resource_provider(|request, handle| {
            if request.requested_url == "custom://style.json" {
                assert_eq!(request.kind, ResourceKind::Style);
                handle
                    .complete(ResourceResponse::ok(VALID_STYLE_JSON.as_bytes().to_vec()))
                    .unwrap();
            }
            ResourceProviderDecision::PassThrough
        })
        .unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();
    // The host wants no style-loaded events, and the release runs anyway.
    map.set_event_mask(RuntimeEventMask::ALL - RuntimeEventMask::MAP_STYLE_LOADED)
        .unwrap();
    map.set_style_json(VALID_STYLE_JSON.as_bytes()).unwrap();
    let releases = Arc::new(AtomicUsize::new(0));
    map.add_custom_geometry_source("custom", options_counting_releases(&releases))
        .unwrap();
    drain_runtime_events(&mut runtime);

    map.set_style_url("custom://style.json").unwrap();
    let mut style_loaded_events = 0;
    for _ in 0..1000 {
        runtime.pump(Some(Duration::ZERO), None).unwrap();
        for event in runtime.drain_events(0).unwrap().iter() {
            if event.event_type() == RuntimeEventType::MapStyleLoaded {
                style_loaded_events += 1;
            }
        }
        if releases.load(Ordering::SeqCst) == 1 {
            break;
        }
        std::thread::sleep(Duration::from_millis(1));
    }

    assert_eq!(
        releases.load(Ordering::SeqCst),
        1,
        "a style replacement should release the detached source's callback state"
    );
    assert_eq!(
        style_loaded_events, 0,
        "the release must not need an event the host left unselected"
    );
    assert_eq!(
        map.event_mask().unwrap(),
        RuntimeEventMask::ALL - RuntimeEventMask::MAP_STYLE_LOADED,
        "the mask should report what the host set"
    );

    map.close().unwrap();
    runtime.close().unwrap();
}

fn mvt_options_counting_releases(releases: &Arc<AtomicUsize>) -> CustomMvtVectorSourceOptions {
    let probe = ReleaseProbe {
        releases: Arc::clone(releases),
    };
    CustomMvtVectorSourceOptions::new(move |_| {
        let _ = &probe;
    })
}

#[test]
// Spec coverage: BND-105 and BND-124.
fn custom_mvt_vector_source_apis_call_real_c_api_and_style_replacement_releases_state() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();
    map.set_style_json(VALID_STYLE_JSON.as_bytes()).unwrap();
    let releases = Arc::new(AtomicUsize::new(0));

    let mut custom_options = mvt_options_counting_releases(&releases).with_cancel_tile(|_| {});
    custom_options.min_zoom = Some(0.0);
    custom_options.max_zoom = Some(2.0);
    map.add_custom_mvt_vector_source("custom", custom_options)
        .unwrap();
    assert_eq!(
        map.style_source_type("custom").unwrap(),
        Some(SourceType::CustomMvtVector)
    );

    let tile_id = CanonicalTileId::new(0, 0, 0);
    map.set_custom_mvt_vector_source_tile_data("custom", tile_id, &[])
        .unwrap();
    map.set_custom_mvt_vector_source_tile_error("custom", tile_id, "missing")
        .unwrap();
    map.invalidate_custom_mvt_vector_source_tile("custom", tile_id)
        .unwrap();
    assert_eq!(releases.load(Ordering::SeqCst), 0);

    assert!(map.remove_style_source("custom").unwrap());
    assert_eq!(releases.load(Ordering::SeqCst), 1);
    assert!(!map.style_source_exists("custom").unwrap());

    map.add_custom_mvt_vector_source("custom", mvt_options_counting_releases(&releases))
        .unwrap();
    let error = map
        .add_custom_mvt_vector_source("custom", mvt_options_counting_releases(&releases))
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    assert_eq!(releases.load(Ordering::SeqCst), 2);

    map.set_style_json(VALID_STYLE_JSON.as_bytes()).unwrap();
    assert_eq!(releases.load(Ordering::SeqCst), 3);

    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-124.
fn custom_mvt_vector_source_state_is_released_on_map_close() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();
    map.set_style_json(VALID_STYLE_JSON.as_bytes()).unwrap();
    let releases = Arc::new(AtomicUsize::new(0));
    map.add_custom_mvt_vector_source("custom", mvt_options_counting_releases(&releases))
        .unwrap();

    map.close().unwrap();

    assert_eq!(releases.load(Ordering::SeqCst), 1);
    runtime.close().unwrap();
}

fn drain_runtime_events(runtime: &mut RuntimeHandle) {
    for _ in 0..20 {
        runtime.pump(Some(Duration::ZERO), None).unwrap();
        let _ = runtime.drain_events(0).unwrap();
    }
}

/// Loads a style and collects the map event types the drains report, so a mask
/// test compares what a map delivered against what it selected.
fn collect_style_load_event_types(
    runtime: &mut RuntimeHandle,
    map: &MapHandle,
    style_json: &str,
) -> Vec<RuntimeEventType> {
    let mut types = Vec::new();
    map.set_style_json(style_json.as_bytes()).unwrap();
    for _ in 0..20 {
        runtime.pump(Some(Duration::ZERO), None).unwrap();
        for event in runtime.drain_events(0).unwrap().iter() {
            if event.source() == RuntimeEventSource::Map(map.id()) {
                types.push(event.event_type());
            }
        }
    }
    types
}

#[test]
// Spec coverage: BND-091.
fn a_narrowed_map_mask_delivers_the_kept_type_alone() {
    let mut runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();

    // The default options mask selects every type.
    assert_eq!(map.event_mask().unwrap(), RuntimeEventMask::ALL);

    map.set_event_mask(RuntimeEventMask::ALL - RuntimeEventMask::MAP_LOADING_STARTED)
        .unwrap();

    let types = collect_style_load_event_types(&mut runtime, &map, VALID_STYLE_JSON);

    assert!(
        types.contains(&RuntimeEventType::MapStyleLoaded),
        "{types:?}"
    );
    assert!(
        !types.contains(&RuntimeEventType::MapLoadingStarted),
        "{types:?}"
    );

    // An empty mask leaves the map with nothing to report.
    map.set_event_mask(RuntimeEventMask::NONE).unwrap();
    assert!(map.event_mask().unwrap().is_empty());
    let types = collect_style_load_event_types(&mut runtime, &map, STYLE_WITH_IDS_JSON);
    assert!(types.is_empty(), "{types:?}");

    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-060 and BND-091.
fn a_creation_mask_narrows_a_map_from_its_first_style_load() {
    let mut runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let mut options = MapOptions::default();
    options.event_mask = RuntimeEventMask::ALL - RuntimeEventMask::MAP_LOADING_STARTED;
    let map = MapHandle::with_options(&runtime, &options).unwrap();

    assert_eq!(
        map.event_mask().unwrap(),
        RuntimeEventMask::ALL - RuntimeEventMask::MAP_LOADING_STARTED
    );
    let types = collect_style_load_event_types(&mut runtime, &map, VALID_STYLE_JSON);

    assert!(
        types.contains(&RuntimeEventType::MapStyleLoaded),
        "{types:?}"
    );
    assert!(
        !types.contains(&RuntimeEventType::MapLoadingStarted),
        "{types:?}"
    );

    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-091.
fn a_map_mask_round_trips_and_rejects_undefined_bits() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();

    map.set_event_mask(RuntimeEventMask::ALL).unwrap();
    assert_eq!(map.event_mask().unwrap(), RuntimeEventMask::ALL);

    // Read, clear one bit, write back: every other bit survives.
    let mut mask = map.event_mask().unwrap();
    mask.remove(RuntimeEventMask::MAP_TILE_ACTION);
    map.set_event_mask(mask).unwrap();
    let read_back = map.event_mask().unwrap();
    assert!(!read_back.contains(RuntimeEventMask::MAP_TILE_ACTION));
    assert!(read_back.contains(RuntimeEventMask::MAP_STYLE_LOADED));
    assert!(read_back.contains(RuntimeEventMask::OFFLINE_OPERATION_COMPLETED));

    let error = map
        .set_event_mask(RuntimeEventMask::from_bits_retain(1 << 63))
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    assert_eq!(error.raw_status(), Some(sys::MLN_STATUS_INVALID_ARGUMENT));
    assert_eq!(map.event_mask().unwrap(), read_back);

    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-060, BND-061, BND-070, and BND-105.
fn style_transition_options_round_trip_through_the_real_c_api() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();

    // MapLibre Native always holds a placement flag, so it reports even before
    // a style is loaded.
    let empty = map.style_transition_options().unwrap();
    assert_eq!(empty.duration_ms, None);
    assert_eq!(empty.delay_ms, None);
    assert_eq!(empty.enable_placement_transitions, Some(true));

    // The style parser fills in its own 300ms duration when the style carries
    // no transition member.
    map.set_style_json(VALID_STYLE_JSON.as_bytes()).unwrap();
    let parsed = map.style_transition_options().unwrap();
    assert_eq!(parsed.duration_ms, Some(300.0));
    assert_eq!(parsed.delay_ms, None);

    // A transition member reports only what it names, so a delay-only
    // transition replaces that default with no duration.
    map.set_style_json(STYLE_WITH_DELAY_ONLY_TRANSITION_JSON.as_bytes())
        .unwrap();
    let delay_only = map.style_transition_options().unwrap();
    assert_eq!(delay_only.duration_ms, None);
    assert_eq!(delay_only.delay_ms, Some(100.0));

    map.set_style_json(STYLE_WITH_TRANSITION_JSON.as_bytes())
        .unwrap();
    let declared = map.style_transition_options().unwrap();
    assert_eq!(declared.duration_ms, Some(750.0));
    assert_eq!(declared.delay_ms, Some(100.0));
    assert_eq!(declared.enable_placement_transitions, Some(true));

    // A present zero stays distinguishable from an omitted field, and an
    // omitted field clears what the style declared rather than merging into it.
    let mut options = StyleTransitionOptions::default();
    options.duration_ms = Some(0.0);
    options.enable_placement_transitions = Some(false);
    map.set_style_transition_options(&options).unwrap();

    let applied = map.style_transition_options().unwrap();
    assert_eq!(applied, options);
    assert_eq!(applied.duration_ms, Some(0.0));
    assert_eq!(applied.delay_ms, None);
    assert_eq!(applied.enable_placement_transitions, Some(false));

    // Omitting the flag leaves the cross-fade on rather than clearing it.
    let mut duration_only = StyleTransitionOptions::default();
    duration_only.duration_ms = Some(250.0);
    map.set_style_transition_options(&duration_only).unwrap();
    assert_eq!(
        map.style_transition_options()
            .unwrap()
            .enable_placement_transitions,
        Some(true)
    );

    // Loading a style replaces the override with what that style declares.
    map.set_style_json(STYLE_WITH_TRANSITION_JSON.as_bytes())
        .unwrap();
    assert_eq!(map.style_transition_options().unwrap(), declared);

    let mut negative = StyleTransitionOptions::default();
    negative.delay_ms = Some(-1.0);
    let error = map.set_style_transition_options(&negative).unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    assert!(error.diagnostic().contains("delay_ms"));

    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-063, BND-064, and BND-105.
fn style_json_buffers_copy_owned_rust_values() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();
    map.set_style_json(STYLE_WITH_IDS_JSON.as_bytes()).unwrap();

    let layer = serde_json::to_vec(&json!({
        "id": "owned-background",
        "type": "background",
        "paint": {"background-opacity": 0.5},
    }))
    .unwrap();
    map.add_style_layer_json(&layer, None).unwrap();
    let copied_layer = map
        .style_layer_json("owned-background")
        .unwrap()
        .expect("added layer should have a JSON snapshot");
    let copied_layer: JsonValue = serde_json::from_slice(&copied_layer).unwrap();

    assert_eq!(
        object_member(&copied_layer, "id"),
        Some(&json!("owned-background"))
    );
    let paint = object_member(&copied_layer, "paint").expect("layer paint should be copied");
    assert_eq!(
        object_member(paint, "background-opacity"),
        Some(&json!(0.5))
    );

    map.set_layer_property("owned-background", "background-opacity", br#"0.75"#)
        .unwrap();
    assert_eq!(
        map.layer_property("owned-background", "background-opacity")
            .unwrap(),
        Some(b"0.75".to_vec())
    );

    let filter = br#"["==",["get","kind"],"park"]"#;
    map.set_layer_filter("geo-fill", Some(filter)).unwrap();
    assert_eq!(map.layer_filter("geo-fill").unwrap(), Some(filter.to_vec()));
    map.set_layer_filter("geo-fill", None).unwrap();
    assert_eq!(map.layer_filter("geo-fill").unwrap(), None);

    let error = map
        .set_layer_filter("owned-background", Some(b"NaN"))
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    assert_eq!(error.raw_status(), Some(sys::MLN_STATUS_INVALID_ARGUMENT));

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

/// Tallies the camera events among the queued runtime events.
///
/// The transition-finished event is queued while the camera command that ends
/// the transition runs, so one drain observes it.
fn drain_camera_events(runtime: &mut RuntimeHandle) -> CameraEventTally {
    let mut tally = CameraEventTally::default();
    for event in runtime.drain_events(0).unwrap().iter() {
        match event.event_type() {
            RuntimeEventType::MapCameraTransitionFinished => {
                let RuntimeEventPayload::CameraTransitionFinished(payload) = event.payload() else {
                    panic!("transition-finished event should carry its typed payload");
                };
                tally.finished_transition_ids.push(payload.transition_id);
            }
            RuntimeEventType::MapCameraDidChange => {
                tally
                    .did_change_modes
                    .push(CameraChangeMode::from_raw(event.code() as u32));
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
    let mut runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let mut options = MapOptions::new(512, 512, 1.0);
    options.mode = MapMode::Continuous;
    let map = MapHandle::with_options(&runtime, &options).unwrap();
    let mut camera = CameraOptions::default();
    camera.center = Some(LatLng::new(45.0, -122.0));
    camera.zoom = Some(4.0);
    // Map construction queues its own camera events, so start from an empty
    // queue.
    let _ = drain_camera_events(&mut runtime);

    // A zero-duration ease resolves inside the call, so its end is reported
    // ahead of the did-change event for the same jump.
    map.ease_to(&camera, Some(&identified_ease(0, 0.0)))
        .unwrap();
    let tally = drain_camera_events(&mut runtime);
    assert_eq!(tally.finished_transition_ids, vec![0]);
    assert!(tally.did_change_followed_finish);
    assert_eq!(tally.did_change_modes, vec![CameraChangeMode::Immediate]);

    camera.zoom = Some(12.0);
    map.ease_to(&camera, Some(&identified_ease(11, 5_000.0)))
        .unwrap();
    let tally = drain_camera_events(&mut runtime);
    assert!(tally.finished_transition_ids.is_empty());

    // A later camera command supersedes the running transition.
    camera.zoom = Some(13.0);
    map.ease_to(&camera, Some(&identified_ease(12, 5_000.0)))
        .unwrap();
    let tally = drain_camera_events(&mut runtime);
    assert_eq!(tally.finished_transition_ids, vec![11]);
    assert_eq!(tally.did_change_modes, vec![CameraChangeMode::Animated]);

    map.cancel_transitions().unwrap();
    let tally = drain_camera_events(&mut runtime);
    assert_eq!(tally.finished_transition_ids, vec![12]);

    // An absent identity keeps the transition silent, so the present zero ID
    // above stayed distinguishable from an omitted one.
    camera.zoom = Some(14.0);
    map.ease_to(&camera, Some(&AnimationOptions::default()))
        .unwrap();
    let tally = drain_camera_events(&mut runtime);
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

    // The next map takes the released slot, so the replayed id names a retired
    // generation of a slot that is live again.
    let second = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();

    let mut width = 0;
    let mut height = 0;
    let mut scale = 0.0;
    // SAFETY: the id is well-formed and the out-pointers are live locals; the
    // C API resolves the id rather than dereferencing it.
    let status = unsafe { sys::mln_map_get_size(released, &mut width, &mut height, &mut scale) };
    assert_eq!(status, sys::MLN_STATUS_INVALID_ARGUMENT);
    assert!(maplibre_native_ffi_core::error::capture_thread_diagnostic().contains("stale"));

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
    // expression in the safe API.
    let wrong_kind = sys::mln_runtime(map.inner.native().unwrap().0);
    // SAFETY: the value is well-formed; the C API rejects it on its kind tag.
    let status = unsafe { sys::mln_runtime_pump(wrong_kind, 0, -1) };

    assert_eq!(status, sys::MLN_STATUS_INVALID_ARGUMENT);
    let message = maplibre_native_ffi_core::error::capture_thread_diagnostic();
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
                    maplibre_native_ffi_core::error::capture_thread_diagnostic(),
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
