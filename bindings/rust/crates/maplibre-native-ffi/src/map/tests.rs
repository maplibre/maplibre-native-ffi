use super::*;
use serde_json::{Value as JsonValue, json};
use std::sync::Arc;
use std::sync::atomic::{AtomicUsize, Ordering};
use std::time::Duration;

use crate::custom_geometry::CustomGeometrySourceOptions;
use crate::events::{
    CommandDisposition, RuntimeEventPayload, RuntimeEventSource, RuntimeEventType,
};
use crate::{
    ErrorKind, RasterDemEncoding, ResourceKind, ResourceProviderDecision, ResourceResponse,
    RuntimeEventMask, TextureImageInfo,
};

const VALID_STYLE_JSON: &str = r#"{"version":8,"sources":{},"layers":[]}"#;
const STYLE_WITH_IDS_JSON: &str = r#"{"version":8,"sources":{"geo":{"type":"geojson","data":{"type":"FeatureCollection","features":[]}}},"layers":[{"id":"background","type":"background"},{"id":"geo-fill","type":"fill","source":"geo"}]}"#;
const STYLE_WITH_TRANSITION_JSON: &str =
    r#"{"version":8,"transition":{"duration":750,"delay":100},"sources":{},"layers":[]}"#;
const STYLE_WITH_DELAY_ONLY_TRANSITION_JSON: &str =
    r#"{"version":8,"transition":{"delay":100},"sources":{},"layers":[]}"#;

macro_rules! operation_result {
    ($operation:expr) => {{
        let operation = $operation.unwrap();
        assert!(operation.wait(Duration::from_secs(5)).unwrap());
        let result = operation.take();
        drop(operation);
        result
    }};
}
fn await_runtime_barrier(runtime: &RuntimeHandle) {
    let barrier = runtime.start_barrier().unwrap();
    assert!(barrier.wait(Duration::from_secs(5)).unwrap());
    barrier.discard().unwrap();
    barrier.release();
}
fn assert_command_disposition(
    runtime: &RuntimeHandle,
    command_id: u64,
    expected: CommandDisposition,
) -> Option<String> {
    await_runtime_barrier(runtime);
    let batch = runtime.drain_events(0).unwrap();
    let matches = batch
        .iter()
        .filter_map(|event| match event.payload() {
            RuntimeEventPayload::CommandFinished(finished) if finished.command_id == command_id => {
                Some((
                    finished.disposition,
                    event.message().unwrap().map(str::to_owned),
                ))
            }
            _ => None,
        })
        .collect::<Vec<_>>();
    assert_eq!(
        matches.len(),
        1,
        "terminal outcome count for command {command_id}"
    );
    assert_eq!(matches[0].0, expected);
    matches
        .into_iter()
        .next()
        .and_then(|(_, diagnostic)| diagnostic)
}

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

    let info = operation_result!(map.style_image_info("patch"))
        .unwrap()
        .unwrap();
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

    let (stretch_x, stretch_y) = operation_result!(map.style_image_stretches("patch"))
        .unwrap()
        .unwrap();
    assert_eq!(stretch_x, vec![ImageStretch::new(0.0, 1.0)]);
    assert_eq!(
        stretch_y,
        vec![ImageStretch::new(0.0, 1.0), ImageStretch::new(1.0, 2.0)]
    );
    assert!(
        operation_result!(map.style_image_stretches("missing"))
            .unwrap()
            .is_none()
    );

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

    assert_eq!(
        operation_result!(map.layer_source_layer("geo-fill")).unwrap(),
        ""
    );
    map.set_layer_source_layer("geo-fill", "roads").unwrap();
    assert_eq!(
        operation_result!(map.layer_source_layer("geo-fill")).unwrap(),
        "roads"
    );
    assert_eq!(
        operation_result!(map.layer_source_id("geo-fill")).unwrap(),
        "geo"
    );

    let rejected_command = map.set_layer_source_layer("background", "roads").unwrap();
    assert_ne!(rejected_command, 0);
    assert_eq!(
        operation_result!(map.layer_source_id("background")).unwrap(),
        ""
    );

    // An unset zoom range crosses the boundary as infinities.
    assert_eq!(
        operation_result!(map.layer_min_zoom("geo-fill")).unwrap(),
        f64::NEG_INFINITY
    );
    assert_eq!(
        operation_result!(map.layer_max_zoom("geo-fill")).unwrap(),
        f64::INFINITY
    );
    map.set_layer_min_zoom("geo-fill", 4.0).unwrap();
    map.set_layer_max_zoom("geo-fill", 12.5).unwrap();
    assert_eq!(
        operation_result!(map.layer_min_zoom("geo-fill")).unwrap(),
        4.0
    );
    assert_eq!(
        operation_result!(map.layer_max_zoom("geo-fill")).unwrap(),
        12.5
    );

    assert_eq!(
        operation_result!(map.layer_visibility("geo-fill")).unwrap(),
        StyleLayerVisibility::Visible
    );
    map.set_layer_visibility("geo-fill", StyleLayerVisibility::None)
        .unwrap();
    assert_eq!(
        operation_result!(map.layer_visibility("geo-fill")).unwrap(),
        StyleLayerVisibility::None
    );

    let rejected_command = map
        .set_layer_visibility("geo-fill", StyleLayerVisibility::Unknown(900))
        .unwrap();
    assert_command_disposition(&runtime, rejected_command, CommandDisposition::Failed);

    let error = operation_result!(map.layer_min_zoom("missing")).unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidState);
    assert!(error.diagnostic().contains("layer does not exist"));

    map.close().unwrap();
    runtime.close().unwrap();
}

