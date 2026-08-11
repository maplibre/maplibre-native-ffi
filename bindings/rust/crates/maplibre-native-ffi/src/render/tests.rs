use std::cell::Cell;
use std::error::Error as StdError;
#[cfg(not(target_os = "emscripten"))]
use std::ffi::CStr;
use std::ffi::CString;
#[cfg(target_os = "windows")]
use std::ffi::c_char;
#[cfg(any(target_os = "linux", target_os = "android", target_os = "windows"))]
use std::ffi::c_void;
use std::marker::PhantomData;
use std::rc::Rc;
use std::sync::atomic::{AtomicI32, AtomicUsize, Ordering};
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
#[cfg(any(target_os = "linux", target_os = "android"))]
use libloading::Library;
use static_assertions::assert_not_impl_any;
#[cfg(target_os = "emscripten")]
use webgl_gl as gl_api;

use super::*;
use crate::logging::test_support::LoggingTestGuard;
use crate::{
    AnimationOptions, CameraChangeMode, CameraOptions, ErrorKind, GeoJsonSourceOptions, LatLng,
    LogSeverity, LogSeverityMask, MapAttachRef, MapHandle, MapMode, MapOptions,
    OpenGLContextProviderMask, RenderBackendMask, RuntimeEventPayload, RuntimeEventType,
    RuntimeHandle, ScreenBox, ScreenPoint,
};

assert_not_impl_any!(NativePointer: Send, Sync);
assert_not_impl_any!(FrameNativePointer<'static>: Send, Sync);
assert_not_impl_any!(FrameOpenGLTextureName<'static>: Send, Sync);
assert_not_impl_any!(RenderSessionHandle: Send, Sync);
assert_not_impl_any!(MetalOwnedTextureFrameHandle: Send, Sync);
assert_not_impl_any!(VulkanOwnedTextureFrameHandle: Send, Sync);
assert_not_impl_any!(OpenGLOwnedTextureFrameHandle: Send, Sync);

const FEATURE_STATE_STYLE_JSON: &str = r#"{"version":8,"sources":{"point":{"type":"geojson","data":{"type":"FeatureCollection","features":[{"type":"Feature","id":"feature-1","properties":{},"geometry":{"type":"Point","coordinates":[0,0]}}]}}},"layers":[{"id":"circle","type":"circle","source":"point","paint":{"circle-radius":["case",["boolean",["feature-state","hover"],false],10,5]}}]}"#;
const QUERY_STYLE_JSON: &str = r##"{"version":8,"sources":{"point":{"type":"geojson","data":{"type":"FeatureCollection","features":[{"type":"Feature","id":"feature-1","geometry":{"type":"Point","coordinates":[-122.4194,37.7749]},"properties":{"kind":"capital","visible":true}}]}}},"layers":[{"id":"background","type":"background","paint":{"background-color":"#d8f1ff"}},{"id":"point-circle","type":"circle","source":"point","paint":{"circle-color":"#f97316","circle-radius":12}}]}"##;
// QUERY_STYLE_JSON's background color.
const QUERY_STYLE_BACKGROUND_RGBA: [u8; 4] = [0xd8, 0xf1, 0xff, 0xff];

const CLUSTER_BASE_STYLE_JSON: &str = r##"{"version":8,"sources":{},"layers":[{"id":"background","type":"background","paint":{"background-color":"#ffffff"}}]}"##;
fn create_owned_texture_session(
    map: &MapAttachRef,
    extent: RenderTargetExtent,
) -> std::result::Result<(OwnedTextureTestContext, RenderSessionHandle), Box<dyn StdError>> {
    let backends = crate::supported_render_backends();
    if backends.contains(RenderBackendMask::METAL) {
        let context = MetalTestContext::new()?;
        let session = map.attach_metal_owned_texture(&MetalOwnedTextureDescriptor::new(
            extent,
            context.descriptor(),
        ))?;
        return Ok((OwnedTextureTestContext::Metal(context), session));
    }
    #[cfg(mln_webgpu_backend)]
    if backends.contains(RenderBackendMask::WEBGPU) {
        let context = WebGpuTestContext::new()?;
        let session = map.attach_webgpu_owned_texture(&WebGpuOwnedTextureDescriptor::new(
            extent,
            context.descriptor(),
        ))?;
        return Ok((OwnedTextureTestContext::WebGpu(context), session));
    }
    #[cfg(not(target_os = "emscripten"))]
    if backends.contains(RenderBackendMask::VULKAN) {
        let context = VulkanTestContext::new()?;
        let session = map.attach_vulkan_owned_texture(&VulkanOwnedTextureDescriptor::new(
            extent,
            context.descriptor(),
        ))?;
        return Ok((OwnedTextureTestContext::Vulkan(Box::new(context)), session));
    }
    if has_opengl_test_context_backend() {
        let context = OpenGLTestContext::new(extent.width, extent.height)?;
        let session = map.attach_opengl_owned_texture(&OpenGLOwnedTextureDescriptor::new(
            extent,
            context.descriptor(),
        ))?;
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

    #[cfg(any(target_os = "linux", target_os = "android"))]
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
        target_os = "windows",
        target_os = "emscripten"
    )))]
    {
        // The Rust test helper implements Linux EGL, Windows WGL, and browser WebGL.
        false
    }
}

fn create_opengl_owned_texture_session(
    map: &MapAttachRef,
    extent: RenderTargetExtent,
) -> std::result::Result<(OpenGLTestContext, RenderSessionHandle), Box<dyn StdError>> {
    let backends = crate::supported_render_backends();
    if !backends.contains(RenderBackendMask::OPENGL) {
        return Err("native library does not support OpenGL owned texture sessions".into());
    }
    let context = OpenGLTestContext::new(extent.width, extent.height)?;
    let session = map.attach_opengl_owned_texture(&OpenGLOwnedTextureDescriptor::new(
        extent,
        context.descriptor(),
    ))?;
    Ok((context, session))
}

fn create_opengl_surface_session(
    map: &MapAttachRef,
    extent: RenderTargetExtent,
) -> std::result::Result<(OpenGLTestContext, RenderSessionHandle), Box<dyn StdError>> {
    let backends = crate::supported_render_backends();
    if !backends.contains(RenderBackendMask::OPENGL) {
        return Err("native library does not support OpenGL surface sessions".into());
    }
    let context = OpenGLTestContext::new(extent.width, extent.height)?;
    let session = map.attach_opengl_surface(&OpenGLSurfaceDescriptor::new(
        extent,
        context.descriptor(),
        context.surface(),
    ))?;
    Ok((context, session))
}

fn create_opengl_borrowed_texture_session(
    map: &MapAttachRef,
    extent: RenderTargetExtent,
) -> std::result::Result<(OpenGLBorrowedTexture, RenderSessionHandle), Box<dyn StdError>> {
    let backends = crate::supported_render_backends();
    if !backends.contains(RenderBackendMask::OPENGL) {
        return Err("native library does not support OpenGL borrowed texture sessions".into());
    }
    let (physical_width, physical_height) = extent.physical_size()?;
    let texture = OpenGLBorrowedTexture::new(physical_width, physical_height)?;
    let session = map.attach_opengl_borrowed_texture(&OpenGLBorrowedTextureDescriptor::new(
        extent,
        physical_width,
        physical_height,
        texture.descriptor(),
        texture.name(),
        gl_api::TEXTURE_2D,
    ))?;
    Ok((texture, session))
}

/// Attaches a WebGPU caller-owned texture session, the way a browser host that
/// allocates its own render target does.
#[cfg(mln_webgpu_backend)]
fn create_webgpu_borrowed_texture_session(
    map: &MapAttachRef,
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
    let session = map.attach_webgpu_borrowed_texture(&texture.descriptor(extent, &context))?;
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
        map: &MapAttachRef,
        extent: RenderTargetExtent,
    ) -> Result<RenderSessionHandle> {
        match self {
            Self::Metal(context) => map.attach_metal_owned_texture(
                &MetalOwnedTextureDescriptor::new(extent, context.descriptor()),
            ),
            #[cfg(not(target_os = "emscripten"))]
            Self::Vulkan(context) => map.attach_vulkan_owned_texture(
                &VulkanOwnedTextureDescriptor::new(extent, context.descriptor()),
            ),
            #[cfg(mln_webgpu_backend)]
            Self::WebGpu(context) => map.attach_webgpu_owned_texture(
                &WebGpuOwnedTextureDescriptor::new(extent, context.descriptor()),
            ),
            Self::OpenGL(context) => map.attach_opengl_owned_texture(
                &OpenGLOwnedTextureDescriptor::new(extent, context.descriptor()),
            ),
        }
    }

    /// Hands the session a placeholder target of this context's own backend.
    /// The setter must match the backend, or an unsupported build answers in
    /// place of the target-kind rejection. The placeholder is never
    /// dereferenced, because the kind is checked before the descriptor.
    fn set_placeholder_borrowed_target(
        &self,
        session: &RenderSessionHandle,
        extent: &RenderTargetExtent,
    ) -> Result<()> {
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
                let Ok(frame) = session.acquire_metal_owned_texture_frame() else {
                    return false;
                };
                let metadata = frame.frame().unwrap();
                let matches = (metadata.width, metadata.height)
                    == (expected.width, expected.height)
                    && metadata.scale_factor == expected.scale_factor;
                frame.close().unwrap();
                matches
            }
            #[cfg(mln_webgpu_backend)]
            Self::WebGpu(_) => {
                let Ok(frame) = session.acquire_webgpu_owned_texture_frame() else {
                    return false;
                };
                let metadata = frame.frame().unwrap();
                let matches = metadata.width == expected.width
                    && metadata.height == expected.height
                    && metadata.scale_factor == expected.scale_factor;
                frame.close().unwrap();
                matches
            }
            #[cfg(not(target_os = "emscripten"))]
            Self::Vulkan(_) => {
                let Ok(frame) = session.acquire_vulkan_owned_texture_frame() else {
                    return false;
                };
                let metadata = frame.frame().unwrap();
                let matches = (metadata.width, metadata.height)
                    == (expected.width, expected.height)
                    && metadata.scale_factor == expected.scale_factor;
                frame.close().unwrap();
                matches
            }
            Self::OpenGL(_) => {
                let Ok(frame) = session.acquire_opengl_owned_texture_frame() else {
                    return false;
                };
                let metadata = frame.frame().unwrap();
                let matches = (metadata.width, metadata.height)
                    == (expected.width, expected.height)
                    && metadata.scale_factor == expected.scale_factor;
                frame.close().unwrap();
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
    #[cfg(any(target_os = "linux", target_os = "android"))]
    platform: EglTestContext,
    #[cfg(target_os = "windows")]
    platform: WglTestContext,
    #[cfg(target_os = "emscripten")]
    platform: WebGlTestContext,
}

