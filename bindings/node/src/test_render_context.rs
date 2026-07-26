//! Private N-API render contexts for end-to-end JavaScript tests.
//!
//! This module is compiled only by the dedicated test-support addon build. Its
//! raw address values stay outside the package's public JavaScript and
//! TypeScript entrypoints.

#[cfg(node_test_backend = "metal")]
use std::ffi::c_void;

use napi::bindgen_prelude::{BigInt, Result};
use napi_derive::napi;

use crate::error;

#[napi(object)]
pub struct NativeTestRenderContextDescriptor {
    pub backend: String,
    pub device_address: Option<BigInt>,
    pub device_context_address: Option<BigInt>,
    pub instance_address: Option<BigInt>,
    pub physical_device_address: Option<BigInt>,
    pub graphics_queue_address: Option<BigInt>,
    pub graphics_queue_family_index: Option<u32>,
    pub get_instance_proc_addr_address: Option<BigInt>,
    pub get_device_proc_addr_address: Option<BigInt>,
    pub display_address: Option<BigInt>,
    pub config_address: Option<BigInt>,
    pub share_context_address: Option<BigInt>,
    pub get_proc_address_address: Option<BigInt>,
}

impl NativeTestRenderContextDescriptor {
    fn empty(backend: &str) -> Self {
        Self {
            backend: backend.to_owned(),
            device_address: None,
            device_context_address: None,
            instance_address: None,
            physical_device_address: None,
            graphics_queue_address: None,
            graphics_queue_family_index: None,
            get_instance_proc_addr_address: None,
            get_device_proc_addr_address: None,
            display_address: None,
            config_address: None,
            share_context_address: None,
            get_proc_address_address: None,
        }
    }
}

fn address(value: usize) -> BigInt {
    BigInt::from(value as u64)
}

#[napi(js_name = "NativeTestRenderContext")]
pub struct NativeTestRenderContext {
    context: Option<BackendContext>,
}

#[napi]
impl NativeTestRenderContext {
    #[napi(factory)]
    pub fn create() -> Result<Self> {
        Ok(Self {
            context: Some(BackendContext::create()?),
        })
    }

    #[napi]
    pub fn descriptor(&self) -> Result<NativeTestRenderContextDescriptor> {
        self.context
            .as_ref()
            .map(BackendContext::descriptor)
            .ok_or_else(|| error::invalid_state("test render context is closed"))
    }

    #[napi]
    pub fn close(&mut self) {
        self.context.take();
    }

    #[napi(getter)]
    pub fn closed(&self) -> bool {
        self.context.is_none()
    }
}

#[cfg(node_test_backend = "metal")]
struct BackendContext {
    device: *mut c_void,
}

#[cfg(node_test_backend = "metal")]
impl BackendContext {
    fn create() -> Result<Self> {
        // SAFETY: The system Metal factory returns an opaque Objective-C object
        // whose address stays valid for this short-lived configured test.
        let device = unsafe { MTLCreateSystemDefaultDevice() };
        if device.is_null() {
            return Err(error::invalid_state(
                "Metal did not return a default device for the test context",
            ));
        }
        // SAFETY: Retaining converts the autoreleased factory result into
        // fixture-owned state released by Drop.
        let device = unsafe { objc_retain(device) };
        Ok(Self { device })
    }

    fn descriptor(&self) -> NativeTestRenderContextDescriptor {
        let mut descriptor = NativeTestRenderContextDescriptor::empty("metal");
        descriptor.device_address = Some(address(self.device as usize));
        descriptor
    }
}

#[cfg(node_test_backend = "metal")]
impl Drop for BackendContext {
    fn drop(&mut self) {
        // SAFETY: create retained this Objective-C object for exactly the
        // fixture lifetime, and the render session is closed before the fixture.
        unsafe { objc_release(self.device) };
    }
}

#[cfg(node_test_backend = "metal")]
#[link(name = "Metal", kind = "framework")]
unsafe extern "C" {
    fn MTLCreateSystemDefaultDevice() -> *mut c_void;
}

