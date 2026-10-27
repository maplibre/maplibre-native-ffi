// Generated from C headers by tools/bindgen. Do not edit.
use super::*;

native_owner! {
    /// Owns one native handle.
    ///
    /// A resource request that a resource provider handles.
    ///
    /// See `mln_resource_request` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    pub struct ResourceRequestHandle(mln_resource_request) dispose |raw| { unsafe { sys::mln_resource_request_release(raw) }; Ok(()) };
    /// Returns the native handle value.
    fn id;
}

impl ResourceRequestHandle {
    /// Completes a C API resource provider request and consumes its handle.
    ///
    /// See `mln_resource_request_complete` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
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

    /// Reports whether MapLibre has cancelled a C API resource provider
    /// request.
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

    /// Hands an outstanding provider request to the built-in network loader and
    /// consumes its handle.
    ///
    /// See `mln_resource_request_pass_through` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    pub fn pass_through(self) -> std::result::Result<(), (Self, Error)> {
        self.consume(|handle| {
            let mut call = Call::new(handle);
            call.status(|handle, out_diagnostic| unsafe {
                sys::mln_resource_request_pass_through(handle, out_diagnostic)
            })
        })
    }

    /// Releases the provider's resource request handle without answering it.
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

    /// Registers a callback that runs when MapLibre cancels a C API resource
    /// provider request. The callback runs on the thread that discards the
    /// request, never a host thread: the runtime's native scheduler thread when
    /// a committed map or runtime command discards it, such as a style change
    /// or map destruction, and a MapLibre worker thread otherwise.
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
