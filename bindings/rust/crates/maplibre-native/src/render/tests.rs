use std::mem;
use std::time::Duration;

use static_assertions::assert_not_impl_any;

use super::*;
use crate::{
    CameraOptions, ErrorKind, JsonMember, LatLng, MapMode, MapOptions, RuntimeEventType,
    RuntimeHandle, ScreenBox, ScreenPoint,
};

assert_not_impl_any!(NativePointer: Send, Sync);
assert_not_impl_any!(FrameNativePointer<'static>: Send, Sync);
assert_not_impl_any!(RenderSessionHandle: Send, Sync);
assert_not_impl_any!(MetalOwnedTextureFrameHandle: Send, Sync);
assert_not_impl_any!(VulkanOwnedTextureFrameHandle: Send, Sync);

const FEATURE_STATE_STYLE_JSON: &str = r#"{"version":8,"sources":{"point":{"type":"geojson","data":{"type":"FeatureCollection","features":[{"type":"Feature","id":"feature-1","properties":{},"geometry":{"type":"Point","coordinates":[0,0]}}]}}},"layers":[{"id":"circle","type":"circle","source":"point","paint":{"circle-radius":["case",["boolean",["feature-state","hover"],false],10,5]}}]}"#;
const QUERY_STYLE_JSON: &str = r##"{"version":8,"sources":{"point":{"type":"geojson","data":{"type":"FeatureCollection","features":[{"type":"Feature","id":"feature-1","geometry":{"type":"Point","coordinates":[-122.4194,37.7749]},"properties":{"kind":"capital","visible":true}}]}}},"layers":[{"id":"background","type":"background","paint":{"background-color":"#d8f1ff"}},{"id":"point-circle","type":"circle","source":"point","paint":{"circle-color":"#f97316","circle-radius":12}}]}"##;
const CLUSTER_STYLE_JSON: &str = r##"{"version":8,"sources":{"cluster-source":{"type":"geojson","cluster":true,"data":{"type":"FeatureCollection","features":[{"type":"Feature","geometry":{"type":"Point","coordinates":[0.0,0.0]},"properties":{"name":"one"}},{"type":"Feature","geometry":{"type":"Point","coordinates":[0.001,0.001]},"properties":{"name":"two"}},{"type":"Feature","geometry":{"type":"Point","coordinates":[0.002,0.002]},"properties":{"name":"three"}}]}}},"layers":[{"id":"background","type":"background","paint":{"background-color":"#ffffff"}},{"id":"cluster-circle","type":"circle","source":"cluster-source","filter":["has","point_count"],"paint":{"circle-color":"#2563eb","circle-radius":20}}]}"##;

fn wait_for_runtime_event(runtime: &RuntimeHandle, event_type: RuntimeEventType) -> bool {
    for _ in 0..100 {
        let _ = runtime.run_once();
        while let Ok(Some(event)) = runtime.poll_event() {
            if event.event_type == event_type {
                return true;
            }
        }
        std::thread::sleep(Duration::from_millis(10));
    }
    false
}

fn load_feature_state_style(
    runtime: &RuntimeHandle,
    map: &MapHandle,
    session: &RenderSessionHandle,
) {
    map.set_style_json(FEATURE_STATE_STYLE_JSON).unwrap();
    assert!(wait_for_runtime_event(
        runtime,
        RuntimeEventType::MapRenderUpdateAvailable
    ));
    session.render_update().unwrap();
}

fn load_query_style(runtime: &RuntimeHandle, map: &MapHandle, session: &RenderSessionHandle) {
    map.jump_to(
        &CameraOptions::new()
            .with_center(LatLng::new(37.7749, -122.4194))
            .with_zoom(10.0),
    )
    .unwrap();
    map.set_style_json(QUERY_STYLE_JSON).unwrap();
    render_available_updates(runtime, session, 5);
}

fn load_cluster_style(runtime: &RuntimeHandle, map: &MapHandle, session: &RenderSessionHandle) {
    map.jump_to(
        &CameraOptions::new()
            .with_center(LatLng::new(0.0, 0.0))
            .with_zoom(0.0),
    )
    .unwrap();
    map.set_style_json(CLUSTER_STYLE_JSON).unwrap();
    render_available_updates(runtime, session, 5);
}

fn render_available_updates(runtime: &RuntimeHandle, session: &RenderSessionHandle, count: usize) {
    for _ in 0..count {
        if wait_for_runtime_event(runtime, RuntimeEventType::MapRenderUpdateAvailable) {
            let _ = session.render_update();
        }
    }
}

fn feature_member<'a>(feature: &'a Feature, key: &str) -> Option<&'a JsonValue> {
    feature
        .properties
        .iter()
        .find(|member| member.key == key)
        .map(|member| &member.value)
}