#[cfg(any(target_os = "linux", target_os = "android"))]
struct EglTestContext {
    egl: egl::Egl,
    _lib: Library,
    display: EGLDisplay,
    config: EGLConfig,
    surface: EGLSurface,
    context: EGLContext,
}

#[cfg(any(target_os = "linux", target_os = "android"))]
impl EglTestContext {
    fn new() -> std::result::Result<Self, Box<dyn StdError>> {
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
            8,
            egl::HEIGHT as EGLint,
            8,
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

#[cfg(any(target_os = "linux", target_os = "android"))]
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
#[cfg(any(target_os = "linux", target_os = "android"))]
struct DedicatedEglTestSurface {
    egl: egl::Egl,
    _lib: Library,
    display: EGLDisplay,
    config: EGLConfig,
    surface: EGLSurface,
}

#[cfg(any(target_os = "linux", target_os = "android"))]
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

#[cfg(any(target_os = "linux", target_os = "android"))]
impl Drop for DedicatedEglTestSurface {
    fn drop(&mut self) {
        unsafe {
            self.egl.DestroySurface(self.display, self.surface);
            self.egl.Terminate(self.display);
        }
    }
}

/// Opens and initializes the EGL display the OpenGL fixtures render through.
#[cfg(any(target_os = "linux", target_os = "android"))]
fn open_egl_display(egl: &egl::Egl) -> std::result::Result<EGLDisplay, Box<dyn StdError>> {
    // A desktop Linux run without a display server needs Mesa's surfaceless
    // platform. Device EGL implementations ship no such platform, so they take
    // the default display.
    #[cfg(any(target_env = "ohos", target_os = "android"))]
    let display = unsafe { egl.GetDisplay(egl::DEFAULT_DISPLAY as *mut c_void) };
    #[cfg(any(target_env = "ohos", target_os = "android"))]
    let display_operation = "eglGetDisplay";
    #[cfg(not(any(target_env = "ohos", target_os = "android")))]
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
    #[cfg(not(any(target_env = "ohos", target_os = "android")))]
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
#[cfg(any(target_os = "linux", target_os = "android"))]
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

#[cfg(any(target_os = "linux", target_os = "android"))]
fn load_egl_library() -> std::result::Result<Library, Box<dyn StdError>> {
    unsafe { Library::new("libEGL.so.1") }
        .or_else(|_| unsafe { Library::new("libEGL.so") })
        .map_err(|error| format!("failed to load libEGL: {error}").into())
}

#[cfg(any(target_os = "linux", target_os = "android"))]
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
        OpenGLContextDescriptor::WebGl(WebGlContextDescriptor::new(self.context))
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

    #[cfg(any(target_os = "linux", target_os = "android"))]
    fn new_platform(_width: u32, _height: u32) -> std::result::Result<Self, Box<dyn StdError>> {
        let platform = EglTestContext::new()?;
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
        #[cfg(any(target_os = "linux", target_os = "android"))]
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

fn wait_for_runtime_event(runtime: &mut RuntimeHandle, event_type: RuntimeEventType) -> bool {
    let deadline = Instant::now() + Duration::from_secs(5);
    while Instant::now() < deadline {
        let _ = runtime.pump(Some(Duration::ZERO));
        let found = runtime
            .drain_events(0)
            .map(|batch| batch.iter().any(|event| event.event_type() == event_type))
            .unwrap_or(false);
        if found {
            return true;
        }
        std::thread::sleep(Duration::from_millis(10));
    }
    false
}

fn static_map_options(width: u32, height: u32, scale_factor: f64) -> MapOptions {
    let mut options = MapOptions::new(width, height, scale_factor);
    options.mode = MapMode::Static;
    options
}

fn load_feature_state_style(
    runtime: &mut RuntimeHandle,
    map: &MapHandle,
    session: &RenderSessionHandle,
) {
    map.set_style_json(FEATURE_STATE_STYLE_JSON.as_bytes())
        .unwrap();
    assert!(wait_for_runtime_event(
        runtime,
        RuntimeEventType::MapRenderUpdateAvailable
    ));
    assert_eq!(session.render_update().unwrap(), RenderResult::Rendered);
}

fn load_query_style(runtime: &mut RuntimeHandle, map: &MapHandle, session: &RenderSessionHandle) {
    let mut camera = CameraOptions::default();
    camera.center = Some(LatLng::new(37.7749, -122.4194));
    camera.zoom = Some(10.0);
    map.jump_to(&camera).unwrap();
    map.set_style_json(QUERY_STYLE_JSON.as_bytes()).unwrap();
    render_available_updates(runtime, session, 5);
}

fn cluster_point(latitude: f64, longitude: f64, name: &str, weight: u64) -> JsonValue {
    json!({
        "type": "Feature",
        "geometry": {"type": "Point", "coordinates": [longitude, latitude]},
        "properties": {"name": name, "weight": weight},
    })
}

/// Builds the clustered source through the GeoJSON data adder so the render
/// tests exercise `GeoJsonSourceOptions`.
fn load_cluster_style(runtime: &mut RuntimeHandle, map: &MapHandle, session: &RenderSessionHandle) {
    let mut camera = CameraOptions::default();
    camera.center = Some(LatLng::new(0.0, 0.0));
    camera.zoom = Some(0.0);
    map.jump_to(&camera).unwrap();
    map.set_style_json(CLUSTER_BASE_STYLE_JSON.as_bytes())
        .unwrap();

    let data = serde_json::to_vec(&json!({
        "type": "FeatureCollection",
        "features": [
            cluster_point(0.0, 0.0, "one", 1),
            cluster_point(0.001, 0.001, "two", 2),
            cluster_point(0.002, 0.002, "three", 3),
        ],
    }))
    .unwrap();
    let mut options = GeoJsonSourceOptions::default();
    options.cluster = Some(true);
    options.cluster_radius = Some(60);
    options.cluster_min_points = Some(2);
    options.cluster_max_zoom = Some(17.0);
    options.cluster_properties =
        Some(serde_json::to_vec(&json!({"weight_sum": ["+", ["get", "weight"]]})).unwrap());
    map.add_geojson_source_data("cluster-source", &data, Some(&options))
        .unwrap();

    let layer = serde_json::to_vec(&json!({
        "id": "cluster-circle",
        "type": "circle",
        "source": "cluster-source",
        "filter": ["has", "point_count"],
        "paint": {"circle-color": "#2563eb", "circle-radius": 20.0},
    }))
    .unwrap();
    map.add_style_layer_json(&layer, None).unwrap();

    render_available_updates(runtime, session, 5);
}

fn numeric_member(value: &JsonValue, key: &str) -> Option<f64> {
    json_member(value, key)?.as_f64()
}

fn render_available_updates(
    runtime: &mut RuntimeHandle,
    session: &RenderSessionHandle,
    count: usize,
) {
    for _ in 0..count {
        if wait_for_runtime_event(runtime, RuntimeEventType::MapRenderUpdateAvailable) {
            let _ = session.render_update();
        }
    }
}

fn render_pending_updates(runtime: &mut RuntimeHandle, session: &RenderSessionHandle) {
    let _ = runtime.pump(Some(Duration::ZERO));
    let updates = runtime
        .drain_events(0)
        .map(|batch| {
            batch
                .iter()
                .filter(|event| event.event_type() == RuntimeEventType::MapRenderUpdateAvailable)
                .count()
        })
        .unwrap_or(0);
    for _ in 0..updates {
        let _ = session.render_update();
    }
}

fn wait_for_rendered_feature(
    runtime: &mut RuntimeHandle,
    session: &RenderSessionHandle,
    geometry: &RenderedQueryGeometry,
    options: &RenderedFeatureQueryOptions,
    description: &str,
) -> JsonValue {
    let deadline = Instant::now() + Duration::from_secs(5);
    while Instant::now() < deadline {
        let features: Vec<JsonValue> = serde_json::from_slice(
            &session
                .query_rendered_features(geometry, Some(options))
                .unwrap(),
        )
        .unwrap();
        if features.len() == 1 {
            return features.into_iter().next().unwrap();
        }
        render_pending_updates(runtime, session);
        std::thread::sleep(Duration::from_millis(1));
    }
    panic!("timed out waiting for {description}");
}

fn wait_for_source_feature(
    runtime: &mut RuntimeHandle,
    session: &RenderSessionHandle,
    source_id: &str,
    options: &SourceFeatureQueryOptions,
    description: &str,
) -> JsonValue {
    let deadline = Instant::now() + Duration::from_secs(5);
    while Instant::now() < deadline {
        let features: Vec<JsonValue> = serde_json::from_slice(
            &session
                .query_source_features(source_id, Some(options))
                .unwrap(),
        )
        .unwrap();
        if features.len() == 1 {
            return features.into_iter().next().unwrap();
        }
        render_pending_updates(runtime, session);
        std::thread::sleep(Duration::from_millis(1));
    }
    panic!("timed out waiting for {description}");
}

fn single_cluster_leaf(
    session: &RenderSessionHandle,
    feature: &JsonValue,
    offset: u64,
) -> JsonValue {
    let feature = serde_json::to_vec(feature).unwrap();
    let arguments = serde_json::to_vec(&json!({"limit": 1, "offset": offset})).unwrap();
    let result = session
        .query_feature_extension(
            "cluster-source",
            &feature,
            "supercluster",
            "leaves",
            Some(&arguments),
        )
        .unwrap();
    let result: JsonValue = serde_json::from_slice(&result).unwrap();
    let leaves = result["features"]
        .as_array()
        .expect("expected leaves feature collection");
    assert_eq!(leaves.len(), 1);
    leaves[0].clone()
}

fn queried_feature(value: &JsonValue) -> &JsonValue {
    &value["feature"]
}

fn feature_member<'a>(feature: &'a JsonValue, key: &str) -> Option<&'a JsonValue> {
    feature.get("properties")?.get(key)
}

fn json_member<'a>(value: &'a JsonValue, key: &str) -> Option<&'a JsonValue> {
    value.as_object()?.get(key)
}

