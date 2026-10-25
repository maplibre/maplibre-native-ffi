//! The runtime and map, driven by the native scheduler thread the runtime
//! owns.

use std::error::Error;
use std::time::Duration;

use maplibre_native_ffi::{
    AnimationOptions, CameraDelta, CameraOptions, CameraUpdate, CameraUpdateMode, GesturePhase,
    LatLng, MapHandle, MapMode, MapOptions, NativeFuture, RuntimeEventFlag, RuntimeHandle,
    RuntimeOptions, ScreenPoint,
};

use crate::render_target::extent;
use crate::viewport::Viewport;

const STYLE_URL: &str = "https://tiles.openfreemap.org/styles/bright";
/// The style a smoke test renders, which needs no network.
const SMOKE_STYLE_JSON: &str = r##"{"version":8,"sources":{},"layers":[{"id":"background","type":"background","paint":{"background-color":"#d8f1ff"}}]}"##;

pub struct MapState {
    map: MapHandle,
    runtime: RuntimeHandle,
}

impl MapState {
    /// Creates the runtime and map.
    pub fn new(viewport: Viewport) -> Result<Self, Box<dyn Error>> {
        let runtime_options = RuntimeOptions {
            cache_path: Some(":memory:".into()),
            ..Default::default()
        };
        let runtime = maplibre_native_ffi::runtime_create(&runtime_options)
            .map_err(|error| format!("runtime creation failed: {error}"))?;

        let map_options = MapOptions {
            initial_extent: extent(viewport),
            map_mode: MapMode::Continuous,
            event_mask: RuntimeEventFlag::empty(),
            ..MapOptions::default()
        };
        let (map, ready) = match runtime.create_map(&map_options) {
            Ok(created) => created,
            Err(error) => {
                let mut message = format!("map creation failed: {error}");
                append_cleanup_result(&mut message, "runtime", close_runtime(runtime));
                return Err(message.into());
            }
        };
        let ready = wait(ready).map_err(|error| format!("map creation failed: {error}"));
        // The map is ours from the call, so a failed creation still closes it.
        let mut state = Self { map, runtime };
        if let Err(mut message) = ready.and_then(|()| {
            state
                .configure()
                .map_err(|error| format!("map initialization failed: {error}"))
        }) {
            if let Err(error) = state.close() {
                append_error(&mut message, error.to_string());
            }
            return Err(message.into());
        }
        Ok(state)
    }

    pub fn map_handle(&self) -> &MapHandle {
        &self.map
    }

    /// Ends any running camera transition, so a starting gesture takes over
    /// from it rather than fighting it.
    pub fn cancel_transitions(&self) -> Result<(), Box<dyn Error>> {
        self.map.discarding().cancel_transitions()?;
        Ok(())
    }

    pub fn set_gesture_in_progress(&mut self, in_progress: bool) -> Result<(), Box<dyn Error>> {
        let update = CameraUpdate {
            gesture_phase: if in_progress {
                GesturePhase::Begin
            } else {
                GesturePhase::End
            },
            ..Default::default()
        };
        self.map.discarding().update_camera(&update)?;
        Ok(())
    }

    pub fn move_by(
        &self,
        dx: f64,
        dy: f64,
        duration_ms: Option<f64>,
    ) -> Result<(), Box<dyn Error>> {
        let delta = CameraDelta {
            offset: Some(ScreenPoint::new(dx, dy)),
            animation: duration_ms.map(animation).unwrap_or_default(),
            ..Default::default()
        };
        self.map.discarding().apply_camera_delta(&delta)?;
        Ok(())
    }

    pub fn scale_by(
        &self,
        scale: f64,
        anchor: ScreenPoint,
        duration_ms: Option<f64>,
    ) -> Result<(), Box<dyn Error>> {
        let delta = CameraDelta {
            scale: Some(scale),
            anchor: Some(anchor),
            animation: duration_ms.map(animation).unwrap_or_default(),
            ..Default::default()
        };
        self.map.discarding().apply_camera_delta(&delta)?;
        Ok(())
    }

