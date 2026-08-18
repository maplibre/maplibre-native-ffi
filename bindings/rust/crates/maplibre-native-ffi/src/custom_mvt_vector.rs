use std::fmt;
use std::os::raw::c_void;
use std::panic::{AssertUnwindSafe, catch_unwind};
use std::ptr;
use std::sync::{Condvar, Mutex};

use maplibre_native_ffi_core as maplibre_core;
use maplibre_native_ffi_sys as sys;

use crate::custom_geometry::CanonicalTileId;

type TileCallback = dyn Fn(CanonicalTileId) + Send + Sync + 'static;

/// Options used when adding a custom MVT vector source.
///
/// Custom MVT vector callbacks may run on native worker threads. Keep callbacks
/// quick, and hand work back to the map owner thread before calling map APIs
/// such as `set_custom_mvt_vector_source_tile_data` or
/// `set_custom_mvt_vector_source_tile_error`.
#[non_exhaustive]
pub struct CustomMvtVectorSourceOptions {
    fetch_tile: Box<TileCallback>,
    cancel_tile: Option<Box<TileCallback>>,
    /// Minimum zoom level at which the source produces tiles.
    pub min_zoom: Option<f64>,
    /// Maximum zoom level at which the source produces tiles.
    pub max_zoom: Option<f64>,
}

impl fmt::Debug for CustomMvtVectorSourceOptions {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("CustomMvtVectorSourceOptions")
            .field("has_cancel_tile", &self.cancel_tile.is_some())
            .field("min_zoom", &self.min_zoom)
            .field("max_zoom", &self.max_zoom)
            .finish_non_exhaustive()
    }
}

impl CustomMvtVectorSourceOptions {
    pub fn new<F>(fetch_tile: F) -> Self
    where
        F: Fn(CanonicalTileId) + Send + Sync + 'static,
    {
        Self {
            fetch_tile: Box::new(fetch_tile),
            cancel_tile: None,
            min_zoom: None,
            max_zoom: None,
        }
    }

    pub fn with_cancel_tile<F>(mut self, cancel_tile: F) -> Self
    where
        F: Fn(CanonicalTileId) + Send + Sync + 'static,
    {
        self.cancel_tile = Some(Box::new(cancel_tile));
        self
    }
}

#[derive(Debug, Default)]
struct CallbackLifecycle {
    active: usize,
    closing: bool,
    closed: bool,
}

/// Callback state for one custom MVT vector source, owned by the C API from a
/// successful add until it invokes the release callback that frees this box.
pub(crate) struct CustomMvtVectorSourceState {
    fetch_tile: Box<TileCallback>,
    cancel_tile: Option<Box<TileCallback>>,
    min_zoom: Option<f64>,
    max_zoom: Option<f64>,
    lifecycle: Mutex<CallbackLifecycle>,
    idle: Condvar,
}

impl fmt::Debug for CustomMvtVectorSourceState {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("CustomMvtVectorSourceState")
            .finish_non_exhaustive()
    }
}

impl CustomMvtVectorSourceState {
    pub(crate) fn new(options: CustomMvtVectorSourceOptions) -> Box<Self> {
        Box::new(Self {
            fetch_tile: options.fetch_tile,
            cancel_tile: options.cancel_tile,
            min_zoom: options.min_zoom,
            max_zoom: options.max_zoom,
            lifecycle: Mutex::new(CallbackLifecycle::default()),
            idle: Condvar::new(),
        })
    }

    pub(crate) fn descriptor(&self) -> sys::mln_custom_mvt_vector_source_options {
        maplibre_core::style::custom_mvt_vector_source_options_to_native(
            maplibre_core::style::CustomMvtVectorSourceDescriptorFields {
                fetch_tile: Some(fetch_tile_trampoline),
                cancel_tile: self
                    .cancel_tile
                    .as_ref()
                    .map(|_| cancel_tile_trampoline as _),
                release_user_data: Some(release_trampoline),
                user_data: ptr::from_ref(self).cast_mut().cast::<c_void>(),
                min_zoom: self.min_zoom,
                max_zoom: self.max_zoom,
            },
        )
    }

