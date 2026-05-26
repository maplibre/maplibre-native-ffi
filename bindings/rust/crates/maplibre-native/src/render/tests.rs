use std::error::Error as StdError;
use std::ffi::{CStr, CString};
use std::time::Duration;

use ash::vk;
use ash::vk::Handle;
use static_assertions::assert_not_impl_any;
#[cfg(target_os = "windows")]
use windows_sys::Win32::Foundation::HWND;
#[cfg(target_os = "windows")]
use windows_sys::Win32::Graphics::Gdi::{GetDC, ReleaseDC};
#[cfg(target_os = "windows")]
use windows_sys::Win32::Graphics::OpenGL::{
    ChoosePixelFormat, PFD_DOUBLEBUFFER, PFD_DRAW_TO_WINDOW, PFD_MAIN_PLANE, PFD_SUPPORT_OPENGL,
    PFD_TYPE_RGBA, PIXELFORMATDESCRIPTOR, SetPixelFormat, wglCreateContext, wglDeleteContext,
    wglGetProcAddress, wglMakeCurrent,
};
#[cfg(target_os = "windows")]
use windows_sys::Win32::System::LibraryLoader::GetModuleHandleW;
#[cfg(target_os = "windows")]
use windows_sys::Win32::UI::WindowsAndMessaging::{
    CS_OWNDC, CreateWindowExW, DefWindowProcW, DestroyWindow, RegisterClassW, WNDCLASSW,
    WS_OVERLAPPEDWINDOW,
};

