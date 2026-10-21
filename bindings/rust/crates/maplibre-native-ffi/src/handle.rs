use std::any::Any;
use std::sync::Arc;

use maplibre_native_ffi_core::{self as maplibre_core, handle::NativeHandle};

use crate::Result;
use crate::call::Call;
use crate::completion::NativeFuture;

/// The state that a child handle keeps alive: its parent's owner.
pub(crate) type Parent = Option<Arc<dyn Any + Send + Sync>>;

/// The shared state behind one public handle type: the native handle, its
/// parent, and the disposal that `Drop` runs.
pub(crate) struct OwnerState<H: NativeHandle> {
    handle: ConcurrentNativeHandle<H>,
    dispose: fn(H) -> Result<()>,
    _parent: Parent,
}

impl<H: NativeHandle> OwnerState<H> {
    /// Takes ownership of `raw` with its disposer and borrowed-output guard.
    ///
    /// # Safety
    ///
    /// `raw` must come from an accepted ownership transfer of handle type `H`,
    /// and `dispose` must be its disposal.
    pub(crate) unsafe fn adopt(
        raw: H,
        native: &'static str,
        counted: bool,
        dispose: fn(H) -> Result<()>,
        parent: Parent,
    ) -> Result<Arc<Self>> {
        // SAFETY: the caller transfers ownership of a live handle.
        let handle = unsafe { ConcurrentNativeHandle::from_handle(raw, native, counted) }?;
        Ok(Arc::new(Self {
            handle,
            dispose,
            _parent: parent,
        }))
    }

    pub(crate) fn id(&self) -> u64 {
        lock(&self.handle.state).raw().to_raw()
    }

    /// This owner, for a child handle to keep alive.
    pub(crate) fn parent(self: &Arc<Self>) -> Parent
    where
        H: Send + Sync,
    {
        Some(Arc::clone(self) as Arc<dyn Any + Send + Sync>)
    }

    pub(crate) fn hand_off(&self) {
        self.handle.hand_off();
    }

    pub(crate) fn is_closed(&self) -> bool {
        self.handle.is_closed()
    }

    /// The raw id; native validates whether it still names a live handle.
    pub(crate) fn native(&self) -> H {
        lock(&self.handle.state).raw()
    }

    /// Prepares a call on the raw handle.
    pub(crate) fn call(&self) -> Call<'_, H> {
        Call::new(self.native(), None)
    }

    /// Admits a call whose results borrow the handle's storage, and holds off
    /// close until the call ends.
    pub(crate) fn read(&self) -> Result<Call<'_, H>> {
        let read = self.handle.read_handle()?;
        let native = read.native;
        Ok(Call::new(native, Some(read)))
    }

    /// Consumes the handle through `close`.
    pub(crate) fn close<R>(&self, close: impl FnOnce(H) -> Result<R>) -> Result<R> {
        self.handle.close_with(close)
    }

    /// Consumes the handle through a completion.
    pub(crate) fn release<T: Send + 'static>(
        &self,
        close: impl FnOnce(H) -> Result<NativeFuture<T>>,
    ) -> Result<NativeFuture<T>> {
        self.handle.close_with(close)
    }
}

impl<H: NativeHandle> Drop for OwnerState<H> {
    fn drop(&mut self) {
        self.handle.finalize_with(self.dispose);
    }
}

