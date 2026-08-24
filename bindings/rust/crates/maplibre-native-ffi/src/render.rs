use std::cell::Cell;
use std::fmt;
use std::marker::PhantomData;
use std::mem;
use std::rc::Rc;

pub use maplibre_core::{PremultipliedRgba8Image, TextureImageInfo};
use maplibre_native_ffi_core as maplibre_core;
use maplibre_native_ffi_core::{OpenGLClientApi, OpenGLContextOwnership, RenderResult};
use maplibre_native_ffi_sys as sys;

use crate::handle::{ThreadAffineNativeHandle, closed_handle_error, out_handle};
use crate::map::MapAttachRef;
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

/// Bit pattern of a borrowed Vulkan non-dispatchable handle. It transfers no
/// ownership and grants no memory access. Zero represents `VK_NULL_HANDLE`.
#[derive(Clone, Copy, PartialEq, Eq, Hash)]
pub struct VulkanHandle {
    bits: u64,
    _thread_affine: PhantomData<Rc<()>>,
}

impl VulkanHandle {
    /// Null Vulkan non-dispatchable handle.
    pub const NULL: Self = Self {
        bits: 0,
        _thread_affine: PhantomData,
    };

    /// Creates a borrowed Vulkan handle from its native bit pattern.
    ///
    /// # Safety
    ///
    /// The bit pattern must have the Vulkan handle type required by every API
    /// that receives it. The host must keep that object valid and synchronized
    /// for the documented borrow window.
    pub unsafe fn from_bits(bits: u64) -> Self {
        Self {
            bits,
            _thread_affine: PhantomData,
        }
    }

    /// Returns this Vulkan handle's native bit pattern.
    pub fn bits(self) -> u64 {
        self.bits
    }

    /// Returns whether this value represents `VK_NULL_HANDLE`.
    pub fn is_null(self) -> bool {
        self.bits == 0
    }
}

impl fmt::Debug for VulkanHandle {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "VulkanHandle(0x{:x})", self.bits)
    }
}

/// Borrowed Vulkan non-dispatchable handle tied to an active texture frame.
#[derive(Clone, Copy, PartialEq, Eq, Hash)]
pub struct FrameVulkanHandle<'frame> {
    bits: u64,
    _frame: PhantomData<&'frame ()>,
    _thread_affine: PhantomData<Rc<()>>,
}

impl<'frame> FrameVulkanHandle<'frame> {
    fn new(bits: u64) -> Self {
        Self {
            bits,
            _frame: PhantomData,
            _thread_affine: PhantomData,
        }
    }

    /// Returns this Vulkan handle's native bit pattern.
    ///
    /// # Safety
    ///
    /// The returned integer no longer carries this value's frame lifetime. Use
    /// it only while the borrowed frame remains open and satisfy the Vulkan
    /// synchronization requirements for the image or image view.
    pub unsafe fn bits(self) -> u64 {
        self.bits
    }

    /// Returns whether this value represents `VK_NULL_HANDLE`.
    pub fn is_null(self) -> bool {
        self.bits == 0
    }
}

impl fmt::Debug for FrameVulkanHandle<'_> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "FrameVulkanHandle(0x{:x})", self.bits)
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

/// Browser WebGL context a session renders into.
///
/// The host creates the context and keeps owning it; a session shares it rather
/// than holding it exclusively. `context` is an
/// `EMSCRIPTEN_WEBGL_CONTEXT_HANDLE`, which the native library requires to be
/// positive.
#[derive(Debug, Clone, PartialEq)]
#[non_exhaustive]
pub struct WebGlContextDescriptor {
    pub context: i32,
}

impl WebGlContextDescriptor {
    pub fn new(context: i32) -> Self {
        Self { context }
    }

