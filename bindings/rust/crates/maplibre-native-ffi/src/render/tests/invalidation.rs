use super::*;

// One drain schedules at most one frame of the latest state. The idle event
// closes the feedback loop after resource loading and transitions finish.
fn render_to_idle(
    runtime: &mut RuntimeHandle,
    session: &RenderSessionHandle,
) -> crate::RenderingStats {
    let deadline = Instant::now() + Duration::from_secs(10);
    let mut stats = None;
    while Instant::now() < deadline {
        runtime.pump(Some(Duration::from_millis(10)), None).unwrap();
        let batch = runtime.drain_events(0).unwrap();
        let mut dirty = false;
        let mut idle = false;
        for event in batch.iter() {
            match event.event_type() {
                RuntimeEventType::MapRenderUpdateAvailable => {
                    dirty = true;
                    idle = false;
                }
                RuntimeEventType::MapIdle => idle = true,
                RuntimeEventType::MapRenderFrameFinished => {
                    if let RuntimeEventPayload::RenderFrame(frame) = event.payload() {
                        stats = Some(frame.stats);
                    }
                }
                _ => {}
            }
        }
        if dirty {
            session.render_update().unwrap();
        } else if idle {
            return stats.expect("idle rendering should report frame statistics");
        }
    }
    panic!("event-driven rendering did not reach idle");
}

#[test]
#[cfg(mln_webgpu_backend)]
fn webgpu_stats_track_draws_across_frames() {
    let mut runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::new(64, 64, 1.0)).unwrap();
    let (_context, session) = create_owned_texture_session(
        &map.attach_ref().unwrap(),
        RenderTargetExtent::new(64, 64, 1.0),
    )
    .unwrap();
    map.set_style_json(FEATURE_STATE_STYLE_JSON.as_bytes())
        .unwrap();
    let first = render_to_idle(&mut runtime, &session);
    assert!(first.frame_count > 0);
    assert!(first.draw_call_count > 0);
    assert!(first.total_draw_call_count >= first.draw_call_count);

    map.request_repaint().unwrap();
    let second = render_to_idle(&mut runtime, &session);
    assert!(second.draw_call_count > 0);
    assert_eq!(second.frame_count, first.frame_count + 1);
    assert_eq!(
        second.total_draw_call_count,
        first.total_draw_call_count + second.draw_call_count
    );

    map.set_layer_property("circle", "visibility", br#""none""#)
        .unwrap();
    let empty = render_to_idle(&mut runtime, &session);
    // Source fading can request several empty frames after the layer is hidden.
    assert!(empty.frame_count > second.frame_count);
    assert_eq!(empty.draw_call_count, 0);
    assert_eq!(empty.total_draw_call_count, second.total_draw_call_count);
    session.close().unwrap();
    map.close().unwrap();
    runtime.close().unwrap();
}

fn center_pixel(session: &RenderSessionHandle) -> [u8; 4] {
    let info = session.texture_image_info().unwrap();
    let mut pixels = vec![0; info.byte_length];
    session.read_premultiplied_rgba8_into(&mut pixels).unwrap();
    let offset = (info.height as usize / 2) * info.stride as usize + (info.width as usize / 2) * 4;
    pixels[offset..offset + 4].try_into().unwrap()
}

