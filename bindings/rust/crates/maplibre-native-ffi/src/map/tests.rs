use super::*;
use serde_json::{Value as JsonValue, json};
use std::sync::atomic::{AtomicUsize, Ordering};
use std::sync::{Arc, Mutex};
use std::time::Duration;

use crate::PremultipliedRgba8Image;
use crate::custom_geometry::{CanonicalTileId, CustomGeometrySourceOptions};
use crate::events::{CommandDisposition, RuntimeEventSource, RuntimeEventType};
use crate::test_support::await_runtime_barrier;
use crate::{
    CommandCompletion, CustomMvtVectorSourceOptions, ErrorKind, NativeFuture, RasterDemEncoding,
    ResourceKind, ResourceProviderDecision, ResourceResponse, RuntimeEventMask, TextureImageInfo,
};

// Preparation is free of any runtime or map and the prepared value is
// immutable, so the handle transfers and shares across threads.
static_assertions::assert_impl_all!(crate::GeoJsonSourceDataHandle: Send, Sync);

const VALID_STYLE_JSON: &str = r#"{"version":8,"sources":{},"layers":[]}"#;
const STYLE_WITH_IDS_JSON: &str = r#"{"version":8,"sources":{"geo":{"type":"geojson","data":{"type":"FeatureCollection","features":[]}}},"layers":[{"id":"background","type":"background"},{"id":"geo-fill","type":"fill","source":"geo"}]}"#;
const STYLE_WITH_TRANSITION_JSON: &str =
    r#"{"version":8,"transition":{"duration":750,"delay":100},"sources":{},"layers":[]}"#;
const STYLE_WITH_DELAY_ONLY_TRANSITION_JSON: &str =
    r#"{"version":8,"transition":{"delay":100},"sources":{},"layers":[]}"#;

/// Waits for exactly `expected` source callback-state releases.
///
/// A command's completion reports the command, not the release of the callback
/// state it dropped, and native may free that state on another thread, so the
/// count is only eventually consistent with the commands the test submitted.
fn await_release_count(runtime: &RuntimeHandle, releases: &Arc<AtomicUsize>, expected: usize) {
    let deadline = std::time::Instant::now() + Duration::from_secs(5);
    loop {
        await_runtime_barrier(runtime);
        let observed = releases.load(Ordering::SeqCst);
        if observed >= expected {
            assert_eq!(observed, expected);
            return;
        }
        assert!(
            std::time::Instant::now() < deadline,
            "expected {expected} releases, observed {observed}"
        );
        std::thread::sleep(Duration::from_millis(5));
    }
}
/// Awaits one ordered command and asserts the disposition it committed with,
/// returning the completion so a caller can assert its status or diagnostic.
fn assert_command_disposition(
    completion: NativeFuture<CommandCompletion>,
    expected: CommandDisposition,
) -> CommandCompletion {
    let completion = crate::completion::blocking(Ok(completion));
    assert_eq!(completion.disposition, expected);
    completion
}

#[test]
// Spec coverage: BND-105.
fn nine_patch_style_image_round_trips_stretch_content_and_text_fit() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map =
        crate::completion::blocking(MapHandle::with_options(&runtime, &MapOptions::default()));
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

    let saved = crate::completion::blocking(map.style_image("patch")).unwrap();
    assert_eq!(saved.stretch_x, vec![ImageStretch::new(0.0, 1.0)]);
    assert_eq!(saved.stretch_y.len(), 2);
    assert_eq!(
        saved.content,
        Some(ImageContent {
            left: 0.5,
            top: 0.5,
            right: 1.5,
            bottom: 1.5,
        })
    );
    // An absent text fit stays distinguishable from a present default.
    assert_eq!(saved.text_fit_width, None);
    assert_eq!(saved.text_fit_height, Some(StyleImageTextFit::Proportional));
    assert!(crate::completion::blocking(map.style_image("missing")).is_none());

    let mut bad = StyleImageOptions::default();
    bad.stretch_x = Some(vec![ImageStretch::new(2.0, 1.0)]);
    let error = map.set_style_image("bad", &image, Some(&bad)).unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);

    map.close_and_wait();
    runtime.close_and_wait();
}

#[test]
// Spec coverage: BND-105.
fn layer_base_accessors_round_trip_through_real_c_abi() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map =
        crate::completion::blocking(MapHandle::with_options(&runtime, &MapOptions::default()));
    map.set_style_json(STYLE_WITH_IDS_JSON.as_bytes()).unwrap();

    assert_eq!(
        crate::completion::blocking(map.layer_source_layer("geo-fill")),
        ""
    );
    map.set_layer_source_layer("geo-fill", "roads").unwrap();
    assert_eq!(
        crate::completion::blocking(map.layer_source_layer("geo-fill")),
        "roads"
    );
    assert_eq!(
        crate::completion::blocking(map.layer_source_id("geo-fill")),
        "geo"
    );

    let rejected_command = map.set_layer_source_layer("background", "roads").unwrap();
    assert_command_disposition(rejected_command, CommandDisposition::Failed);
    assert_eq!(
        crate::completion::blocking(map.layer_source_id("background")),
        ""
    );

    // An unset zoom range crosses the boundary as infinities, and the info
    // aggregate resolves source IDs alongside the scalar fields.
    let info = crate::completion::blocking(map.style_layer_info("geo-fill"))
        .expect("styled layer should report info");
    assert_eq!(info.layer_type, "fill");
    assert_eq!(info.min_zoom, f64::NEG_INFINITY);
    assert_eq!(info.max_zoom, f64::INFINITY);
    assert_eq!(info.visibility, StyleLayerVisibility::Visible);
    assert_eq!(info.source_id.as_deref(), Some("geo"));
    assert_eq!(info.source_layer.as_deref(), Some("roads"));

    map.set_layer_min_zoom("geo-fill", 4.0).unwrap();
    map.set_layer_max_zoom("geo-fill", 12.5).unwrap();
    map.set_layer_visibility("geo-fill", StyleLayerVisibility::None)
        .unwrap();
    let info = crate::completion::blocking(map.style_layer_info("geo-fill")).unwrap();
    assert_eq!(info.min_zoom, 4.0);
    assert_eq!(info.max_zoom, 12.5);
    assert_eq!(info.visibility, StyleLayerVisibility::None);

    // A sourceless layer reports absent source IDs rather than empty ones.
    let info = crate::completion::blocking(map.style_layer_info("background")).unwrap();
    assert_eq!(info.layer_type, "background");
    assert_eq!(info.source_id, None);
    assert_eq!(info.source_layer, None);

    let rejected_command = map
        .set_layer_visibility("geo-fill", StyleLayerVisibility::Unknown(900))
        .unwrap();
    assert_command_disposition(rejected_command, CommandDisposition::Failed);

    // The not-found path reports None instead of an error.
    assert!(crate::completion::blocking(map.style_layer_info("missing")).is_none());

    map.close_and_wait();
    runtime.close_and_wait();
}

fn object_member<'a>(value: &'a JsonValue, key: &str) -> Option<&'a JsonValue> {
    value.as_object()?.get(key)
}

#[test]
// Spec coverage: BND-040, BND-050, and BND-100.
fn map_close_consumes_handle_and_drop_stays_idempotent() {
    let leaks = Arc::new(Mutex::new(Vec::new()));
    let sink = Arc::clone(&leaks);
    assert!(!crate::set_leak_reporter(Some(Box::new(move |leak| {
        sink.lock().unwrap().push(leak);
    }))));

    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map =
        crate::completion::blocking(MapHandle::with_options(&runtime, &MapOptions::default()));

    // close() consumes the handle and retires the map, so the wrapper's own
    // drop that follows finds nothing left to release and reports no leak.
    map.close_and_wait();
    crate::set_leak_reporter(None);
    assert!(leaks.lock().unwrap().is_empty());

    runtime.close_and_wait();
}

#[test]
// Spec coverage: BND-042.
fn map_retains_runtime_after_runtime_handle_is_dropped() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map =
        crate::completion::blocking(MapHandle::with_options(&runtime, &MapOptions::default()));

    drop(runtime);

    map.close_and_wait();
}