    pub(crate) fn to_core(&self) -> maplibre_core::render::WebGlContextDescriptorFields {
        maplibre_core::render::WebGlContextDescriptorFields {
            context: self.context,
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
    pub surface: VulkanHandle,
}

impl VulkanSurfaceDescriptor {
    pub fn new(
        extent: RenderTargetExtent,
        context: VulkanContextDescriptor,
        surface: VulkanHandle,
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
                surface: self.surface.bits(),
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
    pub image: VulkanHandle,
    pub image_view: VulkanHandle,
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
        image: VulkanHandle,
        image_view: VulkanHandle,
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
                image: self.image.bits(),
                image_view: self.image_view.bits(),
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

#[derive(Debug)]
struct RenderSessionState {
    handle: ThreadAffineNativeHandle<sys::mln_render_session>,
    detached: Cell<bool>,
    frame_acquired: Cell<bool>,
}

impl RenderSessionState {
    fn new(native: sys::mln_render_session) -> Result<Self> {
        // SAFETY: native came from a successful render-session attach call and is
        // paired with the matching render-session destroy function.
        let handle = unsafe {
            ThreadAffineNativeHandle::from_handle(
                native,
                sys::mln_render_session_destroy,
                "mln_render_session",
            )
        }?;
        Ok(Self {
            handle,
            detached: Cell::new(false),
            frame_acquired: Cell::new(false),
        })
    }

    fn ensure_no_frame_acquired(&self) -> Result<()> {
        if self.frame_acquired.get() {
            Err(frame_acquired_error())
        } else {
            Ok(())
        }
    }

    fn native(&self) -> Result<sys::mln_render_session> {
        self.handle
            .live_handle()
            .ok_or_else(|| closed_handle_error("RenderSessionHandle"))
    }

    fn close(&self) -> Result<()> {
        self.handle.close()
    }
}

/// Outcome of a successful [`RenderSessionHandle::render_update`] call.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub struct RenderUpdate {
    /// Which outcome the call reached; see [`RenderResult`] for the wake each
    /// variant names.
    pub result: RenderResult,
    /// Whether the map asked for another frame while it rendered this one, as
    /// during an ongoing camera transition. Set only when `result` is
    /// [`RenderResult::Rendered`]; false for every other outcome. This is the
    /// same signal the render-frame-finished event carries, delivered here
    /// without the event round trip, so a host can re-arm its frame loop
    /// before it drains events.
    pub needs_repaint: bool,
}

/// Render session handle bound to the thread that attached it.
///
/// The session holds no Rust-level retention of its map. Native keeps the map
/// alive instead: destroying a map fails while a session is attached to it.
pub struct RenderSessionHandle {
    inner: Rc<RenderSessionState>,
}

impl fmt::Debug for RenderSessionHandle {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("RenderSessionHandle")
            .field("closed", &self.inner.handle.is_closed())
            .field("detached", &self.inner.detached.get())
            .finish()
    }
}

/// Render session after backend resources have been detached.
///
/// A detached session holds no reference to its former map, so it stays
/// destroyable after that map closes.
pub struct DetachedRenderSessionHandle {
    inner: Rc<RenderSessionState>,
}

impl fmt::Debug for DetachedRenderSessionHandle {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("DetachedRenderSessionHandle")
            .field("closed", &self.inner.handle.is_closed())
            .finish()
    }
}

impl DetachedRenderSessionHandle {
    /// Explicitly destroys the detached render session.
    pub fn close(self) -> std::result::Result<(), HandleOperationError<Self>> {
        if let Err(error) = self.inner.close() {
            return Err(HandleOperationError::new(error, self));
        }
        Ok(())
    }
}

fn frame_acquired_error() -> crate::Error {
    crate::Error::new(
        crate::ErrorKind::InvalidState,
        None,
        "render session has an acquired texture frame",
    )
}

/// Copied metadata for an acquired Metal session-owned texture frame.
///
/// Backend pointers are exposed by [`MetalOwnedTextureFrameHandle`] so their
/// lifetime stays tied to the open frame handle.
#[derive(Debug, Clone, Copy, PartialEq)]
#[non_exhaustive]
pub struct MetalOwnedTextureFrame {
    pub generation: u64,
    pub width: u32,
    pub height: u32,
    pub scale_factor: f64,
    pub frame_id: u64,
    pub pixel_format: u64,
}

impl MetalOwnedTextureFrame {
    fn from_native(raw: &sys::mln_metal_owned_texture_frame) -> Self {
        Self {
            generation: raw.generation,
            width: raw.width,
            height: raw.height,
            scale_factor: raw.scale_factor,
            frame_id: raw.frame_id,
            pixel_format: raw.pixel_format,
        }
    }
}

/// Copied metadata for an acquired Vulkan session-owned texture frame.
///
/// Backend handles are exposed by [`VulkanOwnedTextureFrameHandle`] so their
/// lifetime stays tied to the open frame handle.
#[derive(Debug, Clone, Copy, PartialEq)]
#[non_exhaustive]
pub struct VulkanOwnedTextureFrame {
    pub generation: u64,
    pub width: u32,
    pub height: u32,
    pub scale_factor: f64,
    pub frame_id: u64,
    pub format: u32,
    pub layout: u32,
}

impl VulkanOwnedTextureFrame {
    fn from_native(raw: &sys::mln_vulkan_owned_texture_frame) -> Self {
        Self {
            generation: raw.generation,
            width: raw.width,
            height: raw.height,
            scale_factor: raw.scale_factor,
            frame_id: raw.frame_id,
            format: raw.format,
            layout: raw.layout,
        }
    }
}

/// Copied metadata for an acquired WebGPU session-owned texture frame.
///
/// Backend pointers are exposed by [`WebGpuOwnedTextureFrameHandle`] so their
/// lifetime stays tied to the open frame handle.
#[derive(Debug, Clone, Copy, PartialEq)]
#[non_exhaustive]
pub struct WebGpuOwnedTextureFrame {
    pub generation: u64,
    pub width: u32,
    pub height: u32,
    pub scale_factor: f64,
    pub frame_id: u64,
    pub format: u32,
}

impl WebGpuOwnedTextureFrame {
    fn from_native(raw: &sys::mln_webgpu_owned_texture_frame) -> Self {
        Self {
            generation: raw.generation,
            width: raw.width,
            height: raw.height,
            scale_factor: raw.scale_factor,
            frame_id: raw.frame_id,
            format: raw.format,
        }
    }
}

/// Copied metadata for an acquired OpenGL session-owned texture frame.
///
/// The texture object name is exposed by [`OpenGLOwnedTextureFrameHandle`] so
/// its lifetime stays tied to the open frame handle.
#[derive(Debug, Clone, Copy, PartialEq)]
#[non_exhaustive]
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

impl OpenGLOwnedTextureFrame {
    fn from_native(raw: &sys::mln_opengl_owned_texture_frame) -> Self {
        Self {
            generation: raw.generation,
            width: raw.width,
            height: raw.height,
            scale_factor: raw.scale_factor,
            frame_id: raw.frame_id,
            target: raw.target,
            internal_format: raw.internal_format,
            format: raw.format,
            type_: raw.type_,
        }
    }
}

/// RAII guard for an acquired Metal session-owned texture frame.
///
/// Releasing the guard ends the borrow of the backend Metal texture and device.
pub struct MetalOwnedTextureFrameHandle {
    session: Rc<RenderSessionState>,
    raw: sys::mln_metal_owned_texture_frame,
    frame: MetalOwnedTextureFrame,
    closed: Cell<bool>,
    _thread_affine: PhantomData<Rc<()>>,
}

impl fmt::Debug for MetalOwnedTextureFrameHandle {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("MetalOwnedTextureFrameHandle")
            .field("closed", &self.closed.get())
            .field("frame", &self.frame)
            .finish()
    }
}

