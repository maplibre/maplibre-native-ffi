pub use maplibre_native_ffi_sys::NativeHandle;

use crate::report::{Report, report};

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
    report(Report::LeakedHandle(leak));
}

/// Shared ownership bookkeeping for a generational native id.
#[derive(Debug)]
pub struct HandleState<T: NativeHandle> {
    raw: u64,
    _typed: std::marker::PhantomData<fn() -> T>,
    released: bool,
    counted: bool,
    readers: usize,
    releasing: bool,
}

impl<T: NativeHandle> HandleState<T> {
    pub fn new(raw: T, counted: bool) -> crate::Result<Self> {
        if raw.to_raw() == 0 {
            return Err(crate::Error::invalid_argument(
                "native handle must not be zero",
            ));
        }
        Ok(Self {
            raw: raw.to_raw(),
            _typed: std::marker::PhantomData,
            released: false,
            counted,
            readers: 0,
            releasing: false,
        })
    }

    pub fn raw(&self) -> T {
        T::from_raw(self.raw)
    }
    pub fn is_released(&self) -> bool {
        self.released
    }
    pub fn hand_off(&mut self) {
        self.released = true;
    }

    /// Counts a copy from borrowed output storage that has no native view scope.
    pub fn begin_read(&mut self) -> crate::Result<T> {
        if self.releasing {
            return Err(Self::in_use());
        }
        self.readers += 1;
        Ok(T::from_raw(self.raw))
    }

    pub fn end_read(&mut self) {
        self.readers -= 1;
    }

    /// Reserves only storage whose readers native cannot observe.
    pub fn begin_release(&mut self) -> crate::Result<T> {
        if self.readers != 0 || self.releasing {
            return Err(Self::in_use());
        }
        self.releasing = self.counted;
        Ok(T::from_raw(self.raw))
    }

    pub fn end_release(&mut self, accepted: bool) {
        self.released |= accepted;
        self.releasing = false;
    }

    fn in_use() -> crate::Error {
        crate::Error::new(
            crate::ErrorKind::InvalidState,
            None,
            "borrowed output is in use",
        )
    }
}