    pub(crate) fn close(&self) {
        let mut lifecycle = self
            .lifecycle
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        if lifecycle.closed {
            return;
        }
        lifecycle.closing = true;
        while lifecycle.active != 0 {
            lifecycle = self
                .idle
                .wait(lifecycle)
                .unwrap_or_else(|poisoned| poisoned.into_inner());
        }
        lifecycle.closed = true;
    }

    fn invoke_fetch(&self, tile_id: CanonicalTileId) {
        let Some(_guard) = self.enter_callback() else {
            return;
        };
        let _ = catch_unwind(AssertUnwindSafe(|| (self.fetch_tile)(tile_id)));
    }

    fn invoke_cancel(&self, tile_id: CanonicalTileId) {
        let Some(_guard) = self.enter_callback() else {
            return;
        };
        if let Some(cancel_tile) = &self.cancel_tile {
            let _ = catch_unwind(AssertUnwindSafe(|| cancel_tile(tile_id)));
        }
    }

    fn enter_callback(&self) -> Option<CallbackGuard<'_>> {
        let mut lifecycle = self
            .lifecycle
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        if lifecycle.closing || lifecycle.closed {
            return None;
        }
        lifecycle.active += 1;
        Some(CallbackGuard { state: self })
    }

    fn exit_callback(&self) {
        let mut lifecycle = self
            .lifecycle
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        lifecycle.active -= 1;
        if lifecycle.active == 0 {
            self.idle.notify_all();
        }
    }
}

impl Drop for CustomMvtVectorSourceState {
    fn drop(&mut self) {
        self.close();
    }
}

struct CallbackGuard<'a> {
    state: &'a CustomMvtVectorSourceState,
}

impl Drop for CallbackGuard<'_> {
    fn drop(&mut self) {
        self.state.exit_callback();
    }
}

unsafe extern "C" fn fetch_tile_trampoline(
    user_data: *mut c_void,
    tile_id: sys::mln_canonical_tile_id,
) {
    let Some(state) = ptr::NonNull::new(user_data.cast::<CustomMvtVectorSourceState>()) else {
        return;
    };
    // SAFETY: user_data is installed from CustomMvtVectorSourceState::descriptor
    // and remains valid until source/style/map teardown waits for in-flight callbacks.
    unsafe { state.as_ref() }.invoke_fetch(CanonicalTileId::from_native(tile_id));
}

unsafe extern "C" fn release_trampoline(user_data: *mut c_void) {
    let Some(state) = ptr::NonNull::new(user_data.cast::<CustomMvtVectorSourceState>()) else {
        return;
    };
    // SAFETY: The C API invokes this once, with the pointer
    // add_custom_mvt_vector_source handed it, after it stops referencing the
    // state, so this call owns the box. Dropping it waits for in-flight tile
    // callbacks before it frees the host's own callbacks.
    let state = unsafe { Box::from_raw(state.as_ptr()) };
    let _ = catch_unwind(AssertUnwindSafe(move || drop(state)));
}

unsafe extern "C" fn cancel_tile_trampoline(
    user_data: *mut c_void,
    tile_id: sys::mln_canonical_tile_id,
) {
    let Some(state) = ptr::NonNull::new(user_data.cast::<CustomMvtVectorSourceState>()) else {
        return;
    };
    // SAFETY: user_data is installed from CustomMvtVectorSourceState::descriptor
    // and remains valid until source/style/map teardown waits for in-flight callbacks.
    unsafe { state.as_ref() }.invoke_cancel(CanonicalTileId::from_native(tile_id));
}

#[cfg(test)]
mod tests {
    use std::sync::atomic::{AtomicBool, AtomicUsize, Ordering};
    use std::sync::{Arc, Condvar, Mutex};
    use std::time::Duration;

    use super::*;

    fn tile(z: u32, x: u32, y: u32) -> sys::mln_canonical_tile_id {
        CanonicalTileId::new(z, x, y).to_native()
    }

