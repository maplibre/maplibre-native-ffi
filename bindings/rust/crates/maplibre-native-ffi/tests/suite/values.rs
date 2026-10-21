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
        .set_debug_options(MapDebugOption::TILE_BORDERS)
        .unwrap();
    // A later command on the same map completes after the discarded one.
    wait_for(fixture.map().request_repaint());
    assert_eq!(
        fixture.map().get_snapshot().unwrap().debug_options,
        MapDebugOption::TILE_BORDERS
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

    // A style document is an explicit-length view: the bytes after its end
    // would make it invalid JSON if native read past the length.
    let padded = format!("{BACKGROUND_STYLE_JSON}, trailing bytes");
    let document = &padded.as_bytes()[..BACKGROUND_STYLE_JSON.len()];
    let loaded = wait_for(fixture.map().set_style_json(document));
    assert_eq!(loaded.disposition, CommandDisposition::Committed);
    assert_eq!(
        wait_for(fixture.map().get_loaded_style_json()),
        BACKGROUND_STYLE_JSON.as_bytes()
    );

    // A style URL crosses as a view, and the native-lent provider request
    // decodes to exactly that URL.
    fixture.map().set_style_url("custom://style.json").unwrap();
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
    let applied = wait_for(fixture.map().set_style_transition_options(&options));
    assert_eq!(applied.disposition, CommandDisposition::Committed);
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
        .set_event_mask(RuntimeEventMask::from_bits_retain(1 << 63))
        .unwrap_err();
    assert_eq!(error.status(), Status::InvalidArgument);
    assert_eq!(
        fixture.runtime().get_event_mask().unwrap(),
        RuntimeEventMask::ALL
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
    assert_eq!(
        wait_for(Ok(added_source)).disposition,
        CommandDisposition::Committed
    );
    assert_eq!(
        wait_for(Ok(added_image)).disposition,
        CommandDisposition::Committed
    );

    assert_eq!(
        wait_for(fixture.map().get_image_source_coordinates("image")),
        expected_coordinates
    );
    let copied = wait_for(fixture.map().get_style_image("icon")).unwrap();
    assert_eq!(copied.pixels, expected_pixels);
}

#[test]
fn a_record_built_from_its_default_equals_the_native_default() {
    assert_eq!(MapOptions::default(), map_options_default().unwrap());
}
