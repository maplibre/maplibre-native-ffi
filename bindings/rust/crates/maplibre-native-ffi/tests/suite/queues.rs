//! Queue delivery against native: each registration that a queue can carry
//! runs its callbacks from the queue and drops at retirement, completions and
//! their batch-backed results reach their handlers, both dispatchers run them
//! on their threads, and a queue that closes before its registrations retire
//! frees their handlers.

use std::cell::{Cell, RefCell};
use std::rc::Rc;
use std::sync::{Arc, Mutex, mpsc};
use std::time::{Duration, Instant};

use maplibre_native_ffi::*;

use crate::support::graphics::Graphics;
use crate::support::*;

const STYLE_URL: &str = "custom://style.json";

/// Runs what the dispatcher's queue holds, on this thread, until `done`
/// holds. `service` runs between drains, for work that only this thread
/// advances.
fn dispatch_until(
    dispatcher: &LocalDispatcher,
    what: &str,
    mut service: impl FnMut(),
    mut done: impl FnMut() -> bool,
) {
    let deadline = Instant::now() + timeout();
    loop {
        dispatcher.dispatch().unwrap();
        if done() {
            return;
        }
        let left = deadline.saturating_duration_since(Instant::now());
        assert!(!left.is_zero(), "timed out waiting for {what}");
        service();
        dispatcher.wait(left.min(Duration::from_millis(20)));
    }
}

/// Waits for a value that a handler sends.
#[track_caller]
fn received<T>(results: &mpsc::Receiver<T>) -> T {
    results
        .recv_timeout(timeout())
        .expect("no handler ran in time")
}

/// Runs the dispatcher's queue until the probe's handler drops.
fn await_queued_release(dispatcher: &LocalDispatcher, released: &mpsc::Receiver<()>) {
    dispatch_until(
        dispatcher,
        "the handler's retirement",
        || {},
        || released.try_recv().is_ok(),
    );
}