#[test]
// Spec coverage: BND-024, BND-101, and BND-105.
fn style_setters_accept_valid_input_and_reject_embedded_nul() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map =
        crate::completion::blocking(MapHandle::with_options(&runtime, &MapOptions::default()));

    map.set_style_json(VALID_STYLE_JSON.as_bytes()).unwrap();
    let _ = crate::completion::blocking(map.style_source_ids());
    let _ = crate::completion::blocking(map.style_layer_ids());

    map.set_style_json(STYLE_WITH_IDS_JSON.as_bytes()).unwrap();
    let source_ids = crate::completion::blocking(map.style_source_ids());
    let layer_ids = crate::completion::blocking(map.style_layer_ids());
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
    assert_command_disposition(malformed_id, CommandDisposition::Failed);

    map.close_and_wait();
    runtime.close_and_wait();
}

#[test]
// Spec coverage: BND-101.
fn loaded_style_document_and_url_read_back_what_was_loaded() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map =
        crate::completion::blocking(MapHandle::with_options(&runtime, &MapOptions::default()));

    assert!(crate::completion::blocking(map.loaded_style_json()).is_empty());
    assert_eq!(crate::completion::blocking(map.style_url()), "");

    // The document reads back byte-for-byte, so it can be reloaded unchanged.
    map.set_style_json(VALID_STYLE_JSON.as_bytes()).unwrap();
    assert_eq!(
        crate::completion::blocking(map.loaded_style_json()),
        VALID_STYLE_JSON.as_bytes()
    );
    // Inline JSON clears the URL.
    assert_eq!(crate::completion::blocking(map.style_url()), "");

    // The URL records the request before the load succeeds; the document still
    // reports the style that last parsed.
    map.set_style_url("https://example.com/style.json").unwrap();
    assert_eq!(
        crate::completion::blocking(map.style_url()),
        "https://example.com/style.json"
    );
    assert_eq!(
        crate::completion::blocking(map.loaded_style_json()),
        VALID_STYLE_JSON.as_bytes()
    );

    map.close_and_wait();
    runtime.close_and_wait();
}

#[test]
// Spec coverage: BND-105.
fn style_removals_commit_or_fail_with_not_found() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map =
        crate::completion::blocking(MapHandle::with_options(&runtime, &MapOptions::default()));
    map.set_style_json(STYLE_WITH_IDS_JSON.as_bytes()).unwrap();

    let source = serde_json::to_vec(&json!({
        "type": "geojson",
        "data": {"type": "FeatureCollection", "features": []},
    }))
    .unwrap();

    // Removing a missing source fails with the not-found status code.
    let missing = map.remove_style_source("owned-source").unwrap();
    let completion = assert_command_disposition(missing, CommandDisposition::Failed);
    assert_eq!(completion.raw_status, sys::MLN_STATUS_NOT_FOUND);

    // Removing an existing source commits, and the info getter's found flag
    // re-checks existence on both sides of the removal.
    map.add_style_source_json("owned-source", &source).unwrap();
    assert!(crate::completion::blocking(map.style_source_info("owned-source")).is_some());
    let removed = map.remove_style_source("owned-source").unwrap();
    let completion = assert_command_disposition(removed, CommandDisposition::Committed);
    assert_ne!(completion.generation, 0);
    assert!(crate::completion::blocking(map.style_source_info("owned-source")).is_none());

    // Removing a source a layer still uses fails with invalid state.
    let in_use = map.remove_style_source("geo").unwrap();
    let completion = assert_command_disposition(in_use, CommandDisposition::Failed);
    assert_eq!(completion.raw_status, sys::MLN_STATUS_INVALID_STATE);
    assert!(completion.diagnostic.contains("used by a layer"));

    // Layer removal commits and then reports not-found for the same ID.
    let removed = map.remove_style_layer("geo-fill").unwrap();
    assert_command_disposition(removed, CommandDisposition::Committed);
    assert!(crate::completion::blocking(map.style_layer_info("geo-fill")).is_none());
    let missing = map.remove_style_layer("geo-fill").unwrap();
    let completion = assert_command_disposition(missing, CommandDisposition::Failed);
    assert_eq!(completion.raw_status, sys::MLN_STATUS_NOT_FOUND);

    map.close_and_wait();
    runtime.close_and_wait();
}

fn source_type_of(map: &MapHandle, source_id: &str) -> Option<SourceType> {
    crate::completion::blocking(map.style_source_info(source_id)).map(|info| info.source_type)
}

fn test_style_image(data: Vec<u8>) -> PremultipliedRgba8Image {
    PremultipliedRgba8Image::new(TextureImageInfo::new(2, 2, 8, data.len()), data)
}

#[test]
// Spec coverage: BND-069 and BND-105.
fn style_image_copy_uses_rust_owned_buffer() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map =
        crate::completion::blocking(MapHandle::with_options(&runtime, &MapOptions::default()));
    map.set_style_json(VALID_STYLE_JSON.as_bytes()).unwrap();

    let original_pixels = vec![
        255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255, 255, 255, 255, 255,
    ];
    let mut image = test_style_image(original_pixels.clone());

    map.set_style_image("plain", &image, None).unwrap();
    image.data.fill(0);
    let mut copied = crate::completion::blocking(map.style_image("plain"))
        .expect("added Rust image should copy back through C");

    assert_eq!(copied.image.info.width, image.info.width);
    assert_eq!(copied.image.info.height, image.info.height);
    assert_eq!(copied.image.data, original_pixels);
    copied.image.data.fill(1);
    assert_eq!(
        crate::completion::blocking(map.style_image("plain"))
            .expect("style image copy should not expose native storage")
            .image
            .data,
        original_pixels
    );
    // Image removal commits, the info getter's found flag reports the image
    // gone, and a repeat removal fails with the not-found status code.
    let removed = map.remove_style_image("plain").unwrap();
    assert_command_disposition(removed, CommandDisposition::Committed);
    assert!(crate::completion::blocking(map.style_image("plain")).is_none());
    let missing = map.remove_style_image("plain").unwrap();
    let completion = assert_command_disposition(missing, CommandDisposition::Failed);
    assert_eq!(completion.raw_status, sys::MLN_STATUS_NOT_FOUND);

    map.close_and_wait();
    runtime.close_and_wait();
}

#[test]
// Spec coverage: BND-105.
fn image_source_helpers_accept_url_and_inline_images() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map =
        crate::completion::blocking(MapHandle::with_options(&runtime, &MapOptions::default()));
    map.set_style_json(VALID_STYLE_JSON.as_bytes()).unwrap();

    let coordinates = [
        LatLng::new(0.0, 0.0),
        LatLng::new(0.0, 1.0),
        LatLng::new(1.0, 1.0),
        LatLng::new(1.0, 0.0),
    ];
    map.add_image_source_url("url-image", &coordinates, "https://example.com/image.png")
        .unwrap();
    assert_eq!(
        crate::completion::blocking(map.image_source_coordinates("url-image")),
        Some(coordinates)
    );

    let image = test_style_image(vec![1; 16]);
    map.add_image_source_image("inline-image", &coordinates, &image)
        .unwrap();
    assert_eq!(
        source_type_of(&map, "inline-image"),
        Some(SourceType::Image)
    );

    map.close_and_wait();
    runtime.close_and_wait();
}

#[test]
// Spec coverage: BND-105.
fn tile_source_helpers_call_real_c_api() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map =
        crate::completion::blocking(MapHandle::with_options(&runtime, &MapOptions::default()));
    map.set_style_json(VALID_STYLE_JSON.as_bytes()).unwrap();

    map.add_vector_source_url("vector-url", "https://example.com/vector.json", None)
        .unwrap();
    assert_eq!(source_type_of(&map, "vector-url"), Some(SourceType::Vector));

    let mut dem_options = TileSourceOptions::default();
    dem_options.raster_dem_encoding = Some(RasterDemEncoding::Terrarium);
    map.add_raster_dem_source_tiles(
        "dem-tiles",
        &["https://example.com/dem/{z}/{x}/{y}.png"],
        Some(&dem_options),
    )
    .unwrap();
    assert_eq!(
        source_type_of(&map, "dem-tiles"),
        Some(SourceType::RasterDem)
    );

    map.close_and_wait();
    runtime.close_and_wait();
}

