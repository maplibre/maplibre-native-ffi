use std::error::Error as StdError;
#[cfg(not(target_os = "emscripten"))]
use std::ffi::CStr;
use std::ffi::CString;
#[cfg(target_os = "windows")]
use std::ffi::c_char;
#[cfg(any(
    target_os = "linux",
    target_os = "android",
    target_os = "macos",
    target_os = "windows"
))]
use std::ffi::c_void;
use std::sync::atomic::{AtomicUsize, Ordering};
use std::sync::{Arc, Mutex};
use std::time::{Duration, Instant};

#[cfg(not(target_os = "emscripten"))]
use ash::vk;
#[cfg(not(target_os = "emscripten"))]
use ash::vk::Handle;
#[cfg(not(target_os = "emscripten"))]
use glow as gl_api;
#[cfg(not(target_os = "emscripten"))]
use glow::HasContext;
use serde_json::{Value as JsonValue, json};
// Emscripten fixtures bind WebGL directly because glow does not build there.
#[cfg(target_os = "emscripten")]
mod webgl_gl;
#[cfg(any(target_os = "linux", target_os = "android"))]
use glutin_egl_sys::egl;
#[cfg(any(target_os = "linux", target_os = "android"))]
use glutin_egl_sys::egl::types::{EGLConfig, EGLContext, EGLDisplay, EGLSurface, EGLint};
#[cfg(any(target_os = "linux", target_os = "android", target_os = "macos"))]
use libloading::Library;
#[cfg(target_os = "macos")]
use macos_egl as egl;
#[cfg(target_os = "macos")]
use macos_egl::types::{EGLConfig, EGLContext, EGLDisplay, EGLSurface, EGLint};
use static_assertions::{assert_impl_all, assert_not_impl_any};
#[cfg(target_os = "emscripten")]
use webgl_gl as gl_api;

#[cfg(target_os = "macos")]
#[allow(non_snake_case)]
mod macos_egl {
    use std::ffi::{c_char, c_void};

    pub mod types {
        use std::ffi::c_void;

        pub type EGLConfig = *const c_void;
        pub type EGLContext = *const c_void;
        pub type EGLDisplay = *const c_void;
        pub type EGLSurface = *const c_void;
        pub type EGLint = i32;
    }

    use types::{EGLConfig, EGLContext, EGLDisplay, EGLSurface, EGLint};

    pub const FALSE: u32 = 0;
    pub const NONE: u32 = 0x3038;
    pub const WIDTH: u32 = 0x3057;
    pub const HEIGHT: u32 = 0x3056;
    pub const SURFACE_TYPE: u32 = 0x3033;
    pub const PBUFFER_BIT: u32 = 0x0001;
    pub const RENDERABLE_TYPE: u32 = 0x3040;
    pub const OPENGL_ES3_BIT: u32 = 0x0040;
    pub const RED_SIZE: u32 = 0x3024;
    pub const GREEN_SIZE: u32 = 0x3023;
    pub const BLUE_SIZE: u32 = 0x3022;
    pub const ALPHA_SIZE: u32 = 0x3021;
    pub const DEPTH_SIZE: u32 = 0x3025;
    pub const STENCIL_SIZE: u32 = 0x3026;
    pub const CONTEXT_CLIENT_VERSION: u32 = 0x3098;
    pub const OPENGL_ES_API: u32 = 0x30A0;
    pub const DEFAULT_DISPLAY: usize = 0;
    pub const NO_CONTEXT: EGLContext = std::ptr::null();
    pub const NO_DISPLAY: EGLDisplay = std::ptr::null();
    pub const NO_SURFACE: EGLSurface = std::ptr::null();

    type GetDisplay = unsafe extern "system" fn(*mut c_void) -> EGLDisplay;
    type Initialize = unsafe extern "system" fn(EGLDisplay, *mut EGLint, *mut EGLint) -> u32;
    type BindApi = unsafe extern "system" fn(u32) -> u32;
    type ChooseConfig = unsafe extern "system" fn(
        EGLDisplay,
        *const EGLint,
        *mut EGLConfig,
        EGLint,
        *mut EGLint,
    ) -> u32;
    type CreateContext =
        unsafe extern "system" fn(EGLDisplay, EGLConfig, EGLContext, *const EGLint) -> EGLContext;
    type CreatePbufferSurface =
        unsafe extern "system" fn(EGLDisplay, EGLConfig, *const EGLint) -> EGLSurface;
    type MakeCurrent =
        unsafe extern "system" fn(EGLDisplay, EGLSurface, EGLSurface, EGLContext) -> u32;
    type GetError = unsafe extern "system" fn() -> u32;
    type DestroySurface = unsafe extern "system" fn(EGLDisplay, EGLSurface) -> u32;
    type DestroyContext = unsafe extern "system" fn(EGLDisplay, EGLContext) -> u32;
    type Terminate = unsafe extern "system" fn(EGLDisplay) -> u32;
    type GetCurrentContext = unsafe extern "system" fn() -> EGLContext;
    type GetProcAddress = unsafe extern "system" fn(*const c_char) -> *const c_void;

    pub struct Egl {
        GetDisplay: GetDisplay,
        Initialize: Initialize,
        BindAPI: BindApi,
        ChooseConfig: ChooseConfig,
        CreateContext: CreateContext,
        CreatePbufferSurface: CreatePbufferSurface,
        MakeCurrent: MakeCurrent,
        GetError: GetError,
        DestroySurface: DestroySurface,
        DestroyContext: DestroyContext,
        Terminate: Terminate,
        GetCurrentContext: GetCurrentContext,
        GetProcAddress: GetProcAddress,
    }

    impl Egl {
        pub unsafe fn load_with(mut load: impl FnMut(&str) -> *const c_void) -> Self {
            unsafe fn symbol<T: Copy>(pointer: *const c_void, name: &str) -> T {
                assert!(!pointer.is_null(), "EGL loader did not provide {name}");
                // SAFETY: the loader returns the function named by `name`, and
                // every field below declares that function's EGL signature.
                unsafe { std::mem::transmute_copy(&pointer) }
            }

            macro_rules! load {
                ($name:literal) => {
                    unsafe { symbol(load($name), $name) }
                };
            }
            Self {
                GetDisplay: load!("eglGetDisplay"),
                Initialize: load!("eglInitialize"),
                BindAPI: load!("eglBindAPI"),
                ChooseConfig: load!("eglChooseConfig"),
                CreateContext: load!("eglCreateContext"),
                CreatePbufferSurface: load!("eglCreatePbufferSurface"),
                MakeCurrent: load!("eglMakeCurrent"),
                GetError: load!("eglGetError"),
                DestroySurface: load!("eglDestroySurface"),
                DestroyContext: load!("eglDestroyContext"),
                Terminate: load!("eglTerminate"),
                GetCurrentContext: load!("eglGetCurrentContext"),
                GetProcAddress: load!("eglGetProcAddress"),
            }
        }

        pub unsafe fn GetDisplay(&self, display: *mut c_void) -> EGLDisplay {
            unsafe { (self.GetDisplay)(display) }
        }
        pub unsafe fn Initialize(
            &self,
            display: EGLDisplay,
            major: *mut EGLint,
            minor: *mut EGLint,
        ) -> u32 {
            unsafe { (self.Initialize)(display, major, minor) }
        }
        pub unsafe fn BindAPI(&self, api: u32) -> u32 {
            unsafe { (self.BindAPI)(api) }
        }
        pub unsafe fn ChooseConfig(
            &self,
            display: EGLDisplay,
            attributes: *const EGLint,
            config: *mut EGLConfig,
            config_size: EGLint,
            count: *mut EGLint,
        ) -> u32 {
            unsafe { (self.ChooseConfig)(display, attributes, config, config_size, count) }
        }
        pub unsafe fn CreateContext(
            &self,
            display: EGLDisplay,
            config: EGLConfig,
            share: EGLContext,
            attributes: *const EGLint,
        ) -> EGLContext {
            unsafe { (self.CreateContext)(display, config, share, attributes) }
        }
        pub unsafe fn CreatePbufferSurface(
            &self,
            display: EGLDisplay,
            config: EGLConfig,
            attributes: *const EGLint,
        ) -> EGLSurface {
            unsafe { (self.CreatePbufferSurface)(display, config, attributes) }
        }
        pub unsafe fn MakeCurrent(
            &self,
            display: EGLDisplay,
            draw: EGLSurface,
            read: EGLSurface,
            context: EGLContext,
        ) -> u32 {
            unsafe { (self.MakeCurrent)(display, draw, read, context) }
        }
        pub unsafe fn GetError(&self) -> u32 {
            unsafe { (self.GetError)() }
        }
        pub unsafe fn DestroySurface(&self, display: EGLDisplay, surface: EGLSurface) -> u32 {
            unsafe { (self.DestroySurface)(display, surface) }
        }
        pub unsafe fn DestroyContext(&self, display: EGLDisplay, context: EGLContext) -> u32 {
            unsafe { (self.DestroyContext)(display, context) }
        }
        pub unsafe fn Terminate(&self, display: EGLDisplay) -> u32 {
            unsafe { (self.Terminate)(display) }
        }
        pub unsafe fn GetCurrentContext(&self) -> EGLContext {
            unsafe { (self.GetCurrentContext)() }
        }
        pub unsafe fn GetProcAddress(&self, name: *const c_char) -> *const c_void {
            unsafe { (self.GetProcAddress)(name) }
        }
    }
}

use super::*;
use crate::{
    ErrorKind, GeoJsonSourceOptions, MapHandle, MapOptions, OpenGLContextProviderMask,
    RenderBackendMask, RuntimeHandle, ScreenBox, ScreenPoint,
};

assert_not_impl_any!(NativePointer: Send, Sync);
assert_not_impl_any!(FrameNativePointer<'static>: Send, Sync);
assert_not_impl_any!(FrameOpenGLTextureName<'static>: Send, Sync);
assert_impl_all!(RenderSessionHandle: Send, Sync);
assert_impl_all!(AcquiredFrameHandle: Send, Sync);

const FEATURE_STATE_STYLE_JSON: &str = r#"{"version":8,"sources":{"point":{"type":"geojson","data":{"type":"FeatureCollection","features":[{"type":"Feature","id":"feature-1","properties":{},"geometry":{"type":"Point","coordinates":[0,0]}}]}}},"layers":[{"id":"circle","type":"circle","source":"point","paint":{"circle-radius":["case",["boolean",["feature-state","hover"],false],10,5]}}]}"#;
const QUERY_STYLE_JSON: &str = r##"{"version":8,"sources":{"point":{"type":"geojson","data":{"type":"FeatureCollection","features":[{"type":"Feature","id":"feature-1","geometry":{"type":"Point","coordinates":[-122.4194,37.7749]},"properties":{"kind":"capital","visible":true}}]}}},"layers":[{"id":"background","type":"background","paint":{"background-color":"#d8f1ff"}},{"id":"point-circle","type":"circle","source":"point","paint":{"circle-color":"#f97316","circle-radius":12}}]}"##;
#[cfg(mln_webgpu_backend)]
const QUERY_STYLE_BACKGROUND_RGBA: [u8; 4] = [0xd8, 0xf1, 0xff, 0xff];
const CLUSTER_BASE_STYLE_JSON: &str = r##"{"version":8,"sources":{},"layers":[{"id":"background","type":"background","paint":{"background-color":"#ffffff"}}]}"##;
fn wait_until_completed<T>(session: &RenderSessionHandle, operation: &NativeFuture<T>) {
    let deadline = Instant::now() + Duration::from_secs(5);
    while !operation.is_completed().unwrap() {
        let _ = session.service_driver_work(64);
        assert!(Instant::now() < deadline, "render operation timed out");
        std::thread::yield_now();
    }
    maplibre_core::check(operation.terminal_status().unwrap()).unwrap();
}

fn wait_for_operation(session: &RenderSessionHandle, operation: &NativeFuture<()>) {
    wait_until_completed(session, operation);
    operation.finish().unwrap();
}

fn finish_attachment(attachment: RenderSessionAttachment) -> Result<RenderSessionHandle> {
    wait_for_operation(&attachment.session, &attachment.completion);
    Ok(attachment.session)
}

fn finish_unit(session: &RenderSessionHandle, operation: NativeFuture<()>) {
    wait_for_operation(session, &operation);
}

fn release_frame(frame: AcquiredFrameHandle) {
    match frame.release(GpuSync::CPU_COMPLETE) {
        Ok(()) => {}
        Err(error) => panic!("failed to release acquired frame: {}", error.error()),
    }
}

fn caller_attach_options() -> RenderSessionAttachOptions {
    RenderSessionAttachOptions::caller_graphics_thread(2)
}

fn create_owned_texture_session(
    map: &MapHandle,
    extent: RenderTargetExtent,
) -> std::result::Result<(OwnedTextureTestContext, RenderSessionHandle), Box<dyn StdError>> {
    let backends = crate::supported_render_backends();
    if backends.contains(RenderBackendMask::METAL) {
        let context = MetalTestContext::new()?;
        let session = finish_attachment(map.attach_metal_owned_texture(
            &MetalOwnedTextureDescriptor::new(extent, context.descriptor()),
            RenderSessionAttachOptions::core_worker(2),
        )?)?;
        return Ok((OwnedTextureTestContext::Metal(context), session));
    }
    #[cfg(mln_webgpu_backend)]
    if backends.contains(RenderBackendMask::WEBGPU) {
        let context = WebGpuTestContext::new()?;
        let session = finish_attachment(map.attach_webgpu_owned_texture(
            &WebGpuOwnedTextureDescriptor::new(extent, context.descriptor()),
            caller_attach_options(),
        )?)?;
        return Ok((OwnedTextureTestContext::WebGpu(context), session));
    }
    #[cfg(not(target_os = "emscripten"))]
    if backends.contains(RenderBackendMask::VULKAN) {
        let context = VulkanTestContext::new()?;
        let session = finish_attachment(map.attach_vulkan_owned_texture(
            &VulkanOwnedTextureDescriptor::new(extent, context.descriptor()),
            RenderSessionAttachOptions::core_worker(2),
        )?)?;
        return Ok((OwnedTextureTestContext::Vulkan(Box::new(context)), session));
    }
    if has_opengl_test_context_backend() {
        let context = OpenGLTestContext::new(extent.width, extent.height)?;
        let session = finish_attachment(map.attach_opengl_owned_texture(
            &OpenGLOwnedTextureDescriptor::new(extent, context.descriptor()),
            caller_attach_options(),
        )?)?;
        return Ok((OwnedTextureTestContext::OpenGL(Box::new(context)), session));
    }
    Err("no configured render backend offers an owned texture test session".into())
}

/// Whether this build can attach an owned texture session the fixture can
/// build a context for.
fn has_test_owned_texture_session_backend() -> bool {
    let backends = crate::supported_render_backends();
    let webgpu = cfg!(mln_webgpu_backend) && backends.contains(RenderBackendMask::WEBGPU);
    backends.intersects(RenderBackendMask::METAL | RenderBackendMask::VULKAN)
        || webgpu
        || has_opengl_test_context_backend()
}

fn has_opengl_backend() -> bool {
    crate::supported_render_backends().contains(RenderBackendMask::OPENGL)
}

fn has_opengl_test_context_backend() -> bool {
    if !has_opengl_backend() {
        return false;
    }

    #[cfg(any(target_os = "linux", target_os = "android", target_os = "macos"))]
    {
        crate::supported_opengl_context_providers().contains(OpenGLContextProviderMask::EGL)
    }
    #[cfg(target_os = "windows")]
    {
        crate::supported_opengl_context_providers().contains(OpenGLContextProviderMask::WGL)
    }
    #[cfg(target_os = "emscripten")]
    {
        crate::supported_opengl_context_providers().contains(OpenGLContextProviderMask::WEBGL)
    }
    #[cfg(not(any(
        target_os = "linux",
        target_os = "android",
        target_os = "macos",
        target_os = "windows",
        target_os = "emscripten"
    )))]
    {
        // The Rust test helper implements desktop and Android EGL, Windows WGL,
        // and browser WebGL.
        false
    }
}

