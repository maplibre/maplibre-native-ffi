//! The winit event loop owns the window, graphics context, and render session.
//! Input submits camera commands, and native wakes reach the app as user
//! events: a runtime event drain demands a frame for each map update, and a
//! frame-result drain shows what rendered. A caller-driver session also gets
//! driver wakes, which service it.

use std::error::Error;
use std::time::{Duration, Instant};

use winit::event::WindowEvent;
use winit::window::{Window, WindowId};

use crate::graphics::GraphicsContext;
use crate::input::Controller;
use crate::map_state::MapState;
use crate::render_target::{Mode, RenderTarget, driver_label};
use crate::shell::{AppEvent, Wakes};
use crate::viewport::Viewport;

/// How long a frame that did not reach the window waits before it retries,
/// about one display refresh.
const FRAME_RETRY: Duration = Duration::from_millis(16);

pub struct App {
    target: Option<RenderTarget>,
    map: Option<MapState>,
    graphics: GraphicsContext,
    wakes: Wakes,
    window: Window,
    viewport: Viewport,
    input: Controller,
    closed: bool,
    mode: Mode,
    /// When a frame that did not reach the window demands its retry.
    retry_at: Option<Instant>,
    /// Set once a smoke test has rendered its frame.
    smoke_rendered: bool,
}

impl App {
    pub fn new(
        window: Window,
        graphics: GraphicsContext,
        mode: Mode,
        wakes: &Wakes,
    ) -> Result<Self, Box<dyn Error>> {
        let viewport = Viewport::from_window(&window);
        if viewport.is_empty() {
            return Err("window has no drawable extent".into());
        }
        viewport.log("initial viewport");

        let map = MapState::new(viewport, wakes)?;
        let mut target =
            match RenderTarget::attach(mode, map.map_handle(), &graphics, viewport, wakes) {
                Ok(target) => target,
                Err(error) => {
                    let mut message = format!("render target attachment failed: {error}");
                    if let Err(error) = map.close() {
                        message.push_str(&format!("; map state cleanup failed: {error}"));
                    }
                    return Err(message.into());
                }
            };
        // Updates the map published before attachment have no event left to
        // demand their frame.
        target.session_mut().request_frame(false)?;

        Ok(Self {
            target: Some(target),
            map: Some(map),
            graphics,
            wakes: wakes.clone(),
            window,
            viewport,
            input: Controller::default(),
            closed: false,
            mode,
            retry_at: None,
            smoke_rendered: false,
        })
    }

    pub fn print_status(&mut self) {
        let driver = self.target_mut().session_mut().driver();
        println!("render target: {}", self.mode.cli_name());
        println!("render target status: {}", self.mode.status());
        println!("render driver: {}", driver_label(driver));
        Controller::print_controls();
    }

    /// Whether a smoke test has rendered its frame and the app can exit.
    pub fn smoke_rendered(&self) -> bool {
        self.smoke_rendered
    }

    pub fn window_id(&self) -> WindowId {
        self.window.id()
    }

    pub fn retry_at(&self) -> Option<Instant> {
        self.retry_at
    }

    pub fn handle_window_event(&mut self, event: WindowEvent) {
        if self.closed {
            return;
        }
        let result = match event {
            WindowEvent::Resized(_) | WindowEvent::ScaleFactorChanged { .. } => self.resize(),
            event => {
                let map = self.map.as_mut().expect("map is open");
                self.input.handle(&event, self.viewport, map)
            }
        };
        if let Err(error) = result {
            eprintln!("window event failed: {error}");
            self.abort_process(1);
        }
    }

    pub fn handle_app_event(&mut self, event: AppEvent) {
        if self.closed || self.smoke_rendered {
            return;
        }
        let result = match event {
            AppEvent::RuntimeEvents => self.drain_events(),
            AppEvent::DriverWork => self
                .target_mut()
                .session_mut()
                .service()
                .map_err(Into::into),
            AppEvent::TargetReplaced => {
                let target = self.target.as_mut().expect("render target is open");
                target
                    .show_replacements(&self.graphics, &self.wakes)
                    .map_err(Into::into)
            }
            AppEvent::FrameResults => self.show_frame_results(),
        };
        if let Err(error) = result {
            eprintln!("{event:?} failed: {error}");
            self.abort_process(1);
        }
    }