fn object_member<'a>(value: &'a JsonValue, key: &str) -> Option<&'a JsonValue> {
    value.as_object()?.get(key)
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
    let _ = operation_result!(map.style_source_ids()).unwrap();
    let _ = operation_result!(map.style_layer_ids()).unwrap();

    map.set_style_json(STYLE_WITH_IDS_JSON.as_bytes()).unwrap();
    let source_ids = operation_result!(map.style_source_ids()).unwrap();
    let layer_ids = operation_result!(map.style_layer_ids()).unwrap();
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

    let malformed_id = map.set_style_json(b"{").unwrap();
    assert_ne!(malformed_id, 0);
    await_runtime_barrier(&runtime);

    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-101.
fn loaded_style_document_and_url_read_back_what_was_loaded() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();

    assert!(
        operation_result!(map.loaded_style_json())
            .unwrap()
            .is_empty()
    );
    assert_eq!(operation_result!(map.style_url()).unwrap(), "");

    // The document reads back byte-for-byte, so it can be reloaded unchanged.
    map.set_style_json(VALID_STYLE_JSON.as_bytes()).unwrap();
    assert_eq!(
        operation_result!(map.loaded_style_json()).unwrap(),
        VALID_STYLE_JSON.as_bytes()
    );
    // Inline JSON clears the URL.
    assert_eq!(operation_result!(map.style_url()).unwrap(), "");

    // The URL records the request before the load succeeds; the document still
    // reports the style that last parsed.
    map.set_style_url("https://example.com/style.json").unwrap();
    assert_eq!(
        operation_result!(map.style_url()).unwrap(),
        "https://example.com/style.json"
    );
    assert_eq!(
        operation_result!(map.loaded_style_json()).unwrap(),
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

    assert!(!operation_result!(map.style_source_exists("owned-source")).unwrap());
    assert!(!operation_result!(map.remove_style_source("owned-source")).unwrap());

    map.add_style_source_json("owned-source", &source).unwrap();
    assert!(operation_result!(map.style_source_exists("owned-source")).unwrap());
    assert!(operation_result!(map.remove_style_source("owned-source")).unwrap());
    assert!(!operation_result!(map.style_source_exists("owned-source")).unwrap());
    assert!(!operation_result!(map.remove_style_source("owned-source")).unwrap());

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
    assert!(operation_result!(map.style_image_exists("plain")).unwrap());
    let info = operation_result!(map.style_image_info("plain"))
        .unwrap()
        .expect("added image should have copied metadata");
    assert_eq!(info.width, image.info.width);
    assert_eq!(info.height, image.info.height);
    let mut copied = operation_result!(map.copy_style_image_premultiplied_rgba8("plain"))
        .unwrap()
        .expect("added Rust image should copy back through C");

    assert_eq!(copied.image.info.width, image.info.width);
    assert_eq!(copied.image.info.height, image.info.height);
    assert_eq!(copied.image.data, original_pixels);
    copied.image.data.fill(1);
    assert_eq!(
        operation_result!(map.copy_style_image_premultiplied_rgba8("plain"))
            .unwrap()
            .expect("style image copy should not expose native storage")
            .image
            .data,
        original_pixels
    );
    assert!(operation_result!(map.remove_style_image("plain")).unwrap());
    assert!(!operation_result!(map.style_image_exists("plain")).unwrap());
    assert!(!operation_result!(map.remove_style_image("plain")).unwrap());
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
        operation_result!(map.image_source_coordinates("url-image")).unwrap(),
        Some(coordinates)
    );

    let image = test_style_image(vec![1; 16]);
    map.add_image_source_image("inline-image", &coordinates, &image)
        .unwrap();
    assert_eq!(
        operation_result!(map.style_source_type("inline-image")).unwrap(),
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
        operation_result!(map.style_source_type("vector-url")).unwrap(),
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
        operation_result!(map.style_source_type("dem-tiles")).unwrap(),
        Some(SourceType::RasterDem)
    );
}

