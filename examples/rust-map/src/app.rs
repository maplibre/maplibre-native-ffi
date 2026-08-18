//! The winit event loop owns the window, graphics context, and render session.
//! Runtime and map commands are safe to submit from its callbacks.

use std::error::Error;

use winit::event::WindowEvent;
use winit::window::{Window, WindowId};

use crate::graphics::GraphicsContext;
use crate::input::Controller;
use crate::map_state::MapState;
use crate::render_target::{Mode, RenderTarget};
use crate::viewport::Viewport;

pub struct App {
    target: Option<RenderTarget>,
    map: Option<MapState>,
    graphics: GraphicsContext,
    window: Window,
    viewport: Viewport,
    input: Controller,
    viewport_dirty: bool,
    render_requested: bool,
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

        let map = MapState::new(viewport)?;
        let target = match RenderTarget::attach(mode, map.map_handle(), &graphics, viewport) {
            Ok(target) => target,
            Err(error) => {
                let mut message = format!("render target attachment failed: {error}");
                if let Err(error) = map.close() {
                    message.push_str(&format!("; map state cleanup failed: {error}"));
                }
                return Err(message.into());
            }
        };
        window.request_redraw();

        Ok(Self {
            target: Some(target),
            map: Some(map),
            graphics,
            window,
            viewport,
            input: Controller::default(),
            viewport_dirty: false,
            render_requested: true,
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
                let (input, map) = (&mut self.input, self.map.as_mut().expect("map is open"));
                match input.handle(&event, self.viewport, map) {
                    Ok(true) => {
                        self.render_requested = true;
                        self.window.request_redraw();
                    }
                    Ok(false) => {}
                    Err(error) => {
                        eprintln!("camera update failed: {error}");
                        self.abort_process(1);
                    }
                }
            }
        }
    }

    pub fn step(&mut self) {
        if let Err(error) = self.apply_pending_resize() {
            eprintln!("resize failed: {error}");
            self.abort_process(1);
        }
        match self.map.as_ref().expect("map is open").drain_events() {
            Ok(true) => {
                self.render_requested = true;
                self.window.request_redraw();
            }
            Ok(false) => {}
            Err(error) => {
                eprintln!("event drain failed: {error}");
                self.abort_process(1);
            }
        }
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
        self.map.as_mut().expect("map is open").resize(next)?;
        self.graphics.resize(next)?;
        self.target
            .as_mut()
            .expect("render target is open")
            .resize(&self.graphics, next)?;
        self.render_requested = true;
        self.window.request_redraw();
        Ok(())
    }

    fn render_or_exit(&mut self) {
        if let Err(error) = self.render() {
            eprintln!("render failed: {error}");
            self.abort_process(1);
        }
    }

    fn render(&mut self) -> Result<(), Box<dyn Error>> {
        if self.closed || self.viewport.is_empty() || !self.render_requested {
            return Ok(());
        }
        self.render_requested = false;
        if !self
            .target
            .as_mut()
            .expect("render target is open")
            .render_update(&self.graphics)?
        {
            self.render_requested = true;
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
        self.render_requested = false;

        let mut first_error = self.graphics.wait_idle().err().map(|error| {
            format!(
                "{} device wait idle failed: {error}",
                self.graphics.backend_name()
            )
        });

        if let Some(target) = self.target.take()
            && let Err(error) = target.close(&self.graphics)
        {
            append_error(&mut first_error, error.to_string());
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

    fn abort_process(&mut self, code: i32) -> ! {
        self.closed = true;
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

#[cfg(unix)]
fn immediate_exit(code: i32) -> ! {
    // SAFETY: Abort paths intentionally skip destructors after a fatal native or
    // graphics error, and `_exit` terminates the process immediately.
    unsafe { libc::_exit(code) }
}

#[cfg(windows)]
fn immediate_exit(code: i32) -> ! {
    // SAFETY: Abort paths intentionally skip destructors after a fatal native or
    // graphics error, and `TerminateProcess` terminates the current process.
    unsafe {
        windows_sys::Win32::System::Threading::TerminateProcess(
            windows_sys::Win32::System::Threading::GetCurrentProcess(),
            code as u32,
        );
    }
    std::process::abort()
}