#[cfg(node_test_backend = "metal")]
#[link(name = "objc")]
unsafe extern "C" {
    fn objc_retain(value: *mut c_void) -> *mut c_void;
    fn objc_release(value: *mut c_void);
}

#[cfg(node_test_backend = "egl")]
mod egl_backend {
    use std::{path::Path, ptr};

    use khronos_egl as egl;
    use napi::bindgen_prelude::Result;

    use super::{NativeTestRenderContextDescriptor, address, error};

    #[cfg(target_os = "linux")]
    const EGL_PLATFORM_SURFACELESS_MESA: egl::Enum = 0x31DD;
    #[cfg(target_os = "macos")]
    const EGL_PLATFORM_ANGLE_ANGLE: egl::Enum = 0x3202;
    #[cfg(target_os = "macos")]
    const EGL_PLATFORM_ANGLE_TYPE_ANGLE: egl::Attrib = 0x3203;
    #[cfg(target_os = "macos")]
    const EGL_PLATFORM_ANGLE_TYPE_METAL_ANGLE: egl::Attrib = 0x3489;
    #[cfg(target_os = "macos")]
    const EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE: egl::Attrib = 0x3209;
    #[cfg(target_os = "macos")]
    const EGL_PLATFORM_ANGLE_DEVICE_TYPE_HARDWARE_ANGLE: egl::Attrib = 0x320A;

    pub(super) struct BackendContext {
        egl: egl::DynamicInstance<egl::EGL1_5>,
        display: egl::Display,
        config: egl::Config,
        surface: egl::Surface,
        context: egl::Context,
    }

    impl BackendContext {
        pub(super) fn create() -> Result<Self> {
            let egl = load_egl()?;
            let display = create_display(&egl)?;
            egl.initialize(display)
                .map_err(|failure| egl_error("eglInitialize", failure))?;
            if let Err(failure) = egl.bind_api(egl::OPENGL_ES_API) {
                let _ = egl.terminate(display);
                return Err(egl_error("eglBindAPI", failure));
            }

            let config_attributes = [
                egl::SURFACE_TYPE,
                egl::PBUFFER_BIT,
                egl::RENDERABLE_TYPE,
                egl::OPENGL_ES3_BIT,
                egl::RED_SIZE,
                8,
                egl::GREEN_SIZE,
                8,
                egl::BLUE_SIZE,
                8,
                egl::ALPHA_SIZE,
                8,
                egl::DEPTH_SIZE,
                24,
                egl::STENCIL_SIZE,
                8,
                egl::NONE,
            ];
            let config = match egl.choose_first_config(display, &config_attributes) {
                Ok(Some(config)) => config,
                Ok(None) => {
                    let _ = egl.terminate(display);
                    return Err(error::invalid_state(
                        "eglChooseConfig returned no matching test configuration",
                    ));
                }
                Err(failure) => {
                    let _ = egl.terminate(display);
                    return Err(egl_error("eglChooseConfig", failure));
                }
            };

            let context_attributes = [egl::CONTEXT_CLIENT_VERSION, 3, egl::NONE];
            let context = match egl.create_context(display, config, None, &context_attributes) {
                Ok(context) => context,
                Err(failure) => {
                    let _ = egl.terminate(display);
                    return Err(egl_error("eglCreateContext", failure));
                }
            };

            let surface_attributes = [egl::WIDTH, 8, egl::HEIGHT, 8, egl::NONE];
            let surface = match egl.create_pbuffer_surface(display, config, &surface_attributes) {
                Ok(surface) => surface,
                Err(failure) => {
                    let _ = egl.destroy_context(display, context);
                    let _ = egl.terminate(display);
                    return Err(egl_error("eglCreatePbufferSurface", failure));
                }
            };
            if let Err(failure) =
                egl.make_current(display, Some(surface), Some(surface), Some(context))
            {
                let _ = egl.destroy_surface(display, surface);
                let _ = egl.destroy_context(display, context);
                let _ = egl.terminate(display);
                return Err(egl_error("eglMakeCurrent", failure));
            }

            Ok(Self {
                egl,
                display,
                config,
                surface,
                context,
            })
        }