impl MetalOwnedTextureFrameHandle {
    /// Returns copied metadata for this acquired frame.
    pub fn frame(&self) -> Result<&MetalOwnedTextureFrame> {
        if self.closed.get() {
            Err(closed_handle_error("MetalOwnedTextureFrameHandle"))
        } else {
            Ok(&self.frame)
        }
    }
    /// Returns the borrowed Metal texture pointer for backend interop.
    ///
    /// # Safety
    ///
    /// The returned pointer is valid only while this frame handle remains open.
    /// The caller must not store or use it after frame release and must satisfy
    /// Metal synchronization and thread-affinity requirements.
    pub unsafe fn texture(&self) -> Result<FrameNativePointer<'_>> {
        if self.closed.get() {
            Err(closed_handle_error("MetalOwnedTextureFrameHandle"))
        } else {
            // SAFETY: The active native frame owns the validity contract for
            // this borrowed backend handle until release.
            Ok(unsafe { FrameNativePointer::from_ptr(self.raw.texture) })
        }
    }

    /// Returns the borrowed Metal device pointer for backend interop.
    ///
    /// # Safety
    ///
    /// The returned pointer has the same lifetime and synchronization
    /// requirements as [`MetalOwnedTextureFrameHandle::texture`].
    pub unsafe fn device(&self) -> Result<FrameNativePointer<'_>> {
        if self.closed.get() {
            Err(closed_handle_error("MetalOwnedTextureFrameHandle"))
        } else {
            // SAFETY: See texture above.
            Ok(unsafe { FrameNativePointer::from_ptr(self.raw.device) })
        }
    }

    /// Explicitly releases this frame.
    #[allow(clippy::result_large_err)]
    pub fn close(self) -> std::result::Result<(), HandleOperationError<Self>> {
        self.close_with_release(sys::mln_metal_owned_texture_release_frame)
    }

    #[allow(clippy::result_large_err)]
    fn close_with_release(
        self,
        release: unsafe extern "C" fn(
            sys::mln_render_session,
            *const sys::mln_metal_owned_texture_frame,
        ) -> sys::mln_status,
    ) -> std::result::Result<(), HandleOperationError<Self>> {
        if self.closed.get() {
            return Ok(());
        }
        let session = match self.session.native() {
            Ok(session) => session,
            Err(error) => return Err(HandleOperationError::new(error, self)),
        };
        // SAFETY: session is live, and raw is the active frame returned by a
        // successful acquire for this session until release succeeds.
        if let Err(error) = maplibre_core::check(unsafe { release(session, &self.raw) }) {
            return Err(HandleOperationError::new(error, self));
        }
        self.closed.set(true);
        self.session.frame_acquired.set(false);
        Ok(())
    }
}

impl Drop for MetalOwnedTextureFrameHandle {
    fn drop(&mut self) {
        if self.closed.get() {
            return;
        }
        if let Ok(session) = self.session.native() {
            // SAFETY: Best-effort release of the active frame. Drop cannot
            // report errors and never panics.
            let status = unsafe { sys::mln_metal_owned_texture_release_frame(session, &self.raw) };
            if status == sys::MLN_STATUS_OK {
                self.closed.set(true);
                self.session.frame_acquired.set(false);
            }
        }
    }
}

/// RAII guard for an acquired Vulkan session-owned texture frame.
///
/// Releasing the guard ends the borrow of the backend Vulkan image, image view,
/// and device.
pub struct VulkanOwnedTextureFrameHandle {
    session: Rc<RenderSessionState>,
    raw: sys::mln_vulkan_owned_texture_frame,
    frame: VulkanOwnedTextureFrame,
    closed: Cell<bool>,
    _thread_affine: PhantomData<Rc<()>>,
}

impl fmt::Debug for VulkanOwnedTextureFrameHandle {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("VulkanOwnedTextureFrameHandle")
            .field("closed", &self.closed.get())
            .field("frame", &self.frame)
            .finish()
    }
}

impl VulkanOwnedTextureFrameHandle {
    /// Returns copied metadata for this acquired frame.
    pub fn frame(&self) -> Result<&VulkanOwnedTextureFrame> {
        if self.closed.get() {
            Err(closed_handle_error("VulkanOwnedTextureFrameHandle"))
        } else {
            Ok(&self.frame)
        }
    }
    /// Returns the borrowed Vulkan image handle for backend interop.
    ///
    /// # Safety
    ///
    /// The returned handle is valid only while this frame handle remains open.
    /// The caller must not store or use it after frame release and must satisfy
    /// Vulkan synchronization and thread-affinity requirements.
    pub unsafe fn image(&self) -> Result<FrameVulkanHandle<'_>> {
        if self.closed.get() {
            Err(closed_handle_error("VulkanOwnedTextureFrameHandle"))
        } else {
            Ok(FrameVulkanHandle::new(self.raw.image))
        }
    }

    /// Returns the borrowed Vulkan image view handle for backend interop.
    ///
    /// # Safety
    ///
    /// The returned handle has the same lifetime and synchronization
    /// requirements as [`VulkanOwnedTextureFrameHandle::image`].
    pub unsafe fn image_view(&self) -> Result<FrameVulkanHandle<'_>> {
        if self.closed.get() {
            Err(closed_handle_error("VulkanOwnedTextureFrameHandle"))
        } else {
            Ok(FrameVulkanHandle::new(self.raw.image_view))
        }
    }

    /// Returns the borrowed Vulkan device pointer for backend interop.
    ///
    /// # Safety
    ///
    /// The returned pointer has the same lifetime and synchronization
    /// requirements as [`VulkanOwnedTextureFrameHandle::image`].
    pub unsafe fn device(&self) -> Result<FrameNativePointer<'_>> {
        if self.closed.get() {
            Err(closed_handle_error("VulkanOwnedTextureFrameHandle"))
        } else {
            // SAFETY: See image above.
            Ok(unsafe { FrameNativePointer::from_ptr(self.raw.device) })
        }
    }

    /// Explicitly releases this frame.
    #[allow(clippy::result_large_err)]
    pub fn close(self) -> std::result::Result<(), HandleOperationError<Self>> {
        if self.closed.get() {
            return Ok(());
        }
        let session = match self.session.native() {
            Ok(session) => session,
            Err(error) => return Err(HandleOperationError::new(error, self)),
        };
        // SAFETY: session is live, and raw is the active frame returned by a
        // successful acquire for this session until release succeeds.
        if let Err(error) = maplibre_core::check(unsafe {
            sys::mln_vulkan_owned_texture_release_frame(session, &self.raw)
        }) {
            return Err(HandleOperationError::new(error, self));
        }
        self.closed.set(true);
        self.session.frame_acquired.set(false);
        Ok(())
    }
}

