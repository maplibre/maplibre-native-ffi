use std::cell::{Cell, RefCell};
use std::error::Error as StdError;
use std::ffi::{CStr, CString};
use std::marker::PhantomData;
#[cfg(any(target_os = "linux", target_os = "windows"))]
use std::num::NonZeroU32;
use std::ptr::NonNull;
use std::rc::Rc;
use std::sync::atomic::{AtomicI32, AtomicUsize, Ordering};
use std::time::Duration;

use ash::vk;
use ash::vk::Handle;
use glow::HasContext;
#[cfg(target_os = "linux")]
use glutin::config::{AsRawConfig, ConfigSurfaceTypes, RawConfig};
#[cfg(any(target_os = "linux", target_os = "windows"))]
use glutin::config::{ConfigTemplateBuilder, GlConfig};
use glutin::context::PossiblyCurrentContext;
#[cfg(any(target_os = "windows", target_os = "linux"))]
use glutin::context::{AsRawContext, RawContext};
#[cfg(any(target_os = "linux", target_os = "windows"))]
use glutin::context::{ContextApi, ContextAttributesBuilder, Version};
#[cfg(target_os = "linux")]
use glutin::display::{AsRawDisplay, Display as GlutinDisplay, DisplayApiPreference, RawDisplay};
#[cfg(any(target_os = "linux", target_os = "windows"))]
use glutin::display::{GetGlDisplay, GlDisplay};
use glutin::prelude::*;
#[cfg(target_os = "linux")]
use glutin::surface::PbufferSurface;
use glutin::surface::Surface;
#[cfg(any(target_os = "linux", target_os = "windows"))]
use glutin::surface::SurfaceAttributesBuilder;
#[cfg(not(target_os = "linux"))]
use glutin::surface::WindowSurface;
#[cfg(target_os = "linux")]
use glutin::surface::{AsRawSurface, RawSurface};
#[cfg(target_os = "windows")]
use glutin_winit::{ApiPreference, DisplayBuilder};
use static_assertions::assert_not_impl_any;
#[cfg(target_os = "windows")]
use windows_sys::Win32::Foundation::HWND;
#[cfg(target_os = "windows")]
use windows_sys::Win32::Graphics::Gdi::{GetDC, HDC, ReleaseDC};
#[cfg(target_os = "windows")]
use winit::dpi::PhysicalSize;
#[cfg(target_os = "windows")]
use winit::event_loop::EventLoop;
#[cfg(target_os = "windows")]
use winit::platform::windows::EventLoopBuilderExtWindows;
#[cfg(target_os = "windows")]
use winit::raw_window_handle::HasWindowHandle;
#[cfg(target_os = "windows")]
use winit::raw_window_handle::RawWindowHandle;
#[cfg(target_os = "windows")]
use winit::raw_window_handle::Win32WindowHandle;
#[cfg(target_os = "linux")]
use winit::raw_window_handle::{RawDisplayHandle, XlibDisplayHandle};
#[cfg(target_os = "windows")]
use winit::window::{Window, WindowAttributes};