        pub(super) fn descriptor(&self) -> NativeTestRenderContextDescriptor {
            let mut descriptor = NativeTestRenderContextDescriptor::empty("egl");
            descriptor.display_address = Some(address(self.display.as_ptr() as usize));
            descriptor.config_address = Some(address(self.config.as_ptr() as usize));
            descriptor.share_context_address = Some(address(self.context.as_ptr() as usize));
            descriptor
        }
    }

    impl Drop for BackendContext {
        fn drop(&mut self) {
            let _ = self.egl.make_current(self.display, None, None, None);
            let _ = self.egl.destroy_surface(self.display, self.surface);
            let _ = self.egl.destroy_context(self.display, self.context);
            let _ = self.egl.terminate(self.display);
        }
    }

    fn create_display(egl: &egl::DynamicInstance<egl::EGL1_5>) -> Result<egl::Display> {
        #[cfg(target_os = "macos")]
        {
            let attributes = [
                EGL_PLATFORM_ANGLE_TYPE_ANGLE,
                EGL_PLATFORM_ANGLE_TYPE_METAL_ANGLE,
                EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE,
                EGL_PLATFORM_ANGLE_DEVICE_TYPE_HARDWARE_ANGLE,
                egl::ATTRIB_NONE,
            ];
            // SAFETY: The attribute list is terminated and ANGLE accepts a null
            // native display for its Metal platform.
            unsafe {
                egl.get_platform_display(EGL_PLATFORM_ANGLE_ANGLE, ptr::null_mut(), &attributes)
            }
            .map_err(|failure| egl_error("eglGetPlatformDisplay", failure))
        }
        #[cfg(target_os = "linux")]
        {
            let attributes = [egl::ATTRIB_NONE];
            // SAFETY: The attribute list is terminated and the surfaceless
            // platform takes a null native display.
            unsafe {
                egl.get_platform_display(
                    EGL_PLATFORM_SURFACELESS_MESA,
                    ptr::null_mut(),
                    &attributes,
                )
            }
            .map_err(|failure| egl_error("eglGetPlatformDisplay", failure))
        }
    }

    fn load_egl() -> Result<egl::DynamicInstance<egl::EGL1_5>> {
        let configured = std::env::var_os("MAPLIBRE_NATIVE_C_INSTALL_DIR")
            .map(|install_dir| Path::new(&install_dir).join("lib"));
        let candidates: &[&str] = if cfg!(target_os = "macos") {
            &["libEGL.dylib"]
        } else {
            &["libEGL.so.1", "libEGL.so"]
        };
        if let Some(directory) = configured {
            for name in candidates {
                let path = directory.join(name);
                if path.exists() {
                    // SAFETY: The configured native build supplies an EGL 1.5
                    // implementation used by the matching MapLibre backend.
                    return unsafe {
                        egl::DynamicInstance::<egl::EGL1_5>::load_required_from_filename(&path)
                    }
                    .map_err(|failure| {
                        error::invalid_state(format!(
                            "failed to load configured EGL runtime {}: {failure:?}",
                            path.display()
                        ))
                    });
                }
            }
        }
        for name in candidates {
            // SAFETY: This is the platform EGL 1.5 loader selected for tests.
            if let Ok(egl) =
                unsafe { egl::DynamicInstance::<egl::EGL1_5>::load_required_from_filename(name) }
            {
                return Ok(egl);
            }
        }
        Err(error::invalid_state(
            "failed to load an EGL runtime for the configured test backend",
        ))
    }

    fn egl_error(operation: &str, failure: egl::Error) -> napi::Error {
        error::invalid_state(format!("{operation} failed: {failure:?}"))
    }
}

#[cfg(node_test_backend = "egl")]
use egl_backend::BackendContext;

#[cfg(node_test_backend = "vulkan")]
mod vulkan_backend {
    use std::{
        ffi::{CStr, CString},
        path::Path,
    };

    use ash::{vk, vk::Handle};
    use napi::bindgen_prelude::Result;

    use super::{NativeTestRenderContextDescriptor, address, error};

