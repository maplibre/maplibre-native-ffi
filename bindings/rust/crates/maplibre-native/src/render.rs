use std::cell::Cell;
use std::fmt;
use std::marker::PhantomData;
use std::mem;
use std::ptr::NonNull;
use std::rc::Rc;

use maplibre_native_support as support;
use maplibre_native_sys as sys;

use crate::Result;
use crate::handle::{ThreadAffineNativeHandle, closed_handle_error, out_handle};
use crate::map::{MapHandle, MapState};

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

#[derive(Debug, Clone, PartialEq)]
#[non_exhaustive]
pub struct OwnedTextureDescriptor {
    pub width: u32,
    pub height: u32,
    pub scale_factor: f64,
}

impl OwnedTextureDescriptor {
    pub fn new(width: u32, height: u32, scale_factor: f64) -> Self {
        Self {
            width,
            height,
            scale_factor,
        }
    }

    pub(crate) fn to_native(&self) -> sys::mln_owned_texture_descriptor {
        // SAFETY: Default constructor takes no arguments and initializes size.
        let mut raw = unsafe { sys::mln_owned_texture_descriptor_default() };
        raw.width = self.width;
        raw.height = self.height;
        raw.scale_factor = self.scale_factor;
        raw
    }
}

impl Default for OwnedTextureDescriptor {
    fn default() -> Self {
        Self::new(256, 256, 1.0)
    }
}

#[derive(Debug, Clone, PartialEq)]
#[non_exhaustive]
pub struct MetalSurfaceDescriptor {
    pub width: u32,
    pub height: u32,
    pub scale_factor: f64,
    pub layer: NativePointer,
    pub device: NativePointer,
}

impl MetalSurfaceDescriptor {
    pub fn new(width: u32, height: u32, scale_factor: f64, layer: NativePointer) -> Self {
        Self {
            width,
            height,
            scale_factor,
            layer,
            device: NativePointer::NULL,
        }
    }

    pub fn with_device(mut self, device: NativePointer) -> Self {
        self.device = device;
        self
    }

    pub(crate) fn to_native(&self) -> sys::mln_metal_surface_descriptor {
        // SAFETY: Default constructor takes no arguments and initializes size.
        let mut raw = unsafe { sys::mln_metal_surface_descriptor_default() };
        raw.width = self.width;
        raw.height = self.height;
        raw.scale_factor = self.scale_factor;
        raw.layer = self.layer.as_void_ptr();
        raw.device = self.device.as_void_ptr();
        raw
    }
}

#[derive(Debug, Clone, PartialEq)]
#[non_exhaustive]
pub struct VulkanSurfaceDescriptor {
    pub width: u32,
    pub height: u32,
    pub scale_factor: f64,
    pub instance: NativePointer,
    pub physical_device: NativePointer,
    pub device: NativePointer,
    pub graphics_queue: NativePointer,
    pub graphics_queue_family_index: u32,
    pub surface: NativePointer,
}

impl VulkanSurfaceDescriptor {
    #[allow(clippy::too_many_arguments)]
    pub fn new(
        width: u32,
        height: u32,
        scale_factor: f64,
        instance: NativePointer,
        physical_device: NativePointer,
        device: NativePointer,
        graphics_queue: NativePointer,
        graphics_queue_family_index: u32,
        surface: NativePointer,
    ) -> Self {
        Self {
            width,
            height,
            scale_factor,
            instance,
            physical_device,
            device,
            graphics_queue,
            graphics_queue_family_index,
            surface,
        }
    }

    pub(crate) fn to_native(&self) -> sys::mln_vulkan_surface_descriptor {
        // SAFETY: Default constructor takes no arguments and initializes size.
        let mut raw = unsafe { sys::mln_vulkan_surface_descriptor_default() };
        raw.width = self.width;
        raw.height = self.height;
        raw.scale_factor = self.scale_factor;
        raw.instance = self.instance.as_void_ptr();
        raw.physical_device = self.physical_device.as_void_ptr();
        raw.device = self.device.as_void_ptr();
        raw.graphics_queue = self.graphics_queue.as_void_ptr();
        raw.graphics_queue_family_index = self.graphics_queue_family_index;
        raw.surface = self.surface.as_void_ptr();
        raw
    }
}

