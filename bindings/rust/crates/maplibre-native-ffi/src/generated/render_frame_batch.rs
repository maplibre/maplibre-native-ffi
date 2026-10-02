// Generated from C headers by tools/bindgen. Do not edit.
use super::*;

native_owner! {
    /// Owns one `mln_render_frame_batch` native handle.
    pub struct RenderFrameBatchHandle(mln_render_frame_batch) dispose |raw| { unsafe { sys::mln_render_frame_batch_release(raw) }; Ok(()) };
}

impl RenderFrameBatchHandle {
    /// Calls `mln_render_frame_batch_count`.
    pub fn count(&self) -> Result<usize> {
        let mut call = self.inner.call("mln_render_frame_batch_count")?;
        let mut out_count: usize = Default::default();
        call.status(|batch, out_diagnostic| unsafe {
            sys::mln_render_frame_batch_count(batch, &mut out_count, out_diagnostic)
        })?;
        Ok(out_count)
    }

    /// Calls `mln_render_frame_batch_get`.
    pub fn get(&self, index: usize) -> Result<RenderFrameResult> {
        let mut call = self.inner.call("mln_render_frame_batch_get")?;
        let mut out_result: sys::mln_render_frame_result = unsafe { std::mem::zeroed() };
        out_result.size = std::mem::size_of::<sys::mln_render_frame_result>() as _;
        call.status(|batch, out_diagnostic| unsafe {
            sys::mln_render_frame_batch_get(batch, index, &mut out_result, out_diagnostic)
        })?;
        Ok(unsafe { from_native(out_result) }?)
    }

    /// Calls `mln_render_frame_batch_release`.
    pub fn release(&self) -> Result<()> {
        self.inner.close(|batch| {
            let mut call = Call::new(batch, None);
            call.run(|batch| unsafe { sys::mln_render_frame_batch_release(batch) });
            Ok(())
        })
    }
}