fn assert_json_member(value: &JsonValue, key: &str, expected: &JsonValue) {
    assert_eq!(json_member(value, key), Some(expected));
}

fn assert_point_geometry_close(geometry: &JsonValue, expected: LatLng) {
    assert_eq!(geometry["type"], json!("Point"));
    let coordinates = geometry["coordinates"]
        .as_array()
        .expect("expected point coordinates");
    assert!((coordinates[1].as_f64().unwrap() - expected.latitude).abs() < 0.0001);
    assert!((coordinates[0].as_f64().unwrap() - expected.longitude).abs() < 0.0001);
}

#[test]
// Spec coverage: BND-161.
fn native_pointer_round_trips_address() {
    // SAFETY: Test uses a dummy opaque address and does not dereference it.
    let pointer = unsafe { NativePointer::from_address(0x1234) };
    assert_eq!(pointer.address(), 0x1234);
    // SAFETY: Test only verifies address reconstruction; it does not dereference.
    assert_eq!(unsafe { pointer.as_ptr::<u8>() } as usize, 0x1234);
    assert!(NativePointer::NULL.is_null());
}

#[test]
// Spec coverage: BND-161.
fn opengl_context_provider_mask_is_exposed_semantically() {
    let providers = crate::supported_opengl_context_providers();
    let backends = crate::supported_render_backends();
    if backends.contains(RenderBackendMask::OPENGL) {
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
// Spec coverage: BND-162.
fn opengl_owned_texture_session_attaches_with_platform_context() {
    if !has_opengl_test_context_backend() {
        return;
    }
    let mut runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::new(64, 64, 1.0)).unwrap();
    let (_context, session) = create_opengl_owned_texture_session(
        &map.attach_ref().unwrap(),
        RenderTargetExtent::new(32, 16, 1.0),
    )
    .expect("OpenGL owned texture test session should attach when OpenGL is supported");

    let error = session.acquire_opengl_owned_texture_frame().unwrap_err();
    assert!(matches!(
        error.kind(),
        ErrorKind::InvalidState | ErrorKind::Unsupported
    ));

    map.set_style_json(QUERY_STYLE_JSON.as_bytes()).unwrap();
    assert!(wait_for_runtime_event(
        &mut runtime,
        RuntimeEventType::MapRenderUpdateAvailable
    ));
    assert_eq!(session.render_update().unwrap(), RenderResult::Rendered);

    let frame = session.acquire_opengl_owned_texture_frame().unwrap();
    assert_eq!(frame.frame().unwrap().width, 32);
    assert_eq!(frame.frame().unwrap().height, 16);
    assert_eq!(frame.frame().unwrap().target, gl_api::TEXTURE_2D);
    assert_eq!(frame.frame().unwrap().internal_format, gl_api::RGBA8);
    assert_eq!(frame.frame().unwrap().format, gl_api::RGBA);
    assert_eq!(frame.frame().unwrap().type_, gl_api::UNSIGNED_BYTE);
    assert!(!frame.texture().unwrap().is_zero());
    frame.close().unwrap();

    #[cfg(target_os = "windows")]
    {
        let info = session.texture_image_info().unwrap();
        assert_eq!((info.width, info.height), (32, 16));
        assert!(info.stride >= info.width * 4);
        assert!(info.byte_length >= info.stride as usize * info.height as usize);

        let mut undersized = vec![0x7f; info.byte_length - 1];
        let error = session
            .read_premultiplied_rgba8_into(&mut undersized)
            .unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
        assert_eq!(error.raw_status(), Some(sys::MLN_STATUS_INVALID_ARGUMENT));
        assert!(undersized.iter().all(|byte| *byte == 0x7f));

        let mut pixels = vec![0; info.byte_length];
        let copied_info = session.read_premultiplied_rgba8_into(&mut pixels).unwrap();
        assert_eq!(copied_info, info);
        assert!(pixels.iter().any(|byte| *byte != 0));
    }

    session.close().unwrap();
    map.close().unwrap();
    runtime.close().unwrap();
}

/// Whether this platform has an OpenGL surface for a session to render into.
///
/// A browser has none: a WebGL context presents through its canvas, and a
/// browser host renders into a texture instead, so `mln_opengl_surface_attach`
/// has nothing to be handed. Every other OpenGL provider supplies one.
fn has_opengl_surface_test_backend() -> bool {
    has_opengl_test_context_backend()
}

#[test]
// Spec coverage: BND-162.
fn opengl_surface_session_renders_with_platform_context() {
    if !has_opengl_surface_test_backend() {
        return;
    }
    let mut runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::new(64, 64, 1.0)).unwrap();

    let (_context, session) = create_opengl_surface_session(
        &map.attach_ref().unwrap(),
        RenderTargetExtent::new(32, 16, 1.0),
    )
    .expect("OpenGL surface test session should attach when OpenGL is supported");

    map.set_style_json(QUERY_STYLE_JSON.as_bytes()).unwrap();
    assert!(wait_for_runtime_event(
        &mut runtime,
        RuntimeEventType::MapRenderUpdateAvailable
    ));
    assert_eq!(session.render_update().unwrap(), RenderResult::Rendered);

    #[cfg(any(target_os = "windows", target_os = "emscripten"))]
    {
        let pixels = _context.read_surface_rgba(32, 16).unwrap();
        assert!(pixels.iter().any(|byte| *byte != 0));
    }

    session.close().unwrap();
    map.close().unwrap();
    runtime.close().unwrap();
}

/// A dedicated session owns its thread's OpenGL context: it creates a context
/// that joins no share group, keeps it current between renders, and gives the
/// thread back when it closes.
#[cfg(any(target_os = "linux", target_os = "android"))]
#[test]
// Spec coverage: BND-162.
fn dedicated_opengl_surface_session_renders_and_keeps_its_context_current() {
    if !has_opengl_test_context_backend() {
        return;
    }
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::new(64, 64, 1.0)).unwrap();
    let surface = DedicatedEglTestSurface::new(64, 64)
        .expect("a dedicated EGL fixture reaches a display and a pbuffer surface");
    // The fixture makes no context current, so the session has nothing to
    // inherit and nothing to restore.
    assert!(!surface.has_current_context());

    let session = map
        .attach_ref()
        .unwrap()
        .attach_opengl_surface(&OpenGLSurfaceDescriptor::new(
            RenderTargetExtent::new(64, 64, 1.0),
            surface.descriptor(),
            surface.surface(),
        ))
        .expect("a dedicated OpenGL surface session attaches when EGL is supported");

    map.set_style_json(QUERY_STYLE_JSON.as_bytes()).unwrap();
    // The session owns this thread, which is also the map's, so the map only
    // reaches the style and the extent when this loop pumps the runtime.
    let deadline = Instant::now() + Duration::from_secs(5);
    let mut result = RenderResult::NoUpdate;
    while result != RenderResult::Rendered && Instant::now() < deadline {
        let _ = runtime.pump(Some(Duration::ZERO));
        result = session.render_update().unwrap();
        if result != RenderResult::Rendered {
            std::thread::sleep(Duration::from_millis(10));
        }
    }
    assert_eq!(result, RenderResult::Rendered);
    assert!(surface.has_current_context());

    session.close().unwrap();
    // Closing the session releases the thread it had taken over.
    assert!(!surface.has_current_context());
    map.close().unwrap();
    runtime.close().unwrap();
}

#[cfg(mln_webgpu_backend)]
#[test]
// Spec coverage: BND-162.
fn webgpu_surface_session_renders_and_presents_through_a_canvas() {
    if !crate::supported_render_backends().contains(RenderBackendMask::WEBGPU) {
        return;
    }
    let mut runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::new(128, 128, 1.0)).unwrap();
    let context = WebGpuTestContext::new().expect("a browser build reaches a WebGPU device");
    let surface = WebGpuTestSurface::new(&context, 128, 128)
        .expect("a WebGPU instance creates a surface over a canvas");
    let extent = RenderTargetExtent::new(128, 128, 1.0);
    let session = map
        .attach_ref()
        .unwrap()
        .attach_webgpu_surface(&surface.descriptor(extent.clone(), &context))
        .expect("WebGPU surface session should attach when WebGPU is supported");

    map.set_style_json(QUERY_STYLE_JSON.as_bytes()).unwrap();
    assert!(wait_for_runtime_event(
        &mut runtime,
        RuntimeEventType::MapRenderUpdateAvailable
    ));
    // A frame reported rendered means the surface gave up its texture and took
    // it back at present, which is the whole of the acquire and present cycle.
    assert_eq!(session.render_update().unwrap(), RenderResult::Rendered);

    // The frame reached the canvas, not just the render pass: the style paints
    // its background over every pixel.
    //
    // A browser shows a WebGPU frame once the task that drew it yields, which
    // is what expires the texture the surface handed out, so this yields before
    // reading rather than snapshotting the canvas mid-task.
    // SAFETY: yielding is legal on this thread, which entered through main.
    unsafe { emscripten_sleep(1) };
    let pixels = surface.read_rgba(128, 128).unwrap();
    assert!(
        pixels
            .chunks_exact(4)
            .any(|pixel| pixel == QUERY_STYLE_BACKGROUND_RGBA),
        "the session should present its frame to the canvas"
    );

    // A surface hands out one texture per frame, so a second frame proves the
    // first was released rather than held.
    map.set_style_json(CLUSTER_BASE_STYLE_JSON.as_bytes())
        .unwrap();
    assert!(wait_for_runtime_event(
        &mut runtime,
        RuntimeEventType::MapRenderUpdateAvailable
    ));
    assert_eq!(session.render_update().unwrap(), RenderResult::Rendered);

    // A session-owned frame belongs to the texture session kind, so this
    // reports the mismatch rather than handing back a texture it does not own.
    assert_eq!(
        session
            .acquire_webgpu_owned_texture_frame()
            .unwrap_err()
            .kind(),
        ErrorKind::Unsupported
    );

    let replacement = WebGpuTestSurface::new(&context, 128, 128)
        .expect("a WebGPU instance creates a second surface over a canvas");
    session
        .set_webgpu_surface_target(&replacement.descriptor(extent, &context))
        .expect("a replacement naming the same device and format is accepted");
    map.set_style_json(QUERY_STYLE_JSON.as_bytes()).unwrap();
    assert!(wait_for_runtime_event(
        &mut runtime,
        RuntimeEventType::MapRenderUpdateAvailable
    ));
    assert_eq!(session.render_update().unwrap(), RenderResult::Rendered);

    session.close().unwrap();
    map.close().unwrap();
    runtime.close().unwrap();
}

