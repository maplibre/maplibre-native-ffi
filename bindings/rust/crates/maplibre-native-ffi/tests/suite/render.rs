//! Render sessions offscreen, and on the backend this build compiled in with
//! GPU contexts and textures from tests/graphics.

use maplibre_native_ffi::*;

use crate::support::graphics::{Graphics, Texture, with_frame_view};
use crate::support::*;

const EXTENT: LogicalExtent = LogicalExtent::new(32, 16, 1.0);

fn styled_fixture() -> Fixture {
    let fixture = Fixture::with_map_options(MapOptions::new(LogicalExtent::new(
        EXTENT.width,
        EXTENT.height,
        EXTENT.scale_factor,
    )));
    fixture.load_style(BACKGROUND_STYLE_JSON);
    // The first frame renders the update that this publishes.
    fixture.repaint();
    fixture
}

/// A styled fixture with an offscreen session attached.
fn offscreen_fixture() -> (Fixture, Session) {
    let fixture = styled_fixture();
    let session = Session::offscreen(fixture.map(), EXTENT);
    (fixture, session)
}

/// A host texture of EXTENT's size, for one slot of a borrowed ring.
fn host_texture(graphics: &Graphics) -> Texture<'_> {
    graphics.texture(EXTENT.width, EXTENT.height)
}

#[test]
fn an_offscreen_session_renders_a_frame_the_binding_reads_back() {
    let (_fixture, session) = offscreen_fixture();

    let result = session.render_frame();
    assert_eq!(result.disposition, RenderResult::Rendered);
    let frame = session.read_back();
    assert_eq!((frame.info.width, frame.info.height), (32, 16));
    assert_eq!(
        frame.data.len(),
        frame.info.stride as usize * frame.info.height as usize
    );
    for row in frame.data.chunks_exact(frame.info.stride as usize) {
        for pixel in row[..32 * 4].as_chunks::<4>().0 {
            assert_eq!(*pixel, BACKGROUND_RGBA);
        }
    }
    session.close();
}

#[test]
fn a_drain_before_any_demand_returns_no_batch() {
    let graphics = Graphics::new();
    let ring = [host_texture(&graphics)];
    let fixture = styled_fixture();
    let session = graphics.borrowed_texture_session(fixture.map(), EXTENT, &ring);
    // Native writes a null batch and a null frame, which read as empty and as
    // no value.
    assert!(session.handle.drain_frame_results().unwrap().is_empty());
    assert!(session.handle.acquire_frame().unwrap().is_none());
    session.close();
}

#[test]
fn a_frame_view_lasts_for_its_scope_and_its_owners_stay_busy_inside_it() {
    let graphics = Graphics::new();
    // The test holds two frames at once, one in each texture.
    let ring = [host_texture(&graphics), host_texture(&graphics)];
    let fixture = styled_fixture();
    let session = graphics.borrowed_texture_session(fixture.map(), EXTENT, &ring);
    assert_eq!(session.render_frame().disposition, RenderResult::Rendered);
    let frame = session
        .handle
        .acquire_frame()
        .unwrap()
        .expect("a rendered frame");
    // A second frame needs a newer map update to render.
    fixture.repaint();
    assert_eq!(session.render_frame().disposition, RenderResult::Rendered);
    let sibling = session
        .handle
        .acquire_frame()
        .unwrap()
        .expect("a rendered frame");

    let cpu_complete = GpuSync::default();
    let (size, session_abandon) = with_frame_view(&frame, || {
        // Releasing a sibling frame leaves this view's texture alone.
        sibling.release(Some(&cpu_complete)).unwrap();
        session.handle.abandon().unwrap_err().status()
    })
    .unwrap();
    assert_eq!(size, (32, 16));
    // Native keeps the session busy while the view is open.
    assert_eq!(session_abandon, Status::Busy);

    // Once the view's scope ends, the frame releases.
    frame.release(None).unwrap();
    session.close();
}

#[test]
fn dropping_an_attached_session_and_its_parents_disposes_the_graph() {
    let _global = global_state();
    let reports = capture_reports();

    let (fixture, session) = offscreen_fixture();
    assert_eq!(session.render_frame().disposition, RenderResult::Rendered);
    let (runtime, map) = fixture.into_parts();
    // No explicit close: each drop disposes its handle, children first.
    let Session { handle, wakes } = session;
    drop(handle);
    drop(map);
    drop(runtime);
    // The graph retires off this thread, which releases the session's wakes.
    await_release(&wakes.released);

    set_reporter(None);
    let leaked = leaks(&reports);
    assert!(leaked.is_empty(), "{leaked:?}");
}

// An offscreen session's core worker attaches without the host servicing
// driver work, which is what dropping the session takes away.
#[test]
fn dropping_a_session_before_its_attachment_completes_reports_no_leak() {
    let _global = global_state();
    let reports = capture_reports();

    let fixture = styled_fixture();
    let (options, wakes) = Session::attach_options(RenderDriverKind::CoreWorker);
    let (session, attached) = fixture
        .map()
        .attach_offscreen(&OffscreenDescriptor::new(EXTENT), &options, None)
        .unwrap();
    // The drop races the attachment. Native owns the session either way, so
    // the completion still resolves and nothing leaks.
    drop(session);
    let _ = attached
        .wait(timeout())
        .expect("the attachment never completed");
    let (runtime, map) = fixture.into_parts();
    drop(map);
    drop(runtime);
    // The options hold the wakes too.
    drop(options);
    await_release(&wakes.released);

    set_reporter(None);
    let leaked = leaks(&reports);
    assert!(leaked.is_empty(), "{leaked:?}");
}
