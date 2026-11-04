// Generated from C headers by tools/bindgen. Do not edit.
use super::*;

native_owner! {
    /// Owns one native handle.
    ///
    /// Demands render sessions' frames at a display's refreshes.
    ///
    /// See `mln_frame_pacer` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    pub struct FramePacerHandle(mln_frame_pacer) dispose |raw| { unsafe { sys::mln_frame_pacer_release(raw) }; Ok(()) };
    /// Returns the native handle value.
    fn id;
}

impl FramePacerHandle {
    /// Releases a frame pacer.
    ///
    /// Render sessions that use the pacer stay paced until you release them or
    /// set another pacer.
    ///
    /// See `mln_frame_pacer_release` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
    pub fn release(self) -> std::result::Result<(), ReleaseError<Self>> {
        self.consume(|pacer| {
            let mut call = Call::new(pacer);
            call.run(|pacer| unsafe { sys::mln_frame_pacer_release(pacer) });
            Ok(())
        })
    }
}
