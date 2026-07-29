use std::{cell::Cell, ffi::c_void};

use maplibre_native_core::{self as core, handle::NativeHandleState};
use maplibre_native_sys as sys;
use napi::bindgen_prelude::{BigInt, Result, Uint8Array};
use napi_derive::napi;

use crate::{
    error,
    map::{
        NativeMapAttachReference, NativeMapHandle, json_value_to_serde, json_value_to_string,
        parse_geojson, parse_json_value,
    },
    values::ScreenPoint,
};

#[napi(object)]
pub struct RenderTargetExtent {
    pub width: u32,
    pub height: u32,
    pub scale_factor: f64,
}

#[napi(object)]
pub struct PhysicalSize {
    pub width: u32,
    pub height: u32,
}

#[napi(js_name = "renderTargetExtentPhysicalSize")]
pub fn render_target_extent_physical_size(extent: RenderTargetExtent) -> Result<PhysicalSize> {
    let extent = extent.into_native();
    let mut width = 0;
    let mut height = 0;
    core::check(unsafe {
        sys::mln_render_target_extent_physical_size(&extent, &mut width, &mut height)
    })
    .map_err(error::from_core)?;
    Ok(PhysicalSize { width, height })
}

#[napi(object)]
pub struct MetalContextDescriptor {
    pub device_address: Option<BigInt>,
}

#[napi(object)]
pub struct MetalOwnedTextureDescriptor {
    pub extent: RenderTargetExtent,
    pub context: MetalContextDescriptor,
}

#[napi(object)]
pub struct MetalBorrowedTextureDescriptor {
    pub extent: RenderTargetExtent,
    pub physical_width: u32,
    pub physical_height: u32,
    pub texture_address: BigInt,
}

#[napi(object)]
pub struct MetalSurfaceDescriptor {
    pub extent: RenderTargetExtent,
    pub context: MetalContextDescriptor,
    pub layer_address: BigInt,
}

#[napi(object)]
pub struct VulkanContextDescriptor {
    pub instance_address: BigInt,
    pub physical_device_address: BigInt,
    pub device_address: BigInt,
    pub graphics_queue_address: BigInt,
    pub graphics_queue_family_index: u32,
    pub get_instance_proc_addr_address: Option<BigInt>,
    pub get_device_proc_addr_address: Option<BigInt>,
}

#[napi(object)]
pub struct WglContextDescriptor {
    pub device_context_address: BigInt,
    pub share_context_address: BigInt,
    pub get_proc_address_address: Option<BigInt>,
}

#[napi(object)]
pub struct EglContextDescriptor {
    pub display_address: BigInt,
    pub config_address: BigInt,
    pub share_context_address: BigInt,
    pub get_proc_address_address: Option<BigInt>,
}

#[napi(object)]
pub struct OpenGLContextDescriptor {
    pub platform: String,
    pub wgl: Option<WglContextDescriptor>,
    pub egl: Option<EglContextDescriptor>,
}

#[napi(object)]
pub struct VulkanOwnedTextureDescriptor {
    pub extent: RenderTargetExtent,
    pub context: VulkanContextDescriptor,
}

#[napi(object)]
pub struct VulkanBorrowedTextureDescriptor {
    pub extent: RenderTargetExtent,
    pub physical_width: u32,
    pub physical_height: u32,
    pub context: VulkanContextDescriptor,
    pub image_address: BigInt,
    pub image_view_address: BigInt,
    pub format: u32,
    pub initial_layout: u32,
    pub final_layout: u32,
}

#[napi(object)]
pub struct VulkanSurfaceDescriptor {
    pub extent: RenderTargetExtent,
    pub context: VulkanContextDescriptor,
    pub surface_address: BigInt,
}

#[napi(object)]
pub struct OpenGLOwnedTextureDescriptor {
    pub extent: RenderTargetExtent,
    pub context: OpenGLContextDescriptor,
}

#[napi(object)]
pub struct OpenGLBorrowedTextureDescriptor {
    pub extent: RenderTargetExtent,
    pub physical_width: u32,
    pub physical_height: u32,
    pub context: OpenGLContextDescriptor,
    pub texture: u32,
    pub target: u32,
}

#[napi(object)]
pub struct OpenGLSurfaceDescriptor {
    pub extent: RenderTargetExtent,
    pub context: OpenGLContextDescriptor,
    pub surface_address: BigInt,
}

#[napi(object)]
pub struct TextureImageInfo {
    pub width: u32,
    pub height: u32,
    pub stride: u32,
    pub byte_length: i64,
}

#[napi(object)]
pub struct TextureReadback {
    pub info: TextureImageInfo,
    pub pixels: Uint8Array,
}

#[napi(object)]
pub struct NativeMetalOwnedTextureFrame {
    pub generation: BigInt,
    pub width: u32,
    pub height: u32,
    pub scale_factor: f64,
    pub frame_id: BigInt,
    pub texture_address: BigInt,
    pub device_address: BigInt,
    pub pixel_format: BigInt,
}

#[napi(object)]
pub struct NativeVulkanOwnedTextureFrame {
    pub generation: BigInt,
    pub width: u32,
    pub height: u32,
    pub scale_factor: f64,
    pub frame_id: BigInt,
    pub image_address: BigInt,
    pub image_view_address: BigInt,
    pub device_address: BigInt,
    pub format: u32,
    pub layout: u32,
}

#[napi(object)]
pub struct NativeOpenGLOwnedTextureFrame {
    pub generation: BigInt,
    pub width: u32,
    pub height: u32,
    pub scale_factor: f64,
    pub frame_id: BigInt,
    pub texture: u32,
    pub target: u32,
    pub internal_format: u32,
    pub format: u32,
    pub type_: u32,
}

#[napi(object)]
pub struct FeatureStateSelectorInput {
    pub source_id: String,
    pub source_layer_id: Option<String>,
    pub feature_id: Option<String>,
    pub state_key: Option<String>,
}

#[napi(object)]
pub struct ScreenBoxInput {
    pub min: ScreenPoint,
    pub max: ScreenPoint,
}

#[napi(object)]
pub struct RenderedQueryGeometryInput {
    pub kind: String,
    pub point: Option<ScreenPoint>,
    pub box_: Option<ScreenBoxInput>,
    pub points: Option<Vec<ScreenPoint>>,
}

#[napi(object)]
#[derive(Default)]
pub struct RenderedFeatureQueryOptionsInput {
    pub layer_ids: Option<Vec<String>>,
    pub filter: Option<String>,
}

#[napi(object)]
#[derive(Default)]
pub struct SourceFeatureQueryOptionsInput {
    pub source_layer_ids: Option<Vec<String>>,
    pub filter: Option<String>,
}

