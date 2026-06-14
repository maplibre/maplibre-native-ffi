#![deny(unsafe_op_in_unsafe_fn)]

#[cfg(not(any(feature = "metal", feature = "opengl", feature = "vulkan")))]
compile_error!("rust-map requires exactly one backend feature: metal, opengl, or vulkan");
#[cfg(any(
    all(feature = "metal", feature = "opengl"),
    all(feature = "metal", feature = "vulkan"),
    all(feature = "opengl", feature = "vulkan")
))]
compile_error!("rust-map backend features are mutually exclusive");
#[cfg(all(feature = "metal", not(target_os = "macos")))]
compile_error!("rust-map metal backend is only supported on macOS");
#[cfg(all(
    feature = "opengl",
    not(any(target_os = "linux", target_os = "windows"))
))]
compile_error!("rust-map opengl backend is only supported on Linux and Windows");

mod app;
mod graphics;
mod input;
#[cfg(feature = "metal")]
mod metal;
#[cfg(feature = "opengl")]
mod opengl;
mod render_target;
mod shell;
mod viewport;
#[cfg(feature = "vulkan")]
mod vulkan;
#[cfg(feature = "vulkan")]
mod vulkan_texture_compositor;

use std::error::Error;

use render_target::Mode;

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

    shell::run(mode, backends)
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
    backends.contains(graphics::required_backend())
}
