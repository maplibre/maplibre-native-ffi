// Generated from C headers by tools/bindgen. Do not edit.
use super::*;

native_owner! {
    /// Owns one native handle.
    ///
    /// A Vulkan instance, device, and queue that the library creates and owns.
    ///
    /// See `mln_vulkan_device` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    pub struct VulkanDeviceHandle(mln_vulkan_device) dispose |raw| { unsafe { sys::mln_vulkan_device_release(raw) }; Ok(()) };
    /// Returns the native handle value.
    fn id;
}

impl VulkanDeviceHandle {
    /// Creates a Vulkan instance, device, and graphics queue for render
    /// sessions.
    ///
    /// Attach sessions with the context from
    /// `VulkanDeviceHandle::get_context()`, in place of a device of your own.
    /// Without `device_uuid` or `device_luid`, it prefers a discrete GPU. The
    /// device enables the external memory and semaphore extensions that the GPU
    /// offers.
    ///
    /// See `mln_vulkan_device_create` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/vulkan_8h.html).
    ///
    /// # Errors
    ///
    /// - `Status::Unsupported` when `supported_render_backend_mask()` lacks
    ///   `RenderBackendFlag::VULKAN`, or when `presentation` is true and the
    ///   Vulkan loader or the GPU can't present.
    /// - `Status::NotFound` when no GPU matches `device_uuid` and
    ///   `device_luid`.
    /// - `Status::NativeError` when there is no Vulkan loader, no GPU has a
    ///   graphics queue, or Vulkan can't create the instance or the device.
    #[cfg(any(
        target_os = "android",
        target_os = "freebsd",
        target_os = "linux",
        target_os = "macos",
        target_os = "windows"
    ))]
    pub fn create(options: Option<&VulkanDeviceOptions<'_>>) -> Result<VulkanDeviceHandle> {
        let mut call = Call::global();
        crate::abi::validate_abi_version()?;
        let mut out_device = sys::mln_vulkan_device(0);
        let options = call.optional_reference(options.as_ref())?;
        call.status(|_, out_diagnostic| unsafe {
            sys::mln_vulkan_device_create(options, &mut out_device, out_diagnostic)
        })?;
        Ok(VulkanDeviceHandle::adopt(out_device)?)
    }

    /// Gets the context to attach render sessions to the device with.
    ///
    /// Pass it as the context of `MapHandle::attach_vulkan_surface()` or
    /// `MapHandle::attach_vulkan_borrowed_texture()`. The sessions on the
    /// device share its graphics queue. To submit to the queue yourself, set
    /// `RenderSessionAttachOptions::queue_lock` on each session.
    ///
    /// See `mln_vulkan_device_get_context` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/vulkan_8h.html).
    #[cfg(any(
        target_os = "android",
        target_os = "freebsd",
        target_os = "linux",
        target_os = "macos",
        target_os = "windows"
    ))]
    pub fn get_context(&self) -> Result<VulkanDeviceContextResult> {
        let mut call = self.inner.call();
        let mut out_result: sys::mln_vulkan_device_context_result = unsafe { std::mem::zeroed() };
        out_result.size = std::mem::size_of::<sys::mln_vulkan_device_context_result>() as _;
        call.status(|device, out_diagnostic| unsafe {
            sys::mln_vulkan_device_get_context(device, &mut out_result, out_diagnostic)
        })?;
        Ok(unsafe { from_native(out_result) }?)
    }

    /// Releases a Vulkan device.
    ///
    /// Render sessions that attached with the device's context keep using it
    /// until they detach or their release finishes.
    ///
    /// See `mln_vulkan_device_release` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/vulkan_8h.html).
    pub fn release(self) -> std::result::Result<(), ReleaseError<Self>> {
        self.consume(|device| {
            let mut call = Call::new(device);
            call.run(|device| unsafe { sys::mln_vulkan_device_release(device) });
            Ok(())
        })
    }
}
