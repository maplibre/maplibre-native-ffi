// Generated from C headers by tools/bindgen. Do not edit.
use super::*;

native_owner! {
    /// Owns one `mln_event_batch` native handle.
    ///
    /// An owned batch of runtime events from one drain.
    ///
    /// See `mln_event_batch` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    pub struct EventBatchHandle(mln_event_batch) dispose |raw| { unsafe { sys::mln_event_batch_release(raw) }; Ok(()) };
}

impl EventBatchHandle {
    /// Borrows the event and message view stored by an owned event batch.
    ///
    /// See `mln_event_batch_get` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    pub fn get(&self) -> Result<RuntimeEventBatchView> {
        let mut call = self.inner.read("mln_event_batch_get")?;
        let mut out_view: sys::mln_runtime_event_batch_view = unsafe { std::mem::zeroed() };
        out_view.size = std::mem::size_of::<sys::mln_runtime_event_batch_view>() as _;
        call.status(|batch, out_diagnostic| unsafe {
            sys::mln_event_batch_get(batch, &mut out_view, out_diagnostic)
        })?;
        Ok(unsafe { from_native(out_view) }?)
    }

    /// Releases an owned event batch. A null handle is a no-op.
    ///
    /// See `mln_event_batch_release` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
    pub fn release(&self) -> Result<()> {
        self.inner.close(|batch| {
            let mut call = Call::new(batch, None);
            call.run(|batch| unsafe { sys::mln_event_batch_release(batch) });
            Ok(())
        })
    }
}
