//! The browser build's tests.
//!
//! Browser GPU contexts come from JavaScript, so these fixtures create their
//! own rather than loading tests/graphics. scripts/test-rust-browser.sh runs
//! each test in a page of its own, because Chromium keeps GPU and pthread
//! resources past their native handles.

#[cfg(mln_render_backend = "opengl")]
mod webgl;
#[cfg(mln_render_backend = "opengl")]
mod webgl_gl;
#[cfg(mln_render_backend = "webgpu")]
mod webgpu;

use std::sync::Arc;

use maplibre_native_ffi::*;

use crate::support::*;

const SIZE: LogicalSize = LogicalSize::new(32, 16);

/// A fixture whose map matches SIZE, with the background style loaded and
/// an update published for the first frame.
fn styled_fixture() -> Fixture {
    let fixture = Fixture::with_map_options(MapOptions::new(LogicalExtent::new(
        SIZE.width,
        SIZE.height,
        1.0,
    )));
    fixture.load_style(BACKGROUND_STYLE_JSON);
    fixture.repaint();
    fixture
}

fn assert_background(pixels: &[u8]) {
    assert_eq!(pixels.len(), SIZE.width as usize * SIZE.height as usize * 4);
    for pixel in pixels.as_chunks::<4>().0 {
        assert_eq!(*pixel, BACKGROUND_RGBA);
    }
}

#[cfg(mln_render_backend = "opengl")]
#[test]
fn a_webgl_session_renders_into_a_host_texture() {
    let fixture = styled_fixture();
    let texture = webgl::WebGlHostTexture::new(SIZE.width, SIZE.height).unwrap();
    let (options, wakes) = Session::attach_options(RenderDriverKind::CallerGraphicsThread);
    // SAFETY: the texture and its context outlive the session, on this thread.
    let attachment = unsafe {
        fixture
            .map()
            .attach_open_gl_borrowed_texture(&texture.descriptor(SIZE), &options)
    }
    .unwrap();
    let session = Session::finish_attach(attachment, wakes);

    assert_eq!(session.render_frame().disposition, RenderResult::Rendered);
    assert_background(&texture.read_rgba().unwrap());
    session.close();
}

#[cfg(mln_render_backend = "webgpu")]
#[test]
fn a_webgpu_session_renders_into_a_host_texture() {
    let fixture = styled_fixture();
    let context = webgpu::WebGpuTestContext::new().unwrap();
    let texture = webgpu::WebGpuHostTexture::new(&context, SIZE.width, SIZE.height).unwrap();
    let (options, wakes) = Session::attach_options(RenderDriverKind::CallerGraphicsThread);
    // SAFETY: the texture and its device outlive the session, on this thread.
    let attachment = unsafe {
        fixture
            .map()
            .attach_web_gpu_borrowed_texture(&texture.descriptor(SIZE, &context), &options)
    }
    .unwrap();
    let session = Session::finish_attach(attachment, wakes);

    assert_eq!(session.render_frame().disposition, RenderResult::Rendered);
    assert_background(&texture.read_rgba(&context).unwrap());
    session.close();
}

#[test]
fn a_completion_resolves_while_the_browser_runs_its_loop() {
    let fixture = Fixture::new();
    // The test thread blocks on a completion that the browser's run loop
    // delivers, both through a wait and through a future's waker.
    fixture.fence();
    let woken = Arc::new(Signal::default());
    drive(&woken, fixture.map().get_style_url().unwrap(), || {}).unwrap();
}

#[test]
fn a_panicking_void_callback_is_contained_and_reported() {
    assert_panicking_void_callback_is_reported();
}

#[test]
fn a_provider_answers_a_style_request_in_the_browser() {
    let fixture = Fixture::new();
    wait_for(fixture.runtime().set_resource_provider(serving_provider(
        "custom://style.json",
        BACKGROUND_STYLE_JSON,
    )));
    fixture
        .map()
        .discarding()
        .set_style_url("custom://style.json")
        .unwrap();
    fixture.await_event_type(RuntimeEventType::MapStyleLoaded);
}

#[test]
fn dropping_an_attached_session_and_its_parents_disposes_the_graph() {
    let reports = capture_reports();

    let fixture = styled_fixture();
    let session = Session::offscreen(fixture.map(), SIZE);
    assert_eq!(session.render_frame().disposition, RenderResult::Rendered);
    let (runtime, map) = fixture.into_parts();
    // No explicit close: each drop disposes its handle, children first.
    drop(session);
    drop(map);
    drop(runtime);

    set_reporter(None);
    let leaked = leaks(&reports);
    assert!(leaked.is_empty(), "{leaked:?}");
}
