//! Callback registrations: rooting and release, contained panics, request
//! ownership, provider routes, and process-global
//! callbacks.

use std::sync::atomic::{AtomicUsize, Ordering};
use std::sync::{Arc, Mutex, mpsc};

use maplibre_native_ffi::*;

use crate::support::*;

/// A style whose GeoJSON source loads from a URL beside it.
const ROUTED_STYLE_JSON: &str = r#"{"version":8,"sources":{"points":{"type":"geojson","data":"custom://routed/source.geojson"}},"layers":[]}"#;

#[test]
fn a_registration_is_rooted_while_native_holds_it_and_a_rejected_one_is_released() {
    let fixture = Fixture::new();

    // An accepted provider stays alive while native holds it, and replacing
    // it releases it.
    let (probe, released) = release_probe();
    wait_for(
        fixture
            .runtime()
            .set_resource_provider(ResourceProvider::new(move |_, handle| {
                let _ = &probe;
                handle.pass_through().unwrap();
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
    assert_eq!(error.status(), Status::InvalidArgument);
    assert!(error.diagnostic().contains("min_zoom"), "{error}");
    drop(options);
    released
        .try_recv()
        .expect("a rejected registration kept its callback");
}

#[test]
fn a_camera_end_handler_inside_a_command_is_rooted_until_it_runs() {
    let fixture = Fixture::new();

    // The handler sits two records deep in the command's input. Native holds
    // it while the ease runs, and runs it once when its identity is cancelled.
    let (probe, released) = release_probe();
    let (ended, outcomes) = mpsc::channel();
    let update = CameraUpdate {
        mode: CameraUpdateMode::Ease,
        camera: CameraOptions {
            zoom: Some(4.0),
            ..Default::default()
        },
        animation: AnimationOptions {
            duration_ms: Some(60_000.0),
            transition_id: Some(u64::MAX),
            end_handler: CameraTransitionHandler::new(move |end| {
                let _ = &probe;
                ended.send(end).unwrap();
            }),
            ..Default::default()
        },
        ..Default::default()
    };
    wait_for(fixture.map().update_camera(&update));
    drop(update);
    assert!(released.try_recv().is_err());
    wait_for(fixture.map().cancel_camera_transition(u64::MAX));
    let end = outcomes.recv_timeout(timeout()).unwrap();
    assert_eq!(end.outcome, CameraTransitionOutcome::Cancelled);
    assert_ne!(end.generation, 0);
    await_release(&released);
    assert!(outcomes.try_recv().is_err());

    // A rejected command releases the handler before the call returns.
    let (probe, released) = release_probe();
    let delta = CameraDelta {
        scale: Some(-1.0),
        animation: AnimationOptions {
            end_handler: CameraTransitionHandler::new(move |_| {
                let _ = &probe;
            }),
            ..Default::default()
        },
        ..Default::default()
    };
    let error = fixture.map().apply_camera_delta(&delta).unwrap_err();
    assert_eq!(error.status(), Status::InvalidArgument);
    drop(delta);
    released
        .try_recv()
        .expect("a rejected command kept its end handler");
}

#[test]
fn a_registered_callback_does_not_keep_its_runtime_alive() {
    let (probe, released) = release_probe();
    let runtime = runtime_create(&RuntimeOptions::default()).unwrap();
    wait_for(
        runtime.set_resource_provider(ResourceProvider::new(move |_, handle| {
            let _ = &probe;
            handle.pass_through().unwrap();
        })),
    );

    // Native holds the callback, but not the handle it is registered on, so
    // dropping the handle disposes the runtime, which releases the callback.
    drop(runtime);
    await_release(&released);
}

#[test]
fn a_panicking_void_callback_is_contained_and_reported() {
    assert_panicking_void_callback_is_reported();
}

#[test]
fn a_failing_callback_is_contained_and_its_error_is_reported() {
    let _global = global_state();
    let reports = capture_reports();
    let fixture = Fixture::new();
    let calls = Arc::new(AtomicUsize::new(0));
    let counted = Arc::clone(&calls);
    wait_for(
        fixture
            .runtime()
            .set_resource_provider(ResourceProvider::new(move |_, _| {
                counted.fetch_add(1, Ordering::SeqCst);
                // resume_unwind unwinds like a panic but skips the panic hook,
                // which would print a message on every run.
                std::panic::resume_unwind(Box::new("the provider panicked"));
            })),
    );

    // The binding catches the panic at the callback boundary and releases the
    // request it adopted, which fails it.
    fixture
        .map()
        .discarding()
        .set_style_url("custom://style.json")
        .unwrap();
    fixture.await_event_type(RuntimeEventType::MapLoadingFailed);
    assert!(calls.load(Ordering::SeqCst) > 0);

    // A transform that panics drops its request, which keeps its URL instead
    // of leaving MapLibre waiting.
    let deny = denying_provider().callback.unwrap();
    wait_for(
        fixture
            .runtime()
            .set_resource_provider(ResourceProvider::new(move |request, handle| {
                if request.requested_url == "custom://transformed.json" {
                    return handle.pass_through().unwrap();
                }
                deny(request, handle)
            })),
    );
    wait_for(
        fixture
            .runtime()
            .set_resource_transform(ResourceTransform::new(|_, _, _| {
                std::panic::resume_unwind(Box::new("the transform panicked"));
            })),
    );
    fixture
        .map()
        .discarding()
        .set_style_url("custom://transformed.json")
        .unwrap();
    // The online source cannot load the untransformed URL.
    fixture.await_event_type(RuntimeEventType::MapLoadingFailed);
    wait_for(fixture.runtime().clear_resource_transform());

    // OpenHarmony and Emscripten have no HTTP header transforms.
    #[cfg(not(any(target_env = "ohos", target_os = "emscripten")))]
    {
        // A header transform's error drops its headers. A failure status is
        // deliberate, so native receives it unreported; an error without one is
        // a bug, which the reporter receives.
        let refused = Error::invalid_argument("the transform refused");
        let broken = Error::new(Status::Ok, "the transform failed without a status");
        let deliberate = LoopbackServer::start("/refused.json", BACKGROUND_STYLE_JSON);
        let buggy = LoopbackServer::start("/broken.json", BACKGROUND_STYLE_JSON);
        wait_for(fixture.runtime().clear_resource_provider());
        let (answer_refused, answer_broken) = (refused.clone(), broken.clone());
        wait_for(fixture.runtime().set_http_header_transform(
            HttpHeaderTransform::default().with_callback(move |_, url, _| {
                Err(if url.ends_with("/refused.json") {
                    answer_refused.clone()
                } else {
                    answer_broken.clone()
                })
            }),
        ));
        for url in [deliberate.url("/refused.json"), buggy.url("/broken.json")] {
            fixture.map().discarding().set_style_url(&url).unwrap();
            fixture.await_event_type(RuntimeEventType::MapStyleLoaded);
        }
        set_reporter(None);
        // Other tests may run callbacks concurrently, so only this test's errors
        // count.
        let errors: Vec<_> = reports
            .try_iter()
            .filter_map(|report| match report {
                Report::CallbackError {
                    callback: "mln_http_header_transform_callback",
                    error,
                } if error == refused || error == broken => Some(error),
                _ => None,
            })
            .collect();
        assert!(errors.contains(&broken), "{errors:?}");
        assert!(!errors.contains(&refused), "{errors:?}");

        // The runtime keeps working.
        wait_for(fixture.runtime().clear_http_header_transform());
    }
    #[cfg(any(target_env = "ohos", target_os = "emscripten"))]
    {
        drop(reports);
        set_reporter(None);
    }
    wait_for(fixture.runtime().set_resource_provider(denying_provider()));
    fixture.fence();
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
                if request.requested_url != "custom://style.json" {
                    return deny(request, handle);
                }
                sender.lock().unwrap().send(handle).unwrap();
            })),
    );

    fixture
        .map()
        .discarding()
        .set_style_url("custom://style.json")
        .unwrap();
    let handle = requests.recv_timeout(timeout()).unwrap();
    std::thread::spawn(move || {
        assert!(!handle.is_cancelled().unwrap());
        handle
            .complete(&ok_response(BACKGROUND_STYLE_JSON))
            .unwrap();
        // The accepted completion consumed the handle, so native refuses a
        // second one.
        assert!(handle.is_closed());
        let error = handle
            .complete(&ok_response(BACKGROUND_STYLE_JSON))
            .unwrap_err();
        assert_eq!(error.status(), Status::InvalidState);
    })
    .join()
    .unwrap();
    fixture.await_event_type(RuntimeEventType::MapStyleLoaded);
}

