#![deny(unsafe_op_in_unsafe_fn)]

use std::error::Error;
use std::ffi::CString;
use std::time::{Duration, Instant};

use ash::vk;
use ash::vk::Handle;
use maplibre_native::{
    MapHandle, MapMode, MapOptions, NativePointer, RenderBackendMask, RenderSessionHandle,
    RuntimeEventPayload, RuntimeEventType, RuntimeHandle, VulkanSurfaceDescriptor,
};
use raw_window_handle::{HasDisplayHandle, HasWindowHandle};
use winit::dpi::PhysicalSize;
use winit::event::{Event, WindowEvent};
use winit::event_loop::{ControlFlow, EventLoop};
use winit::window::{Window, WindowBuilder};

const INITIAL_WIDTH: u32 = 1280;
const INITIAL_HEIGHT: u32 = 720;
const STYLE_JSON: &str = r##"{
  "version": 8,
  "sources": {},
  "layers": [
    {
      "id": "background",
      "type": "background",
      "paint": { "background-color": "#d8ecff" }
    }
  ]
}"##;

fn main() -> Result<(), Box<dyn Error>> {
    if !maplibre_native::supported_render_backends().contains(RenderBackendMask::VULKAN) {
        return Err("the loaded MapLibre native library does not support Vulkan".into());
    }

    let event_loop = EventLoop::new()?;
    let window = WindowBuilder::new()
        .with_title("MapLibre Rust Vulkan Map")
        .with_inner_size(PhysicalSize::new(INITIAL_WIDTH, INITIAL_HEIGHT))
        .with_resizable(true)
        .build(&event_loop)?;

    let vulkan = VulkanContext::new(&window)?;
    let viewport = Viewport::from_window(&window);
    if viewport.is_empty() {
        return Err("window has no drawable extent".into());
    }
    let runtime = RuntimeHandle::new()?;
    let map = runtime.create_map_with_options(
        &MapOptions::new(viewport.width, viewport.height, viewport.scale_factor)
            .with_mode(MapMode::Continuous),
    )?;
    map.set_style_json(STYLE_JSON)?;
    let session = attach_surface(&map, &vulkan, viewport)?;
    map.request_repaint()?;

    let mut app = App {
        session,
        map,
        runtime,
        _vulkan: vulkan,
        window,
        viewport,
        render_pending: true,
    };

    println!("MapLibre Rust Vulkan map example running. Close the window to exit.");
    event_loop.run(move |event, target| {
        target.set_control_flow(ControlFlow::WaitUntil(
            Instant::now() + Duration::from_millis(16),
        ));

        match event {
            Event::WindowEvent { event, .. } => match event {
                WindowEvent::CloseRequested => target.exit(),
                WindowEvent::Resized(_) | WindowEvent::ScaleFactorChanged { .. } => {
                    if let Err(error) = app.resize() {
                        eprintln!("resize failed: {error}");
                        target.exit();
                    }
                }
                WindowEvent::RedrawRequested => {
                    if let Err(error) = app.render() {
                        eprintln!("render failed: {error}");
                        target.exit();
                    }
                }
                _ => {}
            },
            Event::AboutToWait => {
                if let Err(error) = app.pump_runtime() {
                    eprintln!("runtime update failed: {error}");
                    target.exit();
                }
                if app.render_pending {
                    app.window.request_redraw();
                }
            }
            _ => {}
        }
    })?;

    Ok(())
}

struct App {
    session: RenderSessionHandle,
    map: MapHandle,
    runtime: RuntimeHandle,
    _vulkan: VulkanContext,
    window: Window,
    viewport: Viewport,
    render_pending: bool,
}

