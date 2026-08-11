//! The runtime loop: runtime and map, owned for their whole lifetime by one
//! spawned thread. The render loop attaches its own session against the map
//! reference published from here.

use std::error::Error;
use std::sync::Arc;
use std::sync::mpsc::{Receiver, Sender};
use std::thread;
use std::time::Duration;

use maplibre_native_ffi::{
    AnimationOptions, CameraOptions, LatLng, MapAttachRef, MapHandle, MapMode, MapOptions,
    RuntimeEventMask, RuntimeEventPayload, RuntimeEventSource, RuntimeEventType, RuntimeHandle,
    RuntimeOptions,
};

use crate::channel::{CameraCommand, Shared};
use crate::viewport::Viewport;

const STYLE_URL: &str = "https://tiles.openfreemap.org/styles/bright";

// TODO(map-example-spec): Replace fixed pacing with a host condition variable
// woken by the render loop.
const LOOP_PERIOD: Duration = Duration::from_millis(4);

/// Backstop for the park; the render loop's wake source normally releases it.
const PARK_TIMEOUT: Duration = Duration::from_millis(100);

/// What the runtime loop hands the render loop once the map exists.
pub struct RuntimeLoopHandles {
    pub attach_ref: MapAttachRef,
    pub wake: Arc<maplibre_native_ffi::WakeSource>,
}

/// Runs the runtime loop until the render loop asks for shutdown. Failures land
/// in [`Shared::fail`] for the render loop to report.
pub fn run(
    viewport: Viewport,
    commands: Receiver<CameraCommand>,
    attach: Sender<RuntimeLoopHandles>,
    shared: Arc<Shared>,
) {
    let mut state = match MapState::new(viewport) {
        Ok(state) => state,
        Err(error) => {
            shared.fail(error.to_string());
            return;
        }
    };

    if let Err(error) = pump(&mut state, &commands, attach, &shared) {
        shared.fail(error.to_string());
    }

    // A map with an attached session cannot be destroyed, so wait for the
    // render loop to close its session and request shutdown.
    while !shared.shutdown_requested() {
        thread::sleep(LOOP_PERIOD);
    }
    if let Err(error) = state.close() {
        shared.fail(error.to_string());
    }
}

fn pump(
    state: &mut MapState,
    commands: &Receiver<CameraCommand>,
    attach: Sender<RuntimeLoopHandles>,
    shared: &Shared,
) -> Result<(), Box<dyn Error>> {
    // The attach reference and wake source are the only handles that leave this
    // thread; every other map call stays here.
    let handles = RuntimeLoopHandles {
        attach_ref: state.map.attach_ref()?,
        wake: Arc::new(state.runtime.wake_source()?),
    };
    if attach.send(handles).is_err() {
        return Ok(());
    }
    drop(attach);

    while !shared.shutdown_requested() {
        state.apply_commands(commands)?;
        // No display paces this thread, so it parks in the pump until the
        // runtime has work or the render loop signals the wake source.
        state.runtime.pump(Some(PARK_TIMEOUT))?;
        if state.drain_events()? {
            shared.request_render();
        }
    }
    Ok(())
}

struct MapState {
    runtime: RuntimeHandle,
    map: MapHandle,
}

impl MapState {
    fn new(viewport: Viewport) -> Result<Self, Box<dyn Error>> {
        let mut runtime_options = RuntimeOptions::default();
        runtime_options.cache_path = Some(":memory:".into());
        let runtime = match RuntimeHandle::with_options(&runtime_options) {
            Ok(runtime) => runtime,
            Err(error) => {
                return Err(startup_error(
                    format!("runtime creation failed: {error}"),
                    None,
                    None,
                ));
            }
        };
        let mut map_options = MapOptions::new(
            viewport.logical_width,
            viewport.logical_height,
            viewport.scale_factor,
        );
        map_options.mode = MapMode::Continuous;
        let map = match MapHandle::with_options(&runtime, &map_options) {
            Ok(map) => map,
            Err(error) => {
                return Err(startup_error(
                    format!("map creation failed: {error}"),
                    None,
                    Some(runtime),
                ));
            }
        };
        if let Err(error) = configure_map(&map) {
            return Err(startup_error(
                format!("map initialization failed: {error}"),
                Some(map),
                Some(runtime),
            ));
        }
        Ok(Self { runtime, map })
    }

    /// Applies every queued camera command on the map's owner thread.
    fn apply_commands(
        &self,
        commands: &Receiver<CameraCommand>,
    ) -> maplibre_native_ffi::Result<()> {
        for command in commands.try_iter() {
            self.apply(command)?;
        }
        Ok(())
    }

