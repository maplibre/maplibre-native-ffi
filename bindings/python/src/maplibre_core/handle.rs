use std::sync::{Arc, Mutex, MutexGuard};

pub use maplibre_native_ffi_sys::NativeHandle;

use std::io::Write;

/// A native handle a destructor attempted to destroy and could not. The handle
/// stays live.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct NativeHandleLeak {
    /// The native type name, such as `mln_map`.
    pub type_name: &'static str,
    /// The handle id that was not destroyed.
    pub id: u64,
}

/// Reports a handle a destructor could not destroy. Never panics: it is called
/// from `Drop`, where unwinding would abort.
pub fn report_leak(leak: NativeHandleLeak) {
    let _ = writeln!(
        std::io::stderr(),
        "maplibre-native-ffi: Leaked {} native handle {:#x}; close it explicitly.",
        leak.type_name,
        leak.id
    );
}

/// Ownership bookkeeping for a generational native id. Native serializes
/// calls and releases, so this records only whether ownership has ended.
#[derive(Debug)]
pub struct HandleState<T: NativeHandle> {
    raw: u64,
    _typed: std::marker::PhantomData<fn() -> T>,
    type_name: &'static str,
    released: Mutex<bool>,
}

fn lock<T>(mutex: &Mutex<T>) -> MutexGuard<'_, T> {
    mutex.lock().unwrap_or_else(|p| p.into_inner())
}

impl<T: NativeHandle> HandleState<T> {
    /// Records an owned id.
    pub fn new(raw: T, type_name: &'static str) -> super::Result<Self> {
        if raw.to_raw() == 0 {
            return Err(super::Error::invalid_argument(
                "native handle must not be zero",
            ));
        }
        Ok(Self {
            raw: raw.to_raw(),
            _typed: std::marker::PhantomData,
            type_name,
            released: Mutex::new(false),
        })
    }

    /// Returns the id, including after ownership ends; native validates it.
    pub fn raw(&self) -> T {
        T::from_raw(self.raw)
    }

    /// Reports whether native accepted a release or another consuming call.
    pub fn is_released(&self) -> bool {
        *lock(&self.released)
    }

    /// Starts a consuming call, which ends ownership once committed.
    pub fn reserve(self: &Arc<Self>) -> ReleaseReservation<T> {
        ReleaseReservation {
            owner: Arc::clone(self),
            accepted: false,
        }
    }

    /// Transfers an unreleased handle to disposal and reports a refused disposal.
    /// The caller runs this closure where its host can safely call native.
    pub fn finalizer(
        &self,
        dispose: impl FnOnce(T) -> super::Result<()> + Send + 'static,
    ) -> Option<impl FnOnce() + Send + 'static> {
        let mut released = lock(&self.released);
        if *released {
            return None;
        }
        *released = true;
        let id = self.raw;
        let type_name = self.type_name;
        Some(move || {
            if dispose(T::from_raw(id)).is_err() {
                report_leak(NativeHandleLeak { type_name, id });
            }
        })
    }
}

/// A consuming call in progress, which ends ownership once committed.
pub struct ReleaseReservation<T: NativeHandle> {
    owner: Arc<HandleState<T>>,
    accepted: bool,
}
impl<T: NativeHandle> ReleaseReservation<T> {
    pub fn handle(&self) -> T {
        self.owner.raw()
    }
    pub fn commit(&mut self) {
        self.accepted = true;
    }
}
impl<T: NativeHandle> Drop for ReleaseReservation<T> {
    fn drop(&mut self) {
        *lock(&self.owner.released) |= self.accepted;
    }
}