fn json_member<'a>(value: &'a JsonValue, key: &str) -> Option<&'a JsonValue> {
    let JsonValue::Object(members) = value else {
        return None;
    };
    members
        .iter()
        .find(|member| member.key == key)
        .map(|member| &member.value)
}

fn assert_json_member(value: &JsonValue, key: &str, expected: &JsonValue) {
    assert_eq!(json_member(value, key), Some(expected));
}

#[test]
fn native_pointer_round_trips_address() {
    // SAFETY: Test uses a dummy opaque address and does not dereference it.
    let pointer = unsafe { NativePointer::from_address(0x1234) };
    assert_eq!(pointer.address(), 0x1234);
    // SAFETY: Test only verifies address reconstruction; it does not dereference.
    assert_eq!(unsafe { pointer.as_ptr::<u8>() } as usize, 0x1234);
    assert!(NativePointer::NULL.is_null());
}

#[test]
fn frame_native_pointer_round_trips_address_without_plain_native_pointer() {
    // SAFETY: Test uses a dummy opaque address and does not dereference it.
    let pointer = unsafe { FrameNativePointer::<'_>::from_ptr(0x4321usize as *mut u8) };
    // SAFETY: Test only verifies address reconstruction while the typed frame borrow is live.
    assert_eq!(unsafe { pointer.address() }, 0x4321);
    // SAFETY: Test only verifies raw pointer reconstruction; it does not dereference.
    assert_eq!(unsafe { pointer.as_ptr::<u8>() } as usize, 0x4321);
    assert!(!pointer.is_null());
}

#[test]
fn frame_metadata_copies_values_without_exposing_backend_pointers() {
    let mut metal = empty_metal_owned_texture_frame();
    metal.generation = 1;
    metal.width = 64;
    metal.height = 32;
    metal.scale_factor = 2.0;
    metal.frame_id = 9;
    metal.texture = 0x1000usize as *mut _;
    metal.device = 0x2000usize as *mut _;
    metal.pixel_format = 80;
    let copied = MetalOwnedTextureFrame::from_native(&metal);
    assert_eq!(copied.generation, 1);
    assert_eq!(
        (copied.width, copied.height, copied.scale_factor),
        (64, 32, 2.0)
    );
    assert_eq!(copied.frame_id, 9);
    assert_eq!(copied.pixel_format, 80);

    let mut vulkan = empty_vulkan_owned_texture_frame();
    vulkan.generation = 3;
    vulkan.width = 128;
    vulkan.height = 96;
    vulkan.scale_factor = 1.5;
    vulkan.frame_id = 11;
    vulkan.image = 0x3000usize as *mut _;
    vulkan.image_view = 0x4000usize as *mut _;
    vulkan.device = 0x5000usize as *mut _;
    vulkan.format = 44;
    vulkan.layout = 55;
    let copied = VulkanOwnedTextureFrame::from_native(&vulkan);
    assert_eq!(copied.generation, 3);
    assert_eq!(
        (copied.width, copied.height, copied.scale_factor),
        (128, 96, 1.5)
    );
    assert_eq!(copied.frame_id, 11);
    assert_eq!((copied.format, copied.layout), (44, 55));
}

#[test]
fn descriptor_materialization_fills_sizes_and_pointers() {
    // SAFETY: Test uses dummy opaque addresses and does not dereference them.
    let p1 = unsafe { NativePointer::from_address(0x10) };
    // SAFETY: Test uses dummy opaque addresses and does not dereference them.
    let p2 = unsafe { NativePointer::from_address(0x20) };
    // SAFETY: Test uses dummy opaque addresses and does not dereference them.
    let p3 = unsafe { NativePointer::from_address(0x30) };
    // SAFETY: Test uses dummy opaque addresses and does not dereference them.
    let p4 = unsafe { NativePointer::from_address(0x40) };
    // SAFETY: Test uses dummy opaque addresses and does not dereference them.
    let p5 = unsafe { NativePointer::from_address(0x50) };

    let owned = OwnedTextureDescriptor::new(32, 16, 2.0).to_native();
    assert_eq!(owned.size as usize, mem::size_of_val(&owned));
    assert_eq!(
        (owned.width, owned.height, owned.scale_factor),
        (32, 16, 2.0)
    );

    let metal_surface = MetalSurfaceDescriptor::new(1, 2, 3.0, p1)
        .with_device(p2)
        .to_native();
    assert_eq!(
        metal_surface.size as usize,
        mem::size_of_val(&metal_surface)
    );
    assert_eq!(metal_surface.layer as usize, 0x10);
    assert_eq!(metal_surface.device as usize, 0x20);

    let vulkan_surface = VulkanSurfaceDescriptor::new(1, 2, 3.0, p1, p2, p3, p4, 7, p5).to_native();
    assert_eq!(
        vulkan_surface.size as usize,
        mem::size_of_val(&vulkan_surface)
    );
    assert_eq!(vulkan_surface.instance as usize, 0x10);
    assert_eq!(vulkan_surface.physical_device as usize, 0x20);
    assert_eq!(vulkan_surface.device as usize, 0x30);
    assert_eq!(vulkan_surface.graphics_queue as usize, 0x40);
    assert_eq!(vulkan_surface.graphics_queue_family_index, 7);
    assert_eq!(vulkan_surface.surface as usize, 0x50);

    let metal_owned = MetalOwnedTextureDescriptor::new(1, 2, 3.0, p1).to_native();
    assert_eq!(metal_owned.size as usize, mem::size_of_val(&metal_owned));
    assert_eq!(metal_owned.device as usize, 0x10);

    let metal_borrowed = MetalBorrowedTextureDescriptor::new(1, 2, 3.0, p1).to_native();
    assert_eq!(
        metal_borrowed.size as usize,
        mem::size_of_val(&metal_borrowed)
    );
    assert_eq!(metal_borrowed.texture as usize, 0x10);

    let vulkan_owned = VulkanOwnedTextureDescriptor::new(1, 2, 3.0, p1, p2, p3, p4, 9).to_native();
    assert_eq!(vulkan_owned.size as usize, mem::size_of_val(&vulkan_owned));
    assert_eq!(vulkan_owned.instance as usize, 0x10);
    assert_eq!(vulkan_owned.graphics_queue_family_index, 9);

    let vulkan_borrowed =
        VulkanBorrowedTextureDescriptor::new(1, 2, 3.0, p1, p2, p3, p4, 11, p5, p1, 44, 55, 66)
            .to_native();
    assert_eq!(
        vulkan_borrowed.size as usize,
        mem::size_of_val(&vulkan_borrowed)
    );
    assert_eq!(vulkan_borrowed.image as usize, 0x50);
    assert_eq!(vulkan_borrowed.image_view as usize, 0x10);
    assert_eq!(vulkan_borrowed.format, 44);
    assert_eq!(vulkan_borrowed.initial_layout, 55);
    assert_eq!(vulkan_borrowed.final_layout, 66);
}

#[test]
fn query_descriptor_materialization_sets_fields_and_views() {
    let point = RenderedQueryGeometry::point(ScreenPoint::new(1.0, 2.0)).to_native();
    let raw = point.as_ref();
    assert_eq!(raw.size as usize, mem::size_of_val(raw));
    assert_eq!(raw.type_, sys::MLN_RENDERED_QUERY_GEOMETRY_TYPE_POINT);
    // SAFETY: Active union member is selected by type_.
    let raw_point = unsafe { raw.data.point };
    assert_eq!((raw_point.x, raw_point.y), (1.0, 2.0));

    let box_geometry = RenderedQueryGeometry::box_(ScreenBox::new(
        ScreenPoint::new(1.0, 2.0),
        ScreenPoint::new(3.0, 4.0),
    ))
    .to_native();
    let raw = box_geometry.as_ref();
    assert_eq!(raw.type_, sys::MLN_RENDERED_QUERY_GEOMETRY_TYPE_BOX);
    // SAFETY: Active union member is selected by type_.
    let raw_box = unsafe { raw.data.box_ };
    assert_eq!((raw_box.min.x, raw_box.min.y), (1.0, 2.0));
    assert_eq!((raw_box.max.x, raw_box.max.y), (3.0, 4.0));

    let line = RenderedQueryGeometry::line_string(vec![
        ScreenPoint::new(1.0, 2.0),
        ScreenPoint::new(3.0, 4.0),
    ])
    .to_native();
    let raw = line.as_ref();
    assert_eq!(raw.type_, sys::MLN_RENDERED_QUERY_GEOMETRY_TYPE_LINE_STRING);
    // SAFETY: Active union member is selected by type_.
    let line_string = unsafe { raw.data.line_string };
    assert_eq!(line_string.point_count, 2);
    assert!(!line_string.points.is_null());

    let filter = JsonValue::Array(vec![
        JsonValue::String("has".into()),
        JsonValue::String("kind".into()),
    ]);
    let rendered_options = RenderedFeatureQueryOptions::new()
        .with_layer_ids(vec!["point-circle".into()])
        .with_filter(filter.clone());
    let rendered = rendered_options.to_native().unwrap();
    let raw = rendered.as_ref();
    assert_eq!(raw.size as usize, mem::size_of_val(raw));
    assert_eq!(raw.fields, sys::MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS);
    assert_eq!(raw.layer_id_count, 1);
    assert!(!raw.layer_ids.is_null());
    assert!(!raw.filter.is_null());
    // SAFETY: The materializer keeps the string view valid for this scope.
    assert_eq!(
        unsafe { support::string::copy_string_view(*raw.layer_ids) }.unwrap(),
        "point-circle"
    );

    let source_options = SourceFeatureQueryOptions::new()
        .with_source_layer_ids(vec!["landuse".into()])
        .with_filter(filter);
    let source = source_options.to_native().unwrap();
    let raw = source.as_ref();
    assert_eq!(raw.size as usize, mem::size_of_val(raw));
    assert_eq!(
        raw.fields,
        sys::MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS
    );
    assert_eq!(raw.source_layer_id_count, 1);
    assert!(!raw.source_layer_ids.is_null());
    assert!(!raw.filter.is_null());
    // SAFETY: The materializer keeps the string view valid for this scope.
    assert_eq!(
        unsafe { support::string::copy_string_view(*raw.source_layer_ids) }.unwrap(),
        "landuse"
    );
}

#[test]
fn feature_state_selector_materialization_sets_fields_and_views() {
    let selector = FeatureStateSelector::new("point")
        .with_source_layer_id("layer")
        .with_feature_id("feature-1")
        .with_state_key("hover")
        .unwrap();
    let native = selector.to_native();
    let raw = native.as_ref();

    assert_eq!(raw.size as usize, mem::size_of_val(raw));
    assert_eq!(
        raw.fields,
        sys::MLN_FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID
            | sys::MLN_FEATURE_STATE_SELECTOR_FEATURE_ID
            | sys::MLN_FEATURE_STATE_SELECTOR_STATE_KEY
    );
    // SAFETY: The materializer keeps these views valid for this scope.
    assert_eq!(
        unsafe { support::string::copy_string_view(raw.source_id) }.unwrap(),
        "point"
    );
    assert_eq!(
        unsafe { support::string::copy_string_view(raw.source_layer_id) }.unwrap(),
        "layer"
    );
    assert_eq!(
        unsafe { support::string::copy_string_view(raw.feature_id) }.unwrap(),
        "feature-1"
    );
    assert_eq!(
        unsafe { support::string::copy_string_view(raw.state_key) }.unwrap(),
        "hover"
    );

    let source_only_selector = FeatureStateSelector::new("point");
    let source_only = source_only_selector.to_native();
    let raw = source_only.as_ref();
    assert_eq!(raw.fields, 0);
    assert_eq!(raw.source_layer_id.size, 0);
    assert!(raw.source_layer_id.data.is_null());
    assert_eq!(raw.feature_id.size, 0);
    assert!(raw.feature_id.data.is_null());
    assert_eq!(raw.state_key.size, 0);
    assert!(raw.state_key.data.is_null());

    let error = FeatureStateSelector::new("point")
        .with_state_key("hover")
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
}

#[test]
fn feature_state_set_get_and_remove_copy_snapshots() {
    let runtime = RuntimeHandle::new().unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::new(64, 64, 1.0)).unwrap();
    let session = map
        .attach_owned_texture(&OwnedTextureDescriptor::new(64, 64, 1.0))
        .unwrap();
    let selector = FeatureStateSelector::new("point").with_feature_id("feature-1");
    let state = JsonValue::Object(vec![
        JsonMember::new("hover", JsonValue::Bool(true)),
        JsonMember::new("radius", JsonValue::UInt(20)),
    ]);

    let error = session.set_feature_state(&selector, &state).unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidState);

    load_feature_state_style(&runtime, &map, &session);

    session.set_feature_state(&selector, &state).unwrap();
    let copied = session.get_feature_state(&selector).unwrap();
    assert_json_member(&copied, "hover", &JsonValue::Bool(true));
    assert_json_member(&copied, "radius", &JsonValue::UInt(20));

    let hover_selector = FeatureStateSelector::new("point")
        .with_feature_id("feature-1")
        .with_state_key("hover")
        .unwrap();
    session.remove_feature_state(&hover_selector).unwrap();
    let _ = wait_for_runtime_event(&runtime, RuntimeEventType::MapRenderUpdateAvailable);
    let _ = session.render_update();

    let after_remove = session.get_feature_state(&selector).unwrap();
    assert_json_member(&after_remove, "radius", &JsonValue::UInt(20));
    assert!(json_member(&after_remove, "hover").is_none());

    session.close().unwrap();
    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
