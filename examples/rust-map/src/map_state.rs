//! Autonomous runtime and any-thread map state.

use std::error::Error;
use std::time::Duration;

use winit::event_loop::EventLoopProxy;

use maplibre_native_ffi::{
    AnimationOptions, CameraOptions, CameraUpdate, CameraUpdateMode, GesturePhase, LatLng,
    LogicalExtent, MapHandle, MapMode, MapOptions, MapProjectionHandle, ReadyEndpointKind,
    RuntimeEventMask, RuntimeEventPayload, RuntimeEventSource, RuntimeEventType, RuntimeHandle,
    RuntimeOptions, ScreenPoint,
};

use crate::viewport::Viewport;

const STYLE_URL: &str = "https://tiles.openfreemap.org/styles/bright";
const OPERATION_TIMEOUT: Duration = Duration::from_secs(5);
const MINIMUM_PITCH: f64 = 0.0;
const MAXIMUM_PITCH: f64 = 60.0;

#[derive(Clone, Copy, Debug)]
pub enum CameraCommand {
    CancelTransitions,
    SetGestureInProgress {
        in_progress: bool,
    },
    MoveBy {
        dx: f64,
        dy: f64,
    },
    MoveByAnimated {
        dx: f64,
        dy: f64,
        duration_ms: f64,
    },
    ScaleBy {
        scale: f64,
        anchor: ScreenPoint,
    },
    ScaleByAnimated {
        scale: f64,
        anchor: ScreenPoint,
        duration_ms: f64,
    },
    PitchBy {
        delta: f64,
    },
    AdjustBearing {
        delta: f64,
    },
    AdjustBearingAnimated {
        delta: f64,
        duration_ms: f64,
    },
    AdjustPitchAnimated {
        delta: f64,
        duration_ms: f64,
    },
    ResetOrientation {
        duration_ms: f64,
    },
}

pub struct MapState {
    projection: Option<MapProjectionHandle>,
    map: MapHandle,
    runtime: RuntimeHandle,
    gesture_id: u64,
}