impl Drop for VulkanOwnedTextureFrameHandle {
    fn drop(&mut self) {
        if self.closed.get() {
            return;
        }
        if let Ok(session) = self.session.native() {
            // SAFETY: Best-effort release of the active frame. Drop cannot
            // report errors and never panics.
            let status = unsafe { sys::mln_vulkan_owned_texture_release_frame(session, &self.raw) };
            if status == sys::MLN_STATUS_OK {
                self.closed.set(true);
                self.session.frame_acquired.set(false);
            }
        }
    }
}

/// RAII guard for an acquired WebGPU session-owned texture frame.
///
/// Releasing the guard ends the borrow of the backend WebGPU texture, texture
/// view, and device.
pub struct WebGpuOwnedTextureFrameHandle {
    session: Rc<RenderSessionState>,
    raw: sys::mln_webgpu_owned_texture_frame,
    frame: WebGpuOwnedTextureFrame,
    closed: Cell<bool>,
    _thread_affine: PhantomData<Rc<()>>,
}

impl fmt::Debug for WebGpuOwnedTextureFrameHandle {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("WebGpuOwnedTextureFrameHandle")
            .field("closed", &self.closed.get())
            .field("frame", &self.frame)
            .finish()
    }
}

impl WebGpuOwnedTextureFrameHandle {
    /// Returns copied metadata for this acquired frame.
    pub fn frame(&self) -> Result<&WebGpuOwnedTextureFrame> {
        if self.closed.get() {
            Err(closed_handle_error("WebGpuOwnedTextureFrameHandle"))
        } else {
            Ok(&self.frame)
        }
    }

    /// Returns the borrowed WebGPU texture pointer for backend interop.
    ///
    /// # Safety
    ///
    /// The returned pointer is valid only while this frame handle remains open.
    /// The caller must not store or use it after frame release and must satisfy
    /// WebGPU synchronization and thread-affinity requirements.
    pub unsafe fn texture(&self) -> Result<FrameNativePointer<'_>> {
        if self.closed.get() {
            Err(closed_handle_error("WebGpuOwnedTextureFrameHandle"))
        } else {
            // SAFETY: The active native frame owns the validity contract for
            // this borrowed backend handle until release.
            Ok(unsafe { FrameNativePointer::from_ptr(self.raw.texture) })
        }
    }

    /// Returns the borrowed WebGPU texture view pointer for backend interop.
    ///
    /// # Safety
    ///
    /// The returned pointer has the same lifetime and synchronization
    /// requirements as [`WebGpuOwnedTextureFrameHandle::texture`].
    pub unsafe fn texture_view(&self) -> Result<FrameNativePointer<'_>> {
        if self.closed.get() {
            Err(closed_handle_error("WebGpuOwnedTextureFrameHandle"))
        } else {
            // SAFETY: See texture above.
            Ok(unsafe { FrameNativePointer::from_ptr(self.raw.texture_view) })
        }
    }

    /// Returns the borrowed WebGPU device pointer for backend interop.
    ///
    /// # Safety
    ///
    /// The returned pointer has the same lifetime and synchronization
    /// requirements as [`WebGpuOwnedTextureFrameHandle::texture`].
    pub unsafe fn device(&self) -> Result<FrameNativePointer<'_>> {
        if self.closed.get() {
            Err(closed_handle_error("WebGpuOwnedTextureFrameHandle"))
        } else {
            // SAFETY: See texture above.
            Ok(unsafe { FrameNativePointer::from_ptr(self.raw.device) })
        }
    }

    /// Explicitly releases this frame.
    #[allow(clippy::result_large_err)]
    pub fn close(self) -> std::result::Result<(), HandleOperationError<Self>> {
        if self.closed.get() {
            return Ok(());
        }
        let session = match self.session.native() {
            Ok(session) => session,
            Err(error) => return Err(HandleOperationError::new(error, self)),
        };
        // SAFETY: session is live, and raw is the active frame returned by a
        // successful acquire for this session until release succeeds.
        if let Err(error) = maplibre_core::check(unsafe {
            sys::mln_webgpu_owned_texture_release_frame(session, &self.raw)
        }) {
            return Err(HandleOperationError::new(error, self));
        }
        self.closed.set(true);
        self.session.frame_acquired.set(false);
        Ok(())
    }
}