#[cfg(mln_webgpu_backend)]
#[test]
// Spec coverage: BND-162 and BND-171.
fn webgpu_borrowed_texture_session_renders_into_a_host_texture() {
    if !crate::supported_render_backends().contains(RenderBackendMask::WEBGPU) {
        return;
    }
    let mut runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::new(128, 128, 1.0)).unwrap();

    let (context, texture, session) = create_webgpu_borrowed_texture_session(
        &map.attach_ref().unwrap(),
        RenderTargetExtent::new(128, 128, 1.0),
    )
    .expect("WebGPU borrowed texture session should attach when WebGPU is supported");

    map.set_style_json(QUERY_STYLE_JSON.as_bytes()).unwrap();
    assert!(wait_for_runtime_event(
        &mut runtime,
        RuntimeEventType::MapRenderUpdateAvailable
    ));
    assert_eq!(session.render_update().unwrap(), RenderResult::Rendered);

    // The style paints its background over every pixel, so finding it in the
    // host's texture is what says this session rendered into the one it was
    // handed rather than only reporting that it did.
    let pixels = texture.read_rgba(&context).unwrap();
    assert!(
        pixels
            .chunks_exact(4)
            .any(|pixel| pixel == QUERY_STYLE_BACKGROUND_RGBA),
        "the session should render into the host's texture"
    );

    // A session-owned frame belongs to the other target kind, so this reports
    // the mismatch rather than handing back a texture it does not own.
    assert_eq!(
        session
            .acquire_webgpu_owned_texture_frame()
            .unwrap_err()
            .kind(),
        ErrorKind::Unsupported
    );

    session.close().unwrap();
    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-162 and BND-171.
fn opengl_borrowed_texture_session_renders_with_platform_context() {
    if !has_opengl_test_context_backend() {
        return;
    }
    let mut runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::new(128, 128, 1.0)).unwrap();

    let (texture, session) = create_opengl_borrowed_texture_session(
        &map.attach_ref().unwrap(),
        RenderTargetExtent::new(128, 128, 1.0),
    )
    .expect("OpenGL borrowed texture test session should attach when OpenGL is supported");

    map.set_style_json(QUERY_STYLE_JSON.as_bytes()).unwrap();
    assert!(wait_for_runtime_event(
        &mut runtime,
        RuntimeEventType::MapRenderUpdateAvailable
    ));
    assert_eq!(session.render_update().unwrap(), RenderResult::Rendered);

    let error = session.acquire_opengl_owned_texture_frame().unwrap_err();
    assert_eq!(error.kind(), ErrorKind::Unsupported);

    let pixels = texture.read_rgba().unwrap();
    assert!(pixels.iter().any(|byte| *byte != 0));

    session.close().unwrap();
    let pixels_after_close = texture.read_rgba().unwrap();
    assert!(pixels_after_close.iter().any(|byte| *byte != 0));
    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-175.
fn set_target_hands_a_live_session_a_new_borrowed_texture() {
    if !has_opengl_test_context_backend() {
        return;
    }
    let mut runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::new(128, 128, 1.0)).unwrap();

    let initial_extent = RenderTargetExtent::new(128, 128, 1.0);
    let (mut texture, session) =
        create_opengl_borrowed_texture_session(&map.attach_ref().unwrap(), initial_extent)
            .expect("OpenGL borrowed texture test session should attach when OpenGL is supported");

    map.set_style_json(QUERY_STYLE_JSON.as_bytes()).unwrap();
    assert!(wait_for_runtime_event(
        &mut runtime,
        RuntimeEventType::MapRenderUpdateAvailable
    ));
    assert_eq!(session.render_update().unwrap(), RenderResult::Rendered);

    // A caller-owned texture is sized by its owner, so resize is unsupported.
    let resized_extent = RenderTargetExtent::new(96, 64, 1.0);
    let resize_error = session
        .resize(
            resized_extent.width,
            resized_extent.height,
            resized_extent.scale_factor,
        )
        .unwrap_err();
    assert_eq!(resize_error.kind(), ErrorKind::Unsupported);

    let (physical_width, physical_height) = resized_extent.physical_size().unwrap();
    let replacement = texture
        .allocate_replacement(physical_width, physical_height)
        .unwrap();
    session
        .set_opengl_borrowed_texture_target(&OpenGLBorrowedTextureDescriptor::new(
            resized_extent.clone(),
            physical_width,
            physical_height,
            texture.descriptor(),
            replacement.0.get(),
            gl_api::TEXTURE_2D,
        ))
        .unwrap();
    texture
        .adopt(replacement, physical_width, physical_height)
        .unwrap();

    // QUERY_STYLE_JSON paints this background over every pixel it covers, so
    // finding it in a texture that started zeroed means the session rendered
    // into the replacement.
    const QUERY_STYLE_BACKGROUND_RGBA: [u8; 4] = [0xd8, 0xf1, 0xff, 0xff];
    assert!(
        texture.read_rgba().unwrap().iter().all(|byte| *byte == 0),
        "a freshly allocated replacement should start blank"
    );
    let deadline = Instant::now() + Duration::from_secs(5);
    let mut rendered_into_replacement = false;
    while Instant::now() < deadline && !rendered_into_replacement {
        let _ = runtime.pump(Some(Duration::ZERO));
        let _ = runtime.drain_events(0);
        if session.render_update().unwrap() == RenderResult::Rendered {
            rendered_into_replacement = texture
                .read_rgba()
                .unwrap()
                .chunks_exact(4)
                .any(|pixel| pixel == QUERY_STYLE_BACKGROUND_RGBA);
        }
        std::thread::sleep(Duration::from_millis(1));
    }
    assert!(
        rendered_into_replacement,
        "the session should render into the texture it was handed"
    );

    session.close().unwrap();
    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-176.
fn set_target_reports_unsupported_for_a_session_owned_texture() {
    if !has_test_owned_texture_session_backend() {
        return;
    }
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::new(64, 64, 1.0)).unwrap();
    let extent = RenderTargetExtent::new(64, 64, 1.0);
    let (context, session) =
        create_owned_texture_session(&map.attach_ref().unwrap(), extent.clone())
            .expect("Metal or Vulkan owned texture test session should attach when supported");

    // The setter must be this build's own backend, or "not supported by this
    // build" answers in place of the target-kind rejection under test.
    let error = context
        .set_placeholder_borrowed_target(&session, &extent)
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::Unsupported);

    assert!(session.render_update().is_ok());

    session.close().unwrap();
    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-176.
fn set_target_reports_unsupported_for_a_session_owned_opengl_texture() {
    // The owned texture fixture above covers Metal and Vulkan only. OpenGL has
    // its own fixture, so an OpenGL-only build covers the same rejection here
    // rather than skipping it.
    if !has_opengl_test_context_backend() {
        return;
    }
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::new(64, 64, 1.0)).unwrap();
    let extent = RenderTargetExtent::new(64, 64, 1.0);
    let (context, session) =
        create_opengl_owned_texture_session(&map.attach_ref().unwrap(), extent.clone())
            .expect("OpenGL owned texture test session should attach when OpenGL is supported");

    let error = session
        .set_opengl_borrowed_texture_target(&OpenGLBorrowedTextureDescriptor::new(
            extent,
            64,
            64,
            context.descriptor(),
            1,
            gl_api::TEXTURE_2D,
        ))
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::Unsupported);

    assert!(session.render_update().is_ok());

    session.close().unwrap();
    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-176.
fn set_target_reports_unsupported_for_a_target_kind_the_session_does_not_have() {
    if !has_opengl_test_context_backend() {
        return;
    }
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::new(64, 64, 1.0)).unwrap();

    let extent = RenderTargetExtent::new(64, 64, 1.0);
    let (texture, session) =
        create_opengl_borrowed_texture_session(&map.attach_ref().unwrap(), extent.clone())
            .expect("OpenGL borrowed texture test session should attach when OpenGL is supported");

    // A surface descriptor names a target this session does not have.
    let error = session
        .set_opengl_surface_target(&OpenGLSurfaceDescriptor::new(
            extent.clone(),
            texture.descriptor(),
            // SAFETY: Test passes an opaque non-null address that the rejected
            // call never dereferences.
            unsafe { NativePointer::from_address(0x1) },
        ))
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::Unsupported);

    assert!(session.render_update().is_ok());

    session.close().unwrap();
    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-167, BND-168, and BND-173.
fn frame_native_pointer_round_trips_address_without_plain_native_pointer() {
    // A backend frame pointer is tied to the borrowed frame, so a released
    // handle cannot expose one through a safe API.
    // SAFETY: Test uses a dummy opaque address and does not dereference it.
    let pointer = unsafe { FrameNativePointer::<'_>::from_ptr(0x4321usize as *mut u8) };
    // SAFETY: Test only verifies address reconstruction while the typed frame borrow is live.
    assert_eq!(unsafe { pointer.address() }, 0x4321);
    // SAFETY: Test only verifies raw pointer reconstruction; it does not dereference.
    assert_eq!(unsafe { pointer.as_ptr::<u8>() } as usize, 0x4321);
    assert!(!pointer.is_null());
}

#[test]
// Spec coverage: BND-167.
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

    let mut opengl = empty_opengl_owned_texture_frame();
    opengl.generation = 5;
    opengl.width = 256;
    opengl.height = 128;
    opengl.scale_factor = 2.0;
    opengl.frame_id = 13;
    opengl.texture = 23;
    opengl.target = 0x0de1;
    opengl.internal_format = 0x8058;
    opengl.format = 0x1908;
    opengl.type_ = 0x1401;
    let copied = OpenGLOwnedTextureFrame::from_native(&opengl);
    assert_eq!(copied.generation, 5);
    assert_eq!(
        (copied.width, copied.height, copied.scale_factor),
        (256, 128, 2.0)
    );
    assert_eq!(copied.frame_id, 13);
    assert_eq!(copied.target, 0x0de1);
    assert_eq!(copied.internal_format, 0x8058);
    assert_eq!(copied.format, 0x1908);
    assert_eq!(copied.type_, 0x1401);
}

static FRAME_RELEASE_STATUS: AtomicI32 = AtomicI32::new(sys::MLN_STATUS_OK);
static FRAME_RELEASE_COUNT: AtomicUsize = AtomicUsize::new(0);

unsafe extern "C" fn fake_session_destroy(_session: sys::mln_render_session) -> sys::mln_status {
    sys::MLN_STATUS_OK
}

