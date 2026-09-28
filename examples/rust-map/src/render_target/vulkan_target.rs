use std::error::Error as StdError;

use maplibre_native_ffi::{
    GpuSync, MapHandle, RenderSessionAttachOptions, VulkanBorrowedTextureDescriptor,
    VulkanContextDescriptor, VulkanOwnedTextureDescriptor, VulkanSurfaceDescriptor,
};

use crate::graphics::GraphicsContext;
use crate::map_state::MapState;
use crate::render_target::{
    FrameDriver, FrameOutcome, Mode, compositor_error, extent, require_cpu_complete_producer,
};
use crate::viewport::Viewport;
use crate::vulkan::{BorrowedImage, VulkanContext};
use crate::vulkan_texture_compositor::VulkanTextureCompositor;

pub enum RenderTarget {
    OwnedTexture {
        driver: FrameDriver,
        compositor: Box<VulkanTextureCompositor>,
    },
    BorrowedTexture {
        driver: FrameDriver,
        compositor: Box<VulkanTextureCompositor>,
        image: Box<BorrowedImage>,
    },
    Surface {
        driver: FrameDriver,
    },
}

impl RenderTarget {
    pub fn attach(
        mode: Mode,
        map: &MapHandle,
        graphics: &GraphicsContext,
        viewport: Viewport,
    ) -> maplibre_native_ffi::Result<Self> {
        let vk = graphics.vulkan();
        // The window thread submits and presents on the same VkQueue this
        // descriptor hands over, so the session shares that thread rather than
        // driving the queue from a core worker.
        let options = RenderSessionAttachOptions {
            driver: maplibre_native_ffi::RenderDriverKind::CallerGraphicsThread,
            requested_texture_ring_depth: if mode == Mode::OwnedTexture { 2 } else { 0 },
            ..Default::default()
        };
        match mode {
            Mode::OwnedTexture => {
                let descriptor = VulkanOwnedTextureDescriptor {
                    extent: extent(viewport),
                    context: context_descriptor(vk),
                };
                let driver = FrameDriver::new(unsafe {
                    map.vulkan_owned_texture_attach(&descriptor, &(options))
                }?)?;
                let compositor = VulkanTextureCompositor::new(vk, viewport).map_err(|error| {
                    compositor_error(format!("Vulkan compositor creation failed: {error:?}"))
                })?;
                Ok(Self::OwnedTexture {
                    driver,
                    compositor: Box::new(compositor),
                })
            }
            Mode::BorrowedTexture => {
                let image = BorrowedImage::new(vk, viewport).map_err(|error| {
                    compositor_error(format!("Vulkan image creation failed: {error:?}"))
                })?;
                let descriptor = borrowed_descriptor(vk, viewport, &image);
                let driver = FrameDriver::new(unsafe {
                    map.vulkan_borrowed_texture_attach(&descriptor, &(options))
                }?)?;
                let compositor = VulkanTextureCompositor::new(vk, viewport).map_err(|error| {
                    compositor_error(format!("Vulkan compositor creation failed: {error:?}"))
                })?;
                Ok(Self::BorrowedTexture {
                    driver,
                    compositor: Box::new(compositor),
                    image: Box::new(image),
                })
            }
            Mode::NativeSurface => {
                let descriptor = VulkanSurfaceDescriptor {
                    extent: extent(viewport),
                    context: context_descriptor(vk),
                    surface: vk.surface_handle(),
                };
                Ok(Self::Surface {
                    driver: FrameDriver::new(unsafe {
                        map.vulkan_surface_attach(&descriptor, &(options))
                    }?)?,
                })
            }
        }
    }

