//! Handle ownership: the ABI check at creation, close, parent retention,
//! discarded creations, and exit.

use std::sync::mpsc;

use maplibre_native_ffi::*;

use crate::support::*;

#[test]
fn releasing_a_handle_twice_reaches_native() {
    let (runtime, map) = Fixture::new().into_parts();
    wait_for(map.release());
    assert!(map.is_closed());
    assert_eq!(map.release().unwrap_err().status(), Status::InvalidState);
    wait_for(runtime.release());
}

#[test]
fn a_disposed_handle_is_closed_and_a_second_dispose_does_nothing() {
    let (runtime, map) = Fixture::new().into_parts();
    assert!(!map.is_closed());
    assert_eq!(
        format!("{map:?}"),
        format!("MapHandle(id=0x{:x}, closed=false)", map.id())
    );

    map.dispose().unwrap();
    assert!(map.is_closed());
    map.dispose().unwrap();
    assert_eq!(
        format!("{map:?}"),
        format!("MapHandle(id=0x{:x}, closed=true)", map.id())
    );
    // An asynchronous release still reaches native, which reports the stale
    // handle.
    assert_eq!(map.release().unwrap_err().status(), Status::InvalidState);

    // A runtime accepts its dispose while its disposed map retires.
    runtime.dispose().unwrap();
    assert!(runtime.is_closed());
    runtime.dispose().unwrap();
}

#[test]
fn a_released_projection_is_closed_and_a_second_release_does_nothing() {
    let fixture = Fixture::new();
    let projection = map_projection_create(&fixture.map().get_snapshot().unwrap().view).unwrap();
    assert!(!projection.is_closed());
    assert_ne!(projection.id(), 0);

    projection.release().unwrap();
    assert!(projection.is_closed());
    projection.release().unwrap();
    assert_eq!(
        format!("{projection:?}"),
        format!(
            "MapProjectionHandle(id=0x{:x}, closed=true)",
            projection.id()
        )
    );
}

#[test]
fn a_handle_id_names_its_events_and_outlives_close() {
    let fixture = Fixture::new();
    let loaded = fixture
        .map()
        .set_style_json(BACKGROUND_STYLE_JSON.as_bytes());
    let event = fixture.await_event_type(RuntimeEventType::MapStyleLoaded);
    wait_for(loaded);
    assert_eq!(event.source, fixture.map().id());
    let (runtime, map) = fixture.into_parts();
    wait_for(map.release());
    // A host routes events that it drains after close by the id it kept.
    assert_eq!(map.id(), event.source);
    wait_for(runtime.release());
}

#[test]
fn a_refused_close_leaves_the_runtime_usable_and_a_later_close_succeeds() {
    let (runtime, map) = Fixture::new().into_parts();

    // A runtime refuses to close while one of its maps is live.
    let error = runtime.release().unwrap_err();
    assert_eq!(error.status(), Status::InvalidState);
    wait_for(runtime.clear_resource_transform());

    wait_for(map.release());
    wait_for(runtime.release());
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
        .create_map(&MapOptions::default())
        .unwrap();
    // A later query fences the creation, so the map exists when its future
    // drops, and dropping the future is what retires it.
    fixture.fence();
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
    let projection = map_projection_create(&fixture.map().get_snapshot().unwrap().view).unwrap();
    std::mem::forget((fixture, projection));
    std::process::exit(0);
}