unsafe extern "C" fn fake_metal_frame_release(
    _session: sys::mln_render_session,
    _frame: *const sys::mln_metal_owned_texture_frame,
) -> sys::mln_status {
    FRAME_RELEASE_COUNT.fetch_add(1, Ordering::SeqCst);
    FRAME_RELEASE_STATUS.load(Ordering::SeqCst)
}

#[test]
// Spec coverage: BND-169.
fn failed_frame_release_leaves_frame_live_for_later_release() {
    FRAME_RELEASE_STATUS.store(sys::MLN_STATUS_INVALID_STATE, Ordering::SeqCst);
    FRAME_RELEASE_COUNT.store(0, Ordering::SeqCst);
    let session = Rc::new(RenderSessionState {
        // SAFETY: The fake handle is never dereferenced, and the fake destroy
        // only reports success, so dropping the state is harmless.
        handle: unsafe {
            ThreadAffineNativeHandle::from_handle(
                sys::mln_render_session(0x0400_0000_0000_002a),
                fake_session_destroy,
                "mln_render_session",
            )
            .unwrap()
        },
        detached: Cell::new(false),
        frame_acquired: Cell::new(true),
    });
    let raw = empty_metal_owned_texture_frame();
    let frame = MetalOwnedTextureFrameHandle {
        session: Rc::clone(&session),
        frame: MetalOwnedTextureFrame::from_native(&raw),
        raw,
        closed: Cell::new(false),
        _thread_affine: PhantomData,
    };

    let error = frame
        .close_with_release(fake_metal_frame_release)
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidState);
    assert_eq!(error.raw_status(), Some(sys::MLN_STATUS_INVALID_STATE));
    assert!(session.frame_acquired.get());
    let frame = error.into_handle();
    assert!(frame.frame().is_ok());

    FRAME_RELEASE_STATUS.store(sys::MLN_STATUS_OK, Ordering::SeqCst);
    frame.close_with_release(fake_metal_frame_release).unwrap();

    assert_eq!(FRAME_RELEASE_COUNT.load(Ordering::SeqCst), 2);
    assert!(!session.frame_acquired.get());
}

#[test]
// The map scale factor is fixed at creation, so rendering a target at another
// scale warns instead of failing.
fn scale_factor_warnings_compare_the_preserved_creation_double() {
    if !has_test_owned_texture_session_backend() {
        return;
    }
    let _logging = LoggingTestGuard::new();
    // Warnings dispatch asynchronously by default, which would race this test.
    crate::set_async_log_severity_mask(LogSeverityMask::empty()).unwrap();
    let warnings = Arc::new(Mutex::new(Vec::new()));
    let captured = warnings.clone();
    crate::set_log_callback(move |record| {
        if record.message.contains("scale_factor") {
            captured
                .lock()
                .unwrap()
                .push((record.severity, record.message));
        }
        false
    })
    .unwrap();

    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    // 32.1 differs from its native float representation by more than the
    // warning tolerance, so a comparison against anything but the preserved
    // creation double would warn here.
    let map = MapHandle::with_options(&runtime, &MapOptions::new(1, 1, 32.1)).unwrap();
    let (_context, session) = create_owned_texture_session(
        &map.attach_ref().unwrap(),
        RenderTargetExtent::new(1, 1, 32.1),
    )
    .expect("Metal or Vulkan owned texture test session should attach when supported");

    assert!(warnings.lock().unwrap().is_empty());

    session.resize(1, 1, 2.0).unwrap();
    let mismatch_warnings = warnings.lock().unwrap().clone();
    assert_eq!(mismatch_warnings.len(), 1);
    assert_eq!(mismatch_warnings[0].0, LogSeverity::Warning);
    assert!(mismatch_warnings[0].1.contains('2'));

    session.resize(1, 1, 32.1).unwrap();
    assert_eq!(warnings.lock().unwrap().len(), 1);

    session.resize(1, 1, 2.0).unwrap();
    assert_eq!(warnings.lock().unwrap().len(), 2);
}

#[test]
// Spec coverage: BND-105 and BND-106.
fn feature_state_set_get_and_remove_copy_snapshots() {
    if !has_test_owned_texture_session_backend() {
        return;
    }
    let mut runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::new(64, 64, 1.0)).unwrap();
    let (_context, session) = create_owned_texture_session(
        &map.attach_ref().unwrap(),
        RenderTargetExtent::new(64, 64, 1.0),
    )
    .expect("Metal or Vulkan owned texture test session should attach when supported");
    let selector = FeatureStateSelector::new("point").with_feature_id("feature-1");
    let state = br#"{"hover":true,"radius":20}"#;

    let error = session.set_feature_state(&selector, state).unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidState);

    load_feature_state_style(&mut runtime, &map, &session);

    session.set_feature_state(&selector, state).unwrap();
    let copied: JsonValue =
        serde_json::from_slice(&session.get_feature_state(&selector).unwrap()).unwrap();
    assert_json_member(&copied, "hover", &json!(true));
    assert_json_member(&copied, "radius", &json!(20));

    let hover_selector = FeatureStateSelector::new("point")
        .with_feature_id("feature-1")
        .with_state_key("hover")
        .unwrap();
    session.remove_feature_state(&hover_selector).unwrap();
    let _ = wait_for_runtime_event(&mut runtime, RuntimeEventType::MapRenderUpdateAvailable);
    let _ = session.render_update();

    let after_remove: JsonValue =
        serde_json::from_slice(&session.get_feature_state(&selector).unwrap()).unwrap();
    assert_json_member(&after_remove, "radius", &json!(20));
    assert!(json_member(&after_remove, "hover").is_none());

    session.close().unwrap();
    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-106.
fn rendered_and_source_queries_copy_results() {
    if !has_test_owned_texture_session_backend() {
        return;
    }
    let mut runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::new(64, 64, 1.0)).unwrap();
    let (_context, session) = create_owned_texture_session(
        &map.attach_ref().unwrap(),
        RenderTargetExtent::new(64, 64, 1.0),
    )
    .expect("Metal or Vulkan owned texture test session should attach when supported");

    let error = session
        .query_rendered_features(
            &RenderedQueryGeometry::point(ScreenPoint::new(32.0, 32.0)),
            None,
        )
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidState);

    load_query_style(&mut runtime, &map, &session);
    let state_selector = FeatureStateSelector::new("point").with_feature_id("feature-1");
    let query_state = br#"{"selected":true}"#;
    session
        .set_feature_state(&state_selector, query_state)
        .unwrap();
    let _ = wait_for_runtime_event(&mut runtime, RuntimeEventType::MapRenderUpdateAvailable);
    let _ = session.render_update();

    let query_point = map
        .pixel_for_lat_lng(LatLng::new(37.7749, -122.4194))
        .unwrap();
    let geometry = RenderedQueryGeometry::box_(ScreenBox::new(
        ScreenPoint::new(query_point.x - 20.0, query_point.y - 20.0),
        ScreenPoint::new(query_point.x + 20.0, query_point.y + 20.0),
    ));
    let filter = br#"["==",["get","kind"],"capital"]"#;
    let mut rendered_options = RenderedFeatureQueryOptions::default();
    rendered_options.layer_ids = Some(vec!["point-circle".into()]);
    rendered_options.filter = Some(filter.to_vec());
    let rendered = wait_for_rendered_feature(
        &mut runtime,
        &session,
        &geometry,
        &rendered_options,
        "rendered point feature",
    );
    assert_eq!(rendered["sourceId"], json!("point"));
    assert_eq!(rendered["sourceLayerId"], JsonValue::Null);
    assert_eq!(queried_feature(&rendered)["id"], json!("feature-1"));
    assert_point_geometry_close(
        &queried_feature(&rendered)["geometry"],
        LatLng::new(37.7749, -122.4194),
    );
    assert_eq!(
        feature_member(queried_feature(&rendered), "kind"),
        Some(&json!("capital"))
    );
    assert_eq!(rendered["state"], json!({"selected": true}));

    let mut source_options = SourceFeatureQueryOptions::default();
    source_options.filter = Some(filter.to_vec());
    let source = wait_for_source_feature(
        &mut runtime,
        &session,
        "point",
        &source_options,
        "source point feature",
    );
    assert_eq!(source["sourceId"], json!("point"));
    assert_eq!(source["sourceLayerId"], JsonValue::Null);
    assert_eq!(queried_feature(&source)["id"], json!("feature-1"));
    assert_point_geometry_close(
        &queried_feature(&source)["geometry"],
        LatLng::new(37.7749, -122.4194),
    );
    assert_eq!(
        feature_member(queried_feature(&source), "kind"),
        Some(&json!("capital"))
    );

    session.close().unwrap();
    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-106.
fn rendered_box_queries_clip_to_the_viewport() {
    if !has_test_owned_texture_session_backend() {
        return;
    }
    let mut runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::new(64, 64, 1.0)).unwrap();
    let (_context, session) = create_owned_texture_session(
        &map.attach_ref().unwrap(),
        RenderTargetExtent::new(64, 64, 1.0),
    )
    .expect("Metal or Vulkan owned texture test session should attach when supported");

    load_query_style(&mut runtime, &map, &session);
    let mut options = RenderedFeatureQueryOptions::default();
    options.layer_ids = Some(vec!["point-circle".into()]);

    // A box that over-covers the viewport answers like the viewport itself.
    let oversized = RenderedQueryGeometry::box_(ScreenBox::new(
        ScreenPoint::new(-4096.0, -4096.0),
        ScreenPoint::new(4096.0, 4096.0),
    ));
    let rendered = wait_for_rendered_feature(
        &mut runtime,
        &session,
        &oversized,
        &options,
        "over-covering box query",
    );
    assert_eq!(queried_feature(&rendered)["id"], json!("feature-1"));

    // Corners in either order describe the same box.
    let inverted = RenderedQueryGeometry::box_(ScreenBox::new(
        ScreenPoint::new(4096.0, 4096.0),
        ScreenPoint::new(-4096.0, -4096.0),
    ));
    let inverted_features: Vec<JsonValue> = serde_json::from_slice(
        &session
            .query_rendered_features(&inverted, Some(&options))
            .unwrap(),
    )
    .unwrap();
    assert_eq!(inverted_features.len(), 1);

    // Clipping keeps a fully off-screen box empty instead of collapsing it onto
    // a viewport edge.
    let offscreen = RenderedQueryGeometry::box_(ScreenBox::new(
        ScreenPoint::new(512.0, 512.0),
        ScreenPoint::new(1024.0, 1024.0),
    ));
    let offscreen_features: Vec<JsonValue> = serde_json::from_slice(
        &session
            .query_rendered_features(&offscreen, Some(&options))
            .unwrap(),
    )
    .unwrap();
    assert!(offscreen_features.is_empty());

    session.close().unwrap();
    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-106 and BND-107.
