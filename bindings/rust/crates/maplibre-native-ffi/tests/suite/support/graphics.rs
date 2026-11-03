//! GPU contexts and textures from tests/graphics, loaded over its C ABI.
//!
//! The library installs beside the C API library, so it loads from the same
//! search path the test process already has. See
//! tests/graphics/include/mln_test_graphics.h.
//! The backend is the one this build compiled in, which the build script reads
//! from the native install as `mln_render_backend`.

use std::ffi::{CStr, c_char, c_void};
use std::marker::PhantomData;
use std::sync::OnceLock;

use libloading::Library;
use maplibre_native_ffi::*;

use super::Session;

const BACKEND_METAL: u32 = 1;
const BACKEND_VULKAN: u32 = 2;
const BACKEND_EGL: u32 = 3;
const BACKEND_WGL: u32 = 4;

// Without the native install's artifact descriptor, the build script sets no
// backend, and the attach would fail far from the cause.
#[cfg(not(any(
    mln_render_backend = "metal",
    mln_render_backend = "vulkan",
    mln_render_backend = "opengl"
)))]
compile_error!(
    "the native install names no render backend these tests can drive; \
     check the artifact descriptor under MAPLIBRE_NATIVE_FFI_LOCAL_BUILD"
);

/// The tests/graphics backend that provides this build's render backend.
const BACKEND: u32 = if cfg!(mln_render_backend = "metal") {
    BACKEND_METAL
} else if cfg!(mln_render_backend = "vulkan") {
    BACKEND_VULKAN
} else if cfg!(target_os = "windows") {
    BACKEND_WGL
} else {
    BACKEND_EGL
};

/// Whether this build's sessions share a context that the host makes current,
/// which ties their driver to the host's graphics thread.
const USES_CURRENT_CONTEXT: bool = BACKEND == BACKEND_EGL || BACKEND == BACKEND_WGL;

/// mln_test_graphics_context, field for field.
#[repr(C)]
struct ContextInfo {
    backend: u32,
    vulkan_queue_family_index: u32,
    metal_device: *mut c_void,
    vulkan_instance: *mut c_void,
    vulkan_physical_device: *mut c_void,
    vulkan_device: *mut c_void,
    vulkan_queue: *mut c_void,
    vulkan_get_instance_proc_addr: *mut c_void,
    vulkan_get_device_proc_addr: *mut c_void,
    egl_display: *mut c_void,
    egl_config: *mut c_void,
    egl_context: *mut c_void,
    wgl_device_context: *mut c_void,
    wgl_context: *mut c_void,
    get_proc_address: *mut c_void,
}

/// mln_test_graphics_texture_info, field for field.
#[repr(C)]
struct TextureInfo {
    vulkan_image: u64,
    vulkan_image_view: u64,
    metal_texture: *mut c_void,
    width: u32,
    height: u32,
    format: u32,
    vulkan_final_layout: u32,
    opengl_texture: u32,
    opengl_target: u32,
}

struct Api {
    last_error: unsafe extern "C" fn() -> *const c_char,
    create: unsafe extern "C" fn(u32) -> *mut c_void,
    destroy: unsafe extern "C" fn(*mut c_void),
    get_context: unsafe extern "C" fn(*const c_void, *mut ContextInfo) -> bool,
    make_current: unsafe extern "C" fn(*mut c_void) -> bool,
    texture_create: unsafe extern "C" fn(*mut c_void, u32, u32) -> *mut c_void,
    texture_destroy: unsafe extern "C" fn(*mut c_void),
    texture_get_info: unsafe extern "C" fn(*const c_void, *mut TextureInfo) -> bool,
}