impl MapState {
    pub fn new(
        viewport: Viewport,
        event_loop_proxy: EventLoopProxy<()>,
    ) -> Result<Self, Box<dyn Error>> {
        let mut runtime_options = RuntimeOptions::default();
        runtime_options.cache_path = Some(":memory:".into());
        let runtime = RuntimeHandle::with_options(&runtime_options)
            .map_err(|error| format!("runtime creation failed: {error}"))?;

        if let Err(error) = runtime.set_notification_callback(move || {
            let _ = event_loop_proxy.send_event(());
        }) {
            let mut message = format!("notification callback installation failed: {error}");
            append_cleanup_result(&mut message, "runtime", runtime.close());
            return Err(message.into());
        }

        let mut map_options = MapOptions::new(
            viewport.logical_width,
            viewport.logical_height,
            viewport.scale_factor,
        );
        map_options.mode = MapMode::Continuous;
        let map = match MapHandle::with_options(&runtime, &map_options) {
            Ok(map) => map,
            Err(error) => {
                let mut message = format!("map creation failed: {error}");
                append_cleanup_result(
                    &mut message,
                    "notification callback",
                    runtime.clear_notification_callback(),
                );
                append_cleanup_result(&mut message, "runtime", runtime.close());
                return Err(message.into());
            }
        };
        let projection = match map.create_projection() {
            Ok(projection) => projection,
            Err(error) => {
                return Err(startup_error(
                    format!("projection creation failed: {error}"),
                    None,
                    Some(map),
                    runtime,
                ));
            }
        };

        let mut state = Self {
            projection: Some(projection),
            map,
            runtime,
            gesture_id: 0,
        };
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

    pub fn apply(&mut self, command: CameraCommand) -> Result<(), Box<dyn Error>> {
        let current = self.ordered_camera()?;
        let mut update = CameraUpdate::default();
        match command {
            CameraCommand::CancelTransitions => update.camera = current,
            CameraCommand::SetGestureInProgress { in_progress } => {
                if in_progress {
                    self.gesture_id = self.gesture_id.wrapping_add(1).max(1);
                }
                update.camera = current;
                update.gesture_phase = if in_progress {
                    GesturePhase::Begin
                } else {
                    GesturePhase::End
                };
                update.gesture_id = self.gesture_id;
            }
            CameraCommand::MoveBy { dx, dy } => {
                update.camera = self.moved_camera(current, dx, dy)?;
            }
            CameraCommand::MoveByAnimated {
                dx,
                dy,
                duration_ms,
            } => {
                update.mode = CameraUpdateMode::Ease;
                update.camera = self.moved_camera(current, dx, dy)?;
                update.animation = animation(duration_ms);
            }
            CameraCommand::ScaleBy { scale, anchor } => {
                update.camera.zoom = Some(current.zoom.unwrap_or(0.0) + scale.log2());
                update.camera.anchor = Some(anchor);
            }
            CameraCommand::ScaleByAnimated {
                scale,
                anchor,
                duration_ms,
            } => {
                update.mode = CameraUpdateMode::Ease;
                update.camera.zoom = Some(current.zoom.unwrap_or(0.0) + scale.log2());
                update.camera.anchor = Some(anchor);
                update.animation = animation(duration_ms);
            }
            CameraCommand::PitchBy { delta } => {
                update.camera.pitch = Some(
                    (current.pitch.unwrap_or(0.0) + delta).clamp(MINIMUM_PITCH, MAXIMUM_PITCH),
                );
            }
            CameraCommand::AdjustBearing { delta } => {
                update.camera.bearing = Some(current.bearing.unwrap_or(0.0) + delta);
            }
            CameraCommand::AdjustBearingAnimated { delta, duration_ms } => {
                update.mode = CameraUpdateMode::Ease;
                update.camera.bearing = Some(current.bearing.unwrap_or(0.0) + delta);
                update.animation = animation(duration_ms);
            }
            CameraCommand::AdjustPitchAnimated { delta, duration_ms } => {
                update.mode = CameraUpdateMode::Ease;
                update.camera.pitch = Some(
                    (current.pitch.unwrap_or(0.0) + delta).clamp(MINIMUM_PITCH, MAXIMUM_PITCH),
                );
                update.animation = animation(duration_ms);
            }
            CameraCommand::ResetOrientation { duration_ms } => {
                update.mode = CameraUpdateMode::Ease;
                update.camera.bearing = Some(0.0);
                update.camera.pitch = Some(0.0);
                update.animation = animation(duration_ms);
            }
        }
        self.map.update_camera(&update)?;
        Ok(())
    }

    pub fn resize(&mut self, viewport: Viewport) -> Result<(), Box<dyn Error>> {
        self.map.resize(LogicalExtent {
            width: viewport.logical_width,
            height: viewport.logical_height,
            scale_factor: viewport.scale_factor,
        })?;
        self.wait_for_commands()?;
        if let Some(projection) = self.projection.take() {
            projection
                .close()
                .map_err(|error| format!("projection replacement close failed: {error}"))?;
        }
        self.projection = Some(self.map.create_projection()?);
        Ok(())
    }

    pub fn drain_notifications(&self) -> maplibre_native_ffi::Result<bool> {
        let ready = self.runtime.drain_ready()?;
        let mut render_requested = ready.iter().any(|endpoint| {
            matches!(
                endpoint.kind,
                ReadyEndpointKind::RenderFrames | ReadyEndpointKind::DriverWork
            )
        });
        if !ready
            .iter()
            .any(|endpoint| endpoint.kind == ReadyEndpointKind::RuntimeEvents)
        {
            return Ok(render_requested);
        }

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
        let Self {
            projection,
            map,
            runtime,
            ..
        } = self;
        let mut first_error = projection
            .and_then(|projection| projection.close().err())
            .map(|error| format!("projection close failed: {error}"));
        if let Err(error) = map.close() {
            append_optional_error(&mut first_error, format!("map close failed: {error}"));
        }
        if let Err(error) = runtime.clear_notification_callback() {
            append_optional_error(
                &mut first_error,
                format!("notification callback clear failed: {error}"),
            );
        }
        if let Err(error) = runtime.close() {
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
        self.wait_for_commands()
    }

    fn ordered_camera(&self) -> Result<CameraOptions, Box<dyn Error>> {
        self.wait_for_commands()?;
        Ok(self.map.camera_snapshot()?.camera)
    }

    fn wait_for_commands(&self) -> Result<(), Box<dyn Error>> {
        let barrier = self.runtime.start_barrier()?;
        if !barrier.wait(OPERATION_TIMEOUT)? {
            return Err("runtime barrier timed out".into());
        }
        barrier.discard()?;
        Ok(())
    }

    fn moved_camera(
        &self,
        current: CameraOptions,
        dx: f64,
        dy: f64,
    ) -> maplibre_native_ffi::Result<CameraOptions> {
        let projection = self.projection.as_ref().expect("projection is open");
        projection.set_camera(&current)?;
        let center = current.center.unwrap_or_else(|| LatLng::new(0.0, 0.0));
        let point = projection.pixel_for_lat_lng(center)?;
        let mut camera = CameraOptions::default();
        camera.center =
            Some(projection.lat_lng_for_pixel(ScreenPoint::new(point.x + dx, point.y + dy))?);
        Ok(camera)
    }
}

fn animation(duration_ms: f64) -> AnimationOptions {
    let mut animation = AnimationOptions::default();
    animation.duration_ms = Some(duration_ms);
    animation
}

fn startup_error(
    mut message: String,
    projection: Option<MapProjectionHandle>,
    map: Option<MapHandle>,
    runtime: RuntimeHandle,
) -> Box<dyn Error> {
    if let Some(projection) = projection {
        append_cleanup_result(&mut message, "projection", projection.close());
    }
    if let Some(map) = map {
        append_cleanup_result(&mut message, "map", map.close());
    }
    append_cleanup_result(
        &mut message,
        "notification callback",
        runtime.clear_notification_callback(),
    );
    append_cleanup_result(&mut message, "runtime", runtime.close());
    message.into()
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
