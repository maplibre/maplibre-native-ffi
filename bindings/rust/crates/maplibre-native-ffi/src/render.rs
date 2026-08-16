use std::fmt;
use std::marker::PhantomData;
use std::mem;
use std::rc::Rc;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::{Arc, Mutex};

pub use maplibre_core::{PremultipliedRgba8Image, TextureImageInfo};
use maplibre_native_ffi_core as maplibre_core;
use maplibre_native_ffi_core::{OpenGLClientApi, OpenGLContextOwnership};
use maplibre_native_ffi_sys as sys;

use crate::handle::{ConcurrentNativeHandle, closed_handle_error, out_handle};
use crate::map::MapHandle;
use crate::runtime::{OperationHandle, OperationKind, OperationRegistry};
use crate::{HandleOperationError, Result};

/// Borrowed opaque native address used for backend interop handles. It does not
/// own, retain, dereference, or validate the pointed-to object, and passing it
/// to MapLibre Native transfers no ownership.
#[derive(Clone, Copy, PartialEq, Eq, Hash)]
pub struct NativePointer {
    address: usize,
    _thread_affine: PhantomData<Rc<()>>,
}

impl NativePointer {
    /// Null backend handle value.
    pub const NULL: Self = Self {
        address: 0,
        _thread_affine: PhantomData,
    };

    /// Creates an opaque borrowed pointer value from a native address.
    ///
    /// # Safety
    ///
    /// The address must have the correct backend-native type for every API it
    /// is passed to, and the native object must stay valid for the whole borrow
    /// that API requires. This wrapper validates nothing.
    pub unsafe fn from_address(address: usize) -> Self {
        if address == 0 {
            Self::NULL
        } else {
            Self {
                address,
                _thread_affine: PhantomData,
            }
        }
    }

    /// Creates an opaque borrowed pointer value from a raw pointer.
    ///
    /// # Safety
    ///
    /// The pointer must satisfy the same requirements as
    /// [`NativePointer::from_address`].
    pub unsafe fn from_ptr<T>(ptr: *mut T) -> Self {
        // SAFETY: The caller upholds the native pointer lifetime and type
        // requirements documented above; this conversion only stores the address.
        unsafe { Self::from_address(ptr as usize) }
    }

    /// Returns this opaque value as an integer address.
    pub fn address(self) -> usize {
        self.address
    }

    /// Returns whether this value is null.
    pub fn is_null(self) -> bool {
        self.address == 0
    }

    /// Reconstructs a raw pointer for a backend interop call.
    ///
    /// # Safety
    ///
    /// The caller must choose the correct pointer type and uphold the lifetime,
    /// thread-affinity, synchronization, and aliasing requirements of the
    /// backend API that will receive the pointer.
    pub unsafe fn as_ptr<T>(self) -> *mut T {
        self.address as *mut T
    }

    fn as_void_ptr(self) -> *mut std::ffi::c_void {
        self.address as *mut std::ffi::c_void
    }
}

impl fmt::Debug for NativePointer {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "NativePointer(0x{:x})", self.address)
    }
}

/// Borrowed opaque native address whose validity is tied to an active texture
/// frame. It does not own, retain, dereference, or validate the pointed-to
/// object.
#[derive(Clone, Copy, PartialEq, Eq, Hash)]
pub struct FrameNativePointer<'frame> {
    address: usize,
    _frame: PhantomData<&'frame ()>,
    _thread_affine: PhantomData<Rc<()>>,
}

impl<'frame> FrameNativePointer<'frame> {
    unsafe fn from_ptr<T>(ptr: *mut T) -> Self {
        Self {
            address: ptr as usize,
            _frame: PhantomData,
            _thread_affine: PhantomData,
        }
    }

    /// Returns this opaque value as an integer address.
    ///
    /// # Safety
    ///
    /// The returned integer no longer carries this value's frame lifetime. The
    /// caller must use it only while the borrowed frame remains open and must
    /// satisfy the backend API's type, synchronization, and thread-affinity
    /// requirements.
    pub unsafe fn address(self) -> usize {
        self.address
    }

    /// Returns whether this value is null.
    pub fn is_null(self) -> bool {
        self.address == 0
    }

    /// Reconstructs a raw pointer for a backend interop call.
    ///
    /// # Safety
    ///
    /// The caller must choose the correct pointer type and uphold the lifetime,
    /// thread-affinity, synchronization, and aliasing requirements of the
    /// backend API that will receive the pointer.
    pub unsafe fn as_ptr<T>(self) -> *mut T {
        self.address as *mut T
    }
}

impl fmt::Debug for FrameNativePointer<'_> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "FrameNativePointer(0x{:x})", self.address)
    }
}

/// Borrowed OpenGL texture object name tied to an active texture frame.
#[derive(Clone, Copy, PartialEq, Eq, Hash)]
pub struct FrameOpenGLTextureName<'frame> {
    name: u32,
    _frame: PhantomData<&'frame ()>,
    _thread_affine: PhantomData<Rc<()>>,
}

impl<'frame> FrameOpenGLTextureName<'frame> {
    fn new(name: u32) -> Self {
        Self {
            name,
            _frame: PhantomData,
            _thread_affine: PhantomData,
        }
    }

    /// Returns whether this OpenGL texture object name is zero.
    pub fn is_zero(self) -> bool {
        self.name == 0
    }

    /// Returns the OpenGL texture object name.
    ///
    /// # Safety
    ///
    /// The returned integer no longer carries this value's frame lifetime. Use
    /// it only while the borrowed frame remains open and satisfy OpenGL
    /// synchronization and context-share-group requirements.
    pub unsafe fn value(self) -> u32 {
        self.name
    }
}

impl fmt::Debug for FrameOpenGLTextureName<'_> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "FrameOpenGLTextureName({})", self.name)
    }
}

mod query;
pub use query::{
    FeatureStateSelector, QueriedFeature, RenderedFeatureQueryOptions, RenderedQueryGeometry,
    SourceFeatureQueryOptions,
};
#[derive(Debug, Clone, PartialEq)]
#[non_exhaustive]
pub struct RenderTargetExtent {
    pub width: u32,
    pub height: u32,
    pub scale_factor: f64,
}

impl RenderTargetExtent {
    pub fn new(width: u32, height: u32, scale_factor: f64) -> Self {
        Self {
            width,
            height,
            scale_factor,
        }
    }

    pub(crate) fn to_core(&self) -> maplibre_core::render::RenderTargetExtentFields {
        maplibre_core::render::RenderTargetExtentFields {
            width: self.width,
            height: self.height,
            scale_factor: self.scale_factor,
        }
    }

    /// Returns this extent's physical device-pixel size as
    /// `ceil(logical * scale_factor)` per dimension. Surface and session-owned
    /// texture targets are sized this way; borrowed texture targets state their
    /// physical size instead.
    pub fn physical_size(&self) -> Result<(u32, u32)> {
        let native = maplibre_core::render::render_target_extent_to_native(self.to_core());
        let mut width = 0u32;
        let mut height = 0u32;
        // SAFETY: native is a fully initialized extent and both out pointers
        // reference live locals for the duration of the call.
        maplibre_core::check(unsafe {
            sys::mln_render_target_extent_physical_size(&native, &mut width, &mut height)
        })?;
        Ok((width, height))
    }
}

impl Default for RenderTargetExtent {
    fn default() -> Self {
        Self::new(256, 256, 1.0)
    }
}

#[derive(Debug, Clone, PartialEq)]
#[non_exhaustive]
pub struct MetalContextDescriptor {
    pub device: NativePointer,
}

impl MetalContextDescriptor {
    pub fn new(device: NativePointer) -> Self {
        Self { device }
    }

    pub(crate) fn to_core(&self) -> maplibre_core::render::MetalContextDescriptorFields {
        maplibre_core::render::MetalContextDescriptorFields {
            device: self.device.as_void_ptr(),
        }
    }
}