fn feature_extension_queries_copy_value_and_feature_collection_results() {
    if !has_test_owned_texture_session_backend() {
        return;
    }
    let mut runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::new(64, 64, 1.0)).unwrap();
    let (_context, session) = create_owned_texture_session(
        &map.attach_ref().unwrap(),
        RenderTargetExtent::new(64, 64, 1.0),
    )
    .expect("Metal or Vulkan owned texture test session should attach when supported");

    load_cluster_style(&mut runtime, &map, &session);
    let query_point = map.pixel_for_lat_lng(LatLng::new(0.0, 0.0)).unwrap();
    let geometry = RenderedQueryGeometry::box_(ScreenBox::new(
        ScreenPoint::new(query_point.x - 30.0, query_point.y - 30.0),
        ScreenPoint::new(query_point.x + 30.0, query_point.y + 30.0),
    ));
    let mut options = RenderedFeatureQueryOptions::default();
    options.layer_ids = Some(vec!["cluster-circle".into()]);
    let cluster = wait_for_rendered_feature(
        &mut runtime,
        &session,
        &geometry,
        &options,
        "rendered cluster",
    );

    // Native matches cluster_id by exact JSON value type, so the copied feature
    // keeps the unsigned alternative to resolve on the way back in.
    assert!(matches!(
        feature_member(queried_feature(&cluster), "cluster_id"),
        Some(JsonValue::Number(_))
    ));

    // weight_sum comes from the cluster_properties aggregation lowered through
    // GeoJsonSourceOptions.
    assert_eq!(
        numeric_member(&queried_feature(&cluster)["properties"], "point_count"),
        Some(3.0)
    );
    assert_eq!(
        numeric_member(&queried_feature(&cluster)["properties"], "weight_sum"),
        Some(6.0)
    );

    let cluster_feature = serde_json::to_vec(queried_feature(&cluster)).unwrap();
    let children = session
        .query_feature_extension(
            "cluster-source",
            &cluster_feature,
            "supercluster",
            "children",
            None,
        )
        .unwrap();
    let children: JsonValue = serde_json::from_slice(&children).unwrap();
    assert!(!children["features"].as_array().unwrap().is_empty());

    // Native ignores arguments of another type and falls back to ten leaves at
    // offset zero, so both unsigned bounds must move the observed result.
    let first = single_cluster_leaf(&session, queried_feature(&cluster), 0);
    let second = single_cluster_leaf(&session, queried_feature(&cluster), 1);
    assert_ne!(
        feature_member(&first, "name"),
        feature_member(&second, "name")
    );

    let expansion_zoom = session
        .query_feature_extension(
            "cluster-source",
            &cluster_feature,
            "supercluster",
            "expansion-zoom",
            None,
        )
        .unwrap();
    let expansion_zoom: JsonValue = serde_json::from_slice(&expansion_zoom).unwrap();
    assert!(expansion_zoom.is_u64());

    session.close().unwrap();
    map.close().unwrap();
    runtime.close().unwrap();
}

/// Metal blocks a queue that reaches its in-flight command buffer limit, and on
/// Apple targets each frame takes objects an autorelease pool owns until it
/// drains, so a library that leaves draining to the host wedges partway through
/// a long loop.
#[test]
fn sustained_render_loop_outlasts_the_graphics_queue_depth() {
    if !has_test_owned_texture_session_backend() {
        return;
    }
    let mut runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::new(64, 64, 1.0)).unwrap();
    let (_context, session) = create_owned_texture_session(
        &map.attach_ref().unwrap(),
        RenderTargetExtent::new(64, 64, 1.0),
    )
    .expect("Metal or Vulkan owned texture test session should attach when supported");
    // A background-only style keeps each frame to the passes that take command
    // buffers, without the tile work that would make the loop slow.
    map.set_style_json(CLUSTER_BASE_STYLE_JSON.as_bytes())
        .unwrap();
    render_available_updates(&mut runtime, &session, 3);

    // Metal allows 64 command buffers in flight per queue and a frame takes
    // several, so this many frames clears that limit many times over.
    const TARGET_FRAMES: u32 = 256;
    let mut rendered_frames = 0;
    let mut step = 0;
    while rendered_frames < TARGET_FRAMES && step < 200 {
        let mut camera = CameraOptions::default();
        camera.center = Some(LatLng::new(37.0, -122.0));
        camera.zoom = Some(10.0 + f64::from(step % 8) * 0.25);
        map.jump_to(&camera).unwrap();
        let _ = runtime.pump(Some(Duration::ZERO));
        let updates = runtime
            .drain_events(0)
            .map(|batch| {
                batch
                    .iter()
                    .filter(|event| {
                        event.event_type() == RuntimeEventType::MapRenderUpdateAvailable
                    })
                    .count()
            })
            .unwrap_or(0);
        for _ in 0..updates {
            if session.render_update().unwrap() == RenderResult::Rendered {
                rendered_frames += 1;
            }
        }
        step += 1;
    }

    assert!(
        rendered_frames >= TARGET_FRAMES,
        "the loop rendered {rendered_frames} frames before running out of steps"
    );

    session.close().unwrap();
    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-163 and BND-174.
fn owned_texture_session_enforces_single_session_and_blocks_map_close() {
    if !has_test_owned_texture_session_backend() {
        return;
    }
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::new(64, 64, 1.0)).unwrap();
    let (context, session) = create_owned_texture_session(
        &map.attach_ref().unwrap(),
        RenderTargetExtent::new(32, 16, 1.0),
    )
    .expect("Metal or Vulkan owned texture test session should attach when supported");

    let error = context
        .attach_owned_texture(
            &map.attach_ref().unwrap(),
            RenderTargetExtent::new(32, 16, 1.0),
        )
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidState);

    let error = map.close().unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidState);
    assert!(error.diagnostic().contains("render session"));
    let map = error.into_handle();

    drop(runtime);

    let detached = session.detach().unwrap();
    detached.close().unwrap();
    map.close().unwrap();
}

#[test]
// Spec coverage: BND-042. The binding's parent retention ends at detach rather
// than at close.
fn detached_session_releases_the_parent_map_retention() {
    if !has_test_owned_texture_session_backend() {
        return;
    }
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::new(64, 64, 1.0)).unwrap();
    let (_context, session) = create_owned_texture_session(
        &map.attach_ref().unwrap(),
        RenderTargetExtent::new(32, 16, 1.0),
    )
    .expect("Metal or Vulkan owned texture test session should attach when supported");

    let error = map.close().unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidState);
    let map = error.into_handle();

    let detached = session.detach().unwrap();
    map.close().unwrap();
    detached.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-165.
fn resize_updates_owned_texture_frame_extent() {
    if !has_test_owned_texture_session_backend() {
        return;
    }
    let mut runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &static_map_options(64, 64, 1.0)).unwrap();
    let initial_extent = RenderTargetExtent::new(32, 16, 1.0);
    let resized_extent = RenderTargetExtent::new(48, 24, 1.0);
    let (context, session) =
        create_owned_texture_session(&map.attach_ref().unwrap(), initial_extent)
            .expect("Metal or Vulkan owned texture test session should attach when supported");

    load_query_style(&mut runtime, &map, &session);
    session
        .resize(
            resized_extent.width,
            resized_extent.height,
            resized_extent.scale_factor,
        )
        .unwrap();
    // A static map renders only on request, and the map applies the new logical
    // size on its next pump. Requesting the still image first spends it on an
    // update the session's size gate discards, so pump the resize through.
    runtime.pump(Some(Duration::ZERO)).unwrap();
    map.request_still_image().unwrap();
    let deadline = Instant::now() + Duration::from_secs(5);
    while Instant::now() < deadline {
        render_pending_updates(&mut runtime, &session);
        if context.try_acquire_frame_extent(&session, &resized_extent) {
            session.close().unwrap();
            map.close().unwrap();
            runtime.close().unwrap();
            return;
        }
        std::thread::sleep(Duration::from_millis(1));
    }

    session.close().unwrap();
    map.close().unwrap();
    runtime.close().unwrap();
    panic!("timed out waiting for resized owned texture frame");
}

#[test]
// A resize leaves the map holding an update built for the previous extent, so
// the next render reports a pending size until the map catches up.
fn render_update_reports_size_pending_until_the_map_applies_a_resize() {
    if !has_test_owned_texture_session_backend() {
        return;
    }
    let mut runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::new(64, 64, 1.0)).unwrap();
    let (_context, session) = create_owned_texture_session(
        &map.attach_ref().unwrap(),
        RenderTargetExtent::new(64, 64, 1.0),
    )
    .expect("Metal or Vulkan owned texture test session should attach when supported");

    load_query_style(&mut runtime, &map, &session);
    assert_eq!(session.render_update().unwrap(), RenderResult::Rendered);

    session.resize(96, 48, 1.0).unwrap();
    assert_eq!(session.render_update().unwrap(), RenderResult::SizePending);

    // The map publishes an update for the new size on its own thread, so the
    // state resolves without another host request.
    let deadline = Instant::now() + Duration::from_secs(5);
    let mut result = RenderResult::SizePending;
    while Instant::now() < deadline && result != RenderResult::Rendered {
        let _ = runtime.pump(Some(Duration::ZERO));
        let _ = runtime.drain_events(0);
        result = session.render_update().unwrap();
        if result != RenderResult::Rendered {
            std::thread::sleep(Duration::from_millis(2));
        }
    }
    assert_eq!(result, RenderResult::Rendered);

    session.close().unwrap();
    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// The map size follows the attached target, while the map pixel ratio stays at
