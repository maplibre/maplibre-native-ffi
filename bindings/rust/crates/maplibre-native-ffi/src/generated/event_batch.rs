// Generated from C headers by tools/bindgen. Do not edit.
use super::*;

native_owner! {
    /// Owns one `mln_event_batch` native handle.
    pub struct EventBatchHandle(mln_event_batch) dispose |raw| { unsafe { sys::mln_event_batch_release(raw) }; Ok(()) };
}

impl EventBatchHandle {
    /// Calls `mln_event_batch_get`.
    pub fn get(&self) -> Result<RuntimeEventBatchView> {
        let mut call = self.inner.read("mln_event_batch_get")?;
        let mut out_view: sys::mln_runtime_event_batch_view = unsafe { std::mem::zeroed() };
        out_view.size = std::mem::size_of::<sys::mln_runtime_event_batch_view>() as _;
        call.status(|batch, out_diagnostic| unsafe {
            sys::mln_event_batch_get(batch, &mut out_view, out_diagnostic)
        })?;
        Ok(unsafe { from_native(out_view) }?)
    }

    /// Calls `mln_event_batch_release`.
    pub fn release(&self) -> Result<()> {
        self.inner.close(|batch| {
            let mut call = Call::new(batch, None);
            call.run(|batch| unsafe { sys::mln_event_batch_release(batch) });
            Ok(())
        })
    }
}
