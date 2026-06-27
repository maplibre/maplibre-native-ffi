use maplibre_native::{Error, RenderBackendMask};

use crate::viewport::Viewport;

pub fn log_startup(supported: RenderBackendMask) {
    println!("supported render backends: 0x{:08x}", supported.bits());
    println!("render target mode: owned-texture");
    println!("render target status: samples MapLibre-owned texture frames into the host swapchain");
    println!(
        "Controls:\n  left drag: pan\n  right drag or Ctrl+left drag: rotate with X, pitch with Y\n  scroll: zoom at cursor\n  arrows or WASD: pan\n  + / -: zoom at center\n  Q / E: rotate\n  ] / [: pitch\n  0: reset pitch and bearing"
    );
}

pub fn log_viewport(viewport: &Viewport) {
    println!(
        "viewport: logical={}x{} physical={}x{} scale={:.3}",
        viewport.logical_width,
        viewport.logical_height,
        viewport.physical_width(),
        viewport.physical_height(),
        viewport.scale_factor
    );
}

pub fn log_error(stage: &str, error: &Error) {
    eprintln!("{stage} failed: {error}");
}