#[test]
fn a_transform_hands_its_request_to_another_thread_and_answers_later() {
    let fixture = Fixture::new();
    let server = LoopbackServer::start("/rewritten.json", BACKGROUND_STYLE_JSON);
    let original = server.url("/original.json");
    let rewritten = server.url("/rewritten.json");
    let (sender, requests) = mpsc::channel();
    let sender = Mutex::new(sender);
    let wanted = original.clone();
    // The transform applies to native loading, which the fixture's provider
    // would otherwise take over.
    wait_for(fixture.runtime().clear_resource_provider());
    wait_for(
        fixture
            .runtime()
            .set_resource_transform(ResourceTransform::new(move |_, url, request| {
                if url == wanted {
                    sender.lock().unwrap().send(request).unwrap();
                }
            })),
    );

    fixture.map().discarding().set_style_url(&original).unwrap();
    let request = requests.recv_timeout(timeout()).unwrap();
    std::thread::spawn(move || {
        request.complete(&rewritten).unwrap();
        // The accepted completion consumed the request, so native refuses a
        // second one.
        assert!(request.is_closed());
        let error = request.complete(&rewritten).unwrap_err();
        assert_eq!(error.status(), Status::InvalidState);
    })
    .join()
    .unwrap();
    fixture.await_event_type(RuntimeEventType::MapStyleLoaded);
    assert_eq!(server.requests(), ["/rewritten.json"]);
}

