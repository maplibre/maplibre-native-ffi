//! The runtime and map, driven by the native scheduler thread the runtime
//! owns.

use std::error::Error;
use std::time::Duration;

use maplibre_native_ffi::{
    AnimationOptions, CameraDelta, CameraOptions, CameraUpdate, CameraUpdateMode, GesturePhase,
    LatLng, LogicalExtent, MapHandle, MapMode, MapOptions, RuntimeEventMask, RuntimeEventType,
    RuntimeHandle, RuntimeOptions, ScreenPoint,
};

use crate::shell::{AppEvent, Wakes};
use crate::viewport::Viewport;

const STYLE_URL: &str = "https://tiles.openfreemap.org/styles/bright";
/// The style a smoke test renders, which needs no network.
const SMOKE_STYLE_JSON: &str = r##"{"version":8,"sources":{},"layers":[{"id":"background","type":"background","paint":{"background-color":"#d8f1ff"}}]}"##;

pub struct MapState {
    map: MapHandle,
    runtime: RuntimeHandle,
}

impl MapState {
    /// Creates the runtime, whose event wake posts runtime-event drains to the
    /// event loop, and the map.
    pub fn new(viewport: Viewport, wakes: &Wakes) -> Result<Self, Box<dyn Error>> {
        let runtime_options = RuntimeOptions {
            cache_path: Some(":memory:".into()),
            event_wake: wakes.wake(AppEvent::RuntimeEvents),
            ..Default::default()
        };
        let runtime = maplibre_native_ffi::runtime_create(&runtime_options)
            .map_err(|error| format!("runtime creation failed: {error}"))?;

        let map_options = MapOptions {
            initial_extent: LogicalExtent::new(
                viewport.logical_width,
                viewport.logical_height,
                viewport.scale_factor,
            ),
            map_mode: MapMode::Continuous,
            ..MapOptions::default()
        };
        let map = match runtime.map_create(&map_options).and_then(|future| {
            if !future.wait(Duration::from_secs(30))? {
                return Err(maplibre_native_ffi::Error::new(
                    maplibre_native_ffi::ErrorKind::NotReady,
                    None,
                    "map creation timed out",
                ));
            }
            future.take()
        }) {
            Ok(map) => map,
            Err(error) => {
                let mut message = format!("map creation failed: {error}");
                append_cleanup_result(&mut message, "runtime", close_runtime(runtime));
                return Err(message.into());
            }
        };
        let mut state = Self { map, runtime };
        if let Err(error) = state.configure() {
            let mut message = format!("map initialization failed: {error}");
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

    /// Carries a new logical extent to the map on the paths where the attached
    /// session cannot: a caller-owned texture the host sizes. Target
    /// replacement changes only the graphics resource.
    pub fn resize(&self, viewport: Viewport) -> Result<(), Box<dyn Error>> {
        self.map.resize(LogicalExtent {
            width: viewport.logical_width,
            height: viewport.logical_height,
            scale_factor: viewport.scale_factor,
        })?;
        Ok(())
    }

    /// Ends any running camera transition, so a starting gesture takes over
    /// from it rather than fighting it.
    pub fn cancel_transitions(&self) -> Result<(), Box<dyn Error>> {
        self.map.cancel_transitions()?;
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
        self.map.update_camera(&update)?;
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
        self.map.apply_camera_delta(&delta)?;
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
        self.map.apply_camera_delta(&delta)?;
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
        self.map.apply_camera_delta(&delta)?;
        Ok(())
    }

    pub fn reset_orientation(&self, duration_ms: f64) -> Result<(), Box<dyn Error>> {
        let mut update = CameraUpdate::default();
        update.camera.bearing = Some(0.0);
        update.camera.pitch = Some(0.0);
        update.mode = CameraUpdateMode::Ease;
        update.animation = animation(duration_ms);
        self.map.update_camera(&update)?;
        Ok(())
    }

    /// Drains every queued runtime event and reports whether the map published
    /// a render update.
    pub fn drain_events(&self) -> maplibre_native_ffi::Result<bool> {
        let source = self.map.id();
        Ok(self
            .runtime
            .drain_events()?
            .get()?
            .events
            .iter()
            .any(|event| {
                event.source == source && event.r#type == RuntimeEventType::MapRenderUpdateAvailable
            }))
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
        // The render loop re-arms from the frame result's repaint flag, so the
        // map only has to report updates that arrive between frames.
        self.map
            .set_event_mask(RuntimeEventMask::MAP_RENDER_UPDATE_AVAILABLE)?;
        if crate::smoke_test() {
            self.map.set_style_json(SMOKE_STYLE_JSON.as_bytes())?;
        } else {
            self.map.set_style_url(STYLE_URL)?;
        }
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
        self.map.update_camera(&update)?;
        self.map.request_repaint()?;
        Ok(())
    }
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
        Ok(true) => teardown.take().map_err(|error| error.to_string()),
        Ok(false) => Err("map teardown timed out".to_owned()),
        Err(error) => Err(error.to_string()),
    }
}

/// Closes a runtime and waits for native teardown, so MapLibre's threads stop
/// before the app tears down state that the callbacks use.
fn close_runtime(runtime: RuntimeHandle) -> std::result::Result<(), String> {
    let teardown = runtime.release().map_err(|error| error.to_string())?;
    match teardown.wait(Duration::from_secs(30)) {
        Ok(true) => teardown.take().map_err(|error| error.to_string()),
        Ok(false) => Err("runtime teardown timed out".to_owned()),
        Err(error) => Err(error.to_string()),
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
