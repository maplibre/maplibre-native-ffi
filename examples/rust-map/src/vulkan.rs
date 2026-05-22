use std::error::Error;
use std::ffi::{CStr, CString};

use ash::vk;
use ash::vk::Handle;
use maplibre_native::NativePointer;
use raw_window_handle::{HasDisplayHandle, HasWindowHandle};
use winit::window::Window;

pub struct VulkanContext {
    instance: ash::Instance,
    surface_loader: ash::khr::surface::Instance,
    surface: vk::SurfaceKHR,
    physical_device: vk::PhysicalDevice,
    device: ash::Device,
    graphics_queue: vk::Queue,
    graphics_queue_family_index: u32,
}

impl VulkanContext {
    pub fn new(window: &Window) -> Result<Self, Box<dyn Error>> {
        let entry = load_vulkan_entry()?;
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
        let mut extension_names =
            ash_window::enumerate_required_extensions(display_handle)?.to_vec();
        let mut instance_flags = vk::InstanceCreateFlags::empty();
        if has_instance_extension(&entry, ash::khr::portability_enumeration::NAME)? {
            extension_names.push(ash::khr::portability_enumeration::NAME.as_ptr());
            instance_flags |= vk::InstanceCreateFlags::ENUMERATE_PORTABILITY_KHR;
        }
        let instance_info = vk::InstanceCreateInfo::default()
            .application_info(&app_info)
            .enabled_extension_names(&extension_names)
            .flags(instance_flags);
        // SAFETY: instance_info points to stable extension-name and app-info storage.
        let instance = unsafe { entry.create_instance(&instance_info, None)? };

        // SAFETY: raw display/window handles come from a live winit window.
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
        let mut device_extensions = vec![ash::khr::swapchain::NAME.as_ptr()];
        if has_device_extension(
            &instance,
            physical_device,
            ash::khr::portability_subset::NAME,
        )? {
            device_extensions.push(ash::khr::portability_subset::NAME.as_ptr());
        }
        let device_info = vk::DeviceCreateInfo::default()
            .queue_create_infos(&queue_info)
            .enabled_extension_names(&device_extensions);
        // SAFETY: physical_device and queue family were selected from this instance.
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
        // SAFETY: Queue index 0 exists because the device was created with one queue.
        let graphics_queue = unsafe { device.get_device_queue(graphics_queue_family_index, 0) };

        Ok(Self {
            instance,
            surface_loader,
            surface,
            physical_device,
            device,
            graphics_queue,
            graphics_queue_family_index,
        })
    }

    pub fn wait_idle(&self) -> Result<(), vk::Result> {
        // SAFETY: device is live while VulkanContext is live.
        unsafe { self.device.device_wait_idle() }
    }

    pub fn instance(&self) -> &ash::Instance {
        &self.instance
    }

    pub fn surface_loader(&self) -> &ash::khr::surface::Instance {
        &self.surface_loader
    }

    pub fn surface(&self) -> vk::SurfaceKHR {
        self.surface
    }

    pub fn physical_device(&self) -> vk::PhysicalDevice {
        self.physical_device
    }

    pub fn device(&self) -> &ash::Device {
        &self.device
    }

    pub fn graphics_queue(&self) -> vk::Queue {
        self.graphics_queue
    }

    pub fn instance_pointer(&self) -> NativePointer {
        // SAFETY: The Vulkan instance is live for the render session lifetime.
        unsafe { NativePointer::from_address(self.instance.handle().as_raw() as usize) }
    }

    pub fn physical_device_pointer(&self) -> NativePointer {
        // SAFETY: The physical device is live for the render session lifetime.
        unsafe { NativePointer::from_address(self.physical_device.as_raw() as usize) }
    }

    pub fn device_pointer(&self) -> NativePointer {
        // SAFETY: The Vulkan device is live for the render session lifetime.
        unsafe { NativePointer::from_address(self.device.handle().as_raw() as usize) }
    }

    pub fn graphics_queue_pointer(&self) -> NativePointer {
        // SAFETY: The Vulkan queue is live for the render session lifetime.
        unsafe { NativePointer::from_address(self.graphics_queue.as_raw() as usize) }
    }

    pub fn surface_pointer(&self) -> NativePointer {
        // SAFETY: The Vulkan surface is live for the render session lifetime.
        unsafe { NativePointer::from_address(self.surface.as_raw() as usize) }
    }

    pub fn graphics_queue_family_index(&self) -> u32 {
        self.graphics_queue_family_index
    }
}

impl Drop for VulkanContext {
    fn drop(&mut self) {
        // SAFETY: Objects are destroyed in reverse dependency order after the
        // render target that borrowed them has closed or after process exit.
        unsafe {
            let _ = self.wait_idle();
            self.device.destroy_device(None);
            self.surface_loader.destroy_surface(self.surface, None);
            self.instance.destroy_instance(None);
        }
    }
}

fn load_vulkan_entry() -> Result<ash::Entry, Box<dyn Error>> {
    // SAFETY: Loading the Vulkan loader is delegated to ash. Repository tasks
    // run through Pixi and expose the native library directory to this process.
    unsafe { ash::Entry::load() }.map_err(Into::into)
}

fn has_instance_extension(entry: &ash::Entry, name: &CStr) -> Result<bool, Box<dyn Error>> {
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
) -> Result<bool, Box<dyn Error>> {
    // SAFETY: physical_device came from this live instance.
    let properties = unsafe { instance.enumerate_device_extension_properties(physical_device)? };
    Ok(properties.iter().any(|property| {
        // SAFETY: Vulkan extension names are fixed-size NUL-terminated arrays.
        let property_name = unsafe { CStr::from_ptr(property.extension_name.as_ptr()) };
        property_name == name
    }))
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
            // SAFETY: surface, physical_device, and queue family index are valid.
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