impl Default for MetalContextDescriptor {
    fn default() -> Self {
        Self::new(NativePointer::NULL)
    }
}

#[derive(Debug, Clone, PartialEq)]
#[non_exhaustive]
pub struct VulkanContextDescriptor {
    pub instance: NativePointer,
    pub physical_device: NativePointer,
    pub device: NativePointer,
    pub graphics_queue: NativePointer,
    pub graphics_queue_family_index: u32,
    pub get_instance_proc_addr: NativePointer,
    pub get_device_proc_addr: NativePointer,
}

impl VulkanContextDescriptor {
    #[allow(clippy::too_many_arguments)]
    pub fn new(
        instance: NativePointer,
        physical_device: NativePointer,
        device: NativePointer,
        graphics_queue: NativePointer,
        graphics_queue_family_index: u32,
    ) -> Self {
        Self {
            instance,
            physical_device,
            device,
            graphics_queue,
            graphics_queue_family_index,
            get_instance_proc_addr: NativePointer::NULL,
            get_device_proc_addr: NativePointer::NULL,
        }
    }

    pub(crate) fn to_core(&self) -> maplibre_core::render::VulkanContextDescriptorFields {
        maplibre_core::render::VulkanContextDescriptorFields {
            instance: self.instance.as_void_ptr(),
            physical_device: self.physical_device.as_void_ptr(),
            device: self.device.as_void_ptr(),
            graphics_queue: self.graphics_queue.as_void_ptr(),
            graphics_queue_family_index: self.graphics_queue_family_index,
            get_instance_proc_addr: self.get_instance_proc_addr.as_void_ptr(),
            get_device_proc_addr: self.get_device_proc_addr.as_void_ptr(),
        }
    }
}

impl Default for VulkanContextDescriptor {
    fn default() -> Self {
        Self::new(
            NativePointer::NULL,
            NativePointer::NULL,
            NativePointer::NULL,
            NativePointer::NULL,
            0,
        )
    }
}

/// Browser WebGPU device a session renders with.
///
/// A browser host owns its WebGPU objects, so a session borrows these rather
/// than creating any of them. They must stay valid until the session is
/// detached or closed.
#[derive(Debug, Clone, PartialEq)]
#[non_exhaustive]
pub struct WebGpuContextDescriptor {
    /// Optional for texture sessions.
    pub instance: NativePointer,
    pub device: NativePointer,
    /// Optional; null uses the device's default queue. A non-null queue must
    /// belong to `device`.
    pub queue: NativePointer,
}

impl WebGpuContextDescriptor {
    pub fn new(device: NativePointer) -> Self {
        Self {
            instance: NativePointer::NULL,
            device,
            queue: NativePointer::NULL,
        }
    }

    pub(crate) fn to_core(&self) -> maplibre_core::render::WebGpuContextDescriptorFields {
        maplibre_core::render::WebGpuContextDescriptorFields {
            instance: self.instance.as_void_ptr(),
            device: self.device.as_void_ptr(),
            queue: self.queue.as_void_ptr(),
        }
    }
}

impl Default for WebGpuContextDescriptor {
    fn default() -> Self {
        Self::new(NativePointer::NULL)
    }
}

#[derive(Debug, Clone, PartialEq)]
#[non_exhaustive]
pub struct WglContextDescriptor {
    pub device_context: NativePointer,
    /// Borrowed `HGLRC` whose share group the session context joins. Required
    /// under shared ownership. A dedicated session joins no share group, so it
    /// is null there.
    pub share_context: NativePointer,
    pub get_proc_address: NativePointer,
    /// Whether the session shares its thread with host graphics work.
    pub ownership: OpenGLContextOwnership,
}

impl WglContextDescriptor {
    pub fn new(device_context: NativePointer, share_context: NativePointer) -> Self {
        Self {
            device_context,
            share_context,
            get_proc_address: NativePointer::NULL,
            ownership: OpenGLContextOwnership::Shared,
        }
    }

    pub(crate) fn to_core(&self) -> maplibre_core::render::WglContextDescriptorFields {
        maplibre_core::render::WglContextDescriptorFields {
            device_context: self.device_context.as_void_ptr(),
            share_context: self.share_context.as_void_ptr(),
            get_proc_address: self.get_proc_address.as_void_ptr(),
            ownership: self.ownership.as_raw(),
        }
    }
}

impl Default for WglContextDescriptor {
    fn default() -> Self {
        Self::new(NativePointer::NULL, NativePointer::NULL)
    }
}

#[derive(Debug, Clone, PartialEq)]
#[non_exhaustive]
pub struct EglContextDescriptor {
    pub display: NativePointer,
    pub config: NativePointer,
    /// Borrowed `EGLContext` whose share group the session context joins.
    /// Required under shared ownership, where the session also takes its client
    /// API from this context. A dedicated session joins no share group, so it
    /// is null there and names [`client_api`](Self::client_api) instead.
    pub share_context: NativePointer,
    /// Client API the session creates its context for. Required under dedicated
    /// ownership. A shared session queries
    /// [`share_context`](Self::share_context) for it, so this is ignored there.
    pub client_api: OpenGLClientApi,
    pub get_proc_address: NativePointer,
    /// Whether the session shares its thread with host graphics work.
    pub ownership: OpenGLContextOwnership,
}

impl EglContextDescriptor {
    pub fn new(
        display: NativePointer,
        config: NativePointer,
        share_context: NativePointer,
    ) -> Self {
        Self {
            display,
            config,
            share_context,
            client_api: OpenGLClientApi::Unspecified,
            get_proc_address: NativePointer::NULL,
            ownership: OpenGLContextOwnership::Shared,
        }
    }

    pub(crate) fn to_core(&self) -> maplibre_core::render::EglContextDescriptorFields {
        maplibre_core::render::EglContextDescriptorFields {
            display: self.display.as_void_ptr(),
            config: self.config.as_void_ptr(),
            share_context: self.share_context.as_void_ptr(),
            client_api: self.client_api.as_raw(),
            get_proc_address: self.get_proc_address.as_void_ptr(),
            ownership: self.ownership.as_raw(),
        }
    }
}

impl Default for EglContextDescriptor {
    fn default() -> Self {
        Self::new(
            NativePointer::NULL,
            NativePointer::NULL,
            NativePointer::NULL,
        )
    }
}

/// Browser WebGL context placement.
#[derive(Debug, Clone, PartialEq)]
pub enum WebGlContextDescriptor {
    /// A host-created WebGL context on the caller graphics thread.
    Existing { context: i32 },
    /// A transferable canvas selector whose WebGL2 context is created on the
    /// native core worker.
    TransferredCanvas { canvas_selector: String },
}

impl WebGlContextDescriptor {
    pub fn existing(context: i32) -> Self {
        Self::Existing { context }
    }

    pub fn transferred_canvas(canvas_selector: impl Into<String>) -> Self {
        Self::TransferredCanvas {
            canvas_selector: canvas_selector.into(),
        }
    }

    pub(crate) fn to_core(&self) -> maplibre_core::render::WebGlContextDescriptorFields {
        match self {
            Self::Existing { context } => maplibre_core::render::WebGlContextDescriptorFields {
                kind: sys::MLN_WEBGL_CONTEXT_EXISTING,
                context: *context,
                canvas_selector_data: std::ptr::null(),
                canvas_selector_size: 0,
            },
            Self::TransferredCanvas { canvas_selector } => {
                maplibre_core::render::WebGlContextDescriptorFields {
                    kind: sys::MLN_WEBGL_CONTEXT_TRANSFERRED_CANVAS,
                    context: 0,
                    canvas_selector_data: canvas_selector.as_ptr(),
                    canvas_selector_size: canvas_selector.len(),
                }
            }
        }
    }
}