impl Drop for WebGpuOwnedTextureFrameHandle {
    fn drop(&mut self) {
        if self.closed.get() {
            return;
        }
        if let Ok(session) = self.session.native() {
            // SAFETY: Best-effort release of the active frame. Drop cannot
            // report errors and never panics.
            let status = unsafe { sys::mln_webgpu_owned_texture_release_frame(session, &self.raw) };
            if status == sys::MLN_STATUS_OK {
                self.closed.set(true);
                self.session.frame_acquired.set(false);
            }
        }
    }
}

/// RAII guard for an acquired OpenGL session-owned texture frame.
///
/// Releasing the guard ends the borrow of the backend OpenGL texture object.
pub struct OpenGLOwnedTextureFrameHandle {
    session: Rc<RenderSessionState>,
    raw: sys::mln_opengl_owned_texture_frame,
    frame: OpenGLOwnedTextureFrame,
    closed: Cell<bool>,
    _thread_affine: PhantomData<Rc<()>>,
}

impl fmt::Debug for OpenGLOwnedTextureFrameHandle {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("OpenGLOwnedTextureFrameHandle")
            .field("closed", &self.closed.get())
            .field("frame", &self.frame)
            .finish()
    }
}

impl OpenGLOwnedTextureFrameHandle {
    /// Returns copied metadata for this acquired frame.
    pub fn frame(&self) -> Result<&OpenGLOwnedTextureFrame> {
        if self.closed.get() {
            Err(closed_handle_error("OpenGLOwnedTextureFrameHandle"))
        } else {
            Ok(&self.frame)
        }
    }
    /// Returns the borrowed OpenGL texture object name for backend interop.
    pub fn texture(&self) -> Result<FrameOpenGLTextureName<'_>> {
        if self.closed.get() {
            Err(closed_handle_error("OpenGLOwnedTextureFrameHandle"))
        } else {
            Ok(FrameOpenGLTextureName::new(self.raw.texture))
        }
    }

    /// Explicitly releases this frame.
    #[allow(clippy::result_large_err)]
    pub fn close(self) -> std::result::Result<(), HandleOperationError<Self>> {
        if self.closed.get() {
            return Ok(());
        }
        let session = match self.session.native() {
            Ok(session) => session,
            Err(error) => return Err(HandleOperationError::new(error, self)),
        };
        // SAFETY: session is live, and raw is the active frame returned by a
        // successful acquire for this session until release succeeds.
        if let Err(error) = maplibre_core::check(unsafe {
            sys::mln_opengl_owned_texture_release_frame(session, &self.raw)
        }) {
            return Err(HandleOperationError::new(error, self));
        }
        self.closed.set(true);
        self.session.frame_acquired.set(false);
        Ok(())
    }
}

impl Drop for OpenGLOwnedTextureFrameHandle {
    fn drop(&mut self) {
        if self.closed.get() {
            return;
        }
        if let Ok(session) = self.session.native() {
            // SAFETY: Best-effort release of the active frame. Drop cannot
            // report errors and never panics.
            let status = unsafe { sys::mln_opengl_owned_texture_release_frame(session, &self.raw) };
            if status == sys::MLN_STATUS_OK {
                self.closed.set(true);
                self.session.frame_acquired.set(false);
            }
        }
    }
}

impl RenderSessionHandle {
    pub(crate) fn attach<F>(map: &MapAttachRef, attach: F) -> Result<Self>
    where
        F: FnOnce(sys::mln_map, *mut sys::mln_render_session) -> sys::mln_status,
    {
        let mut out = maplibre_core::ptr::OutHandle::<sys::mln_render_session>::new();
        // A close racing this attach makes the handle stale, which the C API
        // rejects.
        let status = attach(map.map(), out.as_mut_ptr());
        maplibre_core::check(status)?;
        let ptr = out_handle(out, "mln_render_session")?;
        Ok(Self {
            inner: Rc::new(RenderSessionState::new(ptr)?),
        })
    }

    /// Explicitly destroys the render session.
    ///
    /// Native destruction errors are returned. When destruction fails, the
    /// underlying native handle remains live so a later `close` can retry.
    pub fn close(self) -> std::result::Result<(), HandleOperationError<Self>> {
        if let Err(error) = self.inner.ensure_no_frame_acquired() {
            return Err(HandleOperationError::new(error, self));
        }
        if let Err(error) = self.inner.close() {
            return Err(HandleOperationError::new(error, self));
        }
        Ok(())
    }
    /// Resizes this attached render session.
    ///
    /// Surface and owned-texture sessions resize in place, keeping the
    /// renderer along with the tile pyramid, glyph and image atlases, and
    /// symbol placement. A scale factor change retires the renderer instead,
    /// because shaders are compiled for one pixel ratio. Map-owned feature
    /// state survives either way. Borrowed texture targets report an
    /// unsupported-feature error; hand over a new texture with the backend's
    /// `set_*_borrowed_texture_target` method instead.
    pub fn resize(&self, width: u32, height: u32, scale_factor: f64) -> Result<()> {
        self.inner.ensure_no_frame_acquired()?;
        let session = self.inner.native()?;
        // SAFETY: session is a live render session handle owned by this wrapper.
        maplibre_core::check(unsafe {
            sys::mln_render_session_resize(session, width, height, scale_factor)
        })
    }

