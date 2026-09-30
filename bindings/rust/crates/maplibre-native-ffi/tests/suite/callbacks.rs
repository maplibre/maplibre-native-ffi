//! Callback registrations: rooting and release, the admission policy inside a
//! callback, panics, decision handles, and process-global callbacks.

use std::sync::atomic::{AtomicUsize, Ordering};
use std::sync::{Arc, Mutex, mpsc};

use maplibre_native_ffi::*;

use crate::support::*;

#[test]
fn a_registration_is_rooted_while_native_holds_it_and_a_rejected_one_is_released() {
    let fixture = Fixture::new();

    // An accepted provider stays alive while native holds it, and replacing
    // it releases it.
    let (probe, released) = release_probe();
    wait_for(
        fixture
            .runtime()
            .set_resource_provider(ResourceProvider::new(move |_, _| {
                let _ = &probe;
                ResourceProviderDecision::PassThrough
            })),
    );
    assert!(released.try_recv().is_err());
    wait_for(fixture.runtime().set_resource_provider(denying_provider()));
    await_release(&released);

    // A registration that native rejects is never handed over, so the binding
    // releases it before the call returns.
    let (probe, released) = release_probe();
    let options = CustomGeometrySourceOptions {
        min_zoom: Some(3.0),
        max_zoom: Some(2.0),
        ..CustomGeometrySourceOptions::new(move |_| {
            let _ = &probe;
        })
    };
    let error = fixture
        .map()
        .add_custom_geometry_source("rejected", options.clone())
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    assert!(error.raw_status().is_some());
    assert!(error.diagnostic().contains("min_zoom"), "{error}");
    drop(options);
    released
        .try_recv()
        .expect("a rejected registration kept its callback");
}

#[test]
fn a_registered_callback_does_not_keep_its_runtime_alive() {
    let (probe, released) = release_probe();
    let runtime = runtime_create(&RuntimeOptions::default()).unwrap();
    wait_for(
        runtime.set_resource_provider(ResourceProvider::new(move |_, _| {
            let _ = &probe;
            ResourceProviderDecision::PassThrough
        })),
    );

    // Native holds the callback, but not the handle it is registered on, so
    // dropping the handle disposes the runtime, which releases the callback.
    drop(runtime);
    await_release(&released);
}

#[test]
fn a_callback_may_answer_its_request_but_not_fence_or_close_its_runtime() {
    let fixture = Fixture::new();
    let runtime = Arc::downgrade(fixture.runtime());
    let (sender, outcomes) = mpsc::channel();
    let sender = Mutex::new(sender);
    let deny = denying_provider().callback.unwrap();
    wait_for(
        fixture
            .runtime()
            .set_resource_provider(ResourceProvider::new(move |request, handle| {
                if request.requested_url.as_deref() != Some("custom://style.json") {
                    return deny(request, handle);
                }
                let runtime = runtime.upgrade().unwrap();
                let outcome = (
                    runtime.barrier().map(drop).map_err(|error| error.kind()),
                    runtime.release().map(drop).map_err(|error| error.kind()),
                    handle
                        .complete(&ok_response(BACKGROUND_STYLE_JSON))
                        .map_err(|error| error.kind()),
                );
                sender.lock().unwrap().send(outcome).unwrap();
                ResourceProviderDecision::Handle
            })),
    );

    fixture.map().set_style_url("custom://style.json").unwrap();
    fixture.await_event_type(RuntimeEventType::MapStyleLoaded);
    let (barrier, release, completed) = outcomes.recv_timeout(timeout()).unwrap();
    assert_eq!(barrier, Err(ErrorKind::InvalidState));
    assert_eq!(release, Err(ErrorKind::InvalidState));
    assert_eq!(completed, Ok(()));
}