/// OpenGL platform context a render session draws through.
///
/// Each platform descriptor carries its own thread ownership. A browser session
/// renders through the host's own WebGL context, so it is shared only.
#[derive(Debug, Clone, PartialEq)]
#[non_exhaustive]
pub enum OpenGLContextDescriptor {
    Wgl(WglContextDescriptor),
    Egl(EglContextDescriptor),
    WebGl(WebGlContextDescriptor),
}

impl OpenGLContextDescriptor {
    pub(crate) fn to_core(&self) -> maplibre_core::render::OpenGLContextDescriptorFields {
        match self {
            Self::Wgl(descriptor) => {
                maplibre_core::render::OpenGLContextDescriptorFields::Wgl(descriptor.to_core())
            }
            Self::Egl(descriptor) => {
                maplibre_core::render::OpenGLContextDescriptorFields::Egl(descriptor.to_core())
            }
            Self::WebGl(descriptor) => {
                maplibre_core::render::OpenGLContextDescriptorFields::WebGl(descriptor.to_core())
            }
        }
    }
}

#[derive(Debug, Clone, PartialEq)]
#[non_exhaustive]
pub struct MetalSurfaceDescriptor {
    pub extent: RenderTargetExtent,
    pub context: MetalContextDescriptor,
    pub layer: NativePointer,
}

impl MetalSurfaceDescriptor {
    pub fn new(
        extent: RenderTargetExtent,
        context: MetalContextDescriptor,
        layer: NativePointer,
    ) -> Self {
        Self {
            extent,
            context,
            layer,
        }
    }

    pub(crate) fn to_native(&self) -> sys::mln_metal_surface_descriptor {
        maplibre_core::render::metal_surface_descriptor_to_native(
            maplibre_core::render::MetalSurfaceDescriptorFields {
                extent: self.extent.to_core(),
                context: self.context.to_core(),
                layer: self.layer.as_void_ptr(),
            },
        )
    }
}

#[derive(Debug, Clone, PartialEq)]
#[non_exhaustive]
pub struct VulkanSurfaceDescriptor {
    pub extent: RenderTargetExtent,
    pub context: VulkanContextDescriptor,
    pub surface: NativePointer,
}

impl VulkanSurfaceDescriptor {
    pub fn new(
        extent: RenderTargetExtent,
        context: VulkanContextDescriptor,
        surface: NativePointer,
    ) -> Self {
        Self {
            extent,
            context,
            surface,
        }
    }

    pub(crate) fn to_native(&self) -> sys::mln_vulkan_surface_descriptor {
        maplibre_core::render::vulkan_surface_descriptor_to_native(
            maplibre_core::render::VulkanSurfaceDescriptorFields {
                extent: self.extent.to_core(),
                context: self.context.to_core(),
                surface: self.surface.as_void_ptr(),
            },
        )
    }
}

/// WebGPU native surface session attachment options.
///
/// The surface is borrowed: the host creates it from whatever it presents to,
/// which in a browser is a canvas, and keeps it alive for the session. The
/// format is the host's too, because a surface reports what it supports through
/// its adapter, which this descriptor does not carry.
#[derive(Debug, Clone, PartialEq)]
#[non_exhaustive]
pub struct WebGpuSurfaceDescriptor {
    pub extent: RenderTargetExtent,
    pub context: WebGpuContextDescriptor,
    pub surface: NativePointer,
    pub format: u32,
}

impl WebGpuSurfaceDescriptor {
    pub fn new(
        extent: RenderTargetExtent,
        context: WebGpuContextDescriptor,
        surface: NativePointer,
        format: u32,
    ) -> Self {
        Self {
            extent,
            context,
            surface,
            format,
        }
    }

    pub(crate) fn to_native(&self) -> sys::mln_webgpu_surface_descriptor {
        maplibre_core::render::webgpu_surface_descriptor_to_native(
            maplibre_core::render::WebGpuSurfaceDescriptorFields {
                extent: self.extent.to_core(),
                context: self.context.to_core(),
                surface: self.surface.as_void_ptr(),
                format: self.format,
            },
        )
    }
}

#[derive(Debug, Clone, PartialEq)]
#[non_exhaustive]
pub struct OpenGLSurfaceDescriptor {
    pub extent: RenderTargetExtent,
    pub context: OpenGLContextDescriptor,
    pub surface: NativePointer,
}

impl OpenGLSurfaceDescriptor {
    pub fn new(
        extent: RenderTargetExtent,
        context: OpenGLContextDescriptor,
        surface: NativePointer,
    ) -> Self {
        Self {
            extent,
            context,
            surface,
        }
    }

    pub(crate) fn to_native(&self) -> sys::mln_opengl_surface_descriptor {
        maplibre_core::render::opengl_surface_descriptor_to_native(
            maplibre_core::render::OpenGLSurfaceDescriptorFields {
                extent: self.extent.to_core(),
                context: self.context.to_core(),
                surface: self.surface.as_void_ptr(),
            },
        )
    }
}

#[derive(Debug, Clone, PartialEq)]
#[non_exhaustive]
pub struct MetalOwnedTextureDescriptor {
    pub extent: RenderTargetExtent,
    pub context: MetalContextDescriptor,
}

impl MetalOwnedTextureDescriptor {
    pub fn new(extent: RenderTargetExtent, context: MetalContextDescriptor) -> Self {
        Self { extent, context }
    }

    pub(crate) fn to_native(&self) -> sys::mln_metal_owned_texture_descriptor {
        maplibre_core::render::metal_owned_texture_descriptor_to_native(
            maplibre_core::render::MetalOwnedTextureDescriptorFields {
                extent: self.extent.to_core(),
                context: self.context.to_core(),
            },
        )
    }
}

#[derive(Debug, Clone, PartialEq)]
#[non_exhaustive]
pub struct MetalBorrowedTextureDescriptor {
    pub extent: RenderTargetExtent,
    /// Physical texture size in device pixels. The texture is sized by its
    /// owner, so this is stated rather than derived from `extent`.
    pub physical_width: u32,
    pub physical_height: u32,
    pub texture: NativePointer,
}

impl MetalBorrowedTextureDescriptor {
    pub fn new(
        extent: RenderTargetExtent,
        physical_width: u32,
        physical_height: u32,
        texture: NativePointer,
    ) -> Self {
        Self {
            extent,
            physical_width,
            physical_height,
            texture,
        }
    }

    pub(crate) fn to_native(&self) -> sys::mln_metal_borrowed_texture_descriptor {
        maplibre_core::render::metal_borrowed_texture_descriptor_to_native(
            maplibre_core::render::MetalBorrowedTextureDescriptorFields {
                extent: self.extent.to_core(),
                physical_width: self.physical_width,
                physical_height: self.physical_height,
                texture: self.texture.as_void_ptr(),
            },
        )
    }
}

#[derive(Debug, Clone, PartialEq)]
#[non_exhaustive]
pub struct VulkanOwnedTextureDescriptor {
    pub extent: RenderTargetExtent,
    pub context: VulkanContextDescriptor,
}

impl VulkanOwnedTextureDescriptor {
    pub fn new(extent: RenderTargetExtent, context: VulkanContextDescriptor) -> Self {
        Self { extent, context }
    }

    pub(crate) fn to_native(&self) -> sys::mln_vulkan_owned_texture_descriptor {
        maplibre_core::render::vulkan_owned_texture_descriptor_to_native(
            maplibre_core::render::VulkanOwnedTextureDescriptorFields {
                extent: self.extent.to_core(),
                context: self.context.to_core(),
            },
        )
    }
}

#[derive(Debug, Clone, PartialEq)]
#[non_exhaustive]
pub struct VulkanBorrowedTextureDescriptor {
    pub extent: RenderTargetExtent,
    /// Physical image size in device pixels. The image is sized by its owner,
    /// so this is stated rather than derived from `extent`.
    pub physical_width: u32,
    pub physical_height: u32,
    pub context: VulkanContextDescriptor,
    pub image: NativePointer,
    pub image_view: NativePointer,
    pub format: u32,
    pub initial_layout: u32,
    pub final_layout: u32,
}