    pub fn resize(
        &mut self,
        graphics: &GraphicsContext,
        map: &MapState,
        viewport: Viewport,
    ) -> Result<(), Box<dyn StdError>> {
        match self {
            Self::OwnedTexture { driver, compositor } => {
                compositor.resize(viewport).map_err(|error| {
                    compositor_error(format!("Vulkan resize failed: {error:?}"))
                })?;
                driver.resize(viewport)?;
                Ok(())
            }
            Self::BorrowedTexture {
                driver,
                compositor,
                image,
            } => {
                let replacement =
                    BorrowedImage::new(graphics.vulkan(), viewport).map_err(|error| {
                        compositor_error(format!("Vulkan image creation failed: {error:?}"))
                    })?;
                let descriptor = borrowed_descriptor(graphics.vulkan(), viewport, &replacement);
                let operation = unsafe {
                    driver
                        .session()
                        .vulkan_borrowed_texture_set_target(&descriptor)
                }?;
                driver.drive(&operation)?;
                **image = replacement;
                compositor.resize(viewport).map_err(|error| {
                    compositor_error(format!("Vulkan resize failed: {error:?}"))
                })?;
                // Target replacement changes only the graphics resource, so
                // the map takes the new extent directly.
                map.resize(viewport)
            }
            Self::Surface { driver } => {
                driver.resize(viewport)?;
                Ok(())
            }
        }
    }

    pub fn render_update(
        &mut self,
        _graphics: &GraphicsContext,
    ) -> maplibre_native_ffi::Result<FrameOutcome> {
        let present = matches!(self, Self::Surface { .. });
        let driver = match self {
            Self::OwnedTexture { driver, .. }
            | Self::BorrowedTexture { driver, .. }
            | Self::Surface { driver } => driver,
        };
        let mut outcome = driver.render_frame(present)?;
        if !outcome.rendered {
            return Ok(outcome);
        }
        match self {
            Self::OwnedTexture { driver, compositor } => {
                let Some(frame) = driver.acquire_frame()? else {
                    outcome.rendered = false;
                    return Ok(outcome);
                };
                require_cpu_complete_producer(&frame)?;
                outcome.rendered = compositor.draw(&frame)?;
                compositor.wait_idle().map_err(|error| {
                    compositor_error(format!("Vulkan consumer wait failed: {error:?}"))
                })?;
                frame.release(&GpuSync::default())?;
            }
            Self::BorrowedTexture {
                compositor, image, ..
            } => {
                outcome.rendered = compositor
                    .draw_image_view(image.view())
                    .map_err(|error| compositor_error(format!("Vulkan draw failed: {error:?}")))?;
            }
            Self::Surface { .. } => {}
        }
        Ok(outcome)
    }

    pub fn close(self, _graphics: &GraphicsContext) -> Result<(), Box<dyn StdError>> {
        match self {
            Self::OwnedTexture {
                driver,
                mut compositor,
            } => {
                driver.close()?;
                compositor.close()?;
                Ok(())
            }
            Self::BorrowedTexture {
                driver,
                mut compositor,
                image,
            } => {
                driver.close()?;
                compositor.close()?;
                drop(image);
                Ok(())
            }
            Self::Surface { driver } => driver.close(),
        }
    }
}

fn borrowed_descriptor(
    vk: &VulkanContext,
    viewport: Viewport,
    image: &BorrowedImage,
) -> VulkanBorrowedTextureDescriptor {
    VulkanBorrowedTextureDescriptor {
        extent: extent(viewport),
        physical_width: viewport.physical_width,
        physical_height: viewport.physical_height,
        context: context_descriptor(vk),
        image: image.image_handle(),
        image_view: image.view_handle(),
        format: ash::vk::Format::R8G8B8A8_UNORM.as_raw() as u32,
        initial_layout: ash::vk::ImageLayout::UNDEFINED.as_raw() as u32,
        final_layout: ash::vk::ImageLayout::SHADER_READ_ONLY_OPTIMAL.as_raw() as u32,
    }
}

fn context_descriptor(vk: &VulkanContext) -> VulkanContextDescriptor {
    let mut descriptor = VulkanContextDescriptor {
        instance: vk.instance_pointer(),
        physical_device: vk.physical_device_pointer(),
        device: vk.device_pointer(),
        graphics_queue: vk.graphics_queue_pointer(),
        graphics_queue_family_index: vk.graphics_queue_family_index(),
        ..Default::default()
    };
    descriptor.get_instance_proc_addr = vk.get_instance_proc_addr_pointer();
    descriptor.get_device_proc_addr = vk.get_device_proc_addr_pointer();
    descriptor
}