    /// Adds bearing and pitch degrees in one delta. `None` leaves that
    /// component unchanged.
    pub fn adjust_orientation(
        &self,
        bearing: Option<f64>,
        pitch: Option<f64>,
        duration_ms: Option<f64>,
    ) -> Result<(), Box<dyn Error>> {
        let delta = CameraDelta {
            bearing,
            pitch,
            animation: duration_ms.map(animation).unwrap_or_default(),
            ..Default::default()
        };
        self.map.discarding().apply_camera_delta(&delta)?;
        Ok(())
    }

    pub fn reset_orientation(&self, duration_ms: f64) -> Result<(), Box<dyn Error>> {
        let mut update = CameraUpdate::default();
        update.camera.bearing = Some(0.0);
        update.camera.pitch = Some(0.0);
        update.mode = CameraUpdateMode::Ease;
        update.animation = animation(duration_ms);
        self.map.discarding().update_camera(&update)?;
        Ok(())
    }

    pub fn close(self) -> Result<(), Box<dyn Error>> {
        let Self { map, runtime, .. } = self;
        let mut first_error = None;
        if let Err(error) = close_map(map) {
            append_optional_error(&mut first_error, format!("map close failed: {error}"));
        }
        if let Err(error) = close_runtime(runtime) {
            append_optional_error(&mut first_error, format!("runtime close failed: {error}"));
        }
        match first_error {
            Some(error) => Err(error.into()),
            None => Ok(()),
        }
    }

    fn configure(&mut self) -> Result<(), Box<dyn Error>> {
        // A style that fails to apply leaves nothing to render, so its command
        // is awaited. The commands after it are fire-and-forget, and native
        // logs any that fails.
        let style = if crate::smoke_test() {
            self.map.set_style_json(SMOKE_STYLE_JSON.as_bytes())
        } else {
            self.map.set_style_url(STYLE_URL)
        };
        style
            .and_then(wait)
            .map_err(|error| format!("style load failed: {error}"))?;
        let camera = CameraOptions {
            center: Some(LatLng::new(37.7749, -122.4194)),
            zoom: Some(13.0),
            bearing: Some(12.0),
            pitch: Some(30.0),
            ..Default::default()
        };
        let update = CameraUpdate {
            camera,
            ..Default::default()
        };
        self.map.discarding().update_camera(&update)?;
        self.map.discarding().request_repaint()?;
        Ok(())
    }
}

/// Waits for a startup operation, which the runtime's own thread completes.
fn wait<T>(operation: NativeFuture<T>) -> maplibre_native_ffi::Result<T> {
    operation.wait(Duration::from_secs(30)).unwrap_or_else(|_| {
        Err(maplibre_native_ffi::Error::new(
            maplibre_native_ffi::Status::NotReady,
            "timed out",
        ))
    })
}

fn animation(duration_ms: f64) -> AnimationOptions {
    AnimationOptions {
        duration_ms: Some(duration_ms),
        ..Default::default()
    }
}

/// Closes a map and waits for retirement, so its render resources are gone
/// before the runtime that owns its worker closes.
fn close_map(map: MapHandle) -> std::result::Result<(), String> {
    let teardown = map.release().map_err(|error| error.to_string())?;
    match teardown.wait(Duration::from_secs(30)) {
        Ok(result) => result.map_err(|error| error.to_string()),
        Err(_) => Err("map teardown timed out".to_owned()),
    }
}

/// Closes a runtime and waits for native teardown, so MapLibre's threads stop
/// before the app tears down state that the callbacks use.
fn close_runtime(runtime: RuntimeHandle) -> std::result::Result<(), String> {
    let teardown = runtime.release().map_err(|error| error.to_string())?;
    match teardown.wait(Duration::from_secs(30)) {
        Ok(result) => result.map_err(|error| error.to_string()),
        Err(_) => Err("runtime teardown timed out".to_owned()),
    }
}

fn append_cleanup_result<E: std::fmt::Display>(
    message: &mut String,
    resource: &str,
    result: std::result::Result<(), E>,
) {
    if let Err(error) = result {
        append_error(message, format!("{resource} cleanup failed: {error}"));
    }
}

fn append_optional_error(message: &mut Option<String>, error: String) {
    match message {
        Some(message) => append_error(message, error),
        None => *message = Some(error),
    }
}

fn append_error(message: &mut String, error: String) {
    message.push_str("; ");
    message.push_str(&error);
}