fn create_opengl_owned_texture_session(
    map: &MapHandle,
    extent: RenderTargetExtent,
) -> std::result::Result<(OpenGLTestContext, RenderSessionHandle), Box<dyn StdError>> {
    let backends = crate::supported_render_backends();
    if !backends.contains(RenderBackendMask::OPENGL) {
        return Err("native library does not support OpenGL owned texture sessions".into());
    }
    let context = OpenGLTestContext::new(extent.width, extent.height)?;
    let session = finish_attachment(map.attach_opengl_owned_texture(
        &OpenGLOwnedTextureDescriptor::new(extent, context.descriptor()),
        caller_attach_options(),
    )?)?;
    Ok((context, session))
}

fn create_opengl_surface_session(
    map: &MapHandle,
    extent: RenderTargetExtent,
) -> std::result::Result<(OpenGLTestContext, RenderSessionHandle), Box<dyn StdError>> {
    let backends = crate::supported_render_backends();
    if !backends.contains(RenderBackendMask::OPENGL) {
        return Err("native library does not support OpenGL surface sessions".into());
    }
    let context = OpenGLTestContext::new(extent.width, extent.height)?;
    let session = finish_attachment(map.attach_opengl_surface(
        &OpenGLSurfaceDescriptor::new(extent, context.descriptor(), context.surface()),
        caller_attach_options(),
    )?)?;
    Ok((context, session))
}

fn create_opengl_borrowed_texture_session(
    map: &MapHandle,
    extent: RenderTargetExtent,
) -> std::result::Result<(OpenGLBorrowedTexture, RenderSessionHandle), Box<dyn StdError>> {
    let backends = crate::supported_render_backends();
    if !backends.contains(RenderBackendMask::OPENGL) {
        return Err("native library does not support OpenGL borrowed texture sessions".into());
    }
    let (physical_width, physical_height) = extent.physical_size()?;
    let texture = OpenGLBorrowedTexture::new(physical_width, physical_height)?;
    let session = finish_attachment(map.attach_opengl_borrowed_texture(
        &OpenGLBorrowedTextureDescriptor::new(
            extent,
            physical_width,
            physical_height,
            texture.descriptor(),
            texture.name(),
            gl_api::TEXTURE_2D,
        ),
        caller_attach_options(),
    )?)?;
    Ok((texture, session))
}

/// Attaches a WebGPU caller-owned texture session, the way a browser host that
/// allocates its own render target does.
#[cfg(mln_webgpu_backend)]
fn create_webgpu_borrowed_texture_session(
    map: &MapHandle,
    extent: RenderTargetExtent,
) -> std::result::Result<
    (
        WebGpuTestContext,
        WebGpuBorrowedTexture,
        RenderSessionHandle,
    ),
    Box<dyn StdError>,
> {
    let context = WebGpuTestContext::new()?;
    let (physical_width, physical_height) = extent.physical_size()?;
    let texture = WebGpuBorrowedTexture::new(&context, physical_width, physical_height)?;
    let session = finish_attachment(map.attach_webgpu_borrowed_texture(
        &texture.descriptor(extent, &context),
        caller_attach_options(),
    )?)?;
    Ok((context, texture, session))
}

#[allow(dead_code)]
enum OwnedTextureTestContext {
    Metal(MetalTestContext),
    // A browser build has no Vulkan.
    #[cfg(not(target_os = "emscripten"))]
    Vulkan(Box<VulkanTestContext>),
    #[cfg(mln_webgpu_backend)]
    WebGpu(WebGpuTestContext),
    // Boxed: the context is several kilobytes and the other variants are small.
    OpenGL(Box<OpenGLTestContext>),
}

impl OwnedTextureTestContext {
    fn attach_owned_texture(
        &self,
        map: &MapHandle,
        extent: RenderTargetExtent,
    ) -> Result<RenderSessionHandle> {
        let attachment = match self {
            Self::Metal(context) => map.attach_metal_owned_texture(
                &MetalOwnedTextureDescriptor::new(extent, context.descriptor()),
                RenderSessionAttachOptions::core_worker(2),
            ),
            #[cfg(not(target_os = "emscripten"))]
            Self::Vulkan(context) => map.attach_vulkan_owned_texture(
                &VulkanOwnedTextureDescriptor::new(extent, context.descriptor()),
                RenderSessionAttachOptions::core_worker(2),
            ),
            #[cfg(mln_webgpu_backend)]
            Self::WebGpu(context) => map.attach_webgpu_owned_texture(
                &WebGpuOwnedTextureDescriptor::new(extent, context.descriptor()),
                caller_attach_options(),
            ),
            Self::OpenGL(context) => map.attach_opengl_owned_texture(
                &OpenGLOwnedTextureDescriptor::new(extent, context.descriptor()),
                caller_attach_options(),
            ),
        }?;
        finish_attachment(attachment)
    }

    /// Hands the session a placeholder target of this context's own backend.
    /// The setter must match the backend, or an unsupported build answers in
    /// place of the target-kind rejection. The placeholder is never
    /// dereferenced, because the kind is checked before the descriptor.
    fn set_placeholder_borrowed_target(
        &self,
        session: &RenderSessionHandle,
        extent: &RenderTargetExtent,
    ) -> Result<NativeFuture<()>> {
        // SAFETY: Test passes an opaque non-null address that the rejected call
        // never dereferences.
        let placeholder = unsafe { NativePointer::from_address(0x1) };
        match self {
            Self::Metal(_) => session.set_metal_borrowed_texture_target(
                &MetalBorrowedTextureDescriptor::new(extent.clone(), 64, 64, placeholder),
            ),
            #[cfg(mln_webgpu_backend)]
            Self::WebGpu(context) => {
                // A placeholder handle is never dereferenced, but the setter has
                // to match the backend or an unsupported build would answer
                // instead of the session kind.
                session.set_webgpu_borrowed_texture_target(&WebGpuBorrowedTextureDescriptor::new(
                    extent.clone(),
                    64,
                    64,
                    context.descriptor(),
                    placeholder,
                    placeholder,
                    0,
                ))
            }
            #[cfg(not(target_os = "emscripten"))]
            Self::Vulkan(context) => {
                session.set_vulkan_borrowed_texture_target(&VulkanBorrowedTextureDescriptor::new(
                    extent.clone(),
                    64,
                    64,
                    context.descriptor(),
                    placeholder,
                    placeholder,
                    1,
                    0,
                    1,
                ))
            }
            Self::OpenGL(context) => {
                session.set_opengl_borrowed_texture_target(&OpenGLBorrowedTextureDescriptor::new(
                    extent.clone(),
                    64,
                    64,
                    context.descriptor(),
                    1,
                    gl_api::TEXTURE_2D,
                ))
            }
        }
    }

    fn try_acquire_frame_extent(
        &self,
        session: &RenderSessionHandle,
        expected: &RenderTargetExtent,
    ) -> bool {
        match self {
            Self::Metal(_) => {
                let Ok(frame) = session.acquire_frame() else {
                    return false;
                };
                let Ok((metadata, _, _)) = frame.metal_texture() else {
                    return false;
                };
                let matches = (metadata.width, metadata.height)
                    == (expected.width, expected.height)
                    && metadata.scale_factor == expected.scale_factor;
                release_frame(frame);
                matches
            }
            #[cfg(mln_webgpu_backend)]
            Self::WebGpu(_) => {
                let Ok(frame) = session.acquire_frame() else {
                    return false;
                };
                let Ok((metadata, _, _, _)) = frame.webgpu_texture() else {
                    return false;
                };
                let matches = metadata.width == expected.width
                    && metadata.height == expected.height
                    && metadata.scale_factor == expected.scale_factor;
                release_frame(frame);
                matches
            }
            #[cfg(not(target_os = "emscripten"))]
            Self::Vulkan(_) => {
                let Ok(frame) = session.acquire_frame() else {
                    return false;
                };
                let Ok((metadata, _, _, _)) = frame.vulkan_texture() else {
                    return false;
                };
                let matches = (metadata.width, metadata.height)
                    == (expected.width, expected.height)
                    && metadata.scale_factor == expected.scale_factor;
                release_frame(frame);
                matches
            }
            Self::OpenGL(_) => {
                let Ok(frame) = session.acquire_frame() else {
                    return false;
                };
                let Ok((metadata, _)) = frame.opengl_texture() else {
                    return false;
                };
                let matches = (metadata.width, metadata.height)
                    == (expected.width, expected.height)
                    && metadata.scale_factor == expected.scale_factor;
                release_frame(frame);
                matches
            }
        }
    }
}

#[cfg(target_os = "macos")]
#[link(name = "Metal", kind = "framework")]
unsafe extern "C" {
    fn MTLCreateSystemDefaultDevice() -> *mut std::ffi::c_void;
}

struct MetalTestContext {
    device: NativePointer,
}

impl MetalTestContext {
    fn new() -> std::result::Result<Self, Box<dyn StdError>> {
        #[cfg(target_os = "macos")]
        {
            // SAFETY: This calls the system Metal factory and stores the opaque
            // device pointer without dereferencing it in Rust.
            let device = unsafe { MTLCreateSystemDefaultDevice() };
            if device.is_null() {
                return Err("Metal did not return a default device".into());
            }
            Ok(Self {
                // SAFETY: The Metal device remains live for the test context lifetime.
                device: unsafe { NativePointer::from_ptr(device) },
            })
        }

        #[cfg(not(target_os = "macos"))]
        {
            Err("Metal test context is only available on macOS".into())
        }
    }

    fn descriptor(&self) -> MetalContextDescriptor {
        MetalContextDescriptor::new(self.device)
    }
}

struct OpenGLTestContext {
    descriptor: OpenGLContextDescriptor,
    surface_handle: NativePointer,
    gl: gl_api::Context,
    #[cfg(any(target_os = "linux", target_os = "android", target_os = "macos"))]
    platform: EglTestContext,
    #[cfg(target_os = "windows")]
    platform: WglTestContext,
    #[cfg(target_os = "emscripten")]
    platform: WebGlTestContext,
}

#[cfg(any(target_os = "linux", target_os = "android", target_os = "macos"))]
struct EglTestContext {
    egl: egl::Egl,
    _lib: Library,
    display: EGLDisplay,
    config: EGLConfig,
    surface: EGLSurface,
    context: EGLContext,
}

#[cfg(any(target_os = "linux", target_os = "android", target_os = "macos"))]
impl EglTestContext {
    fn new(width: u32, height: u32) -> std::result::Result<Self, Box<dyn StdError>> {
        let lib = load_egl_library()?;
        let egl = load_egl_bindings(&lib)?;
        let display = open_egl_display(&egl)?;

        if unsafe { egl.BindAPI(egl::OPENGL_ES_API) } == egl::FALSE {
            let error = unsafe { egl.GetError() };
            unsafe {
                egl.Terminate(display);
            }
            return Err(format!("eglBindAPI failed with 0x{error:x}").into());
        }

        let config = match choose_egl_pbuffer_config(&egl, display) {
            Ok(config) => config,
            Err(error) => {
                unsafe { egl.Terminate(display) };
                return Err(error);
            }
        };

        let context_attributes = [
            egl::CONTEXT_CLIENT_VERSION as EGLint,
            3,
            egl::NONE as EGLint,
        ];
        let context = unsafe {
            egl.CreateContext(
                display,
                config,
                egl::NO_CONTEXT,
                context_attributes.as_ptr(),
            )
        };
        if context == egl::NO_CONTEXT {
            let error = unsafe { egl.GetError() };
            unsafe {
                egl.Terminate(display);
            }
            return Err(format!("eglCreateContext failed with 0x{error:x}").into());
        }

        let surface_attributes = [
            egl::WIDTH as EGLint,
            width as EGLint,
            egl::HEIGHT as EGLint,
            height as EGLint,
            egl::NONE as EGLint,
        ];
        let surface =
            unsafe { egl.CreatePbufferSurface(display, config, surface_attributes.as_ptr()) };
        if surface == egl::NO_SURFACE {
            let error = unsafe { egl.GetError() };
            unsafe {
                egl.DestroyContext(display, context);
                egl.Terminate(display);
            }
            return Err(format!("eglCreatePbufferSurface failed with 0x{error:x}").into());
        }

        if unsafe { egl.MakeCurrent(display, surface, surface, context) } == egl::FALSE {
            let error = unsafe { egl.GetError() };
            unsafe {
                egl.DestroySurface(display, surface);
                egl.DestroyContext(display, context);
                egl.Terminate(display);
            }
            return Err(format!("eglMakeCurrent failed with 0x{error:x}").into());
        }

        Ok(Self {
            egl,
            _lib: lib,
            display,
            config,
            surface,
            context,
        })
    }

    fn descriptor(&self) -> OpenGLContextDescriptor {
        OpenGLContextDescriptor::Egl(EglContextDescriptor::new(
            unsafe { NativePointer::from_ptr(self.display.cast_mut()) },
            unsafe { NativePointer::from_ptr(self.config.cast_mut()) },
            unsafe { NativePointer::from_ptr(self.context.cast_mut()) },
        ))
    }

    fn surface(&self) -> NativePointer {
        unsafe { NativePointer::from_ptr(self.surface.cast_mut()) }
    }

    fn make_current(&self) -> std::result::Result<(), Box<dyn StdError>> {
        if unsafe {
            self.egl
                .MakeCurrent(self.display, self.surface, self.surface, self.context)
        } == egl::FALSE
        {
            Err(format!("eglMakeCurrent failed with 0x{:x}", unsafe {
                self.egl.GetError()
            })
            .into())
        } else {
            Ok(())
        }
    }

    fn get_proc_address(&self, symbol: &CStr) -> *const c_void {
        unsafe { self.egl.GetProcAddress(symbol.as_ptr().cast()).cast() }
    }
}

#[cfg(any(target_os = "linux", target_os = "android", target_os = "macos"))]
impl Drop for EglTestContext {
    fn drop(&mut self) {
        unsafe {
            self.egl.MakeCurrent(
                self.display,
                egl::NO_SURFACE,
                egl::NO_SURFACE,
                egl::NO_CONTEXT,
            );
            self.egl.DestroySurface(self.display, self.surface);
            self.egl.DestroyContext(self.display, self.context);
            self.egl.Terminate(self.display);
        }
    }
}

/// EGL display and pbuffer surface for a dedicated-ownership session.
///
/// This fixture creates no context and makes none current: naming dedicated
/// ownership is what asks the session to create its own context and keep it
/// current between renders.
#[cfg(any(target_os = "linux", target_os = "android", target_os = "macos"))]
struct DedicatedEglTestSurface {
    egl: egl::Egl,
    _lib: Library,
    display: EGLDisplay,
    config: EGLConfig,
    surface: EGLSurface,
}