#[test]
// Spec coverage: BND-060, BND-061, and BND-105.
fn geojson_source_helpers_accept_prepared_data_and_keep_options_across_updates() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map =
        crate::completion::blocking(MapHandle::with_options(&runtime, &MapOptions::default()));
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
        source_type_of(&map, "geojson-url"),
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
        source_type_of(&map, "geojson-data"),
        Some(SourceType::GeoJson)
    );
    assert_eq!(
        source_type_of(&map, "geojson-data-2"),
        Some(SourceType::GeoJson)
    );

    // Updates install prepared data; the source keeps the options the data it
    // was added with carried.
    let update = map.set_geojson_source_data("geojson-data", &data).unwrap();
    assert_command_disposition(update, CommandDisposition::Committed);
    let update = map.set_geojson_source_data("geojson-url", &data).unwrap();
    assert_command_disposition(update, CommandDisposition::Committed);
    map.set_geojson_source_url("geojson-data-2", "https://example.com/points.geojson")
        .unwrap();

    // The submit-time lease keeps the prepared index alive, so the handle may
    // be released as soon as an install command is submitted, and installed
    // sources keep their own reference.
    let final_install = map.set_geojson_source_data("geojson-data", &data).unwrap();
    data.close();
    assert_command_disposition(final_install, CommandDisposition::Committed);
    assert_eq!(
        source_type_of(&map, "geojson-data"),
        Some(SourceType::GeoJson)
    );

    map.close_and_wait();
    runtime.close_and_wait();
}

#[test]
// Spec coverage: BND-060 and BND-061.
fn set_geojson_source_data_rejects_mismatched_prepared_options() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map =
        crate::completion::blocking(MapHandle::with_options(&runtime, &MapOptions::default()));
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
    let rejected = map.set_geojson_source_data("points", &mismatched).unwrap();
    let completion = assert_command_disposition(rejected, CommandDisposition::Failed);
    assert_eq!(completion.raw_status, sys::MLN_STATUS_INVALID_ARGUMENT);

    // Cluster aggregations are part of the comparison, so data prepared with
    // different cluster_properties is rejected too.
    let mut properties_only = options.clone();
    properties_only.cluster_properties =
        Some(serde_json::to_vec(&json!({"weight_sum": ["+", ["get", "weight"]]})).unwrap());
    let properties_only =
        crate::GeoJsonSourceDataHandle::new(bytes, Some(&properties_only)).unwrap();
    let rejected = map
        .set_geojson_source_data("points", &properties_only)
        .unwrap();
    let completion = assert_command_disposition(rejected, CommandDisposition::Failed);
    assert_eq!(completion.raw_status, sys::MLN_STATUS_INVALID_ARGUMENT);

    // The rejected install left the source usable with matching data.
    let matching = crate::GeoJsonSourceDataHandle::new(bytes, Some(&options)).unwrap();
    let accepted = map.set_geojson_source_data("points", &matching).unwrap();
    assert_command_disposition(accepted, CommandDisposition::Committed);

    map.close_and_wait();
    runtime.close_and_wait();
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
// Spec coverage: BND-060 and BND-061.
fn geojson_data_prepared_off_thread_installs_on_the_map_thread() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map =
        crate::completion::blocking(MapHandle::with_options(&runtime, &MapOptions::default()));
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
        source_type_of(&map, "worker-prepared"),
        Some(SourceType::GeoJson)
    );

    // The prepared value releases off the map thread as well.
    std::thread::spawn(move || drop(data)).join().unwrap();

    map.close_and_wait();
    runtime.close_and_wait();
}

#[test]
// Spec coverage: BND-060 and BND-061.
fn synchronous_tiling_override_targets_live_geojson_sources_only() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map =
        crate::completion::blocking(MapHandle::with_options(&runtime, &MapOptions::default()));
    map.set_style_json(STYLE_WITH_IDS_JSON.as_bytes()).unwrap();

    let enabled = map
        .set_geojson_source_synchronous_tiling("geo", true)
        .unwrap();
    assert_command_disposition(enabled, CommandDisposition::Committed);
    let disabled = map
        .set_geojson_source_synchronous_tiling("geo", false)
        .unwrap();
    assert_command_disposition(disabled, CommandDisposition::Committed);

    let missing = map
        .set_geojson_source_synchronous_tiling("missing", true)
        .unwrap();
    let completion = assert_command_disposition(missing, CommandDisposition::Failed);
    assert_eq!(completion.raw_status, sys::MLN_STATUS_NOT_FOUND);

    map.close_and_wait();
    runtime.close_and_wait();
}

#[test]
// Spec coverage: BND-105.
fn style_source_info_reports_type_and_found_flag() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map =
        crate::completion::blocking(MapHandle::with_options(&runtime, &MapOptions::default()));
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
        crate::completion::blocking(map.style_source_info("missing-source")),
        None
    );

    map.add_style_source_json("empty", &geojson_source).unwrap();
    let info = crate::completion::blocking(map.style_source_info("empty")).unwrap();
    assert_eq!(info.source_type, SourceType::GeoJson);
    assert_eq!(info.raw_source_type, sys::MLN_STYLE_SOURCE_TYPE_GEOJSON);
    assert!(!info.is_volatile);
    assert_eq!(info.attribution, None);

    map.add_style_source_json("vector-meta", &vector_source)
        .unwrap();
    let info = crate::completion::blocking(map.style_source_info("vector-meta")).unwrap();
    assert_eq!(info.source_type, SourceType::Vector);
    assert_eq!(info.raw_source_type, sys::MLN_STYLE_SOURCE_TYPE_VECTOR);
    assert_eq!(info.attribution.as_deref(), Some("Example attribution"));

    map.close_and_wait();
    runtime.close_and_wait();
}

#[test]
// Spec coverage: BND-105.
fn style_source_volatility_round_trips_through_public_api() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map =
        crate::completion::blocking(MapHandle::with_options(&runtime, &MapOptions::default()));
    map.set_style_json(VALID_STYLE_JSON.as_bytes()).unwrap();
    map.add_vector_source_url("source", "https://example.com/source.json", None)
        .unwrap();

    assert!(
        !crate::completion::blocking(map.style_source_info("source"))
            .unwrap()
            .is_volatile
    );

    let volatile = map.set_style_source_volatile("source", true).unwrap();
    assert_command_disposition(volatile, CommandDisposition::Committed);
    assert!(
        crate::completion::blocking(map.style_source_info("source"))
            .unwrap()
            .is_volatile
    );

    let persistent = map.set_style_source_volatile("source", false).unwrap();
    assert_command_disposition(persistent, CommandDisposition::Committed);
    assert!(
        !crate::completion::blocking(map.style_source_info("source"))
            .unwrap()
            .is_volatile
    );

    // A missing source fails the command with the not-found status code.
    let missing = map.set_style_source_volatile("missing", true).unwrap();
    let completion = assert_command_disposition(missing, CommandDisposition::Failed);
    assert_eq!(completion.raw_status, sys::MLN_STATUS_NOT_FOUND);

    map.close_and_wait();
    runtime.close_and_wait();
}

#[test]
// Spec coverage: BND-066, BND-109.
fn style_source_info_copies_reconstructible_source_state() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map =
        crate::completion::blocking(MapHandle::with_options(&runtime, &MapOptions::default()));
    map.set_style_json(VALID_STYLE_JSON.as_bytes()).unwrap();

    map.add_vector_source_url("remote", "https://example.com/source.json", None)
        .unwrap();
    let remote = crate::completion::blocking(map.style_source_info("remote")).unwrap();
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
    let copied = crate::completion::blocking(map.style_source_info("inline")).unwrap();
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

    let removed = map.remove_style_source("inline").unwrap();
    assert_command_disposition(removed, CommandDisposition::Committed);
    map.close_and_wait();
    runtime.close_and_wait();

    assert_eq!(
        copied.tile_json.unwrap().tiles,
        [
            "https://a.example/{z}/{x}/{y}.mlt".to_owned(),
            "https://b.example/{z}/{x}/{y}.mlt".to_owned(),
        ]
    );
}