impl App {
    fn resize(&mut self) -> Result<(), Box<dyn Error>> {
        let next = Viewport::from_window(&self.window);
        if next == self.viewport {
            return Ok(());
        }
        self.viewport = next;
        if next.is_empty() {
            self.render_pending = false;
            return Ok(());
        }
        self.session
            .resize(next.width, next.height, next.scale_factor)?;
        self.map.request_repaint()?;
        self.render_pending = true;
        self.window.request_redraw();
        Ok(())
    }

    fn pump_runtime(&mut self) -> Result<(), Box<dyn Error>> {
        self.runtime.run_once()?;
        while let Some(event) = self.runtime.poll_event()? {
            match event.event_type {
                RuntimeEventType::MapRenderUpdateAvailable => self.render_pending = true,
                RuntimeEventType::MapRenderFrameFinished => {
                    if let RuntimeEventPayload::RenderFrame(frame) = event.payload {
                        self.render_pending |= frame.needs_repaint;
                    }
                }
                _ => {}
            }
        }
        Ok(())
    }

    fn render(&mut self) -> Result<(), Box<dyn Error>> {
        if self.viewport.is_empty() {
            return Ok(());
        }
        self.pump_runtime()?;
        if self.render_pending {
            self.session.render_update()?;
            self.render_pending = false;
        }
        Ok(())
    }
}

#[derive(Clone, Copy, Debug, PartialEq)]
struct Viewport {
    width: u32,
    height: u32,
    scale_factor: f64,
}

impl Viewport {
    fn from_window(window: &Window) -> Self {
        let size = window.inner_size();
        Self {
            width: size.width,
            height: size.height,
            scale_factor: window.scale_factor(),
        }
    }

    fn is_empty(self) -> bool {
        self.width == 0 || self.height == 0
    }
}

struct VulkanContext {
    _entry: ash::Entry,
    instance: ash::Instance,
    surface_loader: ash::khr::surface::Instance,
    surface: vk::SurfaceKHR,
    physical_device: vk::PhysicalDevice,
    device: ash::Device,
    graphics_queue: vk::Queue,
    graphics_queue_family_index: u32,
}

impl VulkanContext {
    fn new(window: &Window) -> Result<Self, Box<dyn Error>> {
        // SAFETY: Loading the Vulkan loader is delegated to ash. The returned entry
        // owns function pointers for the process Vulkan loader.
        let entry = unsafe { ash::Entry::load()? };
        let app_name = CString::new("MapLibre Rust Vulkan Map")?;
        let engine_name = CString::new("maplibre-native-ffi")?;
        let app_info = vk::ApplicationInfo::default()
            .application_name(&app_name)
            .application_version(0)
            .engine_name(&engine_name)
            .engine_version(0)
            .api_version(vk::API_VERSION_1_0);

        let display_handle = window.display_handle()?.as_raw();
        let window_handle = window.window_handle()?.as_raw();
        let extension_names = ash_window::enumerate_required_extensions(display_handle)?;
        let instance_info = vk::InstanceCreateInfo::default()
            .application_info(&app_info)
            .enabled_extension_names(extension_names);
        // SAFETY: instance_info points to stable extension-name and app-info storage
        // for this call. Allocation callbacks are not used.
        let instance = unsafe { entry.create_instance(&instance_info, None)? };

        // SAFETY: The raw display/window handles come from a live winit window,
        // and the created Vulkan surface is destroyed before the instance.
        let surface = match unsafe {
            ash_window::create_surface(&entry, &instance, display_handle, window_handle, None)
        } {
            Ok(surface) => surface,
            Err(error) => {
                // SAFETY: instance was created above and no child objects exist yet.
                unsafe { instance.destroy_instance(None) };
                return Err(error.into());
            }
        };
        let surface_loader = ash::khr::surface::Instance::new(&entry, &instance);

        let (physical_device, graphics_queue_family_index) =
            match pick_physical_device(&instance, &surface_loader, surface) {
                Ok(device) => device,
                Err(error) => {
                    // SAFETY: surface and instance were created above and remain live.
                    unsafe {
                        surface_loader.destroy_surface(surface, None);
                        instance.destroy_instance(None);
                    }
                    return Err(error);
                }
            };
        let priorities = [1.0_f32];
        let queue_info = [vk::DeviceQueueCreateInfo::default()
            .queue_family_index(graphics_queue_family_index)
            .queue_priorities(&priorities)];
        let device_extensions = [ash::khr::swapchain::NAME.as_ptr()];
        let device_info = vk::DeviceCreateInfo::default()
            .queue_create_infos(&queue_info)
            .enabled_extension_names(&device_extensions);
        // SAFETY: physical_device and queue family were selected from this instance;
        // queue and extension slices live for the duration of the call.
        let device = match unsafe { instance.create_device(physical_device, &device_info, None) } {
            Ok(device) => device,
            Err(error) => {
                // SAFETY: surface and instance were created above and remain live.
                unsafe {
                    surface_loader.destroy_surface(surface, None);
                    instance.destroy_instance(None);
                }
                return Err(error.into());
            }
        };
        // SAFETY: Queue index 0 exists because the device was created with one
        // queue from graphics_queue_family_index.
        let graphics_queue = unsafe { device.get_device_queue(graphics_queue_family_index, 0) };

        Ok(Self {
            _entry: entry,
            instance,
            surface_loader,
            surface,
            physical_device,
            device,
            graphics_queue,
            graphics_queue_family_index,
        })
    }
}