#[test]
fn layer_transition_change_reaches_the_pending_render_snapshot() {
    let mut runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::new(64, 64, 1.0)).unwrap();
    let (_context, session) = create_owned_texture_session(
        &map.attach_ref().unwrap(),
        RenderTargetExtent::new(64, 64, 1.0),
    )
    .unwrap();
    map.set_style_json(br##"{"version":8,"sources":{},"transition":{"duration":0},"layers":[{"id":"background","type":"background","paint":{"background-color":"#ff0000"}}]}"##).unwrap();
    render_to_idle(&mut runtime, &session);
    assert_eq!(center_pixel(&session), [255, 0, 0, 255]);

    map.set_layer_property(
        "background",
        "background-color-transition",
        br#"{"duration":0,"delay":60000}"#,
    )
    .unwrap();
    map.set_layer_property("background", "background-color", br##""#0000ff""##)
        .unwrap();
    // Clear the delay after the value write, before rendering either change.
    // A repaint of the cached layer implementation would still carry the delay.
    map.set_layer_property(
        "background",
        "background-color-transition",
        br#"{"duration":0,"delay":0}"#,
    )
    .unwrap();
    render_to_idle(&mut runtime, &session);
    assert_eq!(center_pixel(&session), [0, 0, 255, 255]);
    session.close().unwrap();
    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
fn clearing_renderer_data_restores_feature_state_from_render_events() {
    let mut runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::new(64, 64, 1.0)).unwrap();
    let (_context, session) = create_owned_texture_session(
        &map.attach_ref().unwrap(),
        RenderTargetExtent::new(64, 64, 1.0),
    )
    .unwrap();
    map.set_style_json(br##"{"version":8,"transition":{"duration":0},"sources":{"point":{"type":"geojson","data":{"type":"Feature","id":1,"properties":{},"geometry":{"type":"Point","coordinates":[0,0]}}}},"layers":[{"id":"point","type":"circle","source":"point","paint":{"circle-radius":20,"circle-color":["case",["boolean",["feature-state","selected"],false],"#00ff00","#ff0000"]}}]}"##).unwrap();
    render_to_idle(&mut runtime, &session);
    assert_eq!(center_pixel(&session), [255, 0, 0, 255]);
    let selector = FeatureStateSelector::new("point").with_feature_id("1");
    map.set_feature_state(&selector, br#"{"selected":true}"#)
        .unwrap();
    render_to_idle(&mut runtime, &session);
    assert_eq!(center_pixel(&session), [0, 255, 0, 255]);
    session.clear_data().unwrap();
    render_to_idle(&mut runtime, &session);
    assert_eq!(center_pixel(&session), [0, 255, 0, 255]);
    session.close().unwrap();
    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
fn a_source_added_after_its_layer_renders_without_a_manual_repaint() {
    let mut runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::new(64, 64, 1.0)).unwrap();
    let (_context, session) = create_owned_texture_session(
        &map.attach_ref().unwrap(),
        RenderTargetExtent::new(64, 64, 1.0),
    )
    .unwrap();
    map.set_style_json(br##"{"version":8,"transition":{"duration":0},"sources":{},"layers":[{"id":"point","type":"circle","source":"late","paint":{"circle-radius":20,"circle-color":"#00ff00"}}]}"##).unwrap();
    render_to_idle(&mut runtime, &session);
    assert_eq!(center_pixel(&session), [0, 0, 0, 0]);
    let data = crate::GeoJsonSourceDataHandle::new(
        br#"{"type":"Feature","properties":{},"geometry":{"type":"Point","coordinates":[0,0]}}"#,
        None,
    )
    .unwrap();
    map.add_geojson_source_data("late", &data).unwrap();
    render_to_idle(&mut runtime, &session);
    assert_eq!(center_pixel(&session), [0, 255, 0, 255]);
    data.close();
    session.close().unwrap();
    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
fn removing_a_style_image_updates_pattern_pixels_from_render_events() {
    let mut runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::new(64, 64, 1.0)).unwrap();
    let (_context, session) = create_owned_texture_session(
        &map.attach_ref().unwrap(),
        RenderTargetExtent::new(64, 64, 1.0),
    )
    .unwrap();
    // Shared WebGL contexts preserve the color buffer. An opaque background
    // makes removal visible without relying on a framebuffer clear.
    map.set_style_json(br##"{"version":8,"transition":{"duration":0},"sources":{},"layers":[{"id":"base","type":"background","paint":{"background-color":"#0000ff"}},{"id":"pattern","type":"background","paint":{"background-pattern":"swatch"}}]}"##).unwrap();
    let image = crate::PremultipliedRgba8Image::new(
        crate::TextureImageInfo::new(2, 2, 8, 16),
        [255, 0, 0, 255].repeat(4),
    );
    map.set_style_image("swatch", &image, None).unwrap();
    render_to_idle(&mut runtime, &session);
    assert_eq!(center_pixel(&session), [255, 0, 0, 255]);
    map.remove_style_image("swatch").unwrap();
    render_to_idle(&mut runtime, &session);
    assert_eq!(center_pixel(&session), [0, 0, 255, 255]);
    session.close().unwrap();
    map.close().unwrap();
    runtime.close().unwrap();
}
