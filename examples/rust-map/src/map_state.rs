//! Autonomous runtime and any-thread map state.

use std::error::Error;
use std::time::Duration;

use maplibre_native_ffi::{
    AnimationOptions, CameraDelta, CameraDeltaKind, CameraOptions, CameraUpdate, CameraUpdateMode,
    GesturePhase, LatLng, LogicalExtent, MapHandle, MapMode, MapOptions, RuntimeEventMask,
    RuntimeEventPayload, RuntimeEventSource, RuntimeEventType, RuntimeHandle, RuntimeOptions,
    ScreenPoint,
};

use crate::viewport::Viewport;

const STYLE_URL: &str = "https://tiles.openfreemap.org/styles/bright";

pub struct MapState {
    map: MapHandle,
    runtime: RuntimeHandle,
}

impl MapState {
    pub fn new(viewport: Viewport) -> Result<Self, Box<dyn Error>> {
        let mut runtime_options = RuntimeOptions::default();
        runtime_options.cache_path = Some(":memory:".into());
        let runtime = RuntimeHandle::with_options(&runtime_options)
            .map_err(|error| format!("runtime creation failed: {error}"))?;

        let mut map_options = MapOptions::new(
            viewport.logical_width,
            viewport.logical_height,
            viewport.scale_factor,
        );
        map_options.mode = MapMode::Continuous;
        let map = match MapHandle::with_options(&runtime, &map_options).and_then(|future| {
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

    pub fn set_gesture_in_progress(&mut self, in_progress: bool) -> Result<(), Box<dyn Error>> {
        let mut update = CameraUpdate::default();
        update.gesture_phase = if in_progress {
            GesturePhase::Begin
        } else {
            GesturePhase::End
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
        let mut delta = CameraDelta::default();
        delta.offset = ScreenPoint::new(dx, dy);
        delta.animation = duration_ms.map(animation).unwrap_or_default();
        self.map.apply_camera_delta(&delta)?;
        Ok(())
    }

    pub fn scale_by(
        &self,
        scale: f64,
        anchor: ScreenPoint,
        duration_ms: Option<f64>,
    ) -> Result<(), Box<dyn Error>> {
        let mut delta = CameraDelta::default();
        delta.kind = CameraDeltaKind::Scale;
        delta.amount = scale;
        delta.anchor = Some(anchor);
        delta.animation = duration_ms.map(animation).unwrap_or_default();
        self.map.apply_camera_delta(&delta)?;
        Ok(())
    }

    pub fn adjust_pitch(&self, delta: f64, duration_ms: Option<f64>) -> Result<(), Box<dyn Error>> {
        let mut camera_delta = CameraDelta::default();
        camera_delta.kind = CameraDeltaKind::Pitch;
        camera_delta.amount = delta;
        camera_delta.animation = duration_ms.map(animation).unwrap_or_default();
        self.map.apply_camera_delta(&camera_delta)?;
        Ok(())
    }

    pub fn adjust_bearing(
        &self,
        delta: f64,
        duration_ms: Option<f64>,
    ) -> Result<(), Box<dyn Error>> {
        let mut camera_delta = CameraDelta::default();
        camera_delta.kind = CameraDeltaKind::Bearing;
        camera_delta.amount = delta;
        camera_delta.animation = duration_ms.map(animation).unwrap_or_default();
        self.map.apply_camera_delta(&camera_delta)?;
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

    pub fn resize(&mut self, viewport: Viewport) -> Result<(), Box<dyn Error>> {
        self.map.resize(LogicalExtent {
            width: viewport.logical_width,
            height: viewport.logical_height,
            scale_factor: viewport.scale_factor,
        })?;
        Ok(())
    }

    pub fn drain_events(&self) -> maplibre_native_ffi::Result<bool> {
        let mut render_requested = false;
        let source = RuntimeEventSource::Map(self.map.id());
        let batch = self.runtime.drain_events()?;
        for event in batch.iter().filter(|event| event.source() == source) {
            match event.event_type() {
                RuntimeEventType::MapRenderUpdateAvailable => render_requested = true,
                RuntimeEventType::MapRenderFrameFinished => {
                    if let RuntimeEventPayload::RenderFrame(frame) = event.payload() {
                        render_requested |= frame.needs_repaint;
                    }
                }
                _ => {}
            }
        }
        Ok(render_requested)
    }

    pub fn close(self) -> Result<(), Box<dyn Error>> {
        let Self { map, runtime, .. } = self;
        let mut first_error = None;
        if let Err(error) = map.close() {
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
        self.map.set_event_mask(
            RuntimeEventMask::MAP_RENDER_UPDATE_AVAILABLE
                | RuntimeEventMask::MAP_RENDER_FRAME_FINISHED,
        )?;
        self.map.set_style_url(STYLE_URL)?;
        let mut camera = CameraOptions::default();
        camera.center = Some(LatLng::new(37.7749, -122.4194));
        camera.zoom = Some(13.0);
        camera.bearing = Some(12.0);
        camera.pitch = Some(30.0);
        let mut update = CameraUpdate::default();
        update.camera = camera;
        self.map.update_camera(&update)?;
        self.map.request_repaint()?;
        Ok(())
    }
}

fn animation(duration_ms: f64) -> AnimationOptions {
    let mut animation = AnimationOptions::default();
    animation.duration_ms = Some(duration_ms);
    animation
}

/// Closes a runtime and waits for native teardown, so the process exits after
/// MapLibre's threads and resources are gone rather than racing them.
fn close_runtime(runtime: RuntimeHandle) -> std::result::Result<(), String> {
    let teardown = runtime
        .close()
        .map_err(|error| error.into_error().to_string())?;
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