#[test]
// Spec coverage: BND-060, BND-061, and BND-105.
fn geojson_source_helpers_accept_options_and_keep_them_across_updates() {
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
        operation_result!(map.style_source_type("geojson-url")).unwrap(),
        Some(SourceType::GeoJson)
    );

    let data = serde_json::to_vec(&json!({
        "type": "FeatureCollection",
        "features": [{
            "type": "Feature",
            "geometry": {"type": "Point", "coordinates": [0.0, 0.0]},
            "properties": {"weight": 1},
        }],
    }))
    .unwrap();
    map.add_geojson_source_data("geojson-data", &data, None)
        .unwrap();
    assert_eq!(
        operation_result!(map.style_source_type("geojson-data")).unwrap(),
        Some(SourceType::GeoJson)
    );

    // Updates carry no options; the source keeps what it was added with.
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
    map.set_style_json(VALID_STYLE_JSON.as_bytes()).unwrap();

    let mut options = GeoJsonSourceOptions::default();
    options.cluster = Some(true);

    // MapLibre Native clusters feature collections only; anything else tiles
    // unclustered instead of honouring the requested option.
    let bare = br#"{"type":"Point","coordinates":[0.0,0.0]}"#;
    let bare_id = map
        .add_geojson_source_data("quakes", bare, Some(&options))
        .unwrap();
    assert_ne!(bare_id, 0);
    await_runtime_barrier(&runtime);
    assert!(!operation_result!(map.style_source_exists("quakes")).unwrap());

    let single = br#"{"type":"Feature","geometry":{"type":"Point","coordinates":[0.0,0.0]},"properties":{}}"#;
    let single_id = map
        .add_geojson_source_data("quakes", single, Some(&options))
        .unwrap();
    assert_ne!(single_id, 0);
    await_runtime_barrier(&runtime);
    assert!(!operation_result!(map.style_source_exists("quakes")).unwrap());

    // The constraint belongs to clustering alone, and the rejected ID stays free.
    map.add_geojson_source_data("quakes", bare, None).unwrap();

    // An empty feature collection is accepted; a later update supplies the
    // features to cluster.
    let empty = br#"{"type":"FeatureCollection","features":[]}"#;
    map.add_geojson_source_data("pending", empty, Some(&options))
        .unwrap();

    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-060 and BND-061.