use super::*;
use crate::{
    CameraOptions, ErrorKind, FeatureIdentifier, Geometry, JsonMember, LatLng, MapMode, MapOptions,
    OpenGLContextProviderMask, RenderBackendMask, RuntimeEventType, RuntimeHandle, ScreenBox,
    ScreenPoint,
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
const CLUSTER_STYLE_JSON: &str = r##"{"version":8,"sources":{"cluster-source":{"type":"geojson","cluster":true,"data":{"type":"FeatureCollection","features":[{"type":"Feature","geometry":{"type":"Point","coordinates":[0.0,0.0]},"properties":{"name":"one"}},{"type":"Feature","geometry":{"type":"Point","coordinates":[0.001,0.001]},"properties":{"name":"two"}},{"type":"Feature","geometry":{"type":"Point","coordinates":[0.002,0.002]},"properties":{"name":"three"}}]}}},"layers":[{"id":"background","type":"background","paint":{"background-color":"#ffffff"}},{"id":"cluster-circle","type":"circle","source":"cluster-source","filter":["has","point_count"],"paint":{"circle-color":"#2563eb","circle-radius":20}}]}"##;
fn create_owned_texture_session(
    map: &MapHandle,
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
    if backends.contains(RenderBackendMask::VULKAN) {
        let context = VulkanTestContext::new()?;
        let session = map.attach_vulkan_owned_texture(&VulkanOwnedTextureDescriptor::new(
            extent,
            context.descriptor(),
        ))?;
        return Ok((OwnedTextureTestContext::Vulkan(Box::new(context)), session));
    }
    Err("native library does not support Metal or Vulkan owned texture sessions".into())
}

fn has_test_owned_texture_session_backend() -> bool {
    let backends = crate::supported_render_backends();
    backends.intersects(RenderBackendMask::METAL | RenderBackendMask::VULKAN)
}

fn has_opengl_backend() -> bool {
    crate::supported_render_backends().contains(RenderBackendMask::OPENGL)
}

fn create_opengl_owned_texture_session(
    map: &MapHandle,
    extent: RenderTargetExtent,
) -> std::result::Result<(OpenGLTestContext, RenderSessionHandle), Box<dyn StdError>> {
    let backends = crate::supported_render_backends();
    if !backends.contains(RenderBackendMask::OPENGL) {
        return Err("native library does not support OpenGL owned texture sessions".into());
    }
    let context = OpenGLTestContext::new()?;
    let session = map.attach_opengl_owned_texture(&OpenGLOwnedTextureDescriptor::new(
        extent,
        context.descriptor(),
    ))?;
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
    let context = OpenGLTestContext::new()?;
    let session = map.attach_opengl_surface(&OpenGLSurfaceDescriptor::new(
        extent,
        context.descriptor(),
        context.surface(),
    ))?;
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
    let texture = OpenGLBorrowedTexture::new(extent.width, extent.height)?;
    let session = map.attach_opengl_borrowed_texture(&OpenGLBorrowedTextureDescriptor::new(
        extent,
        texture.descriptor(),
        texture.name(),
        glow::TEXTURE_2D,
    ))?;
    Ok((texture, session))
}

#[allow(dead_code)]
enum OwnedTextureTestContext {
    Metal(MetalTestContext),
    Vulkan(Box<VulkanTestContext>),
}

impl OwnedTextureTestContext {
    fn attach_owned_texture(
        &self,
        map: &MapHandle,
        extent: RenderTargetExtent,
    ) -> Result<RenderSessionHandle> {
        match self {
            Self::Metal(context) => map.attach_metal_owned_texture(
                &MetalOwnedTextureDescriptor::new(extent, context.descriptor()),
            ),
            Self::Vulkan(context) => map.attach_vulkan_owned_texture(
                &VulkanOwnedTextureDescriptor::new(extent, context.descriptor()),
            ),
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
    gl: glow::Context,
    context: PossiblyCurrentContext,
    surface: Surface<OpenGLTestSurface>,
    #[cfg(target_os = "windows")]
    _window: Window,
    #[cfg(target_os = "windows")]
    _event_loop: EventLoop<()>,
    #[cfg(target_os = "windows")]
    hwnd: HWND,
    #[cfg(target_os = "windows")]
    hdc: HDC,
}

#[cfg(target_os = "linux")]
type OpenGLTestSurface = PbufferSurface;
#[cfg(not(target_os = "linux"))]
type OpenGLTestSurface = WindowSurface;

impl OpenGLTestContext {
    fn new() -> std::result::Result<Self, Box<dyn StdError>> {
        Self::new_platform()
    }

    #[cfg(target_os = "linux")]
    fn new_platform() -> std::result::Result<Self, Box<dyn StdError>> {
        let display_handle = RawDisplayHandle::Xlib(XlibDisplayHandle::new(None, 0));
        let display = unsafe { GlutinDisplay::new(display_handle, DisplayApiPreference::Egl)? };
        let template = ConfigTemplateBuilder::new()
            .with_alpha_size(8)
            .with_depth_size(24)
            .with_stencil_size(8)
            .with_surface_type(ConfigSurfaceTypes::PBUFFER)
            .with_pbuffer_sizes(NonZeroU32::new(8).unwrap(), NonZeroU32::new(8).unwrap())
            .build();
        let config = unsafe {
            display
                .find_configs(template)?
                .max_by_key(|config| config.num_samples())
                .ok_or("glutin returned no EGL pbuffer configs")?
        };
        let context_attributes = ContextAttributesBuilder::new()
            .with_context_api(ContextApi::Gles(Some(Version::new(3, 0))))
            .build(None);
        let not_current = unsafe { display.create_context(&config, &context_attributes)? };
        let surface_attributes = SurfaceAttributesBuilder::<PbufferSurface>::new()
            .build(NonZeroU32::new(8).unwrap(), NonZeroU32::new(8).unwrap());
        let surface = unsafe { display.create_pbuffer_surface(&config, &surface_attributes)? };
        let context = not_current.make_current(&surface)?;
        let gl = unsafe {
            glow::Context::from_loader_function(|symbol| {
                let symbol = CString::new(symbol).expect("GL symbol names do not contain NULs");
                display.get_proc_address(&symbol).cast()
            })
        };
        let descriptor =
            opengl_context_descriptor_from_glutin(&config, &context, NativePointer::NULL)?;
        let surface_handle = opengl_surface_handle_from_glutin(&surface, NativePointer::NULL)?;

        Ok(Self {
            descriptor,
            surface_handle,
            gl,
            context,
            surface,
        })
    }

    #[cfg(target_os = "windows")]
    fn new_platform() -> std::result::Result<Self, Box<dyn StdError>> {
        let mut event_loop_builder = EventLoop::builder();
        event_loop_builder.with_any_thread(true);
        let event_loop = event_loop_builder.build()?;

        let window_attributes = WindowAttributes::default()
            .with_visible(false)
            .with_inner_size(PhysicalSize::new(8, 8));
        let template = ConfigTemplateBuilder::new()
            .with_alpha_size(8)
            .with_depth_size(24)
            .with_stencil_size(8);
        let (window, config) = DisplayBuilder::new()
            .with_preference(opengl_api_preference())
            .with_window_attributes(Some(window_attributes))
            .build(&event_loop, template, |configs| {
                configs
                    .max_by_key(|config| config.num_samples())
                    .expect("glutin returned no OpenGL configs")
            })?;
        let window = window.ok_or("glutin did not create a test window")?;
        let raw_window_handle = window.window_handle()?.as_raw();

        let context_attributes = ContextAttributesBuilder::new()
            .with_context_api(ContextApi::OpenGl(Some(Version::new(3, 0))))
            .build(Some(raw_window_handle));
        let display = config.display();
        // SAFETY: raw_window_handle comes from a live winit window and config
        // was selected from the same glutin display.
        let not_current = unsafe { display.create_context(&config, &context_attributes)? };

        let surface_attributes = SurfaceAttributesBuilder::<WindowSurface>::new().build(
            raw_window_handle,
            NonZeroU32::new(8).unwrap(),
            NonZeroU32::new(8).unwrap(),
        );
        // SAFETY: raw_window_handle comes from a live winit window and the
        // surface attributes were built for that window.
        let surface = unsafe { display.create_window_surface(&config, &surface_attributes)? };
        let context = not_current.make_current(&surface)?;
        let gl = unsafe {
            glow::Context::from_loader_function(|symbol| {
                let symbol = CString::new(symbol).expect("GL symbol names do not contain NULs");
                display.get_proc_address(&symbol).cast()
            })
        };

        let hwnd = hwnd_from_window(&window)?;
        let hdc = hdc_from_hwnd(hwnd)?;
        let device_context = unsafe { NativePointer::from_ptr(hdc) };

        let descriptor = opengl_context_descriptor_from_glutin(&config, &context, device_context)?;
        let surface_handle = opengl_surface_handle_from_glutin(&surface, device_context)?;

        Ok(Self {
            descriptor,
            surface_handle,
            gl,
            context,
            surface,
            hwnd,
            hdc,
            _window: window,
            _event_loop: event_loop,
        })
    }

    #[cfg(not(any(target_os = "linux", target_os = "windows")))]
    fn new_platform() -> std::result::Result<Self, Box<dyn StdError>> {
        Err("OpenGL test context is only available on Windows WGL and Linux EGL".into())
    }

    fn descriptor(&self) -> OpenGLContextDescriptor {
        self.descriptor.clone()
    }

    fn surface(&self) -> NativePointer {
        self.surface_handle
    }

    fn make_current(&self) -> std::result::Result<(), Box<dyn StdError>> {
        self.context.make_current(&self.surface)?;
        Ok(())
    }

    fn check_gl_error(&self, operation: &str) -> std::result::Result<(), Box<dyn StdError>> {
        let error = unsafe { self.gl.get_error() };
        if error == glow::NO_ERROR {
            Ok(())
        } else {
            Err(format!("{operation} failed with OpenGL error 0x{error:x}").into())
        }
    }
}

#[cfg(target_os = "windows")]
impl Drop for OpenGLTestContext {
    fn drop(&mut self) {
        // SAFETY: hdc was acquired from this window with GetDC.
        unsafe {
            ReleaseDC(self.hwnd, self.hdc);
        }
    }
}

#[cfg(target_os = "windows")]
fn opengl_api_preference() -> ApiPreference {
    ApiPreference::FallbackEgl
}

#[cfg(any(target_os = "windows", target_os = "linux"))]
fn opengl_context_descriptor_from_glutin(
    _config: &glutin::config::Config,
    context: &PossiblyCurrentContext,
    device_context: NativePointer,
) -> std::result::Result<OpenGLContextDescriptor, Box<dyn StdError>> {
    #[cfg(target_os = "windows")]
    {
        let RawContext::Wgl(raw_context) = context.raw_context() else {
            return Err("glutin did not create a WGL context".into());
        };
        let share_context = unsafe { NativePointer::from_ptr(raw_context.cast_mut()) };
        Ok(OpenGLContextDescriptor::Wgl(WglContextDescriptor::new(
            device_context,
            share_context,
        )))
    }

    #[cfg(target_os = "linux")]
    {
        let _ = device_context;
        let RawDisplay::Egl(raw_display) = _config.display().raw_display() else {
            return Err("glutin did not create an EGL display".into());
        };
        let RawConfig::Egl(raw_config) = _config.raw_config() else {
            return Err("glutin did not choose an EGL config".into());
        };
        let RawContext::Egl(raw_context) = context.raw_context() else {
            return Err("glutin did not create an EGL context".into());
        };
        Ok(OpenGLContextDescriptor::Egl(EglContextDescriptor::new(
            unsafe { NativePointer::from_ptr(raw_display.cast_mut()) },
            unsafe { NativePointer::from_ptr(raw_config.cast_mut()) },
            unsafe { NativePointer::from_ptr(raw_context.cast_mut()) },
        )))
    }

    #[cfg(not(any(target_os = "windows", target_os = "linux")))]
    {
        let _ = (_config, context, device_context);
        Err("OpenGL test context is only available on Windows WGL and Linux EGL".into())
    }
}

#[cfg(any(target_os = "windows", target_os = "linux"))]
fn opengl_surface_handle_from_glutin(
    surface: &Surface<OpenGLTestSurface>,
    device_context: NativePointer,
) -> std::result::Result<NativePointer, Box<dyn StdError>> {
    #[cfg(target_os = "windows")]
    {
        let _ = surface;
        Ok(device_context)
    }

    #[cfg(target_os = "linux")]
    {
        let _ = device_context;
        let RawSurface::Egl(raw_surface) = surface.raw_surface() else {
            return Err("glutin did not create an EGL surface".into());
        };
        Ok(unsafe { NativePointer::from_ptr(raw_surface.cast_mut()) })
    }

    #[cfg(not(any(target_os = "windows", target_os = "linux")))]
    {
        let _ = (surface, device_context);
        Err("OpenGL test surfaces are only available on Windows WGL and Linux EGL".into())
    }
}

#[cfg(target_os = "windows")]
fn hdc_from_hwnd(hwnd: HWND) -> std::result::Result<HDC, Box<dyn StdError>> {
    // SAFETY: hwnd comes from a live winit window.
    let hdc = unsafe { GetDC(hwnd) };
    if hdc.is_null() {
        Err("GetDC returned null".into())
    } else {
        Ok(hdc)
    }
}

#[cfg(target_os = "windows")]
fn hwnd_from_window(window: &Window) -> std::result::Result<HWND, Box<dyn StdError>> {
    let RawWindowHandle::Win32(Win32WindowHandle { hwnd, .. }) = window.window_handle()?.as_raw()
    else {
        return Err("winit did not return a Win32 window handle".into());
    };
    Ok(hwnd.get() as HWND)
}

struct OpenGLBorrowedTexture {
    context: OpenGLTestContext,
    texture: Option<glow::NativeTexture>,
    width: u32,
    height: u32,
}

impl OpenGLBorrowedTexture {
    fn new(width: u32, height: u32) -> std::result::Result<Self, Box<dyn StdError>> {
        let context = OpenGLTestContext::new()?;
        context.make_current()?;
        let texture = unsafe {
            let texture = context.gl.create_texture()?;
            context.gl.bind_texture(glow::TEXTURE_2D, Some(texture));
            context.gl.tex_parameter_i32(
                glow::TEXTURE_2D,
                glow::TEXTURE_MIN_FILTER,
                glow::NEAREST as i32,
            );
            context.gl.tex_parameter_i32(
                glow::TEXTURE_2D,
                glow::TEXTURE_MAG_FILTER,
                glow::NEAREST as i32,
            );
            context.gl.tex_image_2d(
                glow::TEXTURE_2D,
                0,
                glow::RGBA8 as i32,
                width as i32,
                height as i32,
                0,
                glow::RGBA,
                glow::UNSIGNED_BYTE,
                glow::PixelUnpackData::Slice(None),
            );
            context.gl.bind_texture(glow::TEXTURE_2D, None);
            texture
        };
        context.check_gl_error("create borrowed texture")?;
        Ok(Self {
            context,
            texture: Some(texture),
            width,
            height,
        })
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
        unsafe {
            self.context.gl.bind_texture(glow::TEXTURE_2D, self.texture);
            self.context.gl.get_tex_image(
                glow::TEXTURE_2D,
                0,
                glow::RGBA,
                glow::UNSIGNED_BYTE,
                glow::PixelPackData::Slice(Some(&mut pixels)),
            );
            self.context.gl.bind_texture(glow::TEXTURE_2D, None);
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

struct VulkanTestContext {
    _entry: ash::Entry,
    instance: ash::Instance,
    physical_device: vk::PhysicalDevice,
    device: ash::Device,
    graphics_queue: vk::Queue,
    graphics_queue_family_index: u32,
}

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

fn load_vulkan_entry() -> std::result::Result<ash::Entry, Box<dyn StdError>> {
    // SAFETY: Loading the Vulkan loader is delegated to ash.
    unsafe { ash::Entry::load() }.map_err(Into::into)
}

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

fn wait_for_runtime_event(runtime: &RuntimeHandle, event_type: RuntimeEventType) -> bool {
    for _ in 0..100 {
        let _ = runtime.run_once();
        while let Ok(Some(event)) = runtime.poll_event() {
            if event.event_type == event_type {
                return true;
            }
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
    runtime: &RuntimeHandle,
    map: &MapHandle,
    session: &RenderSessionHandle,
) {
    map.set_style_json(FEATURE_STATE_STYLE_JSON).unwrap();
    assert!(wait_for_runtime_event(
        runtime,
        RuntimeEventType::MapRenderUpdateAvailable
    ));
    session.render_update().unwrap();
}

fn load_query_style(runtime: &RuntimeHandle, map: &MapHandle, session: &RenderSessionHandle) {
    let mut camera = CameraOptions::default();
    camera.center = Some(LatLng::new(37.7749, -122.4194));
    camera.zoom = Some(10.0);
    map.jump_to(&camera).unwrap();
    map.set_style_json(QUERY_STYLE_JSON).unwrap();
    render_available_updates(runtime, session, 5);
}

fn load_cluster_style(runtime: &RuntimeHandle, map: &MapHandle, session: &RenderSessionHandle) {
    let mut camera = CameraOptions::default();
    camera.center = Some(LatLng::new(0.0, 0.0));
    camera.zoom = Some(0.0);
    map.jump_to(&camera).unwrap();
    map.set_style_json(CLUSTER_STYLE_JSON).unwrap();
    render_available_updates(runtime, session, 5);
}

fn render_available_updates(runtime: &RuntimeHandle, session: &RenderSessionHandle, count: usize) {
    for _ in 0..count {
        if wait_for_runtime_event(runtime, RuntimeEventType::MapRenderUpdateAvailable) {
            let _ = session.render_update();
        }
    }
}

fn render_pending_updates(runtime: &RuntimeHandle, session: &RenderSessionHandle) {
    let _ = runtime.run_once();
    for _ in 0..100 {
        let Ok(Some(event)) = runtime.poll_event() else {
            return;
        };
        if event.event_type == RuntimeEventType::MapRenderUpdateAvailable {
            let _ = session.render_update();
        }
    }
}

fn wait_for_rendered_feature(
    runtime: &RuntimeHandle,
    session: &RenderSessionHandle,
    geometry: &RenderedQueryGeometry,
    options: &RenderedFeatureQueryOptions,
    description: &str,
) -> QueriedFeature {
    for _ in 0..1000 {
        let features = session
            .query_rendered_features(geometry, Some(options))
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
    runtime: &RuntimeHandle,
    session: &RenderSessionHandle,
    source_id: &str,
    options: &SourceFeatureQueryOptions,
    description: &str,
) -> QueriedFeature {
    for _ in 0..1000 {
        let features = session
            .query_source_features(source_id, Some(options))
            .unwrap();
        if features.len() == 1 {
            return features.into_iter().next().unwrap();
        }
        render_pending_updates(runtime, session);
        std::thread::sleep(Duration::from_millis(1));
    }
    panic!("timed out waiting for {description}");
}

fn feature_member<'a>(feature: &'a Feature, key: &str) -> Option<&'a JsonValue> {
    feature
        .properties
        .iter()
        .find(|member| member.key == key)
        .map(|member| &member.value)
}

fn json_member<'a>(value: &'a JsonValue, key: &str) -> Option<&'a JsonValue> {
    let JsonValue::Object(members) = value else {
        return None;
    };
    members
        .iter()
        .find(|member| member.key == key)
        .map(|member| &member.value)
}

fn assert_json_member(value: &JsonValue, key: &str, expected: &JsonValue) {
    assert_eq!(json_member(value, key), Some(expected));
}

fn assert_point_geometry_close(geometry: &Geometry, expected: LatLng) {
    let Geometry::Point(actual) = geometry else {
        panic!("expected point geometry, got {geometry:?}");
    };
    assert!((actual.latitude - expected.latitude).abs() < 0.0001);
    assert!((actual.longitude - expected.longitude).abs() < 0.0001);
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
        assert!(
            providers.intersects(OpenGLContextProviderMask::WGL | OpenGLContextProviderMask::EGL)
        );
    } else {
        assert!(providers.is_empty());
    }
}

#[test]
// Spec coverage: BND-162.
fn opengl_owned_texture_session_attaches_with_platform_context() {
    if !has_opengl_backend() {
        return;
    }
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::new(64, 64, 1.0)).unwrap();
    let (_context, session) =
        create_opengl_owned_texture_session(&map, RenderTargetExtent::new(32, 16, 1.0))
            .expect("OpenGL owned texture test session should attach when OpenGL is supported");

    let error = session.acquire_opengl_owned_texture_frame().unwrap_err();
    assert!(matches!(
        error.kind(),
        ErrorKind::InvalidState | ErrorKind::Unsupported
    ));

    map.set_style_json(QUERY_STYLE_JSON).unwrap();
    assert!(wait_for_runtime_event(
        &runtime,
        RuntimeEventType::MapRenderUpdateAvailable
    ));
    session.render_update().unwrap();

    let frame = session.acquire_opengl_owned_texture_frame().unwrap();
    assert_eq!(frame.frame().unwrap().width, 32);
    assert_eq!(frame.frame().unwrap().height, 16);
    assert_eq!(frame.frame().unwrap().target, glow::TEXTURE_2D);
    assert_eq!(frame.frame().unwrap().internal_format, glow::RGBA8);
    assert_eq!(frame.frame().unwrap().format, glow::RGBA);
    assert_eq!(frame.frame().unwrap().type_, glow::UNSIGNED_BYTE);
    assert!(!frame.texture().unwrap().is_zero());
    frame.close().unwrap();

    session.close().unwrap();
    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-162.
fn opengl_surface_session_renders_with_platform_context() {
    if !has_opengl_backend() {
        return;
    }
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::new(64, 64, 1.0)).unwrap();

    let (_context, session) =
        create_opengl_surface_session(&map, RenderTargetExtent::new(32, 16, 1.0))
            .expect("OpenGL surface test session should attach when OpenGL is supported");

    map.set_style_json(QUERY_STYLE_JSON).unwrap();
    assert!(wait_for_runtime_event(
        &runtime,
        RuntimeEventType::MapRenderUpdateAvailable
    ));
    session.render_update().unwrap();

    session.close().unwrap();
    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-162 and BND-171.
fn opengl_borrowed_texture_session_renders_with_platform_context() {
    if !has_opengl_backend() {
        return;
    }
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::new(128, 128, 1.0)).unwrap();

    let (texture, session) =
        create_opengl_borrowed_texture_session(&map, RenderTargetExtent::new(128, 128, 1.0))
            .expect("OpenGL borrowed texture test session should attach when OpenGL is supported");

    map.set_style_json(QUERY_STYLE_JSON).unwrap();
    assert!(wait_for_runtime_event(
        &runtime,
        RuntimeEventType::MapRenderUpdateAvailable
    ));
    session.render_update().unwrap();

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
// Spec coverage: BND-167 and BND-168.
fn frame_native_pointer_round_trips_address_without_plain_native_pointer() {
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

unsafe extern "C" fn fake_session_destroy(
    _session: *mut sys::mln_render_session,
) -> sys::mln_status {
    sys::MLN_STATUS_OK
}

unsafe extern "C" fn fake_metal_frame_release(
    _session: *mut sys::mln_render_session,
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
        // SAFETY: The fake handle is never dereferenced. The fake destroy
        // function only reports success so RenderSessionState drop is harmless.
        handle: unsafe {
            ThreadAffineNativeHandle::from_raw(
                NonNull::dangling(),
                fake_session_destroy,
                "mln_render_session",
            )
        },
        map: RefCell::new(None),
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
// Spec coverage: BND-105 and BND-106.
fn feature_state_set_get_and_remove_copy_snapshots() {
    if !has_test_owned_texture_session_backend() {
        return;
    }
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::new(64, 64, 1.0)).unwrap();
    let (_context, session) =
        create_owned_texture_session(&map, RenderTargetExtent::new(64, 64, 1.0))
            .expect("Metal or Vulkan owned texture test session should attach when supported");
    let selector = FeatureStateSelector::new("point").with_feature_id("feature-1");
    let state = JsonValue::Object(vec![
        JsonMember::new("hover", JsonValue::Bool(true)),
        JsonMember::new("radius", JsonValue::UInt(20)),
    ]);

    let error = session.set_feature_state(&selector, &state).unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidState);

    load_feature_state_style(&runtime, &map, &session);

    session.set_feature_state(&selector, &state).unwrap();
    let copied = session.get_feature_state(&selector).unwrap();
    assert_json_member(&copied, "hover", &JsonValue::Bool(true));
    assert_json_member(&copied, "radius", &JsonValue::UInt(20));

    let hover_selector = FeatureStateSelector::new("point")
        .with_feature_id("feature-1")
        .with_state_key("hover")
        .unwrap();
    session.remove_feature_state(&hover_selector).unwrap();
    let _ = wait_for_runtime_event(&runtime, RuntimeEventType::MapRenderUpdateAvailable);
    let _ = session.render_update();

    let after_remove = session.get_feature_state(&selector).unwrap();
    assert_json_member(&after_remove, "radius", &JsonValue::UInt(20));
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
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::new(64, 64, 1.0)).unwrap();
    let (_context, session) =
        create_owned_texture_session(&map, RenderTargetExtent::new(64, 64, 1.0))
            .expect("Metal or Vulkan owned texture test session should attach when supported");

    let error = session
        .query_rendered_features(
            &RenderedQueryGeometry::point(ScreenPoint::new(32.0, 32.0)),
            None,
        )
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidState);

    load_query_style(&runtime, &map, &session);
    let state_selector = FeatureStateSelector::new("point").with_feature_id("feature-1");
    let query_state = JsonValue::Object(vec![JsonMember::new("selected", JsonValue::Bool(true))]);
    session
        .set_feature_state(&state_selector, &query_state)
        .unwrap();
    let _ = wait_for_runtime_event(&runtime, RuntimeEventType::MapRenderUpdateAvailable);
    let _ = session.render_update();

    let query_point = map
        .pixel_for_lat_lng(LatLng::new(37.7749, -122.4194))
        .unwrap();
    let geometry = RenderedQueryGeometry::box_(ScreenBox::new(
        ScreenPoint::new(query_point.x - 20.0, query_point.y - 20.0),
        ScreenPoint::new(query_point.x + 20.0, query_point.y + 20.0),
    ));
    let filter = JsonValue::Array(vec![
        JsonValue::String("==".into()),
        JsonValue::Array(vec![
            JsonValue::String("get".into()),
            JsonValue::String("kind".into()),
        ]),
        JsonValue::String("capital".into()),
    ]);
    let mut rendered_options = RenderedFeatureQueryOptions::default();
    rendered_options.layer_ids = Some(vec!["point-circle".into()]);
    rendered_options.filter = Some(filter.clone());
    let rendered = wait_for_rendered_feature(
        &runtime,
        &session,
        &geometry,
        &rendered_options,
        "rendered point feature",
    );
    assert_eq!(rendered.source_id.as_deref(), Some("point"));
    assert_eq!(rendered.source_layer_id, None);
    assert_eq!(
        rendered.feature.identifier,
        FeatureIdentifier::String("feature-1".into())
    );
    assert_point_geometry_close(&rendered.feature.geometry, LatLng::new(37.7749, -122.4194));
    assert_eq!(
        feature_member(&rendered.feature, "kind"),
        Some(&JsonValue::String("capital".into()))
    );
    assert_eq!(rendered.state, Some(query_state));

    let mut source_options = SourceFeatureQueryOptions::default();
    source_options.filter = Some(filter);
    let source = wait_for_source_feature(
        &runtime,
        &session,
        "point",
        &source_options,
        "source point feature",
    );
    assert_eq!(source.source_id.as_deref(), Some("point"));
    assert_eq!(source.source_layer_id, None);
    assert_eq!(
        source.feature.identifier,
        FeatureIdentifier::String("feature-1".into())
    );
    assert_point_geometry_close(&source.feature.geometry, LatLng::new(37.7749, -122.4194));
    assert_eq!(
        feature_member(&source.feature, "kind"),
        Some(&JsonValue::String("capital".into()))
    );

    session.close().unwrap();
    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-106.
fn feature_extension_queries_copy_value_and_feature_collection_results() {
    if !has_test_owned_texture_session_backend() {
        return;
    }
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::new(64, 64, 1.0)).unwrap();
    let (_context, session) =
        create_owned_texture_session(&map, RenderTargetExtent::new(64, 64, 1.0))
            .expect("Metal or Vulkan owned texture test session should attach when supported");

    load_cluster_style(&runtime, &map, &session);
    let query_point = map.pixel_for_lat_lng(LatLng::new(0.0, 0.0)).unwrap();
    let geometry = RenderedQueryGeometry::box_(ScreenBox::new(
        ScreenPoint::new(query_point.x - 30.0, query_point.y - 30.0),
        ScreenPoint::new(query_point.x + 30.0, query_point.y + 30.0),
    ));
    let mut options = RenderedFeatureQueryOptions::default();
    options.layer_ids = Some(vec!["cluster-circle".into()]);
    let cluster =
        wait_for_rendered_feature(&runtime, &session, &geometry, &options, "rendered cluster");

    let children = session
        .query_feature_extension(
            "cluster-source",
            &cluster.feature,
            "supercluster",
            "children",
            None,
        )
        .unwrap();
    let FeatureExtensionResult::FeatureCollection(children) = children else {
        panic!("expected children feature collection");
    };
    assert!(!children.is_empty());

    let expansion_zoom = session
        .query_feature_extension(
            "cluster-source",
            &cluster.feature,
            "supercluster",
            "expansion-zoom",
            None,
        )
        .unwrap();
    assert!(matches!(
        expansion_zoom,
        FeatureExtensionResult::Value(JsonValue::UInt(_))
    ));

    session.close().unwrap();
    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Spec coverage: BND-163.
fn owned_texture_session_retains_parent_and_enforces_single_session() {
    if !has_test_owned_texture_session_backend() {
        return;
    }
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::new(64, 64, 1.0)).unwrap();
    let (context, session) =
        create_owned_texture_session(&map, RenderTargetExtent::new(32, 16, 1.0))
            .expect("Metal or Vulkan owned texture test session should attach when supported");

    let error = context
        .attach_owned_texture(&map, RenderTargetExtent::new(32, 16, 1.0))
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidState);

    let error = map.close().unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidState);
    assert!(error.diagnostic().contains("child handles are live"));
    let map = error.into_handle();

    drop(runtime);

    let detached = session.detach().unwrap();
    detached.close().unwrap();
    map.close().unwrap();
}