#[test]
// Spec coverage: BND-105.
fn style_source_url_attribution_and_tile_urls_round_trip_through_public_api() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map =
        crate::completion::blocking(MapHandle::with_options(&runtime, &MapOptions::default()));
    map.set_style_json(VALID_STYLE_JSON.as_bytes()).unwrap();

    let mut options = TileSourceOptions::default();
    options.attribution = Some("Example attribution".to_owned());
    let tiles = [
        "https://a.example/{z}/{x}/{y}.pbf",
        "https://b.example/{z}/{x}/{y}.pbf",
    ];
    map.add_vector_source_tiles("inline", &tiles, Some(&options))
        .unwrap();
    map.add_vector_source_url("remote", "https://example.com/source.json", None)
        .unwrap();

    // An inline TileJSON source carries its tile URLs and attribution.
    assert_eq!(
        crate::completion::blocking(map.style_source_attribution("inline")).as_deref(),
        Some("Example attribution")
    );
    assert_eq!(
        crate::completion::blocking(map.style_source_tile_urls("inline")),
        tiles
    );
    assert_eq!(
        crate::completion::blocking(map.style_source_url("inline")),
        None
    );

    // A source that loads its TileJSON from a URL carries the URL instead.
    assert_eq!(
        crate::completion::blocking(map.style_source_url("remote")).as_deref(),
        Some("https://example.com/source.json")
    );
    assert!(crate::completion::blocking(map.style_source_tile_urls("remote")).is_empty());
    assert_eq!(
        crate::completion::blocking(map.style_source_attribution("remote")),
        None
    );

    // A missing source resolves rather than failing: no value for the strings,
    // and an empty sequence for the tile URLs.
    assert_eq!(
        crate::completion::blocking(map.style_source_url("missing-source")),
        None
    );
    assert_eq!(
        crate::completion::blocking(map.style_source_attribution("missing-source")),
        None
    );
    assert!(
        crate::completion::blocking(map.style_source_tile_urls("missing-source")).is_empty(),
        "the C completion reports a missing source the same way it reports a source with no inline TileJSON"
    );

    map.close_and_wait();
    runtime.close_and_wait();
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
    let map =
        crate::completion::blocking(MapHandle::with_options(&runtime, &MapOptions::default()));
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
    let removed = map.remove_style_source("custom").unwrap();
    assert_command_disposition(removed, CommandDisposition::Committed);
    await_release_count(&runtime, &releases, 1);
    assert!(source_type_of(&map, "custom").is_none());

    let first_add = map
        .add_custom_geometry_source("custom", options_counting_releases(&releases))
        .unwrap();
    let duplicate_add = map
        .add_custom_geometry_source("custom", options_counting_releases(&releases))
        .unwrap();
    assert_command_disposition(first_add, CommandDisposition::Committed);
    assert_command_disposition(duplicate_add, CommandDisposition::Failed);
    await_release_count(&runtime, &releases, 2);

    map.set_style_json(VALID_STYLE_JSON.as_bytes()).unwrap();
    await_release_count(&runtime, &releases, 3);

    map.close_and_wait();
    runtime.close_and_wait();
}

#[test]
// Spec coverage: BND-124.
fn custom_geometry_source_state_is_released_on_map_close() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map =
        crate::completion::blocking(MapHandle::with_options(&runtime, &MapOptions::default()));
    map.set_style_json(VALID_STYLE_JSON.as_bytes()).unwrap();
    let releases = Arc::new(AtomicUsize::new(0));
    map.add_custom_geometry_source("custom", options_counting_releases(&releases))
        .unwrap();

    map.close_and_wait();

    // The browser C ABI suite covers callback retirement at native completion.
    // Its synchronous Rust harness keeps the graphics-owning pthread available
    // and relies on process isolation for final teardown instead.
    #[cfg(not(target_os = "emscripten"))]
    assert_eq!(releases.load(Ordering::SeqCst), 1);
    runtime.close_and_wait();
}

#[test]
// Spec coverage: BND-124.
fn custom_geometry_source_adds_to_current_style_after_url_style_request() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map =
        crate::completion::blocking(MapHandle::with_options(&runtime, &MapOptions::default()));
    map.set_style_json(VALID_STYLE_JSON.as_bytes()).unwrap();
    map.set_style_url("unsupported://style.json").unwrap();
    let releases = Arc::new(AtomicUsize::new(0));

    map.add_custom_geometry_source("custom", options_counting_releases(&releases))
        .unwrap();

    assert!(source_type_of(&map, "custom").is_some());
    assert_eq!(releases.load(Ordering::SeqCst), 0);
    map.close_and_wait();
    runtime.close_and_wait();
}

#[test]
// Spec coverage: BND-093 and BND-124.
fn custom_geometry_source_state_releases_after_url_style_replacement() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
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
    let map =
        crate::completion::blocking(MapHandle::with_options(&runtime, &MapOptions::default()));
    // The host wants no style-loaded events, and the release runs anyway.
    map.set_event_mask(RuntimeEventMask::ALL - RuntimeEventMask::MAP_STYLE_LOADED)
        .unwrap();
    map.set_style_json(VALID_STYLE_JSON.as_bytes()).unwrap();
    let releases = Arc::new(AtomicUsize::new(0));
    map.add_custom_geometry_source("custom", options_counting_releases(&releases))
        .unwrap();
    drain_runtime_events(&runtime);

    map.set_style_url("custom://style.json").unwrap();
    let mut style_loaded_events = 0;
    let deadline = std::time::Instant::now() + Duration::from_secs(5);
    while releases.load(Ordering::SeqCst) != 1 {
        for event in runtime.drain_events().unwrap() {
            if event.event_type == RuntimeEventType::MapStyleLoaded {
                style_loaded_events += 1;
            }
        }
        assert!(
            std::time::Instant::now() < deadline,
            "the detached source's callback state was never released"
        );
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

    map.close_and_wait();
    runtime.close_and_wait();
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
    let map =
        crate::completion::blocking(MapHandle::with_options(&runtime, &MapOptions::default()));
    map.set_style_json(VALID_STYLE_JSON.as_bytes()).unwrap();
    let releases = Arc::new(AtomicUsize::new(0));

    let mut custom_options = mvt_options_counting_releases(&releases).with_cancel_tile(|_| {});
    custom_options.min_zoom = Some(0.0);
    custom_options.max_zoom = Some(2.0);
    map.add_custom_mvt_vector_source("custom", custom_options)
        .unwrap();
    assert_eq!(
        source_type_of(&map, "custom"),
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

    let removed = map.remove_style_source("custom").unwrap();
    assert_command_disposition(removed, CommandDisposition::Committed);
    await_release_count(&runtime, &releases, 1);
    assert!(source_type_of(&map, "custom").is_none());

    let first_add = map
        .add_custom_mvt_vector_source("custom", mvt_options_counting_releases(&releases))
        .unwrap();
    let duplicate_add = map
        .add_custom_mvt_vector_source("custom", mvt_options_counting_releases(&releases))
        .unwrap();
    assert_command_disposition(first_add, CommandDisposition::Committed);
    assert_command_disposition(duplicate_add, CommandDisposition::Failed);
    await_release_count(&runtime, &releases, 2);

    map.set_style_json(VALID_STYLE_JSON.as_bytes()).unwrap();
    await_release_count(&runtime, &releases, 3);

    map.close_and_wait();
    runtime.close_and_wait();
}

#[test]
// Spec coverage: BND-124.
fn custom_mvt_vector_source_state_is_released_on_map_close() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map =
        crate::completion::blocking(MapHandle::with_options(&runtime, &MapOptions::default()));
    map.set_style_json(VALID_STYLE_JSON.as_bytes()).unwrap();
    let releases = Arc::new(AtomicUsize::new(0));
    map.add_custom_mvt_vector_source("custom", mvt_options_counting_releases(&releases))
        .unwrap();

    map.close_and_wait();

    // The browser C ABI suite covers callback retirement at native completion.
    // Its synchronous Rust harness keeps the graphics-owning pthread available
    // and relies on process isolation for final teardown instead.
    #[cfg(not(target_os = "emscripten"))]
    assert_eq!(releases.load(Ordering::SeqCst), 1);
    runtime.close_and_wait();
}

