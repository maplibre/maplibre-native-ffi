//! Callback registrations: nested handlers, contained panics, request
//! ownership, and provider routes.

use std::sync::atomic::{AtomicUsize, Ordering};
use std::sync::{Arc, Mutex, mpsc};

use maplibre_native_ffi::*;

use crate::support::*;

/// A style whose GeoJSON source loads from a URL beside it.
const ROUTED_STYLE_JSON: &str = r#"{"version":8,"sources":{"points":{"type":"geojson","data":"custom://routed/source.geojson"}},"layers":[]}"#;

#[test]
fn a_camera_end_handler_inside_a_command_runs_once_when_its_transition_ends() {
    let fixture = Fixture::new();

    // The handler sits two records deep in the command's input, which the
    // host drops once the call returns. Native holds the handler while the
    // ease runs, and runs it once when its identity is cancelled.
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
                ended.send(end).unwrap();
            }),
            ..Default::default()
        },
        ..Default::default()
    };
    wait_for(fixture.map().update_camera(&update));
    drop(update);
    wait_for(fixture.map().cancel_camera_transitions_with_id(u64::MAX));
    let end = outcomes.recv_timeout(timeout()).unwrap();
    assert_eq!(end.outcome, CameraTransitionOutcome::Cancelled);
    assert_ne!(end.generation, 0);
    assert!(outcomes.try_recv().is_err());
}

#[test]
fn a_panicking_void_callback_is_contained_and_reported() {
    assert_panicking_void_callback_is_reported();
}

#[test]
#[cfg_attr(
    target_os = "android",
    ignore = "Android's HTTP stack calls Java, which test executables run without"
)]
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

    // A transform that panics drops its request, which fails it instead of
    // leaving MapLibre waiting.
    let deny = denying_provider().callback;
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
    let deny = denying_provider().callback;
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
    })
    .join()
    .unwrap();
    fixture.await_event_type(RuntimeEventType::MapStyleLoaded);
}

#[test]
#[cfg_attr(
    target_os = "android",
    ignore = "Android's HTTP stack calls Java, which test executables run without"
)]
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
    })
    .join()
    .unwrap();
    fixture.await_event_type(RuntimeEventType::MapStyleLoaded);
    assert_eq!(server.requests(), ["/rewritten.json"]);
}

#[test]
#[cfg_attr(
    target_os = "android",
    ignore = "Android's HTTP stack calls Java, which test executables run without"
)]
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
