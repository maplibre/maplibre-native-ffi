// Generated from C headers by tools/bindgen. Do not edit.
use super::*;

native_owner! {
    /// Owns one `mln_resource_transform_request` native handle.
    ///
    /// A resource URL transform that waits for its host's answer.
    ///
    /// See `mln_resource_transform_request` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    pub struct ResourceTransformRequestHandle(mln_resource_transform_request) dispose |raw| { unsafe { sys::mln_resource_transform_request_release(raw) }; Ok(()) };
    /// Returns the native handle value.
    fn id;
}

impl ResourceTransformRequestHandle {
    /// Answers a resource transform request with a replacement URL and consumes
    /// its handle.
    ///
    /// See `mln_resource_transform_request_complete` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    pub fn complete(self, url: &str) -> std::result::Result<(), (Self, Error)> {
        self.inner
            .close(|request| {
                let mut call = Call::new(request);
                let url = call.input(&url)?;
                call.status(|request, out_diagnostic| unsafe {
                    sys::mln_resource_transform_request_complete(request, url, out_diagnostic)
                })
            })
            .map_err(|error| (self, error))
    }

    /// Releases a resource transform request without answering it.
    ///
    /// See `mln_resource_transform_request_release` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    pub fn release(self) -> std::result::Result<(), (Self, Error)> {
        self.inner
            .close(|request| {
                let mut call = Call::new(request);
                call.run(|request| unsafe { sys::mln_resource_transform_request_release(request) });
                Ok(())
            })
            .map_err(|error| (self, error))
    }
}
