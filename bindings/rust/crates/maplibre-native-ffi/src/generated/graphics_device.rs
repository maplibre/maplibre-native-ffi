// Generated from C headers by tools/bindgen. Do not edit.
use super::*;

native_owner! {
    /// Owns one native handle.
    ///
    /// A GPU device that the library creates to render maps into textures.
    ///
    /// See `mln_graphics_device` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    pub struct GraphicsDeviceHandle(mln_graphics_device) dispose |raw| { unsafe { sys::mln_graphics_device_release(raw) }; Ok(()) };
    /// Returns the native handle value.
    fn id;
}

impl GraphicsDeviceHandle {
    /// Creates a GPU device for rendering maps into textures.
    ///
    /// Render sessions on several maps can share one device.
    ///
    /// See `mln_graphics_device_create` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/graphics__device_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::Unsupported` when this build's render backend can't create a
    ///   device.
    /// - `Status::NativeError` when `adapter_index` names no GPU, or the
    ///   platform can't create the device.
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
    /// Render sessions on the device keep working until they detach.
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
