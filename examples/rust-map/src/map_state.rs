//! The runtime loop: runtime and map, owned for their whole lifetime by one
//! spawned thread.
//!
//! The render target is not here. It belongs to the render loop thread, which
//! owns the window and the graphics context and attaches its own session
//! against the map published from this thread.

use std::error::Error;
use std::sync::Arc;
use std::sync::mpsc::{Receiver, Sender};
use std::thread;
use std::time::Duration;

use maplibre_native::{
    AnimationOptions, CameraOptions, LatLng, MapAttachRef, MapHandle, MapMode, MapOptions,
    RuntimeEventPayload, RuntimeEventSource, RuntimeEventType, RuntimeHandle, RuntimeOptions,
};

use crate::channel::{CameraCommand, Shared};
use crate::viewport::Viewport;

const STYLE_URL: &str = "https://tiles.openfreemap.org/styles/bright";

// TODO(map-example-spec): Replace fixed pacing with a host condition variable
// woken by the render loop. See Cadence.
const LOOP_PERIOD: Duration = Duration::from_millis(4);

/// Backstop for the runtime loop's park. The render loop's wake source is what
/// normally releases it, so this only bounds a pump that nothing signals.
const PARK_TIMEOUT: Duration = Duration::from_millis(100);

/// What the runtime loop hands the render loop once the map exists: the map
/// reference to attach against, and the wake source that releases its park.
pub struct RuntimeLoopHandles {
    pub attach_ref: MapAttachRef,
    pub wake: Arc<maplibre_native::WakeSource>,
}

/// Runs the runtime loop until the render loop asks for shutdown.
///
/// Failures land in [`Shared::fail`] rather than propagating, because this is a
/// thread body; the render loop reports them.
pub fn run(
    viewport: Viewport,
    commands: Receiver<CameraCommand>,
    attach: Sender<RuntimeLoopHandles>,
    shared: Arc<Shared>,
) {
    let state = match MapState::new(viewport) {
        Ok(state) => state,
        Err(error) => {
            shared.fail(error.to_string());
            return;
        }
    };

    // `attach` is consumed here, so the render loop stops waiting for the map
    // as soon as this loop stops publishing one.
    if let Err(error) = pump(&state, &commands, attach, &shared) {
        shared.fail(error.to_string());
    }

    // The render loop closes its session before it requests shutdown, and a map
    // with an attached session cannot be destroyed, so wait for that signal.
    while !shared.shutdown_requested() {
        thread::sleep(LOOP_PERIOD);
    }
    if let Err(error) = state.close() {
        shared.fail(error.to_string());
    }
}

fn pump(
    state: &MapState,
    commands: &Receiver<CameraCommand>,
    attach: Sender<RuntimeLoopHandles>,
    shared: &Shared,
) -> Result<(), Box<dyn Error>> {
    // Publishing the attach reference is what lets the render loop create and
    // own its session, and the wake source is how it releases the park below.
    // Every other map call stays on this thread.
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
        // This thread has no display to pace it, so it takes its cadence from
        // the runtime's own work and parks in between. The render loop signals
        // the wake source, so the bound is a backstop rather than the cadence.
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
    fn apply_commands(&self, commands: &Receiver<CameraCommand>) -> maplibre_native::Result<()> {
        for command in commands.try_iter() {
            self.apply(command)?;
        }
        Ok(())
    }

    /// Applies one decoded camera command. Runs on the map's owner thread,
    /// which is why the read-modify-write commands read the current camera here
    /// rather than on the render loop that produced them.
    fn apply(&self, command: CameraCommand) -> maplibre_native::Result<()> {
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

    fn next_bearing(&self, delta: f64) -> maplibre_native::Result<f64> {
        Ok(self.map.camera()?.bearing.unwrap_or(0.0) + delta)
    }

    /// Drains runtime events, reporting whether the map wants another frame.
    fn drain_events(&self) -> maplibre_native::Result<bool> {
        let source = RuntimeEventSource::Map(self.map.id());
        let mut render_update_available = false;
        while let Some(event) = self.runtime.poll_event()? {
            if event.source != source {
                continue;
            }
            match event.event_type {
                RuntimeEventType::MapRenderUpdateAvailable => render_update_available = true,
                RuntimeEventType::MapRenderFrameFinished => {
                    if let RuntimeEventPayload::RenderFrame(frame) = event.payload {
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

fn configure_map(map: &MapHandle) -> maplibre_native::Result<()> {
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
