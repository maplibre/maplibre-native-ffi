//! Handle ownership: close, parent retention, discarded creations, and exit.

use std::sync::{Arc, mpsc};

use maplibre_native_ffi::*;

use crate::support::*;

#[test]
fn closing_a_handle_twice_does_nothing() {
    let fixture = Fixture::new();
    let projection = wait_for(fixture.map().projection_create());
    projection.close().unwrap();
    // The second close finds the handle closed and never reaches native.
    projection.close().unwrap();
    assert!(projection.is_closed());
    let error = projection.get_camera().unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    assert_eq!(error.raw_status(), None);

    // A release that completes asynchronously does the same: the second one
    // resolves at once.
    let (runtime, map) = fixture.into_parts();
    wait_for(map.release());
    assert!(map.release().unwrap().is_ready());
    wait_for(runtime.release());
}

#[test]
fn a_refused_close_leaves_the_runtime_usable_and_a_later_close_succeeds() {
    let (runtime, map) = Fixture::new().into_parts();

    // A runtime refuses to close while one of its maps is live.
    let error = runtime.release().unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidState);
    assert!(error.raw_status().is_some());
    assert!(!runtime.is_closed());
    wait_for(runtime.barrier());

    wait_for(map.release());
    wait_for(runtime.release());
    assert!(runtime.is_closed());
}

#[test]
fn a_map_keeps_its_runtime_alive_after_the_runtime_handle_drops() {
    let _global = global_state();
    let (sender, leaks) = mpsc::channel();
    set_leak_reporter(Some(Box::new(move |leak| {
        let _ = sender.send(leak);
    })));

    let (runtime, map) = Fixture::new().into_parts();
    drop(runtime);
    // The map holds its runtime, so the runtime's worker still runs commands.
    let loaded = wait_for(map.set_style_json(BACKGROUND_STYLE_JSON.as_bytes()));
    assert_eq!(loaded.disposition, CommandDisposition::Committed);
    // Releasing the map drops the last hold on the runtime, which the
    // binding then disposes on this thread without a leak.
    wait_for(map.release());
    drop(map);

    set_leak_reporter(None);
    let leaked: Vec<_> = leaks.try_iter().collect();
    assert!(leaked.is_empty(), "{leaked:?}");
}

#[test]
fn a_dropped_map_creation_future_retires_the_map() {
    let fixture = Fixture::new();
    let created = fixture
        .runtime()
        .map_create(&MapOptions::default())
        .unwrap();
    // The barrier fences the creation, so the map exists when its future
    // drops, and dropping the future is what retires it.
    fixture.barrier();
    drop(created);

    // A runtime closes only once all of its maps have retired.
    let (runtime, map) = fixture.into_parts();
    wait_for(map.release());
    wait_for(runtime.release());
}

/// Set in the child process that the exit test runs.
const EXIT_CHILD: &str = "MLN_FFI_RUST_EXIT_CHILD";

#[test]
fn a_process_that_exits_with_live_handles_and_callbacks_exits_cleanly() {
    if std::env::var_os(EXIT_CHILD).is_some() {
        exit_with_live_handles_and_callbacks();
    }
    // The test runs itself again in a child process, which exits while its
    // handles, callbacks, and native threads are all still live.
    let status = std::process::Command::new(std::env::current_exe().unwrap())
        .args([
            "lifecycle::a_process_that_exits_with_live_handles_and_callbacks_exits_cleanly",
            "--exact",
            "--test-threads=1",
        ])
        .env(EXIT_CHILD, "1")
        .status()
        .unwrap();
    assert!(status.success(), "the child process ended with {status}");
}

fn exit_with_live_handles_and_callbacks() -> ! {
    let fixture = Fixture::new();
    fixture.load_style(BACKGROUND_STYLE_JSON);
    log_set_callback(Some(Arc::new(|_, _, _, _| 1))).unwrap();
    let projection = wait_for(fixture.map().projection_create());
    std::mem::forget((fixture, projection));
    std::process::exit(0);
}
