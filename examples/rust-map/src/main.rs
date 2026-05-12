#![deny(unsafe_op_in_unsafe_fn)]

mod app;
mod input;
mod render_target;
mod viewport;
mod vulkan;

use std::error::Error;
use std::time::{Duration, Instant};

use app::App;
use winit::dpi::PhysicalSize;
use winit::event::Event;
use winit::event_loop::{ControlFlow, EventLoop};
use winit::window::WindowBuilder;

const INITIAL_WIDTH: u32 = 1280;
const INITIAL_HEIGHT: u32 = 720;

fn main() -> Result<(), Box<dyn Error>> {
    if !maplibre_native::supported_render_backends()
        .contains(maplibre_native::RenderBackendMask::VULKAN)
    {
        return Err("the loaded MapLibre native library does not support Vulkan".into());
    }

    let event_loop = EventLoop::new()?;
    let window = WindowBuilder::new()
        .with_title("MapLibre Rust Vulkan Map")
        .with_inner_size(PhysicalSize::new(INITIAL_WIDTH, INITIAL_HEIGHT))
        .with_resizable(true)
        .build(&event_loop)?;
    let mut app = App::new(window)?;

    app.print_status();
    event_loop.run(move |event, target| {
        target.set_control_flow(ControlFlow::WaitUntil(
            Instant::now() + Duration::from_millis(4),
        ));

        match event {
            Event::WindowEvent { event, .. } => app.handle_window_event(event, target),
            Event::AboutToWait => app.step(target),
            Event::LoopExiting => app.close_or_abort(),
            _ => {}
        }
    })?;

    Ok(())
}
