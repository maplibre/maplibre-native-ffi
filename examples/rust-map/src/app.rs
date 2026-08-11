//! The render loop: window, input decoding, graphics context, and the render
//! session, all owned by the winit event-loop thread. The runtime and the map
//! live on the spawned runtime loop thread, reached through [`crate::channel`].

use std::error::Error;
use std::sync::Arc;
use std::sync::mpsc::{self, Sender};
use std::thread::{self, JoinHandle};

use winit::event::WindowEvent;
use winit::window::{Window, WindowId};

use crate::channel::{CameraCommand, Shared};
use crate::graphics::GraphicsContext;
use crate::input::Controller;
use crate::map_state;
use crate::render_target::{Mode, RenderTarget};
use crate::viewport::Viewport;

pub struct App {
    target: Option<RenderTarget>,
    /// Releases the runtime loop's parked pump so queued work is applied now.
    wake: Arc<maplibre_native_ffi::WakeSource>,
    runtime_thread: Option<JoinHandle<()>>,
    commands: Sender<CameraCommand>,
    shared: Arc<Shared>,
    graphics: GraphicsContext,
    window: Window,
    viewport: Viewport,
    input: Controller,
    viewport_dirty: bool,
    closed: bool,
    mode: Mode,
}

impl App {
    pub fn new(
        window: Window,
        graphics: GraphicsContext,
        mode: Mode,
    ) -> Result<Self, Box<dyn Error>> {
        let viewport = Viewport::from_window(&window);
        if viewport.is_empty() {
            return Err("window has no drawable extent".into());
        }
        viewport.log("initial viewport");

        let shared = Arc::new(Shared::new());
        let (commands, command_queue) = mpsc::channel();
        let (attach_sender, attach_queue) = mpsc::channel();
        let runtime_thread = {
            let shared = Arc::clone(&shared);
            thread::Builder::new()
                .name("maplibre-runtime".into())
                .spawn(move || map_state::run(viewport, command_queue, attach_sender, shared))?
        };

        let handles = match attach_queue.recv() {
            Ok(handles) => handles,
            Err(_) => {
                return Err(stop_runtime_loop(
                    &shared,
                    runtime_thread,
                    "the runtime loop stopped before it published a map".to_string(),
                ));
            }
        };
        let target = match RenderTarget::attach(mode, &handles.attach_ref, &graphics, viewport) {
            Ok(target) => target,
            Err(error) => {
                return Err(stop_runtime_loop(
                    &shared,
                    runtime_thread,
                    format!("render target attachment failed: {error}"),
                ));
            }
        };

        Ok(Self {
            target: Some(target),
            wake: handles.wake,
            runtime_thread: Some(runtime_thread),
            commands,
            shared,
            graphics,
            window,
            viewport,
            input: Controller::default(),
            viewport_dirty: false,
            closed: false,
            mode,
        })
    }

    pub fn print_status(&self) {
        println!("render target: {}", self.mode.cli_name());
        println!("render target status: {}", self.mode.status());
        Controller::print_controls();
    }

    pub fn window_id(&self) -> WindowId {
        self.window.id()
    }

    pub fn handle_window_event(&mut self, event: WindowEvent) {
        if self.closed {
            return;
        }

        match event {
            WindowEvent::Resized(_) | WindowEvent::ScaleFactorChanged { .. } => {
                self.viewport_dirty = true;
            }
            WindowEvent::RedrawRequested => self.render_or_exit(),
            event => {
                if self.input.handle(&event, &self.commands, self.viewport) {
                    // Release the parked pump so the queued command is applied
                    // on this frame.
                    let _ = self.wake.signal();
                    self.shared.request_render();
                    self.window.request_redraw();
                }
            }
        }
    }

    /// One render loop iteration. The runtime loop owns `pump` and the event
    /// drain.
    pub fn step(&mut self) {
        if let Some(error) = self.shared.failure() {
            eprintln!("runtime loop failed: {error}");
            self.abort_process(1);
        }
        if let Err(error) = self.apply_pending_resize() {
            eprintln!("resize failed: {error}");
            self.abort_process(1);
        }
        self.render_or_exit();
    }

    fn apply_pending_resize(&mut self) -> Result<(), Box<dyn Error>> {
        if self.closed || !self.viewport_dirty {
            return Ok(());
        }
        self.viewport_dirty = false;
        let next = Viewport::from_window(&self.window);
        if next == self.viewport {
            return Ok(());
        }
        next.log("resized viewport");
        self.viewport = next;
        if next.is_empty() {
            return Ok(());
        }
        self.graphics.resize(next)?;
        self.target
            .as_mut()
            .expect("render target is open")
            .resize(&self.graphics, next)?;
        self.shared.request_render();
        Ok(())
    }

    fn render_or_exit(&mut self) {
        if let Err(error) = self.render() {
            eprintln!("render failed: {error}");
            self.abort_process(1);
        }
    }

    fn render(&mut self) -> Result<(), Box<dyn Error>> {
        if self.closed || self.viewport.is_empty() {
            return Ok(());
        }
        // Consume first, so a request published during the render survives.
        if !self.shared.consume_render_request() {
            return Ok(());
        }
        if !self
            .target
            .as_mut()
            .expect("render target is open")
            .render_update(&self.graphics)?
        {
            // Nothing reached the screen: the map applies a new logical size on
            // the runtime loop's next pump, so retry.
            self.shared.request_render();
        }
        Ok(())
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
        self.viewport_dirty = false;

        let mut first_error = self.graphics.wait_idle().err().map(|error| {
            format!(
                "{} device wait idle failed: {error}",
                self.graphics.backend_name()
            )
        });

        // Close the session before the runtime loop destroys the map, because a
        // map with an attached session cannot be destroyed.
        if let Some(target) = self.target.take()
            && let Err(error) = target.close(&self.graphics)
        {
            append_error(&mut first_error, error.to_string());
        }
        self.shared.request_shutdown();
        // Release the pump so shutdown is observed now.
        let _ = self.wake.signal();
        if let Some(runtime_thread) = self.runtime_thread.take()
            && runtime_thread.join().is_err()
        {
            append_error(
                &mut first_error,
                "the runtime loop thread panicked".to_string(),
            );
        }
        if let Some(error) = self.shared.failure() {
            append_error(&mut first_error, error);
        }

        match first_error {
            Some(error) => Err(error.into()),
            None => Ok(()),
        }
    }

    fn abort_process(&mut self, code: i32) -> ! {
        self.closed = true;
        immediate_exit(code);
    }
}

/// Stops a runtime loop that outlived a failed startup, and reports the failure
/// it recorded in preference to the one that stopped it.
fn stop_runtime_loop(
    shared: &Shared,
    runtime_thread: JoinHandle<()>,
    message: String,
) -> Box<dyn Error> {
    shared.request_shutdown();
    let panicked = runtime_thread.join().is_err();
    let mut reported = shared.failure().unwrap_or(message);
    if panicked {
        reported.push_str("; the runtime loop thread panicked");
    }
    reported.into()
}

fn append_error(message: &mut Option<String>, error: String) {
    match message {
        Some(message) => message.push_str(&format!("; {error}")),
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
