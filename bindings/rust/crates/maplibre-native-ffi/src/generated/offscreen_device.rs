// Generated from C headers by tools/bindgen. Do not edit.
use super::*;

native_owner! {
    /// Owns one native handle.
    ///
    /// A GPU device that the library creates to render maps offscreen.
    ///
    /// See `mln_offscreen_device` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    pub struct OffscreenDeviceHandle(mln_offscreen_device) dispose |raw| { unsafe { sys::mln_offscreen_device_release(raw) }; Ok(()) };
    /// Returns the native handle value.
    fn id;
}

impl OffscreenDeviceHandle {
    /// Creates a GPU device for rendering maps offscreen.
    ///
    /// `MapHandle::attach_offscreen()` creates a device of its own when it gets
    /// none. Create one to share it among the sessions on many maps, or to pick
    /// a GPU.
    ///
    /// See `mln_offscreen_device_create` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/offscreen_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::Unsupported` when this build's render backend can't create a
    ///   device.
    /// - `Status::NativeError` when `adapter_index` names no GPU, or the
    ///   platform can't create the device.
    pub fn create(options: Option<&OffscreenDeviceOptions>) -> Result<OffscreenDeviceHandle> {
        let mut call = Call::global();
        crate::abi::validate_abi_version()?;
        let mut out_device = sys::mln_offscreen_device(0);
        let options = call.optional_reference(options.as_ref())?;
        call.status(|_, out_diagnostic| unsafe {
            sys::mln_offscreen_device_create(options, &mut out_device, out_diagnostic)
        })?;
        Ok(OffscreenDeviceHandle::adopt(out_device)?)
    }

    /// Releases an offscreen device.
    ///
    /// Render sessions on the device keep working until they detach.
    ///
    /// See `mln_offscreen_device_release` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/offscreen_8h.html).
    pub fn release(self) -> std::result::Result<(), (Self, Error)> {
        self.consume(|device| {
            let mut call = Call::new(device);
            call.run(|device| unsafe { sys::mln_offscreen_device_release(device) });
            Ok(())
        })
    }
}
