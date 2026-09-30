//! Render sessions on the backend this build compiled in, with GPU contexts
//! from tests/graphics.

use std::sync::{Arc, mpsc};

use maplibre_native_ffi::*;

use crate::support::graphics::{Graphics, with_frame_view};
use crate::support::*;

const EXTENT: RenderTargetExtent = RenderTargetExtent::new(32, 16, 1.0);

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
        for pixel in row[..32 * 4].chunks_exact(4) {
            assert_eq!(pixel, BACKGROUND_RGBA);
        }
    }
    session.close();
}

#[test]
fn a_frame_view_lasts_for_its_scope_and_its_owners_stay_busy_inside_it() {
    let graphics = Graphics::new();
    let (fixture, session) = rendering_fixture(&graphics);
    assert_eq!(session.render_frame().disposition, RenderResult::Rendered);
    let frame = session.handle.acquire_frame().unwrap();
    // A second frame needs a newer map update to render.
    fixture.repaint();
    assert_eq!(session.render_frame().disposition, RenderResult::Rendered);
    let sibling = session.handle.acquire_frame().unwrap();

    let no_sync = GpuSync::default();
    let (size, (frame_release, session_abandon)) = with_frame_view(&frame, || {
        // Releasing a sibling frame leaves this view's texture alone.
        sibling.release(&no_sync).unwrap();
        (
            frame.release(&no_sync).unwrap_err().kind(),
            session.handle.abandon().unwrap_err().kind(),
        )
    })
    .unwrap();
    assert_eq!(size, (32, 16));
    // The binding refuses to release a frame while its view is out, and
    // native refuses to abandon the session that owns it.
    assert_eq!(frame_release, ErrorKind::InvalidState);
    assert_eq!(session_abandon, ErrorKind::Busy);

    // Once the view's scope ends, both close.
    frame.release(&no_sync).unwrap();
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
        let error = session.service_driver_work(64).unwrap_err();
        assert_eq!(error.kind(), ErrorKind::WrongThread);
        drop(session);
        finish.send(()).unwrap();
        graphics_thread.join().unwrap();
    });
}

#[test]
fn a_live_session_refuses_map_release_and_keeps_the_map_alive() {
    let _global = global_state();
    let (sender, leaks) = mpsc::channel();
    set_leak_reporter(Some(Box::new(move |leak| {
        let _ = sender.send(leak);
    })));

    let graphics = Graphics::new();
    let (fixture, session) = rendering_fixture(&graphics);
    let (runtime, map) = fixture.into_parts();

    let error = map.release().unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidState);
    assert!(error.diagnostic().contains("render session"), "{error}");
    // The session holds its map, so dropping the map's handle first is not a
    // leak: the map retires once the session lets it go.
    drop(map);
    session.close();
    wait_for(runtime.release());

    set_leak_reporter(None);
    let leaked: Vec<_> = leaks.try_iter().collect();
    assert!(leaked.is_empty(), "{leaked:?}");
}

#[test]
fn dropping_an_attached_session_and_its_parents_disposes_the_graph() {
    let _global = global_state();
    let (sender, leaks) = mpsc::channel();
    set_leak_reporter(Some(Box::new(move |leak| {
        let _ = sender.send(leak);
    })));

    let graphics = Graphics::new();
    let (fixture, session) = rendering_fixture(&graphics);
    assert_eq!(session.render_frame().disposition, RenderResult::Rendered);
    let (runtime, map) = fixture.into_parts();
    // No explicit close: each drop disposes its handle, children first.
    drop(session);
    drop(map);
    drop(runtime);

    set_leak_reporter(None);
    let leaked: Vec<_> = leaks.try_iter().collect();
    assert!(leaked.is_empty(), "{leaked:?}");
}

// Only a core worker attaches without the host servicing driver work, which
// is what dropping the session takes away. Context-sharing backends drive
// their sessions from the host's thread.
#[cfg(not(mln_render_backend = "opengl"))]
#[test]
fn dropping_a_session_before_its_attachment_completes_reports_no_leak() {
    let _global = global_state();
    let (sender, leaks) = mpsc::channel();
    set_leak_reporter(Some(Box::new(move |leak| {
        let _ = sender.send(leak);
    })));

    let graphics = Graphics::new();
    let fixture = styled_fixture();
    let (options, _wakes) = Session::attach_options(RenderDriverKind::CoreWorker);
    let (session, attached) = graphics
        .attach_owned_texture(fixture.map(), EXTENT, &options)
        .unwrap();
    // The drop races the attachment. Native owns the session either way, so
    // the completion still resolves and nothing leaks.
    drop(session);
    assert!(attached.wait(timeout()).unwrap());
    let _ = attached.take();
    let (runtime, map) = fixture.into_parts();
    drop(map);
    drop(runtime);

    set_leak_reporter(None);
    let leaked: Vec<_> = leaks.try_iter().collect();
    assert!(leaked.is_empty(), "{leaked:?}");
}
