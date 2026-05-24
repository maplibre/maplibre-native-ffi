use std::cell::{Cell, RefCell};
use std::fmt;
use std::marker::PhantomData;
use std::mem;
use std::ptr::NonNull;
use std::rc::Rc;

pub use maplibre_core::{PremultipliedRgba8Image, TextureImageInfo};
use maplibre_native_core as maplibre_core;
use maplibre_native_sys as sys;

use crate::handle::{ThreadAffineNativeHandle, closed_handle_error, out_handle};
use crate::map::{MapHandle, MapState};
#[cfg(test)]
use crate::{Feature, JsonValue};
use crate::{HandleOperationError, Result};

/// Borrowed opaque native address used for backend interop handles.
///
/// The value does not own, retain, dereference, or validate the pointed-to
/// object. Passing it to MapLibre Native transfers no ownership and grants the
/// Rust binding no memory access.
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
    /// The caller must ensure the address has the correct backend-native type
    /// for every API it is passed to, and that the native object stays valid for
    /// the complete borrow required by that API. This wrapper does not validate
    /// provenance, alignment, lifetime, thread ownership, or backend type.
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

/// Borrowed opaque native address whose validity is tied to an active texture frame.
///
/// The value does not own, retain, dereference, or validate the pointed-to
/// object. It exists so backend pointers returned from acquired frame handles
/// carry the frame borrow in their Rust type instead of escaping as plain
/// [`NativePointer`] values.
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