    pub(super) struct BackendContext {
        entry: ash::Entry,
        instance: ash::Instance,
        physical_device: vk::PhysicalDevice,
        device: ash::Device,
        graphics_queue: vk::Queue,
        graphics_queue_family_index: u32,
    }

    impl BackendContext {
        pub(super) fn create() -> Result<Self> {
            let entry = load_vulkan_entry()?;
            let app_name = CString::new("maplibre-native-node-tests")
                .expect("static application name contains no NUL");
            let engine_name =
                CString::new("maplibre-native-ffi").expect("static engine name contains no NUL");
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
            // SAFETY: All pointers in instance_info reference live local storage.
            let instance =
                unsafe { entry.create_instance(&instance_info, None) }.map_err(|failure| {
                    error::invalid_state(format!("vkCreateInstance failed: {failure}"))
                })?;

            let (physical_device, graphics_queue_family_index) = pick_physical_device(&instance)
                .inspect_err(|_| {
                    // SAFETY: No device child has been created on failure.
                    unsafe { instance.destroy_instance(None) };
                })?;
            let queue_priorities = [1.0_f32];
            let queue_info = [vk::DeviceQueueCreateInfo::default()
                .queue_family_index(graphics_queue_family_index)
                .queue_priorities(&queue_priorities)];
            let mut device_extensions = Vec::new();
            let has_portability_subset = has_device_extension(
                &instance,
                physical_device,
                ash::khr::portability_subset::NAME,
            )
            .inspect_err(|_| {
                // SAFETY: Device creation has not started, so the instance has
                // no owned child requiring earlier destruction.
                unsafe { instance.destroy_instance(None) };
            })?;
            if has_portability_subset {
                device_extensions.push(ash::khr::portability_subset::NAME.as_ptr());
            }
            // SAFETY: physical_device belongs to this instance.
            let supported = unsafe { instance.get_physical_device_features(physical_device) };
            let features = vk::PhysicalDeviceFeatures {
                sampler_anisotropy: supported.sampler_anisotropy,
                wide_lines: supported.wide_lines,
                ..Default::default()
            };
            let device_info = vk::DeviceCreateInfo::default()
                .queue_create_infos(&queue_info)
                .enabled_extension_names(&device_extensions)
                .enabled_features(&features);
            // SAFETY: Queue and extension descriptors reference live storage.
            let device = unsafe { instance.create_device(physical_device, &device_info, None) }
                .map_err(|failure| {
                    // SAFETY: Device creation failed, so only the instance is live.
                    unsafe { instance.destroy_instance(None) };
                    error::invalid_state(format!("vkCreateDevice failed: {failure}"))
                })?;
            // SAFETY: The device was created with queue zero in this family.
            let graphics_queue = unsafe { device.get_device_queue(graphics_queue_family_index, 0) };
            Ok(Self {
                entry,
                instance,
                physical_device,
                device,
                graphics_queue,
                graphics_queue_family_index,
            })
        }

        pub(super) fn descriptor(&self) -> NativeTestRenderContextDescriptor {
            let mut descriptor = NativeTestRenderContextDescriptor::empty("vulkan");
            descriptor.instance_address = Some(address(self.instance.handle().as_raw() as usize));
            descriptor.physical_device_address =
                Some(address(self.physical_device.as_raw() as usize));
            descriptor.device_address = Some(address(self.device.handle().as_raw() as usize));
            descriptor.graphics_queue_address =
                Some(address(self.graphics_queue.as_raw() as usize));
            descriptor.graphics_queue_family_index = Some(self.graphics_queue_family_index);
            descriptor.get_instance_proc_addr_address = Some(address(
                self.entry.static_fn().get_instance_proc_addr as *const () as usize,
            ));
            descriptor.get_device_proc_addr_address = Some(address(
                self.instance.fp_v1_0().get_device_proc_addr as *const () as usize,
            ));
            descriptor
        }
    }

    impl Drop for BackendContext {
        fn drop(&mut self) {
            // SAFETY: The fixture exclusively owns this device and instance.
            unsafe {
                let _ = self.device.device_wait_idle();
                self.device.destroy_device(None);
                self.instance.destroy_instance(None);
            }
        }
    }

