//! The cross-thread surface between the render loop, which owns the window and
//! the render session, and the runtime loop, which owns the runtime and map.

use std::sync::Mutex;
use std::sync::atomic::{AtomicBool, Ordering};

use maplibre_native_ffi::ScreenPoint;

/// A camera change decoded on the render loop and applied on the map's owner
/// thread. Commands carry deltas wherever the current camera is an input,
/// because the read and write have to happen together on the owner thread.
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

/// Render request, shutdown, and first failure, shared by both loops.
#[derive(Debug)]
pub struct Shared {
    render_request: AtomicBool,
    shutdown: AtomicBool,
    failure: Mutex<Option<String>>,
}

impl Shared {
    pub fn new() -> Self {
        Self {
            render_request: AtomicBool::new(true),
            shutdown: AtomicBool::new(false),
            failure: Mutex::new(None),
        }
    }

    /// Announces that a frame is worth drawing.
    pub fn request_render(&self) {
        self.render_request.store(true, Ordering::Release);
    }

    /// Render loop: takes the pending request, leaving it clear.
    pub fn consume_render_request(&self) -> bool {
        self.render_request.swap(false, Ordering::AcqRel)
    }

    /// Render loop: asks the runtime loop to destroy the map and stop. Call
    /// only after the render session is closed; a map with an attached session
    /// cannot be destroyed.
    pub fn request_shutdown(&self) {
        self.shutdown.store(true, Ordering::Release);
    }

    pub fn shutdown_requested(&self) -> bool {
        self.shutdown.load(Ordering::Acquire)
    }

    /// Records the first failure seen by either loop.
    pub fn fail(&self, message: String) {
        let mut failure = self.lock_failure();
        if failure.is_none() {
            *failure = Some(message);
        }
    }

    pub fn failure(&self) -> Option<String> {
        self.lock_failure().clone()
    }

    /// Recovers a poisoned lock: the stored message stays readable, and the
    /// other loop's panic surfaces through the thread join.
    fn lock_failure(&self) -> std::sync::MutexGuard<'_, Option<String>> {
        self.failure
            .lock()
            .unwrap_or_else(|error| error.into_inner())
    }
}