#[cfg(any(target_os = "linux", target_os = "android", target_os = "macos"))]
impl DedicatedEglTestSurface {
    fn new(width: u32, height: u32) -> std::result::Result<Self, Box<dyn StdError>> {
        let lib = load_egl_library()?;
        let egl = load_egl_bindings(&lib)?;
        let display = open_egl_display(&egl)?;
        let config = match choose_egl_pbuffer_config(&egl, display) {
            Ok(config) => config,
            Err(error) => {
                unsafe { egl.Terminate(display) };
                return Err(error);
            }
        };

        let surface_attributes = [
            egl::WIDTH as EGLint,
            width as EGLint,
            egl::HEIGHT as EGLint,
            height as EGLint,
            egl::NONE as EGLint,
        ];
        let surface =
            unsafe { egl.CreatePbufferSurface(display, config, surface_attributes.as_ptr()) };
        if surface == egl::NO_SURFACE {
            let error = unsafe { egl.GetError() };
            unsafe { egl.Terminate(display) };
            return Err(format!("eglCreatePbufferSurface failed with 0x{error:x}").into());
        }

        Ok(Self {
            egl,
            _lib: lib,
            display,
            config,
            surface,
        })
    }

    fn descriptor(&self) -> OpenGLContextDescriptor {
        let mut context = EglContextDescriptor::new(
            unsafe { NativePointer::from_ptr(self.display.cast_mut()) },
            unsafe { NativePointer::from_ptr(self.config.cast_mut()) },
            NativePointer::NULL,
        );
        context.client_api = OpenGLClientApi::Gles;
        context.ownership = OpenGLContextOwnership::Dedicated;
        OpenGLContextDescriptor::Egl(context)
    }

    fn surface(&self) -> NativePointer {
        unsafe { NativePointer::from_ptr(self.surface.cast_mut()) }
    }

    /// Whether an EGL context is current on this thread.
    fn has_current_context(&self) -> bool {
        unsafe { self.egl.GetCurrentContext() != egl::NO_CONTEXT }
    }
}

#[cfg(any(target_os = "linux", target_os = "android", target_os = "macos"))]
impl Drop for DedicatedEglTestSurface {
    fn drop(&mut self) {
        unsafe {
            self.egl.DestroySurface(self.display, self.surface);
            self.egl.Terminate(self.display);
        }
    }
}

/// Opens and initializes the EGL display the OpenGL fixtures render through.
#[cfg(any(target_os = "linux", target_os = "android", target_os = "macos"))]
fn open_egl_display(egl: &egl::Egl) -> std::result::Result<EGLDisplay, Box<dyn StdError>> {
    // A desktop Linux run without a display server needs Mesa's surfaceless
    // platform. Android and macOS ANGLE take their default displays.
    #[cfg(any(target_env = "ohos", target_os = "android", target_os = "macos"))]
    let display = unsafe { egl.GetDisplay(egl::DEFAULT_DISPLAY as *mut c_void) };
    #[cfg(any(target_env = "ohos", target_os = "android", target_os = "macos"))]
    let display_operation = "eglGetDisplay";
    #[cfg(not(any(target_env = "ohos", target_os = "android", target_os = "macos")))]
    let display = {
        const EGL_PLATFORM_SURFACELESS_MESA: u32 = 0x31DD;
        if !egl.GetPlatformDisplayEXT.is_loaded() {
            return Err("eglGetPlatformDisplayEXT is unavailable".into());
        }
        unsafe {
            egl.GetPlatformDisplayEXT(
                EGL_PLATFORM_SURFACELESS_MESA,
                egl::DEFAULT_DISPLAY as *mut c_void,
                [egl::NONE as EGLint].as_ptr(),
            )
        }
    };
    #[cfg(not(any(target_env = "ohos", target_os = "android", target_os = "macos")))]
    let display_operation = "eglGetPlatformDisplayEXT";
    if display == egl::NO_DISPLAY {
        return Err(format!("{display_operation} failed with 0x{:x}", unsafe {
            egl.GetError()
        })
        .into());
    }

    let mut major = 0;
    let mut minor = 0;
    if unsafe { egl.Initialize(display, &mut major, &mut minor) } == egl::FALSE {
        return Err(format!("eglInitialize failed with 0x{:x}", unsafe {
            egl.GetError()
        })
        .into());
    }
    Ok(display)
}

/// Chooses a pbuffer-capable OpenGL ES 3 config, which the C API requires of
/// every EGL context descriptor.
#[cfg(any(target_os = "linux", target_os = "android", target_os = "macos"))]
fn choose_egl_pbuffer_config(
    egl: &egl::Egl,
    display: EGLDisplay,
) -> std::result::Result<EGLConfig, Box<dyn StdError>> {
    let config_attributes = [
        egl::SURFACE_TYPE as EGLint,
        egl::PBUFFER_BIT as EGLint,
        egl::RENDERABLE_TYPE as EGLint,
        egl::OPENGL_ES3_BIT as EGLint,
        egl::RED_SIZE as EGLint,
        8,
        egl::GREEN_SIZE as EGLint,
        8,
        egl::BLUE_SIZE as EGLint,
        8,
        egl::ALPHA_SIZE as EGLint,
        8,
        egl::DEPTH_SIZE as EGLint,
        24,
        egl::STENCIL_SIZE as EGLint,
        8,
        egl::NONE as EGLint,
    ];
    let mut config: EGLConfig = std::ptr::null_mut();
    let mut config_count = 0;
    if unsafe {
        egl.ChooseConfig(
            display,
            config_attributes.as_ptr(),
            &mut config,
            1,
            &mut config_count,
        )
    } == egl::FALSE
        || config_count == 0
        || config.is_null()
    {
        return Err(format!("eglChooseConfig failed with 0x{:x}", unsafe {
            egl.GetError()
        })
        .into());
    }
    Ok(config)
}

#[cfg(any(target_os = "linux", target_os = "android", target_os = "macos"))]
fn load_egl_library() -> std::result::Result<Library, Box<dyn StdError>> {
    #[cfg(target_os = "macos")]
    let library = unsafe { Library::new("libEGL.dylib") };
    #[cfg(not(target_os = "macos"))]
    let library = unsafe { Library::new("libEGL.so.1") }
        .or_else(|_| unsafe { Library::new("libEGL.so") })
        .map_err(|error| format!("failed to load libEGL: {error}"));
    library.map_err(|error| format!("failed to load libEGL: {error}").into())
}

#[cfg(any(target_os = "linux", target_os = "android", target_os = "macos"))]
fn load_egl_bindings(lib: &Library) -> std::result::Result<egl::Egl, Box<dyn StdError>> {
    type EglGetProcAddress = unsafe extern "system" fn(*const c_void) -> *const c_void;

    let get_proc_address: libloading::Symbol<'_, EglGetProcAddress> =
        unsafe { lib.get(b"eglGetProcAddress\0")? };
    let egl = unsafe {
        egl::Egl::load_with(|symbol| {
            let name = CString::new(symbol).expect("EGL symbol names do not contain NULs");
            if let Ok(loaded) = lib.get::<*const c_void>(name.as_bytes_with_nul()) {
                *loaded
            } else {
                get_proc_address(name.as_ptr().cast())
            }
        })
    };
    Ok(egl)
}

/// The WebGPU header the module links, bound by this crate's build script.
///
/// Generated rather than taken from a crate: the fixtures hand their device to
/// a session as an opaque handle, so it has to come from the very emdawnwebgpu
/// instance the core links.
#[cfg(mln_webgpu_backend)]
#[allow(
    non_snake_case,
    non_camel_case_types,
    non_upper_case_globals,
    dead_code
)]
mod webgpu_sys {
    include!(concat!(env!("OUT_DIR"), "/webgpu.rs"));
}

#[cfg(mln_webgpu_backend)]
unsafe extern "C" {
    /// Suspends and resumes through a task, which is what lets the JavaScript
    /// job queue run. An ordinary sleep blocks the thread with `Atomics.wait`
    /// and would never let it run at all.
    fn emscripten_sleep(milliseconds: u32);
}

/// A WebGPU device of the fixture's own, standing in for a browser host's.
///
/// One per fixture rather than one per thread: a device outliving its fixture
/// holds a runtime keepalive, and libtest offers no hook to end one afterwards.
/// A device also belongs to the thread that created it, because emdawnwebgpu
/// keeps WebGPU objects in the JS realm of that worker, and the suite attaches
/// sessions from more than one thread.
#[cfg(mln_webgpu_backend)]
struct WebGpuTestContext {
    instance: webgpu_sys::WGPUInstance,
    adapter: webgpu_sys::WGPUAdapter,
    device: webgpu_sys::WGPUDevice,
}

/// A slot an asynchronous WebGPU callback writes into, owned by the callback.
///
/// A wait here is bounded, and WebGPU delivers the callback whether or not the
/// wait is still listening. A slot on the waiting stack, or inside the fixture
/// the wait gives up on, would be written through after it is gone, so the
/// callback holds a reference of its own and drops it when it fires.
#[cfg(mln_webgpu_backend)]
type CallbackSlot<T> = std::rc::Rc<std::cell::Cell<Option<T>>>;

#[cfg(mln_webgpu_backend)]
fn callback_slot<T>() -> (CallbackSlot<T>, *mut std::ffi::c_void) {
    let slot: CallbackSlot<T> = std::rc::Rc::new(std::cell::Cell::new(None));
    (slot.clone(), std::rc::Rc::into_raw(slot).cast_mut().cast())
}

/// Stores a callback's result and releases the reference it was given.
///
/// # Safety
///
/// `user_data` is the pointer [`callback_slot`] produced for this one callback,
/// which WebGPU delivers exactly once, and `T` is the type that slot was made
/// with. The pointer carries no type of its own, so each caller names it.
#[cfg(mln_webgpu_backend)]
unsafe fn fill_callback_slot<T>(user_data: *mut std::ffi::c_void, value: T) {
    // SAFETY: the caller guarantees this pointer came from callback_slot and
    // reaches here once, so this takes back that one reference.
    let slot = unsafe {
        std::rc::Rc::from_raw(user_data.cast_const().cast::<std::cell::Cell<Option<T>>>())
    };
    slot.set(Some(value));
}

/// Borrows a null-terminated string as the view WebGPU takes.
///
/// # Safety
///
/// The returned view points into `value`, which outlives every call it is
/// passed to.
#[cfg(mln_webgpu_backend)]
fn string_view(value: &CString) -> webgpu_sys::WGPUStringView {
    webgpu_sys::WGPUStringView {
        data: value.as_ptr(),
        length: value.as_bytes().len(),
    }
}

#[cfg(mln_webgpu_backend)]
unsafe extern "C" fn on_adapter(
    status: webgpu_sys::WGPURequestAdapterStatus,
    adapter: webgpu_sys::WGPUAdapter,
    _message: webgpu_sys::WGPUStringView,
    user_data: *mut std::ffi::c_void,
    _reserved: *mut std::ffi::c_void,
) {
    let adapter = if status == webgpu_sys::WGPURequestAdapterStatus_Success {
        adapter
    } else {
        std::ptr::null_mut()
    };
    // SAFETY: user_data is this request's adapter slot, delivered once.
    unsafe { fill_callback_slot::<webgpu_sys::WGPUAdapter>(user_data, adapter) };
}

#[cfg(mln_webgpu_backend)]
unsafe extern "C" fn on_device(
    status: webgpu_sys::WGPURequestDeviceStatus,
    device: webgpu_sys::WGPUDevice,
    _message: webgpu_sys::WGPUStringView,
    user_data: *mut std::ffi::c_void,
    _reserved: *mut std::ffi::c_void,
) {
    let device = if status == webgpu_sys::WGPURequestDeviceStatus_Success {
        device
    } else {
        std::ptr::null_mut()
    };
    // SAFETY: user_data is this request's device slot, delivered once.
    unsafe { fill_callback_slot::<webgpu_sys::WGPUDevice>(user_data, device) };
}

/// Waits for one of WebGPU's futures.
///
/// Adapter and device requests are asynchronous. The fixtures run on a worker
/// where waiting is legal, so this blocks rather than unwinding the test into a
/// callback. Bounded, so a browser without a WebGPU adapter fails the fixture
/// rather than hanging the suite until the runner's timeout.
///
/// A non-zero timeout is only legal on an instance that asked for timed waits;
/// see [`WebGpuTestContext::new`].
#[cfg(mln_webgpu_backend)]
fn await_future(
    instance: webgpu_sys::WGPUInstance,
    future: webgpu_sys::WGPUFuture,
) -> std::result::Result<(), Box<dyn StdError>> {
    const TIMEOUT_NS: u64 = 5 * 1000 * 1000 * 1000;
    let mut wait = webgpu_sys::WGPUFutureWaitInfo {
        future,
        completed: 0,
    };
    // SAFETY: instance is live and wait points at one writable entry.
    let status = unsafe { webgpu_sys::wgpuInstanceWaitAny(instance, 1, &mut wait, TIMEOUT_NS) };
    if status != webgpu_sys::WGPUWaitStatus_Success || wait.completed == 0 {
        return Err(format!(
            "waiting on a WebGPU future failed (status {status}, completed {})",
            wait.completed
        )
        .into());
    }
    Ok(())
}

#[cfg(mln_webgpu_backend)]
impl WebGpuTestContext {
    fn new() -> std::result::Result<Self, Box<dyn StdError>> {
        // Waiting on a future with a timeout has to be asked for up front, or
        // wgpuInstanceWaitAny rejects every non-zero timeout instead of waiting.
        // The capability needs Asyncify or JSPI, which the emdawnwebgpu port
        // enables, and wgpuCreateInstance answers null when asked without it.
        let mut descriptor: webgpu_sys::WGPUInstanceDescriptor = unsafe { std::mem::zeroed() };
        descriptor.capabilities.timedWaitAnyEnable = 1;
        // SAFETY: descriptor is live for the call.
        let instance = unsafe { webgpu_sys::wgpuCreateInstance(&descriptor) };
        if instance.is_null() {
            return Err("creating a WebGPU instance with timed waits enabled failed".into());
        }
        let context = Self {
            instance,
            adapter: std::ptr::null_mut(),
            device: std::ptr::null_mut(),
        };
        context.request_adapter_and_device()
    }

