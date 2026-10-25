//! Generated value shapes: strings, presence fields, strided batches and
//! unions, open enums and wide integers, and copied arrays.

use maplibre_native_ffi::*;

use crate::support::*;

#[test]
fn discarded_commands_are_fenced_and_sync_rejections_return_errors() {
    let fixture = Fixture::new();
    fixture
        .map()
        .discarding()
        .set_debug_options(MapDebugFlag::TILE_BORDERS)
        .unwrap();
    // A later command on the same map completes after the discarded one.
    wait_for(fixture.map().request_repaint());
    assert_eq!(
        fixture.map().get_snapshot().unwrap().debug_options,
        MapDebugFlag::TILE_BORDERS
    );
    let error = fixture
        .map()
        .discarding()
        .resize(LogicalExtent {
            width: 0,
            height: 8,
            scale_factor: 1.0,
        })
        .unwrap_err();
    assert_eq!(error.status(), Status::InvalidArgument);
}

#[test]
fn host_strings_use_views_and_native_lent_strings_decode() {
    let fixture = Fixture::new();
    wait_for(fixture.runtime().set_resource_provider(serving_provider(
        "custom://style.json",
        BACKGROUND_STYLE_JSON,
    )));

    // A style document is JSON text in an explicit-length view: the bytes
    // after its end would make it invalid JSON if native read past the length,
    // and the loaded style decodes back to the same text.
    let padded = format!("{BACKGROUND_STYLE_JSON}, trailing bytes");
    let document = &padded[..BACKGROUND_STYLE_JSON.len()];
    wait_for(fixture.map().set_style_json(document));
    assert_eq!(
        wait_for(fixture.map().get_loaded_style_json()),
        BACKGROUND_STYLE_JSON
    );

    // A style URL crosses as a view, and the native-lent provider request
    // decodes to exactly that URL.
    fixture
        .map()
        .discarding()
        .set_style_url("custom://style.json")
        .unwrap();
    fixture.await_event_type(RuntimeEventType::MapStyleLoaded);
    assert_eq!(
        wait_for(fixture.map().get_style_url()),
        "custom://style.json"
    );

    // Native rejects a NUL in a URL before it reaches a C-string consumer.
    let error = fixture
        .map()
        .set_style_url("custom://style\0.json")
        .unwrap_err();
    assert_eq!(error.status(), Status::InvalidArgument);
    assert!(error.diagnostic().contains("embedded NUL"), "{error}");
}

#[test]
fn presence_fields_keep_a_present_zero_apart_from_an_absent_value() {
    let fixture = Fixture::new();
    fixture.load_style(BACKGROUND_STYLE_JSON);

    let options = StyleTransitionOptions {
        duration_ms: Some(0.0),
        delay_ms: None,
        enable_placement_transitions: Some(false),
    };
    wait_for(fixture.map().set_style_transition_options(&options));
    let read_back = wait_for(fixture.map().get_style_transition_options());
    assert_eq!(read_back, options);
    assert_eq!(read_back.duration_ms, Some(0.0));
    assert_eq!(read_back.delay_ms, None);
}

#[test]
fn open_enums_keep_unknown_values_and_64_bit_masks_cross_whole() {
    // An enum value this binding predates reaches native unchanged, and native
    // is what rejects it.
    let error = network_set_status(NetworkStatus::Unknown(900)).unwrap_err();
    assert_eq!(error.status(), Status::InvalidArgument);
    assert!(error.diagnostic().contains("network status"), "{error}");

    let fixture = Fixture::new();
    // Only bit 63 is set, so a mask truncated to 32 bits would read as empty
    // and be accepted.
    let error = fixture
        .runtime()
        .set_event_mask(RuntimeEventFlag::from_bits_retain(1 << 63))
        .unwrap_err();
    assert_eq!(error.status(), Status::InvalidArgument);
    assert_eq!(
        fixture.runtime().get_event_mask().unwrap(),
        RuntimeEventFlag::ALL
    );
}