#[napi(js_name = "NativeRenderSessionHandle")]
pub struct NativeRenderSessionHandle {
    state: NativeHandleState<sys::mln_render_session>,
    frame_acquired: Cell<bool>,
}

#[napi(js_name = "createMetalOwnedTextureRenderSession")]
pub fn create_metal_owned_texture_render_session(
    map: &NativeMapHandle,
    descriptor: MetalOwnedTextureDescriptor,
) -> Result<NativeRenderSessionHandle> {
    attach_metal_owned_texture(map, descriptor)
}

#[napi(js_name = "createMetalOwnedTextureRenderSessionFromReference")]
pub fn create_metal_owned_texture_render_session_from_reference(
    map: &NativeMapAttachReference,
    descriptor: MetalOwnedTextureDescriptor,
) -> Result<NativeRenderSessionHandle> {
    attach_metal_owned_texture(map, descriptor)
}

fn attach_metal_owned_texture(
    map: &impl MapAttachTarget,
    descriptor: MetalOwnedTextureDescriptor,
) -> Result<NativeRenderSessionHandle> {
    let mut raw_descriptor = unsafe { sys::mln_metal_owned_texture_descriptor_default() };
    raw_descriptor.extent = descriptor.extent.into_native();
    raw_descriptor.context.device = descriptor.context.device_ptr()?;
    attach_render_session(map, |map, out_session| unsafe {
        sys::mln_metal_owned_texture_attach(map, &raw_descriptor, out_session)
    })
}

#[napi(js_name = "createMetalBorrowedTextureRenderSession")]
pub fn create_metal_borrowed_texture_render_session(
    map: &NativeMapHandle,
    descriptor: MetalBorrowedTextureDescriptor,
) -> Result<NativeRenderSessionHandle> {
    attach_metal_borrowed_texture(map, descriptor)
}

#[napi(js_name = "createMetalBorrowedTextureRenderSessionFromReference")]
pub fn create_metal_borrowed_texture_render_session_from_reference(
    map: &NativeMapAttachReference,
    descriptor: MetalBorrowedTextureDescriptor,
) -> Result<NativeRenderSessionHandle> {
    attach_metal_borrowed_texture(map, descriptor)
}

fn attach_metal_borrowed_texture(
    map: &impl MapAttachTarget,
    descriptor: MetalBorrowedTextureDescriptor,
) -> Result<NativeRenderSessionHandle> {
    let mut raw_descriptor = unsafe { sys::mln_metal_borrowed_texture_descriptor_default() };
    raw_descriptor.extent = descriptor.extent.into_native();
    raw_descriptor.physical_width = descriptor.physical_width;
    raw_descriptor.physical_height = descriptor.physical_height;
    raw_descriptor.texture = bigint_to_ptr(&descriptor.texture_address, "textureAddress")?;
    attach_render_session(map, |map, out_session| unsafe {
        sys::mln_metal_borrowed_texture_attach(map, &raw_descriptor, out_session)
    })
}

#[napi(js_name = "createMetalSurfaceRenderSession")]
pub fn create_metal_surface_render_session(
    map: &NativeMapHandle,
    descriptor: MetalSurfaceDescriptor,
) -> Result<NativeRenderSessionHandle> {
    attach_metal_surface(map, descriptor)
}

#[napi(js_name = "createMetalSurfaceRenderSessionFromReference")]
pub fn create_metal_surface_render_session_from_reference(
    map: &NativeMapAttachReference,
    descriptor: MetalSurfaceDescriptor,
) -> Result<NativeRenderSessionHandle> {
    attach_metal_surface(map, descriptor)
}

fn attach_metal_surface(
    map: &impl MapAttachTarget,
    descriptor: MetalSurfaceDescriptor,
) -> Result<NativeRenderSessionHandle> {
    let mut raw_descriptor = unsafe { sys::mln_metal_surface_descriptor_default() };
    raw_descriptor.extent = descriptor.extent.into_native();
    raw_descriptor.context.device = descriptor.context.device_ptr()?;
    raw_descriptor.layer = bigint_to_ptr(&descriptor.layer_address, "layerAddress")?;
    attach_render_session(map, |map, out_session| unsafe {
        sys::mln_metal_surface_attach(map, &raw_descriptor, out_session)
    })
}

#[napi(js_name = "createVulkanOwnedTextureRenderSession")]
pub fn create_vulkan_owned_texture_render_session(
    map: &NativeMapHandle,
    descriptor: VulkanOwnedTextureDescriptor,
) -> Result<NativeRenderSessionHandle> {
    attach_vulkan_owned_texture(map, descriptor)
}

#[napi(js_name = "createVulkanOwnedTextureRenderSessionFromReference")]
pub fn create_vulkan_owned_texture_render_session_from_reference(
    map: &NativeMapAttachReference,
    descriptor: VulkanOwnedTextureDescriptor,
) -> Result<NativeRenderSessionHandle> {
    attach_vulkan_owned_texture(map, descriptor)
}

fn attach_vulkan_owned_texture(
    map: &impl MapAttachTarget,
    descriptor: VulkanOwnedTextureDescriptor,
) -> Result<NativeRenderSessionHandle> {
    let mut raw_descriptor = unsafe { sys::mln_vulkan_owned_texture_descriptor_default() };
    raw_descriptor.extent = descriptor.extent.into_native();
    raw_descriptor.context = descriptor.context.into_native()?;
    attach_render_session(map, |map, out_session| unsafe {
        sys::mln_vulkan_owned_texture_attach(map, &raw_descriptor, out_session)
    })
}

#[napi(js_name = "createVulkanBorrowedTextureRenderSession")]
pub fn create_vulkan_borrowed_texture_render_session(
    map: &NativeMapHandle,
    descriptor: VulkanBorrowedTextureDescriptor,
) -> Result<NativeRenderSessionHandle> {
    attach_vulkan_borrowed_texture(map, descriptor)
}

#[napi(js_name = "createVulkanBorrowedTextureRenderSessionFromReference")]
pub fn create_vulkan_borrowed_texture_render_session_from_reference(
    map: &NativeMapAttachReference,
    descriptor: VulkanBorrowedTextureDescriptor,
) -> Result<NativeRenderSessionHandle> {
    attach_vulkan_borrowed_texture(map, descriptor)
}