    /// Presents this attached surface session through a new surface, keeping
    /// this session's renderer and its state.
    ///
    /// The descriptor's extent applies as a resize does. A `context.device`
    /// that is neither null nor this session's device reports an
    /// invalid-argument error and leaves the current surface in place.
    pub fn set_metal_surface_target(&self, descriptor: &MetalSurfaceDescriptor) -> Result<()> {
        self.set_target(descriptor.to_native(), |session, raw| {
            // SAFETY: session is a live render session handle owned by this
            // wrapper, and raw is a materialized descriptor valid for this call.
            unsafe { sys::mln_metal_surface_set_target(session, raw) }
        })
    }

    /// Presents this attached surface session through a new surface.
    ///
    /// See [`RenderSessionHandle::set_metal_surface_target`] for what replacing
    /// a surface preserves. The outgoing `VkSurfaceKHR` must still be valid:
    /// this session holds a swapchain built from it, and Vulkan destroys every
    /// swapchain before its surface.
    pub fn set_vulkan_surface_target(&self, descriptor: &VulkanSurfaceDescriptor) -> Result<()> {
        self.set_target(descriptor.to_native(), |session, raw| {
            // SAFETY: session is a live render session handle owned by this
            // wrapper, and raw is a materialized descriptor valid for this call.
            unsafe { sys::mln_vulkan_surface_set_target(session, raw) }
        })
    }

    /// Presents this attached surface session through a new WebGPU surface.
    ///
    /// See [`RenderSessionHandle::set_metal_surface_target`] for what replacing
    /// a surface preserves. The replacement names the same device and format as
    /// the session attached with.
    pub fn set_webgpu_surface_target(&self, descriptor: &WebGpuSurfaceDescriptor) -> Result<()> {
        self.set_target(descriptor.to_native(), |session, raw| {
            // SAFETY: session is a live render session handle owned by this
            // wrapper, and raw is a materialized descriptor valid for this call.
            unsafe { sys::mln_webgpu_surface_set_target(session, raw) }
        })
    }

    /// Presents this attached surface session through a new surface.
    ///
    /// See [`RenderSessionHandle::set_metal_surface_target`] for what replacing
    /// a surface preserves. The new surface is made current on the next render,
    /// so a host may hand over a replacement for one it has already destroyed.
    /// A surface accepted here can still prove unusable, which the next
    /// `render_update` reports rather than this call.
    pub fn set_opengl_surface_target(&self, descriptor: &OpenGLSurfaceDescriptor) -> Result<()> {
        self.set_target(descriptor.to_native(), |session, raw| {
            // SAFETY: session is a live render session handle owned by this
            // wrapper, and raw is a materialized descriptor valid for this call.
            unsafe { sys::mln_opengl_surface_set_target(session, raw) }
        })
    }

    /// Renders this attached texture session into a new caller-owned texture,
    /// keeping this session's renderer. A scale factor change starts a new
    /// renderer, as [`RenderSessionHandle::resize`] does.
    ///
    /// The replacement must belong to the device this session attached with and
    /// carry the pixel format it attached with; otherwise this reports an error
    /// and leaves the current texture in place. The caller owns the replacement
    /// and keeps it valid until the next replacement, detach, or close. The
    /// outgoing texture is neither retained nor read here.
    pub fn set_metal_borrowed_texture_target(
        &self,
        descriptor: &MetalBorrowedTextureDescriptor,
    ) -> Result<()> {
        self.set_target(descriptor.to_native(), |session, raw| {
            // SAFETY: session is a live render session handle owned by this
            // wrapper, and raw is a materialized descriptor valid for this call.
            unsafe { sys::mln_metal_borrowed_texture_set_target(session, raw) }
        })
    }

    /// Renders this attached texture session into a new caller-owned image.
    ///
    /// See [`RenderSessionHandle::set_metal_borrowed_texture_target`] for what
    /// replacing a target preserves. The replacement carries the format and
    /// both layouts this session attached with, since its render pass was built
    /// around them.
    pub fn set_vulkan_borrowed_texture_target(
        &self,
        descriptor: &VulkanBorrowedTextureDescriptor,
    ) -> Result<()> {
        self.set_target(descriptor.to_native(), |session, raw| {
            // SAFETY: session is a live render session handle owned by this
            // wrapper, and raw is a materialized descriptor valid for this call.
            unsafe { sys::mln_vulkan_borrowed_texture_set_target(session, raw) }
        })
    }

    /// Renders this attached texture session into a new caller-owned texture.
    ///
    /// See [`RenderSessionHandle::set_metal_borrowed_texture_target`] for what
    /// replacing a target preserves. The replacement carries the format this
    /// session attached with, and belongs to the device it attached with.
    pub fn set_webgpu_borrowed_texture_target(
        &self,
        descriptor: &WebGpuBorrowedTextureDescriptor,
    ) -> Result<()> {
        self.set_target(descriptor.to_native(), |session, raw| {
            // SAFETY: session is a live render session handle owned by this
            // wrapper, and raw is a materialized descriptor valid for this call.
            unsafe { sys::mln_webgpu_borrowed_texture_set_target(session, raw) }
        })
    }

    /// Renders this attached texture session into a new caller-owned texture.
    ///
    /// See [`RenderSessionHandle::set_metal_borrowed_texture_target`] for what
    /// replacing a target preserves. The replacement belongs to the context
    /// this session attached with, or one in its share group, and the host
    /// context must be current on this thread.
    pub fn set_opengl_borrowed_texture_target(
        &self,
        descriptor: &OpenGLBorrowedTextureDescriptor,
    ) -> Result<()> {
        self.set_target(descriptor.to_native(), |session, raw| {
            // SAFETY: session is a live render session handle owned by this
            // wrapper, and raw is a materialized descriptor valid for this call.
            unsafe { sys::mln_opengl_borrowed_texture_set_target(session, raw) }
        })
    }