#[derive(Debug, Clone, PartialEq)]
#[non_exhaustive]
pub struct MetalOwnedTextureDescriptor {
    pub width: u32,
    pub height: u32,
    pub scale_factor: f64,
    pub device: NativePointer,
}

impl MetalOwnedTextureDescriptor {
    pub fn new(width: u32, height: u32, scale_factor: f64, device: NativePointer) -> Self {
        Self {
            width,
            height,
            scale_factor,
            device,
        }
    }

    pub(crate) fn to_native(&self) -> sys::mln_metal_owned_texture_descriptor {
        // SAFETY: Default constructor takes no arguments and initializes size.
        let mut raw = unsafe { sys::mln_metal_owned_texture_descriptor_default() };
        raw.width = self.width;
        raw.height = self.height;
        raw.scale_factor = self.scale_factor;
        raw.device = self.device.as_void_ptr();
        raw
    }
}

#[derive(Debug, Clone, PartialEq)]
#[non_exhaustive]
pub struct MetalBorrowedTextureDescriptor {
    pub width: u32,
    pub height: u32,
    pub scale_factor: f64,
    pub texture: NativePointer,
}

impl MetalBorrowedTextureDescriptor {
    pub fn new(width: u32, height: u32, scale_factor: f64, texture: NativePointer) -> Self {
        Self {
            width,
            height,
            scale_factor,
            texture,
        }
    }

    pub(crate) fn to_native(&self) -> sys::mln_metal_borrowed_texture_descriptor {
        // SAFETY: Default constructor takes no arguments and initializes size.
        let mut raw = unsafe { sys::mln_metal_borrowed_texture_descriptor_default() };
        raw.width = self.width;
        raw.height = self.height;
        raw.scale_factor = self.scale_factor;
        raw.texture = self.texture.as_void_ptr();
        raw
    }
}

#[derive(Debug, Clone, PartialEq)]
#[non_exhaustive]
pub struct VulkanOwnedTextureDescriptor {
    pub width: u32,
    pub height: u32,
    pub scale_factor: f64,
    pub instance: NativePointer,
    pub physical_device: NativePointer,
    pub device: NativePointer,
    pub graphics_queue: NativePointer,
    pub graphics_queue_family_index: u32,
}

impl VulkanOwnedTextureDescriptor {
    #[allow(clippy::too_many_arguments)]
    pub fn new(
        width: u32,
        height: u32,
        scale_factor: f64,
        instance: NativePointer,
        physical_device: NativePointer,
        device: NativePointer,
        graphics_queue: NativePointer,
        graphics_queue_family_index: u32,
    ) -> Self {
        Self {
            width,
            height,
            scale_factor,
            instance,
            physical_device,
            device,
            graphics_queue,
            graphics_queue_family_index,
        }
    }

    pub(crate) fn to_native(&self) -> sys::mln_vulkan_owned_texture_descriptor {
        // SAFETY: Default constructor takes no arguments and initializes size.
        let mut raw = unsafe { sys::mln_vulkan_owned_texture_descriptor_default() };
        raw.width = self.width;
        raw.height = self.height;
        raw.scale_factor = self.scale_factor;
        raw.instance = self.instance.as_void_ptr();
        raw.physical_device = self.physical_device.as_void_ptr();
        raw.device = self.device.as_void_ptr();
        raw.graphics_queue = self.graphics_queue.as_void_ptr();
        raw.graphics_queue_family_index = self.graphics_queue_family_index;
        raw
    }
}

#[derive(Debug, Clone, PartialEq)]
#[non_exhaustive]
pub struct VulkanBorrowedTextureDescriptor {
    pub width: u32,
    pub height: u32,
    pub scale_factor: f64,
    pub instance: NativePointer,
    pub physical_device: NativePointer,
    pub device: NativePointer,
    pub graphics_queue: NativePointer,
    pub graphics_queue_family_index: u32,
    pub image: NativePointer,
    pub image_view: NativePointer,
    pub format: u32,
    pub initial_layout: u32,
    pub final_layout: u32,
}

