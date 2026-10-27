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

/// Shared ownership bookkeeping for a generational native id.
///
/// Counted owners protect copies from borrowed output that native cannot see.
/// A release fails with INVALID_STATE during a copy, and new reads fail while
/// that release runs. Other owners let native serialize calls and releases.
#[derive(Debug)]
pub struct HandleState<T: NativeHandle> {
    raw: u64,
    _typed: std::marker::PhantomData<fn() -> T>,
    type_name: &'static str,
    tracks_borrowed_output: bool,
    state: Mutex<Ownership>,
}

#[derive(Debug, Default)]
struct Ownership {
    released: bool,
    readers: usize,
    releasing: bool,
}

fn lock<T>(mutex: &Mutex<T>) -> MutexGuard<'_, T> {
    mutex.lock().unwrap_or_else(|p| p.into_inner())
}

impl<T: NativeHandle> HandleState<T> {
    /// Records an owned id and whether copies need a read guard.
    pub fn new(
        raw: T,
        type_name: &'static str,
        tracks_borrowed_output: bool,
    ) -> super::Result<Self> {
        if raw.to_raw() == 0 {
            return Err(super::Error::invalid_argument(
                "native handle must not be zero",
            ));
        }
        Ok(Self {
            raw: raw.to_raw(),
            _typed: std::marker::PhantomData,
            type_name,
            tracks_borrowed_output,
            state: Mutex::new(Ownership::default()),
        })
    }

    /// Returns the id, including after ownership ends; native validates it.
    pub fn raw(&self) -> T {
        T::from_raw(self.raw)
    }

    /// Reports whether native accepted a release or another consuming call.
    pub fn is_released(&self) -> bool {
        lock(&self.state).released
    }

    /// Holds off release until borrowed output has been copied.
    pub fn read(self: &Arc<Self>) -> super::Result<NativeRead<T>> {
        let mut state = lock(&self.state);
        if state.releasing {
            return Err(Self::in_use());
        }
        state.readers += 1;
        Ok(NativeRead {
            owner: Arc::clone(self),
            native: self.raw(),
        })
    }

    /// Fails during a borrowed-output copy; counted owners also block new reads
    /// until the returned guard drops. Native serializes other releases.
    pub fn reserve(self: &Arc<Self>) -> super::Result<ReleaseReservation<T>> {
        let mut state = lock(&self.state);
        self.reserve_locked(&mut state)
    }

    fn reserve_locked(
        self: &Arc<Self>,
        state: &mut Ownership,
    ) -> super::Result<ReleaseReservation<T>> {
        if state.readers != 0 || state.releasing {
            return Err(Self::in_use());
        }
        state.releasing = self.tracks_borrowed_output;
        Ok(ReleaseReservation {
            owner: Arc::clone(self),
            accepted: false,
        })
    }

    /// Marks ownership released only when native accepts the consuming call.
    pub fn close_with<R>(
        self: &Arc<Self>,
        close: impl FnOnce(T) -> super::Result<R>,
    ) -> super::Result<R> {
        let mut reservation = self.reserve()?;
        let result = close(self.raw());
        if result.is_ok() {
            reservation.commit();
        }
        result
    }

    /// Transfers an unreleased handle to disposal and reports a refused disposal.
    /// The caller schedules this closure where its host can safely call native.
    pub fn finalizer(
        &self,
        dispose: impl FnOnce(T) -> super::Result<()> + Send + 'static,
    ) -> Option<impl FnOnce() + Send + 'static> {
        let mut state = lock(&self.state);
        if state.released {
            return None;
        }
        state.released = true;
        let id = self.raw;
        let type_name = self.type_name;
        Some(move || {
            if dispose(T::from_raw(id)).is_err() {
                report_leak(NativeHandleLeak { type_name, id });
            }
        })
    }

    fn in_use() -> super::Error {
        super::Error::new(super::Status::InvalidState, "borrowed output is in use")
    }
}

/// A borrowed-output copy in progress; dropping it permits release again.
pub struct NativeRead<T: NativeHandle> {
    owner: Arc<HandleState<T>>,
    pub native: T,
}
impl<T: NativeHandle> Drop for NativeRead<T> {
    fn drop(&mut self) {
        lock(&self.owner.state).readers -= 1;
    }
}

/// A consuming call in progress, with rollback when native refuses it.
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
        let mut state = lock(&self.owner.state);
        state.released |= self.accepted;
        state.releasing = false;
    }
}
