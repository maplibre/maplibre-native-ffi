use std::cell::RefCell;
use std::ffi::c_void;

use maplibre_native::{Error, NativePointer};

use crate::app::App;
use crate::diagnostics::log_error;
use crate::viewport::Viewport;

thread_local! {
    static APP: RefCell<Option<App>> = const { RefCell::new(None) };
}

#[unsafe(no_mangle)]
pub extern "C" fn mln_browser_map_init(
    logical_width: u32,
    logical_height: u32,
    scale_factor: f64,
    webgpu_device: *mut c_void,
    webgpu_queue: *mut c_void,
) -> i32 {
    APP.with(|slot| {
        slot.borrow_mut().take();
    });

    let result = Viewport::new(logical_width, logical_height, scale_factor)
        .and_then(|viewport| App::new(viewport, pointer(webgpu_device), pointer(webgpu_queue)));

    match result {
        Ok(app) => {
            APP.with(|slot| {
                *slot.borrow_mut() = Some(app);
            });
            0
        }
        Err(error) => {
            log_error("init", &error);
            1
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn mln_browser_map_render_frame() -> i32 {
    with_app_mut(0, |app| Ok(i32::from(app.render_frame()?)))
}

#[unsafe(no_mangle)]
pub extern "C" fn mln_browser_map_acquire_owned_texture() -> usize {
    with_app_mut(0, App::acquire_owned_texture)
}

#[unsafe(no_mangle)]
pub extern "C" fn mln_browser_map_release_owned_texture_frame() -> i32 {
    with_app_mut(1, |app| {
        app.release_owned_texture_frame()?;
        Ok(0)
    })
}

#[unsafe(no_mangle)]
pub extern "C" fn mln_browser_map_resize(
    logical_width: u32,
    logical_height: u32,
    scale_factor: f64,
) -> i32 {
    with_app_mut(1, |app| {
        app.resize(Viewport::new(logical_width, logical_height, scale_factor)?)?;
        Ok(0)
    })
}

#[unsafe(no_mangle)]
pub extern "C" fn mln_browser_map_move_by(delta_x: f64, delta_y: f64) -> i32 {
    with_app_mut(1, |app| {
        app.move_by(delta_x, delta_y)?;
        Ok(0)
    })
}

#[unsafe(no_mangle)]
pub extern "C" fn mln_browser_map_scale_by(scale: f64, x: f64, y: f64) -> i32 {
    with_app_mut(1, |app| {
        app.scale_by(scale, x, y)?;
        Ok(0)
    })
}

#[unsafe(no_mangle)]
pub extern "C" fn mln_browser_map_rotate_pitch_by(bearing_delta: f64, pitch_delta: f64) -> i32 {
    with_app_mut(1, |app| {
        app.rotate_pitch_by(bearing_delta, pitch_delta)?;
        Ok(0)
    })
}

#[unsafe(no_mangle)]
pub extern "C" fn mln_browser_map_rotate_by(bearing_delta: f64) -> i32 {
    with_app_mut(1, |app| {
        app.rotate_by(bearing_delta)?;
        Ok(0)
    })
}

#[unsafe(no_mangle)]
pub extern "C" fn mln_browser_map_pitch_by(pitch_delta: f64) -> i32 {
    with_app_mut(1, |app| {
        app.pitch_by(pitch_delta)?;
        Ok(0)
    })
}

#[unsafe(no_mangle)]
pub extern "C" fn mln_browser_map_reset_orientation() -> i32 {
    with_app_mut(1, |app| {
        app.reset_orientation()?;
        Ok(0)
    })
}

#[unsafe(no_mangle)]
pub extern "C" fn mln_browser_map_jump_to(
    longitude: f64,
    latitude: f64,
    zoom: f64,
    bearing: f64,
    pitch: f64,
) -> i32 {
    with_app_mut(1, |app| {
        app.jump_to(longitude, latitude, zoom, bearing, pitch)?;
        Ok(0)
    })
}

#[unsafe(no_mangle)]
pub extern "C" fn mln_browser_map_cancel_transitions() -> i32 {
    with_app_mut(1, |app| {
        app.cancel_transitions()?;
        Ok(0)
    })
}

fn with_app_mut<T: Copy>(default: T, action: impl FnOnce(&mut App) -> Result<T, Error>) -> T {
    let Some(mut app) = APP.with(|slot| slot.borrow_mut().take()) else {
        return default;
    };
    let result = match action(&mut app) {
        Ok(value) => value,
        Err(error) => {
            log_error("browser map", &error);
            default
        }
    };
    APP.with(|slot| {
        let mut current = slot.borrow_mut();
        if current.is_none() {
            *current = Some(app);
        }
    });
    result
}

fn pointer(address: *mut c_void) -> NativePointer {
    // SAFETY: Browser JS passes opaque WebGPU handles imported by Emscripten's
    // WebGPU glue. Rust only stores and forwards the address to the C ABI.
    unsafe { NativePointer::from_ptr(address) }
}
