//! One native call: its admission, its temporary input storage, and the
//! submission or status check that ends it.
//!
//! A generated operation admits a call through its owner or [`Call::global`],
//! converts each argument through the call, and makes the C call with the
//! method that matches its execution: [`Call::status`] for an immediate
//! status, [`Call::run`] for a value or no result, [`Call::command`] for an
//! ordered command, and [`Call::complete`] for any other completion.

use std::ffi::c_void;

use maplibre_native_ffi_core::{self as maplibre_core, handle::NativeHandle};
use maplibre_native_ffi_sys as sys;

use crate::Result;
use crate::completion::{self, CommandCompletion, NativeFuture};
use crate::convert::{self, InputArena, ToNative};
use crate::handle::NativeRead;

/// An admitted call on receiver `H`, or on no receiver for `()`.
pub(crate) struct Call<'a, H: NativeHandle> {
    native: H,
    // Held through the native call, and dropped in this order: a view scope
    // keeps borrowed graphics resources live, and a read keeps close away from
    // borrowed storage.
    _view: Option<ViewScope>,
    _read: Option<NativeRead<'a, H>>,
    arena: InputArena,
}

impl Call<'static, ()> {
    /// Admits a call that has no receiver.
    pub(crate) fn global(operation: &'static str) -> Result<Self> {
        maplibre_core::callback::check(operation, 0)?;
        Ok(Self::new((), None))
    }
}

impl<'a, H: NativeHandle> Call<'a, H> {
    pub(crate) fn new(native: H, read: Option<NativeRead<'a, H>>) -> Self {
        Self {
            native,
            _view: None,
            _read: read,
            arena: InputArena::default(),
        }
    }

    /// The storage that lives through the native call.
    pub(crate) fn arena(&mut self) -> &mut InputArena {
        &mut self.arena
    }

    /// Converts one argument; the result borrows `value` and this call.
    pub(crate) fn input<N, T: ToNative<N> + ?Sized>(&mut self, value: &T) -> Result<N> {
        value.to_native(&mut self.arena)
    }

    /// Converts a slice into a C array that this call owns.
    pub(crate) fn array<N: 'static, T: ToNative<N>>(&mut self, values: &[T]) -> Result<*const N> {
        convert::array(values, &mut self.arena)
    }

    /// Converts one value into a C pointer that this call owns.
    pub(crate) fn reference<N: 'static, T: ToNative<N> + ?Sized>(
        &mut self,
        value: &T,
    ) -> Result<*const N> {
        convert::reference(value, &mut self.arena)
    }

    /// Converts an optional value into a C pointer, or a null pointer.
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
        maplibre_core::check(|diagnostic| call(self.native, diagnostic))?;
        self.arena.accept_registrations();
        Ok(())
    }

    /// Makes a call that returns a value or nothing, which, as for `status`,
    /// keeps the call open.
    pub(crate) fn run<R>(&mut self, call: impl FnOnce(H) -> R) -> R {
        let result = call(self.native);
        self.arena.accept_registrations();
        result
    }

    /// Ends an ordered command, whose completion reports its own disposition.
    pub(crate) fn command(
        mut self,
        submit: impl FnOnce(H, *const sys::mln_completion, *mut sys::mln_diagnostic) -> sys::mln_status,
    ) -> Result<NativeFuture<CommandCompletion>> {
        let native = self.native;
        let future = completion::submit_command(|completion, diagnostic| {
            submit(native, completion, diagnostic)
        })?;
        self.arena.accept_registrations();
        Ok(future)
    }

    /// Ends an operation whose completion `convert` copies.
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

    /// Keeps borrowed graphics resources live until the call ends.
    ///
    /// # Safety
    ///
    /// The functions must be the matching view scope functions of `H`.
    pub(crate) unsafe fn view(
        &mut self,
        begin: unsafe extern "C" fn(
            H,
            *mut *mut c_void,
            *mut sys::mln_diagnostic,
        ) -> sys::mln_status,
        end: unsafe extern "C" fn(*mut c_void),
    ) -> Result<()> {
        let mut token = std::ptr::null_mut();
        let native = self.native;
        // SAFETY: the caller passes the scope functions of this handle type.
        maplibre_core::check(|diagnostic| unsafe { begin(native, &mut token, diagnostic) })?;
        self._view = Some(ViewScope { token, end });
        Ok(())
    }
}

struct ViewScope {
    token: *mut c_void,
    end: unsafe extern "C" fn(*mut c_void),
}

impl Drop for ViewScope {
    fn drop(&mut self) {
        // SAFETY: begin issued the token, and the scope ends it once.
        unsafe { (self.end)(self.token) };
    }
}
