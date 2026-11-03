// Generated from C headers by tools/bindgen. Do not edit.
use super::*;

native_owner! {
    /// Owns one native handle.
    ///
    /// A URL rewrite that a resource transform answers with the URL to load.
    ///
    /// See `mln_resource_transform_request` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    pub struct ResourceTransformRequestHandle(mln_resource_transform_request) dispose |raw| { unsafe { sys::mln_resource_transform_request_release(raw) }; Ok(()) };
    /// Returns the native handle value.
    fn id;
}

impl ResourceTransformRequestHandle {
    /// Answers a resource transform request with the URL to load instead.
    ///
    /// `url` must not be empty. A failed answer fails the request with
    /// `ResourceErrorReason::Other`, as
    /// `ResourceTransformRequestHandle::release()` does.
    ///
    /// See `mln_resource_transform_request_complete` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    pub fn complete(self, url: &str) -> Result<()> {
        self.consume_always(|request| {
            let mut call = Call::new(request);
            let url = call.input(&url)?;
            call.status(|request, out_diagnostic| unsafe {
                sys::mln_resource_transform_request_complete(request, url, out_diagnostic)
            })
        })
    }

    /// Fails a resource transform request without sending it.
    ///
    /// The library retries a request that failed for a reason it retries, such
    /// as `ResourceErrorReason::Connection`, and the retry calls the transform
    /// again. When this call fails, the request fails with
    /// `ResourceErrorReason::Other`, as
    /// `ResourceTransformRequestHandle::release()` does.
    ///
    /// See `mln_resource_transform_request_fail` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    pub fn fail(self, error: &ResourceTransformError<'_>) -> Result<()> {
        self.consume_always(|request| {
            let mut call = Call::new(request);
            let error = call.reference(&error)?;
            call.status(|request, out_diagnostic| unsafe {
                sys::mln_resource_transform_request_fail(request, error, out_diagnostic)
            })
        })
    }

    /// Releases a resource transform request.
    ///
    /// Releasing a request without an answer fails it with
    /// `ResourceErrorReason::Other`.
    ///
    /// See `mln_resource_transform_request_release` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    pub fn release(self) -> std::result::Result<(), (Self, Error)> {
        self.consume(|request| {
            let mut call = Call::new(request);
            call.run(|request| unsafe { sys::mln_resource_transform_request_release(request) });
            Ok(())
        })
    }
}