mod query;
pub use query::{
    FeatureExtensionResult, FeatureStateSelector, QueriedFeature, RenderedFeatureQueryOptions,
    RenderedQueryGeometry, SourceFeatureQueryOptions,
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

    pub fn with_proc_addresses(
        mut self,
        get_instance_proc_addr: NativePointer,
        get_device_proc_addr: NativePointer,
    ) -> Self {
        self.get_instance_proc_addr = get_instance_proc_addr;
        self.get_device_proc_addr = get_device_proc_addr;
        self
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
    pub texture: NativePointer,
}

impl MetalBorrowedTextureDescriptor {
    pub fn new(extent: RenderTargetExtent, texture: NativePointer) -> Self {
        Self { extent, texture }
    }

    pub(crate) fn to_native(&self) -> sys::mln_metal_borrowed_texture_descriptor {
        maplibre_core::render::metal_borrowed_texture_descriptor_to_native(
            maplibre_core::render::MetalBorrowedTextureDescriptorFields {
                extent: self.extent.to_core(),
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
        context: VulkanContextDescriptor,
        image: NativePointer,
        image_view: NativePointer,
        format: u32,
        initial_layout: u32,
        final_layout: u32,
    ) -> Self {
        Self {
            extent,
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

#[derive(Debug)]
struct RenderSessionState {
    handle: ThreadAffineNativeHandle<sys::mln_render_session>,
    map: RefCell<Option<Rc<MapState>>>,
    detached: Cell<bool>,
    frame_acquired: Cell<bool>,
}

impl RenderSessionState {
    fn new(ptr: NonNull<sys::mln_render_session>, map: Rc<MapState>) -> Self {
        // SAFETY: ptr came from a successful render-session attach call and is
        // paired with the matching render-session destroy function.
        let handle = unsafe {
            ThreadAffineNativeHandle::from_raw(
                ptr,
                sys::mln_render_session_destroy,
                "mln_render_session",
            )
        };
        Self {
            handle,
            map: RefCell::new(Some(map)),
            detached: Cell::new(false),
            frame_acquired: Cell::new(false),
        }
    }

    fn ensure_no_frame_acquired(&self) -> Result<()> {
        if self.frame_acquired.get() {
            Err(frame_acquired_error())
        } else {
            Ok(())
        }
    }

    fn as_ptr(&self) -> Result<*mut sys::mln_render_session> {
        let ptr = self.handle.as_ptr();
        if ptr.is_null() {
            Err(closed_handle_error("RenderSessionHandle"))
        } else {
            Ok(ptr)
        }
    }

    fn close(&self) -> Result<()> {
        self.handle.close()?;
        self.map.borrow_mut().take();
        Ok(())
    }
}

/// Owner-thread render session handle bound to a retained map.
pub struct RenderSessionHandle {
    inner: Rc<RenderSessionState>,
}

impl fmt::Debug for RenderSessionHandle {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("RenderSessionHandle")
            .field("closed", &self.is_closed())
            .field("detached", &self.inner.detached.get())
            .finish()
    }
}

/// Render session after backend resources have been detached.
pub struct DetachedRenderSessionHandle {
    inner: Rc<RenderSessionState>,
}

impl fmt::Debug for DetachedRenderSessionHandle {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("DetachedRenderSessionHandle")
            .field("closed", &self.is_closed())
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

    pub fn is_closed(&self) -> bool {
        self.inner.handle.is_closed()
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
/// Backend pointers are exposed by [`VulkanOwnedTextureFrameHandle`] so their
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
            .field("closed", &self.is_closed())
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

    pub fn is_closed(&self) -> bool {
        self.closed.get()
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
        if self.closed.get() {
            return Ok(());
        }
        let session = match self.session.as_ptr() {
            Ok(session) => session,
            Err(error) => return Err(HandleOperationError::new(error, self)),
        };
        // SAFETY: session is live, and raw is the active frame returned by a
        // successful acquire for this session until release succeeds.
        if let Err(error) = maplibre_core::check(unsafe {
            sys::mln_metal_owned_texture_release_frame(session, &self.raw)
        }) {
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
        if let Ok(session) = self.session.as_ptr() {
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
            .field("closed", &self.is_closed())
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

    pub fn is_closed(&self) -> bool {
        self.closed.get()
    }

    /// Returns the borrowed Vulkan image pointer for backend interop.
    ///
    /// # Safety
    ///
    /// The returned pointer is valid only while this frame handle remains open.
    /// The caller must not store or use it after frame release and must satisfy
    /// Vulkan synchronization and thread-affinity requirements.
    pub unsafe fn image(&self) -> Result<FrameNativePointer<'_>> {
        if self.closed.get() {
            Err(closed_handle_error("VulkanOwnedTextureFrameHandle"))
        } else {
            // SAFETY: The active native frame owns the validity contract for
            // this borrowed backend handle until release.
            Ok(unsafe { FrameNativePointer::from_ptr(self.raw.image) })
        }
    }

    /// Returns the borrowed Vulkan image view pointer for backend interop.
    ///
    /// # Safety
    ///
    /// The returned pointer has the same lifetime and synchronization
    /// requirements as [`VulkanOwnedTextureFrameHandle::image`].
    pub unsafe fn image_view(&self) -> Result<FrameNativePointer<'_>> {
        if self.closed.get() {
            Err(closed_handle_error("VulkanOwnedTextureFrameHandle"))
        } else {
            // SAFETY: See image above.
            Ok(unsafe { FrameNativePointer::from_ptr(self.raw.image_view) })
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
        let session = match self.session.as_ptr() {
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
        if let Ok(session) = self.session.as_ptr() {
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

impl RenderSessionHandle {
    pub(crate) fn attach<F>(map: &MapHandle, attach: F) -> Result<Self>
    where
        F: FnOnce(*mut sys::mln_map, *mut *mut sys::mln_render_session) -> sys::mln_status,
    {
        let map_ptr = map.inner.as_ptr()?;
        let mut out = maplibre_core::ptr::OutPtr::<sys::mln_render_session>::new();
        let status = attach(map_ptr, out.as_mut_ptr());
        maplibre_core::check(status)?;
        let ptr = out_handle(out, "mln_render_session")?;
        Ok(Self {
            inner: Rc::new(RenderSessionState::new(ptr, Rc::clone(&map.inner))),
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

    pub fn is_closed(&self) -> bool {
        self.inner.handle.is_closed()
    }

    /// Resizes this attached render session.
    pub fn resize(&self, width: u32, height: u32, scale_factor: f64) -> Result<()> {
        self.inner.ensure_no_frame_acquired()?;
        let session = self.inner.as_ptr()?;
        // SAFETY: session is a live render session handle owned by this wrapper.
        maplibre_core::check(unsafe {
            sys::mln_render_session_resize(session, width, height, scale_factor)
        })
    }

    /// Processes the latest map render update for this render target.
    pub fn render_update(&self) -> Result<()> {
        self.inner.ensure_no_frame_acquired()?;
        let session = self.inner.as_ptr()?;
        // SAFETY: session is a live render session handle owned by this wrapper.
        maplibre_core::check(unsafe { sys::mln_render_session_render_update(session) })
    }

    /// Detaches backend-bound render resources from the map.
    ///
    /// The native session remains live only for destruction, so successful
    /// detach consumes this handle and returns a detached close-only handle.
    pub fn detach(
        self,
    ) -> std::result::Result<DetachedRenderSessionHandle, HandleOperationError<Self>> {
        if let Err(error) = self.inner.ensure_no_frame_acquired() {
            return Err(HandleOperationError::new(error, self));
        }
        let session = match self.inner.as_ptr() {
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
        let session = self.inner.as_ptr()?;
        // SAFETY: session is a live render session handle owned by this wrapper.
        maplibre_core::check(unsafe { sys::mln_render_session_reduce_memory_use(session) })
    }

    /// Clears renderer data for the session.
    pub fn clear_data(&self) -> Result<()> {
        self.inner.ensure_no_frame_acquired()?;
        let session = self.inner.as_ptr()?;
        // SAFETY: session is a live render session handle owned by this wrapper.
        maplibre_core::check(unsafe { sys::mln_render_session_clear_data(session) })
    }

    /// Dumps renderer debug logs through MapLibre Native logging.
    pub fn dump_debug_logs(&self) -> Result<()> {
        self.inner.ensure_no_frame_acquired()?;
        let session = self.inner.as_ptr()?;
        // SAFETY: session is a live render session handle owned by this wrapper.
        maplibre_core::check(unsafe { sys::mln_render_session_dump_debug_logs(session) })
    }

    /// Returns CPU readback metadata for the most recently rendered texture frame.
    pub fn texture_image_info(&self) -> Result<TextureImageInfo> {
        self.inner.ensure_no_frame_acquired()?;
        let session = self.inner.as_ptr()?;
        // SAFETY: Default constructor takes no arguments and initializes size.
        let mut info = unsafe { sys::mln_texture_image_info_default() };
        // SAFETY: session is live. Passing a null buffer with zero capacity is
        // the documented metadata probe path; out_info points to initialized storage.
        let status = unsafe {
            sys::mln_texture_read_premultiplied_rgba8(session, std::ptr::null_mut(), 0, &mut info)
        };
        if status == sys::MLN_STATUS_OK
            || (status == sys::MLN_STATUS_INVALID_ARGUMENT && info.byte_length > 0)
        {
            Ok(maplibre_core::values::texture_image_info_from_native(&info))
        } else {
            Err(crate::Error::from_status(status))
        }
    }

    /// Reads the most recently rendered texture frame as premultiplied RGBA8.
    pub fn read_premultiplied_rgba8_into(&self, data: &mut [u8]) -> Result<TextureImageInfo> {
        self.inner.ensure_no_frame_acquired()?;
        let session = self.inner.as_ptr()?;
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
        Ok(maplibre_core::values::texture_image_info_from_native(&info))
    }

    /// Reads the most recently rendered texture frame into owned bytes.
    pub fn read_premultiplied_rgba8(&self) -> Result<PremultipliedRgba8Image> {
        let info = self.texture_image_info()?;
        let mut data = vec![0; info.byte_length];
        let info = self.read_premultiplied_rgba8_into(&mut data)?;
        Ok(PremultipliedRgba8Image::new(info, data))
    }

    /// Acquires a borrowed Metal frame from a session-owned texture target.
    pub fn acquire_metal_owned_texture_frame(&self) -> Result<MetalOwnedTextureFrameHandle> {
        self.inner.ensure_no_frame_acquired()?;
        let session = self.inner.as_ptr()?;
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
        let session = self.inner.as_ptr()?;
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

fn empty_vulkan_owned_texture_frame() -> sys::mln_vulkan_owned_texture_frame {
    sys::mln_vulkan_owned_texture_frame {
        size: mem::size_of::<sys::mln_vulkan_owned_texture_frame>() as u32,
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
    }
}

#[cfg(test)]
mod tests;