fn rendered_and_source_queries_copy_results() {
    let runtime = RuntimeHandle::new().unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::new(64, 64, 1.0)).unwrap();
    let session = map
        .attach_owned_texture(&OwnedTextureDescriptor::new(64, 64, 1.0))
        .unwrap();

    let error = session
        .query_rendered_features(
            &RenderedQueryGeometry::point(ScreenPoint::new(32.0, 32.0)),
            None,
        )
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidState);

    load_query_style(&runtime, &map, &session);
    let query_point = map
        .pixel_for_lat_lng(LatLng::new(37.7749, -122.4194))
        .unwrap();
    let geometry = RenderedQueryGeometry::box_(ScreenBox::new(
        ScreenPoint::new(query_point.x - 20.0, query_point.y - 20.0),
        ScreenPoint::new(query_point.x + 20.0, query_point.y + 20.0),
    ));
    let filter = JsonValue::Array(vec![
        JsonValue::String("==".into()),
        JsonValue::Array(vec![
            JsonValue::String("get".into()),
            JsonValue::String("kind".into()),
        ]),
        JsonValue::String("capital".into()),
    ]);
    let rendered_options = RenderedFeatureQueryOptions::new()
        .with_layer_ids(vec!["point-circle".into()])
        .with_filter(filter.clone());
    let rendered = session
        .query_rendered_features(&geometry, Some(&rendered_options))
        .unwrap();
    assert_eq!(rendered.len(), 1);
    assert_eq!(rendered[0].source_id.as_deref(), Some("point"));
    assert_eq!(
        feature_member(&rendered[0].feature, "kind"),
        Some(&JsonValue::String("capital".into()))
    );

    let source_options = SourceFeatureQueryOptions::new().with_filter(filter);
    let source = session
        .query_source_features("point", Some(&source_options))
        .unwrap();
    assert_eq!(source.len(), 1);
    assert_eq!(source[0].source_id.as_deref(), Some("point"));
    assert_eq!(
        feature_member(&source[0].feature, "kind"),
        Some(&JsonValue::String("capital".into()))
    );

    session.close().unwrap();
    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