impl VulkanBorrowedTextureDescriptor {
    #[allow(clippy::too_many_arguments)]
    pub fn new(
        width: u32,
        height: u32,
        scale_factor: f64,
        instance: NativePointer,
        physical_device: NativePointer,
        device: NativePointer,
        graphics_queue: NativePointer,
        graphics_queue_family_index: u32,
        image: NativePointer,
        image_view: NativePointer,
        format: u32,
        initial_layout: u32,
        final_layout: u32,
    ) -> Self {
        Self {
            width,
            height,
            scale_factor,
            instance,
            physical_device,
            device,
            graphics_queue,
            graphics_queue_family_index,
            image,
            image_view,
            format,
            initial_layout,
            final_layout,
        }
    }

    pub(crate) fn to_native(&self) -> sys::mln_vulkan_borrowed_texture_descriptor {
        // SAFETY: Default constructor takes no arguments and initializes size.
        let mut raw = unsafe { sys::mln_vulkan_borrowed_texture_descriptor_default() };
        raw.width = self.width;
        raw.height = self.height;
        raw.scale_factor = self.scale_factor;
        raw.instance = self.instance.as_void_ptr();
        raw.physical_device = self.physical_device.as_void_ptr();
        raw.device = self.device.as_void_ptr();
        raw.graphics_queue = self.graphics_queue.as_void_ptr();
        raw.graphics_queue_family_index = self.graphics_queue_family_index;
        raw.image = self.image.as_void_ptr();
        raw.image_view = self.image_view.as_void_ptr();
        raw.format = self.format;
        raw.initial_layout = self.initial_layout;
        raw.final_layout = self.final_layout;
        raw
    }
}