// the creation value even when the target renders at a different scale factor.
fn map_size_follows_attach_and_resize_and_keeps_the_creation_scale_factor() {
    if !has_test_owned_texture_session_backend() {
        return;
    }
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &static_map_options(64, 32, 2.0)).unwrap();
    assert_eq!(map.size().unwrap(), (64, 32, 2.0));

    // A session enqueues the map size for the map's owner thread, so the map
    // keeps its previous size until the runtime is pumped.
    let (_context, session) = create_owned_texture_session(
        &map.attach_ref().unwrap(),
        RenderTargetExtent::new(32, 16, 1.0),
    )
    .expect("Metal or Vulkan owned texture test session should attach when supported");
    assert_eq!(map.size().unwrap(), (64, 32, 2.0));
    runtime.pump(Some(Duration::ZERO)).unwrap();
    assert_eq!(map.size().unwrap(), (32, 16, 2.0));

    session.resize(48, 24, 1.0).unwrap();
    runtime.pump(Some(Duration::ZERO)).unwrap();
    assert_eq!(map.size().unwrap(), (48, 24, 2.0));

    session.close().unwrap();
    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-048.
//
// Nothing stops a host from dropping the map while a session is live. The C API
// rejects that destroy, and an infallible `Drop` cannot return the error, so it
// reports the handle instead of discarding it.
fn dropping_a_map_with_an_attached_session_reports_a_leak() {
    if !has_test_owned_texture_session_backend() {
        return;
    }
    let leaks: Arc<Mutex<Vec<crate::NativeHandleLeak>>> = Arc::new(Mutex::new(Vec::new()));
    let sink = Arc::clone(&leaks);
    crate::set_leak_reporter(Some(Box::new(move |leak: crate::NativeHandleLeak| {
        sink.lock().unwrap().push(leak);
    })));

    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &static_map_options(64, 64, 1.0)).unwrap();
    let (_context, session) = create_owned_texture_session(
        &map.attach_ref().unwrap(),
        RenderTargetExtent::new(32, 16, 1.0),
    )
    .expect("owned texture test session should attach when supported");

    // The wrong order on purpose: the map goes first, while the session is live.
    drop(map);
    crate::set_leak_reporter(None);

    let reported = leaks.lock().unwrap().clone();
    assert_eq!(reported.len(), 1, "expected exactly one reported leak");
    assert_eq!(reported[0].type_name, "mln_map");
    assert_ne!(reported[0].id, 0);

    // The reported handle makes the leak recoverable: closing the session
    // unblocks the destroy that Drop could not complete, and without it the
    // runtime below would stay pinned forever.
    session.close().unwrap();
    // SAFETY: the handle came from a map whose Rust wrapper was dropped without
    // destroying it, and its session is now closed, so this is the one
    // outstanding destroy for that map.
    let status = unsafe { sys::mln_map_destroy(sys::mln_map(reported[0].id)) };
    assert_eq!(status, sys::MLN_STATUS_OK);

    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-196. Dropping a map destroys it, so the reference names a
// released handle that the C API rejects.
fn dropping_a_map_makes_its_attach_refs_stale() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &static_map_options(64, 64, 1.0)).unwrap();
    let attach_ref = map.attach_ref().unwrap();

    drop(map);

    // The replacement reuses the slot the drop freed, so this proves the
    // generation check rather than an empty slot.
    let replacement = MapHandle::with_options(&runtime, &static_map_options(64, 64, 1.0)).unwrap();

    let error = attach_ref
        .attach_metal_owned_texture(&MetalOwnedTextureDescriptor::new(
            RenderTargetExtent::new(32, 16, 1.0),
            MetalContextDescriptor::new(NativePointer::NULL),
        ))
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    assert!(error.to_string().contains("stale"));

    replacement.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-196. A `MapAttachRef` can outlive its `MapHandle`, and the
// generational handle keeps a stale reference from attaching to a later map.
fn a_stale_attach_ref_reports_a_stale_map() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &static_map_options(64, 64, 1.0)).unwrap();
    let attach_ref = map.attach_ref().unwrap();
    map.close().unwrap();

    // Resolving the map fails before the descriptor is ever inspected, so this
    // needs no render backend.
    let error = attach_ref
        .attach_metal_owned_texture(&MetalOwnedTextureDescriptor::new(
            RenderTargetExtent::new(32, 16, 1.0),
            MetalContextDescriptor::new(NativePointer::NULL),
        ))
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    assert!(error.to_string().contains("stale"));

    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-193 and BND-195.
fn a_second_thread_attaches_a_session_and_renders() {
    if !has_test_owned_texture_session_backend() {
        return;
    }
    let mut runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    // Continuous mode, so the map publishes render updates without a
    // still-image request driving them.
    let map = MapHandle::with_options(&runtime, &MapOptions::new(64, 64, 1.0)).unwrap();
    map.set_style_json(CLUSTER_BASE_STYLE_JSON.as_bytes())
        .unwrap();
    let attach_ref = map.attach_ref().unwrap();

    std::thread::scope(|scope| {
        let worker = scope.spawn(move || {
            let (_context, session) =
                create_owned_texture_session(&attach_ref, RenderTargetExtent::new(64, 64, 1.0))
                    .expect("owned texture test session should attach on this thread");
            // The map applies its logical size on its own thread, so the first
            // renders report no frame until the other thread has pumped.
            let deadline = Instant::now() + Duration::from_secs(5);
            let mut rendered = false;
            while Instant::now() < deadline && !rendered {
                rendered = session.render_update().unwrap() == RenderResult::Rendered;
                if !rendered {
                    std::thread::sleep(Duration::from_millis(2));
                }
            }
            assert!(rendered, "worker thread should render the map");
            // The session must be destroyed on the thread that attached it,
            // which is what frees the map to close below.
            session.close().unwrap();
        });
        let deadline = Instant::now() + Duration::from_secs(10);
        while !worker.is_finished() && Instant::now() < deadline {
            // A short park rather than zero: spinning would burn the deadline
            // before the worker made progress.
            runtime.pump(Some(Duration::from_millis(2))).unwrap();
            let _ = runtime.drain_events(0).unwrap();
        }
        worker.join().expect("worker thread should not panic");
    });

    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-194.
fn session_calls_are_rejected_on_a_foreign_thread() {
    if !has_test_owned_texture_session_backend() {
        return;
    }
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::new(64, 64, 1.0)).unwrap();
    map.set_style_json(CLUSTER_BASE_STYLE_JSON.as_bytes())
        .unwrap();
    let (_context, session) = create_owned_texture_session(
        &map.attach_ref().unwrap(),
        RenderTargetExtent::new(64, 64, 1.0),
    )
    .expect("owned texture test session should attach when supported");

    // The handle is `!Send`, so a foreign thread can only reach the session
    // through the raw handle the binding wraps.
    let native = session.inner.native().unwrap();
    let kind = std::thread::scope(|scope| {
        scope
            .spawn(move || {
                let mut result = maplibre_native_ffi_sys::MLN_RENDER_RESULT_NO_UPDATE;
                // SAFETY: the session is live for the duration of this scope,
                // and the call is expected to be rejected before it is used.
                let status = unsafe {
                    maplibre_native_ffi_sys::mln_render_session_render_update(native, &mut result)
                };
                maplibre_core::check(status).unwrap_err().kind()
            })
            .join()
            .expect("worker thread should not panic")
    });
    assert_eq!(kind, crate::ErrorKind::WrongThread);

    // Still bound and usable on the attaching thread.
    let _ = session.render_update().unwrap();
    session.close().unwrap();
    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-170.
fn acquired_frame_state_rejects_reentrant_session_operations_before_native_calls() {
    if !has_test_owned_texture_session_backend() {
        return;
    }
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &static_map_options(64, 64, 1.0)).unwrap();
    let (_context, session) = create_owned_texture_session(
        &map.attach_ref().unwrap(),
        RenderTargetExtent::new(32, 16, 1.0),
    )
    .expect("Metal or Vulkan owned texture test session should attach when supported");

    session.inner.frame_acquired.set(true);

    let selector = FeatureStateSelector::new("point").with_feature_id("feature-1");
    let detach_error = session.detach().unwrap_err();
    assert_eq!(detach_error.kind(), ErrorKind::InvalidState);
    assert!(detach_error.diagnostic().contains("acquired texture frame"));
    let session = detach_error.into_handle();

    for error in [
        session.resize(32, 16, 1.0).unwrap_err(),
        session.render_update().unwrap_err(),
        session.set_feature_state(&selector, b"{}").unwrap_err(),
        session.get_feature_state(&selector).unwrap_err(),
        session.remove_feature_state(&selector).unwrap_err(),
        session
            .query_rendered_features(
                &RenderedQueryGeometry::point(ScreenPoint::new(0.0, 0.0)),
                None,
            )
            .unwrap_err(),
        session.query_source_features("point", None).unwrap_err(),
        session
            .query_feature_extension(
                "point",
                br#"{"type":"Feature","geometry":null,"properties":{}}"#,
                "x",
                "y",
                None,
            )
            .unwrap_err(),
        session.read_premultiplied_rgba8_into(&mut []).unwrap_err(),
        session.acquire_metal_owned_texture_frame().unwrap_err(),
        session.acquire_vulkan_owned_texture_frame().unwrap_err(),
        session.acquire_opengl_owned_texture_frame().unwrap_err(),
    ] {
        assert_eq!(error.kind(), ErrorKind::InvalidState);
        assert!(error.diagnostic().contains("acquired texture frame"));
    }

    let error = session.close().unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidState);
    assert!(error.diagnostic().contains("acquired texture frame"));
    let session = error.into_handle();

    session.inner.frame_acquired.set(false);
    session.close().unwrap();
}

#[test]
// Spec coverage: BND-164.
fn render_update_without_pending_update_reports_no_update_and_keeps_session_live() {
    if !has_test_owned_texture_session_backend() {
        return;
    }
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &static_map_options(64, 64, 1.0)).unwrap();
    let (_context, session) = create_owned_texture_session(
        &map.attach_ref().unwrap(),
        RenderTargetExtent::new(32, 16, 1.0),
    )
    .expect("Metal or Vulkan owned texture test session should attach when supported");

    assert_eq!(session.render_update().unwrap(), RenderResult::NoUpdate);

    session.close().unwrap();
    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Rust regression: a readback that fails before native has produced a readable
// frame must leave the caller's buffer bytes untouched.
fn texture_readback_without_rendered_frame_maps_native_invalid_state() {
    if !has_test_owned_texture_session_backend() {
        return;
    }
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &static_map_options(64, 64, 1.0)).unwrap();
    let (_context, session) = create_owned_texture_session(
        &map.attach_ref().unwrap(),
        RenderTargetExtent::new(32, 16, 1.0),
    )
    .expect("Metal or Vulkan owned texture test session should attach when supported");

    let _ = session.render_update();
    let mut undersized = [0x7f];
    let error = session
        .read_premultiplied_rgba8_into(&mut undersized)
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidState);
    assert_eq!(error.raw_status(), Some(sys::MLN_STATUS_INVALID_STATE));
    assert_eq!(undersized, [0x7f]);

    session.close().unwrap();
}