    fn load_vulkan_entry() -> Result<ash::Entry> {
        let library_name = if cfg!(target_os = "macos") {
            "libvulkan.1.dylib"
        } else if cfg!(target_os = "windows") {
            "vulkan-1.dll"
        } else {
            "libvulkan.so.1"
        };
        if let Some(install_dir) = std::env::var_os("MAPLIBRE_NATIVE_C_INSTALL_DIR") {
            let runtime_dir = Path::new(&install_dir).join(if cfg!(target_os = "windows") {
                "bin"
            } else {
                "lib"
            });
            let candidate = runtime_dir.join(library_name);
            if candidate.exists() {
                // SAFETY: This is the Vulkan loader installed with the selected
                // native artifact.
                return unsafe { ash::Entry::load_from(&candidate) }.map_err(|failure| {
                    error::invalid_state(format!(
                        "failed to load configured Vulkan runtime {}: {failure}",
                        candidate.display()
                    ))
                });
            }
        }
        // SAFETY: ash selects the platform Vulkan loader by its standard name.
        unsafe { ash::Entry::load() }.map_err(|failure| {
            error::invalid_state(format!("failed to load Vulkan runtime: {failure}"))
        })
    }

    fn has_instance_extension(entry: &ash::Entry, name: &CStr) -> Result<bool> {
        // SAFETY: entry is a live Vulkan loader entry.
        let properties =
            unsafe { entry.enumerate_instance_extension_properties(None) }.map_err(|failure| {
                error::invalid_state(format!(
                    "enumerating Vulkan instance extensions failed: {failure}"
                ))
            })?;
        Ok(properties.iter().any(|property| {
            // SAFETY: Vulkan extension names are NUL-terminated fixed arrays.
            (unsafe { CStr::from_ptr(property.extension_name.as_ptr()) }) == name
        }))
    }

    fn has_device_extension(
        instance: &ash::Instance,
        physical_device: vk::PhysicalDevice,
        name: &CStr,
    ) -> Result<bool> {
        // SAFETY: physical_device belongs to this live instance.
        let properties = unsafe { instance.enumerate_device_extension_properties(physical_device) }
            .map_err(|failure| {
                error::invalid_state(format!(
                    "enumerating Vulkan device extensions failed: {failure}"
                ))
            })?;
        Ok(properties.iter().any(|property| {
            // SAFETY: Vulkan extension names are NUL-terminated fixed arrays.
            (unsafe { CStr::from_ptr(property.extension_name.as_ptr()) }) == name
        }))
    }

    fn pick_physical_device(instance: &ash::Instance) -> Result<(vk::PhysicalDevice, u32)> {
        // SAFETY: instance is live and ash owns enumeration storage.
        let devices = unsafe { instance.enumerate_physical_devices() }.map_err(|failure| {
            error::invalid_state(format!(
                "enumerating Vulkan physical devices failed: {failure}"
            ))
        })?;
        for physical_device in devices {
            // SAFETY: physical_device came from this instance.
            let families =
                unsafe { instance.get_physical_device_queue_family_properties(physical_device) };
            for (index, family) in families.iter().enumerate() {
                if family.queue_count > 0 && family.queue_flags.contains(vk::QueueFlags::GRAPHICS) {
                    return Ok((
                        physical_device,
                        index
                            .try_into()
                            .map_err(|_| error::invalid_state("Vulkan queue index overflow"))?,
                    ));
                }
            }
        }
        Err(error::invalid_state(
            "no Vulkan physical device with a graphics queue was found",
        ))
    }
}

#[cfg(node_test_backend = "vulkan")]
use vulkan_backend::BackendContext;

#[cfg(node_test_backend = "wgl")]
mod wgl_backend {
    use std::{
        ffi::{c_char, c_void},
        mem, ptr,
        sync::atomic::{AtomicUsize, Ordering},
    };

    use napi::bindgen_prelude::Result;

    use super::{NativeTestRenderContextDescriptor, address, error};

    type Hdc = *mut c_void;
    type Hglrc = *mut c_void;
    type Hinstance = *mut c_void;
    type Hwnd = *mut c_void;