/// A queued command's handler that records that it committed.
fn committed(flag: &Rc<Cell<bool>>) -> impl FnOnce(QueuedResult<'_, Command>) + 'static {
    let flag = Rc::clone(flag);
    move |result| {
        result.unwrap();
        flag.set(true);
    }
}

#[test]
fn a_dispatcher_runs_completions_and_registrations_on_its_own_thread() {
    let fixture = Fixture::new();
    let dispatcher = Dispatcher::new().unwrap();
    let (sender, threads) = mpsc::channel();
    let (probe, released) = release_probe();
    let serve = serving_provider(STYLE_URL, BACKGROUND_STYLE_JSON).callback;
    let provider_sender = Mutex::new(sender.clone());
    fixture
        .runtime()
        .queued(dispatcher.queue())
        .set_resource_provider(
            ResourceProvider::new(move |request, handle| {
                let _ = &probe;
                let thread = std::thread::current().id();
                provider_sender.lock().unwrap().send(thread).unwrap();
                serve(request, handle);
            }),
            move |result| {
                result.unwrap();
                sender.send(std::thread::current().id()).unwrap();
            },
        )
        .unwrap();
    let dispatcher_thread = received(&threads);
    assert_ne!(dispatcher_thread, std::thread::current().id());

    fixture.map().discarding().set_style_url(STYLE_URL).unwrap();
    fixture.await_event_type(RuntimeEventType::MapStyleLoaded);
    assert_eq!(received(&threads), dispatcher_thread);

    // Replacing the provider retires the queued one, whose handler drops once
    // the dispatcher runs its RETIRED delivery.
    wait_for(fixture.runtime().set_resource_provider(denying_provider()));
    await_release(&released);
}

#[test]
fn a_local_dispatcher_runs_handlers_that_are_not_send_on_its_thread() {
    let fixture = Fixture::new();
    let dispatcher = LocalDispatcher::new().unwrap();
    let generation = Rc::new(Cell::new(0));
    let seen = Rc::clone(&generation);
    let update = CameraUpdate {
        mode: CameraUpdateMode::Jump,
        camera: CameraOptions {
            zoom: Some(3.0),
            ..Default::default()
        },
        ..Default::default()
    };
    fixture
        .map()
        .queued(dispatcher.queue())
        .update_camera(&update, move |result| {
            seen.set(result.unwrap().generation);
        })
        .unwrap();
    dispatch_until(
        &dispatcher,
        "the camera command",
        || {},
        || generation.get() != 0,
    );

    // A query's result reaches its handler as an owned copy.
    let zoom = Rc::new(Cell::new(None));
    let read = Rc::clone(&zoom);
    fixture
        .map()
        .queued(dispatcher.queue())
        .get_camera(move |result| read.set(Some(result.unwrap().camera.zoom)))
        .unwrap();
    dispatch_until(
        &dispatcher,
        "the camera query",
        || {},
        || zoom.get().is_some(),
    );
    assert_eq!(zoom.get(), Some(3.0));

    // A failed command reaches its handler with its error.
    let failed = Rc::new(Cell::new(false));
    let error = Rc::clone(&failed);
    fixture
        .map()
        .queued(dispatcher.queue())
        .set_style_json("not json", move |result| error.set(result.is_err()))
        .unwrap();
    dispatch_until(&dispatcher, "the failed command", || {}, || failed.get());
}

#[test]
fn a_queued_event_handler_receives_events_and_retires_when_cleared() {
    let fixture = Fixture::new();
    let dispatcher = LocalDispatcher::new().unwrap();
    let (probe, released) = release_probe();
    let (sender, events) = mpsc::channel();
    let accepted = Rc::new(Cell::new(false));
    fixture
        .runtime()
        .queued(dispatcher.queue())
        .set_event_handler(
            RuntimeEventHandler::local(move |delivered| {
                let _ = &probe;
                for event in delivered {
                    let _ = sender.send(event.r#type);
                }
            }),
            committed(&accepted),
        )
        .unwrap();
    fixture
        .map()
        .discarding()
        .set_style_json(BACKGROUND_STYLE_JSON)
        .unwrap();
    dispatch_until(
        &dispatcher,
        "the style's events",
        || {},
        || {
            events
                .try_iter()
                .any(|event| event == RuntimeEventType::MapStyleLoaded)
        },
    );
    assert!(accepted.get());

    fixture
        .runtime()
        .discarding()
        .clear_event_handler()
        .unwrap();
    await_queued_release(&dispatcher, &released);
}

#[test]
fn a_queued_provider_answers_requests_and_retires_when_replaced() {
    let fixture = Fixture::new();
    let dispatcher = LocalDispatcher::new().unwrap();
    let (probe, released) = release_probe();
    let serve = serving_provider(STYLE_URL, BACKGROUND_STYLE_JSON).callback;
    // A local provider runs only on the dispatcher's thread, so it may hold
    // state that isn't Send.
    let answered = Rc::new(RefCell::new(Vec::new()));
    let seen = Rc::clone(&answered);
    let accepted = Rc::new(Cell::new(false));
    fixture
        .runtime()
        .queued(dispatcher.queue())
        .set_resource_provider(
            ResourceProvider::local(move |request, handle| {
                let _ = &probe;
                seen.borrow_mut().push(request.requested_url.clone());
                serve(request, handle);
            }),
            committed(&accepted),
        )
        .unwrap();
    dispatch_until(&dispatcher, "the registration", || {}, || accepted.get());

    fixture.map().discarding().set_style_url(STYLE_URL).unwrap();
    dispatch_until(
        &dispatcher,
        "the request",
        || {},
        || !answered.borrow().is_empty(),
    );
    fixture.await_event_type(RuntimeEventType::MapStyleLoaded);
    assert_eq!(*answered.borrow(), [STYLE_URL]);

    wait_for(fixture.runtime().set_resource_provider(denying_provider()));
    await_queued_release(&dispatcher, &released);
}

#[test]
#[cfg_attr(
    target_os = "android",
    ignore = "Android's HTTP stack calls Java, which test executables run without"
)]
fn a_queued_transform_rewrites_urls_and_retires_when_cleared() {
    let fixture = Fixture::new();
    let server = LoopbackServer::start("/rewritten.json", BACKGROUND_STYLE_JSON);
    let original = server.url("/original.json");
    let rewritten = server.url("/rewritten.json");
    let dispatcher = LocalDispatcher::new().unwrap();
    let (probe, released) = release_probe();
    // The transform applies to native loading, which the fixture's provider
    // would otherwise take over.
    wait_for(fixture.runtime().clear_resource_provider());
    let accepted = Rc::new(Cell::new(false));
    fixture
        .runtime()
        .queued(dispatcher.queue())
        .set_resource_transform(
            ResourceTransform::local(move |_, url, request| {
                let _ = &probe;
                let url = url.replace("/original.json", "/rewritten.json");
                request.complete(&url).unwrap();
            }),
            committed(&accepted),
        )
        .unwrap();
    dispatch_until(&dispatcher, "the registration", || {}, || accepted.get());

    fixture.map().discarding().set_style_url(&original).unwrap();
    let deadline = Instant::now() + timeout();
    while server.requests().is_empty() {
        assert!(
            Instant::now() < deadline,
            "the rewritten URL was never fetched"
        );
        dispatcher.dispatch().unwrap();
        dispatcher.wait(Duration::from_millis(20));
    }
    fixture.await_event_type(RuntimeEventType::MapStyleLoaded);
    assert_eq!(server.requests(), ["/rewritten.json"]);
    let _ = rewritten;

    fixture
        .runtime()
        .discarding()
        .clear_resource_transform()
        .unwrap();
    await_queued_release(&dispatcher, &released);
}

