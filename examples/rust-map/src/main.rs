#![deny(unsafe_op_in_unsafe_fn)]

mod app;
mod graphics;
mod input;
#[cfg(target_os = "macos")]
mod metal;
#[cfg(any(target_os = "linux", target_os = "windows"))]
mod opengl;
mod render_target;
mod viewport;
mod vulkan;
mod vulkan_texture_compositor;

use std::error::Error;
use std::time::{Duration, Instant};

use app::App;
use graphics::GraphicsContext;
use render_target::Mode;
use winit::event::Event;
use winit::event_loop::{ControlFlow, EventLoop};
use winit::window::WindowBuilder;

const INITIAL_WIDTH: u32 = 960;
const INITIAL_HEIGHT: u32 = 640;

fn main() -> Result<(), Box<dyn Error>> {
    let Some(mode) = parse_args(std::env::args().skip(1))? else {
        return Ok(());
    };
    let backends = maplibre_native::supported_render_backends();
    println!("native render backends: {}", render_backend_label(backends));
    if !supports_usable_backend(backends) {
        return Err("the loaded MapLibre native library does not support a backend usable by rust-map on this platform".into());
    }
    maplibre_native::set_log_callback(|record| {
        eprintln!(
            "MapLibre {:?} {:?} {}: {}",
            record.severity, record.event, record.code, record.message
        );
        true
    })?;
    struct ClearLogCallback;
    impl Drop for ClearLogCallback {
        fn drop(&mut self) {
            let _ = maplibre_native::clear_log_callback();
        }
    }
    let _clear_log_callback = ClearLogCallback;

    let event_loop = EventLoop::new()?;
    let window_builder = WindowBuilder::new()
        .with_title("MapLibre Rust Map")
        .with_inner_size(winit::dpi::LogicalSize::new(INITIAL_WIDTH, INITIAL_HEIGHT))
        .with_resizable(true);
    let (window, graphics) = GraphicsContext::create_window(&event_loop, window_builder, backends)?;
    let mut app = App::new(window, graphics, mode)?;

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

fn parse_args(args: impl IntoIterator<Item = String>) -> Result<Option<Mode>, Box<dyn Error>> {
    let mut args = args.into_iter();
    let Some(arg) = args.next() else {
        print_usage();
        std::process::exit(1);
    };
    if arg == "--help" {
        print_usage();
        return Ok(None);
    }
    if arg.starts_with('-') || args.next().is_some() {
        print_usage();
        std::process::exit(1);
    }
    match Mode::parse(&arg) {
        Ok(mode) => Ok(Some(mode)),
        Err(error) => {
            eprintln!("{error}");
            print_usage();
            std::process::exit(1);
        }
    }
}

fn print_usage() {
    eprintln!(
        "Usage: rust-map <mode>\n\nModes:\n  owned-texture     session-owned texture render target\n  borrowed-texture  caller-owned texture render target\n  native-surface    native surface render target"
    );
}

fn render_backend_label(backends: maplibre_native::RenderBackendMask) -> String {
    let mut labels = Vec::new();
    if backends.contains(maplibre_native::RenderBackendMask::METAL) {
        labels.push("metal");
    }
    if backends.contains(maplibre_native::RenderBackendMask::OPENGL) {
        labels.push("opengl");
    }
    if backends.contains(maplibre_native::RenderBackendMask::VULKAN) {
        labels.push("vulkan");
    }
    if labels.is_empty() {
        "none".to_string()
    } else {
        labels.join(",")
    }
}

fn supports_usable_backend(backends: maplibre_native::RenderBackendMask) -> bool {
    #[cfg(target_os = "macos")]
    {
        return backends.intersects(
            maplibre_native::RenderBackendMask::METAL | maplibre_native::RenderBackendMask::VULKAN,
        );
    }
    #[cfg(not(target_os = "macos"))]
    backends.intersects(
        maplibre_native::RenderBackendMask::VULKAN | maplibre_native::RenderBackendMask::OPENGL,
    )
}