#[test]
// Spec coverage: BND-166.
fn texture_readback_copies_metadata_and_fills_reusable_buffers_when_supported() {
    if !has_test_owned_texture_session_backend() {
        return;
    }
    let mut runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &static_map_options(64, 64, 1.0)).unwrap();
    let (_context, session) = create_owned_texture_session(
        &map.attach_ref().unwrap(),
        RenderTargetExtent::new(32, 16, 1.0),
    )
    .expect("Metal or Vulkan owned texture test session should attach when supported");

    load_query_style(&mut runtime, &map, &session);
    map.request_still_image().unwrap();
    let mut info = None;
    let deadline = Instant::now() + Duration::from_secs(5);
    while Instant::now() < deadline {
        render_pending_updates(&mut runtime, &session);
        match session.texture_image_info() {
            Ok(copied) => {
                info = Some(copied);
                break;
            }
            Err(error) if error.kind() == ErrorKind::InvalidState => {}
            Err(error) => panic!("unexpected readback metadata error: {error:?}"),
        }
        std::thread::sleep(Duration::from_millis(1));
    }
    let info = info.expect("timed out waiting for readback metadata");
    assert_eq!((info.width, info.height), (32, 16));
    assert!(info.byte_length > 0);

    let mut undersized = vec![0x7f; info.byte_length - 1];
    let error = session
        .read_premultiplied_rgba8_into(&mut undersized)
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidArgument);
    assert_eq!(error.raw_status(), Some(sys::MLN_STATUS_INVALID_ARGUMENT));
    assert!(undersized.iter().all(|byte| *byte == 0x7f));

    // An empty destination is a caller error here, not the C size probe.
    let empty_error = session.read_premultiplied_rgba8_into(&mut []).unwrap_err();
    assert_eq!(empty_error.kind(), ErrorKind::InvalidArgument);

    let mut reusable = vec![0; info.byte_length];
    let copied_info = session
        .read_premultiplied_rgba8_into(&mut reusable)
        .unwrap();
    assert_eq!(copied_info, info);
    assert_eq!(reusable.len(), info.byte_length);

    // What comes back is the frame that was drawn, not a buffer of the right
    // shape. This renders on rather than reading once: a generation counts as
    // rendered from the first frame that completes for it, which can be before
    // the style has anything to paint, so the first readable frame is blank on
    // every backend.
    let content_deadline = Instant::now() + Duration::from_secs(5);
    let mut rendered_the_style = false;
    while Instant::now() < content_deadline && !rendered_the_style {
        let _ = runtime.pump(Some(Duration::ZERO));
        let _ = runtime.drain_events(0);
        let _ = session.render_update();
        session
            .read_premultiplied_rgba8_into(&mut reusable)
            .unwrap();
        rendered_the_style = reusable
            .chunks_exact(4)
            .any(|pixel| pixel == QUERY_STYLE_BACKGROUND_RGBA);
        std::thread::sleep(Duration::from_millis(5));
    }
    assert!(
        rendered_the_style,
        "readback should return the frame this session rendered"
    );

    session.close().unwrap();
    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-160 and BND-162.
fn backend_specific_attach_calls_report_native_statuses() {
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &static_map_options(64, 64, 1.0)).unwrap();

    let metal_error = map
        .attach_ref()
        .unwrap()
        .attach_metal_owned_texture(&MetalOwnedTextureDescriptor::new(
            RenderTargetExtent::new(32, 16, 1.0),
            MetalContextDescriptor::new(NativePointer::NULL),
        ))
        .unwrap_err();
    assert!(matches!(
        metal_error.kind(),
        ErrorKind::InvalidArgument | ErrorKind::Unsupported
    ));

    let vulkan_error = map
        .attach_ref()
        .unwrap()
        .attach_vulkan_surface(&VulkanSurfaceDescriptor::new(
            RenderTargetExtent::new(32, 16, 1.0),
            VulkanContextDescriptor::new(
                NativePointer::NULL,
                NativePointer::NULL,
                NativePointer::NULL,
                NativePointer::NULL,
                0,
            ),
            NativePointer::NULL,
        ))
        .unwrap_err();
    assert!(matches!(
        vulkan_error.kind(),
        ErrorKind::InvalidArgument | ErrorKind::Unsupported
    ));

    let opengl_context = OpenGLContextDescriptor::Wgl(WglContextDescriptor::new(
        NativePointer::NULL,
        NativePointer::NULL,
    ));
    let opengl_error = map
        .attach_ref()
        .unwrap()
        .attach_opengl_surface(&OpenGLSurfaceDescriptor::new(
            RenderTargetExtent::new(32, 16, 1.0),
            opengl_context.clone(),
            NativePointer::NULL,
        ))
        .unwrap_err();
    assert!(matches!(
        opengl_error.kind(),
        ErrorKind::InvalidArgument | ErrorKind::Unsupported
    ));

    let opengl_error = map
        .attach_ref()
        .unwrap()
        .attach_opengl_owned_texture(&OpenGLOwnedTextureDescriptor::new(
            RenderTargetExtent::new(0, 16, 1.0),
            opengl_context.clone(),
        ))
        .unwrap_err();
    assert_eq!(opengl_error.kind(), ErrorKind::InvalidArgument);

    let opengl_error = map
        .attach_ref()
        .unwrap()
        .attach_opengl_borrowed_texture(&OpenGLBorrowedTextureDescriptor::new(
            RenderTargetExtent::new(32, 16, 1.0),
            32,
            16,
            opengl_context,
            0,
            0x0de1,
        ))
        .unwrap_err();
    assert!(matches!(
        opengl_error.kind(),
        ErrorKind::InvalidArgument | ErrorKind::Unsupported
    ));

    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-160.
fn opengl_attach_calls_report_unsupported_when_backend_unavailable() {
    if crate::supported_render_backends().contains(RenderBackendMask::OPENGL) {
        return;
    }

    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &static_map_options(64, 64, 1.0)).unwrap();
    // SAFETY: Test uses dummy opaque addresses and never dereferences them.
    let fake = unsafe { NativePointer::from_address(1) };
    let opengl_context = OpenGLContextDescriptor::Wgl(WglContextDescriptor::new(fake, fake));

    let error = map
        .attach_ref()
        .unwrap()
        .attach_opengl_owned_texture(&OpenGLOwnedTextureDescriptor::new(
            RenderTargetExtent::new(32, 16, 1.0),
            opengl_context.clone(),
        ))
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::Unsupported);
    assert_eq!(error.raw_status(), Some(sys::MLN_STATUS_UNSUPPORTED));

    let error = map
        .attach_ref()
        .unwrap()
        .attach_opengl_borrowed_texture(&OpenGLBorrowedTextureDescriptor::new(
            RenderTargetExtent::new(32, 16, 1.0),
            32,
            16,
            opengl_context.clone(),
            1,
            gl_api::TEXTURE_2D,
        ))
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::Unsupported);
    assert_eq!(error.raw_status(), Some(sys::MLN_STATUS_UNSUPPORTED));

    let error = map
        .attach_ref()
        .unwrap()
        .attach_opengl_surface(&OpenGLSurfaceDescriptor::new(
            RenderTargetExtent::new(32, 16, 1.0),
            opengl_context,
            fake,
        ))
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::Unsupported);
    assert_eq!(error.raw_status(), Some(sys::MLN_STATUS_UNSUPPORTED));

    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-102. Rendered frames advance a camera transition, so only
// a session-backed map can run an ease to completion.
fn identified_camera_transition_reports_its_end_once_when_it_runs_to_completion() {
    if !has_test_owned_texture_session_backend() {
        return;
    }
    let mut runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let mut options = MapOptions::new(64, 64, 1.0);
    options.mode = MapMode::Continuous;
    let map = MapHandle::with_options(&runtime, &options).unwrap();
    let (_context, session) = create_owned_texture_session(
        &map.attach_ref().unwrap(),
        RenderTargetExtent::new(64, 64, 1.0),
    )
    .expect("Metal or Vulkan owned texture test session should attach when supported");
    load_query_style(&mut runtime, &map, &session);

    let mut animation = AnimationOptions::default();
    animation.transition_id = Some(31);
    animation.duration_ms = Some(200.0);
    let mut camera = CameraOptions::default();
    camera.center = Some(LatLng::new(37.0, -122.0));
    camera.zoom = Some(12.0);
    map.ease_to(&camera, Some(&animation)).unwrap();

    // Keep rendering for a while after the transition ends so a repeated
    // report would show up in the tally.
    let mut finished_transition_ids = Vec::new();
    let mut saw_animated_did_change = false;
    let mut finished_at: Option<Instant> = None;
    let deadline = Instant::now() + Duration::from_secs(10);
    while Instant::now() < deadline {
        let _ = runtime.pump(Some(Duration::ZERO));
        let mut updates = 0;
        for event in runtime.drain_events(0).unwrap().iter() {
            match event.event_type() {
                RuntimeEventType::MapCameraTransitionFinished => {
                    let RuntimeEventPayload::CameraTransitionFinished(payload) = event.payload()
                    else {
                        panic!("transition-finished event should carry its typed payload");
                    };
                    finished_transition_ids.push(payload.transition_id);
                    finished_at.get_or_insert_with(Instant::now);
                }
                RuntimeEventType::MapCameraDidChange => {
                    saw_animated_did_change |= CameraChangeMode::from_raw(event.code() as u32)
                        == CameraChangeMode::Animated;
                }
                RuntimeEventType::MapRenderUpdateAvailable => updates += 1,
                _ => {}
            }
        }
        for _ in 0..updates {
            let _ = session.render_update();
        }
        if finished_at.is_some_and(|at| at.elapsed() >= Duration::from_millis(500)) {
            break;
        }
        std::thread::sleep(Duration::from_millis(5));
    }

    assert_eq!(finished_transition_ids, vec![31]);
    assert!(saw_animated_did_change);
    // Reaching the requested camera shows the transition ran to completion
    // rather than being superseded or cancelled.
    let settled = map.camera().unwrap();
    assert!((settled.zoom.unwrap() - 12.0).abs() < 1e-6);

    session.close().unwrap();
    map.close().unwrap();
    runtime.close().unwrap();
}