fn attach_vulkan_borrowed_texture(
    map: &impl MapAttachTarget,
    descriptor: VulkanBorrowedTextureDescriptor,
) -> Result<NativeRenderSessionHandle> {
    let mut raw_descriptor = unsafe { sys::mln_vulkan_borrowed_texture_descriptor_default() };
    raw_descriptor.extent = descriptor.extent.into_native();
    raw_descriptor.physical_width = descriptor.physical_width;
    raw_descriptor.physical_height = descriptor.physical_height;
    raw_descriptor.context = descriptor.context.into_native()?;
    raw_descriptor.image = bigint_to_ptr(&descriptor.image_address, "imageAddress")?;
    raw_descriptor.image_view = bigint_to_ptr(&descriptor.image_view_address, "imageViewAddress")?;
    raw_descriptor.format = descriptor.format;
    raw_descriptor.initial_layout = descriptor.initial_layout;
    raw_descriptor.final_layout = descriptor.final_layout;
    attach_render_session(map, |map, out_session| unsafe {
        sys::mln_vulkan_borrowed_texture_attach(map, &raw_descriptor, out_session)
    })
}

#[napi(js_name = "createVulkanSurfaceRenderSession")]
pub fn create_vulkan_surface_render_session(
    map: &NativeMapHandle,
    descriptor: VulkanSurfaceDescriptor,
) -> Result<NativeRenderSessionHandle> {
    attach_vulkan_surface(map, descriptor)
}

#[napi(js_name = "createVulkanSurfaceRenderSessionFromReference")]
pub fn create_vulkan_surface_render_session_from_reference(
    map: &NativeMapAttachReference,
    descriptor: VulkanSurfaceDescriptor,
) -> Result<NativeRenderSessionHandle> {
    attach_vulkan_surface(map, descriptor)
}

fn attach_vulkan_surface(
    map: &impl MapAttachTarget,
    descriptor: VulkanSurfaceDescriptor,
) -> Result<NativeRenderSessionHandle> {
    let mut raw_descriptor = unsafe { sys::mln_vulkan_surface_descriptor_default() };
    raw_descriptor.extent = descriptor.extent.into_native();
    raw_descriptor.context = descriptor.context.into_native()?;
    raw_descriptor.surface = bigint_to_ptr(&descriptor.surface_address, "surfaceAddress")?;
    attach_render_session(map, |map, out_session| unsafe {
        sys::mln_vulkan_surface_attach(map, &raw_descriptor, out_session)
    })
}

#[napi(js_name = "createOpenGLOwnedTextureRenderSession")]
pub fn create_opengl_owned_texture_render_session(
    map: &NativeMapHandle,
    descriptor: OpenGLOwnedTextureDescriptor,
) -> Result<NativeRenderSessionHandle> {
    attach_opengl_owned_texture(map, descriptor)
}

#[napi(js_name = "createOpenGLOwnedTextureRenderSessionFromReference")]
pub fn create_opengl_owned_texture_render_session_from_reference(
    map: &NativeMapAttachReference,
    descriptor: OpenGLOwnedTextureDescriptor,
) -> Result<NativeRenderSessionHandle> {
    attach_opengl_owned_texture(map, descriptor)
}

fn attach_opengl_owned_texture(
    map: &impl MapAttachTarget,
    descriptor: OpenGLOwnedTextureDescriptor,
) -> Result<NativeRenderSessionHandle> {
    let mut raw_descriptor = unsafe { sys::mln_opengl_owned_texture_descriptor_default() };
    raw_descriptor.extent = descriptor.extent.into_native();
    raw_descriptor.context = descriptor.context.into_native()?;
    attach_render_session(map, |map, out_session| unsafe {
        sys::mln_opengl_owned_texture_attach(map, &raw_descriptor, out_session)
    })
}

#[napi(js_name = "createOpenGLBorrowedTextureRenderSession")]
pub fn create_opengl_borrowed_texture_render_session(
    map: &NativeMapHandle,
    descriptor: OpenGLBorrowedTextureDescriptor,
) -> Result<NativeRenderSessionHandle> {
    attach_opengl_borrowed_texture(map, descriptor)
}

#[napi(js_name = "createOpenGLBorrowedTextureRenderSessionFromReference")]
pub fn create_opengl_borrowed_texture_render_session_from_reference(
    map: &NativeMapAttachReference,
    descriptor: OpenGLBorrowedTextureDescriptor,
) -> Result<NativeRenderSessionHandle> {
    attach_opengl_borrowed_texture(map, descriptor)
}

fn attach_opengl_borrowed_texture(
    map: &impl MapAttachTarget,
    descriptor: OpenGLBorrowedTextureDescriptor,
) -> Result<NativeRenderSessionHandle> {
    let mut raw_descriptor = unsafe { sys::mln_opengl_borrowed_texture_descriptor_default() };
    raw_descriptor.extent = descriptor.extent.into_native();
    raw_descriptor.physical_width = descriptor.physical_width;
    raw_descriptor.physical_height = descriptor.physical_height;
    raw_descriptor.context = descriptor.context.into_native()?;
    raw_descriptor.texture = descriptor.texture;
    raw_descriptor.target = descriptor.target;
    attach_render_session(map, |map, out_session| unsafe {
        sys::mln_opengl_borrowed_texture_attach(map, &raw_descriptor, out_session)
    })
}

#[napi(js_name = "createOpenGLSurfaceRenderSession")]
pub fn create_opengl_surface_render_session(
    map: &NativeMapHandle,
    descriptor: OpenGLSurfaceDescriptor,
) -> Result<NativeRenderSessionHandle> {
    attach_opengl_surface(map, descriptor)
}

#[napi(js_name = "createOpenGLSurfaceRenderSessionFromReference")]
pub fn create_opengl_surface_render_session_from_reference(
    map: &NativeMapAttachReference,
    descriptor: OpenGLSurfaceDescriptor,
) -> Result<NativeRenderSessionHandle> {
    attach_opengl_surface(map, descriptor)
}

fn attach_opengl_surface(
    map: &impl MapAttachTarget,
    descriptor: OpenGLSurfaceDescriptor,
) -> Result<NativeRenderSessionHandle> {
    let mut raw_descriptor = unsafe { sys::mln_opengl_surface_descriptor_default() };
    raw_descriptor.extent = descriptor.extent.into_native();
    raw_descriptor.context = descriptor.context.into_native()?;
    raw_descriptor.surface = bigint_to_ptr(&descriptor.surface_address, "surfaceAddress")?;
    attach_render_session(map, |map, out_session| unsafe {
        sys::mln_opengl_surface_attach(map, &raw_descriptor, out_session)
    })
}

#[napi]
impl NativeRenderSessionHandle {
    #[napi]
    pub fn close(&self) -> Result<()> {
        self.ensure_no_frame_acquired()?;
        unsafe { self.state.close_status(sys::mln_render_session_destroy) }
            .map_err(error::from_core)
    }