fn clustered_geojson_source_reports_non_point_geometry() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();
    map.set_style_json(VALID_STYLE_JSON.as_bytes()).unwrap();

    let mut options = GeoJsonSourceOptions::default();
    options.cluster = Some(true);

    let mixed = br#"{"type":"FeatureCollection","features":[{"type":"Feature","geometry":{"type":"Point","coordinates":[0.0,0.0]},"properties":{}},{"type":"Feature","geometry":{"type":"GeometryCollection","geometries":[{"type":"Point","coordinates":[1.0,1.0]}]},"properties":{}}]}"#;

    // Supercluster reads every feature geometry as a point.
    let mixed_id = map
        .add_geojson_source_data("quakes", mixed, Some(&options))
        .unwrap();
    assert_ne!(mixed_id, 0);
    await_runtime_barrier(&runtime);
    assert!(!operation_result!(map.style_source_exists("quakes")).unwrap());

    // The constraint belongs to clustering alone, and the rejected ID stays free.
    map.add_geojson_source_data("quakes", mixed, None).unwrap();

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

    assert_eq!(
        operation_result!(map.style_source_type("missing-source")).unwrap(),
        None
    );
    assert_eq!(
        operation_result!(map.style_source_info("missing-source")).unwrap(),
        None
    );

    map.add_style_source_json("empty", &geojson_source).unwrap();
    assert_eq!(
        operation_result!(map.style_source_type("empty")).unwrap(),
        Some(SourceType::GeoJson)
    );
    let info = operation_result!(map.style_source_info("empty"))
        .unwrap()
        .unwrap();
    assert_eq!(info.source_type, SourceType::GeoJson);
    assert_eq!(info.raw_source_type, sys::MLN_STYLE_SOURCE_TYPE_GEOJSON);
    assert!(!info.is_volatile);
    assert_eq!(info.attribution, None);

    map.add_style_source_json("vector-meta", &vector_source)
        .unwrap();
    assert_eq!(
        operation_result!(map.style_source_type("vector-meta")).unwrap(),
        Some(SourceType::Vector)
    );
    let info = operation_result!(map.style_source_info("vector-meta"))
        .unwrap()
        .unwrap();
    assert_eq!(info.source_type, SourceType::Vector);
    assert_eq!(info.raw_source_type, sys::MLN_STYLE_SOURCE_TYPE_VECTOR);
    assert_eq!(info.attribution.as_deref(), Some("Example attribution"));

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
    let remote = operation_result!(map.style_source_info("remote"))
        .unwrap()
        .unwrap();
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
    let copied = operation_result!(map.style_source_info("inline"))
        .unwrap()
        .unwrap();
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

    assert!(operation_result!(map.remove_style_source("inline")).unwrap());
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
    assert!(operation_result!(map.remove_style_source("custom")).unwrap());
    assert_eq!(releases.load(Ordering::SeqCst), 1);
    assert!(!operation_result!(map.style_source_exists("custom")).unwrap());

    let first_add = map
        .add_custom_geometry_source("custom", options_counting_releases(&releases))
        .unwrap();
    let duplicate_add = map
        .add_custom_geometry_source("custom", options_counting_releases(&releases))
        .unwrap();
    assert_ne!(first_add, duplicate_add);
    await_runtime_barrier(&runtime);
    assert_eq!(releases.load(Ordering::SeqCst), 2);

    map.set_style_json(VALID_STYLE_JSON.as_bytes()).unwrap();
    await_runtime_barrier(&runtime);
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

    assert!(operation_result!(map.style_source_exists("custom")).unwrap());
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
        std::thread::sleep(std::time::Duration::from_millis(1));
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

    map.close().unwrap();
    runtime.close().unwrap();
}

