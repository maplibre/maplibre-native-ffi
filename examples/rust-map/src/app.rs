use maplibre_native::{
    CameraOptions, LatLng, MapMode, MapOptions, RuntimeEventPayload, RuntimeEventSource,
    RuntimeEventType, RuntimeHandle,
};
use std::error::Error;
use winit::event::WindowEvent;
use winit::event_loop::EventLoopWindowTarget;
use winit::window::Window;

use crate::input::Controller;
use crate::render_target::{Mode, RenderTarget};
use crate::viewport::Viewport;
use crate::vulkan::VulkanContext;

const STYLE_URL: &str = "https://tiles.openfreemap.org/styles/bright";

pub struct App {
    target: Option<RenderTarget>,
    map: Option<maplibre_native::MapHandle>,
    runtime: Option<RuntimeHandle>,
    _vulkan: VulkanContext,
    window: Window,
    viewport: Viewport,
    input: Controller,
    render_pending: bool,
    viewport_dirty: bool,
    closed: bool,
    mode: Mode,
}

impl App {
    pub fn new(window: Window, mode: Mode) -> Result<Self, Box<dyn Error>> {
        let vulkan = VulkanContext::new(&window)?;
        let viewport = Viewport::from_window(&window);
        if viewport.is_empty() {
            return Err("window has no drawable extent".into());
        }

        let runtime = RuntimeHandle::new()?;
        let map = runtime.create_map_with_options(
            &MapOptions::new(
                viewport.logical_width,
                viewport.logical_height,
                viewport.scale_factor,
            )
            .with_mode(MapMode::Continuous),
        )?;
        viewport.log("initial viewport");
        let target = RenderTarget::attach(mode, &map, &vulkan, viewport)?;
        map.set_style_url(STYLE_URL)?;
        map.jump_to(
            &CameraOptions::new()
                .with_center(LatLng::new(37.7749, -122.4194))
                .with_zoom(13.0)
                .with_bearing(12.0)
                .with_pitch(30.0),
        )?;
        map.request_repaint()?;

        Ok(Self {
            target: Some(target),
            map: Some(map),
            runtime: Some(runtime),
            _vulkan: vulkan,
            window,
            viewport,
            input: Controller::default(),
            render_pending: true,
            viewport_dirty: false,
            closed: false,
            mode,
        })
    }

    pub fn print_status(&self) {
        println!("MapLibre Rust Vulkan map example running. Close the window to exit.");
        println!("rust-map render target: {}", self.mode.cli_name());
        println!("render target status: {}", self.mode.status());
        Controller::print_controls();
    }

    pub fn handle_window_event(&mut self, event: WindowEvent, target: &EventLoopWindowTarget<()>) {
        if self.closed {
            if matches!(event, WindowEvent::CloseRequested) {
                target.exit();
            }
            return;
        }

        match event {
            WindowEvent::CloseRequested => self.request_exit(target),
            WindowEvent::Resized(_) | WindowEvent::ScaleFactorChanged { .. } => self.queue_resize(),
            WindowEvent::RedrawRequested => self.render_or_exit(),
            event => match self.input.handle(
                &event,
                self.map.as_ref().expect("map is open"),
                self.viewport,
            ) {
                Ok(true) => {
                    self.render_pending = true;
                    self.window.request_redraw();
                }
                Ok(false) => {}
                Err(error) => {
                    eprintln!("input failed: {error}");
                    self.abort_process(1);
                }
            },
        }
    }

    pub fn step(&mut self) {
        if let Err(error) = self.pump_runtime() {
            eprintln!("runtime update failed: {error}");
            self.abort_process(1);
        }
        if self.render_pending {
            self.render_or_exit();
        }
    }

    fn queue_resize(&mut self) {
        self.viewport_dirty = true;
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
            self.render_pending = false;
            return Ok(());
        }
        self.target
            .as_mut()
            .expect("render target is open")
            .resize(next)?;
        self.map.as_ref().expect("map is open").request_repaint()?;
        self.render_pending = true;
        Ok(())
    }

    fn pump_runtime(&mut self) -> Result<(), Box<dyn Error>> {
        if self.closed {
            return Ok(());
        }
        self.apply_pending_resize()?;
        let runtime = self.runtime.as_ref().expect("runtime is open");
        runtime.run_once()?;
        while let Some(event) = runtime.poll_event()? {
            match event.event_type {
                RuntimeEventType::MapRenderUpdateAvailable
                    if event.source
                        == RuntimeEventSource::Map(
                            self.map.as_ref().expect("map is open").id(),
                        ) =>
                {
                    self.render_pending = true;
                }
                RuntimeEventType::MapRenderFrameFinished => {
                    if let RuntimeEventPayload::RenderFrame(frame) = event.payload {
                        self.render_pending |= frame.needs_repaint;
                    }
                }
                _ => {}
            }
        }
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
        self.pump_runtime()?;
        if self.render_pending {
            self.target
                .as_mut()
                .expect("render target is open")
                .render_update()?;
            self.render_pending = false;
        }
        Ok(())
    }

    pub fn close_or_abort(&mut self) {
        if let Err(error) = self.close_resources() {
            eprintln!("shutdown failed: {error}");
            self.abort_process(1);
        }
    }

    fn request_exit(&mut self, target: &EventLoopWindowTarget<()>) {
        self.close_or_abort();
        target.exit();
    }

    fn close_resources(&mut self) -> Result<(), Box<dyn Error>> {
        if self.closed {
            return Ok(());
        }
        self.closed = true;
        self.render_pending = false;
        self.viewport_dirty = false;
        if let Some(target) = self.target.take() {
            target.close()?;
        }
        if let Some(map) = self.map.take() {
            map.close()?;
        }
        if let Some(runtime) = self.runtime.take() {
            runtime.close()?;
        }
        Ok(())
    }

    fn abort_process(&mut self, code: i32) -> ! {
        self.closed = true;
        self.render_pending = false;
        immediate_exit(code);
    }
}

fn immediate_exit(code: i32) -> ! {
    unsafe extern "C" {
        fn _exit(status: std::ffi::c_int) -> !;
    }

    // SAFETY: `_exit` terminates the process without running native teardown.
    // The example uses it on close because the current macOS Vulkan stack can
    // abort while MapLibre native tears down thread-local state after the window
    // has closed. The operating system reclaims the example's resources.
    unsafe { _exit(code) }
}
