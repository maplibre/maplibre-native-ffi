use maplibre_native_ffi_sys::NativeHandle;

use crate::Result;
use crate::call::Call;
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

/// The one owner of a native handle, which `Drop` disposes.
///
/// Native keeps a handle's parent alive for as long as the handle needs it,
/// and serializes calls and releases on the handle, so an owner holds only the
/// id.
pub(crate) struct Owner<H: NativeHandle + Send> {
    native: H,
    dispose: fn(H) -> Result<()>,
    type_name: &'static str,
}

impl<H: NativeHandle + Send> Owner<H> {
    /// Takes ownership of `native` with its disposer.
    ///
    /// # Safety
    ///
    /// `native` must come from an accepted ownership transfer of handle type
    /// `H`, and `dispose` must be its disposal.
    pub(crate) unsafe fn adopt(
        native: H,
        type_name: &'static str,
        dispose: fn(H) -> Result<()>,
    ) -> Result<Self> {
        if native.to_raw() == 0 {
            return Err(crate::Error::invalid_argument(
                "native handle must not be zero",
            ));
        }
        Ok(Self {
            native,
            dispose,
            type_name,
        })
    }

    pub(crate) fn id(&self) -> u64 {
        self.native.to_raw()
    }

    /// The raw id; native validates whether it still names a live handle.
    pub(crate) fn native(&self) -> H {
        self.native
    }

    /// Prepares a call on the raw handle.
    pub(crate) fn call(&self) -> Call<'_, H> {
        Call::new(self.native)
    }
}

impl<H: NativeHandle + Send> Drop for Owner<H> {
    fn drop(&mut self) {
        let (native, dispose, type_name) = (self.native, self.dispose, self.type_name);
        crate::finalize::finalize(move || {
            if dispose(native).is_err() {
                report(Report::LeakedHandle(NativeHandleLeak {
                    type_name,
                    id: native.to_raw(),
                }));
            }
        });
    }
}

/// Declares a public handle type that owns one native handle of type
/// `$native`, which `$dispose` disposes when the owner drops. The attributes
/// before `fn id` document its `id`.
macro_rules! native_owner {
    (
        $(#[$meta:meta])*
        pub struct $name:ident($native:ident) dispose $dispose:expr;
        $(#[$id_meta:meta])*
        fn id;
    ) => {
        $(#[$meta])*
        pub struct $name {
            pub(crate) inner: $crate::handle::Owner<maplibre_native_ffi_sys::$native>,
        }
        impl std::fmt::Debug for $name {
            fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
                write!(f, "{}(id={:#x})", stringify!($name), self.id())
            }
        }
        impl $name {
            pub(crate) fn adopt(raw: maplibre_native_ffi_sys::$native) -> $crate::Result<Self> {
                let dispose: fn(maplibre_native_ffi_sys::$native) -> $crate::Result<()> = $dispose;
                // SAFETY: generated operations adopt only the handles that an
                // accepted native call transferred, with their disposal.
                let inner = unsafe {
                    $crate::handle::Owner::adopt(raw, stringify!($native), dispose)
                }?;
                Ok(Self { inner })
            }

            /// Runs a consuming call, which ends this owner once native
            /// accepts it and hands it back otherwise.
            pub(crate) fn consume<R>(
                self,
                close: impl FnOnce(maplibre_native_ffi_sys::$native) -> $crate::Result<R>,
            ) -> std::result::Result<R, (Self, $crate::Error)> {
                match close(self.inner.native()) {
                    Ok(value) => {
                        // Native took the handle, so nothing is left to dispose.
                        std::mem::forget(self);
                        Ok(value)
                    }
                    Err(error) => Err((self, error)),
                }
            }

            $(#[$id_meta])*
            pub fn id(&self) -> u64 {
                self.inner.id()
            }
        }
    };
}

pub(crate) use native_owner;

/// A batch that a drain published, which drop releases once its view is
/// copied, whether or not the copy succeeds.
pub(crate) struct Drained<H: Copy> {
    raw: H,
    release: unsafe extern "C" fn(H),
}

impl<H: Copy> Drained<H> {
    /// # Safety
    ///
    /// Native must have published `raw` to this caller, and `release` must be
    /// its release, which nothing else calls.
    pub(crate) unsafe fn new(raw: H, release: unsafe extern "C" fn(H)) -> Self {
        Self { raw, release }
    }
}

impl<H: Copy> Drop for Drained<H> {
    fn drop(&mut self) {
        // SAFETY: `new` received the only claim on the batch.
        unsafe { (self.release)(self.raw) }
    }
}

pub(crate) fn lock<T>(mutex: &std::sync::Mutex<T>) -> std::sync::MutexGuard<'_, T> {
    mutex.lock().unwrap_or_else(|p| p.into_inner())
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::Error;
    use maplibre_native_ffi_sys as sys;
    use std::sync::mpsc;
    use std::time::Duration;

    #[test]
    fn a_handle_dropped_in_a_callback_disposes_off_its_stack_and_reports_a_failure() {
        let (sender, leaks) = mpsc::channel();
        crate::set_reporter(Some(Box::new(move |report| {
            if let crate::Report::LeakedHandle(leak) = report {
                let _ = sender.send((leak, std::thread::current().id()));
            }
        })));
        let id = 0x0d00_0000_0000_0007;
        // SAFETY: the disposer never reaches native.
        let owner = unsafe {
            Owner::adopt(sys::mln_map(id), "mln_map", |_| {
                Err(Error::invalid_argument("dispose refused"))
            })
        }
        .unwrap();

        // A drop inside a native callback must not dispose on that callback's
        // stack, so the disposal runs on the finalization thread, and its
        // failure reaches the reporter there.
        {
            let _callback = crate::finalize::CallbackScope::enter();
            drop(owner);
        }
        let received = leaks.recv_timeout(Duration::from_secs(10));
        crate::set_reporter(None);

        let (leak, thread) = received.expect("the failed disposal was never reported");
        assert_eq!(
            leak,
            crate::NativeHandleLeak {
                type_name: "mln_map",
                id,
            }
        );
        assert_ne!(thread, std::thread::current().id());
    }
}