use super::*;
use crate::{
    CameraOptions, ErrorKind, JsonMember, LatLng, MapMode, MapOptions, OpenGLContextProviderMask,
    RenderBackendMask, RuntimeEventType, RuntimeHandle, ScreenBox, ScreenPoint,
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
#[cfg(target_os = "windows")]
const GL_NO_ERROR: u32 = 0;
const GL_TEXTURE_2D: u32 = 0x0de1;
#[cfg(target_os = "windows")]
const GL_RGBA: u32 = 0x1908;
#[cfg(target_os = "windows")]
const GL_UNSIGNED_BYTE: u32 = 0x1401;
#[cfg(target_os = "windows")]
const GL_RGBA8: i32 = 0x8058;
#[cfg(target_os = "windows")]
const GL_TEXTURE_MAG_FILTER: u32 = 0x2800;
#[cfg(target_os = "windows")]
const GL_TEXTURE_MIN_FILTER: u32 = 0x2801;
#[cfg(target_os = "windows")]
const GL_NEAREST: i32 = 0x2600;

// Keep this to the OpenGL 1.1 symbols exported by opengl32.dll; adding a GL
// loader crate here would only support the Windows test helper.
#[cfg(target_os = "windows")]
#[link(name = "opengl32")]
unsafe extern "system" {
    fn glBindTexture(target: u32, texture: u32);
    fn glDeleteTextures(n: i32, textures: *const u32);
    fn glGenTextures(n: i32, textures: *mut u32);
    fn glGetError() -> u32;
    fn glGetTexImage(
        target: u32,
        level: i32,
        format: u32,
        type_: u32,
        pixels: *mut std::ffi::c_void,
    );
    fn glTexImage2D(
        target: u32,
        level: i32,
        internal_format: i32,
        width: i32,
        height: i32,
        border: i32,
        format: u32,
        type_: u32,
        pixels: *const std::ffi::c_void,
    );
    fn glTexParameteri(target: u32, pname: u32, param: i32);
}

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
        GL_TEXTURE_2D,
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

#[cfg(target_os = "windows")]
struct WglWindow {
    hwnd: HWND,
}

#[cfg(target_os = "windows")]
impl WglWindow {
    fn new() -> std::result::Result<Self, Box<dyn StdError>> {
        let class_name: Vec<u16> = "MaplibreNativeRustWglTest\0".encode_utf16().collect();
        // SAFETY: Passing null requests the current process module handle.
        let hinstance = unsafe { GetModuleHandleW(std::ptr::null()) };
        if hinstance.is_null() {
            return Err("GetModuleHandleW returned null".into());
        }
        let class = WNDCLASSW {
            style: CS_OWNDC,
            lpfnWndProc: Some(DefWindowProcW),
            hInstance: hinstance,
            lpszClassName: class_name.as_ptr(),
            ..unsafe { std::mem::zeroed() }
        };
        // SAFETY: class points to stable storage for the duration of the call.
        let _ = unsafe { RegisterClassW(&class) };
        // SAFETY: The class was registered above or by an earlier test in this process.
        let hwnd = unsafe {
            CreateWindowExW(
                0,
                class_name.as_ptr(),
                class_name.as_ptr(),
                WS_OVERLAPPEDWINDOW,
                0,
                0,
                8,
                8,
                std::ptr::null_mut(),
                std::ptr::null_mut(),
                hinstance,
                std::ptr::null(),
            )
        };
        if hwnd.is_null() {
            return Err("CreateWindowExW returned null".into());
        }
        Ok(Self { hwnd })
    }
}

#[cfg(target_os = "windows")]
impl Drop for WglWindow {
    fn drop(&mut self) {
        // SAFETY: hwnd belongs to this helper window.
        unsafe {
            DestroyWindow(self.hwnd);
        }
    }
}

struct OpenGLTestContext {
    #[cfg(target_os = "windows")]
    window: WglWindow,
    #[cfg(target_os = "windows")]
    device_context: NativePointer,
    #[cfg(target_os = "windows")]
    share_context: NativePointer,
}

impl OpenGLTestContext {
    fn new() -> std::result::Result<Self, Box<dyn StdError>> {
        #[cfg(target_os = "windows")]
        {
            let window = WglWindow::new()?;
            // SAFETY: hwnd is a live helper window with CS_OWNDC.
            let hdc = unsafe { GetDC(window.hwnd) };
            if hdc.is_null() {
                return Err("GetDC returned null".into());
            }
            let pfd = PIXELFORMATDESCRIPTOR {
                nSize: std::mem::size_of::<PIXELFORMATDESCRIPTOR>() as u16,
                nVersion: 1,
                dwFlags: PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
                iPixelType: PFD_TYPE_RGBA,
                cColorBits: 32,
                cDepthBits: 24,
                cStencilBits: 8,
                iLayerType: PFD_MAIN_PLANE as u8,
                ..unsafe { std::mem::zeroed() }
            };
            // SAFETY: hdc is live and pfd points to initialized storage.
            let pixel_format = unsafe { ChoosePixelFormat(hdc, &pfd) };
            if pixel_format == 0 {
                // SAFETY: hdc came from this window.
                unsafe {
                    ReleaseDC(window.hwnd, hdc);
                }
                return Err("ChoosePixelFormat returned 0".into());
            }
            // SAFETY: hdc is live and pfd describes the selected pixel format.
            if unsafe { SetPixelFormat(hdc, pixel_format, &pfd) } == 0 {
                unsafe {
                    ReleaseDC(window.hwnd, hdc);
                }
                return Err("SetPixelFormat failed".into());
            }
            // SAFETY: hdc has an OpenGL-capable pixel format.
            let hglrc = unsafe { wglCreateContext(hdc) };
            if hglrc.is_null() {
                unsafe {
                    ReleaseDC(window.hwnd, hdc);
                }
                return Err("wglCreateContext returned null".into());
            }
            // SAFETY: Make the host context current once so WGL extension
            // lookups and texture share-group behavior are available.
            if unsafe { wglMakeCurrent(hdc, hglrc) } == 0 {
                unsafe {
                    wglDeleteContext(hglrc);
                    ReleaseDC(window.hwnd, hdc);
                }
                return Err("wglMakeCurrent failed".into());
            }
            Ok(Self {
                window,
                // SAFETY: The WGL handles remain live for the test context lifetime.
                device_context: unsafe { NativePointer::from_ptr(hdc) },
                share_context: unsafe { NativePointer::from_ptr(hglrc) },
            })
        }

        #[cfg(target_os = "linux")]
        {
            // TODO(linux): Add an EGL pbuffer/context helper for Rust OpenGL
            // binding tests on a Linux machine with the CI EGL/llvmpipe stack.
            Err("OpenGL EGL test context is not available in Rust tests yet".into())
        }

        #[cfg(not(any(target_os = "windows", target_os = "linux")))]
        {
            Err("OpenGL test context is only available on Windows WGL".into())
        }
    }

    fn descriptor(&self) -> OpenGLContextDescriptor {
        #[cfg(target_os = "windows")]
        {
            OpenGLContextDescriptor::wgl(
                WglContextDescriptor::new(self.device_context, self.share_context)
                    .with_proc_address(unsafe {
                        NativePointer::from_address(wglGetProcAddress as *const () as usize)
                    }),
            )
        }

        #[cfg(not(target_os = "windows"))]
        {
            unreachable!("OpenGLTestContext::new is only implemented on Windows")
        }
    }

    fn surface(&self) -> NativePointer {
        #[cfg(target_os = "windows")]
        {
            self.device_context
        }

        #[cfg(target_os = "linux")]
        {
            // TODO(linux): Return an EGLSurface once the Rust EGL helper exists.
            unreachable!("OpenGLTestContext::new is not implemented on Linux yet")
        }

        #[cfg(not(any(target_os = "windows", target_os = "linux")))]
        {
            unreachable!("OpenGL test surfaces are only available on Windows WGL")
        }
    }

    fn make_current(&self) -> std::result::Result<(), Box<dyn StdError>> {
        #[cfg(target_os = "windows")]
        {
            // SAFETY: The WGL handles belong to this helper and remain live for
            // its lifetime.
            if unsafe {
                wglMakeCurrent(
                    self.device_context.as_ptr::<std::ffi::c_void>(),
                    self.share_context.as_ptr::<std::ffi::c_void>(),
                )
            } == 0
            {
                Err("wglMakeCurrent failed".into())
            } else {
                Ok(())
            }
        }

        #[cfg(not(target_os = "windows"))]
        {
            // TODO(linux): Make the EGL context current for Rust OpenGL helpers.
            Err("OpenGL make-current helper is only available on Windows WGL".into())
        }
    }

    fn check_gl_error(&self, operation: &str) -> std::result::Result<(), Box<dyn StdError>> {
        #[cfg(target_os = "windows")]
        {
            // SAFETY: A current context exists when callers check GL state.
            let error = unsafe { glGetError() };
            if error == GL_NO_ERROR {
                Ok(())
            } else {
                Err(format!("{operation} failed with OpenGL error 0x{error:x}").into())
            }
        }

        #[cfg(not(target_os = "windows"))]
        {
            let _ = operation;
            // TODO(linux): Check EGL/OpenGL errors once the Linux helper exists.
            Err("OpenGL error checking is only available on Windows WGL".into())
        }
    }
}

#[cfg(target_os = "windows")]
impl Drop for OpenGLTestContext {
    fn drop(&mut self) {
        // SAFETY: Handles belong to this test context and are released in
        // dependency order after waiting for current-context teardown.
        unsafe {
            let hdc = self.device_context.as_ptr::<std::ffi::c_void>();
            let hglrc = self.share_context.as_ptr::<std::ffi::c_void>();
            let _ = wglMakeCurrent(std::ptr::null_mut(), std::ptr::null_mut());
            wglDeleteContext(hglrc);
            ReleaseDC(self.window.hwnd, hdc);
        }
    }
}

struct OpenGLBorrowedTexture {
    context: OpenGLTestContext,
    texture: u32,
    width: u32,
    height: u32,
}

impl OpenGLBorrowedTexture {
    fn new(width: u32, height: u32) -> std::result::Result<Self, Box<dyn StdError>> {
        let context = OpenGLTestContext::new()?;
        #[cfg(target_os = "windows")]
        {
            context.make_current()?;
            let mut texture = 0;
            // SAFETY: A current WGL context exists on this thread, and the
            // texture object stays in the context share group for this helper's lifetime.
            unsafe {
                glGenTextures(1, &mut texture);
                glBindTexture(GL_TEXTURE_2D, texture);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTexImage2D(
                    GL_TEXTURE_2D,
                    0,
                    GL_RGBA8,
                    width as i32,
                    height as i32,
                    0,
                    GL_RGBA,
                    GL_UNSIGNED_BYTE,
                    std::ptr::null(),
                );
                glBindTexture(GL_TEXTURE_2D, 0);
            }
            context.check_gl_error("create borrowed texture")?;
            Ok(Self {
                context,
                texture,
                width,
                height,
            })
        }

        #[cfg(not(target_os = "windows"))]
        {
            let _ = (context, width, height);
            // TODO(linux): Allocate an EGL-shared GL texture for Rust binding
            // borrowed-texture tests once the Linux EGL helper is validated.
            Err("OpenGL borrowed texture helper is only available on Windows WGL".into())
        }
    }

    fn descriptor(&self) -> OpenGLContextDescriptor {
        self.context.descriptor()
    }

    fn name(&self) -> u32 {
        self.texture
    }

    fn read_rgba(&self) -> std::result::Result<Vec<u8>, Box<dyn StdError>> {
        #[cfg(target_os = "windows")]
        {
            self.context.make_current()?;
            let mut pixels = vec![0_u8; self.width as usize * self.height as usize * 4];
            // SAFETY: The texture belongs to the current context share group,
            // and pixels points to enough writable storage for the RGBA8 image.
            unsafe {
                glBindTexture(GL_TEXTURE_2D, self.texture);
                glGetTexImage(
                    GL_TEXTURE_2D,
                    0,
                    GL_RGBA,
                    GL_UNSIGNED_BYTE,
                    pixels.as_mut_ptr().cast(),
                );
                glBindTexture(GL_TEXTURE_2D, 0);
            }
            self.context.check_gl_error("read borrowed texture")?;
            Ok(pixels)
        }

        #[cfg(not(target_os = "windows"))]
        {
            // TODO(linux): Read back from the EGL texture helper once it exists.
            Err("OpenGL borrowed texture readback is only available on Windows WGL".into())
        }
    }
}

impl Drop for OpenGLBorrowedTexture {
    fn drop(&mut self) {
        #[cfg(target_os = "windows")]
        {
            if self.texture != 0 && self.context.make_current().is_ok() {
                // SAFETY: The texture was created by this helper in the current
                // context share group and has not been deleted yet.
                unsafe {
                    glDeleteTextures(1, &self.texture);
                }
                self.texture = 0;
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
        VulkanContextDescriptor::new(
            // SAFETY: Vulkan handles remain live for the test context lifetime.
            unsafe { NativePointer::from_address(self.instance.handle().as_raw() as usize) },
            unsafe { NativePointer::from_address(self.physical_device.as_raw() as usize) },
            unsafe { NativePointer::from_address(self.device.handle().as_raw() as usize) },
            unsafe { NativePointer::from_address(self.graphics_queue.as_raw() as usize) },
            self.graphics_queue_family_index,
        )
        .with_proc_addresses(
            // SAFETY: Function pointers remain valid while the ash entry and instance are live.
            unsafe {
                NativePointer::from_address(
                    self._entry.static_fn().get_instance_proc_addr as *const () as usize,
                )
            },
            unsafe {
                NativePointer::from_address(
                    self.instance.fp_v1_0().get_device_proc_addr as *const () as usize,
                )
            },
        )
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
    map.jump_to(
        &CameraOptions::new()
            .with_center(LatLng::new(37.7749, -122.4194))
            .with_zoom(10.0),
    )
    .unwrap();
    map.set_style_json(QUERY_STYLE_JSON).unwrap();
    render_available_updates(runtime, session, 5);
}

fn load_cluster_style(runtime: &RuntimeHandle, map: &MapHandle, session: &RenderSessionHandle) {
    map.jump_to(
        &CameraOptions::new()
            .with_center(LatLng::new(0.0, 0.0))
            .with_zoom(0.0),
    )
    .unwrap();
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
fn opengl_owned_texture_session_attaches_with_platform_context() {
    let runtime = RuntimeHandle::new().unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::new(64, 64, 1.0)).unwrap();
    let Ok((_context, session)) =
        create_opengl_owned_texture_session(&map, RenderTargetExtent::new(32, 16, 1.0))
    else {
        map.close().unwrap();
        runtime.close().unwrap();
        return;
    };

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
    assert_eq!(frame.frame().unwrap().target, GL_TEXTURE_2D);
    assert_eq!(frame.frame().unwrap().internal_format, GL_RGBA8 as u32);
    assert_eq!(frame.frame().unwrap().format, GL_RGBA);
    assert_eq!(frame.frame().unwrap().type_, GL_UNSIGNED_BYTE);
    assert!(!frame.texture().unwrap().is_zero());
    frame.close().unwrap();

    session.close().unwrap();
    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
fn opengl_surface_session_renders_with_platform_context() {
    let runtime = RuntimeHandle::new().unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::new(64, 64, 1.0)).unwrap();

    let Ok((_context, session)) =
        create_opengl_surface_session(&map, RenderTargetExtent::new(32, 16, 1.0))
    else {
        map.close().unwrap();
        runtime.close().unwrap();
        return;
    };

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
fn opengl_borrowed_texture_session_renders_with_platform_context() {
    let runtime = RuntimeHandle::new().unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::new(128, 128, 1.0)).unwrap();

    let Ok((texture, session)) =
        create_opengl_borrowed_texture_session(&map, RenderTargetExtent::new(128, 128, 1.0))
    else {
        map.close().unwrap();
        runtime.close().unwrap();
        return;
    };

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
    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
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

#[test]
fn feature_state_set_get_and_remove_copy_snapshots() {
    let runtime = RuntimeHandle::new().unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::new(64, 64, 1.0)).unwrap();
    let Ok((_context, session)) =
        create_owned_texture_session(&map, RenderTargetExtent::new(64, 64, 1.0))
    else {
        return;
    };
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
fn rendered_and_source_queries_copy_results() {
    let runtime = RuntimeHandle::new().unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::new(64, 64, 1.0)).unwrap();
    let Ok((_context, session)) =
        create_owned_texture_session(&map, RenderTargetExtent::new(64, 64, 1.0))
    else {
        return;
    };

    let error = session
        .query_rendered_features(
            &RenderedQueryGeometry::point(ScreenPoint::new(32.0, 32.0)),
            None,
        )
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::InvalidState);

    load_query_style(&runtime, &map, &session);
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
    let rendered_options = RenderedFeatureQueryOptions::new()
        .with_layer_ids(vec!["point-circle".into()])
        .with_filter(filter.clone());
    let rendered = wait_for_rendered_feature(
        &runtime,
        &session,
        &geometry,
        &rendered_options,
        "rendered point feature",
    );
    assert_eq!(rendered.source_id.as_deref(), Some("point"));
    assert_eq!(
        feature_member(&rendered.feature, "kind"),
        Some(&JsonValue::String("capital".into()))
    );

    let source_options = SourceFeatureQueryOptions::new().with_filter(filter);
    let source = wait_for_source_feature(
        &runtime,
        &session,
        "point",
        &source_options,
        "source point feature",
    );
    assert_eq!(source.source_id.as_deref(), Some("point"));
    assert_eq!(
        feature_member(&source.feature, "kind"),
        Some(&JsonValue::String("capital".into()))
    );

    session.close().unwrap();
    map.close().unwrap();
    runtime.close().unwrap();
}

#[test]
fn feature_extension_queries_copy_value_and_feature_collection_results() {
    let runtime = RuntimeHandle::new().unwrap();
    let map = MapHandle::with_options(&runtime, &MapOptions::new(64, 64, 1.0)).unwrap();
    let Ok((_context, session)) =
        create_owned_texture_session(&map, RenderTargetExtent::new(64, 64, 1.0))
    else {
        return;
    };

    load_cluster_style(&runtime, &map, &session);
    let query_point = map.pixel_for_lat_lng(LatLng::new(0.0, 0.0)).unwrap();
    let geometry = RenderedQueryGeometry::box_(ScreenBox::new(
        ScreenPoint::new(query_point.x - 30.0, query_point.y - 30.0),
        ScreenPoint::new(query_point.x + 30.0, query_point.y + 30.0),
    ));
    let options = RenderedFeatureQueryOptions::new().with_layer_ids(vec!["cluster-circle".into()]);
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
fn owned_texture_session_retains_parent_and_enforces_single_session() {
    let runtime = RuntimeHandle::new().unwrap();
    let map = MapHandle::with_options(
        &runtime,
        &MapOptions::new(64, 64, 1.0).with_mode(MapMode::Static),
    )
    .unwrap();
    let Ok((context, session)) =
        create_owned_texture_session(&map, RenderTargetExtent::new(32, 16, 1.0))
    else {
        return;
    };

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
fn acquired_frame_state_rejects_reentrant_session_operations_before_native_calls() {
    let runtime = RuntimeHandle::new().unwrap();
    let map = MapHandle::with_options(
        &runtime,
        &MapOptions::new(64, 64, 1.0).with_mode(MapMode::Static),
    )
    .unwrap();
    let Ok((_context, session)) =
        create_owned_texture_session(&map, RenderTargetExtent::new(32, 16, 1.0))
    else {
        return;
    };

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
fn texture_readback_reports_documented_error_kinds_for_unsized_buffer() {
    let runtime = RuntimeHandle::new().unwrap();
    let map = MapHandle::with_options(
        &runtime,
        &MapOptions::new(64, 64, 1.0).with_mode(MapMode::Static),
    )
    .unwrap();
    let Ok((_context, session)) =
        create_owned_texture_session(&map, RenderTargetExtent::new(32, 16, 1.0))
    else {
        return;
    };

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
            )
            .with_proc_addresses(NativePointer::NULL, NativePointer::NULL),
            NativePointer::NULL,
        ))
        .unwrap_err();
    assert!(matches!(
        vulkan_error.kind(),
        ErrorKind::InvalidArgument | ErrorKind::Unsupported
    ));

    let opengl_context = OpenGLContextDescriptor::wgl(WglContextDescriptor::new(
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
fn opengl_attach_calls_report_unsupported_when_backend_unavailable() {
    if crate::supported_render_backends().contains(RenderBackendMask::OPENGL) {
        return;
    }

    let runtime = RuntimeHandle::new().unwrap();
    let map = MapHandle::with_options(
        &runtime,
        &MapOptions::new(64, 64, 1.0).with_mode(MapMode::Static),
    )
    .unwrap();
    // SAFETY: Test uses dummy opaque addresses and never dereferences them.
    let fake = unsafe { NativePointer::from_address(1) };
    let opengl_context = OpenGLContextDescriptor::wgl(WglContextDescriptor::new(fake, fake));

    let error = map
        .attach_opengl_owned_texture(&OpenGLOwnedTextureDescriptor::new(
            RenderTargetExtent::new(32, 16, 1.0),
            opengl_context.clone(),
        ))
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::Unsupported);

    let error = map
        .attach_opengl_borrowed_texture(&OpenGLBorrowedTextureDescriptor::new(
            RenderTargetExtent::new(32, 16, 1.0),
            opengl_context.clone(),
            1,
            GL_TEXTURE_2D,
        ))
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::Unsupported);

    let error = map
        .attach_opengl_surface(&OpenGLSurfaceDescriptor::new(
            RenderTargetExtent::new(32, 16, 1.0),
            opengl_context,
            fake,
        ))
        .unwrap_err();
    assert_eq!(error.kind(), ErrorKind::Unsupported);

    map.close().unwrap();
    runtime.close().unwrap();
}