#[test]
// Spec coverage: BND-165.
fn resize_updates_owned_texture_frame_extent() {
    if !has_test_owned_texture_session_backend() {
        return;
    }
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &static_map_options(64, 64, 1.0)).unwrap();
    let initial_extent = RenderTargetExtent::new(32, 16, 1.0);
    let resized_extent = RenderTargetExtent::new(48, 24, 1.0);
    let (context, session) = create_owned_texture_session(&map, initial_extent)
        .expect("Metal or Vulkan owned texture test session should attach when supported");

    load_query_style(&runtime, &map, &session);
    session
        .resize(
            resized_extent.width,
            resized_extent.height,
            resized_extent.scale_factor,
        )
        .unwrap();
    map.request_still_image().unwrap();
    for _ in 0..200 {
        let _ = runtime.run_once();
        while let Ok(Some(event)) = runtime.poll_event() {
            if event.event_type != RuntimeEventType::MapRenderUpdateAvailable {
                continue;
            }
            let _ = session.render_update();
        }
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
// Spec coverage: BND-170.
fn acquired_frame_state_rejects_reentrant_session_operations_before_native_calls() {
    if !has_test_owned_texture_session_backend() {
        return;
    }
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &static_map_options(64, 64, 1.0)).unwrap();
    let (_context, session) =
        create_owned_texture_session(&map, RenderTargetExtent::new(32, 16, 1.0))
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
        session
            .set_feature_state(&selector, &JsonValue::Object(Vec::new()))
            .unwrap_err(),
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
                &Feature::new(crate::Geometry::Empty, Vec::new()),
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
fn render_update_without_pending_update_maps_invalid_state_and_keeps_session_live() {
    if !has_test_owned_texture_session_backend() {
        return;
    }
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &static_map_options(64, 64, 1.0)).unwrap();
    let (_context, session) =
        create_owned_texture_session(&map, RenderTargetExtent::new(32, 16, 1.0))
            .expect("Metal or Vulkan owned texture test session should attach when supported");

    let error = session.render_update().unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidState);
    assert_eq!(error.raw_status(), Some(sys::MLN_STATUS_INVALID_STATE));

    session.close().unwrap();
    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