    /// Applies one decoded camera command on the map's owner thread, where
    /// read-modify-write commands also read the current camera.
    fn apply(&self, command: CameraCommand) -> maplibre_native_ffi::Result<()> {
        let map = &self.map;
        match command {
            CameraCommand::CancelTransitions => map.cancel_transitions(),
            CameraCommand::SetGestureInProgress { in_progress } => {
                map.set_gesture_in_progress(in_progress)
            }
            CameraCommand::MoveBy { dx, dy } => map.move_by(dx, dy),
            CameraCommand::MoveByAnimated {
                dx,
                dy,
                duration_ms,
            } => map.move_by_animated(dx, dy, Some(&animation(duration_ms))),
            CameraCommand::ScaleBy { scale, anchor } => map.scale_by(scale, Some(anchor)),
            CameraCommand::ScaleByAnimated {
                scale,
                anchor,
                duration_ms,
            } => map.scale_by_animated(scale, Some(anchor), Some(&animation(duration_ms))),
            CameraCommand::PitchBy { delta } => map.pitch_by(delta),
            CameraCommand::AdjustBearing { delta } => {
                map.jump_to(&bearing_camera(self.next_bearing(delta)?))
            }
            CameraCommand::AdjustBearingAnimated { delta, duration_ms } => map.ease_to(
                &bearing_camera(self.next_bearing(delta)?),
                Some(&animation(duration_ms)),
            ),
            CameraCommand::AdjustPitchAnimated { delta, duration_ms } => {
                let pitch = (map.camera()?.pitch.unwrap_or(0.0) + delta).clamp(0.0, 60.0);
                let mut camera = CameraOptions::default();
                camera.pitch = Some(pitch);
                map.ease_to(&camera, Some(&animation(duration_ms)))
            }
            CameraCommand::ResetOrientation { duration_ms } => {
                let mut camera = CameraOptions::default();
                camera.bearing = Some(0.0);
                camera.pitch = Some(0.0);
                map.ease_to(&camera, Some(&animation(duration_ms)))
            }
        }
    }

    fn next_bearing(&self, delta: f64) -> maplibre_native_ffi::Result<f64> {
        Ok(self.map.camera()?.bearing.unwrap_or(0.0) + delta)
    }

    /// Drains one batch of runtime events, reporting whether the map wants
    /// another frame.
    fn drain_events(&mut self) -> maplibre_native_ffi::Result<bool> {
        let source = RuntimeEventSource::Map(self.map.id());
        let mut render_update_available = false;
        // One drain takes every event the pump produced. The batch borrows
        // runtime storage, and this loop keeps nothing from it.
        let batch = self.runtime.drain_events(0)?;
        for event in batch.iter() {
            if event.source() != source {
                continue;
            }
            match event.event_type() {
                RuntimeEventType::MapRenderUpdateAvailable => render_update_available = true,
                RuntimeEventType::MapRenderFrameFinished => {
                    if let RuntimeEventPayload::RenderFrame(frame) = event.payload() {
                        render_update_available |= frame.needs_repaint;
                    }
                }
                _ => {}
            }
        }
        Ok(render_update_available)
    }

    fn close(self) -> Result<(), Box<dyn Error>> {
        let mut first_error = self
            .map
            .close()
            .err()
            .map(|error| format!("map close failed: {error}"));
        if let Err(error) = self.runtime.close() {
            append_error(&mut first_error, format!("runtime close failed: {error}"));
        }
        match first_error {
            Some(error) => Err(error.into()),
            None => Ok(()),
        }
    }
}

fn configure_map(map: &MapHandle) -> maplibre_native_ffi::Result<()> {
    // The two event types the runtime loop reads. A map queues no event of an
    // unselected type, so this runs before the style load.
    map.set_event_mask(
        RuntimeEventMask::MAP_RENDER_UPDATE_AVAILABLE | RuntimeEventMask::MAP_RENDER_FRAME_FINISHED,
    )?;
    map.set_style_url(STYLE_URL)?;
    let mut camera = CameraOptions::default();
    camera.center = Some(LatLng::new(37.7749, -122.4194));
    camera.zoom = Some(13.0);
    camera.bearing = Some(12.0);
    camera.pitch = Some(30.0);
    map.jump_to(&camera)?;
    map.request_repaint()
}

fn animation(duration_ms: f64) -> AnimationOptions {
    let mut animation = AnimationOptions::default();
    animation.duration_ms = Some(duration_ms);
    animation
}

fn bearing_camera(bearing: f64) -> CameraOptions {
    let mut camera = CameraOptions::default();
    camera.bearing = Some(bearing);
    camera
}

fn startup_error(
    mut message: String,
    map: Option<MapHandle>,
    runtime: Option<RuntimeHandle>,
) -> Box<dyn Error> {
    if let Some(map) = map {
        append_cleanup_result(&mut message, "map", map.close());
    }
    if let Some(runtime) = runtime {
        append_cleanup_result(&mut message, "runtime", runtime.close());
    }
    message.into()
}

fn append_cleanup_result<E: std::fmt::Display>(
    message: &mut String,
    resource: &str,
    result: std::result::Result<(), E>,
) {
    if let Err(error) = result {
        message.push_str(&format!("; {resource} cleanup failed: {error}"));
    }
}

fn append_error(message: &mut Option<String>, error: String) {
    match message {
        Some(message) => message.push_str(&format!("; {error}")),
        None => *message = Some(error),
    }
}