    fn request_adapter_and_device(mut self) -> std::result::Result<Self, Box<dyn StdError>> {
        let options: webgpu_sys::WGPURequestAdapterOptions = unsafe { std::mem::zeroed() };
        let (adapter_slot, adapter_user_data) = callback_slot::<webgpu_sys::WGPUAdapter>();
        let mut adapter_info: webgpu_sys::WGPURequestAdapterCallbackInfo =
            unsafe { std::mem::zeroed() };
        adapter_info.mode = webgpu_sys::WGPUCallbackMode_AllowProcessEvents;
        adapter_info.callback = Some(on_adapter);
        adapter_info.userdata1 = adapter_user_data;
        // SAFETY: instance is live, options outlives the call, and the slot
        // outlives the request whatever the wait below does.
        let future = unsafe {
            webgpu_sys::wgpuInstanceRequestAdapter(self.instance, &options, adapter_info)
        };
        await_future(self.instance, future)?;
        self.adapter = adapter_slot
            .take()
            .filter(|adapter| !adapter.is_null())
            .ok_or("this browser provided no WebGPU adapter")?;

        let device_descriptor: webgpu_sys::WGPUDeviceDescriptor = unsafe { std::mem::zeroed() };
        let (device_slot, device_user_data) = callback_slot::<webgpu_sys::WGPUDevice>();
        let mut device_info: webgpu_sys::WGPURequestDeviceCallbackInfo =
            unsafe { std::mem::zeroed() };
        device_info.mode = webgpu_sys::WGPUCallbackMode_AllowProcessEvents;
        device_info.callback = Some(on_device);
        device_info.userdata1 = device_user_data;
        // SAFETY: adapter is live, the descriptor outlives the call, and the
        // slot outlives the request whatever the wait below does.
        let future = unsafe {
            webgpu_sys::wgpuAdapterRequestDevice(self.adapter, &device_descriptor, device_info)
        };
        await_future(self.instance, future)?;
        self.device = device_slot
            .take()
            .filter(|device| !device.is_null())
            .ok_or("this browser provided no WebGPU device")?;
        Ok(self)
    }

    fn descriptor(&self) -> WebGpuContextDescriptor {
        // SAFETY: these handles are live until this value drops, and the
        // session borrows them for no longer.
        unsafe {
            let mut descriptor = WebGpuContextDescriptor::new(NativePointer::from_ptr(self.device));
            descriptor.instance = NativePointer::from_ptr(self.instance);
            // Null asks the session for the device's default queue, which is
            // what a browser host without a queue of its own hands over.
            descriptor.queue = NativePointer::NULL;
            descriptor
        }
    }
}

#[cfg(mln_webgpu_backend)]
impl Drop for WebGpuTestContext {
    /// Destroys rather than only releasing the device: emdawnwebgpu takes a
    /// runtime keepalive per device and returns it when `device.lost` settles,
    /// which destroying is what resolves. Releasing alone leaves the keepalive
    /// standing.
    ///
    /// The yields are what let emdawnwebgpu run the continuations the destroy
    /// queued; without them the next device request on this thread never
    /// resolves. They are bounded and not an assertion: a test's map outlives
    /// the fixture that lent it this device, so the last keepalive of a run
    /// comes back later, once that map is gone. The entry point forces the exit
    /// for that one; see bindings/rust/emscripten_proxy_main.c.
    fn drop(&mut self) {
        // SAFETY: these handles are this value's and are released exactly once.
        unsafe {
            if !self.device.is_null() {
                webgpu_sys::wgpuDeviceDestroy(self.device);
                webgpu_sys::wgpuDeviceRelease(self.device);
            }
            if !self.adapter.is_null() {
                webgpu_sys::wgpuAdapterRelease(self.adapter);
            }
            if !self.instance.is_null() {
                webgpu_sys::wgpuInstanceRelease(self.instance);
            }
            for _ in 0..100 {
                emscripten_sleep(1);
            }
        }
    }
}

/// A WebGPU surface over a canvas, the way a browser host presents.
///
/// The canvas is the fixture's own OffscreenCanvas, registered by the same JS
/// support the WebGL fixtures use, so this needs no page of its own.
#[cfg(mln_webgpu_backend)]
struct WebGpuTestSurface {
    surface: webgpu_sys::WGPUSurface,
    id: CString,
    format: u32,
}

/// The canvas format this device prefers, as a WebGPU enum value.
#[cfg(mln_webgpu_backend)]
fn preferred_canvas_format() -> u32 {
    // SAFETY: the call takes nothing and returns a plain code.
    if unsafe { webgl::mln_test_preferred_canvas_format() } == 1 {
        webgpu_sys::WGPUTextureFormat_BGRA8Unorm
    } else {
        webgpu_sys::WGPUTextureFormat_RGBA8Unorm
    }
}

#[cfg(mln_webgpu_backend)]
impl WebGpuTestSurface {
    fn new(
        context: &WebGpuTestContext,
        width: u32,
        height: u32,
    ) -> std::result::Result<Self, Box<dyn StdError>> {
        let id = register_offscreen_canvas(width, height)?;
        let selector = CString::new(format!("#{}", id.to_str().expect("ASCII id")))
            .expect("the generated selector contains no NUL");

        // SAFETY: the instance is live, and both the chained source and the
        // descriptor outlive the call that reads them.
        let surface = unsafe {
            let mut source: webgpu_sys::WGPUEmscriptenSurfaceSourceCanvasHTMLSelector =
                std::mem::zeroed();
            source.chain.sType = webgpu_sys::WGPUSType_EmscriptenSurfaceSourceCanvasHTMLSelector;
            source.selector = string_view(&selector);
            let mut descriptor: webgpu_sys::WGPUSurfaceDescriptor = std::mem::zeroed();
            descriptor.nextInChain = std::ptr::from_mut(&mut source.chain);
            webgpu_sys::wgpuInstanceCreateSurface(context.instance, &descriptor)
        };
        if surface.is_null() {
            // SAFETY: id is still live.
            unsafe { webgl::mln_test_unregister_offscreen_canvas(id.as_ptr()) };
            return Err("creating the fixture's WebGPU canvas surface failed".into());
        }

        Ok(Self {
            surface,
            id,
            // What a browser host passes: the canvas format this device
            // prefers. Configuring another one is legal and costs the browser
            // an extra copy, which it says so in the console.
            format: preferred_canvas_format(),
        })
    }

    fn descriptor(
        &self,
        extent: RenderTargetExtent,
        context: &WebGpuTestContext,
    ) -> WebGpuSurfaceDescriptor {
        // SAFETY: the surface is live until this value drops, and the session
        // borrows it for no longer.
        unsafe {
            WebGpuSurfaceDescriptor::new(
                extent,
                context.descriptor(),
                NativePointer::from_ptr(self.surface),
                self.format,
            )
        }
    }
}

#[cfg(mln_webgpu_backend)]
impl WebGpuTestSurface {
    /// Reads the canvas this surface presents to.
    ///
    /// The session configures the surface for rendering only, so its frame
    /// texture is not copyable and the canvas is what a test can read. That is
    /// also the honest thing to check: it says the frame was presented, not
    /// merely drawn.
    fn read_rgba(
        &self,
        width: u32,
        height: u32,
    ) -> std::result::Result<Vec<u8>, Box<dyn StdError>> {
        let mut pixels = vec![0_u8; (width * height * 4) as usize];
        // SAFETY: the id is live, and the buffer is this call's with its own
        // capacity reported.
        let copied = unsafe {
            webgl::mln_test_read_canvas_rgba(self.id.as_ptr(), pixels.as_mut_ptr(), pixels.len())
        };
        if copied != pixels.len() {
            return Err(format!(
                "reading the fixture's canvas returned {copied} of {} bytes",
                pixels.len()
            )
            .into());
        }
        Ok(pixels)
    }
}

#[cfg(mln_webgpu_backend)]
impl Drop for WebGpuTestSurface {
    fn drop(&mut self) {
        // SAFETY: the surface and the registration are this value's, and both
        // are released exactly once.
        unsafe {
            webgpu_sys::wgpuSurfaceRelease(self.surface);
            webgl::mln_test_unregister_offscreen_canvas(self.id.as_ptr());
        }
    }
}

/// A caller-owned WebGPU texture and view, the way a host that allocates its own
/// render target hands one over.
#[cfg(mln_webgpu_backend)]
struct WebGpuBorrowedTexture {
    texture: webgpu_sys::WGPUTexture,
    texture_view: webgpu_sys::WGPUTextureView,
    format: u32,
    width: u32,
    height: u32,
}

#[cfg(mln_webgpu_backend)]
impl WebGpuBorrowedTexture {
    fn new(
        context: &WebGpuTestContext,
        width: u32,
        height: u32,
    ) -> std::result::Result<Self, Box<dyn StdError>> {
        let format = webgpu_sys::WGPUTextureFormat_RGBA8Unorm;
        let mut descriptor: webgpu_sys::WGPUTextureDescriptor = unsafe { std::mem::zeroed() };
        // Render attachment because the session draws into it, and texture
        // binding because a host samples it afterwards.
        descriptor.usage = webgpu_sys::WGPUTextureUsage_RenderAttachment
            | webgpu_sys::WGPUTextureUsage_TextureBinding
            | webgpu_sys::WGPUTextureUsage_CopySrc;
        descriptor.dimension = webgpu_sys::WGPUTextureDimension_2D;
        descriptor.size = webgpu_sys::WGPUExtent3D {
            width,
            height,
            depthOrArrayLayers: 1,
        };
        descriptor.format = format;
        descriptor.mipLevelCount = 1;
        descriptor.sampleCount = 1;

        // SAFETY: the device is live and descriptor is live for the call.
        let texture = unsafe { webgpu_sys::wgpuDeviceCreateTexture(context.device, &descriptor) };
        if texture.is_null() {
            return Err("creating the fixture's WebGPU texture failed".into());
        }
        // SAFETY: texture is the handle just created.
        let texture_view = unsafe { webgpu_sys::wgpuTextureCreateView(texture, std::ptr::null()) };
        if texture_view.is_null() {
            // SAFETY: releasing the texture this call created.
            unsafe { webgpu_sys::wgpuTextureRelease(texture) };
            return Err("creating the fixture's WebGPU texture view failed".into());
        }

        Ok(Self {
            texture,
            texture_view,
            format,
            width,
            height,
        })
    }

    fn descriptor(
        &self,
        extent: RenderTargetExtent,
        context: &WebGpuTestContext,
    ) -> WebGpuBorrowedTextureDescriptor {
        // SAFETY: both handles are live until this value drops, and the session
        // borrows them for no longer.
        unsafe {
            WebGpuBorrowedTextureDescriptor::new(
                extent,
                self.width,
                self.height,
                context.descriptor(),
                NativePointer::from_ptr(self.texture),
                NativePointer::from_ptr(self.texture_view),
                self.format,
            )
        }
    }
}

#[cfg(mln_webgpu_backend)]
unsafe extern "C" fn on_buffer_mapped(
    status: webgpu_sys::WGPUMapAsyncStatus,
    _message: webgpu_sys::WGPUStringView,
    user_data: *mut std::ffi::c_void,
    _reserved: *mut std::ffi::c_void,
) {
    // SAFETY: user_data is this request's status slot, delivered once.
    unsafe { fill_callback_slot::<webgpu_sys::WGPUMapAsyncStatus>(user_data, status) };
}

#[cfg(mln_webgpu_backend)]
impl WebGpuBorrowedTexture {
    /// Reads this texture back, so a test can tell what the session drew into
    /// it rather than only that the call returned.
    ///
    /// Independent of the readback the C API offers: this waits on the
    /// fixture's own instance, which asked for timed waits, so a blank result
    /// here means the frame is blank rather than that a wait went wrong.
    fn read_rgba(
        &self,
        context: &WebGpuTestContext,
    ) -> std::result::Result<Vec<u8>, Box<dyn StdError>> {
        // WebGPU pads every row of a texture-to-buffer copy to 256 bytes.
        const ROW_ALIGNMENT: u32 = 256;
        let row_stride = self.width * 4;
        let aligned_row_stride = row_stride.div_ceil(ROW_ALIGNMENT) * ROW_ALIGNMENT;
        let mapped_size = u64::from(aligned_row_stride) * u64::from(self.height);

        // SAFETY: every handle below is live for the calls it is passed to, and
        // each descriptor outlives its own call.
        unsafe {
            let mut buffer_descriptor: webgpu_sys::WGPUBufferDescriptor = std::mem::zeroed();
            buffer_descriptor.size = mapped_size;
            buffer_descriptor.usage =
                webgpu_sys::WGPUBufferUsage_CopyDst | webgpu_sys::WGPUBufferUsage_MapRead;
            let staging = webgpu_sys::wgpuDeviceCreateBuffer(context.device, &buffer_descriptor);
            if staging.is_null() {
                return Err("creating the fixture's readback buffer failed".into());
            }

            let encoder =
                webgpu_sys::wgpuDeviceCreateCommandEncoder(context.device, std::ptr::null());
            let mut source: webgpu_sys::WGPUTexelCopyTextureInfo = std::mem::zeroed();
            source.texture = self.texture;
            source.aspect = webgpu_sys::WGPUTextureAspect_All;
            let mut target: webgpu_sys::WGPUTexelCopyBufferInfo = std::mem::zeroed();
            target.buffer = staging;
            target.layout.bytesPerRow = aligned_row_stride;
            target.layout.rowsPerImage = self.height;
            let extent = webgpu_sys::WGPUExtent3D {
                width: self.width,
                height: self.height,
                depthOrArrayLayers: 1,
            };
            webgpu_sys::wgpuCommandEncoderCopyTextureToBuffer(encoder, &source, &target, &extent);
            let commands = webgpu_sys::wgpuCommandEncoderFinish(encoder, std::ptr::null());
            webgpu_sys::wgpuCommandEncoderRelease(encoder);
            let queue = webgpu_sys::wgpuDeviceGetQueue(context.device);
            webgpu_sys::wgpuQueueSubmit(queue, 1, &commands);
            webgpu_sys::wgpuCommandBufferRelease(commands);
            webgpu_sys::wgpuQueueRelease(queue);

            let (status_slot, status_user_data) = callback_slot::<webgpu_sys::WGPUMapAsyncStatus>();
            let mut callback_info: webgpu_sys::WGPUBufferMapCallbackInfo = std::mem::zeroed();
            callback_info.mode = webgpu_sys::WGPUCallbackMode_WaitAnyOnly;
            callback_info.callback = Some(on_buffer_mapped);
            callback_info.userdata1 = status_user_data;
            let future = webgpu_sys::wgpuBufferMapAsync(
                staging,
                webgpu_sys::WGPUMapMode_Read,
                0,
                mapped_size as usize,
                callback_info,
            );
            // Releases the buffer however the wait ends, which a `?` straight
            // out of here would skip.
            let waited = await_future(context.instance, future);
            let status = status_slot.take();
            if waited.is_err() || status != Some(webgpu_sys::WGPUMapAsyncStatus_Success) {
                webgpu_sys::wgpuBufferRelease(staging);
                waited?;
                return Err(
                    format!("mapping the fixture's readback buffer failed ({status:?})").into(),
                );
            }

            let mapped =
                webgpu_sys::wgpuBufferGetConstMappedRange(staging, 0, mapped_size as usize)
                    .cast::<u8>();
            if mapped.is_null() {
                webgpu_sys::wgpuBufferUnmap(staging);
                webgpu_sys::wgpuBufferRelease(staging);
                return Err("the fixture's readback buffer produced no mapped range".into());
            }
            let mut pixels = Vec::with_capacity((row_stride * self.height) as usize);
            for row in 0..self.height {
                let offset = (row * aligned_row_stride) as usize;
                pixels.extend_from_slice(std::slice::from_raw_parts(
                    mapped.add(offset),
                    row_stride as usize,
                ));
            }
            webgpu_sys::wgpuBufferUnmap(staging);
            webgpu_sys::wgpuBufferRelease(staging);
            Ok(pixels)
        }
    }
}