/// Discards every event queued so far, so a later drain reports only what the
/// test caused after this point.
fn drain_runtime_events(runtime: &RuntimeHandle) {
    await_runtime_barrier(runtime);
    let _ = runtime.drain_events().unwrap();
}

/// Loads a style and collects the map-sourced event types it produced, up to
/// and including the style-loaded event.
///
/// Style loading is asynchronous, so this drains to a deadline rather than for
/// a fixed number of turns.
fn collect_style_load_event_types(
    runtime: &RuntimeHandle,
    map: &MapHandle,
    style_json: &str,
) -> Vec<RuntimeEventType> {
    let mut types = Vec::new();
    map.set_style_json(style_json.as_bytes()).unwrap();
    let deadline = std::time::Instant::now() + Duration::from_secs(5);
    loop {
        for event in runtime.drain_events().unwrap() {
            if event.source == RuntimeEventSource::Map(map.id()) {
                types.push(event.event_type);
            }
        }
        if types.contains(&RuntimeEventType::MapStyleLoaded) {
            return types;
        }
        assert!(
            std::time::Instant::now() < deadline,
            "style never finished loading: {types:?}"
        );
        std::thread::sleep(Duration::from_millis(1));
    }
}

/// Loads a style whose events the map's mask gates entirely, and returns the
/// map-sourced events that arrived anyway.
///
/// The awaited command plus a runtime barrier fence the negative assertion:
/// once both have finished, every event this load could have queued is queued.
fn collect_gated_style_load_event_types(
    runtime: &RuntimeHandle,
    map: &MapHandle,
    style_json: &str,
) -> Vec<RuntimeEventType> {
    assert_command_disposition(
        map.set_style_json(style_json.as_bytes()).unwrap(),
        CommandDisposition::Committed,
    );
    await_runtime_barrier(runtime);
    runtime
        .drain_events()
        .unwrap()
        .into_iter()
        .filter(|event| event.source == RuntimeEventSource::Map(map.id()))
        .map(|event| event.event_type)
        .collect()
}

#[test]
// Spec coverage: BND-091.
fn a_narrowed_map_mask_delivers_the_kept_type_alone() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map =
        crate::completion::blocking(MapHandle::with_options(&runtime, &MapOptions::default()));

    map.set_event_mask(RuntimeEventMask::ALL - RuntimeEventMask::MAP_LOADING_STARTED)
        .unwrap();

    let types = collect_style_load_event_types(&runtime, &map, VALID_STYLE_JSON);

    assert!(
        types.contains(&RuntimeEventType::MapStyleLoaded),
        "{types:?}"
    );
    assert!(
        !types.contains(&RuntimeEventType::MapLoadingStarted),
        "{types:?}"
    );

    // An empty mask leaves the map with nothing to report.
    assert_command_disposition(
        map.set_event_mask(RuntimeEventMask::NONE).unwrap(),
        CommandDisposition::Committed,
    );
    let types = collect_gated_style_load_event_types(&runtime, &map, STYLE_WITH_IDS_JSON);
    assert!(types.is_empty(), "{types:?}");

    map.close_and_wait();
    runtime.close_and_wait();
}

#[test]
// Spec coverage: BND-060 and BND-091.
fn a_creation_mask_narrows_a_map_from_its_first_style_load() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let mut options = MapOptions::default();
    options.event_mask = RuntimeEventMask::ALL - RuntimeEventMask::MAP_LOADING_STARTED;
    let map = crate::completion::blocking(MapHandle::with_options(&runtime, &options));

    let types = collect_style_load_event_types(&runtime, &map, VALID_STYLE_JSON);

    assert!(
        types.contains(&RuntimeEventType::MapStyleLoaded),
        "{types:?}"
    );
    assert!(
        !types.contains(&RuntimeEventType::MapLoadingStarted),
        "{types:?}"
    );

    map.close_and_wait();
    runtime.close_and_wait();
}

#[test]
// Spec coverage: BND-091.
fn map_mask_commands_complete_and_reject_undefined_bits() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map =
        crate::completion::blocking(MapHandle::with_options(&runtime, &MapOptions::default()));

    let first = map.set_event_mask(RuntimeEventMask::ALL).unwrap();
    let second = map
        .set_event_mask(RuntimeEventMask::ALL - RuntimeEventMask::MAP_TILE_ACTION)
        .unwrap();
    assert_command_disposition(first, CommandDisposition::Committed);
    assert_command_disposition(second, CommandDisposition::Committed);

    let error = map
        .set_event_mask(RuntimeEventMask::from_bits_retain(1 << 63))
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    assert_eq!(error.raw_status(), Some(sys::MLN_STATUS_INVALID_ARGUMENT));

    map.close_and_wait();
    runtime.close_and_wait();
}

#[test]
// Spec coverage: BND-060, BND-061, BND-070, and BND-105.
fn style_transition_options_round_trip_through_the_real_c_api() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map =
        crate::completion::blocking(MapHandle::with_options(&runtime, &MapOptions::default()));

    // MapLibre Native always holds a placement flag, so it reports even before
    // a style is loaded.
    let empty = crate::completion::blocking(map.style_transition_options());
    assert_eq!(empty.duration_ms, None);
    assert_eq!(empty.delay_ms, None);
    assert_eq!(empty.enable_placement_transitions, Some(true));

    // The style parser fills in its own 300ms duration when the style carries
    // no transition member.
    map.set_style_json(VALID_STYLE_JSON.as_bytes()).unwrap();
    let parsed = crate::completion::blocking(map.style_transition_options());
    assert_eq!(parsed.duration_ms, Some(300.0));
    assert_eq!(parsed.delay_ms, None);

    // A transition member reports only what it names, so a delay-only
    // transition replaces that default with no duration.
    map.set_style_json(STYLE_WITH_DELAY_ONLY_TRANSITION_JSON.as_bytes())
        .unwrap();
    let delay_only = crate::completion::blocking(map.style_transition_options());
    assert_eq!(delay_only.duration_ms, None);
    assert_eq!(delay_only.delay_ms, Some(100.0));

    map.set_style_json(STYLE_WITH_TRANSITION_JSON.as_bytes())
        .unwrap();
    let declared = crate::completion::blocking(map.style_transition_options());
    assert_eq!(declared.duration_ms, Some(750.0));
    assert_eq!(declared.delay_ms, Some(100.0));
    assert_eq!(declared.enable_placement_transitions, Some(true));

    // A present zero stays distinguishable from an omitted field, and an
    // omitted field clears what the style declared rather than merging into it.
    let mut options = StyleTransitionOptions::default();
    options.duration_ms = Some(0.0);
    options.enable_placement_transitions = Some(false);
    map.set_style_transition_options(&options).unwrap();

    let applied = crate::completion::blocking(map.style_transition_options());
    assert_eq!(applied, options);
    assert_eq!(applied.duration_ms, Some(0.0));
    assert_eq!(applied.delay_ms, None);
    assert_eq!(applied.enable_placement_transitions, Some(false));

    // Omitting the flag leaves the cross-fade on rather than clearing it.
    let mut duration_only = StyleTransitionOptions::default();
    duration_only.duration_ms = Some(250.0);
    map.set_style_transition_options(&duration_only).unwrap();
    assert_eq!(
        crate::completion::blocking(map.style_transition_options()).enable_placement_transitions,
        Some(true)
    );

    // Loading a style replaces the override with what that style declares.
    map.set_style_json(STYLE_WITH_TRANSITION_JSON.as_bytes())
        .unwrap();
    assert_eq!(
        crate::completion::blocking(map.style_transition_options()),
        declared
    );

    let mut negative = StyleTransitionOptions::default();
    negative.delay_ms = Some(-1.0);
    let rejected_command = map.set_style_transition_options(&negative).unwrap();
    let completion = assert_command_disposition(rejected_command, CommandDisposition::Failed);
    assert!(completion.diagnostic.contains("delay_ms"));

    map.close_and_wait();
    runtime.close_and_wait();
}

