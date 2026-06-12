use std::error::Error;
use std::time::{Duration, Instant};

use winit::application::ApplicationHandler;
use winit::event::{StartCause, WindowEvent};
use winit::event_loop::{ActiveEventLoop, ControlFlow, EventLoop};
use winit::window::{Window, WindowAttributes, WindowId};

use crate::app::App;
use crate::graphics::GraphicsContext;
use crate::render_target::Mode;

const INITIAL_WIDTH: u32 = 960;
const INITIAL_HEIGHT: u32 = 640;

pub fn run(mode: Mode, backends: maplibre_native::RenderBackendMask) -> Result<(), Box<dyn Error>> {
    let event_loop = EventLoop::new()?;
    let mut shell = Shell::new(mode, backends);
    let run_result = event_loop.run_app(&mut shell);
    if let Some(error) = shell.startup_error {
        return Err(error);
    }
    run_result.map_err(Into::into)
}

struct Shell {
    mode: Mode,
    backends: maplibre_native::RenderBackendMask,
    app: Option<App>,
    startup_error: Option<Box<dyn Error>>,
}

impl Shell {
    fn new(mode: Mode, backends: maplibre_native::RenderBackendMask) -> Self {
        Self {
            mode,
            backends,
            app: None,
            startup_error: None,
        }
    }

    fn startup(&mut self, event_loop: &ActiveEventLoop) -> Result<(), Box<dyn Error>> {
        let (window, graphics) =
            GraphicsContext::create_window(event_loop, window_attributes(), self.backends)?;
        let app = App::new(window, graphics, self.mode)?;
        app.print_status();
        self.app = Some(app);
        Ok(())
    }
}

impl ApplicationHandler for Shell {
    fn new_events(&mut self, event_loop: &ActiveEventLoop, _cause: StartCause) {
        event_loop.set_control_flow(ControlFlow::WaitUntil(
            Instant::now() + Duration::from_millis(4),
        ));
    }

    fn resumed(&mut self, event_loop: &ActiveEventLoop) {
        if self.app.is_some() || self.startup_error.is_some() {
            return;
        }
        if let Err(error) = self.startup(event_loop) {
            self.startup_error = Some(error);
            event_loop.exit();
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
            app.close_or_abort();
            event_loop.exit();
            return;
        }
        app.handle_window_event(event);
    }

    fn about_to_wait(&mut self, _event_loop: &ActiveEventLoop) {
        if let Some(app) = self.app.as_mut() {
            app.step();
        }
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