fn feature_extension_queries_copy_value_and_feature_collection_results() {
    let runtime = RuntimeHandle::new().unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::new(64, 64, 1.0)).unwrap();
    let session = map
        .attach_owned_texture(&OwnedTextureDescriptor::new(64, 64, 1.0))
        .unwrap();

    load_cluster_style(&runtime, &map, &session);
    let query_point = map.pixel_for_lat_lng(LatLng::new(0.0, 0.0)).unwrap();
    let geometry = RenderedQueryGeometry::box_(ScreenBox::new(
        ScreenPoint::new(query_point.x - 30.0, query_point.y - 30.0),
        ScreenPoint::new(query_point.x + 30.0, query_point.y + 30.0),
    ));
    let options = RenderedFeatureQueryOptions::new().with_layer_ids(vec!["cluster-circle".into()]);
    let cluster = (0..100)
        .find_map(|_| {
            let clusters = session
                .query_rendered_features(&geometry, Some(&options))
                .ok()?;
            if clusters.len() == 1 {
                clusters.into_iter().next()
            } else {
                render_available_updates(&runtime, &session, 1);
                std::thread::sleep(Duration::from_millis(1));
                None
            }
        })
        .expect("timed out waiting for rendered cluster");

    let children = session
        .query_feature_extension(
            "cluster-source",
            &cluster.feature,
            "supercluster",
            "children",
            None,
        )
        .unwrap();
    let FeatureExtensionResult::FeatureCollection(children) = children else {
        panic!("expected children feature collection");
    };
    assert!(!children.is_empty());

    let expansion_zoom = session
        .query_feature_extension(
            "cluster-source",
            &cluster.feature,
            "supercluster",
            "expansion-zoom",
            None,
        )
        .unwrap();
    assert!(matches!(
        expansion_zoom,
        FeatureExtensionResult::Value(JsonValue::UInt(_))
    ));

    session.close().unwrap();
    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
