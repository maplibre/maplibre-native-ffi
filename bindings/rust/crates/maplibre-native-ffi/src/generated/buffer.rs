// Generated from C headers by tools/bindgen. Do not edit.
use super::*;

native_owner! {
    /// Owns one `mln_buffer` native handle.
    pub struct BufferHandle(mln_buffer) dispose |raw| { unsafe { sys::mln_buffer_destroy(raw) }; Ok(()) };
}

impl BufferHandle {
    /// Calls `mln_buffer_destroy`.
    pub fn destroy(&self) -> Result<()> {
        self.inner.close(|buffer| {
            let mut call = Call::new(buffer, None);
            call.run(|buffer| unsafe { sys::mln_buffer_destroy(buffer) });
            Ok(())
        })
    }

    /// Calls `mln_buffer_get`.
    pub fn get(&self) -> Result<Vec<u8>> {
        let mut call = self.inner.read("mln_buffer_get")?;
        let mut out_view: sys::mln_buffer_view = unsafe { std::mem::zeroed() };
        call.status(|buffer, out_diagnostic| unsafe {
            sys::mln_buffer_get(buffer, &mut out_view, out_diagnostic)
        })?;
        Ok(unsafe { from_native(out_view) }?)
    }
}
