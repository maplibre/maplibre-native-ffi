// Generated from C headers by tools/bindgen. Do not edit.
use super::*;

native_owner! {
    /// Owns one `mln_render_frame_batch` native handle.
    ///
    /// An owned batch of frame results from one drain.
    ///
    /// See `mln_render_frame_batch` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    pub struct RenderFrameBatchHandle(mln_render_frame_batch) dispose |raw| { unsafe { sys::mln_render_frame_batch_release(raw) }; Ok(()) };
}

impl RenderFrameBatchHandle {
    /// Borrows the result view stored by an owned frame-result batch.
    ///
    /// See `mln_render_frame_batch_get` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    pub fn get(&self) -> Result<RenderFrameBatchView> {
        let mut call = self.inner.read("mln_render_frame_batch_get")?;
        let mut out_view: sys::mln_render_frame_batch_view = unsafe { std::mem::zeroed() };
        out_view.size = std::mem::size_of::<sys::mln_render_frame_batch_view>() as _;
        call.status(|batch, out_diagnostic| unsafe {
            sys::mln_render_frame_batch_get(batch, &mut out_view, out_diagnostic)
        })?;
        Ok(unsafe { from_native(out_view) }?)
    }

    /// Releases a frame-result batch.
    ///
    /// See `mln_render_frame_batch_release` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    pub fn release(&self) -> Result<()> {
        self.inner.close(|batch| {
            let mut call = Call::new(batch, None);
            call.run(|batch| unsafe { sys::mln_render_frame_batch_release(batch) });
            Ok(())
        })
    }
}