    /// Shared body for the `set_*_target` methods. The caller-materialized
    /// descriptor lives for the whole call, which is all the C API borrows it
    /// for.
    fn set_target<D>(
        &self,
        raw: D,
        set_target: impl FnOnce(sys::mln_render_session, &D) -> sys::mln_status,
    ) -> Result<()> {
        self.inner.ensure_no_frame_acquired()?;
        let session = self.inner.native()?;
        maplibre_core::check(set_target(session, &raw))
    }

    /// Processes the latest map render update for this render target.
    ///
    /// The map retains its latest update, so repeated calls re-render it and
    /// report [`RenderResult::Rendered`] again. Every other result names the
    /// wake to wait for: [`RenderResult::NoUpdate`] and
    /// [`RenderResult::SizePending`] resolve on a render-update-available
    /// event, and [`RenderResult::TargetNotReady`] resolves when the host
    /// changes the render target.
    ///
    /// The returned [`RenderUpdate::needs_repaint`] reports whether the map
    /// asked for another frame while rendering this one, so a frame loop can
    /// re-arm without draining events first.
    pub fn render_update(&self) -> Result<RenderUpdate> {
        self.inner.ensure_no_frame_acquired()?;
        let session = self.inner.native()?;
        let mut result = sys::MLN_RENDER_RESULT_NO_UPDATE;
        let mut needs_repaint = false;
        // SAFETY: session is a live render session handle owned by this wrapper,
        // and result and needs_repaint point to caller-owned output storage.
        maplibre_core::check(unsafe {
            sys::mln_render_session_render_update(session, &raw mut result, &raw mut needs_repaint)
        })?;
        Ok(RenderUpdate {
            result: RenderResult::from_raw(result),
            needs_repaint,
        })
    }

    /// Detaches backend-bound render resources from the map, consuming this
    /// handle and returning a close-only handle.
    ///
    /// A detached session no longer reaches its map, so the map is free to
    /// close and the detached session stays destroyable afterwards.
    pub fn detach(
        self,
    ) -> std::result::Result<DetachedRenderSessionHandle, HandleOperationError<Self>> {
        if let Err(error) = self.inner.ensure_no_frame_acquired() {
            return Err(HandleOperationError::new(error, self));
        }
        let session = match self.inner.native() {
            Ok(session) => session,
            Err(error) => return Err(HandleOperationError::new(error, self)),
        };
        // SAFETY: session is a live render session handle owned by this wrapper.
        if let Err(error) = maplibre_core::check(unsafe { sys::mln_render_session_detach(session) })
        {
            return Err(HandleOperationError::new(error, self));
        }
        self.inner.detached.set(true);
        Ok(DetachedRenderSessionHandle {
            inner: Rc::clone(&self.inner),
        })
    }

    /// Asks the session renderer to release cached resources where possible.
    pub fn reduce_memory_use(&self) -> Result<()> {
        self.inner.ensure_no_frame_acquired()?;
        let session = self.inner.native()?;
        // SAFETY: session is a live render session handle owned by this wrapper.
        maplibre_core::check(unsafe { sys::mln_render_session_reduce_memory_use(session) })
    }

    /// Clears renderer data for the session.
    pub fn clear_data(&self) -> Result<()> {
        self.inner.ensure_no_frame_acquired()?;
        let session = self.inner.native()?;
        // SAFETY: session is a live render session handle owned by this wrapper.
        maplibre_core::check(unsafe { sys::mln_render_session_clear_data(session) })
    }

    /// Dumps renderer debug logs through MapLibre Native logging.
    pub fn dump_debug_logs(&self) -> Result<()> {
        self.inner.ensure_no_frame_acquired()?;
        let session = self.inner.native()?;
        // SAFETY: session is a live render session handle owned by this wrapper.
        maplibre_core::check(unsafe { sys::mln_render_session_dump_debug_logs(session) })
    }

    /// Returns CPU readback metadata for the most recently rendered texture frame.
    pub fn texture_image_info(&self) -> Result<TextureImageInfo> {
        self.inner.ensure_no_frame_acquired()?;
        let session = self.inner.native()?;
        // SAFETY: Default constructor takes no arguments and initializes size.
        let mut info = unsafe { sys::mln_texture_image_info_default() };
        // SAFETY: session is live. Passing a null buffer with zero capacity is
        // the documented metadata probe path; out_info points to initialized storage.
        maplibre_core::check(unsafe {
            sys::mln_texture_read_premultiplied_rgba8(session, std::ptr::null_mut(), 0, &mut info)
        })?;
        Ok(maplibre_core::values::texture_image_info_from_native(&info))
    }

    /// Reads the most recently rendered texture frame as premultiplied RGBA8.
    pub fn read_premultiplied_rgba8_into(&self, data: &mut [u8]) -> Result<TextureImageInfo> {
        self.inner.ensure_no_frame_acquired()?;
        let session = self.inner.native()?;
        // SAFETY: Default constructor takes no arguments and initializes size.
        let mut info = unsafe { sys::mln_texture_image_info_default() };
        let data_ptr = if data.is_empty() {
            std::ptr::null_mut()
        } else {
            data.as_mut_ptr()
        };
        // SAFETY: session is live, data_ptr either points to data's mutable
        // storage for data.len() bytes or is null for an empty buffer, and info
        // points to initialized writable storage.
        maplibre_core::check(unsafe {
            sys::mln_texture_read_premultiplied_rgba8(session, data_ptr, data.len(), &mut info)
        })?;
        // An empty destination reaches native code as the size probe, which
        // succeeds without copying, so report it as too small here.
        if data.is_empty() && info.byte_length > 0 {
            return Err(crate::Error::invalid_argument(format!(
                "buffer length 0 is smaller than the required {} bytes",
                info.byte_length
            )));
        }
        Ok(maplibre_core::values::texture_image_info_from_native(&info))
    }

