// Generated from C headers by tools/bindgen. Do not edit.
use super::*;

native_owner! {
    /// Owns one native handle.
    ///
    /// A GPU device that native code creates for texture render targets.
    ///
    /// See `mln_graphics_device` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    pub struct GraphicsDeviceHandle(mln_graphics_device) dispose |raw| { unsafe { sys::mln_graphics_device_release(raw) }; Ok(()) };
    /// Returns the native handle value.
    fn id;
}

impl GraphicsDeviceHandle {
    /// Creates a graphics device for this build's render backend.
    ///
    /// See `mln_graphics_device_create` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/graphics__device_8h.html).
    pub fn create(options: Option<&GraphicsDeviceOptions>) -> Result<GraphicsDeviceHandle> {
        let mut call = Call::global();
        crate::abi::validate_abi_version()?;
        let mut out_device = sys::mln_graphics_device(0);
        let options = call.optional_reference(options.as_ref())?;
        call.status(|_, out_diagnostic| unsafe {
            sys::mln_graphics_device_create(options, &mut out_device, out_diagnostic)
        })?;
        Ok(GraphicsDeviceHandle::adopt(out_device)?)
    }

    /// Releases a graphics device.
    ///
    /// See `mln_graphics_device_release` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/graphics__device_8h.html).
    pub fn release(self) -> std::result::Result<(), (Self, Error)> {
        self.consume(|device| {
            let mut call = Call::new(device);
            call.run(|device| unsafe { sys::mln_graphics_device_release(device) });
            Ok(())
        })
    }
}
