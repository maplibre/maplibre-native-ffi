// Generated from C headers by tools/bindgen. Do not edit.
use super::*;

native_owner! {
    /// Owns one `mln_buffer` native handle.
    ///
    /// An owned buffer of bytes.
    ///
    /// See `mln_buffer` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    pub struct BufferHandle(mln_buffer) dispose |raw| { unsafe { sys::mln_buffer_destroy(raw) }; Ok(()) };
}

impl BufferHandle {
    /// Destroys an owned buffer. A null handle is a no-op.
    ///
    /// See `mln_buffer_destroy` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    pub fn destroy(&self) -> Result<()> {
        self.inner.close(|buffer| {
            let mut call = Call::new(buffer, None);
            call.run(|buffer| unsafe { sys::mln_buffer_destroy(buffer) });
            Ok(())
        })
    }

    /// Borrows the data stored by an owned buffer.
    ///
    /// See `mln_buffer_get` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    pub fn get(&self) -> Result<Vec<u8>> {
        let mut call = self.inner.read("mln_buffer_get")?;
        let mut out_view: sys::mln_buffer_view = unsafe { std::mem::zeroed() };
        call.status(|buffer, out_diagnostic| unsafe {
            sys::mln_buffer_get(buffer, &mut out_view, out_diagnostic)
        })?;
        Ok(unsafe { from_native(out_view) }?)
    }
}
