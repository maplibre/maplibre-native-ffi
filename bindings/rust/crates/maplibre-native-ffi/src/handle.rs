use std::any::Any;
use std::sync::Arc;

use maplibre_native_ffi_core::{
    self as maplibre_core,
    handle::{HandleState, NativeHandle},
};

use crate::Result;
use crate::call::Call;

/// The state that a child handle keeps alive: its parent's owner.
pub(crate) type Parent = Option<Arc<dyn Any + Send + Sync>>;

/// The shared state behind one public handle type: the native handle, its
/// parent, and the disposal that `Drop` runs.
pub(crate) struct OwnerState<H: NativeHandle> {
    handle: Arc<HandleState<H>>,
    dispose: fn(H) -> Result<()>,
    _parent: Parent,
}

impl<H: NativeHandle> OwnerState<H> {
    /// Takes ownership of `raw` with its disposer.
    ///
    /// # Safety
    ///
    /// `raw` must come from an accepted ownership transfer of handle type `H`,
    /// and `dispose` must be its disposal.
    pub(crate) unsafe fn adopt(
        raw: H,
        native: &'static str,
        dispose: fn(H) -> Result<()>,
        parent: Parent,
    ) -> Result<Arc<Self>> {
        let handle = Arc::new(HandleState::new(raw, native, false)?);
        Ok(Arc::new(Self {
            handle,
            dispose,
            _parent: parent,
        }))
    }

    pub(crate) fn id(&self) -> u64 {
        self.handle.raw().to_raw()
    }

    /// This owner, for a child handle to keep alive.
    pub(crate) fn parent(self: &Arc<Self>) -> Parent
    where
        H: Send + Sync,
    {
        Some(Arc::clone(self) as Arc<dyn Any + Send + Sync>)
    }

    /// The raw id; native validates whether it still names a live handle.
    pub(crate) fn native(&self) -> H {
        self.handle.raw()
    }

    /// Prepares a call on the raw handle.
    pub(crate) fn call(&self) -> Call<'_, H> {
        Call::new(self.native())
    }

    /// Consumes the handle through `close`, which native may refuse.
    pub(crate) fn close<R>(&self, close: impl FnOnce(H) -> Result<R>) -> Result<R> {
        self.handle.close_with(close)
    }
}

impl<H: NativeHandle> Drop for OwnerState<H> {
    fn drop(&mut self) {
        if let Some(finalize) = self.handle.finalizer(self.dispose) {
            maplibre_core::callback::finalize(finalize);
        }
    }
}

/// Declares a public handle type that owns one native handle of type
/// `$native`, which `$dispose` disposes when the last reference drops. The
/// attributes before `fn id` document its `id`.
macro_rules! native_owner {
    (
        $(#[$meta:meta])*
        pub struct $name:ident($native:ident) dispose $dispose:expr;
        $(#[$id_meta:meta])*
        fn id;
    ) => {
        $(#[$meta])*
        pub struct $name {
            pub(crate) inner: std::sync::Arc<$crate::handle::OwnerState<maplibre_native_ffi_sys::$native>>,
        }
        impl std::fmt::Debug for $name {
            fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
                write!(f, "{}(id={:#x})", stringify!($name), self.id())
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
                        dispose,
                        parent,
                    )
                }?;
                Ok(Self { inner })
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
    fn a_batch_read_on_another_thread_holds_off_release() {
        let handle = Arc::new(
            HandleState::new(
                sys::mln_render_frame_batch(9),
                "mln_render_frame_batch",
                true,
            )
            .unwrap(),
        );
        let (reading, entered) = mpsc::channel();
        let (finish, finished) = mpsc::channel();
        let reader = std::thread::spawn({
            let handle = Arc::clone(&handle);
            move || {
                let read = handle.read().unwrap();
                reading.send(read.native.0).unwrap();
                finished.recv().unwrap();
            }
        });
        assert_eq!(entered.recv_timeout(Duration::from_secs(10)).unwrap(), 9);
        let error = handle
            .close_with::<()>(|_| panic!("release while output is borrowed"))
            .unwrap_err();
        assert_eq!(error.status(), crate::Status::InvalidState);
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
        let handle = HandleState::new(sys::mln_map(id), "mln_map", false).unwrap();

        // A drop inside a native callback must not dispose on that callback's
        // stack, so the disposal runs on the finalization thread, and its
        // failure reaches the reporter there.
        {
            let _callback = maplibre_core::callback::CallbackScope::enter();
            maplibre_core::callback::finalize(
                handle
                    .finalizer(|_| Err(Error::invalid_argument("dispose refused")))
                    .unwrap(),
            );
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
        assert!(handle.is_released());
    }
}
