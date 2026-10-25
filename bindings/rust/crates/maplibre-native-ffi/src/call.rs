//! Keep the receiver and input storage live through the native call. Copy
//! borrowed outputs before the view scope ends.

use std::ffi::c_void;

use maplibre_native_ffi_core::{self as maplibre_core, handle::NativeHandle};
use maplibre_native_ffi_sys as sys;

use crate::Result;
use crate::completion::{self, CommandCompletion, NativeFuture};
use crate::convert::{self, InputArena, ToNative};

pub(crate) struct Call<'a, H: NativeHandle> {
    native: H,
    // The receiver and its parent stay alive while borrowed output is copied.
    _owner: std::marker::PhantomData<&'a H>,
    // Held through the native call; a view scope keeps borrowed graphics
    // resources live.
    _view: Option<ViewScope>,
    arena: InputArena,
}

impl Call<'static, ()> {
    pub(crate) fn global() -> Self {
        Self::new(())
    }
}

impl<'a, H: NativeHandle> Call<'a, H> {
    pub(crate) fn new(native: H) -> Self {
        Self {
            native,
            _owner: std::marker::PhantomData,
            _view: None,
            arena: InputArena::default(),
        }
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
        maplibre_core::check(|diagnostic| call(self.native, diagnostic))?;
        self.arena.accept_registrations();
        Ok(())
    }

    /// Keeps registration roots pending when native reports that it kept none.
    pub(crate) fn status_registered(
        &mut self,
        call: impl FnOnce(H, *mut sys::mln_diagnostic) -> (sys::mln_status, bool),
    ) -> Result<()> {
        let mut accepted = false;
        maplibre_core::check(|diagnostic| {
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
        maplibre_core::check(|diagnostic| {
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
        maplibre_core::check(|diagnostic| submit(self.native, std::ptr::null(), diagnostic))?;
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
