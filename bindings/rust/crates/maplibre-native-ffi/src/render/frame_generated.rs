// Generated from C headers by tools/bindgen. Do not edit.
use super::*;

impl AcquiredFrameHandle {
    /// Calls `mln_acquired_frame_dispose` using its header execution and ownership contract.
    pub fn dispose(&self) -> Result<()> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let result = self.handle.close_with(|native| {
            maplibre_core::check(unsafe { sys::mln_acquired_frame_dispose(native) })?;
            Ok(())
        })?;
        Ok(result.unwrap_or_else(|| Default::default()))
    }

    /// Calls `mln_acquired_frame_get_metal_texture` using its header execution and ownership contract.
    pub fn get_metal_texture<R>(
        &self,
        callback: impl FnOnce(&maplibre_core::generated::MetalOwnedTextureFrame) -> R,
    ) -> Result<R> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.native()?;
        maplibre_core::callback::check("mln_acquired_frame_get_metal_texture", native.0)?;
        let _scope = unsafe {
            maplibre_core::handle::NativeViewScope::begin(
                native,
                sys::mln_adapter_acquired_frame_view_begin,
                sys::mln_adapter_acquired_frame_view_end,
            )
        }?;
        let mut binding_arg_1: sys::mln_metal_owned_texture_frame =
            maplibre_core::generated::MetalOwnedTextureFrame::default().to_native();
        maplibre_core::check(unsafe {
            sys::mln_acquired_frame_get_metal_texture(native, &mut binding_arg_1)
        })?;
        let value = maplibre_core::generated::MetalOwnedTextureFrame::from_native(binding_arg_1);
        Ok(callback(&value))
    }

    /// Calls `mln_acquired_frame_get_opengl_texture` using its header execution and ownership contract.
    pub fn get_opengl_texture<R>(
        &self,
        callback: impl FnOnce(&maplibre_core::generated::OpenglOwnedTextureFrame) -> R,
    ) -> Result<R> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.native()?;
        maplibre_core::callback::check("mln_acquired_frame_get_opengl_texture", native.0)?;
        let _scope = unsafe {
            maplibre_core::handle::NativeViewScope::begin(
                native,
                sys::mln_adapter_acquired_frame_view_begin,
                sys::mln_adapter_acquired_frame_view_end,
            )
        }?;
        let mut binding_arg_1: sys::mln_opengl_owned_texture_frame =
            maplibre_core::generated::OpenglOwnedTextureFrame::default().to_native();
        maplibre_core::check(unsafe {
            sys::mln_acquired_frame_get_opengl_texture(native, &mut binding_arg_1)
        })?;
        let value = maplibre_core::generated::OpenglOwnedTextureFrame::from_native(binding_arg_1);
        Ok(callback(&value))
    }

    /// Calls `mln_acquired_frame_get_producer_sync` using its header execution and ownership contract.
    pub fn get_producer_sync<R>(
        &self,
        callback: impl FnOnce(&maplibre_core::generated::GpuSync) -> R,
    ) -> Result<R> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.native()?;
        maplibre_core::callback::check("mln_acquired_frame_get_producer_sync", native.0)?;
        let _scope = unsafe {
            maplibre_core::handle::NativeViewScope::begin(
                native,
                sys::mln_adapter_acquired_frame_view_begin,
                sys::mln_adapter_acquired_frame_view_end,
            )
        }?;
        let mut binding_arg_1: sys::mln_gpu_sync =
            maplibre_core::generated::GpuSync::default().to_native();
        maplibre_core::check(unsafe {
            sys::mln_acquired_frame_get_producer_sync(native, &mut binding_arg_1)
        })?;
        let value = maplibre_core::generated::GpuSync::from_native(binding_arg_1);
        Ok(callback(&value))
    }

    /// Calls `mln_acquired_frame_get_result` using its header execution and ownership contract.
    pub fn get_result(&self) -> Result<maplibre_core::generated::RenderFrameResult> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.native()?;
        maplibre_core::callback::check("mln_acquired_frame_get_result", native.0)?;
        let mut binding_arg_1: sys::mln_render_frame_result =
            maplibre_core::generated::RenderFrameResult::default().to_native();
        maplibre_core::check(unsafe {
            sys::mln_acquired_frame_get_result(native, &mut binding_arg_1)
        })?;
        Ok(maplibre_core::generated::RenderFrameResult::from_native(
            binding_arg_1,
        ))
    }

    /// Calls `mln_acquired_frame_get_vulkan_texture` using its header execution and ownership contract.
    pub fn get_vulkan_texture<R>(
        &self,
        callback: impl FnOnce(&maplibre_core::generated::VulkanOwnedTextureFrame) -> R,
    ) -> Result<R> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.native()?;
        maplibre_core::callback::check("mln_acquired_frame_get_vulkan_texture", native.0)?;
        let _scope = unsafe {
            maplibre_core::handle::NativeViewScope::begin(
                native,
                sys::mln_adapter_acquired_frame_view_begin,
                sys::mln_adapter_acquired_frame_view_end,
            )
        }?;
        let mut binding_arg_1: sys::mln_vulkan_owned_texture_frame =
            maplibre_core::generated::VulkanOwnedTextureFrame::default().to_native();
        maplibre_core::check(unsafe {
            sys::mln_acquired_frame_get_vulkan_texture(native, &mut binding_arg_1)
        })?;
        let value = maplibre_core::generated::VulkanOwnedTextureFrame::from_native(binding_arg_1);
        Ok(callback(&value))
    }

    /// Calls `mln_acquired_frame_get_webgpu_texture` using its header execution and ownership contract.
    pub fn get_webgpu_texture<R>(
        &self,
        callback: impl FnOnce(&maplibre_core::generated::WebgpuOwnedTextureFrame) -> R,
    ) -> Result<R> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let native = self.native()?;
        maplibre_core::callback::check("mln_acquired_frame_get_webgpu_texture", native.0)?;
        let _scope = unsafe {
            maplibre_core::handle::NativeViewScope::begin(
                native,
                sys::mln_adapter_acquired_frame_view_begin,
                sys::mln_adapter_acquired_frame_view_end,
            )
        }?;
        let mut binding_arg_1: sys::mln_webgpu_owned_texture_frame =
            maplibre_core::generated::WebgpuOwnedTextureFrame::default().to_native();
        maplibre_core::check(unsafe {
            sys::mln_acquired_frame_get_webgpu_texture(native, &mut binding_arg_1)
        })?;
        let value = maplibre_core::generated::WebgpuOwnedTextureFrame::from_native(binding_arg_1);
        Ok(callback(&value))
    }

    /// Calls `mln_acquired_frame_release` using its header execution and ownership contract.
    pub fn release(&self, binding_arg_1: &maplibre_core::generated::GpuSync) -> Result<()> {
        // SAFETY: input storage lives through submission; callback values are copied before return.
        let binding_arg_1 = binding_arg_1.to_native();
        let result = self.handle.close_with(|mut native| {
            maplibre_core::check(unsafe {
                sys::mln_acquired_frame_release(&mut native, &binding_arg_1)
            })?;
            Ok(())
        })?;
        Ok(result.unwrap_or_else(|| Default::default()))
    }
}