    #[test]
    // Spec coverage: BND-124.
    fn custom_mvt_vector_callbacks_invoke_fetch_and_cancel_with_copied_tile_id() {
        let fetched = Arc::new(Mutex::new(Vec::new()));
        let cancelled = Arc::new(Mutex::new(Vec::new()));
        let fetched_callback = Arc::clone(&fetched);
        let cancelled_callback = Arc::clone(&cancelled);
        let state = CustomMvtVectorSourceState::new(
            CustomMvtVectorSourceOptions::new(move |tile_id| {
                fetched_callback.lock().unwrap().push(tile_id);
            })
            .with_cancel_tile(move |tile_id| {
                cancelled_callback.lock().unwrap().push(tile_id);
            }),
        );
        let descriptor = state.descriptor();

        unsafe {
            descriptor.fetch_tile.unwrap()(descriptor.user_data, tile(1, 2, 3));
            descriptor.cancel_tile.unwrap()(descriptor.user_data, tile(4, 5, 6));
        }

        assert_eq!(
            fetched.lock().unwrap().as_slice(),
            &[CanonicalTileId::new(1, 2, 3)]
        );
        assert_eq!(
            cancelled.lock().unwrap().as_slice(),
            &[CanonicalTileId::new(4, 5, 6)]
        );
    }

    #[test]
    // Spec coverage: BND-121.
    fn custom_mvt_vector_callbacks_contain_panics() {
        let cancel_called = Arc::new(AtomicBool::new(false));
        let cancel_called_callback = Arc::clone(&cancel_called);
        let state = CustomMvtVectorSourceState::new(
            CustomMvtVectorSourceOptions::new(|_| panic!("fetch panic")).with_cancel_tile(
                move |_| {
                    cancel_called_callback.store(true, Ordering::SeqCst);
                    panic!("cancel panic");
                },
            ),
        );
        let descriptor = state.descriptor();

        unsafe {
            descriptor.fetch_tile.unwrap()(descriptor.user_data, tile(0, 0, 0));
            descriptor.cancel_tile.unwrap()(descriptor.user_data, tile(0, 0, 0));
        }

        assert!(cancel_called.load(Ordering::SeqCst));
    }

    #[test]
    // Spec coverage: BND-124.
    fn custom_mvt_vector_state_release_waits_for_active_upcalls() {
        let entered = Arc::new((Mutex::new(false), Condvar::new()));
        let release = Arc::new((Mutex::new(false), Condvar::new()));
        let closed = Arc::new(AtomicBool::new(false));
        let close_attempts = Arc::new(AtomicUsize::new(0));
        let entered_callback = Arc::clone(&entered);
        let release_callback = Arc::clone(&release);
        let state = CustomMvtVectorSourceState::new(CustomMvtVectorSourceOptions::new(move |_| {
            let (entered_lock, entered_cvar) = &*entered_callback;
            *entered_lock.lock().unwrap() = true;
            entered_cvar.notify_all();

            let (release_lock, release_cvar) = &*release_callback;
            let released = release_lock.lock().unwrap();
            let (_released, timeout) = release_cvar
                .wait_timeout_while(released, Duration::from_secs(5), |released| !*released)
                .unwrap();
            assert!(!timeout.timed_out());
        }));
        let descriptor = state.descriptor();
        let callback = descriptor.fetch_tile.unwrap();
        let user_data = descriptor.user_data as usize;

        std::thread::scope(|scope| {
            scope.spawn(move || unsafe {
                callback(user_data as *mut c_void, tile(1, 1, 1));
            });
            let (entered_lock, entered_cvar) = &*entered;
            let entered_guard = entered_lock.lock().unwrap();
            let (_entered_guard, timeout) = entered_cvar
                .wait_timeout_while(entered_guard, Duration::from_secs(5), |entered| !*entered)
                .unwrap();
            assert!(!timeout.timed_out());

            let closed_for_thread = Arc::clone(&closed);
            let close_attempts_for_thread = Arc::clone(&close_attempts);
            let state_ref = &*state;
            scope.spawn(move || {
                close_attempts_for_thread.fetch_add(1, Ordering::SeqCst);
                state_ref.close();
                closed_for_thread.store(true, Ordering::SeqCst);
            });

            std::thread::sleep(Duration::from_millis(50));
            assert_eq!(close_attempts.load(Ordering::SeqCst), 1);
            assert!(!closed.load(Ordering::SeqCst));
            let (release_lock, release_cvar) = &*release;
            *release_lock.lock().unwrap() = true;
            release_cvar.notify_all();
        });

        assert!(closed.load(Ordering::SeqCst));
        unsafe {
            descriptor.fetch_tile.unwrap()(descriptor.user_data, tile(9, 9, 9));
        }
    }
}