impl VulkanBorrowedTextureDescriptor {
    #[allow(clippy::too_many_arguments)]
    pub fn new(
        extent: RenderTargetExtent,
        physical_width: u32,
        physical_height: u32,
        context: VulkanContextDescriptor,
        image: NativePointer,
        image_view: NativePointer,
        format: u32,
        initial_layout: u32,
        final_layout: u32,
    ) -> Self {
        Self {
            extent,
            physical_width,
            physical_height,
            context,
            image,
            image_view,
            format,
            initial_layout,
            final_layout,
        }
    }

    pub(crate) fn to_native(&self) -> sys::mln_vulkan_borrowed_texture_descriptor {
        maplibre_core::render::vulkan_borrowed_texture_descriptor_to_native(
            maplibre_core::render::VulkanBorrowedTextureDescriptorFields {
                extent: self.extent.to_core(),
                physical_width: self.physical_width,
                physical_height: self.physical_height,
                context: self.context.to_core(),
                image: self.image.as_void_ptr(),
                image_view: self.image_view.as_void_ptr(),
                format: self.format,
                initial_layout: self.initial_layout,
                final_layout: self.final_layout,
            },
        )
    }
}

#[derive(Debug, Clone, PartialEq)]
#[non_exhaustive]
pub struct WebGpuOwnedTextureDescriptor {
    pub extent: RenderTargetExtent,
    pub context: WebGpuContextDescriptor,
}

impl WebGpuOwnedTextureDescriptor {
    pub fn new(extent: RenderTargetExtent, context: WebGpuContextDescriptor) -> Self {
        Self { extent, context }
    }

    pub(crate) fn to_native(&self) -> sys::mln_webgpu_owned_texture_descriptor {
        maplibre_core::render::webgpu_owned_texture_descriptor_to_native(
            maplibre_core::render::WebGpuOwnedTextureDescriptorFields {
                extent: self.extent.to_core(),
                context: self.context.to_core(),
            },
        )
    }
}

#[derive(Debug, Clone, PartialEq)]
#[non_exhaustive]
pub struct WebGpuBorrowedTextureDescriptor {
    pub extent: RenderTargetExtent,
    /// Physical texture size in device pixels. The texture is sized by its
    /// owner, so this is stated rather than derived from `extent`.
    pub physical_width: u32,
    pub physical_height: u32,
    pub context: WebGpuContextDescriptor,
    pub texture: NativePointer,
    pub texture_view: NativePointer,
    /// Backend-native `WGPUTextureFormat` value.
    pub format: u32,
}

impl WebGpuBorrowedTextureDescriptor {
    #[allow(clippy::too_many_arguments)]
    pub fn new(
        extent: RenderTargetExtent,
        physical_width: u32,
        physical_height: u32,
        context: WebGpuContextDescriptor,
        texture: NativePointer,
        texture_view: NativePointer,
        format: u32,
    ) -> Self {
        Self {
            extent,
            physical_width,
            physical_height,
            context,
            texture,
            texture_view,
            format,
        }
    }

    pub(crate) fn to_native(&self) -> sys::mln_webgpu_borrowed_texture_descriptor {
        maplibre_core::render::webgpu_borrowed_texture_descriptor_to_native(
            maplibre_core::render::WebGpuBorrowedTextureDescriptorFields {
                extent: self.extent.to_core(),
                physical_width: self.physical_width,
                physical_height: self.physical_height,
                context: self.context.to_core(),
                texture: self.texture.as_void_ptr(),
                texture_view: self.texture_view.as_void_ptr(),
                format: self.format,
            },
        )
    }
}

#[derive(Debug, Clone, PartialEq)]
#[non_exhaustive]
pub struct OpenGLOwnedTextureDescriptor {
    pub extent: RenderTargetExtent,
    pub context: OpenGLContextDescriptor,
}

impl OpenGLOwnedTextureDescriptor {
    pub fn new(extent: RenderTargetExtent, context: OpenGLContextDescriptor) -> Self {
        Self { extent, context }
    }

    pub(crate) fn to_native(&self) -> sys::mln_opengl_owned_texture_descriptor {
        maplibre_core::render::opengl_owned_texture_descriptor_to_native(
            maplibre_core::render::OpenGLOwnedTextureDescriptorFields {
                extent: self.extent.to_core(),
                context: self.context.to_core(),
            },
        )
    }
}

#[derive(Debug, Clone, PartialEq)]
#[non_exhaustive]
pub struct OpenGLBorrowedTextureDescriptor {
    pub extent: RenderTargetExtent,
    /// Physical texture size in device pixels. The texture is sized by its
    /// owner, so this is stated rather than derived from `extent`.
    pub physical_width: u32,
    pub physical_height: u32,
    pub context: OpenGLContextDescriptor,
    pub texture: u32,
    pub target: u32,
}

impl OpenGLBorrowedTextureDescriptor {
    pub fn new(
        extent: RenderTargetExtent,
        physical_width: u32,
        physical_height: u32,
        context: OpenGLContextDescriptor,
        texture: u32,
        target: u32,
    ) -> Self {
        Self {
            extent,
            physical_width,
            physical_height,
            context,
            texture,
            target,
        }
    }

    pub(crate) fn to_native(&self) -> sys::mln_opengl_borrowed_texture_descriptor {
        maplibre_core::render::opengl_borrowed_texture_descriptor_to_native(
            maplibre_core::render::OpenGLBorrowedTextureDescriptorFields {
                extent: self.extent.to_core(),
                physical_width: self.physical_width,
                physical_height: self.physical_height,
                context: self.context.to_core(),
                texture: self.texture,
                target: self.target,
            },
        )
    }
}

/// Native execution placement for a render session.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum RenderDriverKind {
    CoreWorker,
    CallerGraphicsThread,
    Unknown(u32),
}

impl RenderDriverKind {
    fn to_native(self) -> u32 {
        match self {
            Self::CoreWorker => sys::MLN_RENDER_DRIVER_CORE_WORKER,
            Self::CallerGraphicsThread => sys::MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD,
            Self::Unknown(value) => value,
        }
    }

    fn from_native(value: u32) -> Self {
        match value {
            sys::MLN_RENDER_DRIVER_CORE_WORKER => Self::CoreWorker,
            sys::MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD => Self::CallerGraphicsThread,
            value => Self::Unknown(value),
        }
    }
}

/// Attachment policy. Notification roles inherit the map runtime's native
/// receiver-scoped source.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct RenderSessionAttachOptions {
    pub driver: RenderDriverKind,
    pub requested_texture_ring_depth: u32,
}

impl RenderSessionAttachOptions {
    pub fn core_worker(requested_texture_ring_depth: u32) -> Self {
        Self {
            driver: RenderDriverKind::CoreWorker,
            requested_texture_ring_depth,
        }
    }

    pub fn caller_graphics_thread(requested_texture_ring_depth: u32) -> Self {
        Self {
            driver: RenderDriverKind::CallerGraphicsThread,
            requested_texture_ring_depth,
        }
    }

    fn to_native(self) -> sys::mln_render_session_attach_options {
        let mut raw = unsafe { sys::mln_render_session_attach_options_default() };
        raw.driver = self.driver.to_native();
        raw.requested_texture_ring_depth = self.requested_texture_ring_depth;
        raw
    }
}

