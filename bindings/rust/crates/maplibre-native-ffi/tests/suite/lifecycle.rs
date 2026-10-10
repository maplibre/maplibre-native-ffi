//! Handle ownership: the ABI check at creation, close, parent retention,
//! discarded creations, and exit.

use std::sync::mpsc;

use maplibre_native_ffi::*;

use crate::support::*;

#[test]
fn creation_checks_the_c_abi_version_before_it_reaches_native() {
    // Each entry point that creates a root handle checks the version first,
    // so a mismatch fails without a native status.
    use maplibre_native_ffi_core::{EXPECTED_C_ABI_VERSION, set_abi_version_override};
    set_abi_version_override(Some(EXPECTED_C_ABI_VERSION + 1));
    let runtime = runtime_create(&RuntimeOptions::default());
    let geojson =
        geojson_source_data_create(br#"{"type":"FeatureCollection","features":[]}"#, None);
    set_abi_version_override(None);

    for error in [runtime.unwrap_err(), geojson.unwrap_err()] {
        assert_eq!(error.kind(), ErrorKind::AbiVersionMismatch);
        assert_eq!(error.raw_status(), None);
        assert!(
            error
                .diagnostic()
                .contains("unsupported MapLibre Native C ABI version")
        );
    }
}

#[test]
fn closing_a_handle_twice_does_nothing() {
    let fixture = Fixture::new();
    let projection = wait_for(fixture.map().projection_create());
    projection.close().unwrap();
    // The second close finds the handle closed and never reaches native.
    projection.close().unwrap();
    assert!(projection.is_closed());
    let error = projection.get_camera().unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidState);
    assert_eq!(error.raw_status(), None);
    assert_eq!(error.diagnostic(), "MapProjectionHandle is closed");

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
    let reports = capture_reports();

    let (runtime, map) = Fixture::new().into_parts();
    drop(runtime);
    // The map holds its runtime, so the runtime's worker still runs commands.
    let loaded = wait_for(map.set_style_json(BACKGROUND_STYLE_JSON.as_bytes()));
    assert_eq!(loaded.disposition, CommandDisposition::Committed);
    // Releasing the map drops the last hold on the runtime, which the
    // binding then disposes on this thread without a leak.
    wait_for(map.release());
    drop(map);

    set_reporter(None);
    let leaked = leaks(&reports);
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
    let mut child = std::process::Command::new(std::env::current_exe().unwrap())
        .args([
            "lifecycle::a_process_that_exits_with_live_handles_and_callbacks_exits_cleanly",
            "--exact",
            "--test-threads=1",
        ])
        .env(EXIT_CHILD, "1")
        .spawn()
        .unwrap();
    // A hang at exit is the regression this test catches, so the wait is
    // bounded. The waiter thread reaps the child, so its id stays valid for
    // the kill until the wait returns.
    let id = child.id();
    let (sender, exited) = mpsc::channel();
    std::thread::spawn(move || sender.send(child.wait()));
    let Ok(status) = exited.recv_timeout(timeout()) else {
        kill(id);
        panic!("the child process did not exit within {:?}", timeout());
    };
    let status = status.unwrap();
    assert!(status.success(), "the child process ended with {status}");
}

/// Kills the process with this id, which the caller has not reaped.
fn kill(id: u32) {
    let id = id.to_string();
    let mut command = if cfg!(windows) {
        let mut command = std::process::Command::new("taskkill");
        command.args(["/F", "/PID", &id]);
        command
    } else {
        let mut command = std::process::Command::new("kill");
        command.args(["-KILL", &id]);
        command
    };
    let _ = command.status();
}

fn exit_with_live_handles_and_callbacks() -> ! {
    let fixture = Fixture::new();
    fixture.load_style(BACKGROUND_STYLE_JSON);
    log_set_callback(LogHandler::new(|_, _, _, _| 1)).unwrap();
    let projection = wait_for(fixture.map().projection_create());
    std::mem::forget((fixture, projection));
    std::process::exit(0);
}