    /// Acquires a borrowed Metal frame from a session-owned texture target.
    pub fn acquire_metal_owned_texture_frame(&self) -> Result<MetalOwnedTextureFrameHandle> {
        self.inner.ensure_no_frame_acquired()?;
        let session = self.inner.native()?;
        let mut raw = empty_metal_owned_texture_frame();
        // SAFETY: session is live and raw points to initialized writable frame storage.
        maplibre_core::check(unsafe {
            sys::mln_metal_owned_texture_acquire_frame(session, &mut raw)
        })?;
        self.inner.frame_acquired.set(true);
        Ok(MetalOwnedTextureFrameHandle {
            session: Rc::clone(&self.inner),
            frame: MetalOwnedTextureFrame::from_native(&raw),
            raw,
            closed: Cell::new(false),
            _thread_affine: PhantomData,
        })
    }

    /// Acquires a borrowed Vulkan frame from a session-owned texture target.
    pub fn acquire_vulkan_owned_texture_frame(&self) -> Result<VulkanOwnedTextureFrameHandle> {
        self.inner.ensure_no_frame_acquired()?;
        let session = self.inner.native()?;
        let mut raw = empty_vulkan_owned_texture_frame();
        // SAFETY: session is live and raw points to initialized writable frame storage.
        maplibre_core::check(unsafe {
            sys::mln_vulkan_owned_texture_acquire_frame(session, &mut raw)
        })?;
        self.inner.frame_acquired.set(true);
        Ok(VulkanOwnedTextureFrameHandle {
            session: Rc::clone(&self.inner),
            frame: VulkanOwnedTextureFrame::from_native(&raw),
            raw,
            closed: Cell::new(false),
            _thread_affine: PhantomData,
        })
    }

    /// Acquires a borrowed WebGPU frame from a session-owned texture target.
    pub fn acquire_webgpu_owned_texture_frame(&self) -> Result<WebGpuOwnedTextureFrameHandle> {
        self.inner.ensure_no_frame_acquired()?;
        let session = self.inner.native()?;
        let mut raw = empty_webgpu_owned_texture_frame();
        // SAFETY: session is live and raw points to initialized writable frame storage.
        maplibre_core::check(unsafe {
            sys::mln_webgpu_owned_texture_acquire_frame(session, &mut raw)
        })?;
        self.inner.frame_acquired.set(true);
        Ok(WebGpuOwnedTextureFrameHandle {
            session: Rc::clone(&self.inner),
            frame: WebGpuOwnedTextureFrame::from_native(&raw),
            raw,
            closed: Cell::new(false),
            _thread_affine: PhantomData,
        })
    }

    /// Acquires a borrowed OpenGL frame from a session-owned texture target.
    pub fn acquire_opengl_owned_texture_frame(&self) -> Result<OpenGLOwnedTextureFrameHandle> {
        self.inner.ensure_no_frame_acquired()?;
        let session = self.inner.native()?;
        let mut raw = empty_opengl_owned_texture_frame();
        // SAFETY: session is live and raw points to initialized writable frame storage.
        maplibre_core::check(unsafe {
            sys::mln_opengl_owned_texture_acquire_frame(session, &mut raw)
        })?;
        self.inner.frame_acquired.set(true);
        Ok(OpenGLOwnedTextureFrameHandle {
            session: Rc::clone(&self.inner),
            frame: OpenGLOwnedTextureFrame::from_native(&raw),
            raw,
            closed: Cell::new(false),
            _thread_affine: PhantomData,
        })
    }
}

fn empty_metal_owned_texture_frame() -> sys::mln_metal_owned_texture_frame {
    sys::mln_metal_owned_texture_frame {
        size: mem::size_of::<sys::mln_metal_owned_texture_frame>() as u32,
        generation: 0,
        width: 0,
        height: 0,
        scale_factor: 0.0,
        frame_id: 0,
        texture: std::ptr::null_mut(),
        device: std::ptr::null_mut(),
        pixel_format: 0,
    }
}

fn empty_webgpu_owned_texture_frame() -> sys::mln_webgpu_owned_texture_frame {
    sys::mln_webgpu_owned_texture_frame {
        size: mem::size_of::<sys::mln_webgpu_owned_texture_frame>() as u32,
        generation: 0,
        width: 0,
        height: 0,
        scale_factor: 0.0,
        frame_id: 0,
        texture: std::ptr::null_mut(),
        texture_view: std::ptr::null_mut(),
        device: std::ptr::null_mut(),
        format: 0,
    }
}

fn empty_vulkan_owned_texture_frame() -> sys::mln_vulkan_owned_texture_frame {
    sys::mln_vulkan_owned_texture_frame {
        size: mem::size_of::<sys::mln_vulkan_owned_texture_frame>() as u32,
        generation: 0,
        width: 0,
        height: 0,
        scale_factor: 0.0,
        frame_id: 0,
        image: 0,
        image_view: 0,
        device: std::ptr::null_mut(),
        format: 0,
        layout: 0,
    }
}

fn empty_opengl_owned_texture_frame() -> sys::mln_opengl_owned_texture_frame {
    sys::mln_opengl_owned_texture_frame {
        size: mem::size_of::<sys::mln_opengl_owned_texture_frame>() as u32,
        generation: 0,
        width: 0,
        height: 0,
        scale_factor: 0.0,
        frame_id: 0,
        texture: 0,
        target: 0,
        internal_format: 0,
        format: 0,
        type_: 0,
    }
}

#[cfg(test)]
mod tests;