/// Declares a public handle type that owns one native handle of type
/// `$native`, which `$dispose` disposes when the last reference drops.
macro_rules! native_owner {
    ($(#[$meta:meta])* pub struct $name:ident($native:ident) dispose $dispose:expr, counted $counted:expr;) => {
        $(#[$meta])*
        pub struct $name {
            pub(crate) inner: std::sync::Arc<$crate::handle::OwnerState<maplibre_native_ffi_sys::$native>>,
        }
        impl std::fmt::Debug for $name {
            fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
                f.debug_struct(stringify!($name))
                    .field("closed", &self.inner.is_closed())
                    .finish()
            }
        }
        impl $name {
            pub(crate) fn adopt(
                raw: maplibre_native_ffi_sys::$native,
                parent: $crate::handle::Parent,
            ) -> $crate::Result<Self> {
                let dispose: fn(maplibre_native_ffi_sys::$native) -> $crate::Result<()> = $dispose;
                // SAFETY: generated operations adopt only the handles that an
                // accepted native call transferred, with their disposal.
                let inner = unsafe {
                    $crate::handle::OwnerState::adopt(
                        raw,
                        stringify!($native),
                        $counted,
                        dispose,
                        parent,
                    )
                }?;
                Ok(Self { inner })
            }

            /// Returns the native handle value, which event sources report for
            /// this handle.
            pub fn id(&self) -> u64 {
                self.inner.id()
            }

        }
    };
}

pub(crate) use native_owner;

pub(crate) struct ConcurrentNativeHandle<T: NativeHandle> {
    state: std::sync::Mutex<maplibre_core::handle::HandleState<T>>,
    type_name: &'static str,
}

impl<T: NativeHandle> ConcurrentNativeHandle<T> {
    unsafe fn from_handle(raw: T, type_name: &'static str, counted: bool) -> Result<Self> {
        Ok(Self {
            state: std::sync::Mutex::new(maplibre_core::handle::HandleState::new(raw, counted)?),
            type_name,
        })
    }

    fn is_closed(&self) -> bool {
        lock(&self.state).is_released()
    }
    fn hand_off(&self) {
        lock(&self.state).hand_off();
    }

    fn read_handle(&self) -> Result<NativeRead<'_, T>> {
        let native = lock(&self.state).begin_read()?;
        Ok(NativeRead {
            owner: self,
            native,
        })
    }

    fn close_with<R>(&self, close: impl FnOnce(T) -> Result<R>) -> Result<R> {
        let native = lock(&self.state).begin_release()?;
        let mut reservation = CloseReservation {
            owner: self,
            accepted: false,
        };
        let result = close(native);
        reservation.accepted = result.is_ok();
        drop(reservation);
        result
    }

    fn finalize_with(&mut self, dispose: impl FnOnce(T) -> Result<()> + Send + 'static) {
        let state = self.state.get_mut().unwrap_or_else(|p| p.into_inner());
        if state.is_released() {
            return;
        }
        let id = state.raw().to_raw();
        state.hand_off();
        let type_name = self.type_name;
        maplibre_core::callback::finalize(move || {
            if dispose(T::from_raw(id)).is_err() {
                maplibre_core::handle::report_leak(maplibre_core::handle::NativeHandleLeak {
                    type_name,
                    id,
                });
            }
        });
    }
}

pub(crate) struct NativeRead<'a, T: NativeHandle> {
    owner: &'a ConcurrentNativeHandle<T>,
    pub(crate) native: T,
}
impl<T: NativeHandle> Drop for NativeRead<'_, T> {
    fn drop(&mut self) {
        lock(&self.owner.state).end_read();
    }
}

struct CloseReservation<'a, T: NativeHandle> {
    owner: &'a ConcurrentNativeHandle<T>,
    accepted: bool,
}
impl<T: NativeHandle> Drop for CloseReservation<'_, T> {
    fn drop(&mut self) {
        lock(&self.owner.state).end_release(self.accepted);
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
    fn a_batch_read_on_another_thread_holds_off_release() {
        let handle = Arc::new(unsafe {
            ConcurrentNativeHandle::from_handle(
                sys::mln_render_frame_batch(9),
                "mln_render_frame_batch",
                true,
            )
            .unwrap()
        });
        let (reading, entered) = mpsc::channel();
        let (finish, finished) = mpsc::channel();
        let reader = std::thread::spawn({
            let handle = Arc::clone(&handle);
            move || {
                let read = handle.read_handle().unwrap();
                reading.send(read.native.0).unwrap();
                finished.recv().unwrap();
            }
        });
        assert_eq!(entered.recv_timeout(Duration::from_secs(10)).unwrap(), 9);
        let error = handle
            .close_with::<()>(|_| panic!("release while output is borrowed"))
            .unwrap_err();
        assert_eq!(error.kind(), crate::ErrorKind::InvalidState);
        finish.send(()).unwrap();
        reader.join().unwrap();
        handle.close_with(|_| Ok(())).unwrap();
    }

    #[test]
    fn a_handle_dropped_in_a_callback_disposes_off_its_stack_and_reports_a_failure() {
        let (sender, leaks) = mpsc::channel();
        crate::set_reporter(Some(Box::new(move |report| {
            if let crate::Report::LeakedHandle(leak) = report {
                let _ = sender.send((leak, std::thread::current().id()));
            }
        })));
        let id = 0x0d00_0000_0000_0007;
        let mut handle = unsafe {
            ConcurrentNativeHandle::from_handle(sys::mln_map(id), "mln_map", false).unwrap()
        };

        // A drop inside a native callback must not dispose on that callback's
        // stack, so the disposal runs on the finalization thread, and its
        // failure reaches the reporter there.
        {
            let _callback = maplibre_core::callback::CallbackScope::enter();
            handle.finalize_with(|_| Err(Error::invalid_argument("dispose refused")));
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
        assert!(handle.is_closed());
    }
}
