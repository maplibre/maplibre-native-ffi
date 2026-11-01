//! Snippets for the docs site's Events and logs page. They compile with the
//! tests, which never run them.

#![allow(dead_code)]

use std::sync::mpsc;

use maplibre_native_ffi::*;

fn on_style_loaded() {}

fn on_loading_failed(_message: &str) {}

fn write_log(_severity: LogSeverity, _message: &str) {}

async fn events_and_logs(runtime: &RuntimeHandle) -> Result<()> {
    // #region handler
    // #region queue
    // The handler runs on a library thread. It sends the events to your thread.
    let (sender, batches) = mpsc::channel();
    runtime
        .discarding()
        .set_event_handler(RuntimeEventHandler::new(move |events| {
            let _ = sender.send(events);
        }))?;
    // #endregion queue
    // #endregion handler

    // #region mask
    let map_options = MapOptions {
        event_mask: Some(RuntimeEventFlag::MAP_STYLE_LOADED | RuntimeEventFlag::MAP_LOADING_FAILED),
        ..MapOptions::new(LogicalExtent::new(512, 512, 1.0))
    };
    let (map, created) = runtime.create_map(&map_options)?;
    // #endregion mask
    created.await?;

    // #region handler
    // #region queue
    // On your thread, such as on each turn of your event loop:
    for event in batches.try_iter().flatten() {
        if event.source != map.id() {
            continue;
        }

        match event.r#type {
            RuntimeEventType::MapStyleLoaded => on_style_loaded(),
            RuntimeEventType::MapLoadingFailed => on_loading_failed(&event.message),
            _ => {}
        }
    }
    // #endregion queue
    // #endregion handler

    // #region logs
    // The callback runs on a library thread, and can't call the library.
    set_log_callback(LogHandler::new(|severity, _event, _code, message| {
        write_log(severity, message);
        // Consume the record. Return false to let the platform logger print it too.
        true
    }))?;
    // #endregion logs

    Ok(())
}

fn main() {}
