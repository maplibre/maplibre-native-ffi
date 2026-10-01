//! The winit event loop. Native wakes reach it as user events, and the app
//! does the work they schedule on this thread.

use std::error::Error;
use std::sync::{Arc, Condvar, Mutex};
use std::time::{Duration, Instant};

use maplibre_native_ffi::Wake;
use winit::application::ApplicationHandler;
use winit::event::{StartCause, WindowEvent};
use winit::event_loop::{ActiveEventLoop, ControlFlow, EventLoop, EventLoopProxy};
use winit::window::{Window, WindowAttributes, WindowId};

use crate::app::App;
use crate::graphics::GraphicsContext;
use crate::render_target::Mode;

const INITIAL_WIDTH: u32 = 960;
const INITIAL_HEIGHT: u32 = 640;
/// How long a smoke test waits for its first rendered frame before it fails.
const SMOKE_DEADLINE: Duration = Duration::from_secs(60);

/// The work a native wake asks the event loop to do.
#[derive(Clone, Copy, Debug)]
pub enum AppEvent {
    /// The runtime event queue has events to drain.
    RuntimeEvents,
    /// The render session has frame results to drain.
    FrameResults,
    /// The render session has caller-driver work to service.
    DriverWork,
}

/// Builds wakes that post an [`AppEvent`] to the event loop from any native
/// thread.
#[derive(Clone)]
pub struct Wakes {
    proxy: EventLoopProxy<AppEvent>,
    driver_wait: DriverWait,
}

impl Wakes {
    pub fn wake(&self, event: AppEvent) -> Wake {
        let proxy = self.proxy.clone();
        let driver_wait = self.driver_wait.clone();
        Wake::new(move || {
            // The send fails only once the loop has exited, when nothing is
            // left to wake.
            let _ = proxy.send_event(event);
            if matches!(event, AppEvent::DriverWork) {
                driver_wait.signal();
            }
        })
    }

    pub fn driver_wait(&self) -> DriverWait {
        self.driver_wait.clone()
    }
}

/// Startup and shutdown block on a session's lifecycle completion outside the
/// event loop. Every driver wake also signals this wait, so it services driver
/// work only when there is some.
#[derive(Clone, Default)]
pub struct DriverWait(Arc<(Mutex<bool>, Condvar)>);

impl DriverWait {
    fn signal(&self) {
        let (signaled, condvar) = &*self.0;
        *signaled
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner()) = true;
        condvar.notify_all();
    }

    pub fn clear(&self) {
        *self
            .0
            .0
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner()) = false;
    }

    pub fn wait(&self) {
        let (signaled, condvar) = &*self.0;
        let guard = signaled
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        let _signaled = condvar
            .wait_while(guard, |signaled| !*signaled)
            .unwrap_or_else(|poisoned| poisoned.into_inner());
    }
}

pub fn run(
    mode: Mode,
    backends: maplibre_native_ffi::RenderBackendFlag,
) -> Result<(), Box<dyn Error>> {
    let event_loop = EventLoop::<AppEvent>::with_user_event().build()?;
    let mut shell = Shell {
        mode,
        backends,
        wakes: Wakes {
            proxy: event_loop.create_proxy(),
            driver_wait: DriverWait::default(),
        },
        app: None,
        error: None,
        smoke_deadline: crate::smoke_test().then(|| Instant::now() + SMOKE_DEADLINE),
    };
    let run_result = event_loop.run_app(&mut shell);
    if let Some(error) = shell.error {
        return Err(error);
    }
    run_result.map_err(Into::into)
}

struct Shell {
    mode: Mode,
    backends: maplibre_native_ffi::RenderBackendFlag,
    wakes: Wakes,
    app: Option<App>,
    error: Option<Box<dyn Error>>,
    smoke_deadline: Option<Instant>,
}

impl Shell {
    fn startup(&mut self, event_loop: &ActiveEventLoop) -> Result<(), Box<dyn Error>> {
        let (window, graphics) =
            GraphicsContext::create_window(event_loop, window_attributes(), self.backends)?;
        let app = App::new(window, graphics, self.mode, &self.wakes)?;
        app.print_status();
        self.app = Some(app);
        Ok(())
    }

    fn exit(&mut self, event_loop: &ActiveEventLoop) {
        if let Some(app) = self.app.as_mut() {
            app.close_or_abort();
        }
        event_loop.exit();
    }
}

impl ApplicationHandler<AppEvent> for Shell {
    fn new_events(&mut self, _event_loop: &ActiveEventLoop, cause: StartCause) {
        if let (StartCause::ResumeTimeReached { .. }, Some(app)) = (cause, self.app.as_mut()) {
            app.retry_if_due(Instant::now());
        }
    }

    fn resumed(&mut self, event_loop: &ActiveEventLoop) {
        if self.app.is_some() || self.error.is_some() {
            return;
        }
        if let Err(error) = self.startup(event_loop) {
            self.error = Some(error);
            event_loop.exit();
        }
    }

    fn user_event(&mut self, _event_loop: &ActiveEventLoop, event: AppEvent) {
        if let Some(app) = self.app.as_mut() {
            app.handle_app_event(event);
        }
    }

    fn window_event(
        &mut self,
        event_loop: &ActiveEventLoop,
        window_id: WindowId,
        event: WindowEvent,
    ) {
        let Some(app) = self.app.as_mut() else {
            return;
        };
        if app.window_id() != window_id {
            return;
        }
        if matches!(event, WindowEvent::CloseRequested) {
            self.exit(event_loop);
            return;
        }
        app.handle_window_event(event);
    }

    fn about_to_wait(&mut self, event_loop: &ActiveEventLoop) {
        let Some(app) = self.app.as_ref() else {
            return;
        };
        if app.smoke_rendered() {
            self.exit(event_loop);
            return;
        }
        if self
            .smoke_deadline
            .is_some_and(|deadline| Instant::now() >= deadline)
        {
            self.error = Some(
                format!(
                    "smoke: no frame rendered within {}s",
                    SMOKE_DEADLINE.as_secs()
                )
                .into(),
            );
            self.exit(event_loop);
            return;
        }
        // The loop sleeps until a native wake or window event arrives, or
        // until a paced retry or the smoke deadline comes due.
        let deadline = [app.retry_at(), self.smoke_deadline]
            .into_iter()
            .flatten()
            .min();
        event_loop.set_control_flow(match deadline {
            Some(deadline) => ControlFlow::WaitUntil(deadline),
            None => ControlFlow::Wait,
        });
    }

    fn exiting(&mut self, _event_loop: &ActiveEventLoop) {
        if let Some(app) = self.app.as_mut() {
            app.close_or_abort();
        }
    }
}

fn window_attributes() -> WindowAttributes {
    Window::default_attributes()
        .with_title("MapLibre Rust Map")
        .with_inner_size(winit::dpi::LogicalSize::new(INITIAL_WIDTH, INITIAL_HEIGHT))
        .with_resizable(true)
}