#[cfg(mln_webgpu_backend)]
impl Drop for WebGpuBorrowedTexture {
    fn drop(&mut self) {
        // SAFETY: both handles are this value's and are released exactly once.
        unsafe {
            webgpu_sys::wgpuTextureViewRelease(self.texture_view);
            webgpu_sys::wgpuTextureRelease(self.texture);
        }
    }
}

#[cfg(target_os = "emscripten")]
mod webgl {
    use std::ffi::{CStr, c_char, c_void};

    // emscripten/html5_webgl.h. Fields in declaration order; `bool` there is C23
    // `_Bool`, which Rust's `bool` matches.
    #[repr(C)]
    pub(super) struct ContextAttributes {
        pub alpha: bool,
        pub depth: bool,
        pub stencil: bool,
        pub antialias: bool,
        pub premultiplied_alpha: bool,
        pub preserve_drawing_buffer: bool,
        pub power_preference: i32,
        pub fail_if_major_performance_caveat: bool,
        pub major_version: i32,
        pub minor_version: i32,
        pub enable_extensions_by_default: bool,
        pub explicit_swap_control: bool,
        pub proxy_context_to_main_thread: i32,
        pub render_via_offscreen_back_buffer: bool,
    }

    /// EMSCRIPTEN_WEBGL_CONTEXT_PROXY_DISALLOW.
    pub(super) const PROXY_DISALLOW: i32 = 0;
    /// EMSCRIPTEN_RESULT_SUCCESS.
    pub(super) const RESULT_SUCCESS: i32 = 0;

    unsafe extern "C" {
        pub(super) fn emscripten_webgl_init_context_attributes(attributes: *mut ContextAttributes);
        pub(super) fn emscripten_webgl_create_context(
            target: *const c_char,
            attributes: *const ContextAttributes,
        ) -> i32;
        pub(super) fn emscripten_webgl_make_context_current(context: i32) -> i32;
        pub(super) fn emscripten_webgl_destroy_context(context: i32) -> i32;
        pub(super) fn emscripten_webgl_get_proc_address(name: *const c_char) -> *mut c_void;

        // bindings/rust/crates/maplibre-native-ffi/emscripten/test_support.js.
        pub(super) fn mln_test_register_offscreen_canvas(
            name: *const c_char,
            width: i32,
            height: i32,
        );
        pub(super) fn mln_test_unregister_offscreen_canvas(name: *const c_char);
        // Only the WebGPU surface fixtures present to a canvas and read it
        // back; a WebGL build links the same JS support and calls neither.
        #[cfg(mln_webgpu_backend)]
        pub(super) fn mln_test_preferred_canvas_format() -> i32;
        #[cfg(mln_webgpu_backend)]
        pub(super) fn mln_test_read_canvas_rgba(
            name: *const c_char,
            out: *mut u8,
            capacity: usize,
        ) -> usize;
    }

    pub(super) fn proc_address(symbol: &CStr) -> *const c_void {
        // SAFETY: symbol is a valid null-terminated name.
        unsafe { emscripten_webgl_get_proc_address(symbol.as_ptr()) }
    }
}

/// A WebGL2 context on a canvas of the fixture's own.
///
/// The browser owns the context a session renders into, so this creates a real
/// one and hands it over the way a browser host would. Each fixture gets a
/// private OffscreenCanvas because an OffscreenCanvas belongs to one thread and
/// the suite attaches sessions from more than one.
#[cfg(target_os = "emscripten")]
struct WebGlTestContext {
    context: i32,
    id: CString,
}

#[cfg(target_os = "emscripten")]
static WEBGL_CANVAS_SERIAL: AtomicUsize = AtomicUsize::new(1);

/// Registers an OffscreenCanvas of the fixture's own and returns its id.
///
/// Both browser backends take their render target from a canvas, so both reach
/// the same registry; see emscripten/test_support.js.
#[cfg(target_os = "emscripten")]
fn register_offscreen_canvas(
    width: u32,
    height: u32,
) -> std::result::Result<CString, Box<dyn StdError>> {
    let serial = WEBGL_CANVAS_SERIAL.fetch_add(1, Ordering::Relaxed);
    let id = CString::new(format!("mln-rust-test-{serial}"))
        .expect("the generated canvas id contains no NUL");
    // SAFETY: id is a live null-terminated string, and the extent is the
    // fixture's own.
    unsafe {
        webgl::mln_test_register_offscreen_canvas(id.as_ptr(), width as i32, height as i32);
    }
    Ok(id)
}

#[cfg(target_os = "emscripten")]
impl WebGlTestContext {
    fn new(width: u32, height: u32) -> std::result::Result<Self, Box<dyn StdError>> {
        let id = register_offscreen_canvas(width, height)?;

        let mut attributes = std::mem::MaybeUninit::<webgl::ContextAttributes>::uninit();
        // SAFETY: emscripten fills every field of the struct this points at.
        let mut attributes = unsafe {
            webgl::emscripten_webgl_init_context_attributes(attributes.as_mut_ptr());
            attributes.assume_init()
        };
        // WebGL2 is the GLES 3.0 the OpenGL backend targets.
        attributes.major_version = 2;
        attributes.minor_version = 0;
        attributes.depth = true;
        attributes.stencil = true;
        attributes.antialias = false;
        // The suite renders into its own texture rather than presenting, so the
        // context needs no drawing buffer preservation and there is no swap to
        // take over. It stays on the thread that created it.
        attributes.preserve_drawing_buffer = false;
        attributes.explicit_swap_control = false;
        attributes.proxy_context_to_main_thread = webgl::PROXY_DISALLOW;

        let target = CString::new(format!("#{}", id.to_str().expect("ASCII id")))
            .expect("the generated selector contains no NUL");
        // SAFETY: both pointers are live for the call.
        let context =
            unsafe { webgl::emscripten_webgl_create_context(target.as_ptr(), &attributes) };
        if context <= 0 {
            // SAFETY: id is still live.
            unsafe { webgl::mln_test_unregister_offscreen_canvas(id.as_ptr()) };
            return Err(format!("creating the fixture's WebGL context failed: {context}").into());
        }

        let created = Self { context, id };
        created.make_current()?;
        Ok(created)
    }

    fn descriptor(&self) -> OpenGLContextDescriptor {
        OpenGLContextDescriptor::WebGl(WebGlContextDescriptor::existing(self.context))
    }

    // A browser session presents to the canvas this context was created
    // against, so the context names the surface and the descriptor's own
    // surface field stays null. See validate_opengl_surface_descriptor().
    fn surface(&self) -> NativePointer {
        NativePointer::NULL
    }

    fn make_current(&self) -> std::result::Result<(), Box<dyn StdError>> {
        // SAFETY: the context is live until this value is dropped.
        let result = unsafe { webgl::emscripten_webgl_make_context_current(self.context) };
        if result == webgl::RESULT_SUCCESS {
            Ok(())
        } else {
            Err(format!("making the fixture's WebGL context current failed: {result}").into())
        }
    }
}

#[cfg(target_os = "emscripten")]
impl Drop for WebGlTestContext {
    fn drop(&mut self) {
        // SAFETY: the context and the registration are this value's, and both
        // are released exactly once.
        unsafe {
            webgl::emscripten_webgl_destroy_context(self.context);
            webgl::mln_test_unregister_offscreen_canvas(self.id.as_ptr());
        }
    }
}

#[cfg(target_os = "windows")]
struct WglTestContext {
    window: wgl::Hwnd,
    device_context: wgl::Hdc,
    share_context: wgl::Hglrc,
}

#[cfg(target_os = "windows")]
impl WglTestContext {
    fn new(width: u32, height: u32) -> std::result::Result<Self, Box<dyn StdError>> {
        let class_name = CString::new(format!(
            "MaplibreNativeRustWglTest{}",
            OPENGL_TEST_CONTEXT_CLASS_ID.fetch_add(1, Ordering::Relaxed)
        ))?;
        let module = unsafe { wgl::GetModuleHandleA(std::ptr::null()) };
        if module.is_null() {
            return Err("GetModuleHandleA returned null".into());
        }

        let window_class = wgl::WndClassA {
            style: wgl::CS_OWNDC,
            lpfnWndProc: Some(wgl_window_proc),
            cbClsExtra: 0,
            cbWndExtra: 0,
            hInstance: module,
            hIcon: std::ptr::null_mut(),
            hCursor: std::ptr::null_mut(),
            hbrBackground: std::ptr::null_mut(),
            lpszMenuName: std::ptr::null(),
            lpszClassName: class_name.as_ptr(),
        };
        unsafe {
            wgl::RegisterClassA(&window_class);
        }

        let window = unsafe {
            wgl::CreateWindowExA(
                0,
                class_name.as_ptr(),
                class_name.as_ptr(),
                wgl::WS_OVERLAPPEDWINDOW,
                0,
                0,
                width as i32,
                height as i32,
                std::ptr::null_mut(),
                std::ptr::null_mut(),
                module,
                std::ptr::null_mut(),
            )
        };
        if window.is_null() {
            return Err("CreateWindowExA returned null".into());
        }

        let device_context = unsafe { wgl::GetDC(window) };
        if device_context.is_null() {
            unsafe {
                wgl::DestroyWindow(window);
            }
            return Err("GetDC returned null".into());
        }

        let pixel_format_descriptor = wgl::PixelFormatDescriptor {
            nSize: std::mem::size_of::<wgl::PixelFormatDescriptor>() as u16,
            nVersion: 1,
            dwFlags: wgl::PFD_DRAW_TO_WINDOW | wgl::PFD_SUPPORT_OPENGL | wgl::PFD_DOUBLEBUFFER,
            iPixelType: wgl::PFD_TYPE_RGBA,
            cColorBits: 32,
            cDepthBits: 24,
            cStencilBits: 8,
            iLayerType: wgl::PFD_MAIN_PLANE,
            ..Default::default()
        };
        let pixel_format =
            unsafe { wgl::ChoosePixelFormat(device_context, &pixel_format_descriptor) };
        if pixel_format == 0 {
            unsafe {
                wgl::ReleaseDC(window, device_context);
                wgl::DestroyWindow(window);
            }
            return Err("ChoosePixelFormat returned zero".into());
        }
        if unsafe { wgl::SetPixelFormat(device_context, pixel_format, &pixel_format_descriptor) }
            == 0
        {
            unsafe {
                wgl::ReleaseDC(window, device_context);
                wgl::DestroyWindow(window);
            }
            return Err("SetPixelFormat failed".into());
        }

        let share_context = unsafe { wgl::wglCreateContext(device_context) };
        if share_context.is_null() {
            unsafe {
                wgl::ReleaseDC(window, device_context);
                wgl::DestroyWindow(window);
            }
            return Err("wglCreateContext returned null".into());
        }
        if unsafe { wgl::wglMakeCurrent(device_context, share_context) } == 0 {
            unsafe {
                wgl::wglDeleteContext(share_context);
                wgl::ReleaseDC(window, device_context);
                wgl::DestroyWindow(window);
            }
            return Err("wglMakeCurrent failed".into());
        }

        Ok(Self {
            window,
            device_context,
            share_context,
        })
    }

    fn descriptor(&self) -> OpenGLContextDescriptor {
        OpenGLContextDescriptor::Wgl(WglContextDescriptor::new(
            unsafe { NativePointer::from_ptr(self.device_context) },
            unsafe { NativePointer::from_ptr(self.share_context) },
        ))
    }

    fn surface(&self) -> NativePointer {
        unsafe { NativePointer::from_ptr(self.device_context) }
    }

    fn make_current(&self) -> std::result::Result<(), Box<dyn StdError>> {
        if unsafe { wgl::wglMakeCurrent(self.device_context, self.share_context) } == 0 {
            Err("wglMakeCurrent failed".into())
        } else {
            Ok(())
        }
    }

    fn get_proc_address(&self, symbol: &CStr) -> *const c_void {
        let proc = unsafe { wgl::wglGetProcAddress(symbol.as_ptr()) };
        if is_valid_wgl_proc_address(proc) {
            return proc.cast();
        }

        let module = unsafe { wgl::GetModuleHandleA(c"opengl32.dll".as_ptr()) };
        let module = if module.is_null() {
            unsafe { wgl::LoadLibraryA(c"opengl32.dll".as_ptr()) }
        } else {
            module
        };
        if module.is_null() {
            return std::ptr::null();
        }
        unsafe { wgl::GetProcAddress(module, symbol.as_ptr()).cast() }
    }
}

#[cfg(target_os = "windows")]
impl Drop for WglTestContext {
    fn drop(&mut self) {
        unsafe {
            wgl::wglMakeCurrent(std::ptr::null_mut(), std::ptr::null_mut());
            wgl::wglDeleteContext(self.share_context);
            wgl::ReleaseDC(self.window, self.device_context);
            wgl::DestroyWindow(self.window);
        }
    }
}

#[cfg(target_os = "windows")]
fn is_valid_wgl_proc_address(proc: *mut c_void) -> bool {
    let address = proc as usize;
    !proc.is_null() && address > 3 && address != usize::MAX
}

#[cfg(target_os = "windows")]
unsafe extern "system" fn wgl_window_proc(
    window: wgl::Hwnd,
    message: u32,
    wparam: usize,
    lparam: isize,
) -> isize {
    unsafe { wgl::DefWindowProcA(window, message, wparam, lparam) }
}

#[cfg(target_os = "windows")]
static OPENGL_TEST_CONTEXT_CLASS_ID: AtomicUsize = AtomicUsize::new(1);

#[cfg(target_os = "windows")]
mod wgl {
    use super::{c_char, c_void};

    pub type Hdc = *mut c_void;
    pub type Hglrc = *mut c_void;
    pub type Hinstance = *mut c_void;
    pub type Hwnd = *mut c_void;

    pub const CS_OWNDC: u32 = 0x0020;
    pub const PFD_DOUBLEBUFFER: u32 = 0x0000_0001;
    pub const PFD_DRAW_TO_WINDOW: u32 = 0x0000_0004;
    pub const PFD_SUPPORT_OPENGL: u32 = 0x0000_0020;
    pub const PFD_TYPE_RGBA: u8 = 0;
    pub const PFD_MAIN_PLANE: u8 = 0;
    pub const WS_OVERLAPPEDWINDOW: u32 = 0x00cf_0000;

    #[repr(C)]
    #[derive(Default)]
    #[allow(non_snake_case)]
    pub struct WndClassA {
        pub style: u32,
        pub lpfnWndProc: Option<unsafe extern "system" fn(Hwnd, u32, usize, isize) -> isize>,
        pub cbClsExtra: i32,
        pub cbWndExtra: i32,
        pub hInstance: Hinstance,
        pub hIcon: *mut c_void,
        pub hCursor: *mut c_void,
        pub hbrBackground: *mut c_void,
        pub lpszMenuName: *const c_char,
        pub lpszClassName: *const c_char,
    }

    #[repr(C)]
    #[derive(Default)]
    #[allow(non_snake_case)]
    pub struct PixelFormatDescriptor {
        pub nSize: u16,
        pub nVersion: u16,
        pub dwFlags: u32,
        pub iPixelType: u8,
        pub cColorBits: u8,
        pub cRedBits: u8,
        pub cRedShift: u8,
        pub cGreenBits: u8,
        pub cGreenShift: u8,
        pub cBlueBits: u8,
        pub cBlueShift: u8,
        pub cAlphaBits: u8,
        pub cAlphaShift: u8,
        pub cAccumBits: u8,
        pub cAccumRedBits: u8,
        pub cAccumGreenBits: u8,
        pub cAccumBlueBits: u8,
        pub cAccumAlphaBits: u8,
        pub cDepthBits: u8,
        pub cStencilBits: u8,
        pub cAuxBuffers: u8,
        pub iLayerType: u8,
        pub bReserved: u8,
        pub dwLayerMask: u32,
        pub dwVisibleMask: u32,
        pub dwDamageMask: u32,
    }