// Rust regression: failed readback before native has produced a readable frame
// must preserve caller-owned buffer bytes even though BND-166 covers the later
// deterministic metadata/capacity/read path.
fn texture_readback_without_rendered_frame_maps_native_invalid_state() {
    if !has_test_owned_texture_session_backend() {
        return;
    }
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &static_map_options(64, 64, 1.0)).unwrap();
    let (_context, session) =
        create_owned_texture_session(&map, RenderTargetExtent::new(32, 16, 1.0))
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
    let runtime = RuntimeHandle::with_options(&crate::RuntimeOptions::default()).unwrap();
    let map = MapHandle::with_options(&runtime, &static_map_options(64, 64, 1.0)).unwrap();
    let (_context, session) =
        create_owned_texture_session(&map, RenderTargetExtent::new(32, 16, 1.0))
            .expect("Metal or Vulkan owned texture test session should attach when supported");

    load_query_style(&runtime, &map, &session);
    map.request_still_image().unwrap();
    let mut info = None;
    for _ in 0..200 {
        let _ = runtime.run_once();
        while let Ok(Some(event)) = runtime.poll_event() {
            if event.event_type == RuntimeEventType::MapRenderUpdateAvailable {
                let _ = session.render_update();
            }
        }
        match session.texture_image_info() {
            Ok(copied) => {
                info = Some(copied);
                break;
            }
            Err(error) if error.kind() == ErrorKind::Unsupported => {
                session.close().unwrap();
                map.close().unwrap();
                runtime.close().unwrap();
                return;
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

    let mut reusable = vec![0; info.byte_length];
    let copied_info = session
        .read_premultiplied_rgba8_into(&mut reusable)
        .unwrap();
    assert_eq!(copied_info, info);
    assert_eq!(reusable.len(), info.byte_length);

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
        .attach_opengl_owned_texture(&OpenGLOwnedTextureDescriptor::new(
            RenderTargetExtent::new(0, 16, 1.0),
            opengl_context.clone(),
        ))
        .unwrap_err();
    assert_eq!(opengl_error.kind(), ErrorKind::InvalidArgument);

    let opengl_error = map
        .attach_opengl_borrowed_texture(&OpenGLBorrowedTextureDescriptor::new(
            RenderTargetExtent::new(32, 16, 1.0),
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
        .attach_opengl_owned_texture(&OpenGLOwnedTextureDescriptor::new(
            RenderTargetExtent::new(32, 16, 1.0),
            opengl_context.clone(),
        ))
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::Unsupported);
    assert_eq!(error.raw_status(), Some(sys::MLN_STATUS_UNSUPPORTED));

    let error = map
        .attach_opengl_borrowed_texture(&OpenGLBorrowedTextureDescriptor::new(
            RenderTargetExtent::new(32, 16, 1.0),
            opengl_context.clone(),
            1,
            glow::TEXTURE_2D,
        ))
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::Unsupported);
    assert_eq!(error.raw_status(), Some(sys::MLN_STATUS_UNSUPPORTED));

    let error = map
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
