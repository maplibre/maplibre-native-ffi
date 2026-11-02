// Generated from C headers by tools/bindgen. Do not edit.
use super::*;

native_owner! {
    /// Owns one native handle.
    ///
    /// A resource load that a resource provider answers.
    ///
    /// See `mln_resource_request` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    pub struct ResourceRequestHandle(mln_resource_request) dispose |raw| { unsafe { sys::mln_resource_request_release(raw) }; Ok(()) };
    /// Returns the native handle value.
    fn id;
}

impl ResourceRequestHandle {
    /// Answers a resource request.
    ///
    /// See `mln_resource_request_complete` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::InvalidState` when the library cancelled the request.
    pub fn complete(
        self,
        response: &ResourceResponse<'_>,
    ) -> std::result::Result<(), (Self, Error)> {
        self.consume(|handle| {
            let mut call = Call::new(handle);
            let response = call.reference(&response)?;
            call.status(|handle, out_diagnostic| unsafe {
                sys::mln_resource_request_complete(handle, response, out_diagnostic)
            })
        })
    }

    /// Reads whether the library cancelled a resource request.
    ///
    /// See `mln_resource_request_is_cancelled` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    pub fn is_cancelled(&self) -> Result<bool> {
        let mut call = self.inner.call();
        let mut out_cancelled: bool = Default::default();
        call.status(|handle, out_diagnostic| unsafe {
            sys::mln_resource_request_is_cancelled(handle, &mut out_cancelled, out_diagnostic)
        })?;
        Ok(out_cancelled)
    }

    /// Hands a resource request back to the library to load as usual.
    ///
    /// See `mln_resource_request_pass_through` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::InvalidState` when the library cancelled the request.
    pub fn pass_through(self) -> std::result::Result<(), (Self, Error)> {
        self.consume(|handle| {
            let mut call = Call::new(handle);
            call.status(|handle, out_diagnostic| unsafe {
                sys::mln_resource_request_pass_through(handle, out_diagnostic)
            })
        })
    }

    /// Releases a resource request.
    ///
    /// See `mln_resource_request_release` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    pub fn release(self) -> std::result::Result<(), (Self, Error)> {
        self.consume(|handle| {
            let mut call = Call::new(handle);
            call.run(|handle| unsafe { sys::mln_resource_request_release(handle) });
            Ok(())
        })
    }

    /// Calls a callback when the library cancels a resource request.
    ///
    /// The callback never runs after the request is answered. A request takes
    /// one cancel callback. When the library has already cancelled the request,
    /// the call reports that instead and the callback never runs.
    ///
    /// See `mln_resource_request_set_cancel_callback` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    pub fn set_cancel_callback(&self, handler: ResourceRequestCancelHandler) -> Result<bool> {
        let mut call = self.inner.call();
        let mut out_cancelled: bool = Default::default();
        let handler = call.reference(&handler)?;
        call.status_registered(|handle, out_diagnostic| {
            let status = unsafe {
                sys::mln_resource_request_set_cancel_callback(
                    handle,
                    handler,
                    &mut out_cancelled,
                    out_diagnostic,
                )
            };
            (status, !out_cancelled)
        })?;
        Ok(out_cancelled)
    }
}