fn api() -> &'static Api {
    static API: OnceLock<Api> = OnceLock::new();
    API.get_or_init(|| {
        // The build's own install holds the library. A runner that pushes the
        // tests to a device pushes it beside them, where the platform's
        // search path finds it by name.
        let name = libloading::library_filename("mln_test_graphics");
        let path = option_env!("MLN_FFI_TEST_GRAPHICS_DIR")
            .map(|directory| std::path::Path::new(directory).join(&name))
            .filter(|path| path.exists())
            .map_or_else(|| name.clone(), Into::into);
        // SAFETY: tests/graphics runs no initialization when it loads.
        let library = unsafe { Library::new(&path) }
            .unwrap_or_else(|error| panic!("loading {path:?} failed: {error}"));
        // The library stays loaded for the process, so its functions outlive
        // every graphics object.
        let library: &'static Library = Box::leak(Box::new(library));
        // SAFETY: each symbol has the signature that mln_test_graphics.h
        // declares for it.
        unsafe {
            Api {
                last_error: *library.get(b"mln_test_graphics_last_error\0").unwrap(),
                create: *library.get(b"mln_test_graphics_create\0").unwrap(),
                destroy: *library.get(b"mln_test_graphics_destroy\0").unwrap(),
                get_context: *library.get(b"mln_test_graphics_get_context\0").unwrap(),
                make_current: *library.get(b"mln_test_graphics_make_current\0").unwrap(),
                texture_create: *library.get(b"mln_test_graphics_texture_create\0").unwrap(),
                texture_destroy: *library.get(b"mln_test_graphics_texture_destroy\0").unwrap(),
                texture_get_info: *library
                    .get(b"mln_test_graphics_texture_get_info\0")
                    .unwrap(),
            }
        }
    })
}

fn last_error() -> String {
    // SAFETY: the library returns a NUL-terminated string for this thread.
    unsafe { CStr::from_ptr((api().last_error)()) }
        .to_string_lossy()
        .into_owned()
}

/// A device or context standing in for the host's, on the thread that created
/// it. Destroy every session that uses it first, and wait for a disposed one
/// to release its wakes.
pub struct Graphics {
    raw: *mut c_void,
    context: ContextInfo,
}

impl Graphics {
    /// Creates the build backend's device or context. A context-sharing
    /// backend makes it current on the calling thread, which then drives the
    /// sessions that share it.
    pub fn new() -> Self {
        // SAFETY: create takes a backend value and returns an owned object or
        // null.
        let raw = unsafe { (api().create)(BACKEND) };
        assert!(!raw.is_null(), "tests/graphics failed: {}", last_error());
        // SAFETY: the struct is plain data, and all zeroes is its empty value.
        let mut context: ContextInfo = unsafe { std::mem::zeroed() };
        // SAFETY: raw is live and context is writable for the call.
        assert!(
            unsafe { (api().get_context)(raw, &mut context) },
            "tests/graphics failed: {}",
            last_error()
        );
        let graphics = Self { raw, context };
        if USES_CURRENT_CONTEXT {
            // SAFETY: raw is live and belongs to this thread.
            assert!(
                unsafe { (api().make_current)(graphics.raw) },
                "tests/graphics failed: {}",
                last_error()
            );
        }
        graphics
    }

    /// The driver a borrowed-texture session on this context takes by
    /// default: the calling thread when it shares a current context, and
    /// otherwise the core's own worker.
    fn default_driver() -> RenderDriverKind {
        if USES_CURRENT_CONTEXT {
            RenderDriverKind::CallerGraphicsThread
        } else {
            RenderDriverKind::CoreWorker
        }
    }