fn drain_runtime_events(runtime: &mut RuntimeHandle) {
    for _ in 0..20 {
        std::thread::sleep(std::time::Duration::from_millis(1));
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
        std::thread::sleep(std::time::Duration::from_millis(1));
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
fn map_mask_commands_return_ids_and_reject_undefined_bits() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();

    let first_id = map.set_event_mask(RuntimeEventMask::ALL).unwrap();
    let second_id = map
        .set_event_mask(RuntimeEventMask::ALL - RuntimeEventMask::MAP_TILE_ACTION)
        .unwrap();
    assert!(second_id > first_id);

    let error = map
        .set_event_mask(RuntimeEventMask::from_bits_retain(1 << 63))
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    assert_eq!(error.raw_status(), Some(sys::MLN_STATUS_INVALID_ARGUMENT));

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
    let empty = operation_result!(map.style_transition_options()).unwrap();
    assert_eq!(empty.duration_ms, None);
    assert_eq!(empty.delay_ms, None);
    assert_eq!(empty.enable_placement_transitions, Some(true));

    // The style parser fills in its own 300ms duration when the style carries
    // no transition member.
    map.set_style_json(VALID_STYLE_JSON.as_bytes()).unwrap();
    let parsed = operation_result!(map.style_transition_options()).unwrap();
    assert_eq!(parsed.duration_ms, Some(300.0));
    assert_eq!(parsed.delay_ms, None);

    // A transition member reports only what it names, so a delay-only
    // transition replaces that default with no duration.
    map.set_style_json(STYLE_WITH_DELAY_ONLY_TRANSITION_JSON.as_bytes())
        .unwrap();
    let delay_only = operation_result!(map.style_transition_options()).unwrap();
    assert_eq!(delay_only.duration_ms, None);
    assert_eq!(delay_only.delay_ms, Some(100.0));

    map.set_style_json(STYLE_WITH_TRANSITION_JSON.as_bytes())
        .unwrap();
    let declared = operation_result!(map.style_transition_options()).unwrap();
    assert_eq!(declared.duration_ms, Some(750.0));
    assert_eq!(declared.delay_ms, Some(100.0));
    assert_eq!(declared.enable_placement_transitions, Some(true));

    // A present zero stays distinguishable from an omitted field, and an
    // omitted field clears what the style declared rather than merging into it.
    let mut options = StyleTransitionOptions::default();
    options.duration_ms = Some(0.0);
    options.enable_placement_transitions = Some(false);
    map.set_style_transition_options(&options).unwrap();

    let applied = operation_result!(map.style_transition_options()).unwrap();
    assert_eq!(applied, options);
    assert_eq!(applied.duration_ms, Some(0.0));
    assert_eq!(applied.delay_ms, None);
    assert_eq!(applied.enable_placement_transitions, Some(false));

    // Omitting the flag leaves the cross-fade on rather than clearing it.
    let mut duration_only = StyleTransitionOptions::default();
    duration_only.duration_ms = Some(250.0);
    map.set_style_transition_options(&duration_only).unwrap();
    assert_eq!(
        operation_result!(map.style_transition_options())
            .unwrap()
            .enable_placement_transitions,
        Some(true)
    );

    // Loading a style replaces the override with what that style declares.
    map.set_style_json(STYLE_WITH_TRANSITION_JSON.as_bytes())
        .unwrap();
    assert_eq!(
        operation_result!(map.style_transition_options()).unwrap(),
        declared
    );

    let mut negative = StyleTransitionOptions::default();
    negative.delay_ms = Some(-1.0);
    let rejected_command = map.set_style_transition_options(&negative).unwrap();
    let diagnostic =
        assert_command_disposition(&runtime, rejected_command, CommandDisposition::Failed);
    assert!(diagnostic.unwrap().contains("delay_ms"));

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
    let copied_layer = operation_result!(map.style_layer_json("owned-background"))
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
        operation_result!(map.layer_property("owned-background", "background-opacity")).unwrap(),
        Some(b"0.75".to_vec())
    );

    let filter = br#"["==",["get","kind"],"park"]"#;
    map.set_layer_filter("geo-fill", Some(filter)).unwrap();
    assert_eq!(
        operation_result!(map.layer_filter("geo-fill")).unwrap(),
        Some(filter.to_vec())
    );
    map.set_layer_filter("geo-fill", None).unwrap();
    assert_eq!(
        operation_result!(map.layer_filter("geo-fill")).unwrap(),
        None
    );

    let rejected_command = map
        .set_layer_filter("owned-background", Some(b"NaN"))
        .unwrap();
    assert_ne!(rejected_command, 0);
    assert_eq!(
        operation_result!(map.layer_filter("owned-background")).unwrap(),
        None
    );

    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
fn camera_commands_return_ids_and_ordered_queries_take_typed_results() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();
    let center = LatLng::new(45.0, -122.0);
    let mut update = CameraUpdate::default();
    update.camera.center = Some(center);
    update.camera.zoom = Some(4.0);

    let command_id = map.update_camera(&update).unwrap();
    assert_ne!(command_id, 0);

    let query = map.start_camera_query().unwrap();
    assert!(query.wait(Duration::from_secs(5)).unwrap());
    let queried = query.take().unwrap();
    query.release();
    assert_eq!(queried.camera.center, Some(center));
    assert_eq!(queried.camera.zoom, Some(4.0));

    let published = map.camera_snapshot().unwrap();
    assert!(published.generation <= queried.generation);
    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-045.
fn map_accepts_concurrent_commands_and_cross_thread_snapshots() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();

    std::thread::scope(|scope| {
        let first = scope.spawn(|| map.request_repaint().unwrap());
        let second = scope.spawn(|| map.request_repaint().unwrap());
        let first_id = first.join().unwrap();
        let second_id = second.join().unwrap();
        assert_ne!(first_id, 0);
        assert_ne!(second_id, 0);
        await_runtime_barrier(&runtime);
        assert_ne!(second_id, first_id);

        let snapshot = scope.spawn(|| map.snapshot().unwrap()).join().unwrap();
        assert_eq!(snapshot.logical_extent.width, MapOptions::default().width);
    });
    map.close().unwrap();
    runtime.close().unwrap();
}