    #[link(name = "kernel32")]
    unsafe extern "system" {
        pub fn GetModuleHandleA(module_name: *const c_char) -> Hinstance;
        pub fn GetProcAddress(module: Hinstance, proc_name: *const c_char) -> *mut c_void;
        pub fn LoadLibraryA(file_name: *const c_char) -> Hinstance;
    }

    #[link(name = "user32")]
    unsafe extern "system" {
        pub fn RegisterClassA(window_class: *const WndClassA) -> u16;
        pub fn CreateWindowExA(
            extended_style: u32,
            class_name: *const c_char,
            window_name: *const c_char,
            style: u32,
            x: i32,
            y: i32,
            width: i32,
            height: i32,
            parent: Hwnd,
            menu: *mut c_void,
            instance: Hinstance,
            param: *mut c_void,
        ) -> Hwnd;
        pub fn DefWindowProcA(window: Hwnd, message: u32, wparam: usize, lparam: isize) -> isize;
        pub fn DestroyWindow(window: Hwnd) -> i32;
        pub fn GetDC(window: Hwnd) -> Hdc;
        pub fn ReleaseDC(window: Hwnd, device_context: Hdc) -> i32;
    }

    #[link(name = "gdi32")]
    unsafe extern "system" {
        pub fn ChoosePixelFormat(
            device_context: Hdc,
            descriptor: *const PixelFormatDescriptor,
        ) -> i32;
        pub fn SetPixelFormat(
            device_context: Hdc,
            format: i32,
            descriptor: *const PixelFormatDescriptor,
        ) -> i32;
    }

    #[link(name = "opengl32")]
    unsafe extern "system" {
        pub fn wglCreateContext(device_context: Hdc) -> Hglrc;
        pub fn wglDeleteContext(context: Hglrc) -> i32;
        pub fn wglGetProcAddress(name: *const c_char) -> *mut c_void;
        pub fn wglMakeCurrent(device_context: Hdc, context: Hglrc) -> i32;
    }
}

impl OpenGLTestContext {
    fn new(width: u32, height: u32) -> std::result::Result<Self, Box<dyn StdError>> {
        Self::new_platform(width, height)
    }

    #[cfg(any(target_os = "linux", target_os = "android", target_os = "macos"))]
    fn new_platform(width: u32, height: u32) -> std::result::Result<Self, Box<dyn StdError>> {
        let platform = EglTestContext::new(width, height)?;
        let gl = unsafe {
            gl_api::Context::from_loader_function(|symbol| {
                let symbol = CString::new(symbol).expect("GL symbol names do not contain NULs");
                platform.get_proc_address(&symbol).cast()
            })
        };
        let descriptor = platform.descriptor();
        let surface_handle = platform.surface();

        Ok(Self {
            descriptor,
            surface_handle,
            gl,
            platform,
        })
    }

    #[cfg(target_os = "windows")]
    fn new_platform(width: u32, height: u32) -> std::result::Result<Self, Box<dyn StdError>> {
        let platform = WglTestContext::new(width, height)?;
        let gl = unsafe {
            gl_api::Context::from_loader_function(|symbol| {
                let symbol = CString::new(symbol).expect("GL symbol names do not contain NULs");
                platform.get_proc_address(&symbol).cast()
            })
        };
        let descriptor = platform.descriptor();
        let surface_handle = platform.surface();

        Ok(Self {
            descriptor,
            surface_handle,
            gl,
            platform,
        })
    }

    #[cfg(target_os = "emscripten")]
    fn new_platform(width: u32, height: u32) -> std::result::Result<Self, Box<dyn StdError>> {
        let platform = WebGlTestContext::new(width, height)?;
        // glow builds its native backend for Emscripten, where the GLES entry
        // points are linked into the module rather than looked up at run time.
        let gl = unsafe {
            gl_api::Context::from_loader_function(|symbol| {
                let symbol = CString::new(symbol).expect("GL symbol names do not contain NULs");
                webgl::proc_address(&symbol).cast()
            })
        };
        let descriptor = platform.descriptor();
        let surface_handle = platform.surface();

        Ok(Self {
            descriptor,
            surface_handle,
            gl,
            platform,
        })
    }

    #[cfg(not(any(
        target_os = "linux",
        target_os = "android",
        target_os = "macos",
        target_os = "windows",
        target_os = "emscripten"
    )))]
    fn new_platform(_width: u32, _height: u32) -> std::result::Result<Self, Box<dyn StdError>> {
        Err("OpenGL test context is only available on WGL, EGL, and WebGL".into())
    }

    fn descriptor(&self) -> OpenGLContextDescriptor {
        self.descriptor.clone()
    }

    fn surface(&self) -> NativePointer {
        self.surface_handle
    }

    fn make_current(&self) -> std::result::Result<(), Box<dyn StdError>> {
        #[cfg(any(target_os = "linux", target_os = "android", target_os = "macos"))]
        {
            self.platform.make_current()?;
            Ok(())
        }
        #[cfg(target_os = "windows")]
        {
            self.platform.make_current()?;
            Ok(())
        }
        #[cfg(target_os = "emscripten")]
        {
            self.platform.make_current()?;
            Ok(())
        }
        #[cfg(not(any(
            target_os = "linux",
            target_os = "android",
            target_os = "macos",
            target_os = "windows",
            target_os = "emscripten"
        )))]
        {
            Ok(())
        }
    }

    fn check_gl_error(&self, operation: &str) -> std::result::Result<(), Box<dyn StdError>> {
        let error = unsafe { self.gl.get_error() };
        if error == gl_api::NO_ERROR {
            Ok(())
        } else {
            Err(format!("{operation} failed with OpenGL error 0x{error:x}").into())
        }
    }

    /// Reads the surface this context presents to.
    ///
    /// The browser reads the context's own drawing buffer, which is what its
    /// canvas composites, so it names no read buffer: WebGL has one, and
    /// selecting FRONT is not a choice it offers.
    #[cfg(any(target_os = "windows", target_os = "emscripten"))]
    fn read_surface_rgba(
        &self,
        width: u32,
        height: u32,
    ) -> std::result::Result<Vec<u8>, Box<dyn StdError>> {
        self.make_current()?;
        let mut pixels = vec![0_u8; width as usize * height as usize * 4];
        unsafe {
            self.gl.bind_framebuffer(gl_api::FRAMEBUFFER, None);
            #[cfg(target_os = "windows")]
            self.gl.read_buffer(gl_api::FRONT);
            self.gl.read_pixels(
                0,
                0,
                width as i32,
                height as i32,
                gl_api::RGBA,
                gl_api::UNSIGNED_BYTE,
                gl_api::PixelPackData::Slice(Some(&mut pixels)),
            );
        }
        self.check_gl_error("read OpenGL surface")?;
        Ok(pixels)
    }
}

struct OpenGLBorrowedTexture {
    context: OpenGLTestContext,
    texture: Option<gl_api::NativeTexture>,
    width: u32,
    height: u32,
}

impl OpenGLBorrowedTexture {
    fn new(width: u32, height: u32) -> std::result::Result<Self, Box<dyn StdError>> {
        let context = OpenGLTestContext::new(width, height)?;
        let texture = Self::create_texture(&context, width, height)?;
        Ok(Self {
            context,
            texture: Some(texture),
            width,
            height,
        })
    }

    fn create_texture(
        context: &OpenGLTestContext,
        width: u32,
        height: u32,
    ) -> std::result::Result<gl_api::NativeTexture, Box<dyn StdError>> {
        context.make_current()?;
        let texture = unsafe {
            let texture = context.gl.create_texture()?;
            context.gl.bind_texture(gl_api::TEXTURE_2D, Some(texture));
            context.gl.tex_parameter_i32(
                gl_api::TEXTURE_2D,
                gl_api::TEXTURE_MIN_FILTER,
                gl_api::NEAREST as i32,
            );
            context.gl.tex_parameter_i32(
                gl_api::TEXTURE_2D,
                gl_api::TEXTURE_MAG_FILTER,
                gl_api::NEAREST as i32,
            );
            // Zeroed so that a readback proving the session rendered into this
            // texture cannot pass on undefined contents.
            let blank = vec![0_u8; (width as usize) * (height as usize) * 4];
            context.gl.tex_image_2d(
                gl_api::TEXTURE_2D,
                0,
                gl_api::RGBA8 as i32,
                width as i32,
                height as i32,
                0,
                gl_api::RGBA,
                gl_api::UNSIGNED_BYTE,
                gl_api::PixelUnpackData::Slice(Some(&blank)),
            );
            context.gl.bind_texture(gl_api::TEXTURE_2D, None);
            texture
        };
        context.check_gl_error("create borrowed texture")?;
        Ok(texture)
    }

    /// Allocates a replacement in this helper's own context. The outgoing
    /// texture stays live until the caller adopts the replacement.
    fn allocate_replacement(
        &self,
        width: u32,
        height: u32,
    ) -> std::result::Result<gl_api::NativeTexture, Box<dyn StdError>> {
        Self::create_texture(&self.context, width, height)
    }

    /// Tracks a replacement the session has taken and releases the outgoing one.
    fn adopt(
        &mut self,
        texture: gl_api::NativeTexture,
        width: u32,
        height: u32,
    ) -> std::result::Result<(), Box<dyn StdError>> {
        let previous = self.texture.replace(texture);
        self.width = width;
        self.height = height;
        if let Some(previous) = previous {
            self.context.make_current()?;
            unsafe {
                self.context.gl.delete_texture(previous);
            }
            self.context
                .check_gl_error("delete replaced borrowed texture")?;
        }
        Ok(())
    }

    fn descriptor(&self) -> OpenGLContextDescriptor {
        self.context.descriptor()
    }

    fn name(&self) -> u32 {
        self.texture.map(|texture| texture.0.get()).unwrap_or(0)
    }

    fn read_rgba(&self) -> std::result::Result<Vec<u8>, Box<dyn StdError>> {
        self.context.make_current()?;
        let mut pixels = vec![0_u8; self.width as usize * self.height as usize * 4];
        let texture = self.texture.ok_or("borrowed texture has been deleted")?;
        unsafe {
            let framebuffer = self.context.gl.create_framebuffer()?;
            self.context
                .gl
                .bind_framebuffer(gl_api::FRAMEBUFFER, Some(framebuffer));
            self.context.gl.framebuffer_texture_2d(
                gl_api::FRAMEBUFFER,
                gl_api::COLOR_ATTACHMENT0,
                gl_api::TEXTURE_2D,
                Some(texture),
                0,
            );
            let status = self
                .context
                .gl
                .check_framebuffer_status(gl_api::FRAMEBUFFER);
            if status != gl_api::FRAMEBUFFER_COMPLETE {
                self.context.gl.bind_framebuffer(gl_api::FRAMEBUFFER, None);
                self.context.gl.delete_framebuffer(framebuffer);
                return Err(
                    format!("borrowed texture framebuffer is incomplete: 0x{status:x}").into(),
                );
            }
            self.context.gl.read_pixels(
                0,
                0,
                self.width as i32,
                self.height as i32,
                gl_api::RGBA,
                gl_api::UNSIGNED_BYTE,
                gl_api::PixelPackData::Slice(Some(&mut pixels)),
            );
            self.context.gl.bind_framebuffer(gl_api::FRAMEBUFFER, None);
            self.context.gl.delete_framebuffer(framebuffer);
        }
        self.context.check_gl_error("read borrowed texture")?;
        Ok(pixels)
    }
}

impl Drop for OpenGLBorrowedTexture {
    fn drop(&mut self) {
        if let Some(texture) = self.texture.take()
            && self.context.make_current().is_ok()
        {
            unsafe {
                self.context.gl.delete_texture(texture);
            }
        }
    }
}

#[cfg(not(target_os = "emscripten"))]
struct VulkanTestContext {
    _entry: ash::Entry,
    instance: ash::Instance,
    physical_device: vk::PhysicalDevice,
    device: ash::Device,
    graphics_queue: vk::Queue,
    graphics_queue_family_index: u32,
}

#[cfg(not(target_os = "emscripten"))]
impl VulkanTestContext {
    fn new() -> std::result::Result<Self, Box<dyn StdError>> {
        let entry = load_vulkan_entry()?;
        let app_name = CString::new("maplibre-native-rust-tests")?;
        let engine_name = CString::new("maplibre-native-ffi")?;
        let app_info = vk::ApplicationInfo::default()
            .application_name(&app_name)
            .application_version(1)
            .engine_name(&engine_name)
            .engine_version(1)
            .api_version(vk::API_VERSION_1_1);

        let mut instance_extensions = Vec::new();
        let mut instance_flags = vk::InstanceCreateFlags::empty();
        if has_instance_extension(&entry, ash::khr::portability_enumeration::NAME)? {
            instance_extensions.push(ash::khr::portability_enumeration::NAME.as_ptr());
            instance_flags |= vk::InstanceCreateFlags::ENUMERATE_PORTABILITY_KHR;
        }
        let instance_info = vk::InstanceCreateInfo::default()
            .application_info(&app_info)
            .enabled_extension_names(&instance_extensions)
            .flags(instance_flags);
        // SAFETY: instance_info points to stable app-info and extension-name storage.
        let instance = unsafe { entry.create_instance(&instance_info, None)? };

        let (physical_device, graphics_queue_family_index) =
            match pick_vulkan_physical_device(&instance) {
                Ok(value) => value,
                Err(error) => {
                    // SAFETY: instance was created above and has no children yet.
                    unsafe { instance.destroy_instance(None) };
                    return Err(error);
                }
            };

        let queue_priorities = [1.0_f32];
        let queue_info = [vk::DeviceQueueCreateInfo::default()
            .queue_family_index(graphics_queue_family_index)
            .queue_priorities(&queue_priorities)];
        let mut device_extensions = Vec::new();
        if has_device_extension(
            &instance,
            physical_device,
            ash::khr::portability_subset::NAME,
        )? {
            device_extensions.push(ash::khr::portability_subset::NAME.as_ptr());
        }
        // SAFETY: physical_device came from this live instance.
        let supported_features = unsafe { instance.get_physical_device_features(physical_device) };
        let features = vk::PhysicalDeviceFeatures {
            sampler_anisotropy: supported_features.sampler_anisotropy,
            wide_lines: supported_features.wide_lines,
            ..Default::default()
        };
        let device_info = vk::DeviceCreateInfo::default()
            .queue_create_infos(&queue_info)
            .enabled_extension_names(&device_extensions)
            .enabled_features(&features);
        // SAFETY: physical_device and queue family were selected from this instance.
        let device = match unsafe { instance.create_device(physical_device, &device_info, None) } {
            Ok(device) => device,
            Err(error) => {
                // SAFETY: instance is live and has no device child.
                unsafe { instance.destroy_instance(None) };
                return Err(error.into());
            }
        };
        // SAFETY: Queue index 0 exists because the device was created with one queue.
        let graphics_queue = unsafe { device.get_device_queue(graphics_queue_family_index, 0) };

        Ok(Self {
            _entry: entry,
            instance,
            physical_device,
            device,
            graphics_queue,
            graphics_queue_family_index,
        })
    }