#[test]
// Spec coverage: BND-063, BND-064, and BND-105.
fn style_json_buffers_copy_owned_rust_values() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map =
        crate::completion::blocking(MapHandle::with_options(&runtime, &MapOptions::default()));
    map.set_style_json(STYLE_WITH_IDS_JSON.as_bytes()).unwrap();

    let layer = serde_json::to_vec(&json!({
        "id": "owned-background",
        "type": "background",
        "paint": {"background-opacity": 0.5},
    }))
    .unwrap();
    map.add_style_layer_json(&layer, None).unwrap();
    let copied_layer = crate::completion::blocking(map.style_layer_json("owned-background"))
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
        crate::completion::blocking(map.layer_property("owned-background", "background-opacity")),
        Some(b"0.75".to_vec())
    );

    let filter = br#"["==",["get","kind"],"park"]"#;
    map.set_layer_filter("geo-fill", Some(filter)).unwrap();
    assert_eq!(
        crate::completion::blocking(map.layer_filter("geo-fill")),
        Some(filter.to_vec())
    );
    map.set_layer_filter("geo-fill", None).unwrap();
    assert_eq!(
        crate::completion::blocking(map.layer_filter("geo-fill")),
        None
    );

    let rejected_command = map
        .set_layer_filter("owned-background", Some(b"NaN"))
        .unwrap();
    assert_command_disposition(rejected_command, CommandDisposition::Failed);
    assert_eq!(
        crate::completion::blocking(map.layer_filter("owned-background")),
        None
    );

    map.close_and_wait();
    runtime.close_and_wait();
}

#[test]
fn camera_commands_and_ordered_queries_return_typed_completions() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map =
        crate::completion::blocking(MapHandle::with_options(&runtime, &MapOptions::default()));
    let center = LatLng::new(45.0, -122.0);
    let mut update = CameraUpdate::default();
    update.camera.center = Some(center);
    update.camera.zoom = Some(4.0);

    let command = map.update_camera(&update).unwrap();

    let query = map.camera_query().unwrap();
    assert!(query.wait(Duration::from_secs(5)).unwrap());
    let queried = query.take().unwrap();
    assert_command_disposition(command, CommandDisposition::Committed);
    assert_eq!(queried.camera.center, Some(center));
    assert_eq!(queried.camera.zoom, Some(4.0));

    let published = map.camera_snapshot().unwrap();
    assert!(published.generation <= queried.generation);
    map.close_and_wait();
    runtime.close_and_wait();
}

#[test]
// Spec coverage: BND-045.
fn map_accepts_concurrent_commands_and_cross_thread_snapshots() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map =
        crate::completion::blocking(MapHandle::with_options(&runtime, &MapOptions::default()));

    std::thread::scope(|scope| {
        let first = scope.spawn(|| map.request_repaint().unwrap());
        let second = scope.spawn(|| map.request_repaint().unwrap());
        let first = first.join().unwrap();
        let second = second.join().unwrap();
        assert_command_disposition(first, CommandDisposition::Committed);
        assert_command_disposition(second, CommandDisposition::Committed);

        let snapshot = scope.spawn(|| map.snapshot().unwrap()).join().unwrap();
        assert_eq!(snapshot.logical_extent.width, MapOptions::default().width);
    });
    map.close_and_wait();
    runtime.close_and_wait();
}

#[test]
// Spec coverage: BND-102.
fn a_snapshot_at_the_committed_generation_observes_the_commit() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map =
        crate::completion::blocking(MapHandle::with_options(&runtime, &MapOptions::default()));
    assert_eq!(
        map.snapshot().unwrap().debug_options,
        MapDebugOptions::empty()
    );

    let completion = map
        .set_debug_options(MapDebugOptions::TILE_BORDERS)
        .unwrap();
    let completion = assert_command_disposition(completion, CommandDisposition::Committed);
    assert_ne!(completion.generation, 0);

    // The commit fence: a snapshot at or past the reported generation shows
    // the committed value.
    let snapshot = map.snapshot().unwrap();
    assert!(snapshot.generation >= completion.generation);
    assert_eq!(snapshot.debug_options, MapDebugOptions::TILE_BORDERS);

    map.close_and_wait();
    runtime.close_and_wait();
}

#[test]
// Spec coverage: BND-102.
fn snapshot_fields_round_trip_through_their_set_commands() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map =
        crate::completion::blocking(MapHandle::with_options(&runtime, &MapOptions::default()));

    let mut tile = crate::MapTileOptions::default();
    tile.prefetch_zoom_delta = Some(3);
    tile.lod_scale = Some(1.5);
    map.set_tile_options(&tile).unwrap();

    let mut bounds = BoundOptions::default();
    bounds.min_zoom = Some(2.0);
    bounds.max_zoom = Some(15.0);
    map.set_bounds(&bounds).unwrap();

    let mut viewport = crate::MapViewportOptions::default();
    viewport.viewport_mode = Some(crate::ViewportMode::FlippedY);
    map.set_viewport_options(&viewport).unwrap();

    let mut free_camera = FreeCameraOptions::default();
    free_camera.position = Some(crate::Vec3::new(0.25, 0.25, 0.5));
    map.set_free_camera_options(&free_camera).unwrap();

    let stats_command = map.set_rendering_stats_view_enabled(true).unwrap();
    let completion = assert_command_disposition(stats_command, CommandDisposition::Committed);

    let snapshot = map.snapshot().unwrap();
    assert!(snapshot.generation >= completion.generation);
    assert_eq!(snapshot.tile.prefetch_zoom_delta, Some(3));
    assert_eq!(snapshot.tile.lod_scale, Some(1.5));
    assert_eq!(snapshot.bounds.min_zoom, Some(2.0));
    assert_eq!(snapshot.bounds.max_zoom, Some(15.0));
    assert_eq!(
        snapshot.viewport.viewport_mode,
        Some(crate::ViewportMode::FlippedY)
    );
    // Native renormalizes altitude, so assert the committed ground position.
    let position = snapshot.free_camera.position.unwrap();
    assert!((position.x - 0.25).abs() < 1e-6);
    assert!((position.y - 0.25).abs() < 1e-6);
    assert!(position.z > 0.0);
    assert!(snapshot.rendering_stats_view_enabled);

    map.close_and_wait();
    runtime.close_and_wait();
}

