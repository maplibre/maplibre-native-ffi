//! Render sessions on the backend this build compiled in, with GPU contexts
//! from tests/graphics.

use std::sync::{Arc, mpsc};

use maplibre_native_ffi::*;

use crate::support::graphics::{Graphics, with_frame_view};
use crate::support::*;

const EXTENT: LogicalExtent = LogicalExtent::new(32, 16, 1.0);

/// A fixture whose map matches EXTENT, with the background style loaded.
fn styled_fixture() -> Fixture {
    let fixture = Fixture::with_map_options(MapOptions {
        initial_extent: LogicalExtent::new(EXTENT.width, EXTENT.height, EXTENT.scale_factor),
        ..Default::default()
    });
    fixture.load_style(BACKGROUND_STYLE_JSON);
    // The first frame renders the update that this publishes.
    fixture.repaint();
    fixture
}

/// A styled fixture with an owned-texture session attached on this thread,
/// which drives it.
fn rendering_fixture(graphics: &Graphics) -> (Fixture, Session) {
    let fixture = styled_fixture();
    let session = graphics.owned_texture_session(fixture.map(), EXTENT);
    (fixture, session)
}

#[test]
fn an_owned_texture_session_renders_a_frame_the_binding_reads_back() {
    let graphics = Graphics::new();
    let (_fixture, session) = rendering_fixture(&graphics);

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
    let (_fixture, session) = rendering_fixture(&graphics);
    // Native reports both calls as not ready, which reads as empty and as no
    // value.
    assert!(session.handle.drain_frame_results().unwrap().is_empty());
    assert!(session.handle.acquire_frame().unwrap().is_none());
    session.close();
}

#[test]
fn a_frame_view_lasts_for_its_scope_and_its_owners_stay_busy_inside_it() {
    let graphics = Graphics::new();
    let (fixture, session) = rendering_fixture(&graphics);
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
    let (size, (frame_release, session_abandon)) = with_frame_view(&frame, || {
        // Releasing a sibling frame leaves this view's texture alone.
        sibling.release(Some(&cpu_complete)).unwrap();
        (
            frame.release(None).unwrap_err().status(),
            session.handle.abandon().unwrap_err().status(),
        )
    })
    .unwrap();
    assert_eq!(size, (32, 16));
    // Native keeps the frame and its session busy while the view is open.
    assert_eq!(frame_release, Status::Busy);
    assert_eq!(session_abandon, Status::Busy);

    // Once the view's scope ends, both close.
    assert!(!frame.is_closed());
    frame.release(None).unwrap();
    assert!(frame.is_closed() && sibling.is_closed());
    // Either consuming call on a closed frame does nothing.
    frame.release(None).unwrap();
    frame.discard().unwrap();
    session.close();
}

#[test]
fn a_caller_driven_session_is_serviced_from_the_thread_that_claimed_it() {
    let fixture = styled_fixture();
    let map = fixture.map();

    // The host's graphics thread creates the context, attaches, and drives
    // the session; this thread holds only the session's control handle.
    let (sender, attached) = mpsc::channel::<Arc<RenderSessionHandle>>();
    let (finish, finished) = mpsc::channel::<()>();
    std::thread::scope(|scope| {
        let graphics_thread = scope.spawn(move || {
            let graphics = Graphics::new();
            let (options, wakes) = Session::attach_options(RenderDriverKind::CallerGraphicsThread);
            let session = Session::finish_attach(
                graphics
                    .attach_owned_texture(map, EXTENT, &options)
                    .unwrap(),
                wakes,
            );
            assert_eq!(session.render_frame().disposition, RenderResult::Rendered);
            let Session { handle, wakes } = session;
            let handle = Arc::new(handle);
            sender.send(Arc::clone(&handle)).unwrap();
            finished.recv_timeout(timeout()).unwrap();
            // The graphics thread is the sole owner again, and closes it.
            let handle = Arc::into_inner(handle).unwrap();
            Session { handle, wakes }.close();
        });

        let session = attached.recv_timeout(timeout()).unwrap();
        // Controls work from any thread, but the driver's work belongs to the
        // thread whose first service call claimed it.
        assert_eq!(
            session.get_snapshot().unwrap().state,
            RenderSessionState::Attached
        );
        let error = session.service_driver_work().unwrap_err();
        assert_eq!(error.status(), Status::WrongThread);
        drop(session);
        finish.send(()).unwrap();
        graphics_thread.join().unwrap();
    });
}

#[test]
fn a_released_session_lets_the_host_destroy_its_graphics_objects() {
    let graphics = Graphics::new();
    let (_fixture, session) = rendering_fixture(&graphics);
    assert_eq!(session.render_frame().disposition, RenderResult::Rendered);
    // The release completes once the attached session has retired, so the
    // device may go while the map stays open.
    session.drive(session.handle.release().unwrap()).unwrap();
    drop(graphics);
}

#[test]
fn a_session_dropped_while_attached_retires_before_its_map_releases() {
    let graphics = Graphics::new();
    let (fixture, session) = rendering_fixture(&graphics);
    assert_eq!(session.render_frame().disposition, RenderResult::Rendered);
    // Drop disposes the attached session without reporting its retirement.
    drop(session);
    let (runtime, map) = fixture.into_parts();
    // The map's release completes after the session releases its map
    // attachment, once no library thread touches the device.
    wait_for(map.release());
    drop(graphics);
    wait_for(runtime.release());
}

#[test]
fn dropping_an_attached_session_and_its_parents_disposes_the_graph() {
    let _global = global_state();
    let reports = capture_reports();

    let graphics = Graphics::new();
    let (fixture, session) = rendering_fixture(&graphics);
    assert_eq!(session.render_frame().disposition, RenderResult::Rendered);
    let (runtime, map) = fixture.into_parts();
    // No explicit close: each drop disposes its handle, children first.
    let Session { handle, wakes } = session;
    drop(handle);
    drop(map);
    drop(runtime);
    // The graph retires off this thread, and the session can still reach the
    // device until it does, so the device outlives the session's wakes.
    await_release(&wakes.released);

    set_reporter(None);
    let leaked = leaks(&reports);
    assert!(leaked.is_empty(), "{leaked:?}");
}

// Only a core worker attaches without the host servicing driver work, which
// is what dropping the session takes away. Context-sharing backends drive
// their sessions from the host's thread.
#[cfg(not(mln_render_backend = "opengl"))]
#[test]
fn dropping_a_session_before_its_attachment_completes_reports_no_leak() {
    let _global = global_state();
    let reports = capture_reports();

    let graphics = Graphics::new();
    let fixture = styled_fixture();
    let (options, wakes) = Session::attach_options(RenderDriverKind::CoreWorker);
    let (session, attached) = graphics
        .attach_owned_texture(fixture.map(), EXTENT, &options)
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
    // The options hold the wakes too, and the device outlives them.
    drop(options);
    await_release(&wakes.released);

    set_reporter(None);
    let leaked = leaks(&reports);
    assert!(leaked.is_empty(), "{leaked:?}");
}
