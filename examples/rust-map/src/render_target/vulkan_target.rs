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
        let options =
            RenderSessionAttachOptions::caller_graphics_thread(if mode == Mode::OwnedTexture {
                2
            } else {
                0
            });
        match mode {
            Mode::OwnedTexture => {
                let descriptor =
                    VulkanOwnedTextureDescriptor::new(extent(viewport), context_descriptor(vk));
                let driver =
                    FrameDriver::new(map.attach_vulkan_owned_texture(&descriptor, options)?)?;
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
                let driver =
                    FrameDriver::new(map.attach_vulkan_borrowed_texture(&descriptor, options)?)?;
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
                let descriptor = VulkanSurfaceDescriptor::new(
                    extent(viewport),
                    context_descriptor(vk),
                    vk.surface_handle(),
                );
                Ok(Self::Surface {
                    driver: FrameDriver::new(map.attach_vulkan_surface(&descriptor, options)?)?,
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
                let operation = driver
                    .session()
                    .set_vulkan_borrowed_texture_target(&descriptor)?;
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
                frame
                    .release(GpuSync::CpuComplete)
                    .map_err(|error| error.into_error())?;
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
    VulkanBorrowedTextureDescriptor::new(
        extent(viewport),
        viewport.physical_width,
        viewport.physical_height,
        context_descriptor(vk),
        image.image_handle(),
        image.view_handle(),
        ash::vk::Format::R8G8B8A8_UNORM.as_raw() as u32,
        ash::vk::ImageLayout::UNDEFINED.as_raw() as u32,
        ash::vk::ImageLayout::SHADER_READ_ONLY_OPTIMAL.as_raw() as u32,
    )
}

fn context_descriptor(vk: &VulkanContext) -> VulkanContextDescriptor {
    let mut descriptor = VulkanContextDescriptor::new(
        vk.instance_pointer(),
        vk.physical_device_pointer(),
        vk.device_pointer(),
        vk.graphics_queue_pointer(),
        vk.graphics_queue_family_index(),
    );
    descriptor.get_instance_proc_addr = vk.get_instance_proc_addr_pointer();
    descriptor.get_device_proc_addr = vk.get_device_proc_addr_pointer();
    descriptor
}