impl Default for RenderSessionAttachOptions {
    fn default() -> Self {
        Self::core_worker(1)
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct RenderSessionCapabilities {
    pub driver: RenderDriverKind,
    pub texture_ring_depth: u32,
    pub frame_acquisition: bool,
    pub readback: bool,
    pub consumer_sync: bool,
    pub presentation: bool,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum RenderSessionLifecycle {
    Attaching,
    Attached,
    Detaching,
    Detached,
    TargetLost,
    Abandoned,
    Unknown(u32),
}

fn lifecycle_from_native(value: u32) -> RenderSessionLifecycle {
    match value {
        sys::MLN_RENDER_SESSION_STATE_ATTACHING => RenderSessionLifecycle::Attaching,
        sys::MLN_RENDER_SESSION_STATE_ATTACHED => RenderSessionLifecycle::Attached,
        sys::MLN_RENDER_SESSION_STATE_DETACHING => RenderSessionLifecycle::Detaching,
        sys::MLN_RENDER_SESSION_STATE_DETACHED => RenderSessionLifecycle::Detached,
        sys::MLN_RENDER_SESSION_STATE_TARGET_LOST => RenderSessionLifecycle::TargetLost,
        sys::MLN_RENDER_SESSION_STATE_ABANDONED => RenderSessionLifecycle::Abandoned,
        value => RenderSessionLifecycle::Unknown(value),
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum FrameDisposition {
    Rendered,
    NoUpdate,
    SizePending,
    TargetNotReady,
    Superseded,
    DeadlineMissed,
    Unknown(u32),
}

fn disposition_from_native(value: u32) -> FrameDisposition {
    match value {
        sys::MLN_RENDER_RESULT_RENDERED => FrameDisposition::Rendered,
        sys::MLN_RENDER_RESULT_NO_UPDATE => FrameDisposition::NoUpdate,
        sys::MLN_RENDER_RESULT_SIZE_PENDING => FrameDisposition::SizePending,
        sys::MLN_RENDER_RESULT_TARGET_NOT_READY => FrameDisposition::TargetNotReady,
        sys::MLN_RENDER_RESULT_SUPERSEDED => FrameDisposition::Superseded,
        sys::MLN_RENDER_RESULT_DEADLINE_MISSED => FrameDisposition::DeadlineMissed,
        value => FrameDisposition::Unknown(value),
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct FrameDemand {
    pub if_needed: bool,
    pub present: bool,
    pub token: u64,
    pub coalescing_boundary: u64,
    pub presentation_time_ns: i64,
    pub deadline_ns: i64,
}

impl Default for FrameDemand {
    fn default() -> Self {
        Self {
            if_needed: true,
            present: false,
            token: 0,
            coalescing_boundary: 0,
            presentation_time_ns: 0,
            deadline_ns: 0,
        }
    }
}

impl FrameDemand {
    fn to_native(self) -> sys::mln_frame_demand {
        let mut flags = 0;
        if self.if_needed {
            flags |= sys::MLN_FRAME_DEMAND_IF_NEEDED;
        }
        if self.present {
            flags |= sys::MLN_FRAME_DEMAND_PRESENT;
        }
        sys::mln_frame_demand {
            size: mem::size_of::<sys::mln_frame_demand>() as u32,
            flags,
            token: self.token,
            coalescing_boundary: self.coalescing_boundary,
            presentation_time_ns: self.presentation_time_ns,
            deadline_ns: self.deadline_ns,
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct RenderFrameResult {
    pub disposition: FrameDisposition,
    pub token: u64,
    pub map_update_generation: u64,
    pub extent_generation: u64,
    pub frame_generation: u64,
    pub presentation_time_ns: i64,
    /// Whether the map asked for another frame while it rendered this one, as
    /// during an ongoing camera transition. Set only when `disposition` is
    /// [`FrameDisposition::Rendered`]; false for every other outcome. This is
    /// the same signal the render-frame-finished event carries, delivered with
    /// the frame result so a host can re-arm its frame loop without the
    /// runtime event round trip.
    pub needs_repaint: bool,
}

fn frame_result_from_native(raw: sys::mln_render_frame_result) -> RenderFrameResult {
    RenderFrameResult {
        disposition: disposition_from_native(raw.disposition),
        token: raw.token,
        map_update_generation: raw.map_update_generation,
        extent_generation: raw.extent_generation,
        frame_generation: raw.frame_generation,
        presentation_time_ns: raw.presentation_time_ns,
        needs_repaint: raw.needs_repaint,
    }
}

#[derive(Debug, Clone, PartialEq)]
pub struct RenderSessionSnapshot {
    pub state: RenderSessionLifecycle,
    pub driver: RenderDriverKind,
    pub latest_result: FrameDisposition,
    pub extent: RenderTargetExtent,
    pub generation: u64,
    pub map_update_generation: u64,
    pub rendered_update_generation: u64,
    pub extent_generation: u64,
    pub frame_generation: u64,
    pub latest_demand_token: u64,
    pub pending_demand_count: u32,
    pub acquired_frame_count: u32,
    pub target_ready: bool,
    pub pending_changes: bool,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct RenderAbandonResult {
    pub quarantined: bool,
    pub quarantined_resource_count: u32,
}

#[derive(Debug)]
struct RenderSessionState {
    handle: ConcurrentNativeHandle<sys::mln_render_session>,
    operations: Arc<OperationRegistry>,
}

impl RenderSessionState {
    fn new(native: sys::mln_render_session, operations: Arc<OperationRegistry>) -> Result<Self> {
        let handle = unsafe { ConcurrentNativeHandle::from_handle(native, "mln_render_session") }?;
        Ok(Self { handle, operations })
    }

    fn native(&self) -> Result<sys::mln_render_session> {
        self.handle
            .live_handle()
            .ok_or_else(|| closed_handle_error("RenderSessionHandle"))
    }

    fn operation<T>(
        &self,
        operation: sys::mln_operation,
        kind: OperationKind,
    ) -> Result<OperationHandle<T>> {
        OperationHandle::new(operation, kind, Arc::clone(&self.operations))
    }

    fn destroy(&self) -> Result<()> {
        let Some(session) = self.handle.live_handle() else {
            return Ok(());
        };
        maplibre_core::check(unsafe { sys::mln_render_session_destroy(session) })?;
        self.handle.mark_closed();
        Ok(())
    }
}

impl Drop for RenderSessionState {
    fn drop(&mut self) {
        if self.destroy().is_err() {
            self.handle.leak_for_report();
        }
    }
}

/// Send-safe render-session control handle.
#[derive(Clone)]
pub struct RenderSessionHandle {
    inner: Arc<RenderSessionState>,
}

impl fmt::Debug for RenderSessionHandle {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("RenderSessionHandle")
            .field("closed", &self.inner.handle.is_closed())
            .finish()
    }
}

/// Attaching session and the operation that reports attachment completion.
#[derive(Debug)]
pub struct RenderSessionAttachment {
    pub session: RenderSessionHandle,
    pub operation: OperationHandle<()>,
}

impl RenderSessionHandle {
    pub(crate) fn attach<F>(
        map: &MapHandle,
        options: RenderSessionAttachOptions,
        attach: F,
    ) -> Result<RenderSessionAttachment>
    where
        F: FnOnce(
            sys::mln_map,
            *const sys::mln_render_session_attach_options,
            *mut sys::mln_render_session,
            *mut sys::mln_operation,
        ) -> sys::mln_status,
    {
        let map_native = map.inner.native()?;
        let raw_options = options.to_native();
        let mut session = maplibre_core::ptr::OutHandle::<sys::mln_render_session>::new();
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(attach(
            map_native,
            &raw_options,
            session.as_mut_ptr(),
            &mut operation,
        ))?;
        let session = out_handle(session, "mln_render_session")?;
        let operations = map.operation_registry()?;
        Ok(RenderSessionAttachment {
            session: Self {
                inner: Arc::new(RenderSessionState::new(session, Arc::clone(&operations))?),
            },
            operation: OperationHandle::new(operation, OperationKind::RenderAttach, operations)?,
        })
    }

    fn start_unit(
        &self,
        kind: OperationKind,
        start: impl FnOnce(sys::mln_render_session, *mut sys::mln_operation) -> sys::mln_status,
    ) -> Result<OperationHandle<()>> {
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(start(self.inner.native()?, &mut operation))?;
        self.inner.operation(operation, kind)
    }

    pub fn capabilities(&self) -> Result<RenderSessionCapabilities> {
        let mut raw: sys::mln_render_session_capabilities = unsafe { mem::zeroed() };
        raw.size = mem::size_of::<sys::mln_render_session_capabilities>() as u32;
        maplibre_core::check(unsafe {
            sys::mln_render_session_get_capabilities(self.inner.native()?, &mut raw)
        })?;
        Ok(RenderSessionCapabilities {
            driver: RenderDriverKind::from_native(raw.driver),
            texture_ring_depth: raw.texture_ring_depth,
            frame_acquisition: raw.flags & sys::MLN_RENDER_SESSION_CAPABILITY_FRAME_ACQUISITION
                != 0,
            readback: raw.flags & sys::MLN_RENDER_SESSION_CAPABILITY_READBACK != 0,
            consumer_sync: raw.flags & sys::MLN_RENDER_SESSION_CAPABILITY_CONSUMER_SYNC != 0,
            presentation: raw.flags & sys::MLN_RENDER_SESSION_CAPABILITY_PRESENTATION != 0,
        })
    }

    pub fn snapshot(&self) -> Result<RenderSessionSnapshot> {
        let mut raw: sys::mln_render_session_snapshot = unsafe { mem::zeroed() };
        raw.size = mem::size_of::<sys::mln_render_session_snapshot>() as u32;
        maplibre_core::check(unsafe {
            sys::mln_render_session_get_snapshot(self.inner.native()?, &mut raw)
        })?;
        Ok(RenderSessionSnapshot {
            state: lifecycle_from_native(raw.state),
            driver: RenderDriverKind::from_native(raw.driver),
            latest_result: disposition_from_native(raw.latest_result),
            extent: RenderTargetExtent::new(
                raw.extent.width,
                raw.extent.height,
                raw.extent.scale_factor,
            ),
            generation: raw.generation,
            map_update_generation: raw.map_update_generation,
            rendered_update_generation: raw.rendered_update_generation,
            extent_generation: raw.extent_generation,
            frame_generation: raw.frame_generation,
            latest_demand_token: raw.latest_demand_token,
            pending_demand_count: raw.pending_demand_count,
            acquired_frame_count: raw.acquired_frame_count,
            target_ready: raw.target_ready,
            pending_changes: raw.pending_changes,
        })
    }

    pub fn request_frame(&self, demand: FrameDemand) -> Result<()> {
        let raw = demand.to_native();
        maplibre_core::check(unsafe {
            sys::mln_render_session_request_frame(self.inner.native()?, &raw)
        })
    }

    pub fn drain_frame_results(&self, max_results: usize) -> Result<RenderFrameBatch> {
        let mut batch = sys::mln_render_frame_batch(0);
        maplibre_core::check(unsafe {
            sys::mln_render_session_drain_frame_results(
                self.inner.native()?,
                max_results,
                &mut batch,
            )
        })?;
        RenderFrameBatch::new(batch)
    }

    pub fn acquire_frame(&self) -> Result<AcquiredFrameHandle> {
        let mut frame = sys::mln_acquired_frame(0);
        maplibre_core::check(unsafe {
            sys::mln_render_session_acquire_frame(self.inner.native()?, &mut frame)
        })?;
        AcquiredFrameHandle::new(frame, Arc::clone(&self.inner.operations))
    }

    pub fn resize(&self, extent: &RenderTargetExtent) -> Result<OperationHandle<()>> {
        let raw = maplibre_core::render::render_target_extent_to_native(extent.to_core());
        self.start_unit(OperationKind::RenderResize, |session, operation| unsafe {
            sys::mln_render_session_resize_start(session, &raw, operation)
        })
    }

    pub fn barrier(&self, min_update_generation: u64) -> Result<OperationHandle<()>> {
        self.start_unit(OperationKind::RenderBarrier, |session, operation| unsafe {
            sys::mln_render_session_barrier_start(session, min_update_generation, operation)
        })
    }

    pub fn reduce_memory_use(&self) -> Result<OperationHandle<()>> {
        self.start_unit(
            OperationKind::RenderMaintenance,
            |session, operation| unsafe {
                sys::mln_render_session_reduce_memory_use_start(session, operation)
            },
        )
    }

    pub fn clear_data(&self) -> Result<OperationHandle<()>> {
        self.start_unit(
            OperationKind::RenderMaintenance,
            |session, operation| unsafe {
                sys::mln_render_session_clear_data_start(session, operation)
            },
        )
    }

    pub fn dump_debug_logs(&self) -> Result<OperationHandle<()>> {
        self.start_unit(
            OperationKind::RenderMaintenance,
            |session, operation| unsafe {
                sys::mln_render_session_dump_debug_logs_start(session, operation)
            },
        )
    }

    pub fn read_premultiplied_rgba8(&self) -> Result<OperationHandle<PremultipliedRgba8Image>> {
        let mut operation = sys::mln_operation(0);
        maplibre_core::check(unsafe {
            sys::mln_texture_read_premultiplied_rgba8_start(self.inner.native()?, &mut operation)
        })?;
        self.inner
            .operation(operation, OperationKind::RenderReadback)
    }

    /// Services typed native work on the caller driver's graphics thread.
    ///
    /// The first successful call fixes the graphics-thread identity. Later
    /// calls from another thread return a wrong-thread error.
    pub fn service_driver_work(&self, max_work: usize) -> Result<usize> {
        let mut serviced = 0;
        maplibre_core::check(unsafe {
            sys::mln_render_session_service_driver_work(
                self.inner.native()?,
                max_work,
                &mut serviced,
            )
        })?;
        Ok(serviced)
    }

    pub fn detach(&self) -> Result<OperationHandle<()>> {
        self.start_unit(OperationKind::RenderDetach, |session, operation| unsafe {
            sys::mln_render_session_detach_start(session, operation)
        })
    }

    pub fn abandon(&self) -> Result<RenderAbandonResult> {
        let mut raw: sys::mln_render_abandon_result = unsafe { mem::zeroed() };
        raw.size = mem::size_of::<sys::mln_render_abandon_result>() as u32;
        maplibre_core::check(unsafe {
            sys::mln_render_session_abandon(self.inner.native()?, &mut raw)
        })?;
        Ok(RenderAbandonResult {
            quarantined: raw.disposition == sys::MLN_RENDER_ABANDON_DISPOSITION_QUARANTINED,
            quarantined_resource_count: raw.quarantined_resource_count,
        })
    }

    pub fn destroy(self) -> std::result::Result<(), HandleOperationError<Self>> {
        if let Err(error) = self.inner.destroy() {
            return Err(HandleOperationError::new(error, self));
        }
        Ok(())
    }

    fn set_target<D>(
        &self,
        raw: &D,
        start: unsafe extern "C" fn(
            sys::mln_render_session,
            *const D,
            *mut sys::mln_operation,
        ) -> sys::mln_status,
    ) -> Result<OperationHandle<()>> {
        self.start_unit(
            OperationKind::RenderSetTarget,
            |session, operation| unsafe { start(session, raw, operation) },
        )
    }

    pub fn set_metal_surface_target(
        &self,
        value: &MetalSurfaceDescriptor,
    ) -> Result<OperationHandle<()>> {
        self.set_target(&value.to_native(), sys::mln_metal_surface_set_target_start)
    }

    pub fn set_vulkan_surface_target(
        &self,
        value: &VulkanSurfaceDescriptor,
    ) -> Result<OperationHandle<()>> {
        self.set_target(&value.to_native(), sys::mln_vulkan_surface_set_target_start)
    }

    pub fn set_webgpu_surface_target(
        &self,
        value: &WebGpuSurfaceDescriptor,
    ) -> Result<OperationHandle<()>> {
        self.set_target(&value.to_native(), sys::mln_webgpu_surface_set_target_start)
    }

    pub fn set_opengl_surface_target(
        &self,
        value: &OpenGLSurfaceDescriptor,
    ) -> Result<OperationHandle<()>> {
        self.set_target(&value.to_native(), sys::mln_opengl_surface_set_target_start)
    }

    pub fn set_metal_borrowed_texture_target(
        &self,
        value: &MetalBorrowedTextureDescriptor,
    ) -> Result<OperationHandle<()>> {
        self.set_target(
            &value.to_native(),
            sys::mln_metal_borrowed_texture_set_target_start,
        )
    }

    pub fn set_vulkan_borrowed_texture_target(
        &self,
        value: &VulkanBorrowedTextureDescriptor,
    ) -> Result<OperationHandle<()>> {
        self.set_target(
            &value.to_native(),
            sys::mln_vulkan_borrowed_texture_set_target_start,
        )
    }

    pub fn set_webgpu_borrowed_texture_target(
        &self,
        value: &WebGpuBorrowedTextureDescriptor,
    ) -> Result<OperationHandle<()>> {
        self.set_target(
            &value.to_native(),
            sys::mln_webgpu_borrowed_texture_set_target_start,
        )
    }

    pub fn set_opengl_borrowed_texture_target(
        &self,
        value: &OpenGLBorrowedTextureDescriptor,
    ) -> Result<OperationHandle<()>> {
        self.set_target(
            &value.to_native(),
            sys::mln_opengl_borrowed_texture_set_target_start,
        )
    }
}

pub struct RenderFrameBatch {
    raw: sys::mln_render_frame_batch,
}

impl RenderFrameBatch {
    fn new(raw: sys::mln_render_frame_batch) -> Result<Self> {
        if raw.0 == 0 {
            return Err(crate::Error::invalid_argument(
                "frame batch must not be zero",
            ));
        }
        Ok(Self { raw })
    }

    pub fn len(&self) -> Result<usize> {
        let mut count = 0;
        maplibre_core::check(unsafe { sys::mln_render_frame_batch_count(self.raw, &mut count) })?;
        Ok(count)
    }

    pub fn is_empty(&self) -> Result<bool> {
        Ok(self.len()? == 0)
    }

    pub fn get(&self, index: usize) -> Result<RenderFrameResult> {
        let mut raw: sys::mln_render_frame_result = unsafe { mem::zeroed() };
        raw.size = mem::size_of::<sys::mln_render_frame_result>() as u32;
        maplibre_core::check(unsafe {
            sys::mln_render_frame_batch_get(self.raw, index, &mut raw)
        })?;
        Ok(frame_result_from_native(raw))
    }

    pub fn copy_results(&self) -> Result<Vec<RenderFrameResult>> {
        (0..self.len()?).map(|index| self.get(index)).collect()
    }
}

impl Drop for RenderFrameBatch {
    fn drop(&mut self) {
        unsafe { sys::mln_render_frame_batch_release(self.raw) };
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum GpuSyncKind {
    CpuComplete,
    MetalSharedEvent,
    VulkanTimelineSemaphore,
    OpenGlFence,
    WebGpuToken,
    Unknown(u32),
}

/// Consumer completion passed when releasing an acquired frame.
///
/// A non-CPU backend object remains caller-owned and must stay valid until the
/// returned frame-release operation completes. Constructing its
/// [`NativePointer`] is unsafe because Rust cannot verify that lifetime.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct GpuSync {
    pub kind: GpuSyncKind,
    pub object: NativePointer,
    pub value: u64,
}

impl GpuSync {
    pub const CPU_COMPLETE: Self = Self {
        kind: GpuSyncKind::CpuComplete,
        object: NativePointer::NULL,
        value: 0,
    };
}

/// Producer synchronization borrowed from an acquired frame.
///
/// Backend objects remain valid only while the frame lease is live.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct FrameGpuSync<'frame> {
    pub kind: GpuSyncKind,
    pub object: FrameNativePointer<'frame>,
    pub value: u64,
}

impl FrameGpuSync<'_> {
    fn from_native(raw: sys::mln_gpu_sync) -> Self {
        let kind = match raw.kind {
            sys::MLN_GPU_SYNC_CPU_COMPLETE => GpuSyncKind::CpuComplete,
            sys::MLN_GPU_SYNC_METAL_SHARED_EVENT => GpuSyncKind::MetalSharedEvent,
            sys::MLN_GPU_SYNC_VULKAN_TIMELINE_SEMAPHORE => GpuSyncKind::VulkanTimelineSemaphore,
            sys::MLN_GPU_SYNC_OPENGL_FENCE => GpuSyncKind::OpenGlFence,
            sys::MLN_GPU_SYNC_WEBGPU_TOKEN => GpuSyncKind::WebGpuToken,
            value => GpuSyncKind::Unknown(value),
        };
        Self {
            kind,
            object: unsafe { FrameNativePointer::from_ptr(raw.object) },
            value: raw.value,
        }
    }
}

impl GpuSync {
    fn to_native(self) -> sys::mln_gpu_sync {
        let mut raw = unsafe { sys::mln_gpu_sync_default() };
        raw.kind = match self.kind {
            GpuSyncKind::CpuComplete => sys::MLN_GPU_SYNC_CPU_COMPLETE,
            GpuSyncKind::MetalSharedEvent => sys::MLN_GPU_SYNC_METAL_SHARED_EVENT,
            GpuSyncKind::VulkanTimelineSemaphore => sys::MLN_GPU_SYNC_VULKAN_TIMELINE_SEMAPHORE,
            GpuSyncKind::OpenGlFence => sys::MLN_GPU_SYNC_OPENGL_FENCE,
            GpuSyncKind::WebGpuToken => sys::MLN_GPU_SYNC_WEBGPU_TOKEN,
            GpuSyncKind::Unknown(value) => value,
        };
        raw.object = self.object.as_void_ptr();
        raw.value = self.value;
        raw
    }
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct MetalOwnedTextureFrame {
    pub generation: u64,
    pub width: u32,
    pub height: u32,
    pub scale_factor: f64,
    pub frame_id: u64,
    pub pixel_format: u64,
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct VulkanOwnedTextureFrame {
    pub generation: u64,
    pub width: u32,
    pub height: u32,
    pub scale_factor: f64,
    pub frame_id: u64,
    pub format: u32,
    pub layout: u32,
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct WebGpuOwnedTextureFrame {
    pub generation: u64,
    pub width: u32,
    pub height: u32,
    pub scale_factor: f64,
    pub frame_id: u64,
    pub format: u32,
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct OpenGLOwnedTextureFrame {
    pub generation: u64,
    pub width: u32,
    pub height: u32,
    pub scale_factor: f64,
    pub frame_id: u64,
    pub target: u32,
    pub internal_format: u32,
    pub format: u32,
    pub type_: u32,
}

/// Owned lease for one session-owned texture-ring slot.
///
/// Call [`AcquiredFrameHandle::release`] with the completion of every consumer
/// GPU access. Dropping an untouched frame releases it as CPU-complete.
/// Dropping a frame after exposing backend handles leaks the lease rather than
/// allowing the session to reuse resources that the GPU may still access.
#[must_use = "an acquired frame must be released with the consumer's GPU completion"]
pub struct AcquiredFrameHandle {
    raw: Mutex<Option<sys::mln_acquired_frame>>,
    backend_exposed: AtomicBool,
    operations: Arc<OperationRegistry>,
}

impl AcquiredFrameHandle {
    fn new(raw: sys::mln_acquired_frame, operations: Arc<OperationRegistry>) -> Result<Self> {
        if raw.0 == 0 {
            return Err(crate::Error::invalid_argument(
                "acquired frame must not be zero",
            ));
        }
        Ok(Self {
            raw: Mutex::new(Some(raw)),
            backend_exposed: AtomicBool::new(false),
            operations,
        })
    }

    fn native(&self) -> Result<sys::mln_acquired_frame> {
        self.raw
            .lock()
            .unwrap_or_else(|p| p.into_inner())
            .ok_or_else(|| closed_handle_error("AcquiredFrameHandle"))
    }

    pub fn result(&self) -> Result<RenderFrameResult> {
        let mut raw: sys::mln_render_frame_result = unsafe { mem::zeroed() };
        raw.size = mem::size_of::<sys::mln_render_frame_result>() as u32;
        maplibre_core::check(unsafe {
            sys::mln_acquired_frame_get_result(self.native()?, &mut raw)
        })?;
        Ok(frame_result_from_native(raw))
    }

    pub fn producer_sync(&self) -> Result<FrameGpuSync<'_>> {
        let mut raw = unsafe { sys::mln_gpu_sync_default() };
        maplibre_core::check(unsafe {
            sys::mln_acquired_frame_get_producer_sync(self.native()?, &mut raw)
        })?;
        Ok(FrameGpuSync::from_native(raw))
    }

    pub fn release(
        self,
        consumer_completion: GpuSync,
    ) -> std::result::Result<OperationHandle<()>, HandleOperationError<Self>> {
        let mut guard = self.raw.lock().unwrap_or_else(|p| p.into_inner());
        let Some(mut frame) = *guard else {
            drop(guard);
            return Err(HandleOperationError::new(
                closed_handle_error("AcquiredFrameHandle"),
                self,
            ));
        };
        let sync = consumer_completion.to_native();
        let mut operation = sys::mln_operation(0);
        if let Err(error) = maplibre_core::check(unsafe {
            sys::mln_acquired_frame_release_start(&mut frame, &sync, &mut operation)
        }) {
            drop(guard);
            return Err(HandleOperationError::new(error, self));
        }
        *guard = None;
        drop(guard);
        match OperationHandle::new(
            operation,
            OperationKind::RenderFrameRelease,
            Arc::clone(&self.operations),
        ) {
            Ok(operation) => Ok(operation),
            Err(error) => Err(HandleOperationError::new(error, self)),
        }
    }
}

impl Drop for AcquiredFrameHandle {
    fn drop(&mut self) {
        let Some(mut frame) = self
            .raw
            .get_mut()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
            .take()
        else {
            return;
        };
        if self.backend_exposed.load(Ordering::Acquire) {
            maplibre_core::handle::report_leak(maplibre_core::handle::NativeHandleLeak {
                type_name: "mln_acquired_frame",
                id: frame.0,
            });
            return;
        }
        let sync = GpuSync::CPU_COMPLETE.to_native();
        let mut operation = sys::mln_operation(0);
        let status =
            unsafe { sys::mln_acquired_frame_release_start(&mut frame, &sync, &mut operation) };
        if status == sys::MLN_STATUS_OK && operation.0 != 0 {
            unsafe { sys::mln_operation_release(operation) };
        }
    }
}

impl AcquiredFrameHandle {
    pub fn metal_texture(
        &self,
    ) -> Result<(
        MetalOwnedTextureFrame,
        FrameNativePointer<'_>,
        FrameNativePointer<'_>,
    )> {
        let mut raw: sys::mln_metal_owned_texture_frame = unsafe { mem::zeroed() };
        raw.size = mem::size_of::<sys::mln_metal_owned_texture_frame>() as u32;
        maplibre_core::check(unsafe {
            sys::mln_acquired_frame_get_metal_texture(self.native()?, &mut raw)
        })?;
        self.backend_exposed.store(true, Ordering::Release);
        Ok((
            MetalOwnedTextureFrame {
                generation: raw.generation,
                width: raw.width,
                height: raw.height,
                scale_factor: raw.scale_factor,
                frame_id: raw.frame_id,
                pixel_format: raw.pixel_format,
            },
            unsafe { FrameNativePointer::from_ptr(raw.texture) },
            unsafe { FrameNativePointer::from_ptr(raw.device) },
        ))
    }

    pub fn vulkan_texture(
        &self,
    ) -> Result<(
        VulkanOwnedTextureFrame,
        FrameNativePointer<'_>,
        FrameNativePointer<'_>,
        FrameNativePointer<'_>,
    )> {
        let mut raw: sys::mln_vulkan_owned_texture_frame = unsafe { mem::zeroed() };
        raw.size = mem::size_of::<sys::mln_vulkan_owned_texture_frame>() as u32;
        maplibre_core::check(unsafe {
            sys::mln_acquired_frame_get_vulkan_texture(self.native()?, &mut raw)
        })?;
        self.backend_exposed.store(true, Ordering::Release);
        Ok((
            VulkanOwnedTextureFrame {
                generation: raw.generation,
                width: raw.width,
                height: raw.height,
                scale_factor: raw.scale_factor,
                frame_id: raw.frame_id,
                format: raw.format,
                layout: raw.layout,
            },
            unsafe { FrameNativePointer::from_ptr(raw.image) },
            unsafe { FrameNativePointer::from_ptr(raw.image_view) },
            unsafe { FrameNativePointer::from_ptr(raw.device) },
        ))
    }

    pub fn webgpu_texture(
        &self,
    ) -> Result<(
        WebGpuOwnedTextureFrame,
        FrameNativePointer<'_>,
        FrameNativePointer<'_>,
        FrameNativePointer<'_>,
    )> {
        let mut raw: sys::mln_webgpu_owned_texture_frame = unsafe { mem::zeroed() };
        raw.size = mem::size_of::<sys::mln_webgpu_owned_texture_frame>() as u32;
        maplibre_core::check(unsafe {
            sys::mln_acquired_frame_get_webgpu_texture(self.native()?, &mut raw)
        })?;
        self.backend_exposed.store(true, Ordering::Release);
        Ok((
            WebGpuOwnedTextureFrame {
                generation: raw.generation,
                width: raw.width,
                height: raw.height,
                scale_factor: raw.scale_factor,
                frame_id: raw.frame_id,
                format: raw.format,
            },
            unsafe { FrameNativePointer::from_ptr(raw.texture) },
            unsafe { FrameNativePointer::from_ptr(raw.texture_view) },
            unsafe { FrameNativePointer::from_ptr(raw.device) },
        ))
    }

    /// Copies OpenGL texture data on the fixed caller graphics thread.
    pub fn opengl_texture(&self) -> Result<(OpenGLOwnedTextureFrame, FrameOpenGLTextureName<'_>)> {
        let mut raw: sys::mln_opengl_owned_texture_frame = unsafe { mem::zeroed() };
        raw.size = mem::size_of::<sys::mln_opengl_owned_texture_frame>() as u32;
        maplibre_core::check(unsafe {
            sys::mln_acquired_frame_get_opengl_texture(self.native()?, &mut raw)
        })?;
        self.backend_exposed.store(true, Ordering::Release);
        Ok((
            OpenGLOwnedTextureFrame {
                generation: raw.generation,
                width: raw.width,
                height: raw.height,
                scale_factor: raw.scale_factor,
                frame_id: raw.frame_id,
                target: raw.target,
                internal_format: raw.internal_format,
                format: raw.format,
                type_: raw.type_,
            },
            FrameOpenGLTextureName::new(raw.texture),
        ))
    }
}

impl OperationHandle<PremultipliedRgba8Image> {
    pub fn take(&self) -> Result<PremultipliedRgba8Image> {
        let mut data = sys::mln_buffer(0);
        let mut info = unsafe { sys::mln_texture_image_info_default() };
        self.with_result_operation(|operation| {
            maplibre_core::check(unsafe {
                sys::mln_texture_read_premultiplied_rgba8_take_result(
                    operation, &mut data, &mut info,
                )
            })
        })?;
        Ok(PremultipliedRgba8Image::new(
            maplibre_core::values::texture_image_info_from_native(&info),
            unsafe { maplibre_core::string::copy_owned_buffer(data) }?,
        ))
    }
}

#[cfg(test)]
mod tests;