#[test]
fn a_queued_cancel_handler_reports_a_cancelled_request_and_retires() {
    let fixture = Fixture::new();
    let (sender, requests) = mpsc::channel();
    let sender = Mutex::new(sender);
    let deny = denying_provider().callback;
    wait_for(
        fixture
            .runtime()
            .set_resource_provider(ResourceProvider::new(move |request, handle| {
                if request.requested_url != STYLE_URL {
                    return deny(request, handle);
                }
                sender.lock().unwrap().send(handle).unwrap();
            })),
    );
    fixture.map().discarding().set_style_url(STYLE_URL).unwrap();
    let handle = received(&requests);

    let dispatcher = LocalDispatcher::new().unwrap();
    let (probe, released) = release_probe();
    let cancelled = Rc::new(Cell::new(false));
    let saw = Arc::new(Mutex::new(false));
    let reported = Arc::clone(&saw);
    let already = handle
        .queued(dispatcher.queue())
        .set_cancel_callback(ResourceRequestCancelHandler::local(move || {
            let _ = &probe;
            *reported.lock().unwrap() = true;
        }))
        .unwrap();
    assert!(!already);

    // Releasing the map discards its pending style request, which MapLibre
    // cancels.
    let (runtime, map) = fixture.into_parts();
    wait_for(map.release());
    dispatch_until(&dispatcher, "the cancel", || {}, || *saw.lock().unwrap());
    cancelled.set(true);
    handle.release().unwrap();
    await_queued_release(&dispatcher, &released);
    wait_for(runtime.release());
}

#[test]
fn a_queued_camera_end_handler_inside_a_command_runs_once_and_retires() {
    let fixture = Fixture::new();
    let dispatcher = LocalDispatcher::new().unwrap();
    let (probe, released) = release_probe();
    let (sender, ends) = mpsc::channel();
    let update = CameraUpdate {
        mode: CameraUpdateMode::Ease,
        camera: CameraOptions {
            zoom: Some(4.0),
            ..Default::default()
        },
        animation: AnimationOptions {
            duration_ms: Some(60_000.0),
            transition_id: Some(u64::MAX),
            end_handler: CameraTransitionHandler::local(move |end| {
                let _ = &probe;
                sender.send(end.outcome).unwrap();
            }),
            ..Default::default()
        },
        ..Default::default()
    };
    let accepted = Rc::new(Cell::new(false));
    fixture
        .map()
        .queued(dispatcher.queue())
        .update_camera(&update, committed(&accepted))
        .unwrap();
    drop(update);
    wait_for(fixture.map().cancel_camera_transition(u64::MAX));
    dispatch_until(
        &dispatcher,
        "the camera end",
        || {},
        || ends.try_recv() == Ok(CameraTransitionOutcome::Cancelled),
    );
    assert!(accepted.get());
    await_queued_release(&dispatcher, &released);
    assert!(ends.try_recv().is_err());
}

