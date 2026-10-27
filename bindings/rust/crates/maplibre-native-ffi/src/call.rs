//! Keep the receiver and input storage live through the native call. Copy
//! borrowed outputs before the call returns, or tie them to a view scope.

use std::ffi::c_void;

use maplibre_native_ffi_sys::{self as sys, NativeHandle};

use crate::Result;
use crate::completion::{self, CommandCompletion, NativeFuture};
use crate::convert::{self, InputArena, QueueSink, ToNative};

pub(crate) struct Call<'a, H: NativeHandle> {
    native: H,
    // The receiver stays alive while borrowed output is copied.
    _owner: std::marker::PhantomData<&'a H>,
    arena: InputArena<'a>,
}

impl Call<'_, ()> {
    pub(crate) fn global() -> Self {
        Self::new(())
    }
}

impl<'a, H: NativeHandle> Call<'a, H> {
    pub(crate) fn new(native: H) -> Self {
        Self {
            native,
            _owner: std::marker::PhantomData,
            arena: InputArena::default(),
        }
    }

    /// Puts the call's registrations in queue mode on `queue`, so the queue
    /// holds their handlers.
    pub(crate) fn queue(mut self, queue: &'a dyn QueueSink) -> Self {
        self.arena.set_queue(queue);
        self
    }

    pub(crate) fn input<N, T: ToNative<N> + ?Sized>(&mut self, value: &T) -> Result<N> {
        value.to_native(&mut self.arena)
    }

    pub(crate) fn array<N: 'static, T: ToNative<N>>(&mut self, values: &[T]) -> Result<*const N> {
        convert::array(values, &mut self.arena)
    }

    pub(crate) fn reference<N: 'static, T: ToNative<N> + ?Sized>(
        &mut self,
        value: &T,
    ) -> Result<*const N> {
        convert::reference(value, &mut self.arena)
    }

    pub(crate) fn optional_reference<N: 'static, T: ToNative<N> + ?Sized>(
        &mut self,
        value: Option<&T>,
    ) -> Result<*const N> {
        convert::optional_reference(value, &mut self.arena)
    }

    /// Makes a call that returns a status. The call's read and view stay
    /// open until it drops, so outputs that borrow native storage are copied
    /// before it does.
    pub(crate) fn status(
        &mut self,
        call: impl FnOnce(H, *mut sys::mln_diagnostic) -> sys::mln_status,
    ) -> Result<()> {
        crate::error::check(|diagnostic| call(self.native, diagnostic))?;
        self.arena.accept_registrations();
        Ok(())
    }

    /// Keeps registration roots pending when native reports that it kept none.
    pub(crate) fn status_registered(
        &mut self,
        call: impl FnOnce(H, *mut sys::mln_diagnostic) -> (sys::mln_status, bool),
    ) -> Result<()> {
        let mut accepted = false;
        crate::error::check(|diagnostic| {
            let (status, retained) = call(self.native, diagnostic);
            accepted = retained;
            status
        })?;
        if accepted {
            self.arena.accept_registrations();
        }
        Ok(())
    }

    /// Makes a call that returns a status, where `absent` reports that native
    /// published no output rather than an error. Returns whether the output
    /// holds a value.
    pub(crate) fn status_unless(
        &mut self,
        absent: sys::mln_status,
        call: impl FnOnce(H, *mut sys::mln_diagnostic) -> sys::mln_status,
    ) -> Result<bool> {
        let mut present = true;
        crate::error::check(|diagnostic| {
            let status = call(self.native, diagnostic);
            if status != absent {
                return status;
            }
            present = false;
            sys::MLN_STATUS_OK
        })?;
        self.arena.accept_registrations();
        Ok(present)
    }

    pub(crate) fn run<R>(&mut self, call: impl FnOnce(H) -> R) -> R {
        let result = call(self.native);
        self.arena.accept_registrations();
        result
    }

    pub(crate) fn command(
        self,
        submit: impl FnOnce(H, *const sys::mln_completion, *mut sys::mln_diagnostic) -> sys::mln_status,
    ) -> Result<NativeFuture<CommandCompletion>> {
        self.complete(submit, completion::command)
    }

    pub(crate) fn discard(
        mut self,
        submit: impl FnOnce(H, *const sys::mln_completion, *mut sys::mln_diagnostic) -> sys::mln_status,
    ) -> Result<()> {
        crate::error::check(|diagnostic| submit(self.native, std::ptr::null(), diagnostic))?;
        self.arena.accept_registrations();
        Ok(())
    }

    pub(crate) fn complete<T: Send + 'static>(
        mut self,
        submit: impl FnOnce(H, *const sys::mln_completion, *mut sys::mln_diagnostic) -> sys::mln_status,
        convert: impl FnOnce(&sys::mln_completion_result) -> Result<T> + Send + 'static,
    ) -> Result<NativeFuture<T>> {
        let native = self.native;
        let future = completion::submit(
            |completion, diagnostic| submit(native, completion, diagnostic),
            convert,
        )?;
        self.arena.accept_registrations();
        Ok(future)
    }
}

/// A native view scope, which keeps the native objects that its owner's
/// getters lend valid until it drops. Ending it is allowed on any thread.
pub(crate) struct ViewScope {
    token: *mut c_void,
    end: unsafe extern "C" fn(*mut c_void),
}

// SAFETY: native ends a scope on any thread, and the token is only passed back
// to that end.
unsafe impl Send for ViewScope {}
// SAFETY: a shared scope exposes nothing.
unsafe impl Sync for ViewScope {}

impl ViewScope {
    /// Begins a scope on `native`.
    ///
    /// # Safety
    ///
    /// The functions must be the matching view scope functions of `H`.
    pub(crate) unsafe fn begin<H: NativeHandle>(
        native: H,
        begin: unsafe extern "C" fn(
            H,
            *mut *mut c_void,
            *mut sys::mln_diagnostic,
        ) -> sys::mln_status,
        end: unsafe extern "C" fn(*mut c_void),
    ) -> Result<Self> {
        let mut token = std::ptr::null_mut();
        // SAFETY: the caller passes the scope functions of this handle type.
        crate::error::check(|diagnostic| unsafe { begin(native, &mut token, diagnostic) })?;
        Ok(Self { token, end })
    }
}

impl Drop for ViewScope {
    fn drop(&mut self) {
        // SAFETY: begin issued the token, and the scope ends it once.
        unsafe { (self.end)(self.token) };
    }
}

/// A value that a view getter lends, valid until its view drops.
///
/// It reads as the value itself, and its borrow keeps the view open while the
/// value is held. The native objects that it names, such as a backend
/// texture, stay valid only while the view lives. Their names are plain data
/// that can still be copied out, so a name used after the view ends is the
/// fault of the unsafe graphics call that uses it.
pub struct Lent<'v, T> {
    value: T,
    _view: std::marker::PhantomData<&'v ()>,
}

impl<T> Lent<'_, T> {
    pub(crate) fn new(value: T) -> Self {
        Self {
            value,
            _view: std::marker::PhantomData,
        }
    }
}

impl<T> std::ops::Deref for Lent<'_, T> {
    type Target = T;

    fn deref(&self) -> &T {
        &self.value
    }
}

impl<T: std::fmt::Debug> std::fmt::Debug for Lent<'_, T> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        self.value.fmt(f)
    }
}
