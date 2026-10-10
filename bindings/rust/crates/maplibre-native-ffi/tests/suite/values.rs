//! Generated value shapes: strings, presence fields, strided batches and
//! unions, open enums and wide integers, and copied arrays.

use maplibre_native_ffi::*;
use maplibre_native_ffi_sys as sys;

use crate::support::*;

#[test]
fn strings_cross_as_terminated_and_explicit_length_and_embedded_nul_is_rejected() {
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
        wait_for(fixture.map().loaded_style_json()),
        BACKGROUND_STYLE_JSON.as_bytes()
    );

    // A style URL crosses NUL-terminated, and the provider that serves it
    // sees exactly that URL.
    fixture.map().set_style_url("custom://style.json").unwrap();
    fixture.await_event_type(RuntimeEventType::MapStyleLoaded);
    assert_eq!(wait_for(fixture.map().style_url()), "custom://style.json");

    // A terminated string cannot carry a NUL, so the binding rejects it before
    // any native call.
    let error = fixture
        .map()
        .set_style_url("custom://style\0.json")
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    assert_eq!(error.raw_status(), None);
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
fn a_strided_batch_and_an_unknown_union_arm_decode_without_losing_data() {
    // A batch from a newer library: each event record is longer than this
    // binding's, and the second event carries a payload this binding predates.
    let stride = std::mem::size_of::<sys::mln_runtime_event>() + 24;
    let messages = b"first\0second\0";
    let events = [
        (
            sys::MLN_RUNTIME_EVENT_MAP_IDLE,
            sys::MLN_RUNTIME_EVENT_PAYLOAD_NONE,
            0,
            5,
        ),
        (sys::MLN_RUNTIME_EVENT_MAP_STYLE_LOADED, 0xfff0, 6, 6),
    ];
    let mut storage = vec![0_u8; stride * events.len()];
    for (index, (event_type, payload_type, offset, length)) in events.into_iter().enumerate() {
        // SAFETY: the record is plain data, and all zeroes is a valid value.
        let mut event: sys::mln_runtime_event = unsafe { std::mem::zeroed() };
        event.type_ = event_type;
        event.source_type = sys::MLN_RUNTIME_EVENT_SOURCE_MAP;
        event.source = 0xabcd_0000_0000_0000 | index as u64;
        event.generation = 0x1_0000_0000 + index as u64;
        event.payload_type = payload_type;
        event.message_offset = offset;
        event.message_size = length;
        // SAFETY: the storage holds a full stride for every event, and the
        // write does not rely on its alignment.
        unsafe {
            storage
                .as_mut_ptr()
                .add(index * stride)
                .cast::<sys::mln_runtime_event>()
                .write_unaligned(event);
        }
    }
    // SAFETY: the view is plain data, and all zeroes is a valid value.
    let mut raw: sys::mln_runtime_event_batch_view = unsafe { std::mem::zeroed() };
    raw.size = std::mem::size_of::<sys::mln_runtime_event_batch_view>() as u32;
    raw.event_size = stride as u32;
    raw.events = storage.as_ptr().cast();
    raw.event_count = events.len();
    raw.messages = messages.as_ptr().cast();
    raw.messages_size = messages.len();

    // SAFETY: every pointer in the view addresses the storage above.
    let view = unsafe { RuntimeEventBatchView::from_native(raw) }.unwrap();
    assert_eq!(view.events.len(), 2);
    assert_eq!(view.events[0].r#type, RuntimeEventType::MapIdle);
    assert_eq!(view.events[0].payload, RuntimeEventPayload::Empty);
    assert_eq!(view.events[0].message, "first");
    assert_eq!(view.events[1].r#type, RuntimeEventType::MapStyleLoaded);
    assert_eq!(view.events[1].source, 0xabcd_0000_0000_0001);
    assert_eq!(view.events[1].generation, 0x1_0000_0001);
    assert_eq!(view.events[1].payload, RuntimeEventPayload::Unknown(0xfff0));
    assert_eq!(view.events[1].message, "second");
}

#[test]
fn open_enums_keep_unknown_values_and_64_bit_masks_cross_whole() {
    // An enum value this binding predates reaches native unchanged, and native
    // is what rejects it.
    let error = network_status_set(NetworkStatus::Unknown(999_999)).unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    assert_eq!(error.raw_status(), Some(sys::MLN_STATUS_INVALID_ARGUMENT));
    assert!(error.diagnostic().contains("network status"), "{error}");

    let fixture = Fixture::new();
    fixture.load_style(BACKGROUND_STYLE_JSON);
    let rejected = wait_for(
        fixture
            .map()
            .set_layer_visibility("background", StyleLayerVisibility::Unknown(900)),
    );
    assert_eq!(rejected.disposition, CommandDisposition::Failed);

    // Only bit 63 is set, so a mask truncated to 32 bits would read as empty
    // and be accepted.
    let error = fixture
        .runtime()
        .set_event_mask(RuntimeEventMask::from_bits_retain(1 << 63))
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    assert_eq!(error.raw_status(), Some(sys::MLN_STATUS_INVALID_ARGUMENT));
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
        Some(expected_coordinates)
    );
    let copied = wait_for(fixture.map().get_style_image_info("icon")).unwrap();
    assert_eq!(copied.pixels, expected_pixels);
}

#[test]
fn a_record_built_from_its_default_equals_the_native_default() {
    assert_eq!(MapOptions::default(), map_options_default().unwrap());
}