#[test]
fn a_panicking_callback_is_contained_and_its_request_fails() {
    let fixture = Fixture::new();
    let calls = Arc::new(AtomicUsize::new(0));
    let counted = Arc::clone(&calls);
    wait_for(
        fixture
            .runtime()
            .set_resource_provider(ResourceProvider::new(move |_, _| {
                counted.fetch_add(1, Ordering::SeqCst);
                panic!("the provider panicked");
            })),
    );

    // The binding catches the panic at the callback boundary, and native
    // fails the request it got no decision for.
    fixture.map().set_style_url("custom://style.json").unwrap();
    fixture.await_event_type(RuntimeEventType::MapLoadingFailed);
    assert!(calls.load(Ordering::SeqCst) > 0);
    // The runtime keeps working.
    wait_for(fixture.runtime().set_resource_provider(denying_provider()));
    fixture.barrier();
}

#[test]
fn a_provider_hands_its_request_to_another_thread_and_answers_later() {
    let fixture = Fixture::new();
    let (sender, requests) = mpsc::channel();
    let sender = Mutex::new(sender);
    let deny = denying_provider().callback.unwrap();
    wait_for(
        fixture
            .runtime()
            .set_resource_provider(ResourceProvider::new(move |request, handle| {
                if request.requested_url.as_deref() != Some("custom://style.json") {
                    return deny(request, handle);
                }
                sender.lock().unwrap().send(handle).unwrap();
                ResourceProviderDecision::Handle
            })),
    );

    fixture.map().set_style_url("custom://style.json").unwrap();
    let handle = requests.recv_timeout(timeout()).unwrap();
    std::thread::spawn(move || {
        assert!(!handle.cancelled().unwrap());
        handle
            .complete(&ok_response(BACKGROUND_STYLE_JSON))
            .unwrap();
        handle.close().unwrap();
    })
    .join()
    .unwrap();
    fixture.await_event_type(RuntimeEventType::MapStyleLoaded);
}

#[test]
fn replacing_the_log_callback_releases_the_one_it_replaced() {
    let _global = global_state();
    let (first, first_released) = release_probe();
    log_set_callback(Some(Arc::new(move |_, _, _, _| {
        let _ = &first;
        1
    })))
    .unwrap();
    assert!(first_released.try_recv().is_err());

    let (second, second_released) = release_probe();
    log_set_callback(Some(Arc::new(move |_, _, _, _| {
        let _ = &second;
        1
    })))
    .unwrap();
    await_release(&first_released);
    assert!(second_released.try_recv().is_err());

    log_clear_callback().unwrap();
    await_release(&second_released);
}

#[test]
fn registering_for_an_already_cancelled_request_reports_it_and_roots_nothing() {
    let fixture = Fixture::new();
    let (sender, requests) = mpsc::channel();
    let sender = Mutex::new(sender);
    let deny = denying_provider().callback.unwrap();
    wait_for(
        fixture
            .runtime()
            .set_resource_provider(ResourceProvider::new(move |request, handle| {
                if request.requested_url.as_deref() != Some("custom://style.json") {
                    return deny(request, handle);
                }
                sender.lock().unwrap().send(handle).unwrap();
                ResourceProviderDecision::Handle
            })),
    );
    fixture.map().set_style_url("custom://style.json").unwrap();
    let handle = requests.recv_timeout(timeout()).unwrap();

    // Releasing the map discards its pending style request, which MapLibre
    // then cancels.
    let (runtime, map) = fixture.into_parts();
    wait_for(map.release());
    await_condition("the request to report cancelled", || {
        handle.cancelled().unwrap()
    });

    // Native reports the cancellation instead of accepting the callback, so
    // the binding drops the callback unrun before returning.
    let (probe, released) = release_probe();
    let already_cancelled = handle
        .set_cancel_callback(move || {
            let _ = &probe;
            panic!("a cancelled request ran its cancel callback");
        })
        .unwrap();
    assert!(already_cancelled);
    released
        .try_recv()
        .expect("the binding kept a callback that native never took");
    handle.close().unwrap();
    wait_for(runtime.release());
}
