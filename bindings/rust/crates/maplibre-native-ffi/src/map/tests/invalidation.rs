use super::*;

fn take_updates(runtime: &mut RuntimeHandle) -> usize {
    runtime
        .drain_events(0)
        .unwrap()
        .iter()
        .filter(|event| event.event_type() == RuntimeEventType::MapRenderUpdateAvailable)
        .count()
}

#[test]
fn source_and_image_lifecycle_publish_render_updates() {
    let mut runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();
    map.set_style_json(VALID_STYLE_JSON.as_bytes()).unwrap();
    take_updates(&mut runtime);

    let data =
        crate::GeoJsonSourceDataHandle::new(br#"{"type":"FeatureCollection","features":[]}"#, None)
            .unwrap();
    map.add_geojson_source_data("geo", &data).unwrap();
    assert!(take_updates(&mut runtime) > 0, "inline GeoJSON insertion");
    assert!(map.remove_style_source("geo").unwrap());
    assert!(take_updates(&mut runtime) > 0, "source removal");

    let image = PremultipliedRgba8Image::new(TextureImageInfo::new(1, 1, 4, 4), vec![255; 4]);
    let coordinates = [
        LatLng::new(1.0, -1.0),
        LatLng::new(1.0, 1.0),
        LatLng::new(-1.0, 1.0),
        LatLng::new(-1.0, -1.0),
    ];
    map.add_image_source_image("image", &coordinates, &image)
        .unwrap();
    assert!(
        take_updates(&mut runtime) > 0,
        "inline image source insertion"
    );
    map.set_style_image("icon", &image, None).unwrap();
    take_updates(&mut runtime);
    map.remove_style_image("icon").unwrap();
    assert!(take_updates(&mut runtime) > 0, "style image removal");

    data.close();
    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
fn tile_and_transition_options_publish_render_updates() {
    let mut runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();
    map.set_style_json(STYLE_WITH_IDS_JSON.as_bytes()).unwrap();
    take_updates(&mut runtime);
    let initial = map.tile_options().unwrap();
    let mut updates: [MapTileOptions; 6] = std::array::from_fn(|_| MapTileOptions::default());
    updates[0].prefetch_zoom_delta = Some(initial.prefetch_zoom_delta.unwrap() + 1);
    updates[1].lod_min_radius = Some(initial.lod_min_radius.unwrap() + 1.0);
    updates[2].lod_scale = Some(initial.lod_scale.unwrap() + 1.0);
    updates[3].lod_pitch_threshold = Some(30.0);
    updates[4].lod_zoom_shift = Some(initial.lod_zoom_shift.unwrap() + 1.0);
    updates[5].lod_mode = Some(crate::TileLodMode::Distance);
    for options in updates {
        map.set_tile_options(&options).unwrap();
        assert!(take_updates(&mut runtime) > 0, "tile options: {options:?}");
        map.set_tile_options(&options).unwrap();
        assert_eq!(take_updates(&mut runtime), 0, "unchanged tile options");
    }
    map.set_layer_property(
        "background",
        "background-color-transition",
        br#"{"duration":123}"#,
    )
    .unwrap();
    assert!(take_updates(&mut runtime) > 0, "layer transition");
    let mut transition = StyleTransitionOptions::default();
    transition.duration_ms = Some(123.0);
    map.set_style_transition_options(&transition).unwrap();
    assert!(take_updates(&mut runtime) > 0, "style transition");
    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
fn only_effective_feature_state_mutations_publish_updates() {
    let mut runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::default()).unwrap();
    map.set_style_json(STYLE_WITH_IDS_JSON.as_bytes()).unwrap();
    take_updates(&mut runtime);
    let selector = crate::FeatureStateSelector::new("geo").with_feature_id("point");
    map.set_feature_state(&selector, br#"{"selected":true}"#)
        .unwrap();
    assert!(take_updates(&mut runtime) > 0);
    map.set_feature_state(&selector, br#"{"selected":true}"#)
        .unwrap();
    map.set_feature_state(&selector, b"{}").unwrap();
    assert_eq!(take_updates(&mut runtime), 0);
    map.remove_feature_state(&selector).unwrap();
    assert!(take_updates(&mut runtime) > 0);
    map.remove_feature_state(&selector).unwrap();
    assert_eq!(take_updates(&mut runtime), 0);
    map.close().unwrap();
    runtime.close().unwrap();
}