#[test]
// Spec coverage: BND-103.
fn unwrapped_coordinate_conversions_preserve_visible_world_copies() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = crate::completion::blocking(MapHandle::with_options(
        &runtime,
        &MapOptions::new(1024, 512, 1.0),
    ));
    let mut update = CameraUpdate::default();
    update.camera.center = Some(LatLng::new(0.0, 180.0));
    update.camera.zoom = Some(0.0);
    let command = map.update_camera(&update).unwrap();
    assert_command_disposition(command, CommandDisposition::Committed);
    await_runtime_barrier(&runtime);

    // The viewport is two world copies wide, so its edges name the same
    // wrapped longitude in different copies.
    let points = [
        ScreenPoint::new(0.0, 256.0),
        ScreenPoint::new(1024.0, 256.0),
    ];
    let wrapped = crate::completion::blocking(map.lat_lngs_for_pixels(&points));
    let unwrapped = crate::completion::blocking(map.lat_lngs_for_pixels_unwrapped(&points));
    assert!(
        wrapped
            .iter()
            .all(|coordinate| (-180.0..=180.0).contains(&coordinate.longitude))
    );
    assert!(unwrapped[1].longitude - unwrapped[0].longitude > 360.0);

    let right_wrapped = crate::completion::blocking(map.lat_lng_for_pixel(points[1]));
    assert!((-180.0..=180.0).contains(&right_wrapped.longitude));
    let right = crate::completion::blocking(map.lat_lng_for_pixel_unwrapped(points[1]));
    assert!((right.longitude - unwrapped[1].longitude).abs() < 1e-10);

    // The forward conversion places the camera center at the viewport center,
    // and the batch form agrees with the single-coordinate form.
    let center = LatLng::new(0.0, 180.0);
    let batch =
        crate::completion::blocking(map.pixels_for_lat_lngs(&[center, LatLng::new(0.0, 0.0)]));
    assert_eq!(batch.len(), 2);
    assert!((batch[0].x - 512.0).abs() < 1e-6);
    assert!((batch[0].y - 256.0).abs() < 1e-6);
    assert_ne!(batch[1].x, batch[0].x);
    let single = crate::completion::blocking(map.pixel_for_lat_lng(center));
    assert!((single.x - batch[0].x).abs() < 1e-6);
    assert!((single.y - batch[0].y).abs() < 1e-6);

    // A projection snapshot reports the same pair of readings.
    let projection = crate::completion::blocking(map.create_projection());
    assert!((-180.0..=180.0).contains(&projection.lat_lng_for_pixel(points[1]).unwrap().longitude));
    let projected_right = projection.lat_lng_for_pixel_unwrapped(points[1]).unwrap();
    assert!((projected_right.longitude - right.longitude).abs() < 1e-10);
    projection.close().unwrap();

    map.close_and_wait();
    runtime.close_and_wait();
}

/// Submits one camera update and awaits the command that starts it. A long
/// duration keeps a transition running until a later command ends it, so each
/// terminal outcome below is the one the test named.
fn submit_camera_update(
    map: &MapHandle,
    mode: crate::CameraUpdateMode,
    zoom: f64,
    transition_id: Option<u64>,
) {
    let mut update = CameraUpdate::default();
    update.mode = mode;
    update.camera.zoom = Some(zoom);
    update.animation.duration_ms = Some(60_000.0);
    update.animation.transition_id = transition_id;
    assert_command_disposition(
        map.update_camera(&update).unwrap(),
        CommandDisposition::Committed,
    );
}

/// Drains once and returns the transition IDs the drain reported finished.
fn drain_finished_transitions(runtime: &RuntimeHandle) -> Vec<u64> {
    runtime
        .drain_events()
        .unwrap()
        .into_iter()
        .filter_map(|event| match event.payload {
            crate::RuntimeEventPayload::CameraTransitionFinished(payload) => {
                Some(payload.transition_id)
            }
            _ => None,
        })
        .collect()
}

#[test]
// Spec coverage: BND-102.
fn a_camera_transition_reports_one_terminal_outcome() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map =
        crate::completion::blocking(MapHandle::with_options(&runtime, &MapOptions::default()));
    await_runtime_barrier(&runtime);
    let _ = runtime.drain_events().unwrap();

    // A running transition has not ended, so it reports nothing yet.
    submit_camera_update(&map, crate::CameraUpdateMode::Ease, 4.0, Some(11));
    assert!(drain_finished_transitions(&runtime).is_empty());

    // A second ease replaces the first, which ends and reports its own ID.
    submit_camera_update(&map, crate::CameraUpdateMode::Ease, 6.0, Some(12));
    assert_eq!(drain_finished_transitions(&runtime), vec![11]);

    // A jump cancels the running transition, which reports the cancelled ID.
    submit_camera_update(&map, crate::CameraUpdateMode::Jump, 8.0, None);
    assert_eq!(drain_finished_transitions(&runtime), vec![12]);

    // A transition started without an ID is silent, and so is its end.
    submit_camera_update(&map, crate::CameraUpdateMode::Ease, 10.0, None);
    submit_camera_update(&map, crate::CameraUpdateMode::Jump, 12.0, None);
    assert!(drain_finished_transitions(&runtime).is_empty());

    map.close_and_wait();
    runtime.close_and_wait();
}

#[test]
// Spec coverage: BND-102.
fn cancel_transitions_ends_a_running_transition() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map =
        crate::completion::blocking(MapHandle::with_options(&runtime, &MapOptions::default()));
    await_runtime_barrier(&runtime);
    let _ = runtime.drain_events().unwrap();

    submit_camera_update(&map, crate::CameraUpdateMode::Fly, 9.0, Some(21));
    assert!(drain_finished_transitions(&runtime).is_empty());

    assert_command_disposition(
        map.cancel_transitions().unwrap(),
        CommandDisposition::Committed,
    );
    assert_eq!(drain_finished_transitions(&runtime), vec![21]);

    // Cancelling again with nothing running still commits and reports nothing.
    assert_command_disposition(
        map.cancel_transitions().unwrap(),
        CommandDisposition::Committed,
    );
    assert!(drain_finished_transitions(&runtime).is_empty());

    map.close_and_wait();
    runtime.close_and_wait();
}

#[test]
// Spec coverage: BND-102.
fn gesture_phases_open_and_close_the_published_gesture_flag() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map =
        crate::completion::blocking(MapHandle::with_options(&runtime, &MapOptions::default()));
    assert!(!map.snapshot().unwrap().gesture_in_progress);

    // Begin and Update hold the gesture open; End and Cancel close it. The
    // phase travels with the camera update rather than in its own command.
    let mut update = CameraUpdate::default();
    for (phase, expected) in [
        (crate::GesturePhase::Begin, true),
        (crate::GesturePhase::Update, true),
        (crate::GesturePhase::End, false),
        (crate::GesturePhase::Begin, true),
        (crate::GesturePhase::Cancel, false),
    ] {
        update.gesture_phase = phase;
        let completion = assert_command_disposition(
            map.update_camera(&update).unwrap(),
            CommandDisposition::Committed,
        );
        let snapshot = map.snapshot().unwrap();
        assert!(snapshot.generation >= completion.generation);
        assert_eq!(snapshot.gesture_in_progress, expected, "phase {phase:?}");
    }

    map.close_and_wait();
    runtime.close_and_wait();
}

#[test]
// Spec coverage: BND-102.
fn relative_camera_deltas_apply_to_the_current_camera() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = crate::completion::blocking(MapHandle::with_options(
        &runtime,
        &MapOptions::new(256, 256, 1.0),
    ));
    let mut update = CameraUpdate::default();
    update.camera.center = Some(LatLng::new(0.0, 0.0));
    update.camera.zoom = Some(4.0);
    update.camera.bearing = Some(0.0);
    update.camera.pitch = Some(0.0);
    assert_command_disposition(
        map.update_camera(&update).unwrap(),
        CommandDisposition::Committed,
    );

    let mut delta = CameraDelta::default();
    delta.kind = crate::CameraDeltaKind::Move;
    delta.offset = ScreenPoint::new(32.0, 0.0);
    assert_command_disposition(
        map.apply_camera_delta(&delta).unwrap(),
        CommandDisposition::Committed,
    );
    let moved = crate::completion::blocking(map.camera_query()).camera;
    assert_ne!(moved.center.unwrap().longitude, 0.0);

    let mut delta = CameraDelta::default();
    delta.kind = crate::CameraDeltaKind::Scale;
    delta.amount = 2.0;
    delta.anchor = Some(ScreenPoint::new(128.0, 128.0));
    assert_command_disposition(
        map.apply_camera_delta(&delta).unwrap(),
        CommandDisposition::Committed,
    );
    let scaled = crate::completion::blocking(map.camera_query()).camera;
    assert!((scaled.zoom.unwrap() - 5.0).abs() < 1e-6);

    let mut delta = CameraDelta::default();
    delta.kind = crate::CameraDeltaKind::Bearing;
    delta.amount = 45.0;
    assert_command_disposition(
        map.apply_camera_delta(&delta).unwrap(),
        CommandDisposition::Committed,
    );
    let rotated = crate::completion::blocking(map.camera_query()).camera;
    assert!((rotated.bearing.unwrap() - 45.0).abs() < 1e-6);

    // Pitch adds to the current pitch, the opposite sign of Map::pitchBy().
    let mut delta = CameraDelta::default();
    delta.kind = crate::CameraDeltaKind::Pitch;
    delta.amount = 20.0;
    assert_command_disposition(
        map.apply_camera_delta(&delta).unwrap(),
        CommandDisposition::Committed,
    );
    let pitched = crate::completion::blocking(map.camera_query()).camera;
    assert!((pitched.pitch.unwrap() - 20.0).abs() < 1e-6);

    map.close_and_wait();
    runtime.close_and_wait();
}

