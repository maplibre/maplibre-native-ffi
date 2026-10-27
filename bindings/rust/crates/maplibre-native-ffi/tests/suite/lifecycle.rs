//! Handle ownership: the ABI check at creation, close, drop order, and exit.

use std::sync::mpsc;

use maplibre_native_ffi::*;

use crate::support::*;

#[test]
fn a_handle_formats_its_id_and_dispose_consumes_it() {
    let (runtime, map) = Fixture::new().into_parts();
    assert_eq!(
        format!("{map:?}"),
        format!("MapHandle(id=0x{:x})", map.id())
    );

    map.dispose().unwrap();
    // A runtime accepts its dispose while its disposed map retires.
    runtime.dispose().unwrap();
}

#[test]
fn a_projection_release_consumes_it() {
    let fixture = Fixture::new();
    let projection =
        MapProjectionHandle::create(&fixture.map().get_snapshot().unwrap().view).unwrap();
    assert_ne!(projection.id(), 0);
    projection.release().unwrap();
}

#[test]
fn a_handle_id_names_its_events() {
    let fixture = Fixture::new();
    let loaded = fixture.map().set_style_json(BACKGROUND_STYLE_JSON);
    let event = fixture.await_event_type(RuntimeEventType::MapStyleLoaded);
    wait_for(loaded);
    assert_eq!(event.source, fixture.map().id());
    // A host routes events that it drains after close by the id it kept.
    assert_eq!(fixture.map().id(), event.source);
}

#[test]
fn a_refused_close_leaves_the_runtime_usable_and_a_later_close_succeeds() {
    let (runtime, map) = Fixture::new().into_parts();

    // A runtime refuses to close while one of its maps is live, and hands
    // itself back.
    let (runtime, error) = runtime.release().unwrap_err();
    assert_eq!(error.status(), Status::InvalidState);
    wait_for(runtime.clear_resource_transform());

    wait_for(map.release());
    wait_for(runtime.release());
}

#[test]
fn dropping_a_runtime_before_its_map_disposes_it_and_the_map_still_releases() {
    let _global = global_state();
    let reports = capture_reports();

    let (runtime, map) = Fixture::new().into_parts();
    // A runtime owner holds no reference to its maps, so its drop disposes it
    // at once, and native retires it once the map is gone.
    drop(runtime);
    wait_for(map.release());

    set_reporter(None);
    let leaked = leaks(&reports);
    assert!(leaked.is_empty(), "{leaked:?}");
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
    set_log_callback(LogHandler::new(|_, _, _, _| true)).unwrap();
    let projection =
        MapProjectionHandle::create(&fixture.map().get_snapshot().unwrap().view).unwrap();
    std::mem::forget((fixture, projection));
    std::process::exit(0);
}