    #[napi(getter)]
    pub fn closed(&self) -> bool {
        self.state.is_closed()
    }

    #[napi]
    pub fn resize(&self, width: u32, height: u32, scale_factor: f64) -> Result<()> {
        self.ensure_no_frame_acquired()?;
        core::check(unsafe {
            sys::mln_render_session_resize(self.state.as_ptr(), width, height, scale_factor)
        })
        .map_err(error::from_core)
    }

    #[napi(js_name = "renderUpdate")]
    pub fn render_update(&self) -> Result<bool> {
        self.ensure_no_frame_acquired()?;
        let mut rendered = false;
        core::check(unsafe {
            sys::mln_render_session_render_update(self.state.as_ptr(), &mut rendered)
        })
        .map_err(error::from_core)?;
        Ok(rendered)
    }

    #[napi]
    pub fn detach(&self) -> Result<()> {
        self.ensure_no_frame_acquired()?;
        core::check(unsafe { sys::mln_render_session_detach(self.state.as_ptr()) })
            .map_err(error::from_core)
    }

    #[napi(js_name = "reduceMemoryUse")]
    pub fn reduce_memory_use(&self) -> Result<()> {
        self.ensure_no_frame_acquired()?;
        core::check(unsafe { sys::mln_render_session_reduce_memory_use(self.state.as_ptr()) })
            .map_err(error::from_core)
    }

    #[napi(js_name = "clearData")]
    pub fn clear_data(&self) -> Result<()> {
        self.ensure_no_frame_acquired()?;
        core::check(unsafe { sys::mln_render_session_clear_data(self.state.as_ptr()) })
            .map_err(error::from_core)
    }

    #[napi(js_name = "dumpDebugLogs")]
    pub fn dump_debug_logs(&self) -> Result<()> {
        self.ensure_no_frame_acquired()?;
        core::check(unsafe { sys::mln_render_session_dump_debug_logs(self.state.as_ptr()) })
            .map_err(error::from_core)
    }

    #[napi(js_name = "setFeatureState")]
    pub fn set_feature_state(
        &self,
        selector: FeatureStateSelectorInput,
        state: String,
    ) -> Result<()> {
        self.ensure_no_frame_acquired()?;
        let selector = selector.into_core()?;
        let native_selector = core::query::feature_state_selector_to_native(&selector);
        let state = parse_json_value(state)?;
        let native_state =
            core::json::json_value_try_to_native(&state).map_err(error::from_core)?;
        core::check(unsafe {
            sys::mln_render_session_set_feature_state(
                self.state.as_ptr(),
                native_selector.as_ptr(),
                native_state.as_ref(),
            )
        })
        .map_err(error::from_core)
    }

    #[napi(js_name = "getFeatureState")]
    pub fn get_feature_state(&self, selector: FeatureStateSelectorInput) -> Result<String> {
        self.ensure_no_frame_acquired()?;
        let selector = selector.into_core()?;
        let native_selector = core::query::feature_state_selector_to_native(&selector);
        let mut snapshot = std::ptr::null_mut();
        core::check(unsafe {
            sys::mln_render_session_get_feature_state(
                self.state.as_ptr(),
                native_selector.as_ptr(),
                &mut snapshot,
            )
        })
        .map_err(error::from_core)?;
        let snapshot = std::ptr::NonNull::new(snapshot)
            .ok_or_else(|| error::invalid_argument("feature state snapshot result was null"))?;
        let value = unsafe { core::json::copy_json_snapshot(Some(snapshot)) }
            .map_err(error::from_core)?
            .unwrap_or(core::JsonValue::Null);
        json_value_to_string(value)
    }

    #[napi(js_name = "removeFeatureState")]
    pub fn remove_feature_state(&self, selector: FeatureStateSelectorInput) -> Result<()> {
        self.ensure_no_frame_acquired()?;
        let selector = selector.into_core()?;
        let native_selector = core::query::feature_state_selector_to_native(&selector);
        core::check(unsafe {
            sys::mln_render_session_remove_feature_state(
                self.state.as_ptr(),
                native_selector.as_ptr(),
            )
        })
        .map_err(error::from_core)
    }

    #[napi(js_name = "queryRenderedFeatures")]
    pub fn query_rendered_features(
        &self,
        geometry: RenderedQueryGeometryInput,
        options: Option<RenderedFeatureQueryOptionsInput>,
    ) -> Result<String> {
        self.ensure_no_frame_acquired()?;
        let geometry = geometry.into_core()?;
        let native_geometry = core::query::rendered_query_geometry_to_native(&geometry);
        let options = options.unwrap_or_default().into_core()?;
        let native_options = core::query::rendered_feature_query_options_to_native(&options)
            .map_err(error::from_core)?;
        let mut result = std::ptr::null_mut();
        core::check(unsafe {
            sys::mln_render_session_query_rendered_features(
                self.state.as_ptr(),
                native_geometry.as_ptr(),
                native_options.as_ptr(),
                &mut result,
            )
        })
        .map_err(error::from_core)?;
        queried_features_to_string(result)
    }

    #[napi(js_name = "querySourceFeatures")]
    pub fn query_source_features(
        &self,
        source_id: String,
        options: Option<SourceFeatureQueryOptionsInput>,
    ) -> Result<String> {
        self.ensure_no_frame_acquired()?;
        let source_id = core::string::string_view(&source_id);
        let options = options.unwrap_or_default().into_core()?;
        let native_options = core::query::source_feature_query_options_to_native(&options)
            .map_err(error::from_core)?;
        let mut result = std::ptr::null_mut();
        core::check(unsafe {
            sys::mln_render_session_query_source_features(
                self.state.as_ptr(),
                source_id.raw(),
                native_options.as_ptr(),
                &mut result,
            )
        })
        .map_err(error::from_core)?;
        queried_features_to_string(result)
    }

    #[napi(js_name = "queryFeatureExtension")]
    pub fn query_feature_extension(
        &self,
        source_id: String,
        feature: String,
        extension: String,
        extension_field: String,
        arguments: Option<String>,
    ) -> Result<String> {
        self.ensure_no_frame_acquired()?;
        let source_id = core::string::string_view(&source_id);
        let extension = core::string::string_view(&extension);
        let extension_field = core::string::string_view(&extension_field);
        let feature = feature_from_geojson_string(feature)?;
        let native_feature =
            core::geojson::feature_try_to_native(&feature, 0).map_err(error::from_core)?;
        let arguments = arguments.map(parse_json_value).transpose()?;
        let native_arguments = arguments
            .as_ref()
            .map(core::json::json_value_try_to_native)
            .transpose()
            .map_err(error::from_core)?;
        let native_arguments_ptr = native_arguments
            .as_ref()
            .map_or(std::ptr::null(), |arguments| arguments.as_ref());
        let mut result = std::ptr::null_mut();
        core::check(unsafe {
            sys::mln_render_session_query_feature_extensions(
                self.state.as_ptr(),
                source_id.raw(),
                native_feature.as_ref(),
                extension.raw(),
                extension_field.raw(),
                native_arguments_ptr,
                &mut result,
            )
        })
        .map_err(error::from_core)?;
        feature_extension_result_to_string(result)
    }