    const CS_OWNDC: u32 = 0x0020;
    const PFD_DOUBLEBUFFER: u32 = 0x0000_0001;
    const PFD_DRAW_TO_WINDOW: u32 = 0x0000_0004;
    const PFD_SUPPORT_OPENGL: u32 = 0x0000_0020;
    const PFD_TYPE_RGBA: u8 = 0;
    const PFD_MAIN_PLANE: u8 = 0;
    const WS_OVERLAPPEDWINDOW: u32 = 0x00cf_0000;

    #[repr(C)]
    #[derive(Default)]
    #[allow(non_snake_case)]
    struct WndClassA {
        style: u32,
        lpfnWndProc: Option<unsafe extern "system" fn(Hwnd, u32, usize, isize) -> isize>,
        cbClsExtra: i32,
        cbWndExtra: i32,
        hInstance: Hinstance,
        hIcon: *mut c_void,
        hCursor: *mut c_void,
        hbrBackground: *mut c_void,
        lpszMenuName: *const c_char,
        lpszClassName: *const c_char,
    }

    #[repr(C)]
    #[derive(Default)]
    #[allow(non_snake_case)]
    struct PixelFormatDescriptor {
        nSize: u16,
        nVersion: u16,
        dwFlags: u32,
        iPixelType: u8,
        cColorBits: u8,
        cRedBits: u8,
        cRedShift: u8,
        cGreenBits: u8,
        cGreenShift: u8,
        cBlueBits: u8,
        cBlueShift: u8,
        cAlphaBits: u8,
        cAlphaShift: u8,
        cAccumBits: u8,
        cAccumRedBits: u8,
        cAccumGreenBits: u8,
        cAccumBlueBits: u8,
        cAccumAlphaBits: u8,
        cDepthBits: u8,
        cStencilBits: u8,
        cAuxBuffers: u8,
        iLayerType: u8,
        bReserved: u8,
        dwLayerMask: u32,
        dwVisibleMask: u32,
        dwDamageMask: u32,
    }

    static CLASS_ID: AtomicUsize = AtomicUsize::new(1);

    pub(super) struct BackendContext {
        window: Hwnd,
        device_context: Hdc,
        share_context: Hglrc,
    }

    impl BackendContext {
        pub(super) fn create() -> Result<Self> {
            let class_name = format!(
                "MaplibreNativeNodeWglTest{}\0",
                CLASS_ID.fetch_add(1, Ordering::Relaxed)
            );
            // SAFETY: Null requests the current executable module.
            let module = unsafe { GetModuleHandleA(ptr::null()) };
            if module.is_null() {
                return Err(error::invalid_state("GetModuleHandleA returned null"));
            }
            let window_class = WndClassA {
                style: CS_OWNDC,
                lpfnWndProc: Some(window_proc),
                hInstance: module,
                lpszClassName: class_name.as_ptr().cast(),
                ..Default::default()
            };
            // SAFETY: window_class points to stable storage for the call.
            unsafe { RegisterClassA(&window_class) };
            // SAFETY: The class name is NUL-terminated and remains live for the
            // call; no parent/menu/creation payload is required.
            let window = unsafe {
                CreateWindowExA(
                    0,
                    class_name.as_ptr().cast(),
                    class_name.as_ptr().cast(),
                    WS_OVERLAPPEDWINDOW,
                    0,
                    0,
                    8,
                    8,
                    ptr::null_mut(),
                    ptr::null_mut(),
                    module,
                    ptr::null_mut(),
                )
            };
            if window.is_null() {
                return Err(error::invalid_state("CreateWindowExA returned null"));
            }
            // SAFETY: window is a live HWND.
            let device_context = unsafe { GetDC(window) };
            if device_context.is_null() {
                // SAFETY: window is owned by this failing constructor.
                unsafe { DestroyWindow(window) };
                return Err(error::invalid_state("GetDC returned null"));
            }
            let pixel_format = PixelFormatDescriptor {
                nSize: mem::size_of::<PixelFormatDescriptor>() as u16,
                nVersion: 1,
                dwFlags: PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
                iPixelType: PFD_TYPE_RGBA,
                cColorBits: 32,
                cDepthBits: 24,
                cStencilBits: 8,
                iLayerType: PFD_MAIN_PLANE,
                ..Default::default()
            };
            // SAFETY: device_context is live and pixel_format is initialized.
            let format = unsafe { ChoosePixelFormat(device_context, &pixel_format) };
            // SAFETY: Same live context and descriptor as above.
            if format == 0 || unsafe { SetPixelFormat(device_context, format, &pixel_format) } == 0
            {
                // SAFETY: Both handles are live and owned here.
                unsafe {
                    ReleaseDC(window, device_context);
                    DestroyWindow(window);
                }
                return Err(error::invalid_state(
                    "configuring the WGL pixel format failed",
                ));
            }
            // SAFETY: The device context has a selected OpenGL pixel format.
            let share_context = unsafe { wglCreateContext(device_context) };
            // SAFETY: Both handles are live when non-null.
            if share_context.is_null()
                || unsafe { wglMakeCurrent(device_context, share_context) } == 0
            {
                // SAFETY: Release only objects successfully created above.
                unsafe {
                    if !share_context.is_null() {
                        wglDeleteContext(share_context);
                    }
                    ReleaseDC(window, device_context);
                    DestroyWindow(window);
                }
                return Err(error::invalid_state("creating the WGL context failed"));
            }
            Ok(Self {
                window,
                device_context,
                share_context,
            })
        }

