use std::error::Error;
use std::ffi::{CStr, CString};

use ash::vk;
use ash::vk::Handle;
use maplibre_native::NativePointer;
use raw_window_handle::{HasDisplayHandle, HasWindowHandle};
use winit::window::Window;

pub struct VulkanContext {
    entry: ash::Entry,
    instance: ash::Instance,
    surface_loader: ash::khr::surface::Instance,
    surface: vk::SurfaceKHR,
    physical_device: vk::PhysicalDevice,
    device: ash::Device,
    graphics_queue: vk::Queue,
    graphics_queue_family_index: u32,
}

pub struct BorrowedImage {
    device: ash::Device,
    image: vk::Image,
    memory: vk::DeviceMemory,
    view: vk::ImageView,
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
            entry,
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

    pub fn get_instance_proc_addr_pointer(&self) -> NativePointer {
        // SAFETY: The function pointer remains valid while the ash entry is live.
        unsafe {
            NativePointer::from_address(
                self.entry.static_fn().get_instance_proc_addr as *const () as usize,
            )
        }
    }

    pub fn get_device_proc_addr_pointer(&self) -> NativePointer {
        // SAFETY: The function pointer remains valid while the ash instance is live.
        unsafe {
            NativePointer::from_address(
                self.instance.fp_v1_0().get_device_proc_addr as *const () as usize,
            )
        }
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

impl BorrowedImage {
    pub fn new(
        context: &VulkanContext,
        viewport: crate::viewport::Viewport,
    ) -> Result<Self, vk::Result> {
        let device = context.device().clone();
        let image_info = vk::ImageCreateInfo::default()
            .image_type(vk::ImageType::TYPE_2D)
            .format(vk::Format::R8G8B8A8_UNORM)
            .extent(vk::Extent3D {
                width: viewport.physical_width,
                height: viewport.physical_height,
                depth: 1,
            })
            .mip_levels(1)
            .array_layers(1)
            .samples(vk::SampleCountFlags::TYPE_1)
            .tiling(vk::ImageTiling::OPTIMAL)
            .usage(vk::ImageUsageFlags::COLOR_ATTACHMENT | vk::ImageUsageFlags::SAMPLED)
            .sharing_mode(vk::SharingMode::EXCLUSIVE)
            .initial_layout(vk::ImageLayout::UNDEFINED);
        // SAFETY: image_info is fully initialized and the device is live.
        let image = unsafe { device.create_image(&image_info, None)? };
        let memory_requirements = unsafe { device.get_image_memory_requirements(image) };
        let memory_type_index = match find_memory_type(
            context.instance(),
            context.physical_device(),
            memory_requirements.memory_type_bits,
            vk::MemoryPropertyFlags::DEVICE_LOCAL,
        ) {
            Ok(index) => index,
            Err(error) => {
                // SAFETY: image was created above and has not been bound to memory.
                unsafe { device.destroy_image(image, None) };
                return Err(error);
            }
        };
        let allocate_info = vk::MemoryAllocateInfo::default()
            .allocation_size(memory_requirements.size)
            .memory_type_index(memory_type_index);
        // SAFETY: allocation size/type came from the image memory requirements.
        let memory = match unsafe { device.allocate_memory(&allocate_info, None) } {
            Ok(memory) => memory,
            Err(error) => {
                // SAFETY: image was created above and no child objects exist.
                unsafe { device.destroy_image(image, None) };
                return Err(error);
            }
        };
        if let Err(error) = unsafe { device.bind_image_memory(image, memory, 0) } {
            // SAFETY: image and memory were created above and no image view exists yet.
            unsafe {
                device.free_memory(memory, None);
                device.destroy_image(image, None);
            }
            return Err(error);
        }

        let view_info = vk::ImageViewCreateInfo::default()
            .image(image)
            .view_type(vk::ImageViewType::TYPE_2D)
            .format(vk::Format::R8G8B8A8_UNORM)
            .subresource_range(
                vk::ImageSubresourceRange::default()
                    .aspect_mask(vk::ImageAspectFlags::COLOR)
                    .level_count(1)
                    .layer_count(1),
            );
        // SAFETY: image is live and view_info describes its color subresource.
        let view = match unsafe { device.create_image_view(&view_info, None) } {
            Ok(view) => view,
            Err(error) => {
                // SAFETY: image and memory were created above and no view exists.
                unsafe {
                    device.free_memory(memory, None);
                    device.destroy_image(image, None);
                }
                return Err(error);
            }
        };

        Ok(Self {
            device,
            image,
            memory,
            view,
        })
    }

    pub fn view(&self) -> vk::ImageView {
        self.view
    }

    pub fn image_pointer(&self) -> NativePointer {
        // SAFETY: The Vulkan image is live while the borrowed texture session is live.
        unsafe { NativePointer::from_address(self.image.as_raw() as usize) }
    }

    pub fn view_pointer(&self) -> NativePointer {
        // SAFETY: The Vulkan image view is live while the borrowed texture session is live.
        unsafe { NativePointer::from_address(self.view.as_raw() as usize) }
    }
}

impl Drop for BorrowedImage {
    fn drop(&mut self) {
        // SAFETY: The image view, image, and memory were created from this live
        // device and are destroyed in reverse dependency order. BorrowedImage
        // is only constructable after all three handles are non-null, so Drop
        // runs unconditionally.
        unsafe {
            self.device.destroy_image_view(self.view, None);
            self.device.destroy_image(self.image, None);
            self.device.free_memory(self.memory, None);
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

fn find_memory_type(
    instance: &ash::Instance,
    physical_device: vk::PhysicalDevice,
    type_bits: u32,
    properties: vk::MemoryPropertyFlags,
) -> Result<u32, vk::Result> {
    // SAFETY: physical_device came from this live instance.
    let memory_properties =
        unsafe { instance.get_physical_device_memory_properties(physical_device) };
    for index in 0..memory_properties.memory_type_count {
        let type_supported = (type_bits & (1 << index)) != 0;
        let has_properties = memory_properties.memory_types[index as usize]
            .property_flags
            .contains(properties);
        if type_supported && has_properties {
            return Ok(index);
        }
    }
    Err(vk::Result::ERROR_FEATURE_NOT_PRESENT)
}