    #[napi(js_name = "acquireMetalOwnedTextureFrame")]
    pub fn acquire_metal_owned_texture_frame(&self) -> Result<NativeMetalOwnedTextureFrame> {
        self.ensure_no_frame_acquired()?;
        let mut frame = sys::mln_metal_owned_texture_frame {
            size: std::mem::size_of::<sys::mln_metal_owned_texture_frame>() as u32,
            generation: 0,
            width: 0,
            height: 0,
            scale_factor: 0.0,
            frame_id: 0,
            texture: std::ptr::null_mut(),
            device: std::ptr::null_mut(),
            pixel_format: 0,
        };
        core::check(unsafe {
            sys::mln_metal_owned_texture_acquire_frame(self.state.as_ptr(), &mut frame)
        })
        .map_err(error::from_core)?;
        self.frame_acquired.set(true);
        Ok(NativeMetalOwnedTextureFrame::from_native(frame))
    }

    #[napi(js_name = "releaseMetalOwnedTextureFrame")]
    pub fn release_metal_owned_texture_frame(
        &self,
        frame: NativeMetalOwnedTextureFrame,
    ) -> Result<()> {
        self.ensure_frame_acquired()?;
        let frame = frame.into_native()?;
        core::check(unsafe {
            sys::mln_metal_owned_texture_release_frame(self.state.as_ptr(), &frame)
        })
        .map_err(error::from_core)?;
        self.frame_acquired.set(false);
        Ok(())
    }

    #[napi(js_name = "acquireVulkanOwnedTextureFrame")]
    pub fn acquire_vulkan_owned_texture_frame(&self) -> Result<NativeVulkanOwnedTextureFrame> {
        self.ensure_no_frame_acquired()?;
        let mut frame = sys::mln_vulkan_owned_texture_frame {
            size: std::mem::size_of::<sys::mln_vulkan_owned_texture_frame>() as u32,
            generation: 0,
            width: 0,
            height: 0,
            scale_factor: 0.0,
            frame_id: 0,
            image: std::ptr::null_mut(),
            image_view: std::ptr::null_mut(),
            device: std::ptr::null_mut(),
            format: 0,
            layout: 0,
        };
        core::check(unsafe {
            sys::mln_vulkan_owned_texture_acquire_frame(self.state.as_ptr(), &mut frame)
        })
        .map_err(error::from_core)?;
        self.frame_acquired.set(true);
        Ok(NativeVulkanOwnedTextureFrame::from_native(frame))
    }

    #[napi(js_name = "releaseVulkanOwnedTextureFrame")]
    pub fn release_vulkan_owned_texture_frame(
        &self,
        frame: NativeVulkanOwnedTextureFrame,
    ) -> Result<()> {
        self.ensure_frame_acquired()?;
        let frame = frame.into_native()?;
        core::check(unsafe {
            sys::mln_vulkan_owned_texture_release_frame(self.state.as_ptr(), &frame)
        })
        .map_err(error::from_core)?;
        self.frame_acquired.set(false);
        Ok(())
    }

    #[napi(js_name = "acquireOpenGLOwnedTextureFrame")]
    pub fn acquire_opengl_owned_texture_frame(&self) -> Result<NativeOpenGLOwnedTextureFrame> {
        self.ensure_no_frame_acquired()?;
        let mut frame = sys::mln_opengl_owned_texture_frame {
            size: std::mem::size_of::<sys::mln_opengl_owned_texture_frame>() as u32,
            generation: 0,
            width: 0,
            height: 0,
            scale_factor: 1.0,
            frame_id: 0,
            texture: 0,
            target: 0,
            internal_format: 0,
            format: 0,
            type_: 0,
        };
        core::check(unsafe {
            sys::mln_opengl_owned_texture_acquire_frame(self.state.as_ptr(), &mut frame)
        })
        .map_err(error::from_core)?;
        self.frame_acquired.set(true);
        Ok(NativeOpenGLOwnedTextureFrame::from_native(frame))
    }

    #[napi(js_name = "releaseOpenGLOwnedTextureFrame")]
    pub fn release_opengl_owned_texture_frame(
        &self,
        frame: NativeOpenGLOwnedTextureFrame,
    ) -> Result<()> {
        self.ensure_frame_acquired()?;
        let raw = frame.into_native()?;
        core::check(unsafe {
            sys::mln_opengl_owned_texture_release_frame(self.state.as_ptr(), &raw)
        })
        .map_err(error::from_core)?;
        self.frame_acquired.set(false);
        Ok(())
    }

    #[napi(js_name = "readPremultipliedRgba8")]
    pub fn read_premultiplied_rgba8(&self) -> Result<TextureReadback> {
        self.ensure_no_frame_acquired()?;
        let mut info = unsafe { sys::mln_texture_image_info_default() };
        let first_status = unsafe {
            sys::mln_texture_read_premultiplied_rgba8(
                self.state.as_ptr(),
                std::ptr::null_mut(),
                0,
                &mut info,
            )
        };
        if first_status != sys::MLN_STATUS_INVALID_ARGUMENT {
            core::check(first_status).map_err(error::from_core)?;
        }
        let mut pixels = vec![0; info.byte_length];
        core::check(unsafe {
            sys::mln_texture_read_premultiplied_rgba8(
                self.state.as_ptr(),
                pixels.as_mut_ptr(),
                pixels.len(),
                &mut info,
            )
        })
        .map_err(error::from_core)?;
        Ok(TextureReadback {
            info: TextureImageInfo::from_native(info),
            pixels: Uint8Array::from(pixels),
        })
    }

    #[napi(js_name = "readPremultipliedRgba8Into")]
    pub fn read_premultiplied_rgba8_into(
        &self,
        mut pixels: Uint8Array,
    ) -> Result<TextureImageInfo> {
        self.ensure_no_frame_acquired()?;
        let mut info = unsafe { sys::mln_texture_image_info_default() };
        let pixels = unsafe { pixels.as_mut() };
        core::check(unsafe {
            sys::mln_texture_read_premultiplied_rgba8(
                self.state.as_ptr(),
                pixels.as_mut_ptr(),
                pixels.len(),
                &mut info,
            )
        })
        .map_err(error::from_core)?;
        Ok(TextureImageInfo::from_native(info))
    }
}