#[test]
fn array_inputs_are_copied_when_they_are_submitted() {
    let fixture = Fixture::new();
    fixture.load_style(BACKGROUND_STYLE_JSON);

    let mut coordinates = vec![
        LatLng::new(0.0, 0.0),
        LatLng::new(0.0, 1.0),
        LatLng::new(1.0, 1.0),
        LatLng::new(1.0, 0.0),
    ];
    let expected_coordinates = coordinates.clone();
    let mut image = PremultipliedRgba8Image {
        width: 2,
        height: 2,
        stride: 8,
        pixels: (0..16).collect(),
    };
    let expected_pixels = image.pixels.clone();
    let added_source = fixture
        .map()
        .add_image_source_image("image", &coordinates, &image)
        .unwrap();
    let added_image = fixture.map().set_style_image("icon", &image, None).unwrap();

    // The caller may reuse its arrays as soon as the calls return.
    coordinates.fill(LatLng::new(9.0, 9.0));
    image.pixels.fill(0);
    wait_on(added_source);
    wait_on(added_image);

    assert_eq!(
        wait_for(fixture.map().get_image_source_coordinates("image")),
        expected_coordinates
    );
    let copied = wait_for(fixture.map().get_style_image("icon")).unwrap();
    assert_eq!(copied.pixels, expected_pixels);
}

#[test]
fn a_record_built_from_its_default_equals_the_native_default() {
    assert_eq!(MapOptions::default(), map_options_default());
}

// An enum without a zero member has no default rather than an unknown 0, and
// a record that holds one has none either.
static_assertions::assert_not_impl_any!(AmbientCacheOperation: Default);
static_assertions::assert_not_impl_any!(RuntimeEvent: Default);

#[test]
fn an_enum_defaults_to_its_zero_member() {
    assert_eq!(OpenglClientApi::default(), OpenglClientApi::Unspecified);
    assert_eq!(OpenglClientApi::default().as_raw(), 0);
}

#[test]
fn synchronous_calls_fill_caller_sized_outputs_in_input_order() {
    let fixture = Fixture::new();
    let projection = map_projection_create(&fixture.map().get_snapshot().unwrap().view).unwrap();
    let coordinates = [
        LatLng::new(0.0, 0.0),
        LatLng::new(10.0, 20.0),
        LatLng::new(-12.0, -30.0),
    ];
    let points = projection.pixels_for_lat_lngs(&coordinates).unwrap();
    let singles: Vec<_> = coordinates
        .iter()
        .map(|&value| projection.pixel_for_lat_lng(value).unwrap())
        .collect();
    assert_eq!(points, singles);
    let wrapped: Vec<_> = points
        .iter()
        .map(|&point| projection.lat_lng_for_pixel(point).unwrap())
        .collect();
    assert_eq!(projection.lat_lngs_for_pixels(&points).unwrap(), wrapped);
    let unwrapped: Vec<_> = points
        .iter()
        .map(|&point| projection.lat_lng_for_pixel_unwrapped(point).unwrap())
        .collect();
    assert_eq!(
        projection.lat_lngs_for_pixels_unwrapped(&points).unwrap(),
        unwrapped
    );
    assert!(projection.pixels_for_lat_lngs(&[]).unwrap().is_empty());
    assert!(projection.lat_lngs_for_pixels(&[]).unwrap().is_empty());
    assert!(
        projection
            .lat_lngs_for_pixels_unwrapped(&[])
            .unwrap()
            .is_empty()
    );
    projection.release().unwrap();
}

#[test]
fn caller_sized_text_returns_results_longer_than_its_first_storage() {
    let zoom = style_expression_create(
        r#"["interpolate", ["linear"], ["zoom"], 0, 0, 10, 100]"#,
        None,
    )
    .unwrap();
    let inputs = StyleExpressionInputs {
        zoom: Some(5.0),
        ..Default::default()
    };
    assert_eq!(zoom.evaluate(Some(&inputs)).unwrap(), "50.0");
    zoom.release().unwrap();

    // The result outgrows the runtime's first storage, so the call runs again.
    let text = format!("\"{}\"", "a".repeat(1000));
    let long = style_expression_create(&text, None).unwrap();
    assert_eq!(long.evaluate(None).unwrap(), text);
    long.release().unwrap();
}