#[derive(Debug)]
struct RenderSessionState {
    handle: ThreadAffineNativeHandle<sys::mln_render_session>,
    _map: Rc<MapState>,
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
            _map: map,
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

fn frame_acquired_error() -> crate::Error {
    crate::Error::new(
        crate::ErrorKind::InvalidState,
        None,
        "render session has an acquired texture frame",
    )
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[non_exhaustive]
pub struct TextureImageInfo {
    pub width: u32,
    pub height: u32,
    pub stride: u32,
    pub byte_length: usize,
}

impl TextureImageInfo {
    fn from_native(raw: &sys::mln_texture_image_info) -> Self {
        Self {
            width: raw.width,
            height: raw.height,
            stride: raw.stride,
            byte_length: raw.byte_length,
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
#[non_exhaustive]
pub struct PremultipliedRgba8Image {
    pub info: TextureImageInfo,
    pub data: Vec<u8>,
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
    pub unsafe fn texture(&self) -> Result<NativePointer> {
        if self.closed.get() {
            Err(closed_handle_error("MetalOwnedTextureFrameHandle"))
        } else {
            // SAFETY: The active native frame owns the validity contract for
            // this borrowed backend handle until release.
            Ok(unsafe { NativePointer::from_ptr(self.raw.texture) })
        }
    }

    /// Returns the borrowed Metal device pointer for backend interop.
    ///
    /// # Safety
    ///
    /// The returned pointer has the same lifetime and synchronization
    /// requirements as [`MetalOwnedTextureFrameHandle::texture`].
    pub unsafe fn device(&self) -> Result<NativePointer> {
        if self.closed.get() {
            Err(closed_handle_error("MetalOwnedTextureFrameHandle"))
        } else {
            // SAFETY: See texture above.
            Ok(unsafe { NativePointer::from_ptr(self.raw.device) })
        }
    }

    /// Explicitly releases this frame.
    pub fn close(&self) -> Result<()> {
        if self.closed.get() {
            return Ok(());
        }
        let session = self.session.as_ptr()?;
        // SAFETY: session is live, and raw is the active frame returned by a
        // successful acquire for this session until release succeeds.
        support::check(unsafe { sys::mln_metal_owned_texture_release_frame(session, &self.raw) })?;
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
    pub unsafe fn image(&self) -> Result<NativePointer> {
        if self.closed.get() {
            Err(closed_handle_error("VulkanOwnedTextureFrameHandle"))
        } else {
            // SAFETY: The active native frame owns the validity contract for
            // this borrowed backend handle until release.
            Ok(unsafe { NativePointer::from_ptr(self.raw.image) })
        }
    }

    /// Returns the borrowed Vulkan image view pointer for backend interop.
    ///
    /// # Safety
    ///
    /// The returned pointer has the same lifetime and synchronization
    /// requirements as [`VulkanOwnedTextureFrameHandle::image`].
    pub unsafe fn image_view(&self) -> Result<NativePointer> {
        if self.closed.get() {
            Err(closed_handle_error("VulkanOwnedTextureFrameHandle"))
        } else {
            // SAFETY: See image above.
            Ok(unsafe { NativePointer::from_ptr(self.raw.image_view) })
        }
    }

    /// Returns the borrowed Vulkan device pointer for backend interop.
    ///
    /// # Safety
    ///
    /// The returned pointer has the same lifetime and synchronization
    /// requirements as [`VulkanOwnedTextureFrameHandle::image`].
    pub unsafe fn device(&self) -> Result<NativePointer> {
        if self.closed.get() {
            Err(closed_handle_error("VulkanOwnedTextureFrameHandle"))
        } else {
            // SAFETY: See image above.
            Ok(unsafe { NativePointer::from_ptr(self.raw.device) })
        }
    }

    /// Explicitly releases this frame.
    pub fn close(&self) -> Result<()> {
        if self.closed.get() {
            return Ok(());
        }
        let session = self.session.as_ptr()?;
        // SAFETY: session is live, and raw is the active frame returned by a
        // successful acquire for this session until release succeeds.
        support::check(unsafe { sys::mln_vulkan_owned_texture_release_frame(session, &self.raw) })?;
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
        let mut out = support::ptr::OutPtr::<sys::mln_render_session>::new();
        let status = attach(map_ptr, out.as_mut_ptr());
        support::check(status)?;
        let ptr = out_handle(out, "mln_render_session")?;
        Ok(Self {
            inner: Rc::new(RenderSessionState::new(ptr, Rc::clone(&map.inner))),
        })
    }

    /// Explicitly destroys the render session.
    ///
    /// Native destruction errors are returned. When destruction fails, the
    /// underlying native handle remains live so a later `close` can retry.
    pub fn close(&self) -> Result<()> {
        self.inner.ensure_no_frame_acquired()?;
        self.inner.handle.close()
    }

    pub fn is_closed(&self) -> bool {
        self.inner.handle.is_closed()
    }

    /// Resizes this attached render session.
    pub fn resize(&self, width: u32, height: u32, scale_factor: f64) -> Result<()> {
        self.inner.ensure_no_frame_acquired()?;
        let session = self.inner.as_ptr()?;
        // SAFETY: session is a live render session handle owned by this wrapper.
        support::check(unsafe {
            sys::mln_render_session_resize(session, width, height, scale_factor)
        })
    }

    /// Processes the latest map render update for this render target.
    pub fn render_update(&self) -> Result<()> {
        self.inner.ensure_no_frame_acquired()?;
        let session = self.inner.as_ptr()?;
        // SAFETY: session is a live render session handle owned by this wrapper.
        support::check(unsafe { sys::mln_render_session_render_update(session) })
    }

    /// Detaches backend-bound render resources from the map.
    pub fn detach(&self) -> Result<()> {
        self.inner.ensure_no_frame_acquired()?;
        if self.inner.detached.get() {
            return Ok(());
        }
        let session = self.inner.as_ptr()?;
        // SAFETY: session is a live render session handle owned by this wrapper.
        support::check(unsafe { sys::mln_render_session_detach(session) })?;
        self.inner.detached.set(true);
        Ok(())
    }

    /// Asks the session renderer to release cached resources where possible.
    pub fn reduce_memory_use(&self) -> Result<()> {
        self.inner.ensure_no_frame_acquired()?;
        let session = self.inner.as_ptr()?;
        // SAFETY: session is a live render session handle owned by this wrapper.
        support::check(unsafe { sys::mln_render_session_reduce_memory_use(session) })
    }

    /// Clears renderer data for the session.
    pub fn clear_data(&self) -> Result<()> {
        self.inner.ensure_no_frame_acquired()?;
        let session = self.inner.as_ptr()?;
        // SAFETY: session is a live render session handle owned by this wrapper.
        support::check(unsafe { sys::mln_render_session_clear_data(session) })
    }

    /// Dumps renderer debug logs through MapLibre Native logging.
    pub fn dump_debug_logs(&self) -> Result<()> {
        self.inner.ensure_no_frame_acquired()?;
        let session = self.inner.as_ptr()?;
        // SAFETY: session is a live render session handle owned by this wrapper.
        support::check(unsafe { sys::mln_render_session_dump_debug_logs(session) })
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
            Ok(TextureImageInfo::from_native(&info))
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
        support::check(unsafe {
            sys::mln_texture_read_premultiplied_rgba8(session, data_ptr, data.len(), &mut info)
        })?;
        Ok(TextureImageInfo::from_native(&info))
    }

    /// Reads the most recently rendered texture frame into owned bytes.
    pub fn read_premultiplied_rgba8(&self) -> Result<PremultipliedRgba8Image> {
        let info = self.texture_image_info()?;
        let mut data = vec![0; info.byte_length];
        let info = self.read_premultiplied_rgba8_into(&mut data)?;
        Ok(PremultipliedRgba8Image { info, data })
    }

    /// Acquires a borrowed Metal frame from a session-owned texture target.
    pub fn acquire_metal_owned_texture_frame(&self) -> Result<MetalOwnedTextureFrameHandle> {
        self.inner.ensure_no_frame_acquired()?;
        let session = self.inner.as_ptr()?;
        let mut raw = empty_metal_owned_texture_frame();
        // SAFETY: session is live and raw points to initialized writable frame storage.
        support::check(unsafe { sys::mln_metal_owned_texture_acquire_frame(session, &mut raw) })?;
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
        support::check(unsafe { sys::mln_vulkan_owned_texture_acquire_frame(session, &mut raw) })?;
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
mod tests {
    use std::mem;

    use static_assertions::assert_not_impl_any;

    use super::*;
    use crate::{ErrorKind, MapMode, MapOptions, RuntimeHandle};

    assert_not_impl_any!(NativePointer: Send, Sync);
    assert_not_impl_any!(RenderSessionHandle: Send, Sync);
    assert_not_impl_any!(MetalOwnedTextureFrameHandle: Send, Sync);
    assert_not_impl_any!(VulkanOwnedTextureFrameHandle: Send, Sync);

    #[test]
    fn native_pointer_round_trips_address() {
        // SAFETY: Test uses a dummy opaque address and does not dereference it.
        let pointer = unsafe { NativePointer::from_address(0x1234) };
        assert_eq!(pointer.address(), 0x1234);
        // SAFETY: Test only verifies address reconstruction; it does not dereference.
        assert_eq!(unsafe { pointer.as_ptr::<u8>() } as usize, 0x1234);
        assert!(NativePointer::NULL.is_null());
    }

    #[test]
    fn frame_metadata_copies_values_without_exposing_backend_pointers() {
        let mut metal = empty_metal_owned_texture_frame();
        metal.generation = 1;
        metal.width = 64;
        metal.height = 32;
        metal.scale_factor = 2.0;
        metal.frame_id = 9;
        metal.texture = 0x1000usize as *mut _;
        metal.device = 0x2000usize as *mut _;
        metal.pixel_format = 80;
        let copied = MetalOwnedTextureFrame::from_native(&metal);
        assert_eq!(copied.generation, 1);
        assert_eq!(
            (copied.width, copied.height, copied.scale_factor),
            (64, 32, 2.0)
        );
        assert_eq!(copied.frame_id, 9);
        assert_eq!(copied.pixel_format, 80);

        let mut vulkan = empty_vulkan_owned_texture_frame();
        vulkan.generation = 3;
        vulkan.width = 128;
        vulkan.height = 96;
        vulkan.scale_factor = 1.5;
        vulkan.frame_id = 11;
        vulkan.image = 0x3000usize as *mut _;
        vulkan.image_view = 0x4000usize as *mut _;
        vulkan.device = 0x5000usize as *mut _;
        vulkan.format = 44;
        vulkan.layout = 55;
        let copied = VulkanOwnedTextureFrame::from_native(&vulkan);
        assert_eq!(copied.generation, 3);
        assert_eq!(
            (copied.width, copied.height, copied.scale_factor),
            (128, 96, 1.5)
        );
        assert_eq!(copied.frame_id, 11);
        assert_eq!((copied.format, copied.layout), (44, 55));
    }

    #[test]
    fn descriptor_materialization_fills_sizes_and_pointers() {
        // SAFETY: Test uses dummy opaque addresses and does not dereference them.
        let p1 = unsafe { NativePointer::from_address(0x10) };
        // SAFETY: Test uses dummy opaque addresses and does not dereference them.
        let p2 = unsafe { NativePointer::from_address(0x20) };
        // SAFETY: Test uses dummy opaque addresses and does not dereference them.
        let p3 = unsafe { NativePointer::from_address(0x30) };
        // SAFETY: Test uses dummy opaque addresses and does not dereference them.
        let p4 = unsafe { NativePointer::from_address(0x40) };
        // SAFETY: Test uses dummy opaque addresses and does not dereference them.
        let p5 = unsafe { NativePointer::from_address(0x50) };

        let owned = OwnedTextureDescriptor::new(32, 16, 2.0).to_native();
        assert_eq!(owned.size as usize, mem::size_of_val(&owned));
        assert_eq!(
            (owned.width, owned.height, owned.scale_factor),
            (32, 16, 2.0)
        );

        let metal_surface = MetalSurfaceDescriptor::new(1, 2, 3.0, p1)
            .with_device(p2)
            .to_native();
        assert_eq!(
            metal_surface.size as usize,
            mem::size_of_val(&metal_surface)
        );
        assert_eq!(metal_surface.layer as usize, 0x10);
        assert_eq!(metal_surface.device as usize, 0x20);

        let vulkan_surface =
            VulkanSurfaceDescriptor::new(1, 2, 3.0, p1, p2, p3, p4, 7, p5).to_native();
        assert_eq!(
            vulkan_surface.size as usize,
            mem::size_of_val(&vulkan_surface)
        );
        assert_eq!(vulkan_surface.instance as usize, 0x10);
        assert_eq!(vulkan_surface.physical_device as usize, 0x20);
        assert_eq!(vulkan_surface.device as usize, 0x30);
        assert_eq!(vulkan_surface.graphics_queue as usize, 0x40);
        assert_eq!(vulkan_surface.graphics_queue_family_index, 7);
        assert_eq!(vulkan_surface.surface as usize, 0x50);

        let metal_owned = MetalOwnedTextureDescriptor::new(1, 2, 3.0, p1).to_native();
        assert_eq!(metal_owned.size as usize, mem::size_of_val(&metal_owned));
        assert_eq!(metal_owned.device as usize, 0x10);

        let metal_borrowed = MetalBorrowedTextureDescriptor::new(1, 2, 3.0, p1).to_native();
        assert_eq!(
            metal_borrowed.size as usize,
            mem::size_of_val(&metal_borrowed)
        );
        assert_eq!(metal_borrowed.texture as usize, 0x10);

        let vulkan_owned =
            VulkanOwnedTextureDescriptor::new(1, 2, 3.0, p1, p2, p3, p4, 9).to_native();
        assert_eq!(vulkan_owned.size as usize, mem::size_of_val(&vulkan_owned));
        assert_eq!(vulkan_owned.instance as usize, 0x10);
        assert_eq!(vulkan_owned.graphics_queue_family_index, 9);

        let vulkan_borrowed =
            VulkanBorrowedTextureDescriptor::new(1, 2, 3.0, p1, p2, p3, p4, 11, p5, p1, 44, 55, 66)
                .to_native();
        assert_eq!(
            vulkan_borrowed.size as usize,
            mem::size_of_val(&vulkan_borrowed)
        );
        assert_eq!(vulkan_borrowed.image as usize, 0x50);
        assert_eq!(vulkan_borrowed.image_view as usize, 0x10);
        assert_eq!(vulkan_borrowed.format, 44);
        assert_eq!(vulkan_borrowed.initial_layout, 55);
        assert_eq!(vulkan_borrowed.final_layout, 66);
    }

    #[test]
    fn owned_texture_session_retains_parent_and_enforces_single_session() {
        let runtime = RuntimeHandle::new().unwrap();
        let map = MapHandle::with_options(
            &runtime,
            &MapOptions::new(64, 64, 1.0).with_mode(MapMode::Static),
        )
        .unwrap();
        let session = map
            .attach_owned_texture(&OwnedTextureDescriptor::new(32, 16, 1.0))
            .unwrap();

        let error = map
            .attach_owned_texture(&OwnedTextureDescriptor::new(32, 16, 1.0))
            .unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidState);

        drop(map);
        drop(runtime);

        session.detach().unwrap();
        session.detach().unwrap();
        session.close().unwrap();
        session.close().unwrap();
    }

    #[test]
    fn acquired_frame_state_rejects_reentrant_session_operations_before_native_calls() {
        let runtime = RuntimeHandle::new().unwrap();
        let map = MapHandle::with_options(
            &runtime,
            &MapOptions::new(64, 64, 1.0).with_mode(MapMode::Static),
        )
        .unwrap();
        let session = map
            .attach_owned_texture(&OwnedTextureDescriptor::new(32, 16, 1.0))
            .unwrap();

        session.inner.frame_acquired.set(true);

        for error in [
            session.resize(32, 16, 1.0).unwrap_err(),
            session.render_update().unwrap_err(),
            session.detach().unwrap_err(),
            session.close().unwrap_err(),
            session.read_premultiplied_rgba8_into(&mut []).unwrap_err(),
            session.acquire_metal_owned_texture_frame().unwrap_err(),
            session.acquire_vulkan_owned_texture_frame().unwrap_err(),
        ] {
            assert_eq!(error.kind(), ErrorKind::InvalidState);
            assert!(error.diagnostic().contains("acquired texture frame"));
        }

        session.inner.frame_acquired.set(false);
        session.close().unwrap();
    }

    #[test]
    fn texture_readback_reports_documented_error_kinds_for_unsized_buffer() {
        let runtime = RuntimeHandle::new().unwrap();
        let map = MapHandle::with_options(
            &runtime,
            &MapOptions::new(64, 64, 1.0).with_mode(MapMode::Static),
        )
        .unwrap();
        let session = map
            .attach_owned_texture(&OwnedTextureDescriptor::new(32, 16, 1.0))
            .unwrap();

        let _ = session.render_update();
        let mut empty = [];
        let error = session
            .read_premultiplied_rgba8_into(&mut empty)
            .unwrap_err();
        assert!(matches!(
            error.kind(),
            ErrorKind::InvalidArgument | ErrorKind::InvalidState | ErrorKind::Unsupported
        ));

        session.close().unwrap();
    }

    #[test]
    fn backend_specific_attach_calls_report_native_statuses() {
        let runtime = RuntimeHandle::new().unwrap();
        let map = MapHandle::with_options(
            &runtime,
            &MapOptions::new(64, 64, 1.0).with_mode(MapMode::Static),
        )
        .unwrap();

        let metal_error = map
            .attach_metal_owned_texture(&MetalOwnedTextureDescriptor::new(
                32,
                16,
                1.0,
                NativePointer::NULL,
            ))
            .unwrap_err();
        assert!(matches!(
            metal_error.kind(),
            ErrorKind::InvalidArgument | ErrorKind::Unsupported
        ));

        let vulkan_error = map
            .attach_vulkan_surface(&VulkanSurfaceDescriptor::new(
                32,
                16,
                1.0,
                NativePointer::NULL,
                NativePointer::NULL,
                NativePointer::NULL,
                NativePointer::NULL,
                0,
                NativePointer::NULL,
            ))
            .unwrap_err();
        assert!(matches!(
            vulkan_error.kind(),
            ErrorKind::InvalidArgument | ErrorKind::Unsupported
        ));

        map.close().unwrap();
        runtime.close().unwrap();
    }
}