    fn descriptor(&self) -> VulkanContextDescriptor {
        let mut descriptor = VulkanContextDescriptor::new(
            // SAFETY: Vulkan handles remain live for the test context lifetime.
            unsafe { NativePointer::from_address(self.instance.handle().as_raw() as usize) },
            unsafe { NativePointer::from_address(self.physical_device.as_raw() as usize) },
            unsafe { NativePointer::from_address(self.device.handle().as_raw() as usize) },
            unsafe { NativePointer::from_address(self.graphics_queue.as_raw() as usize) },
            self.graphics_queue_family_index,
        );
        // SAFETY: Function pointers remain valid while the ash entry and instance are live.
        descriptor.get_instance_proc_addr = unsafe {
            NativePointer::from_address(
                self._entry.static_fn().get_instance_proc_addr as *const () as usize,
            )
        };
        descriptor.get_device_proc_addr = unsafe {
            NativePointer::from_address(
                self.instance.fp_v1_0().get_device_proc_addr as *const () as usize,
            )
        };
        descriptor
    }
}

#[cfg(not(target_os = "emscripten"))]
impl Drop for VulkanTestContext {
    fn drop(&mut self) {
        // SAFETY: Device and instance are live and destroyed in dependency order.
        unsafe {
            let _ = self.device.device_wait_idle();
            self.device.destroy_device(None);
            self.instance.destroy_instance(None);
        }
    }
}

#[cfg(not(target_os = "emscripten"))]
fn load_vulkan_entry() -> std::result::Result<ash::Entry, Box<dyn StdError>> {
    if let Ok(install_dir) = std::env::var("MAPLIBRE_NATIVE_C_INSTALL_DIR") {
        let library_dir = std::path::Path::new(&install_dir).join(if cfg!(target_os = "windows") {
            "bin"
        } else {
            "lib"
        });
        let library_name = if cfg!(target_os = "macos") {
            "libvulkan.1.dylib"
        } else if cfg!(target_os = "windows") {
            "vulkan-1.dll"
        } else {
            "libvulkan.so.1"
        };
        let library_path = library_dir.join(library_name);
        if library_path.exists() {
            // SAFETY: Loading the Vulkan loader is delegated to ash.
            return unsafe { ash::Entry::load_from(&library_path) }.map_err(Into::into);
        }
    }

    // SAFETY: Loading the Vulkan loader is delegated to ash.
    unsafe { ash::Entry::load() }.map_err(Into::into)
}

#[cfg(not(target_os = "emscripten"))]
fn has_instance_extension(
    entry: &ash::Entry,
    name: &CStr,
) -> std::result::Result<bool, Box<dyn StdError>> {
    // SAFETY: entry is a live Vulkan loader entry.
    let properties = unsafe { entry.enumerate_instance_extension_properties(None)? };
    Ok(properties.iter().any(|property| {
        // SAFETY: Vulkan extension names are fixed-size NUL-terminated arrays.
        let property_name = unsafe { CStr::from_ptr(property.extension_name.as_ptr()) };
        property_name == name
    }))
}

#[cfg(not(target_os = "emscripten"))]
fn has_device_extension(
    instance: &ash::Instance,
    physical_device: vk::PhysicalDevice,
    name: &CStr,
) -> std::result::Result<bool, Box<dyn StdError>> {
    // SAFETY: physical_device came from this live instance.
    let properties = unsafe { instance.enumerate_device_extension_properties(physical_device)? };
    Ok(properties.iter().any(|property| {
        // SAFETY: Vulkan extension names are fixed-size NUL-terminated arrays.
        let property_name = unsafe { CStr::from_ptr(property.extension_name.as_ptr()) };
        property_name == name
    }))
}

#[cfg(not(target_os = "emscripten"))]
fn pick_vulkan_physical_device(
    instance: &ash::Instance,
) -> std::result::Result<(vk::PhysicalDevice, u32), Box<dyn StdError>> {
    // SAFETY: instance is live and enumeration writes into ash-owned vectors.
    let devices = unsafe { instance.enumerate_physical_devices()? };
    for physical_device in devices {
        // SAFETY: physical_device came from this live instance.
        let families =
            unsafe { instance.get_physical_device_queue_family_properties(physical_device) };
        for (index, family) in families.iter().enumerate() {
            if family.queue_count > 0 && family.queue_flags.contains(vk::QueueFlags::GRAPHICS) {
                return Ok((physical_device, index.try_into()?));
            }
        }
    }
    Err("no Vulkan physical device with a graphics queue was found".into())
}

fn await_runtime_barrier(runtime: &RuntimeHandle) {
    let operation = runtime.barrier().unwrap();
    assert!(operation.wait(Duration::from_secs(5)).unwrap());
    maplibre_core::check(operation.terminal_status().unwrap()).unwrap();
    operation.finish().unwrap();
}

fn render_frame(session: &RenderSessionHandle, if_needed: bool) -> RenderFrameResult {
    static NEXT_TOKEN: AtomicUsize = AtomicUsize::new(1);
    let token = NEXT_TOKEN.fetch_add(1, Ordering::Relaxed) as u64;
    session
        .request_frame(FrameDemand {
            if_needed,
            present: session.capabilities().unwrap().presentation,
            token,
            ..FrameDemand::default()
        })
        .unwrap();

    let deadline = Instant::now() + Duration::from_secs(5);
    loop {
        let _ = session.service_driver_work(64);
        let batch = match session.drain_frame_results() {
            Ok(batch) => batch,
            Err(error) if error.kind() == ErrorKind::NotReady => {
                assert!(Instant::now() < deadline, "frame demand timed out");
                std::thread::yield_now();
                continue;
            }
            Err(error) => panic!("failed to drain frame results: {error}"),
        };
        if let Some(result) = batch
            .copy_results()
            .unwrap()
            .into_iter()
            .find(|result| result.token == token)
        {
            return result;
        }
        assert!(Instant::now() < deadline, "frame demand timed out");
        std::thread::yield_now();
    }
}

fn close_session(session: RenderSessionHandle) {
    finish_unit(&session, session.detach().unwrap());
    session.destroy().unwrap();
}

fn take_json(session: &RenderSessionHandle, operation: NativeFuture<Vec<u8>>) -> JsonValue {
    wait_until_completed(session, &operation);
    serde_json::from_slice(&operation.take().unwrap()).unwrap()
}

fn take_features(
    session: &RenderSessionHandle,
    operation: NativeFuture<Vec<QueriedFeature>>,
) -> Vec<QueriedFeature> {
    wait_until_completed(session, &operation);
    operation.take().unwrap()
}

fn feature_json(feature: &QueriedFeature) -> JsonValue {
    serde_json::from_slice(&feature.feature).unwrap()
}

#[test]
fn native_pointer_round_trips_address() {
    // SAFETY: The test reconstructs but never dereferences this dummy address.
    let pointer = unsafe { NativePointer::from_address(0x1234) };
    assert_eq!(pointer.address(), 0x1234);
    assert_eq!(unsafe { pointer.as_ptr::<u8>() } as usize, 0x1234);
    assert!(NativePointer::NULL.is_null());
}

#[test]
fn opengl_context_provider_mask_matches_backend_availability() {
    let providers = crate::supported_opengl_context_providers();
    if has_opengl_backend() {
        assert!(providers.intersects(
            OpenGLContextProviderMask::WGL
                | OpenGLContextProviderMask::EGL
                | OpenGLContextProviderMask::WEBGL
        ));
    } else {
        assert!(providers.is_empty());
    }
}

#[test]
fn owned_texture_session_renders_acquires_resizes_and_reads_back() {
    if !has_test_owned_texture_session_backend() {
        return;
    }

    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = crate::completion::blocking(MapHandle::with_options(
        &runtime,
        &MapOptions::new(32, 16, 1.0),
    ));
    let initial_extent = RenderTargetExtent::new(32, 16, 1.0);
    let (context, session) = create_owned_texture_session(&map, initial_extent.clone()).unwrap();

    let capabilities = session.capabilities().unwrap();
    assert!(capabilities.frame_acquisition);
    assert_eq!(
        session.snapshot().unwrap().state,
        RenderSessionLifecycle::Attached
    );

    map.set_style_json(QUERY_STYLE_JSON.as_bytes()).unwrap();
    await_runtime_barrier(&runtime);
    let mut rendered = render_frame(&session, false);
    assert_eq!(rendered.disposition, FrameDisposition::Rendered);
    // The frame result carries the map's follow-up request; once placement
    // settles with no transition in flight, the map stops asking to repaint.
    let repaint_deadline = Instant::now() + Duration::from_secs(5);
    while rendered.needs_repaint {
        assert!(Instant::now() < repaint_deadline, "map never settled");
        rendered = render_frame(&session, false);
        assert_eq!(rendered.disposition, FrameDisposition::Rendered);
    }
    assert!(context.try_acquire_frame_extent(&session, &initial_extent));

    if capabilities.readback {
        let operation = session.read_premultiplied_rgba8().unwrap();
        wait_until_completed(&session, &operation);
        let image = operation.take().unwrap();
        assert_eq!((image.info.width, image.info.height), (32, 16));
        assert_eq!(image.data.len(), image.info.byte_length);
        assert!(image.data.iter().any(|byte| *byte != 0));
    }

    let resized_extent = RenderTargetExtent::new(48, 24, 1.0);
    finish_unit(&session, session.resize(&resized_extent).unwrap());
    drop(
        map.resize(crate::LogicalExtent {
            width: 48,
            height: 24,
            scale_factor: 1.0,
        })
        .unwrap(),
    );
    await_runtime_barrier(&runtime);
    let deadline = Instant::now() + Duration::from_secs(5);
    loop {
        let result = render_frame(&session, false);
        if result.disposition == FrameDisposition::Rendered {
            break;
        }
        assert_eq!(result.disposition, FrameDisposition::SizePending);
        // The repaint flag is meaningful on rendered frames alone.
        assert!(!result.needs_repaint);
        assert!(
            Instant::now() < deadline,
            "resized frame did not become renderable"
        );
        await_runtime_barrier(&runtime);
    }
    assert!(context.try_acquire_frame_extent(&session, &resized_extent));
    assert_eq!(session.snapshot().unwrap().extent, resized_extent);

    match context.set_placeholder_borrowed_target(&session, &resized_extent) {
        Ok(operation) => {
            let deadline = Instant::now() + Duration::from_secs(5);
            while !operation.is_completed().unwrap() {
                let _ = session.service_driver_work(64);
                assert!(Instant::now() < deadline, "target replacement timed out");
            }
            assert_ne!(operation.terminal_status().unwrap(), sys::MLN_STATUS_OK);
        }
        Err(error) => assert_ne!(error.raw_status(), Some(sys::MLN_STATUS_OK)),
    }

    close_session(session);
    let session = context
        .attach_owned_texture(&map, RenderTargetExtent::new(24, 12, 1.0))
        .unwrap();
    close_session(session);
    map.close().unwrap();
    runtime.close_and_wait();
}

#[test]
fn opengl_owned_texture_exposes_backend_metadata() {
    if !has_opengl_test_context_backend() {
        return;
    }

    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = crate::completion::blocking(MapHandle::with_options(
        &runtime,
        &MapOptions::new(32, 16, 1.0),
    ));
    let (_context, session) =
        create_opengl_owned_texture_session(&map, RenderTargetExtent::new(32, 16, 1.0)).unwrap();
    map.set_style_json(QUERY_STYLE_JSON.as_bytes()).unwrap();
    await_runtime_barrier(&runtime);
    assert_eq!(
        render_frame(&session, false).disposition,
        FrameDisposition::Rendered
    );

    let frame = session.acquire_frame().unwrap();
    let (metadata, texture) = frame.opengl_texture().unwrap();
    assert_eq!((metadata.width, metadata.height), (32, 16));
    assert_eq!(metadata.target, gl_api::TEXTURE_2D);
    assert_eq!(metadata.internal_format, gl_api::RGBA8);
    assert!(!texture.is_zero());
    release_frame(frame);

    close_session(session);
    map.close().unwrap();
    runtime.close_and_wait();
}

#[cfg(any(target_os = "linux", target_os = "android", target_os = "macos"))]
#[test]
fn dedicated_opengl_surface_keeps_its_context_on_the_graphics_thread() {
    if !has_opengl_backend() {
        return;
    }

    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = crate::completion::blocking(MapHandle::with_options(
        &runtime,
        &MapOptions::new(32, 16, 1.0),
    ));
    let surface = DedicatedEglTestSurface::new(32, 16).unwrap();
    assert!(!surface.has_current_context());
    let session = finish_attachment(
        map.attach_opengl_surface(
            &OpenGLSurfaceDescriptor::new(
                RenderTargetExtent::new(32, 16, 1.0),
                surface.descriptor(),
                surface.surface(),
            ),
            caller_attach_options(),
        )
        .unwrap(),
    )
    .unwrap();
    assert!(surface.has_current_context());
    map.set_style_json(QUERY_STYLE_JSON.as_bytes()).unwrap();
    await_runtime_barrier(&runtime);
    assert_eq!(
        render_frame(&session, false).disposition,
        FrameDisposition::Rendered
    );
    assert!(surface.has_current_context());

    close_session(session);
    map.close().unwrap();
    runtime.close_and_wait();
}

#[test]
fn opengl_surface_session_renders_into_the_platform_surface() {
    if !has_opengl_test_context_backend() || cfg!(target_os = "emscripten") {
        return;
    }

    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = crate::completion::blocking(MapHandle::with_options(
        &runtime,
        &MapOptions::new(32, 16, 1.0),
    ));
    let (_context, session) =
        create_opengl_surface_session(&map, RenderTargetExtent::new(32, 16, 1.0)).unwrap();
    map.set_style_json(QUERY_STYLE_JSON.as_bytes()).unwrap();
    await_runtime_barrier(&runtime);

    assert_eq!(
        render_frame(&session, false).disposition,
        FrameDisposition::Rendered
    );
    #[cfg(any(target_os = "windows", target_os = "emscripten"))]
    {
        let pixels = _context.read_surface_rgba(32, 16).unwrap();
        assert!(pixels.iter().any(|byte| *byte != 0));
    }

    close_session(session);
    map.close().unwrap();
    runtime.close_and_wait();
}

