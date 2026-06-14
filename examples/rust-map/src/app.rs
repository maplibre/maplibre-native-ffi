use maplibre_native::{
    CameraOptions, LatLng, MapMode, MapOptions, RuntimeEventPayload, RuntimeEventSource,
    RuntimeEventType, RuntimeHandle, RuntimeOptions,
};
use std::error::Error;
use winit::event::WindowEvent;
use winit::window::{Window, WindowId};

use crate::graphics::GraphicsContext;
use crate::input::Controller;
use crate::render_target::{Mode, RenderTarget};
use crate::viewport::Viewport;

const STYLE_URL: &str = "https://tiles.openfreemap.org/styles/bright";

pub struct App {
    target: Option<RenderTarget>,
    map: Option<maplibre_native::MapHandle>,
    runtime: Option<RuntimeHandle>,
    graphics: GraphicsContext,
    window: Window,
    viewport: Viewport,
    input: Controller,
    render_pending: bool,
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

        let runtime =
            match RuntimeHandle::with_options(&RuntimeOptions::new().with_cache_path(":memory:")) {
                Ok(runtime) => runtime,
                Err(error) => {
                    return Err(startup_error(
                        format!("runtime creation failed: {error}"),
                        None,
                        None,
                    ));
                }
            };
        let map_options = MapOptions::new(
            viewport.logical_width,
            viewport.logical_height,
            viewport.scale_factor,
        )
        .with_mode(MapMode::Continuous);
        let map = match runtime.create_map_with_options(&map_options) {
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
        viewport.log("initial viewport");
        let target = match RenderTarget::attach(mode, &map, &graphics, viewport) {
            Ok(target) => target,
            Err(error) => {
                return Err(startup_error(
                    format!("render target attachment failed: {error}"),
                    Some(map),
                    Some(runtime),
                ));
            }
        };

        Ok(Self {
            target: Some(target),
            map: Some(map),
            runtime: Some(runtime),
            graphics,
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
        self.graphics.resize(next)?;
        if self
            .target
            .as_ref()
            .expect("render target is open")
            .needs_reattach_on_resize()
        {
            let old_target = self.target.take().expect("render target is open");
            old_target.close(&self.graphics)?;
            let new_target = RenderTarget::attach(
                self.mode,
                self.map.as_ref().expect("map is open"),
                &self.graphics,
                next,
            )?;
            self.target = Some(new_target);
        } else {
            self.target
                .as_mut()
                .expect("render target is open")
                .resize(next)?;
        }
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
                RuntimeEventType::MapRenderFrameFinished
                    if event.source
                        == RuntimeEventSource::Map(
                            self.map.as_ref().expect("map is open").id(),
                        ) =>
                {
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
        if self.render_pending {
            match self
                .target
                .as_mut()
                .expect("render target is open")
                .render_update(&self.graphics)
            {
                Ok(()) => self.render_pending = false,
                Err(error) if error.kind() == maplibre_native::ErrorKind::InvalidState => {}
                Err(error) => return Err(error.into()),
            }
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
        self.render_pending = false;
        self.viewport_dirty = false;

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
        if let Some(runtime) = self.runtime.take()
            && let Err(error) = runtime.close()
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
        self.render_pending = false;
        immediate_exit(code);
    }
}

fn configure_map(map: &maplibre_native::MapHandle) -> maplibre_native::Result<()> {
    map.set_style_url(STYLE_URL)?;
    map.jump_to(
        &CameraOptions::new()
            .with_center(LatLng::new(37.7749, -122.4194))
            .with_zoom(13.0)
            .with_bearing(12.0)
            .with_pitch(30.0),
    )?;
    map.request_repaint()
}

fn startup_error(
    mut message: String,
    map: Option<maplibre_native::MapHandle>,
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
