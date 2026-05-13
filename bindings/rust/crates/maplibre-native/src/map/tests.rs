use super::*;
use std::time::Duration;

use crate::events::{RuntimeEventSource, RuntimeEventType, empty_runtime_event};
use crate::{
    CustomGeometrySourceOptions, EdgeInsets, ErrorKind, JsonMember, MapMode, ResourceKind,
    ResourceProviderDecision, ResourceResponse, TextureImageInfo,
};

const VALID_STYLE_JSON: &str = r#"{"version":8,"sources":{},"layers":[]}"#;
const STYLE_WITH_IDS_JSON: &str = r#"{"version":8,"sources":{"geo":{"type":"geojson","data":{"type":"FeatureCollection","features":[]}}},"layers":[{"id":"background","type":"background"},{"id":"geo-fill","type":"fill","source":"geo"}]}"#;

fn object_member<'a>(value: &'a JsonValue, key: &str) -> Option<&'a JsonValue> {
    let JsonValue::Object(members) = value else {
        return None;
    };
    members
        .iter()
        .find(|member| member.key == key)
        .map(|member| &member.value)
}

#[test]
fn map_close_consumes_handle_and_drop_stays_idempotent() {
    let runtime = RuntimeHandle::new().unwrap();
    let map = MapHandle::new(&runtime).unwrap();

    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
fn map_retains_runtime_after_runtime_handle_is_dropped() {
    let runtime = RuntimeHandle::new().unwrap();
    let map = MapHandle::new(&runtime).unwrap();

    drop(runtime);

    map.close().unwrap();
}

#[test]
fn style_setters_accept_valid_input_and_reject_embedded_nul() {
    let runtime = RuntimeHandle::new().unwrap();
    let map = MapHandle::new(&runtime).unwrap();

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
    assert!(matches!(
        error.kind(),
        ErrorKind::InvalidArgument | ErrorKind::NativeError
    ));
    assert!(error.raw_status().is_some());
    assert!(!error.diagnostic().trim().is_empty());

    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
fn source_type_preserves_raw_values() {
    assert_eq!(SourceType::Unknown.raw_value(), 0);
    assert_eq!(SourceType::from_raw(0), SourceType::Unknown);
    assert_eq!(
        SourceType::GeoJson.raw_value(),
        sys::MLN_STYLE_SOURCE_TYPE_GEOJSON
    );
    assert_eq!(SourceType::from_raw(999_101), SourceType::Other(999_101));
    assert_eq!(SourceType::Other(999_101).raw_value(), 999_101);
}

#[test]
fn style_source_exists_and_remove_call_real_c_api() {
    let runtime = RuntimeHandle::new().unwrap();
    let map = MapHandle::new(&runtime).unwrap();
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
    PremultipliedRgba8Image {
        info: TextureImageInfo {
            width: 2,
            height: 2,
            stride: 8,
            byte_length: data.len(),
        },
        data,
    }
}

#[test]
fn style_image_copy_uses_rust_owned_buffer() {
    let runtime = RuntimeHandle::new().unwrap();
    let map = MapHandle::new(&runtime).unwrap();
    map.set_style_json(VALID_STYLE_JSON).unwrap();

    let image = test_style_image(vec![
        255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255, 255, 255, 255, 255,
    ]);

    map.set_style_image("plain", &image, None).unwrap();
    let copied = map
        .copy_style_image_premultiplied_rgba8("plain")
        .unwrap()
        .expect("added Rust image should copy back through C");

    assert_eq!(copied.image.info.width, image.info.width);
    assert_eq!(copied.image.info.height, image.info.height);
    assert_eq!(copied.image.data, image.data);
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
fn image_source_helpers_accept_url_and_inline_images() {
    let runtime = RuntimeHandle::new().unwrap();
    let map = MapHandle::new(&runtime).unwrap();
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
fn tile_source_helpers_call_real_c_api() {
    let runtime = RuntimeHandle::new().unwrap();
    let map = MapHandle::new(&runtime).unwrap();
    map.set_style_json(VALID_STYLE_JSON).unwrap();

    map.add_vector_source_url("vector-url", "https://example.com/vector.json", None)
        .unwrap();
    assert_eq!(
        map.style_source_type("vector-url").unwrap(),
        Some(SourceType::Vector)
    );

    let dem_options =
        TileSourceOptions::new().with_raster_dem_encoding(RasterDemEncoding::Terrarium);
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
fn style_source_type_and_info_call_real_c_api() {
    let runtime = RuntimeHandle::new().unwrap();
    let map = MapHandle::new(&runtime).unwrap();
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
fn custom_geometry_source_apis_call_real_c_api_and_style_replacement_releases_state() {
    let runtime = RuntimeHandle::new().unwrap();
    let map = MapHandle::new(&runtime).unwrap();
    map.set_style_json(VALID_STYLE_JSON).unwrap();

    map.add_custom_geometry_source(
        "custom",
        CustomGeometrySourceOptions::new(|_| {})
            .with_cancel_tile(|_| {})
            .with_min_zoom(0.0)
            .with_max_zoom(2.0)
            .with_tolerance(0.375)
            .with_tile_size(512)
            .with_buffer(64)
            .with_clip(true)
            .with_wrap(false),
    )
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
fn custom_geometry_source_state_is_released_on_map_close() {
    let runtime = RuntimeHandle::new().unwrap();
    let map = MapHandle::new(&runtime).unwrap();
    map.set_style_json(VALID_STYLE_JSON).unwrap();
    map.add_custom_geometry_source("custom", CustomGeometrySourceOptions::new(|_| {}))
        .unwrap();
    assert_eq!(map.custom_geometry_source_count_for_testing(), 1);

    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
fn custom_geometry_source_state_ignores_stale_style_loaded_events() {
    let runtime = RuntimeHandle::new().unwrap();
    let map = MapHandle::new(&runtime).unwrap();
    map.set_style_json(VALID_STYLE_JSON).unwrap();
    map.add_custom_geometry_source("custom", CustomGeometrySourceOptions::new(|_| {}))
        .unwrap();

    let mut event = empty_runtime_event();
    event.type_ = sys::MLN_RUNTIME_EVENT_MAP_STYLE_LOADED;
    event.source_type = sys::MLN_RUNTIME_EVENT_SOURCE_MAP;
    event.source = map.inner.handle.as_ptr().cast();
    runtime.inner.apply_event_side_effects_for_testing(&event);

    assert_eq!(map.custom_geometry_source_count_for_testing(), 1);
    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
fn custom_geometry_source_state_releases_detached_sources_on_style_loaded_event() {
    let runtime = RuntimeHandle::new().unwrap();
    let map = MapHandle::new(&runtime).unwrap();
    map.set_style_json(VALID_STYLE_JSON).unwrap();
    map.add_custom_geometry_source("custom", CustomGeometrySourceOptions::new(|_| {}))
        .unwrap();

    let source_id = maplibre_native_support::string::string_view("custom");
    let mut removed = false;
    // SAFETY: map is live, source_id is valid for this call, and removed
    // points to writable storage. This bypasses the binding cleanup path to
    // model native style replacement detaching the source.
    let status = unsafe {
        sys::mln_map_remove_style_source(map.inner.handle.as_ptr(), source_id.raw(), &mut removed)
    };
    assert_eq!(status, sys::MLN_STATUS_OK);
    assert!(removed);
    assert_eq!(map.custom_geometry_source_count_for_testing(), 1);

    let mut event = empty_runtime_event();
    event.type_ = sys::MLN_RUNTIME_EVENT_MAP_STYLE_LOADED;
    event.source_type = sys::MLN_RUNTIME_EVENT_SOURCE_MAP;
    event.source = map.inner.handle.as_ptr().cast();
    runtime.inner.apply_event_side_effects_for_testing(&event);

    assert_eq!(map.custom_geometry_source_count_for_testing(), 0);
    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
fn custom_geometry_source_adds_to_current_style_after_url_style_request() {
    let runtime = RuntimeHandle::new().unwrap();
    let map = MapHandle::new(&runtime).unwrap();
    map.set_style_json(VALID_STYLE_JSON).unwrap();
    map.set_style_url("unsupported://style.json").unwrap();

    map.add_custom_geometry_source("custom", CustomGeometrySourceOptions::new(|_| {}))
        .unwrap();

    assert_eq!(map.custom_geometry_source_count_for_testing(), 1);
    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
fn custom_geometry_source_state_releases_after_url_style_replacement() {
    let runtime = RuntimeHandle::new().unwrap();
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
    let map = MapHandle::new(&runtime).unwrap();
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
        runtime.run_once().unwrap();
        while runtime.poll_event().unwrap().is_some() {}
    }
}

fn wait_for_map_event(runtime: &RuntimeHandle, map: &MapHandle, event_type: RuntimeEventType) {
    for _ in 0..1000 {
        runtime.run_once().unwrap();
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
fn style_json_descriptors_copy_owned_rust_values() {
    let runtime = RuntimeHandle::new().unwrap();
    let map = MapHandle::new(&runtime).unwrap();
    map.set_style_json(VALID_STYLE_JSON).unwrap();

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

    let error = map
        .set_layer_filter("owned-background", Some(&JsonValue::Double(f64::NAN)))
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    assert_eq!(error.raw_status(), None);

    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
fn camera_jump_and_coordinate_conversions_round_trip() {
    let runtime = RuntimeHandle::new().unwrap();
    let map = RuntimeHandle::create_map_with_options(
        &runtime,
        &MapOptions::new(512, 512, 1.0).with_mode(MapMode::Continuous),
    )
    .unwrap();
    let center = LatLng::new(45.0, -122.0);

    map.jump_to(&CameraOptions::new().with_center(center).with_zoom(4.0))
        .unwrap();
    let camera = map.camera().unwrap();
    assert_eq!(camera.center, Some(center));
    assert_eq!(camera.zoom, Some(4.0));

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

#[test]
fn empty_coordinate_slice_is_rejected_before_calling_c() {
    let runtime = RuntimeHandle::new().unwrap();
    let map = MapHandle::new(&runtime).unwrap();
    let fit = CameraFitOptions::new().with_padding(EdgeInsets::new(1.0, 1.0, 1.0, 1.0));

    let error = map.camera_for_lat_lngs(&[], Some(&fit)).unwrap_err();

    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    assert_eq!(error.raw_status(), None);
    assert!(error.diagnostic().contains("at least one coordinate"));

    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
fn projection_mode_round_trips_through_real_c_api() {
    let runtime = RuntimeHandle::new().unwrap();
    let map = MapHandle::new(&runtime).unwrap();

    let projection_mode = ProjectionMode::new()
        .with_axonometric(false)
        .with_x_skew(0.0)
        .with_y_skew(0.0);
    map.set_projection_mode(&projection_mode).unwrap();
    let copied_projection_mode = map.projection_mode().unwrap();

    assert_eq!(copied_projection_mode.axonometric, Some(false));

    map.close().unwrap();
    runtime.close().unwrap();
}