impl NativeRenderSessionHandle {
    fn ensure_frame_acquired(&self) -> Result<()> {
        if !self.frame_acquired.get() {
            return Err(error::invalid_state(
                "texture frame scope is not active for this render session",
            ));
        }
        Ok(())
    }

    fn ensure_no_frame_acquired(&self) -> Result<()> {
        if self.frame_acquired.get() {
            return Err(error::invalid_state(
                "texture frame scope is active for this render session",
            ));
        }
        Ok(())
    }
}

impl RenderedQueryGeometryInput {
    fn into_core(self) -> Result<core::query::RenderedQueryGeometry> {
        match self.kind.as_str() {
            "point" => Ok(core::query::RenderedQueryGeometry::point(
                self.point
                    .ok_or_else(|| error::invalid_argument("point query geometry requires point"))?
                    .into_core(),
            )),
            "box" => {
                let box_ = self
                    .box_
                    .ok_or_else(|| error::invalid_argument("box query geometry requires box"))?;
                Ok(core::query::RenderedQueryGeometry::box_(
                    core::values::ScreenBox {
                        min: box_.min.into_core(),
                        max: box_.max.into_core(),
                    },
                ))
            }
            "lineString" => Ok(core::query::RenderedQueryGeometry::line_string(
                self.points
                    .ok_or_else(|| {
                        error::invalid_argument("lineString query geometry requires points")
                    })?
                    .into_iter()
                    .map(ScreenPoint::into_core)
                    .collect(),
            )),
            other => Err(error::invalid_argument(format!(
                "query geometry kind must be 'point', 'box', or 'lineString', got '{other}'"
            ))),
        }
    }
}

impl RenderedFeatureQueryOptionsInput {
    fn into_core(self) -> Result<core::query::RenderedFeatureQueryOptions> {
        let mut options = core::query::RenderedFeatureQueryOptions::default();
        if let Some(layer_ids) = self.layer_ids {
            options.layer_ids = Some(layer_ids);
        }
        if let Some(filter) = self.filter {
            options.filter = Some(parse_json_value(filter)?);
        }
        Ok(options)
    }
}

impl SourceFeatureQueryOptionsInput {
    fn into_core(self) -> Result<core::query::SourceFeatureQueryOptions> {
        let mut options = core::query::SourceFeatureQueryOptions::default();
        if let Some(source_layer_ids) = self.source_layer_ids {
            options.source_layer_ids = Some(source_layer_ids);
        }
        if let Some(filter) = self.filter {
            options.filter = Some(parse_json_value(filter)?);
        }
        Ok(options)
    }
}

impl FeatureStateSelectorInput {
    fn into_core(self) -> Result<core::query::FeatureStateSelector> {
        let mut selector = core::query::FeatureStateSelector::new(self.source_id);
        if let Some(source_layer_id) = self.source_layer_id {
            selector = selector.with_source_layer_id(source_layer_id);
        }
        if let Some(feature_id) = self.feature_id {
            selector = selector.with_feature_id(feature_id);
        }
        if let Some(state_key) = self.state_key {
            selector = selector
                .with_state_key(state_key)
                .map_err(error::from_core)?;
        }
        Ok(selector)
    }
}

impl RenderTargetExtent {
    fn into_native(self) -> sys::mln_render_target_extent {
        sys::mln_render_target_extent {
            size: std::mem::size_of::<sys::mln_render_target_extent>() as u32,
            width: self.width,
            height: self.height,
            scale_factor: self.scale_factor,
        }
    }
}

impl VulkanContextDescriptor {
    fn into_native(self) -> Result<sys::mln_vulkan_context_descriptor> {
        Ok(sys::mln_vulkan_context_descriptor {
            size: std::mem::size_of::<sys::mln_vulkan_context_descriptor>() as u32,
            instance: bigint_to_ptr(&self.instance_address, "instanceAddress")?,
            physical_device: bigint_to_ptr(&self.physical_device_address, "physicalDeviceAddress")?,
            device: bigint_to_ptr(&self.device_address, "deviceAddress")?,
            graphics_queue: bigint_to_ptr(&self.graphics_queue_address, "graphicsQueueAddress")?,
            graphics_queue_family_index: self.graphics_queue_family_index,
            get_instance_proc_addr: optional_bigint_to_ptr(
                self.get_instance_proc_addr_address.as_ref(),
                "getInstanceProcAddr",
            )?,
            get_device_proc_addr: optional_bigint_to_ptr(
                self.get_device_proc_addr_address.as_ref(),
                "getDeviceProcAddr",
            )?,
        })
    }
}

impl OpenGLContextDescriptor {
    fn into_native(self) -> Result<sys::mln_opengl_context_descriptor> {
        let mut raw = sys::mln_opengl_context_descriptor {
            size: std::mem::size_of::<sys::mln_opengl_context_descriptor>() as u32,
            platform: sys::MLN_OPENGL_CONTEXT_PLATFORM_UNSPECIFIED,
            data: sys::mln_opengl_context_descriptor__bindgen_ty_1 {
                wgl: sys::mln_wgl_context_descriptor {
                    size: std::mem::size_of::<sys::mln_wgl_context_descriptor>() as u32,
                    device_context: std::ptr::null_mut(),
                    share_context: std::ptr::null_mut(),
                    get_proc_address: std::ptr::null_mut(),
                },
            },
        };
        match self.platform.as_str() {
            "wgl" => {
                let wgl = self.wgl.ok_or_else(|| {
                    error::invalid_argument("OpenGL WGL context requires wgl fields")
                })?;
                raw.platform = sys::MLN_OPENGL_CONTEXT_PLATFORM_WGL;
                raw.data = sys::mln_opengl_context_descriptor__bindgen_ty_1 {
                    wgl: sys::mln_wgl_context_descriptor {
                        size: std::mem::size_of::<sys::mln_wgl_context_descriptor>() as u32,
                        device_context: bigint_to_ptr(
                            &wgl.device_context_address,
                            "deviceContext",
                        )?,
                        share_context: bigint_to_ptr(&wgl.share_context_address, "shareContext")?,
                        get_proc_address: optional_bigint_to_ptr(
                            wgl.get_proc_address_address.as_ref(),
                            "getProcAddress",
                        )?,
                    },
                };
            }
            "egl" => {
                let egl = self.egl.ok_or_else(|| {
                    error::invalid_argument("OpenGL EGL context requires egl fields")
                })?;
                raw.platform = sys::MLN_OPENGL_CONTEXT_PLATFORM_EGL;
                raw.data = sys::mln_opengl_context_descriptor__bindgen_ty_1 {
                    egl: sys::mln_egl_context_descriptor {
                        size: std::mem::size_of::<sys::mln_egl_context_descriptor>() as u32,
                        display: bigint_to_ptr(&egl.display_address, "display")?,
                        config: bigint_to_ptr(&egl.config_address, "config")?,
                        share_context: bigint_to_ptr(&egl.share_context_address, "shareContext")?,
                        get_proc_address: optional_bigint_to_ptr(
                            egl.get_proc_address_address.as_ref(),
                            "getProcAddress",
                        )?,
                    },
                };
            }
            other => {
                return Err(error::invalid_argument(format!(
                    "OpenGL context platform must be 'wgl' or 'egl', got '{other}'"
                )));
            }
        }
        Ok(raw)
    }
}