    /// Creates a width by height RGBA texture on this device or context.
    pub fn texture(&self, width: u32, height: u32) -> Texture<'_> {
        // SAFETY: raw is live and belongs to this thread.
        let raw = unsafe { (api().texture_create)(self.raw, width, height) };
        assert!(!raw.is_null(), "tests/graphics failed: {}", last_error());
        let mut texture = Texture {
            raw,
            // SAFETY: the struct is plain data, and all zeroes is its empty
            // value.
            info: unsafe { std::mem::zeroed() },
            graphics: PhantomData,
        };
        // SAFETY: raw is live and info is writable for the call.
        assert!(
            unsafe { (api().texture_get_info)(texture.raw, &mut texture.info) },
            "tests/graphics failed: {}",
            last_error()
        );
        texture
    }

    /// Attaches a session that renders into `ring`, one texture per slot, all
    /// of this context's and the same size.
    pub fn attach_borrowed_texture(
        &self,
        map: &MapHandle,
        logical_size: LogicalSize,
        ring: &[Texture<'_>],
        options: &RenderSessionAttachOptions,
    ) -> Result<(RenderSessionHandle, NativeFuture<()>)> {
        let context = &self.context;
        let info = &ring.first().expect("a ring has a texture").info;
        let (physical_width, physical_height) = (info.width, info.height);
        // SAFETY: every handle comes from this live graphics object, and the
        // textures outlive the session.
        unsafe {
            match BACKEND {
                #[cfg(mln_render_backend = "metal")]
                BACKEND_METAL => map.attach_metal_borrowed_texture(
                    &MetalBorrowedTextureDescriptor::new(
                        logical_size,
                        physical_width,
                        physical_height,
                        ring.iter()
                            .map(|texture| MetalBorrowedTexture::new(texture.info.metal_texture))
                            .collect(),
                    ),
                    options,
                ),
                #[cfg(mln_render_backend = "vulkan")]
                BACKEND_VULKAN => map.attach_vulkan_borrowed_texture(
                    &VulkanBorrowedTextureDescriptor::new(
                        logical_size,
                        physical_width,
                        physical_height,
                        VulkanContextDescriptor {
                            instance: context.vulkan_instance,
                            physical_device: context.vulkan_physical_device,
                            device: context.vulkan_device,
                            graphics_queue: context.vulkan_queue,
                            graphics_queue_family_index: context.vulkan_queue_family_index,
                            get_instance_proc_addr: context.vulkan_get_instance_proc_addr,
                            get_device_proc_addr: context.vulkan_get_device_proc_addr,
                        },
                        ring.iter()
                            .map(|texture| {
                                VulkanBorrowedTexture::new(
                                    texture.info.vulkan_image,
                                    texture.info.vulkan_image_view,
                                )
                            })
                            .collect(),
                        info.format,
                        info.vulkan_final_layout,
                    ),
                    options,
                ),
                _ => map.attach_open_gl_borrowed_texture(
                    &OpenGlBorrowedTextureDescriptor::new(
                        logical_size,
                        physical_width,
                        physical_height,
                        OpenGlContextDescriptor {
                            ownership: OpenGlContextOwnership::Shared,
                            data: if BACKEND == BACKEND_WGL {
                                OpenGlContextDescriptorData::Wgl(WglContextDescriptor {
                                    device_context: context.wgl_device_context,
                                    share_context: context.wgl_context,
                                    get_proc_address: context.get_proc_address,
                                })
                            } else {
                                OpenGlContextDescriptorData::Egl(EglContextDescriptor {
                                    share_context: context.egl_context,
                                    ..EglContextDescriptor::new(
                                        context.egl_display,
                                        context.egl_config,
                                    )
                                })
                            },
                        },
                        ring.iter()
                            .map(|texture| OpenGlBorrowedTexture::new(texture.info.opengl_texture))
                            .collect(),
                        info.opengl_target,
                    ),
                    options,
                ),
            }
        }
    }

    /// Attaches a session that renders into `ring` with this build's default
    /// driver, and waits until it is attached.
    pub fn borrowed_texture_session(
        &self,
        map: &MapHandle,
        logical_size: LogicalSize,
        ring: &[Texture<'_>],
    ) -> Session {
        let (options, wakes) = Session::attach_options(Self::default_driver());
        Session::finish_attach(
            self.attach_borrowed_texture(map, logical_size, ring, &options)
                .unwrap(),
            wakes,
        )
    }
}

impl Drop for Graphics {
    fn drop(&mut self) {
        // SAFETY: raw is this value's and is destroyed once.
        unsafe { (api().destroy)(self.raw) };
    }
}

/// A texture that the host owns, for a borrowed ring. Drop it only once every
/// session that renders into it has retired.
pub struct Texture<'graphics> {
    raw: *mut c_void,
    info: TextureInfo,
    graphics: PhantomData<&'graphics Graphics>,
}

impl Drop for Texture<'_> {
    fn drop(&mut self) {
        // SAFETY: raw is this value's and is destroyed once, before the
        // graphics object it borrows.
        unsafe { (api().texture_destroy)(self.raw) };
    }
}

/// Runs `inside` while `frame` lends out its backend texture view, the way a
/// host samples a frame, and returns the view's physical size with what
/// `inside` returned.
pub fn with_frame_view<R>(
    frame: &AcquiredFrameHandle,
    inside: impl FnOnce() -> R,
) -> Result<((u32, u32), R)> {
    let view = frame.view()?;
    let size = match BACKEND {
        #[cfg(mln_render_backend = "metal")]
        BACKEND_METAL => view
            .get_metal_texture()
            .map(|texture| (texture.width, texture.height)),
        #[cfg(mln_render_backend = "vulkan")]
        BACKEND_VULKAN => view
            .get_vulkan_texture()
            .map(|texture| (texture.width, texture.height)),
        _ => view
            .get_open_gl_texture()
            .map(|texture| (texture.width, texture.height)),
    }?;
    Ok((size, inside()))
}