#[test]
fn a_provider_receives_only_the_requests_its_routes_match() {
    let fixture = Fixture::new();
    let server = LoopbackServer::start("/native/style.json", BACKGROUND_STYLE_JSON);
    let (sender, requests) = mpsc::channel();
    let sender = Mutex::new(sender);
    // The call consumes the provider, its routes, and their strings, so
    // native can only be matching against the copy it took.
    let provider = ResourceProvider {
        routes: vec![
            // Another kind: the style request below matches this pattern, so
            // a kind mask lost on the way to native would claim it.
            ResourceRoute {
                kinds: ResourceKindFlag::TILE,
                flags: UrlMatchFlag::GLOB,
                url: "**".to_owned(),
            },
            // Only a URL pattern: every kind under it reaches the provider.
            ResourceRoute {
                flags: UrlMatchFlag::GLOB,
                url: "custom://routed/**".to_owned(),
                ..Default::default()
            },
        ],
        ..ResourceProvider::new(move |request, handle| {
            let url = request.requested_url;
            let body = match url.as_str() {
                "custom://routed/style.json" => ROUTED_STYLE_JSON,
                _ => r#"{"type":"FeatureCollection","features":[]}"#,
            };
            handle.complete(&ok_response(body)).unwrap();
            sender.lock().unwrap().send((request.kind, url)).unwrap();
        })
    };
    wait_for(fixture.runtime().set_resource_provider(provider));

    // The style and the source it names both match the URL-only route.
    fixture
        .map()
        .discarding()
        .set_style_url("custom://routed/style.json")
        .unwrap();
    fixture.await_event_type(RuntimeEventType::MapStyleLoaded);
    let mut received = vec![
        requests.recv_timeout(timeout()).unwrap(),
        requests.recv_timeout(timeout()).unwrap(),
    ];
    received.sort_by(|left, right| left.1.cmp(&right.1));
    assert_eq!(
        received,
        [
            (
                ResourceKind::Source,
                "custom://routed/source.geojson".to_owned()
            ),
            (ResourceKind::Style, "custom://routed/style.json".to_owned()),
        ]
    );

    // A style that no route claims loads natively, without reaching the
    // provider.
    fixture
        .map()
        .discarding()
        .set_style_url(&server.url("/native/style.json"))
        .unwrap();
    fixture.await_event_type(RuntimeEventType::MapStyleLoaded);
    assert_eq!(server.requests(), ["/native/style.json"]);
    assert!(requests.try_recv().is_err());
}

#[test]
fn replacing_the_log_callback_releases_the_one_it_replaced() {
    let _global = global_state();
    let (first, first_released) = release_probe();
    log_set_callback(LogHandler::new(move |_, _, _, _| {
        let _ = &first;
        true
    }))
    .unwrap();
    assert!(first_released.try_recv().is_err());

    let (second, second_released) = release_probe();
    log_set_callback(LogHandler::new(move |_, _, _, _| {
        let _ = &second;
        true
    }))
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
                if request.requested_url != "custom://style.json" {
                    return deny(request, handle);
                }
                sender.lock().unwrap().send(handle).unwrap();
            })),
    );
    fixture
        .map()
        .discarding()
        .set_style_url("custom://style.json")
        .unwrap();
    let handle = requests.recv_timeout(timeout()).unwrap();

    // Releasing the map discards its pending style request, which MapLibre
    // then cancels.
    let (runtime, map) = fixture.into_parts();
    wait_for(map.release());
    await_condition("the request to report cancelled", || {
        handle.is_cancelled().unwrap()
    });

    // Native reports the cancellation instead of accepting the callback, so
    // the binding drops the callback unrun before returning.
    let (probe, released) = release_probe();
    let already_cancelled = handle
        .set_cancel_callback(ResourceRequestCancelHandler::new(move || {
            let _ = &probe;
            panic!("a cancelled request ran its cancel callback");
        }))
        .unwrap();
    assert!(already_cancelled);
    released
        .try_recv()
        .expect("the binding kept a callback that native never took");
    handle.release().unwrap();
    wait_for(runtime.release());
}