#[test]
fn opengl_borrowed_texture_session_replaces_its_target() {
    if !has_opengl_test_context_backend() {
        return;
    }

    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = crate::completion::blocking(MapHandle::with_options(
        &runtime,
        &MapOptions::new(32, 16, 1.0),
    ));
    let initial_extent = RenderTargetExtent::new(32, 16, 1.0);
    let (mut texture, session) =
        create_opengl_borrowed_texture_session(&map, initial_extent).unwrap();
    map.set_style_json(QUERY_STYLE_JSON.as_bytes()).unwrap();
    await_runtime_barrier(&runtime);
    assert_eq!(
        render_frame(&session, false).disposition,
        FrameDisposition::Rendered
    );
    assert!(texture.read_rgba().unwrap().iter().any(|byte| *byte != 0));

    let replacement = texture.allocate_replacement(48, 24).unwrap();
    let replacement_name = replacement.0.get();
    let replacement_extent = RenderTargetExtent::new(48, 24, 1.0);
    let operation = session
        .set_opengl_borrowed_texture_target(&OpenGLBorrowedTextureDescriptor::new(
            replacement_extent.clone(),
            48,
            24,
            texture.descriptor(),
            replacement_name,
            gl_api::TEXTURE_2D,
        ))
        .unwrap();
    finish_unit(&session, operation);
    texture.adopt(replacement, 48, 24).unwrap();
    drop(
        map.resize(crate::LogicalExtent {
            width: 48,
            height: 24,
            scale_factor: 1.0,
        })
        .unwrap(),
    );
    await_runtime_barrier(&runtime);
    assert_eq!(
        render_frame(&session, false).disposition,
        FrameDisposition::Rendered
    );
    assert!(texture.read_rgba().unwrap().iter().any(|byte| *byte != 0));
    assert_eq!(session.snapshot().unwrap().extent, replacement_extent);

    close_session(session);
    map.close().unwrap();
    runtime.close_and_wait();
}

#[cfg(mln_webgpu_backend)]
#[test]
fn webgpu_surface_session_renders_into_the_browser_canvas() {
    if !crate::supported_render_backends().contains(RenderBackendMask::WEBGPU) {
        return;
    }

    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = crate::completion::blocking(MapHandle::with_options(
        &runtime,
        &MapOptions::new(64, 64, 1.0),
    ));
    let context = WebGpuTestContext::new().unwrap();
    let surface = WebGpuTestSurface::new(&context, 64, 64).unwrap();
    let session = finish_attachment(
        map.attach_webgpu_surface(
            &surface.descriptor(RenderTargetExtent::new(64, 64, 1.0), &context),
            caller_attach_options(),
        )
        .unwrap(),
    )
    .unwrap();
    map.set_style_json(QUERY_STYLE_JSON.as_bytes()).unwrap();
    await_runtime_barrier(&runtime);
    assert_eq!(
        render_frame(&session, false).disposition,
        FrameDisposition::Rendered
    );
    assert!(
        surface
            .read_rgba(64, 64)
            .unwrap()
            .chunks_exact(4)
            .any(|pixel| pixel == QUERY_STYLE_BACKGROUND_RGBA)
    );

    close_session(session);
    map.close().unwrap();
    runtime.close_and_wait();
}

#[cfg(mln_webgpu_backend)]
#[test]
fn webgpu_borrowed_texture_session_renders_into_a_host_texture() {
    if !crate::supported_render_backends().contains(RenderBackendMask::WEBGPU) {
        return;
    }

    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = crate::completion::blocking(MapHandle::with_options(
        &runtime,
        &MapOptions::new(64, 64, 1.0),
    ));
    let (context, texture, session) =
        create_webgpu_borrowed_texture_session(&map, RenderTargetExtent::new(64, 64, 1.0)).unwrap();
    map.set_style_json(QUERY_STYLE_JSON.as_bytes()).unwrap();
    await_runtime_barrier(&runtime);
    assert_eq!(
        render_frame(&session, false).disposition,
        FrameDisposition::Rendered
    );
    assert!(
        texture
            .read_rgba(&context)
            .unwrap()
            .chunks_exact(4)
            .any(|pixel| pixel == QUERY_STYLE_BACKGROUND_RGBA)
    );

    close_session(session);
    map.close().unwrap();
    runtime.close_and_wait();
}

#[test]
fn feature_state_and_rendered_queries_copy_native_results() {
    if !has_test_owned_texture_session_backend() {
        return;
    }

    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = crate::completion::blocking(MapHandle::with_options(
        &runtime,
        &MapOptions::new(64, 64, 1.0),
    ));
    let (_context, session) =
        create_owned_texture_session(&map, RenderTargetExtent::new(64, 64, 1.0)).unwrap();
    map.set_style_json(FEATURE_STATE_STYLE_JSON.as_bytes())
        .unwrap();
    await_runtime_barrier(&runtime);
    assert_eq!(
        render_frame(&session, false).disposition,
        FrameDisposition::Rendered
    );

    let selector = FeatureStateSelector::new("point").with_feature_id("feature-1");
    crate::completion::blocking(map.set_feature_state(&selector, br#"{"hover":true,"count":3}"#));
    let state: JsonValue = serde_json::from_slice(&crate::completion::blocking(
        map.get_feature_state(&selector),
    ))
    .unwrap();
    assert_eq!(state["hover"], true);
    assert_eq!(state["count"], 3);

    let deadline = Instant::now() + Duration::from_secs(5);
    let features = loop {
        let query = session
            .query_rendered_features(
                &RenderedQueryGeometry::box_(ScreenBox::new(
                    ScreenPoint::new(0.0, 0.0),
                    ScreenPoint::new(64.0, 64.0),
                )),
                None,
            )
            .unwrap();
        wait_until_completed(&session, &query);
        let features = query.take().unwrap();
        if !features.is_empty() {
            break features;
        }
        assert!(
            Instant::now() < deadline,
            "rendered feature did not become queryable"
        );
        map.request_repaint().unwrap();
        await_runtime_barrier(&runtime);
        let _ = render_frame(&session, false);
    };
    assert_eq!(features[0].source_id.as_deref(), Some("point"));
    assert_eq!(features[0].source_layer_id, None);
    assert_eq!(feature_json(&features[0])["id"], "feature-1");
    let rendered_state: JsonValue =
        serde_json::from_slice(features[0].state.as_deref().unwrap()).unwrap();
    assert_eq!(rendered_state, json!({"hover": true, "count": 3}));

    let source_features = take_features(
        &session,
        session.query_source_features("point", None).unwrap(),
    );
    assert_eq!(source_features.len(), 1);
    assert_eq!(source_features[0].source_id.as_deref(), Some("point"));
    assert_eq!(feature_json(&source_features[0])["id"], "feature-1");

    let oversized = take_features(
        &session,
        session
            .query_rendered_features(
                &RenderedQueryGeometry::box_(ScreenBox::new(
                    ScreenPoint::new(-4096.0, -4096.0),
                    ScreenPoint::new(4096.0, 4096.0),
                )),
                None,
            )
            .unwrap(),
    );
    assert_eq!(oversized.len(), 1);

    let inverted = take_features(
        &session,
        session
            .query_rendered_features(
                &RenderedQueryGeometry::box_(ScreenBox::new(
                    ScreenPoint::new(4096.0, 4096.0),
                    ScreenPoint::new(-4096.0, -4096.0),
                )),
                None,
            )
            .unwrap(),
    );
    assert_eq!(inverted.len(), 1);

    let offscreen = take_features(
        &session,
        session
            .query_rendered_features(
                &RenderedQueryGeometry::box_(ScreenBox::new(
                    ScreenPoint::new(512.0, 512.0),
                    ScreenPoint::new(1024.0, 1024.0),
                )),
                None,
            )
            .unwrap(),
    );
    assert!(offscreen.is_empty());

    // Feature state belongs to the map, so a style replacement copies it into
    // the reloaded style instead of dropping it.
    crate::completion::blocking(map.set_style_json(FEATURE_STATE_STYLE_JSON.as_bytes()));
    let copied: JsonValue = serde_json::from_slice(&crate::completion::blocking(
        map.get_feature_state(&selector),
    ))
    .unwrap();
    assert_eq!(copied["hover"], true);
    assert_eq!(copied["count"], 3);

    // A selector with a state key removes only that key.
    let hover_selector = FeatureStateSelector::new("point")
        .with_feature_id("feature-1")
        .with_state_key("hover")
        .unwrap();
    crate::completion::blocking(map.remove_feature_state(&hover_selector));
    let after_remove: JsonValue = serde_json::from_slice(&crate::completion::blocking(
        map.get_feature_state(&selector),
    ))
    .unwrap();
    assert_eq!(after_remove["count"], 3);
    assert!(after_remove.get("hover").is_none());

    crate::completion::blocking(map.remove_feature_state(&selector));
    close_session(session);
    map.close().unwrap();
    runtime.close_and_wait();
}

#[test]
fn cluster_feature_extensions_copy_values_and_feature_collections() {
    if !has_test_owned_texture_session_backend() {
        return;
    }

    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = crate::completion::blocking(MapHandle::with_options(
        &runtime,
        &MapOptions::new(64, 64, 1.0),
    ));
    let (_context, session) =
        create_owned_texture_session(&map, RenderTargetExtent::new(64, 64, 1.0)).unwrap();
    map.set_style_json(CLUSTER_BASE_STYLE_JSON.as_bytes())
        .unwrap();

    let data = serde_json::to_vec(&json!({
        "type": "FeatureCollection",
        "features": [
            {"type":"Feature","geometry":{"type":"Point","coordinates":[0.0,0.0]},"properties":{"name":"one","weight":1}},
            {"type":"Feature","geometry":{"type":"Point","coordinates":[0.001,0.001]},"properties":{"name":"two","weight":2}},
            {"type":"Feature","geometry":{"type":"Point","coordinates":[0.002,0.002]},"properties":{"name":"three","weight":3}}
        ]
    }))
    .unwrap();
    let mut options = GeoJsonSourceOptions::default();
    options.cluster = Some(true);
    options.cluster_radius = Some(60);
    options.cluster_min_points = Some(2);
    options.cluster_max_zoom = Some(17.0);
    options.cluster_properties =
        Some(serde_json::to_vec(&json!({"weight_sum":["+",["get","weight"]]})).unwrap());
    let data = crate::GeoJsonSourceDataHandle::new(&data, Some(&options)).unwrap();
    map.add_geojson_source_data("cluster-source", &data)
        .unwrap();
    map.add_style_layer_json(
        br##"{"id":"cluster-circle","type":"circle","source":"cluster-source","filter":["has","point_count"],"paint":{"circle-color":"#2563eb","circle-radius":20}}"##,
        None,
    )
    .unwrap();
    await_runtime_barrier(&runtime);
    assert_eq!(
        render_frame(&session, false).disposition,
        FrameDisposition::Rendered
    );

    let deadline = Instant::now() + Duration::from_secs(5);
    let cluster = loop {
        let queried = take_features(
            &session,
            session
                .query_rendered_features(
                    &RenderedQueryGeometry::box_(ScreenBox::new(
                        ScreenPoint::new(0.0, 0.0),
                        ScreenPoint::new(64.0, 64.0),
                    )),
                    None,
                )
                .unwrap(),
        );
        if let Some(cluster) = queried.into_iter().next() {
            break cluster;
        }
        assert!(
            Instant::now() < deadline,
            "cluster did not become queryable"
        );
        map.request_repaint().unwrap();
        await_runtime_barrier(&runtime);
        let _ = render_frame(&session, false);
    };
    let feature_value = feature_json(&cluster);
    assert_eq!(feature_value["properties"]["point_count"], 3);
    assert_eq!(
        feature_value["properties"]["weight_sum"].as_f64(),
        Some(6.0)
    );
    let feature = cluster.feature;

    let children = take_json(
        &session,
        session
            .query_feature_extension("cluster-source", &feature, "supercluster", "children", None)
            .unwrap(),
    );
    assert!(!children["features"].as_array().unwrap().is_empty());

    let expansion_zoom = take_json(
        &session,
        session
            .query_feature_extension(
                "cluster-source",
                &feature,
                "supercluster",
                "expansion-zoom",
                None,
            )
            .unwrap(),
    );
    assert!(expansion_zoom.is_u64());

    close_session(session);
    map.close().unwrap();
    runtime.close_and_wait();
}

#[test]
fn live_session_blocks_map_close_and_drop_reports_the_leaked_map() {
    if !has_test_owned_texture_session_backend() {
        return;
    }

    let leaks = Arc::new(Mutex::new(Vec::new()));
    let sink = Arc::clone(&leaks);
    assert!(!crate::set_leak_reporter(Some(Box::new(move |leak| {
        sink.lock().unwrap().push(leak);
    }))));

    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = crate::completion::blocking(MapHandle::with_options(
        &runtime,
        &MapOptions::new(32, 32, 1.0),
    ));
    let (_context, session) =
        create_owned_texture_session(&map, RenderTargetExtent::new(32, 32, 1.0)).unwrap();

    let close_error = map.close().unwrap_err();
    assert_eq!(close_error.kind(), ErrorKind::InvalidState);
    let map = close_error.into_handle();
    drop(map);
    crate::set_leak_reporter(None);

    let reported = leaks.lock().unwrap().clone();
    assert_eq!(reported.len(), 1);
    assert_eq!(reported[0].type_name, "mln_map");
    assert_ne!(reported[0].id, 0);

    close_session(session);
    crate::completion::blocking(crate::completion::submit(
        |completion| unsafe { sys::mln_map_release(sys::mln_map(reported[0].id), completion) },
        crate::completion::unit,
    ));
    runtime.close_and_wait();
}

#[test]
fn sustained_frame_demands_outlast_the_texture_ring_depth() {
    if !has_test_owned_texture_session_backend() {
        return;
    }

    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = crate::completion::blocking(MapHandle::with_options(
        &runtime,
        &MapOptions::new(32, 32, 1.0),
    ));
    let (_context, session) =
        create_owned_texture_session(&map, RenderTargetExtent::new(32, 32, 1.0)).unwrap();
    map.set_style_json(QUERY_STYLE_JSON.as_bytes()).unwrap();
    await_runtime_barrier(&runtime);

    for _ in 0..64 {
        map.request_repaint().unwrap();
        await_runtime_barrier(&runtime);
        assert_eq!(
            render_frame(&session, false).disposition,
            FrameDisposition::Rendered
        );
    }
    assert!(session.snapshot().unwrap().frame_generation >= 64);

    close_session(session);
    map.close().unwrap();
    runtime.close_and_wait();
}

#[test]
fn cloned_session_controls_can_be_used_from_another_thread() {
    if !has_test_owned_texture_session_backend() {
        return;
    }

    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = crate::completion::blocking(MapHandle::with_options(
        &runtime,
        &MapOptions::new(32, 32, 1.0),
    ));
    let (_context, session) =
        create_owned_texture_session(&map, RenderTargetExtent::new(32, 32, 1.0)).unwrap();
    let clone = session.clone();
    let snapshot = std::thread::spawn(move || clone.snapshot().unwrap())
        .join()
        .unwrap();
    assert_eq!(snapshot.state, RenderSessionLifecycle::Attached);

    close_session(session);
    map.close().unwrap();
    runtime.close_and_wait();
}

#[test]
fn texture_readback_before_a_frame_reports_invalid_state() {
    if !has_test_owned_texture_session_backend() {
        return;
    }

    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = crate::completion::blocking(MapHandle::with_options(
        &runtime,
        &MapOptions::new(16, 16, 1.0),
    ));
    let (_context, session) =
        create_owned_texture_session(&map, RenderTargetExtent::new(16, 16, 1.0)).unwrap();
    if session.capabilities().unwrap().readback {
        let operation = session.read_premultiplied_rgba8().unwrap();
        while !operation.is_completed().unwrap() {
            let _ = session.service_driver_work(64);
            std::thread::yield_now();
        }
        assert_eq!(
            operation.terminal_status().unwrap(),
            sys::MLN_STATUS_INVALID_STATE
        );
    }

    close_session(session);
    map.close().unwrap();
    runtime.close_and_wait();
}
