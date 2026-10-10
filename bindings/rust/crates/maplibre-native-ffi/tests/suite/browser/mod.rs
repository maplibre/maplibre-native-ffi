//! The browser build's tests: a render smoke test on the build's backend, a
//! completion on the browser's run loop, a resource provider callback, and a
//! drop graph.
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

const EXTENT: LogicalExtent = LogicalExtent::new(32, 16, 1.0);

/// A fixture whose map matches EXTENT, with the background style loaded and
/// an update published for the first frame.
fn styled_fixture() -> Fixture {
    let fixture = Fixture::with_map_options(MapOptions {
        initial_extent: LogicalExtent::new(EXTENT.width, EXTENT.height, EXTENT.scale_factor),
        ..Default::default()
    });
    fixture.load_style(BACKGROUND_STYLE_JSON);
    fixture.repaint();
    fixture
}

fn assert_background(pixels: &[u8]) {
    assert_eq!(
        pixels.len(),
        EXTENT.width as usize * EXTENT.height as usize * 4
    );
    for pixel in pixels.chunks_exact(4) {
        assert_eq!(pixel, BACKGROUND_RGBA);
    }
}

/// Attaches a session-owned texture on a context of the build's backend,
/// which this thread drives, and returns the context that must outlive it.
#[cfg(mln_render_backend = "opengl")]
fn owned_texture_session(map: &MapHandle) -> (webgl::WebGlTestContext, Session) {
    let context = webgl::WebGlTestContext::new(EXTENT.width, EXTENT.height).unwrap();
    let (options, wakes) = Session::attach_options(RenderDriverKind::CallerGraphicsThread);
    // SAFETY: the context outlives the session, on this thread.
    let attachment = unsafe {
        map.attach_opengl_owned_texture(
            &OpenglOwnedTextureDescriptor {
                extent: EXTENT,
                context: context.descriptor(),
            },
            &options,
        )
    }
    .unwrap();
    (context, Session::finish_attach(attachment, wakes))
}

#[cfg(mln_render_backend = "webgpu")]
fn owned_texture_session(map: &MapHandle) -> (webgpu::WebGpuTestContext, Session) {
    let context = webgpu::WebGpuTestContext::new().unwrap();
    let (options, wakes) = Session::attach_options(RenderDriverKind::CallerGraphicsThread);
    // SAFETY: the device outlives the session, on this thread.
    let attachment = unsafe {
        map.attach_webgpu_owned_texture(
            &WebgpuOwnedTextureDescriptor {
                extent: EXTENT,
                context: context.descriptor(),
            },
            &options,
        )
    }
    .unwrap();
    (context, Session::finish_attach(attachment, wakes))
}

#[cfg(mln_render_backend = "opengl")]
#[test]
fn a_webgl_session_renders_into_a_host_texture() {
    let fixture = styled_fixture();
    let texture = webgl::WebGlBorrowedTexture::new(EXTENT.width, EXTENT.height).unwrap();
    let (options, wakes) = Session::attach_options(RenderDriverKind::CallerGraphicsThread);
    // SAFETY: the texture and its context outlive the session, on this thread.
    let attachment = unsafe {
        fixture
            .map()
            .attach_opengl_borrowed_texture(&texture.descriptor(EXTENT), &options)
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
    let texture =
        webgpu::WebGpuBorrowedTexture::new(&context, EXTENT.width, EXTENT.height).unwrap();
    let (options, wakes) = Session::attach_options(RenderDriverKind::CallerGraphicsThread);
    // SAFETY: the texture and its device outlive the session, on this thread.
    let attachment = unsafe {
        fixture
            .map()
            .attach_webgpu_borrowed_texture(&texture.descriptor(EXTENT, &context), &options)
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
    fixture.barrier();
    let woken = Arc::new(Signal::default());
    drive(&woken, fixture.runtime().barrier().unwrap(), || 0).unwrap();
}

#[test]
fn a_provider_answers_a_style_request_in_the_browser() {
    let fixture = Fixture::new();
    wait_for(fixture.runtime().set_resource_provider(serving_provider(
        "custom://style.json",
        BACKGROUND_STYLE_JSON,
    )));
    fixture.map().set_style_url("custom://style.json").unwrap();
    fixture.await_event_type(RuntimeEventType::MapStyleLoaded);
}

#[test]
fn dropping_an_attached_session_and_its_parents_disposes_the_graph() {
    let reports = capture_reports();

    let fixture = styled_fixture();
    let (context, session) = owned_texture_session(fixture.map());
    assert_eq!(session.render_frame().disposition, RenderResult::Rendered);
    let (runtime, map) = fixture.into_parts();
    // No explicit close: each drop disposes its handle, children first.
    drop(session);
    drop(map);
    drop(runtime);
    drop(context);

    set_reporter(None);
    let leaked = leaks(&reports);
    assert!(leaked.is_empty(), "{leaked:?}");
}