impl Drop for VulkanContext {
    fn drop(&mut self) {
        // SAFETY: Objects are destroyed in reverse dependency order after the
        // event loop has dropped the MapLibre render session that borrowed them.
        unsafe {
            let _ = self.device.device_wait_idle();
            self.device.destroy_device(None);
            self.surface_loader.destroy_surface(self.surface, None);
            self.instance.destroy_instance(None);
        }
    }
}

fn pick_physical_device(
    instance: &ash::Instance,
    surface_loader: &ash::khr::surface::Instance,
    surface: vk::SurfaceKHR,
) -> Result<(vk::PhysicalDevice, u32), Box<dyn Error>> {
    // SAFETY: instance is live and enumeration writes into ash-owned vectors.
    let devices = unsafe { instance.enumerate_physical_devices()? };
    for physical_device in devices {
        // SAFETY: physical_device came from this live instance.
        let families =
            unsafe { instance.get_physical_device_queue_family_properties(physical_device) };
        for (index, family) in families.iter().enumerate() {
            let supports_graphics = family.queue_flags.contains(vk::QueueFlags::GRAPHICS);
            // SAFETY: surface, physical_device, and queue family index are valid
            // for this live instance and surface loader.
            let supports_present = unsafe {
                surface_loader.get_physical_device_surface_support(
                    physical_device,
                    index as u32,
                    surface,
                )?
            };
            if supports_graphics && supports_present {
                return Ok((physical_device, index as u32));
            }
        }
    }
    Err("no Vulkan physical device has a graphics queue that can present to the window".into())
}

fn attach_surface(
    map: &MapHandle,
    vulkan: &VulkanContext,
    viewport: Viewport,
) -> maplibre_native::Result<RenderSessionHandle> {
    map.attach_vulkan_surface(&VulkanSurfaceDescriptor::new(
        viewport.width,
        viewport.height,
        viewport.scale_factor,
        // SAFETY: These Vulkan handles are live for the render session lifetime.
        unsafe { NativePointer::from_address(vulkan.instance.handle().as_raw() as usize) },
        unsafe { NativePointer::from_address(vulkan.physical_device.as_raw() as usize) },
        unsafe { NativePointer::from_address(vulkan.device.handle().as_raw() as usize) },
        unsafe { NativePointer::from_address(vulkan.graphics_queue.as_raw() as usize) },
        vulkan.graphics_queue_family_index,
        unsafe { NativePointer::from_address(vulkan.surface.as_raw() as usize) },
    ))
}