        pub(super) fn descriptor(&self) -> NativeTestRenderContextDescriptor {
            let mut descriptor = NativeTestRenderContextDescriptor::empty("wgl");
            descriptor.device_context_address = Some(address(self.device_context as usize));
            descriptor.share_context_address = Some(address(self.share_context as usize));
            descriptor.get_proc_address_address =
                Some(address(wglGetProcAddress as *const () as usize));
            descriptor
        }
    }

    impl Drop for BackendContext {
        fn drop(&mut self) {
            // SAFETY: These WGL objects are exclusively owned by the fixture.
            unsafe {
                wglMakeCurrent(ptr::null_mut(), ptr::null_mut());
                wglDeleteContext(self.share_context);
                ReleaseDC(self.window, self.device_context);
                DestroyWindow(self.window);
            }
        }
    }

    unsafe extern "system" fn window_proc(
        window: Hwnd,
        message: u32,
        wparam: usize,
        lparam: isize,
    ) -> isize {
        // SAFETY: Forwarding the exact window procedure arguments is required by
        // Win32 for messages this hidden helper does not handle.
        unsafe { DefWindowProcA(window, message, wparam, lparam) }
    }

    #[link(name = "kernel32")]
    unsafe extern "system" {
        fn GetModuleHandleA(module_name: *const c_char) -> Hinstance;
    }

    #[link(name = "user32")]
    unsafe extern "system" {
        fn RegisterClassA(window_class: *const WndClassA) -> u16;
        fn CreateWindowExA(
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
        fn DefWindowProcA(window: Hwnd, message: u32, wparam: usize, lparam: isize) -> isize;
        fn DestroyWindow(window: Hwnd) -> i32;
        fn GetDC(window: Hwnd) -> Hdc;
        fn ReleaseDC(window: Hwnd, device_context: Hdc) -> i32;
    }

    #[link(name = "gdi32")]
    unsafe extern "system" {
        fn ChoosePixelFormat(device_context: Hdc, descriptor: *const PixelFormatDescriptor) -> i32;
        fn SetPixelFormat(
            device_context: Hdc,
            format: i32,
            descriptor: *const PixelFormatDescriptor,
        ) -> i32;
    }

    #[link(name = "opengl32")]
    unsafe extern "system" {
        fn wglCreateContext(device_context: Hdc) -> Hglrc;
        fn wglDeleteContext(context: Hglrc) -> i32;
        fn wglGetProcAddress(name: *const c_char) -> *mut c_void;
        fn wglMakeCurrent(device_context: Hdc, context: Hglrc) -> i32;
    }
}

#[cfg(node_test_backend = "wgl")]
use wgl_backend::BackendContext;