impl MetalContextDescriptor {
    fn device_ptr(&self) -> Result<*mut c_void> {
        self.device_address
            .as_ref()
            .map(|value| bigint_to_ptr(value, "deviceAddress"))
            .transpose()
            .map(|value| value.unwrap_or(std::ptr::null_mut()))
    }
}

impl TextureImageInfo {
    fn from_native(raw: sys::mln_texture_image_info) -> Self {
        Self {
            width: raw.width,
            height: raw.height,
            stride: raw.stride,
            byte_length: raw.byte_length as i64,
        }
    }
}

impl NativeMetalOwnedTextureFrame {
    fn from_native(raw: sys::mln_metal_owned_texture_frame) -> Self {
        Self {
            generation: BigInt::from(raw.generation),
            width: raw.width,
            height: raw.height,
            scale_factor: raw.scale_factor,
            frame_id: BigInt::from(raw.frame_id),
            texture_address: ptr_to_bigint(raw.texture),
            device_address: ptr_to_bigint(raw.device),
            pixel_format: BigInt::from(raw.pixel_format),
        }
    }

    fn into_native(self) -> Result<sys::mln_metal_owned_texture_frame> {
        Ok(sys::mln_metal_owned_texture_frame {
            size: std::mem::size_of::<sys::mln_metal_owned_texture_frame>() as u32,
            generation: bigint_to_u64(&self.generation, "generation")?,
            width: self.width,
            height: self.height,
            scale_factor: self.scale_factor,
            frame_id: bigint_to_u64(&self.frame_id, "frameId")?,
            texture: bigint_to_ptr(&self.texture_address, "textureAddress")?,
            device: bigint_to_ptr(&self.device_address, "deviceAddress")?,
            pixel_format: bigint_to_u64(&self.pixel_format, "pixelFormat")?,
        })
    }
}

impl NativeVulkanOwnedTextureFrame {
    fn from_native(raw: sys::mln_vulkan_owned_texture_frame) -> Self {
        Self {
            generation: BigInt::from(raw.generation),
            width: raw.width,
            height: raw.height,
            scale_factor: raw.scale_factor,
            frame_id: BigInt::from(raw.frame_id),
            image_address: ptr_to_bigint(raw.image),
            image_view_address: ptr_to_bigint(raw.image_view),
            device_address: ptr_to_bigint(raw.device),
            format: raw.format,
            layout: raw.layout,
        }
    }

    fn into_native(self) -> Result<sys::mln_vulkan_owned_texture_frame> {
        Ok(sys::mln_vulkan_owned_texture_frame {
            size: std::mem::size_of::<sys::mln_vulkan_owned_texture_frame>() as u32,
            generation: bigint_to_u64(&self.generation, "generation")?,
            width: self.width,
            height: self.height,
            scale_factor: self.scale_factor,
            frame_id: bigint_to_u64(&self.frame_id, "frameId")?,
            image: bigint_to_ptr(&self.image_address, "imageAddress")?,
            image_view: bigint_to_ptr(&self.image_view_address, "imageViewAddress")?,
            device: bigint_to_ptr(&self.device_address, "deviceAddress")?,
            format: self.format,
            layout: self.layout,
        })
    }
}

impl NativeOpenGLOwnedTextureFrame {
    fn from_native(raw: sys::mln_opengl_owned_texture_frame) -> Self {
        Self {
            generation: BigInt::from(raw.generation),
            width: raw.width,
            height: raw.height,
            scale_factor: raw.scale_factor,
            frame_id: BigInt::from(raw.frame_id),
            texture: raw.texture,
            target: raw.target,
            internal_format: raw.internal_format,
            format: raw.format,
            type_: raw.type_,
        }
    }

    fn into_native(self) -> Result<sys::mln_opengl_owned_texture_frame> {
        Ok(sys::mln_opengl_owned_texture_frame {
            size: std::mem::size_of::<sys::mln_opengl_owned_texture_frame>() as u32,
            generation: bigint_to_u64(&self.generation, "generation")?,
            width: self.width,
            height: self.height,
            scale_factor: self.scale_factor,
            frame_id: bigint_to_u64(&self.frame_id, "frameId")?,
            texture: self.texture,
            target: self.target,
            internal_format: self.internal_format,
            format: self.format,
            type_: self.type_,
        })
    }
}

fn feature_from_geojson_string(value: String) -> Result<core::Feature> {
    match parse_geojson(value)? {
        core::GeoJson::Feature(feature) => Ok(feature),
        _ => Err(error::invalid_argument(
            "feature extension query requires a GeoJSON Feature",
        )),
    }
}

fn feature_extension_result_to_string(
    result: *mut sys::mln_feature_extension_result,
) -> Result<String> {
    let result = std::ptr::NonNull::new(result)
        .ok_or_else(|| error::invalid_argument("feature extension result was null"))?;
    let result =
        unsafe { core::query::copy_feature_extension_result(result) }.map_err(error::from_core)?;
    let value = match result {
        core::query::FeatureExtensionResult::Value(value) => serde_json::json!({
            "kind": "value",
            "value": json_value_to_serde(value),
        }),
        core::query::FeatureExtensionResult::FeatureCollection(features) => {
            serde_json::json!({
                "kind": "featureCollection",
                "features": features.into_iter().map(feature_to_serde).collect::<Vec<_>>(),
            })
        }
        core::query::FeatureExtensionResult::Unknown(raw) => serde_json::json!({
            "kind": "unknown",
            "rawType": raw,
        }),
        _ => serde_json::json!({
            "kind": "unknown",
            "rawType": u32::MAX,
        }),
    };
    serde_json::to_string(&value).map_err(|serialize_error| {
        error::invalid_argument(format!(
            "feature extension result could not be serialized: {serialize_error}"
        ))
    })
}