#[test]
fn queued_custom_sources_fetch_tiles_and_retire_when_removed() {
    let graphics = Graphics::new();
    let extent = LogicalExtent::new(64, 64, 1.0);
    let fixture = Fixture::with_map_options(MapOptions::new(extent));
    fixture.load_style(BACKGROUND_STYLE_JSON);
    let dispatcher = LocalDispatcher::new().unwrap();
    let (geometry_probe, geometry_released) = release_probe();
    let (vector_probe, vector_released) = release_probe();
    let fetched = Arc::new(Mutex::new(Vec::new()));
    let geometry_fetched = Arc::clone(&fetched);
    let vector_fetched = Arc::clone(&fetched);
    let queued = fixture.map().queued(dispatcher.queue());
    let added = Rc::new(Cell::new(0));
    let count = |added: &Rc<Cell<i32>>| {
        let added = Rc::clone(added);
        move |result: QueuedResult<'_, Command>| {
            result.unwrap();
            added.set(added.get() + 1);
        }
    };
    queued
        .add_custom_geometry_source(
            "geometry",
            CustomGeometrySourceOptions::local(move |_| {
                let _ = &geometry_probe;
                geometry_fetched.lock().unwrap().push("geometry");
            }),
            count(&added),
        )
        .unwrap();
    queued
        .add_custom_vector_source(
            "vector",
            CustomVectorSourceOptions::local(move |_| {
                let _ = &vector_probe;
                vector_fetched.lock().unwrap().push("vector");
            }),
            count(&added),
        )
        .unwrap();
    for (id, source, extra) in [
        ("geometry-dots", "geometry", ""),
        ("vector-dots", "vector", r#","source-layer":"points""#),
    ] {
        let layer = format!(r#"{{"id":"{id}","type":"circle","source":"{source}"{extra}}}"#);
        wait_for(fixture.map().add_style_layer_json(&layer, ""));
    }
    dispatch_until(&dispatcher, "the sources", || {}, || added.get() == 2);

    // Rendering makes the sources' tiles required, which fetches them through
    // the queue.
    let session = graphics.owned_texture_session(fixture.map(), extent);
    dispatch_until(
        &dispatcher,
        "both sources' tile fetches",
        || {
            fixture.repaint();
            session.render_frame();
        },
        || {
            let fetched = fetched.lock().unwrap();
            fetched.contains(&"geometry") && fetched.contains(&"vector")
        },
    );
    session.close();

    for (layer, source) in [("geometry-dots", "geometry"), ("vector-dots", "vector")] {
        wait_for(fixture.map().remove_style_layer(layer));
        wait_for(fixture.map().remove_style_source(source));
    }
    await_queued_release(&dispatcher, &geometry_released);
    await_queued_release(&dispatcher, &vector_released);
}

#[test]
fn a_queued_log_handler_receives_records_and_retires_when_cleared() {
    let _global = global_state();
    let fixture = Fixture::new();
    let dispatcher = LocalDispatcher::new().unwrap();
    let (probe, released) = release_probe();
    let (sender, records) = mpsc::channel();
    set_log_callback_queued(
        dispatcher.queue(),
        LogHandler::local(move |severity, _, _, message| {
            let _ = &probe;
            let _ = sender.send((severity, message.to_owned()));
            true
        }),
    )
    .unwrap();

    // A discarded command that fails logs its failure as a warning.
    fixture
        .map()
        .discarding()
        .set_style_json("not json")
        .unwrap();
    fixture.fence();
    dispatch_until(
        &dispatcher,
        "a log record",
        || {},
        || {
            records
                .try_iter()
                .any(|(severity, _)| severity == LogSeverity::Warning)
        },
    );

    clear_log_callback().unwrap();
    await_queued_release(&dispatcher, &released);
}

#[test]
fn a_queued_readback_lends_its_pixels_from_the_batch() {
    let graphics = Graphics::new();
    let extent = LogicalExtent::new(32, 16, 1.0);
    let fixture = Fixture::with_map_options(MapOptions::new(extent));
    fixture.load_style(BACKGROUND_STYLE_JSON);
    fixture.repaint();
    let session = graphics.owned_texture_session(fixture.map(), extent);
    session.render_frame();

    let dispatcher = LocalDispatcher::new().unwrap();
    let kept = Rc::new(RefCell::new(None));
    let keep = Rc::clone(&kept);
    session
        .handle
        .queued(dispatcher.queue())
        .read_texture(move |readback| {
            let readback = readback.unwrap();
            assert_eq!((readback.info.width, readback.info.height), (32, 16));
            // The bytes are the batch's, which `keep` holds past the handler
            // without a copy.
            let bytes = readback.keep();
            assert_eq!(bytes.as_ptr(), readback.data().as_ptr());
            *keep.borrow_mut() = Some((bytes, readback.info.stride));
        })
        .unwrap();
    dispatch_until(
        &dispatcher,
        "the readback",
        || {
            let _ = session.handle.service_driver_work();
        },
        || kept.borrow().is_some(),
    );
    let (bytes, stride) = kept.borrow_mut().take().unwrap();
    for row in bytes.chunks_exact(stride as usize) {
        for pixel in row[..32 * 4].as_chunks::<4>().0 {
            assert_eq!(*pixel, BACKGROUND_RGBA);
        }
    }
    drop(bytes);
    session.close();
}

#[test]
fn closing_a_queue_before_its_registrations_retire_frees_their_handlers() {
    let fixture = Fixture::new();
    let queue = DeliveryQueue::<Shared>::new(None).unwrap();
    let (probe, released) = release_probe();
    let (sender, results) = mpsc::channel();
    let (query_sender, queried) = mpsc::channel();
    fixture
        .runtime()
        .queued(&queue)
        .set_resource_provider(
            ResourceProvider::new(move |_, handle| {
                let _ = &probe;
                handle.pass_through().unwrap();
            }),
            move |result| sender.send(result.map(|_| ())).unwrap(),
        )
        .unwrap();
    fixture
        .map()
        .queued(&queue)
        .get_style_url(move |url| query_sender.send(url.map(|_| ())).unwrap())
        .unwrap();
    fixture.fence();

    // Nothing drained the queue, so closing it cancels both completions and
    // drops the provider's handler, though native still holds the
    // registration.
    queue.close();
    await_release(&released);
    assert_eq!(received(&results).unwrap_err().status(), Status::Cancelled);
    assert_eq!(received(&queried).unwrap_err().status(), Status::Cancelled);

    // Native refuses the closed queue's requests, which fail, instead of
    // waiting on a handler that is gone.
    fixture.map().discarding().set_style_url(STYLE_URL).unwrap();
    fixture.await_event_type(RuntimeEventType::MapLoadingFailed);
    wait_for(fixture.runtime().set_resource_provider(denying_provider()));
}

#[test]
fn closing_a_dispatcher_runs_what_is_queued_and_cancels_what_is_not() {
    let fixture = Fixture::new();
    let dispatcher = Dispatcher::new().unwrap();
    let (sender, results) = mpsc::channel();
    let ran = sender.clone();
    fixture
        .map()
        .queued(dispatcher.queue())
        .get_style_url(move |url| ran.send(url.map(|_| ())).unwrap())
        .unwrap();
    fixture.fence();
    // An ease that outlives the dispatcher never completes before it closes.
    let update = CameraUpdate {
        mode: CameraUpdateMode::Ease,
        camera: CameraOptions {
            zoom: Some(4.0),
            ..Default::default()
        },
        animation: AnimationOptions {
            duration_ms: Some(60_000.0),
            transition_id: Some(u64::MAX - 1),
            end_handler: CameraTransitionHandler::new(|_| {}),
            ..Default::default()
        },
        ..Default::default()
    };
    let (end_sender, ended) = mpsc::channel();
    fixture
        .map()
        .queued(dispatcher.queue())
        .update_camera(&update, move |result| {
            end_sender.send(result.map(|_| ())).unwrap()
        })
        .unwrap();
    fixture.fence();
    dispatcher.close();
    received(&results).unwrap();
    received(&ended).unwrap();
    wait_for(fixture.map().cancel_camera_transition(u64::MAX - 1));
}
