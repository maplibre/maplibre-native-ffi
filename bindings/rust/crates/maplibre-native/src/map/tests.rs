use super::*;
use crate::events::empty_runtime_event;
use crate::{
    ConstrainMode, CustomGeometrySourceOptions, EdgeInsets, ErrorKind, Feature, FeatureIdentifier,
    JsonMember, MapMode, NorthOrientation, TextureImageInfo, TileLodMode, ViewportMode,
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
fn map_create_and_close() {
    let runtime = RuntimeHandle::new().unwrap();
    let map = MapHandle::new(&runtime).unwrap();

    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
fn map_create_with_options_and_close() {
    let runtime = RuntimeHandle::new().unwrap();
    let options = MapOptions::new(320, 240, 2.0).with_mode(MapMode::Static);
    let map = MapHandle::with_options(&runtime, &options).unwrap();

    map.close().unwrap();
    runtime.close().unwrap();
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

    let error = map.style_source_exists("").unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    assert!(error.raw_status().is_some());

    let error = map.remove_style_source("").unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    assert!(error.raw_status().is_some());

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
fn style_image_add_query_copy_and_remove_call_real_c_api() {
    let runtime = RuntimeHandle::new().unwrap();
    let map = MapHandle::new(&runtime).unwrap();
    map.set_style_json(VALID_STYLE_JSON).unwrap();

    let plain = test_style_image(vec![
        255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255, 255, 255, 255, 255,
    ]);
    let sdf = test_style_image(vec![1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16]);

    assert!(!map.style_image_exists("plain").unwrap());
    assert_eq!(map.style_image_info("plain").unwrap(), None);
    assert_eq!(
        map.copy_style_image_premultiplied_rgba8("plain").unwrap(),
        None
    );
    assert!(!map.remove_style_image("plain").unwrap());

    map.set_style_image("plain", &plain, None).unwrap();
    assert!(map.style_image_exists("plain").unwrap());
    let info = map.style_image_info("plain").unwrap().unwrap();
    assert_eq!(info.width, 2);
    assert_eq!(info.height, 2);
    assert_eq!(info.stride, 8);
    assert_eq!(info.byte_length, 16);
    assert_eq!(info.pixel_ratio, 1.0);
    assert!(!info.sdf);
    let copied = map
        .copy_style_image_premultiplied_rgba8("plain")
        .unwrap()
        .unwrap();
    assert_eq!(copied.image.info.width, info.width);
    assert_eq!(copied.image.info.height, info.height);
    assert_eq!(copied.image.info.stride, info.stride);
    assert_eq!(copied.image.info.byte_length, info.byte_length);
    assert_eq!(copied.pixel_ratio, info.pixel_ratio);
    assert_eq!(copied.sdf, info.sdf);
    assert_eq!(copied.image.data, plain.data);

    map.set_style_image(
        "sdf",
        &sdf,
        Some(
            &StyleImageOptions::new()
                .with_pixel_ratio(2.0)
                .with_sdf(true),
        ),
    )
    .unwrap();
    let info = map.style_image_info("sdf").unwrap().unwrap();
    assert_eq!(info.pixel_ratio, 2.0);
    assert!(info.sdf);
    let copied = map
        .copy_style_image_premultiplied_rgba8("sdf")
        .unwrap()
        .unwrap();
    assert_eq!(copied.pixel_ratio, 2.0);
    assert!(copied.sdf);
    assert_eq!(copied.image.data, sdf.data);

    let replacement = test_style_image(vec![16; 16]);
    map.set_style_image(
        "sdf",
        &replacement,
        Some(&StyleImageOptions::new().with_sdf(false)),
    )
    .unwrap();
    let info = map.style_image_info("sdf").unwrap().unwrap();
    assert_eq!(info.pixel_ratio, 1.0);
    assert!(!info.sdf);
    let copied = map
        .copy_style_image_premultiplied_rgba8("sdf")
        .unwrap()
        .unwrap();
    assert_eq!(copied.image.data, replacement.data);

    assert!(map.remove_style_image("plain").unwrap());
    assert!(!map.style_image_exists("plain").unwrap());
    assert!(!map.remove_style_image("plain").unwrap());

    let error = map.style_image_exists("").unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    assert!(error.raw_status().is_some());

    let error = map.set_style_image("", &plain, None).unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    assert!(error.raw_status().is_some());

    let error = map.remove_style_image("").unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    assert!(error.raw_status().is_some());

    let error = map.style_image_info("").unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    assert!(error.raw_status().is_some());

    let error = map.copy_style_image_premultiplied_rgba8("").unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    assert!(error.raw_status().is_some());
}

#[test]
fn style_image_descriptor_materialization_rejects_invalid_images_and_options() {
    let runtime = RuntimeHandle::new().unwrap();
    let map = MapHandle::new(&runtime).unwrap();
    map.set_style_json(VALID_STYLE_JSON).unwrap();

    let too_short = PremultipliedRgba8Image {
        info: TextureImageInfo {
            width: 2,
            height: 2,
            stride: 8,
            byte_length: 16,
        },
        data: vec![0; 15],
    };
    let error = map.set_style_image("bad", &too_short, None).unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    assert!(error.raw_status().is_some());

    let image = test_style_image(vec![0; 16]);
    let error = map
        .set_style_image(
            "bad-options",
            &image,
            Some(&StyleImageOptions::new().with_pixel_ratio(0.0)),
        )
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    assert!(error.raw_status().is_some());
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
fn image_source_url_add_get_and_update_coordinates_call_real_c_api() {
    let runtime = RuntimeHandle::new().unwrap();
    let map = MapHandle::new(&runtime).unwrap();
    map.set_style_json(VALID_STYLE_JSON).unwrap();

    let coordinates = image_source_coordinates();
    assert_eq!(map.image_source_coordinates("missing").unwrap(), None);

    map.add_image_source_url("url-image", &coordinates, "https://example.com/image.png")
        .unwrap();
    assert!(map.style_source_exists("url-image").unwrap());
    assert_eq!(
        map.style_source_type("url-image").unwrap(),
        Some(SourceType::Image)
    );
    assert_eq!(
        map.image_source_coordinates("url-image").unwrap(),
        Some(coordinates)
    );

    let error = map
        .set_image_source_url("missing", "https://example.com/missing.png")
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    assert!(error.raw_status().is_some());

    map.set_image_source_url("url-image", "https://example.com/replacement.png")
        .unwrap();

    let error = map.set_image_source_url("url-image", "").unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    assert!(error.raw_status().is_some());
    let updated = [
        LatLng::new(2.0, 2.0),
        LatLng::new(2.0, 3.0),
        LatLng::new(3.0, 3.0),
        LatLng::new(3.0, 2.0),
    ];
    map.set_image_source_coordinates("url-image", &updated)
        .unwrap();
    assert_eq!(
        map.image_source_coordinates("url-image").unwrap(),
        Some(updated)
    );

    let error = map
        .add_image_source_url("", &coordinates, "https://example.com/a.png")
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    assert!(error.raw_status().is_some());

    let error = map
        .add_image_source_url("bad-url", &coordinates, "")
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    assert!(error.raw_status().is_some());

    let error = map
        .set_image_source_coordinates("missing", &coordinates)
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    assert!(error.raw_status().is_some());

    let error = map.image_source_coordinates("").unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    assert!(error.raw_status().is_some());
}

#[test]
fn image_source_inline_image_add_and_update_call_real_c_api() {
    let runtime = RuntimeHandle::new().unwrap();
    let map = MapHandle::new(&runtime).unwrap();
    map.set_style_json(VALID_STYLE_JSON).unwrap();

    let coordinates = image_source_coordinates();
    let image = test_style_image(vec![1; 16]);
    map.add_image_source_image("inline-image", &coordinates, &image)
        .unwrap();
    assert_eq!(
        map.style_source_type("inline-image").unwrap(),
        Some(SourceType::Image)
    );
    assert_eq!(
        map.image_source_coordinates("inline-image").unwrap(),
        Some(coordinates)
    );

    let replacement = test_style_image(vec![2; 16]);
    let error = map
        .set_image_source_image("missing", &replacement)
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    assert!(error.raw_status().is_some());

    map.set_image_source_image("inline-image", &replacement)
        .unwrap();

    let too_short = PremultipliedRgba8Image {
        info: TextureImageInfo {
            width: 2,
            height: 2,
            stride: 8,
            byte_length: 16,
        },
        data: vec![0; 15],
    };
    let error = map
        .set_image_source_image("inline-image", &too_short)
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    assert!(error.raw_status().is_some());
    map.set_image_source_url("inline-image", "https://example.com/after-inline.png")
        .unwrap();

    let error = map.set_image_source_image("", &replacement).unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    assert!(error.raw_status().is_some());
}

#[test]
fn image_source_methods_reject_non_image_sources() {
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
    map.add_style_source_json("geo", &geojson_source).unwrap();

    let error = map.image_source_coordinates("geo").unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    assert!(error.raw_status().is_some());

    let error = map
        .set_image_source_url("geo", "https://example.com/not-image.png")
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    assert!(error.raw_status().is_some());

    let image = test_style_image(vec![3; 16]);
    let error = map.set_image_source_image("geo", &image).unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    assert!(error.raw_status().is_some());

    let coordinates = image_source_coordinates();
    let error = map
        .set_image_source_coordinates("geo", &coordinates)
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    assert!(error.raw_status().is_some());
}

#[test]
fn tile_source_helpers_call_real_c_api() {
    let runtime = RuntimeHandle::new().unwrap();
    let map = MapHandle::new(&runtime).unwrap();
    map.set_style_json(VALID_STYLE_JSON).unwrap();

    let vector_options = TileSourceOptions::new()
        .with_min_zoom(1.0)
        .with_max_zoom(12.0)
        .with_attribution("© vector")
        .with_scheme(TileScheme::Xyz)
        .with_bounds(LatLngBounds::new(
            LatLng::new(-10.0, -20.0),
            LatLng::new(10.0, 20.0),
        ))
        .with_vector_encoding(VectorTileEncoding::Mvt);
    map.add_vector_source_url(
        "vector-url",
        "https://example.com/vector.json",
        Some(&vector_options),
    )
    .unwrap();
    assert_eq!(
        map.style_source_type("vector-url").unwrap(),
        Some(SourceType::Vector)
    );

    map.add_vector_source_tiles(
        "vector-tiles",
        &["https://example.com/vector/{z}/{x}/{y}.pbf"],
        None,
    )
    .unwrap();
    assert_eq!(
        map.style_source_type("vector-tiles").unwrap(),
        Some(SourceType::Vector)
    );

    let raster_options = TileSourceOptions::new()
        .with_tile_size(256)
        .with_scheme(TileScheme::Tms)
        .with_attribution("© raster");
    map.add_raster_source_url(
        "raster-url",
        "https://example.com/raster.json",
        Some(&raster_options),
    )
    .unwrap();
    assert_eq!(
        map.style_source_type("raster-url").unwrap(),
        Some(SourceType::Raster)
    );

    map.add_raster_source_tiles(
        "raster-tiles",
        &["https://example.com/raster/{z}/{x}/{y}.png"],
        None,
    )
    .unwrap();
    assert_eq!(
        map.style_source_type("raster-tiles").unwrap(),
        Some(SourceType::Raster)
    );

    let dem_options = TileSourceOptions::new()
        .with_tile_size(512)
        .with_raster_dem_encoding(RasterDemEncoding::Terrarium);
    map.add_raster_dem_source_url(
        "dem-url",
        "https://example.com/dem.json",
        Some(&dem_options),
    )
    .unwrap();
    assert_eq!(
        map.style_source_type("dem-url").unwrap(),
        Some(SourceType::RasterDem)
    );

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

    let error = map
        .add_vector_source_url("", "https://example.com/vector.json", None)
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);

    let error = map
        .add_raster_source_tiles("empty-tiles", &[] as &[&str], None)
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);

    let vector_only_options =
        TileSourceOptions::new().with_vector_encoding(VectorTileEncoding::Mvt);
    let error = map
        .add_raster_source_tiles(
            "raster-with-vector-option",
            &["https://example.com/raster/{z}/{x}/{y}.png"],
            Some(&vector_only_options),
        )
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    assert!(error.raw_status().is_some());

    let dem_only_options =
        TileSourceOptions::new().with_raster_dem_encoding(RasterDemEncoding::Terrarium);
    let error = map
        .add_vector_source_tiles(
            "vector-with-dem-option",
            &["https://example.com/vector/{z}/{x}/{y}.pbf"],
            Some(&dem_only_options),
        )
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    assert!(error.raw_status().is_some());

    let error = map
        .add_raster_source_tiles(
            "raster-with-dem-option",
            &["https://example.com/raster/{z}/{x}/{y}.png"],
            Some(&dem_only_options),
        )
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    assert!(error.raw_status().is_some());
}

#[test]
fn terrain_and_location_layer_helpers_call_real_c_api() {
    let runtime = RuntimeHandle::new().unwrap();
    let map = MapHandle::new(&runtime).unwrap();
    map.set_style_json(VALID_STYLE_JSON).unwrap();

    map.add_raster_dem_source_tiles(
        "dem",
        &["https://example.com/dem/{z}/{x}/{y}.png"],
        Some(&TileSourceOptions::new().with_raster_dem_encoding(RasterDemEncoding::Mapbox)),
    )
    .unwrap();
    map.add_hillshade_layer("hillshade", "dem", None).unwrap();
    map.add_color_relief_layer("color-relief", "dem", None)
        .unwrap();

    map.add_location_indicator_layer("location", None).unwrap();
    map.set_location_indicator_location("location", LatLng::new(37.8, -122.4), 12.0)
        .unwrap();
    map.set_location_indicator_bearing("location", 45.0)
        .unwrap();
    map.set_location_indicator_accuracy_radius("location", 24.0)
        .unwrap();
    map.set_location_indicator_image_name(
        "location",
        LocationIndicatorImageKind::Top,
        "location-top",
    )
    .unwrap();

    assert_eq!(
        map.layer_property("location", "location").unwrap(),
        Some(JsonValue::Array(vec![
            JsonValue::Double(-122.4),
            JsonValue::Double(37.8),
            JsonValue::Double(12.0),
        ]))
    );
    assert_eq!(
        map.layer_property("location", "bearing").unwrap(),
        Some(JsonValue::Double(45.0))
    );
    assert_eq!(
        map.layer_property("location", "accuracy-radius").unwrap(),
        Some(JsonValue::Double(24.0))
    );
    assert_eq!(
        map.layer_property("location", "top-image").unwrap(),
        Some(JsonValue::Object(vec![
            JsonMember::new("available", JsonValue::Bool(false)),
            JsonMember::new("name", JsonValue::String("location-top".to_owned())),
        ]))
    );

    let error = map.add_hillshade_layer("", "dem", None).unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);

    map.add_raster_source_tiles(
        "raster",
        &["https://example.com/raster/{z}/{x}/{y}.png"],
        None,
    )
    .unwrap();
    let error = map
        .add_hillshade_layer("wrong-source-type", "raster", None)
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);

    let error = map
        .set_location_indicator_image_name("location", LocationIndicatorImageKind::Bearing, "")
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
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

    let error = map.style_source_type("").unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    assert!(error.raw_status().is_some());

    let error = map.style_source_info("").unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    assert!(error.raw_status().is_some());

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
fn custom_geometry_source_state_releases_on_pending_url_style_loaded_event() {
    let runtime = RuntimeHandle::new().unwrap();
    let map = MapHandle::new(&runtime).unwrap();
    map.set_style_json(VALID_STYLE_JSON).unwrap();
    map.add_custom_geometry_source("custom", CustomGeometrySourceOptions::new(|_| {}))
        .unwrap();
    map.inner.mark_custom_geometry_sources_pending_url_cleanup();

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
fn custom_geometry_source_add_rejects_pending_url_style_replacement() {
    let runtime = RuntimeHandle::new().unwrap();
    let map = MapHandle::new(&runtime).unwrap();
    map.set_style_json(VALID_STYLE_JSON).unwrap();
    map.inner.mark_custom_geometry_sources_pending_url_cleanup();

    let error = map
        .add_custom_geometry_source("custom", CustomGeometrySourceOptions::new(|_| {}))
        .unwrap_err();

    assert_eq!(error.kind(), ErrorKind::InvalidState);
    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
fn style_json_and_geojson_descriptors_call_real_c_api() {
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
    map.add_style_source_json("owned-json-source", &source)
        .unwrap();

    let geojson = GeoJson::Feature(
        Feature::new(
            Geometry::Point(LatLng::new(1.0, 2.0)),
            vec![JsonMember::new("name", JsonValue::String("one".to_owned()))],
        )
        .with_identifier(FeatureIdentifier::String("feature-1".to_owned())),
    );
    map.add_geojson_source_data("owned-geojson-source", &geojson)
        .unwrap();
    map.set_geojson_source_data(
        "owned-geojson-source",
        &GeoJson::FeatureCollection(Vec::new()),
    )
    .unwrap();

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
    assert_eq!(
        object_member(&copied_layer, "type"),
        Some(&JsonValue::String("background".to_owned()))
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

    let error = map
        .set_layer_filter("owned-background", Some(&JsonValue::Double(f64::NAN)))
        .err()
        .unwrap();
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
fn camera_commands_and_queries_use_real_c_api() {
    let runtime = RuntimeHandle::new().unwrap();
    let map = MapHandle::new(&runtime).unwrap();
    let camera = CameraOptions::new()
        .with_center(LatLng::new(0.0, 0.0))
        .with_zoom(1.0);
    let animation = AnimationOptions::new().with_duration_ms(0.0);

    map.ease_to(&camera, Some(&animation)).unwrap();
    map.fly_to(&camera, Some(&animation)).unwrap();
    map.move_by(0.0, 0.0).unwrap();
    map.move_by_animated(0.0, 0.0, Some(&animation)).unwrap();
    map.scale_by(1.0, Some(ScreenPoint::new(128.0, 128.0)))
        .unwrap();
    map.scale_by_animated(1.0, None, Some(&animation)).unwrap();
    map.rotate_by(ScreenPoint::new(0.0, 0.0), ScreenPoint::new(0.0, 0.0))
        .unwrap();
    map.rotate_by_animated(
        ScreenPoint::new(0.0, 0.0),
        ScreenPoint::new(0.0, 0.0),
        Some(&animation),
    )
    .unwrap();
    map.pitch_by(0.0).unwrap();
    map.pitch_by_animated(0.0, Some(&animation)).unwrap();
    map.cancel_transitions().unwrap();

    let bounds = LatLngBounds::new(LatLng::new(-1.0, -1.0), LatLng::new(1.0, 1.0));
    let fit = CameraFitOptions::new().with_padding(EdgeInsets::new(1.0, 1.0, 1.0, 1.0));
    map.camera_for_lat_lng_bounds(bounds, Some(&fit)).unwrap();
    map.camera_for_lat_lngs(&[LatLng::new(0.0, 0.0), LatLng::new(1.0, 1.0)], Some(&fit))
        .unwrap();
    let error = map.camera_for_lat_lngs(&[], Some(&fit)).unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    assert_eq!(error.raw_status(), None);
    assert!(error.diagnostic().contains("at least one coordinate"));
    map.camera_for_geometry(
        &Geometry::LineString(vec![LatLng::new(0.0, 0.0), LatLng::new(1.0, 1.0)]),
        Some(&fit),
    )
    .unwrap();
    map.lat_lng_bounds_for_camera(&camera).unwrap();
    map.lat_lng_bounds_for_camera_unwrapped(&camera).unwrap();

    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
fn map_state_viewport_tile_debug_and_projection_mode_round_trip() {
    let runtime = RuntimeHandle::new().unwrap();
    let map = MapHandle::new(&runtime).unwrap();

    map.set_debug_options(MapDebugOptions::TILE_BORDERS | MapDebugOptions::PARSE_STATUS)
        .unwrap();
    let debug = map.debug_options().unwrap();
    assert!(debug.contains(MapDebugOptions::TILE_BORDERS));
    assert!(debug.contains(MapDebugOptions::PARSE_STATUS));

    map.set_rendering_stats_view_enabled(true).unwrap();
    assert!(map.rendering_stats_view_enabled().unwrap());
    assert!(!map.is_fully_loaded().unwrap());
    map.dump_debug_logs().unwrap();

    let viewport = MapViewportOptions::new()
        .with_north_orientation(NorthOrientation::Up)
        .with_constrain_mode(ConstrainMode::HeightOnly)
        .with_viewport_mode(ViewportMode::Default)
        .with_frustum_offset(EdgeInsets::new(0.0, 0.0, 0.0, 0.0));
    map.set_viewport_options(&viewport).unwrap();
    let copied_viewport = map.viewport_options().unwrap();
    assert_eq!(
        copied_viewport.north_orientation,
        Some(NorthOrientation::Up)
    );
    assert_eq!(
        copied_viewport.constrain_mode,
        Some(ConstrainMode::HeightOnly)
    );
    assert_eq!(copied_viewport.viewport_mode, Some(ViewportMode::Default));

    let tile = MapTileOptions::new()
        .with_prefetch_zoom_delta(1)
        .with_lod_mode(TileLodMode::Default);
    map.set_tile_options(&tile).unwrap();
    let copied_tile = map.tile_options().unwrap();
    assert_eq!(copied_tile.prefetch_zoom_delta, Some(1));
    assert_eq!(copied_tile.lod_mode, Some(TileLodMode::Default));

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

#[test]
fn bounds_and_free_camera_operations_call_c_api() {
    let runtime = RuntimeHandle::new().unwrap();
    let map = MapHandle::new(&runtime).unwrap();

    let bounds = BoundOptions::new()
        .with_bounds(LatLngBounds::new(
            LatLng::new(-10.0, -20.0),
            LatLng::new(10.0, 20.0),
        ))
        .with_min_zoom(0.0)
        .with_max_zoom(20.0)
        .with_min_pitch(0.0)
        .with_max_pitch(60.0);
    map.set_bounds(&bounds).unwrap();
    let copied_bounds = map.bounds().unwrap();
    assert_eq!(copied_bounds.min_zoom, Some(0.0));
    assert_eq!(copied_bounds.max_zoom, Some(20.0));

    let free = map.free_camera_options().unwrap();
    map.set_free_camera_options(&free).unwrap();

    map.close().unwrap();
    runtime.close().unwrap();
}