fn queried_features_to_string(result: *mut sys::mln_feature_query_result) -> Result<String> {
    let result = std::ptr::NonNull::new(result)
        .ok_or_else(|| error::invalid_argument("feature query result was null"))?;
    let features =
        unsafe { core::query::copy_feature_query_result(result) }.map_err(error::from_core)?;
    let value =
        serde_json::Value::Array(features.into_iter().map(queried_feature_to_serde).collect());
    serde_json::to_string(&value).map_err(|serialize_error| {
        error::invalid_argument(format!(
            "query result could not be serialized: {serialize_error}"
        ))
    })
}

fn queried_feature_to_serde(feature: core::query::QueriedFeature) -> serde_json::Value {
    serde_json::json!({
        "feature": feature_to_serde(feature.feature),
        "sourceId": feature.source_id,
        "sourceLayerId": feature.source_layer_id,
        "state": feature.state.map(json_value_to_serde),
    })
}

fn feature_to_serde(feature: core::Feature) -> serde_json::Value {
    let properties = feature
        .properties
        .into_iter()
        .map(|member| (member.key, json_value_to_serde(member.value)))
        .collect::<serde_json::Map<_, _>>();
    let mut value = serde_json::json!({
        "type": "Feature",
        "geometry": geometry_to_serde(feature.geometry),
        "properties": properties,
    });
    if !matches!(feature.identifier, core::FeatureIdentifier::Null) {
        value["id"] = feature_identifier_to_serde(feature.identifier);
    }
    value
}

fn feature_identifier_to_serde(identifier: core::FeatureIdentifier) -> serde_json::Value {
    match identifier {
        core::FeatureIdentifier::Null => serde_json::Value::Null,
        core::FeatureIdentifier::UInt(value) => serde_json::Value::Number(value.into()),
        core::FeatureIdentifier::Int(value) => serde_json::Value::Number(value.into()),
        core::FeatureIdentifier::Double(value) => serde_json::Number::from_f64(value)
            .map(serde_json::Value::Number)
            .unwrap_or(serde_json::Value::Null),
        core::FeatureIdentifier::String(value) => serde_json::Value::String(value),
        _ => serde_json::Value::Null,
    }
}

fn geometry_to_serde(geometry: core::Geometry) -> serde_json::Value {
    match geometry {
        core::Geometry::Empty => serde_json::Value::Null,
        core::Geometry::Point(coordinate) => serde_json::json!({
            "type": "Point",
            "coordinates": [coordinate.longitude, coordinate.latitude]
        }),
        core::Geometry::LineString(coordinates) => serde_json::json!({
            "type": "LineString",
            "coordinates": coordinates_to_serde(coordinates)
        }),
        core::Geometry::Polygon(rings) => serde_json::json!({
            "type": "Polygon",
            "coordinates": rings.into_iter().map(coordinates_to_serde).collect::<Vec<_>>()
        }),
        core::Geometry::MultiPoint(coordinates) => serde_json::json!({
            "type": "MultiPoint",
            "coordinates": coordinates_to_serde(coordinates)
        }),
        core::Geometry::MultiLineString(lines) => serde_json::json!({
            "type": "MultiLineString",
            "coordinates": lines.into_iter().map(coordinates_to_serde).collect::<Vec<_>>()
        }),
        core::Geometry::MultiPolygon(polygons) => serde_json::json!({
            "type": "MultiPolygon",
            "coordinates": polygons
                .into_iter()
                .map(|rings| rings.into_iter().map(coordinates_to_serde).collect::<Vec<_>>())
                .collect::<Vec<_>>()
        }),
        core::Geometry::GeometryCollection(geometries) => serde_json::json!({
            "type": "GeometryCollection",
            "geometries": geometries.into_iter().map(geometry_to_serde).collect::<Vec<_>>()
        }),
        _ => serde_json::Value::Null,
    }
}

fn coordinates_to_serde(coordinates: Vec<core::LatLng>) -> serde_json::Value {
    serde_json::Value::Array(
        coordinates
            .into_iter()
            .map(|coordinate| serde_json::json!([coordinate.longitude, coordinate.latitude]))
            .collect(),
    )
}

trait MapAttachTarget {
    fn with_live<T>(&self, attach: impl FnOnce(*mut sys::mln_map) -> T) -> Result<T>;
}

impl MapAttachTarget for NativeMapHandle {
    fn with_live<T>(&self, attach: impl FnOnce(*mut sys::mln_map) -> T) -> Result<T> {
        self.with_live_for_attach(attach)
    }
}

impl MapAttachTarget for NativeMapAttachReference {
    fn with_live<T>(&self, attach: impl FnOnce(*mut sys::mln_map) -> T) -> Result<T> {
        self.with_live(attach)
    }
}

fn attach_render_session(
    map: &impl MapAttachTarget,
    attach: impl FnOnce(*mut sys::mln_map, *mut *mut sys::mln_render_session) -> sys::mln_status,
) -> Result<NativeRenderSessionHandle> {
    let mut session = std::ptr::null_mut();
    let status = map.with_live(|map| attach(map, &mut session))?;
    core::check(status).map_err(error::from_core)?;
    let state = unsafe { NativeHandleState::from_raw_ptr(session, "RenderSessionHandle") }
        .map_err(error::from_core)?;
    Ok(NativeRenderSessionHandle {
        state,
        frame_acquired: Cell::new(false),
    })
}

fn ptr_to_bigint(value: *mut c_void) -> BigInt {
    BigInt::from(value as usize as u64)
}

fn bigint_to_ptr(value: &BigInt, field_name: &str) -> Result<*mut c_void> {
    let value = bigint_to_u64(value, field_name)?;
    Ok(value as usize as *mut c_void)
}

fn optional_bigint_to_ptr(value: Option<&BigInt>, field_name: &str) -> Result<*mut c_void> {
    value
        .map(|value| bigint_to_ptr(value, field_name))
        .transpose()
        .map(|value| value.unwrap_or(std::ptr::null_mut()))
}

fn bigint_to_u64(value: &BigInt, field_name: &str) -> Result<u64> {
    let (signed, value, lossless) = value.get_u64();
    if signed || !lossless {
        return Err(error::invalid_argument(format!(
            "{field_name} must be an unsigned pointer-sized bigint"
        )));
    }
    Ok(value)
}

impl Drop for NativeRenderSessionHandle {
    fn drop(&mut self) {
        if let Some(address) = self.state.leak_for_report() {
            crate::maplibre::report_native_handle_leak(self.state.type_name(), address);
        }
    }
}