    /// Demands the retry of a frame that did not reach the window, once due.
    pub fn retry_if_due(&mut self, now: Instant) {
        if self.closed || self.retry_at.is_none_or(|retry_at| retry_at > now) {
            return;
        }
        self.retry_at = None;
        // The map update was consumed without reaching the window, so the
        // retry forces a frame rather than waiting for another update.
        if let Err(error) = self
            .target_mut()
            .session_mut()
            .request_frame(true)
            .map(drop)
        {
            eprintln!("frame retry failed: {error}");
            self.abort_process(1);
        }
    }

    fn target_mut(&mut self) -> &mut RenderTarget {
        self.target.as_mut().expect("render target is open")
    }

    fn drain_events(&mut self) -> Result<(), Box<dyn Error>> {
        if self.map.as_ref().expect("map is open").drain_events()? {
            self.target_mut().session_mut().request_frame(false)?;
        }
        Ok(())
    }

    fn show_frame_results(&mut self) -> Result<(), Box<dyn Error>> {
        let target = self.target.as_mut().expect("render target is open");
        let results = target.session_mut().drain_results()?;
        let presented = results.rendered && target.present(&self.graphics, &self.wakes)?;
        if presented && crate::smoke_test() {
            println!("smoke: rendered a frame");
            self.smoke_rendered = true;
            return Ok(());
        }
        if results.target_not_ready || (results.rendered && !presented) {
            // The map update was consumed without reaching the window, so the
            // retry forces a frame rather than waiting for another update.
            self.retry_at = Some(Instant::now() + FRAME_RETRY);
        } else if results.needs_repaint {
            target.session_mut().request_frame(false)?;
        }
        if results.any {
            target.session_mut().compositor_done()?;
        }
        Ok(())
    }

    fn resize(&mut self) -> Result<(), Box<dyn Error>> {
        let next = Viewport::from_window(&self.window);
        if next == self.viewport {
            return Ok(());
        }
        next.log("resized viewport");
        self.viewport = next;
        if next.is_empty() {
            return Ok(());
        }
        // The attached session's resize is the extent authority, except on the
        // paths that hand over a graphics resource instead; the render target
        // resizes the map itself there. A later resize supersedes an earlier
        // one that has not applied yet.
        let target = self.target.as_mut().expect("render target is open");
        target.resize(
            &self.graphics,
            self.map.as_ref().expect("map is open"),
            next,
            &self.wakes,
        )
    }

    pub fn close_or_abort(&mut self) {
        if let Err(error) = self.close_resources() {
            eprintln!("shutdown failed: {error}");
            self.abort_process(1);
        }
    }

    fn close_resources(&mut self) -> Result<(), Box<dyn Error>> {
        if self.closed {
            return Ok(());
        }
        self.closed = true;
        self.retry_at = None;

        let mut first_error = None;
        if let Some(target) = self.target.take()
            && let Err(error) = target.close(&self.graphics)
        {
            append_error(&mut first_error, error.to_string());
        }
        // Once the session detached, nothing else submits graphics work.
        if let Err(error) = self.graphics.wait_idle() {
            append_error(
                &mut first_error,
                format!(
                    "{} device wait idle failed: {error}",
                    self.graphics.backend_name()
                ),
            );
        }
        if let Some(map) = self.map.take()
            && let Err(error) = map.close()
        {
            append_error(&mut first_error, error.to_string());
        }

        match first_error {
            Some(error) => Err(error.into()),
            None => Ok(()),
        }
    }

    /// Exits at once after an error, skipping the ordered shutdown. A
    /// core-worker session keeps making graphics calls on its own thread, so
    /// the session is abandoned before the process exits.
    fn abort_process(&mut self, code: i32) -> ! {
        self.closed = true;
        if let Some(target) = self.target.as_mut() {
            target.session_mut().abandon();
        }
        immediate_exit(code);
    }
}

fn append_error(message: &mut Option<String>, error: String) {
    match message {
        Some(message) => {
            message.push_str("; ");
            message.push_str(&error);
        }
        None => *message = Some(error),
    }
}

fn immediate_exit(code: i32) -> ! {
    unsafe extern "C" {
        fn _exit(status: std::ffi::c_int) -> !;
    }

    // SAFETY: `_exit` terminates without running native teardown, which the
    // macOS Vulkan stack can abort during after the window has closed. The
    // operating system reclaims the example's resources.
    unsafe { _exit(code) }
}