fn owned_texture_session_retains_parent_and_enforces_single_session() {
    let runtime = RuntimeHandle::new().unwrap();
    let map = MapHandle::with_options(
        &runtime,
        &MapOptions::new(64, 64, 1.0).with_mode(MapMode::Static),
    )
    .unwrap();
    let session = map
        .attach_owned_texture(&OwnedTextureDescriptor::new(32, 16, 1.0))
        .unwrap();

    let error = map
        .attach_owned_texture(&OwnedTextureDescriptor::new(32, 16, 1.0))
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidState);

    let error = map.close().unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidState);
    assert!(error.diagnostic().contains("child handles are live"));
    let map = error.into_handle();

    drop(runtime);

    let detached = session.detach().unwrap();
    detached.close().unwrap();
    map.close().unwrap();
}

#[test]
fn acquired_frame_state_rejects_reentrant_session_operations_before_native_calls() {
    let runtime = RuntimeHandle::new().unwrap();
    let map = MapHandle::with_options(
        &runtime,
        &MapOptions::new(64, 64, 1.0).with_mode(MapMode::Static),
    )
    .unwrap();
    let session = map
        .attach_owned_texture(&OwnedTextureDescriptor::new(32, 16, 1.0))
        .unwrap();

    session.inner.frame_acquired.set(true);

    let selector = FeatureStateSelector::new("point").with_feature_id("feature-1");
    let detach_error = session.detach().unwrap_err();
    assert_eq!(detach_error.kind(), ErrorKind::InvalidState);
    assert!(detach_error.diagnostic().contains("acquired texture frame"));
    let session = detach_error.into_handle();

    for error in [
        session.resize(32, 16, 1.0).unwrap_err(),
        session.render_update().unwrap_err(),
        session
            .set_feature_state(&selector, &JsonValue::Object(Vec::new()))
            .unwrap_err(),
        session.get_feature_state(&selector).unwrap_err(),
        session.remove_feature_state(&selector).unwrap_err(),
        session
            .query_rendered_features(
                &RenderedQueryGeometry::point(ScreenPoint::new(0.0, 0.0)),
                None,
            )
            .unwrap_err(),
        session.query_source_features("point", None).unwrap_err(),
        session
            .query_feature_extension(
                "point",
                &Feature::new(crate::Geometry::Empty, Vec::new()),
                "x",
                "y",
                None,
            )
            .unwrap_err(),
        session.read_premultiplied_rgba8_into(&mut []).unwrap_err(),
        session.acquire_metal_owned_texture_frame().unwrap_err(),
        session.acquire_vulkan_owned_texture_frame().unwrap_err(),
    ] {
        assert_eq!(error.kind(), ErrorKind::InvalidState);
        assert!(error.diagnostic().contains("acquired texture frame"));
    }

    let error = session.close().unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidState);
    assert!(error.diagnostic().contains("acquired texture frame"));
    let session = error.into_handle();

    session.inner.frame_acquired.set(false);
    session.close().unwrap();
}

