#![deny(unsafe_op_in_unsafe_fn)]

mod app;
mod input;
mod render_target;
mod viewport;
mod vulkan;
mod vulkan_texture_compositor;

use std::error::Error;
use std::time::{Duration, Instant};

use app::App;
use render_target::Mode;
use winit::dpi::LogicalSize;
use winit::event::Event;
use winit::event_loop::{ControlFlow, EventLoop};
use winit::window::WindowBuilder;

const INITIAL_WIDTH: u32 = 1280;
const INITIAL_HEIGHT: u32 = 720;

fn main() -> Result<(), Box<dyn Error>> {
    let mode = parse_args(std::env::args().skip(1))?;
    if !maplibre_native::supported_render_backends()
        .contains(maplibre_native::RenderBackendMask::VULKAN)
    {
        return Err("the loaded MapLibre native library does not support Vulkan".into());
    }

    let event_loop = EventLoop::new()?;
    let window = WindowBuilder::new()
        .with_title("MapLibre Rust Vulkan Map")
        .with_inner_size(LogicalSize::new(INITIAL_WIDTH, INITIAL_HEIGHT))
        .with_resizable(true)
        .build(&event_loop)?;
    let mut app = App::new(window, mode)?;

    app.print_status();
    event_loop.run(move |event, target| {
        target.set_control_flow(ControlFlow::WaitUntil(
            Instant::now() + Duration::from_millis(4),
        ));

        match event {
            Event::WindowEvent { event, .. } => app.handle_window_event(event, target),
            Event::AboutToWait => app.step(),
            Event::LoopExiting => app.close_or_abort(),
            _ => {}
        }
    })?;

    Ok(())
}

fn parse_args(args: impl IntoIterator<Item = String>) -> Result<Mode, Box<dyn Error>> {
    let mut mode = Mode::OwnedTexture;
    for arg in args {
        if let Some(value) = arg.strip_prefix("--render-target=") {
            mode = Mode::parse(value)?;
        } else if !arg.starts_with('-') {
            mode = Mode::parse(&arg)?;
        } else {
            return Err(format!("unknown argument: {arg}").into());
        }
    }
    Ok(mode)
}
