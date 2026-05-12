use maplibre_native::{
    CameraOptions, EdgeInsets, Feature, FeatureIdentifier, GeoJson, Geometry, JsonMember,
    JsonValue, LatLng, MapMode, MapOptions, MetalBorrowedTextureDescriptor,
    MetalOwnedTextureDescriptor, MetalSurfaceDescriptor, NativePointer, NetworkStatus,
    OwnedTextureDescriptor, RenderBackendMask, RuntimeHandle, RuntimeOptions, ScreenPoint,
    VulkanBorrowedTextureDescriptor, VulkanOwnedTextureDescriptor, VulkanSurfaceDescriptor,
    c_version, network_status, set_network_status, supported_render_backends,
};

struct NetworkStatusRestore(NetworkStatus);

impl Drop for NetworkStatusRestore {
    fn drop(&mut self) {
        let _ = set_network_status(self.0);
    }
}

#[test]
fn public_api_uses_safe_rust_types() {
    assert_eq!(c_version(), 0);

    let backends: RenderBackendMask = supported_render_backends();
    assert!(backends.intersects(RenderBackendMask::METAL | RenderBackendMask::VULKAN));

    let original: NetworkStatus = network_status().unwrap();
    let _restore = NetworkStatusRestore(original);

    set_network_status(NetworkStatus::Offline).unwrap();
    assert_eq!(network_status().unwrap(), NetworkStatus::Offline);
}

#[test]
fn public_handles_create_pump_drain_and_close() {
    let runtime = RuntimeHandle::with_options(
        &RuntimeOptions::new()
            .with_asset_path("")
            .with_cache_path("")
            .with_maximum_cache_size(0),
    )
    .unwrap();
    runtime.run_once().unwrap();
    let _ = runtime.discard_one_event().unwrap();
    runtime.drain_events().unwrap();

    let map_options = MapOptions::new(320, 240, 1.0).with_mode(MapMode::Continuous);
    let map = runtime.create_map_with_options(&map_options).unwrap();
    drop(runtime);

    map.close().unwrap();
}

#[test]
fn public_descriptors_use_owned_rust_values() {
    let camera = CameraOptions::new()
        .with_center(LatLng::new(45.0, -122.0))
        .with_zoom(10.0)
        .with_padding(EdgeInsets::new(1.0, 2.0, 3.0, 4.0))
        .with_anchor(ScreenPoint::new(128.0, 64.0));

    assert_eq!(camera.center, Some(LatLng::new(45.0, -122.0)));
    assert_eq!(camera.zoom, Some(10.0));
    assert_eq!(camera.padding.unwrap().right, 4.0);

    let json = JsonValue::Object(vec![
        JsonMember::new("id", JsonValue::UInt(u64::MAX)),
        JsonMember::new("id", JsonValue::String("duplicate key is preserved".into())),
        JsonMember::new("nested", JsonValue::Array(vec![JsonValue::Bool(true)])),
    ]);
    assert_eq!(
        json,
        JsonValue::Object(vec![
            JsonMember::new("id", JsonValue::UInt(u64::MAX)),
            JsonMember::new("id", JsonValue::String("duplicate key is preserved".into())),
            JsonMember::new("nested", JsonValue::Array(vec![JsonValue::Bool(true)])),
        ])
    );

    let feature = Feature::new(
        Geometry::Point(LatLng::new(1.0, 2.0)),
        vec![JsonMember::new("name", JsonValue::String("park".into()))],
    )
    .with_identifier(FeatureIdentifier::String("feature-1".into()));
    let geojson = GeoJson::Feature(feature);
    assert!(matches!(geojson, GeoJson::Feature(_)));

    let texture = OwnedTextureDescriptor::new(128, 64, 2.0);
    assert_eq!(texture.width, 128);
    assert_eq!(texture.height, 64);
    // SAFETY: The test uses dummy opaque addresses and does not dereference them.
    let pointer = unsafe { NativePointer::from_address(0x1234) };
    assert_eq!(pointer.address(), 0x1234);
    let _ = MetalSurfaceDescriptor::new(128, 64, 2.0, pointer).with_device(pointer);
    let _ =
        VulkanSurfaceDescriptor::new(128, 64, 2.0, pointer, pointer, pointer, pointer, 0, pointer);
    let _ = MetalOwnedTextureDescriptor::new(128, 64, 2.0, pointer);
    let _ = MetalBorrowedTextureDescriptor::new(128, 64, 2.0, pointer);
    let _ = VulkanOwnedTextureDescriptor::new(128, 64, 2.0, pointer, pointer, pointer, pointer, 0);
    let _ = VulkanBorrowedTextureDescriptor::new(
        128, 64, 2.0, pointer, pointer, pointer, pointer, 0, pointer, pointer, 44, 55, 66,
    );
}