#[test]
// Spec coverage: BND-102.
fn unbounded_and_world_bounds_constrain_the_camera_differently() {
    fn jumped_longitude(map: &MapHandle, longitude: f64) -> f64 {
        let mut update = CameraUpdate::default();
        update.camera.center = Some(LatLng::new(0.0, longitude));
        update.camera.zoom = Some(2.0);
        assert_command_disposition(
            map.update_camera(&update).unwrap(),
            CommandDisposition::Committed,
        );
        crate::completion::blocking(map.camera_query())
            .camera
            .center
            .unwrap()
            .longitude
    }

    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map =
        crate::completion::blocking(MapHandle::with_options(&runtime, &MapOptions::default()));

    assert_eq!(
        map.snapshot().unwrap().bounds.bounds,
        Some(crate::BoundsConstraint::Unbounded)
    );
    assert!((jumped_longitude(&map, 200.0) - -160.0).abs() < 1e-6);

    let world = LatLngBounds::new(LatLng::new(-90.0, -180.0), LatLng::new(90.0, 180.0));
    let mut options = BoundOptions::default();
    options.bounds = Some(crate::BoundsConstraint::Bounded(world));
    assert_command_disposition(
        map.set_bounds(&options).unwrap(),
        CommandDisposition::Committed,
    );
    assert_eq!(
        map.snapshot().unwrap().bounds.bounds,
        Some(crate::BoundsConstraint::Bounded(world))
    );
    // World bounds clamp at the antimeridian instead of wrapping.
    assert!((jumped_longitude(&map, 200.0) - 180.0).abs() < 1e-6);

    let mut options = BoundOptions::default();
    options.bounds = Some(crate::BoundsConstraint::Unbounded);
    assert_command_disposition(
        map.set_bounds(&options).unwrap(),
        CommandDisposition::Committed,
    );
    // Releasing the constraint restores antimeridian wrapping.
    assert!((jumped_longitude(&map, 200.0) - -160.0).abs() < 1e-6);

    map.close_and_wait();
    runtime.close_and_wait();
}

#[test]
// Spec coverage: BND-102.
fn projection_mode_round_trips_through_the_published_snapshot() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map =
        crate::completion::blocking(MapHandle::with_options(&runtime, &MapOptions::default()));

    let mut projection_mode = ProjectionMode::default();
    projection_mode.axonometric = Some(true);
    projection_mode.x_skew = Some(0.5);
    projection_mode.y_skew = Some(0.25);
    let completion = assert_command_disposition(
        map.set_projection_mode(&projection_mode).unwrap(),
        CommandDisposition::Committed,
    );

    let snapshot = map.snapshot().unwrap();
    assert!(snapshot.generation >= completion.generation);
    assert_eq!(snapshot.projection_mode.axonometric, Some(true));
    assert_eq!(snapshot.projection_mode.x_skew, Some(0.5));
    assert_eq!(snapshot.projection_mode.y_skew, Some(0.25));

    map.close_and_wait();
    runtime.close_and_wait();
}

#[test]
// Spec coverage: BND-025 and BND-068.
fn empty_coordinate_slice_is_rejected_before_calling_c() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map =
        crate::completion::blocking(MapHandle::with_options(&runtime, &MapOptions::default()));
    let mut fit = CameraFitOptions::default();
    fit.padding = Some(crate::EdgeInsets::new(1.0, 1.0, 1.0, 1.0));

    let error = map.camera_for_lat_lngs(&[], Some(&fit)).unwrap_err();

    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    assert_eq!(error.raw_status(), None);
    assert!(error.diagnostic().contains("at least one coordinate"));

    map.close_and_wait();
    runtime.close_and_wait();
}

#[test]
// Spec coverage: BND-102.
fn resize_rejects_a_scale_factor_the_map_was_not_created_with() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = crate::completion::blocking(MapHandle::with_options(
        &runtime,
        &MapOptions::new(64, 32, 1.0),
    ));

    let resized = assert_command_disposition(
        map.resize(LogicalExtent {
            width: 96,
            height: 48,
            scale_factor: 1.0,
        })
        .unwrap(),
        CommandDisposition::Committed,
    );
    let snapshot = map.snapshot().unwrap();
    assert!(snapshot.generation >= resized.generation);
    assert_eq!(
        (
            snapshot.logical_extent.width,
            snapshot.logical_extent.height
        ),
        (96, 48)
    );

    let error = map
        .resize(LogicalExtent {
            width: 96,
            height: 48,
            scale_factor: 2.0,
        })
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    assert_eq!(error.raw_status(), Some(sys::MLN_STATUS_INVALID_ARGUMENT));
    assert_eq!(map.snapshot().unwrap().logical_extent.scale_factor, 1.0);

    map.close_and_wait();
    runtime.close_and_wait();
}

#[test]
// Spec coverage: BND-046.
fn map_close_cancels_a_command_that_never_reached_a_terminal_outcome() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let mut options = MapOptions::new(32, 32, 1.0);
    options.mode = crate::MapMode::Static;
    let map = crate::completion::blocking(MapHandle::with_options(&runtime, &options));

    // A static map with no render session never produces the still image, so
    // the request is still pending when the map retires.
    let pending = map.request_still_image().unwrap();
    map.close_and_wait();

    assert!(pending.wait(Duration::from_secs(5)).unwrap());
    let error = pending.take().unwrap_err();
    assert_eq!(error.kind(), ErrorKind::Cancelled);
    assert_eq!(error.raw_status(), Some(sys::MLN_STATUS_CANCELLED));

    runtime.close_and_wait();
}

#[test]
// Spec coverage: BND-102.
fn dump_debug_logs_commits_on_the_map_worker() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map =
        crate::completion::blocking(MapHandle::with_options(&runtime, &MapOptions::default()));

    assert_command_disposition(
        map.dump_debug_logs().unwrap(),
        CommandDisposition::Committed,
    );

    map.close_and_wait();
    runtime.close_and_wait();
}

#[test]
// Spec coverage: BND-105.
fn narrow_style_image_copies_read_the_same_image_as_the_aggregate() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map =
        crate::completion::blocking(MapHandle::with_options(&runtime, &MapOptions::default()));
    map.set_style_json(VALID_STYLE_JSON.as_bytes()).unwrap();

    let image =
        PremultipliedRgba8Image::new(crate::TextureImageInfo::new(2, 2, 8, 16), vec![7u8; 16]);
    let mut options = StyleImageOptions::default();
    options.stretch_x = Some(vec![ImageStretch::new(0.0, 1.0)]);
    options.stretch_y = Some(vec![ImageStretch::new(1.0, 2.0)]);
    assert_command_disposition(
        map.set_style_image("narrow", &image, Some(&options))
            .unwrap(),
        CommandDisposition::Committed,
    );

    let aggregate = crate::completion::blocking(map.style_image("narrow")).unwrap();
    let pixels =
        crate::completion::blocking(map.style_image_premultiplied_rgba8("narrow")).unwrap();
    let (stretch_x, stretch_y) =
        crate::completion::blocking(map.style_image_stretches("narrow")).unwrap();

    assert_eq!(pixels, aggregate.image.data);
    assert_eq!(stretch_x, aggregate.stretch_x);
    assert_eq!(stretch_y, aggregate.stretch_y);

    // A missing image completes with no value on both narrow reads.
    assert!(crate::completion::blocking(map.style_image_premultiplied_rgba8("missing")).is_none());
    assert!(crate::completion::blocking(map.style_image_stretches("missing")).is_none());

    map.close_and_wait();
    runtime.close_and_wait();
}