#[test]
fn texture_readback_reports_documented_error_kinds_for_unsized_buffer() {
    let runtime = RuntimeHandle::new().unwrap();
    let map = MapHandle::with_options(
        &runtime,
        &MapOptions::new(64, 64, 1.0).with_mode(MapMode::Static),
    )
    .unwrap();
    let session = map
        .attach_owned_texture(&OwnedTextureDescriptor::new(32, 16, 1.0))
        .unwrap();

    let _ = session.render_update();
    let mut empty = [];
    let error = session
        .read_premultiplied_rgba8_into(&mut empty)
        .unwrap_err();
    assert!(matches!(
        error.kind(),
        ErrorKind::InvalidArgument | ErrorKind::InvalidState | ErrorKind::Unsupported
    ));

    session.close().unwrap();
}

#[test]
fn backend_specific_attach_calls_report_native_statuses() {
    let runtime = RuntimeHandle::new().unwrap();
    let map = MapHandle::with_options(
        &runtime,
        &MapOptions::new(64, 64, 1.0).with_mode(MapMode::Static),
    )
    .unwrap();

    let metal_error = map
        .attach_metal_owned_texture(&MetalOwnedTextureDescriptor::new(
            32,
            16,
            1.0,
            NativePointer::NULL,
        ))
        .unwrap_err();
    assert!(matches!(
        metal_error.kind(),
        ErrorKind::InvalidArgument | ErrorKind::Unsupported
    ));

    let vulkan_error = map
        .attach_vulkan_surface(&VulkanSurfaceDescriptor::new(
            32,
            16,
            1.0,
            NativePointer::NULL,
            NativePointer::NULL,
            NativePointer::NULL,
            NativePointer::NULL,
            0,
            NativePointer::NULL,
        ))
        .unwrap_err();
    assert!(matches!(
        vulkan_error.kind(),
        ErrorKind::InvalidArgument | ErrorKind::Unsupported
    ));

    map.close().unwrap();
    runtime.close().unwrap();
}
