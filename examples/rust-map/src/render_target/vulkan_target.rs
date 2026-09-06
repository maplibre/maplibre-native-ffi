use std::error::Error as StdError;
use std::time::Duration;

use maplibre_native_ffi::{
    Error, ErrorKind, GpuSync, MapHandle, RenderSessionAttachOptions, RenderSessionAttachment,
    RenderSessionHandle, VulkanBorrowedTextureDescriptor, VulkanContextDescriptor,
    VulkanOwnedTextureDescriptor, VulkanSurfaceDescriptor,
};

use crate::graphics::GraphicsContext;
use crate::render_target::{Mode, extent, request_render_frame, require_cpu_complete_producer};
use crate::viewport::Viewport;
use crate::vulkan::{BorrowedImage, VulkanContext};
use crate::vulkan_texture_compositor::VulkanTextureCompositor;

pub enum RenderTarget {
    OwnedTexture {
        session: RenderSessionHandle,
        compositor: Box<VulkanTextureCompositor>,
    },
    BorrowedTexture {
        session: RenderSessionHandle,
        compositor: Box<VulkanTextureCompositor>,
        image: Box<BorrowedImage>,
    },
    Surface {
        session: RenderSessionHandle,
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
        let options =
            RenderSessionAttachOptions::core_worker(if mode == Mode::OwnedTexture { 2 } else { 0 });
        match mode {
            Mode::OwnedTexture => {
                let descriptor =
                    VulkanOwnedTextureDescriptor::new(extent(viewport), context_descriptor(vk));
                let session =
                    finish_attach(map.attach_vulkan_owned_texture(&descriptor, options)?)?;
                let compositor = VulkanTextureCompositor::new(vk, viewport).map_err(|error| {
                    compositor_error(format!("Vulkan compositor creation failed: {error:?}"))
                })?;
                Ok(Self::OwnedTexture {
                    session,
                    compositor: Box::new(compositor),
                })
            }
            Mode::BorrowedTexture => {
                let image = BorrowedImage::new(vk, viewport).map_err(|error| {
                    compositor_error(format!("Vulkan image creation failed: {error:?}"))
                })?;
                let descriptor = borrowed_descriptor(vk, viewport, &image);
                let session =
                    finish_attach(map.attach_vulkan_borrowed_texture(&descriptor, options)?)?;
                let compositor = VulkanTextureCompositor::new(vk, viewport).map_err(|error| {
                    compositor_error(format!("Vulkan compositor creation failed: {error:?}"))
                })?;
                Ok(Self::BorrowedTexture {
                    session,
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
                    session: finish_attach(map.attach_vulkan_surface(&descriptor, options)?)?,
                })
            }
        }
    }

    pub fn resize(
        &mut self,
        graphics: &GraphicsContext,
        viewport: Viewport,
    ) -> maplibre_native_ffi::Result<()> {
        match self {
            Self::OwnedTexture {
                session,
                compositor,
            } => {
                compositor.resize(viewport).map_err(|error| {
                    compositor_error(format!("Vulkan resize failed: {error:?}"))
                })?;
                drop(session.resize(&extent(viewport))?);
                Ok(())
            }
            Self::BorrowedTexture {
                session,
                compositor,
                image,
            } => {
                let replacement =
                    BorrowedImage::new(graphics.vulkan(), viewport).map_err(|error| {
                        compositor_error(format!("Vulkan image creation failed: {error:?}"))
                    })?;
                let descriptor = borrowed_descriptor(graphics.vulkan(), viewport, &replacement);
                let operation = session.set_vulkan_borrowed_texture_target(&descriptor)?;
                wait_core(&operation, "Vulkan target replacement")?;
                **image = replacement;
                compositor.resize(viewport).map_err(|error| {
                    compositor_error(format!("Vulkan resize failed: {error:?}"))
                })?;
                Ok(())
            }
            Self::Surface { session } => {
                drop(session.resize(&extent(viewport))?);
                Ok(())
            }
        }
    }

    pub fn render_update(
        &mut self,
        _graphics: &GraphicsContext,
    ) -> maplibre_native_ffi::Result<bool> {
        let present = matches!(self, Self::Surface { .. });
        let session = match self {
            Self::OwnedTexture { session, .. }
            | Self::BorrowedTexture { session, .. }
            | Self::Surface { session } => session,
        };
        let rendered = request_render_frame(session, present, false)?;
        if !rendered {
            return Ok(false);
        }
        match self {
            Self::OwnedTexture {
                session,
                compositor,
            } => {
                let frame = session.acquire_frame()?;
                require_cpu_complete_producer(&frame)?;
                let presented = compositor.draw(&frame)?;
                compositor.wait_idle().map_err(|error| {
                    compositor_error(format!("Vulkan consumer wait failed: {error:?}"))
                })?;
                frame
                    .release(GpuSync::CpuComplete)
                    .map_err(|error| error.into_error())?;
                Ok(presented)
            }
            Self::BorrowedTexture {
                compositor, image, ..
            } => compositor
                .draw_image_view(image.view())
                .map_err(|error| compositor_error(format!("Vulkan draw failed: {error:?}"))),
            Self::Surface { .. } => Ok(true),
        }
    }

    pub fn close(self, _graphics: &GraphicsContext) -> Result<(), Box<dyn StdError>> {
        match self {
            Self::OwnedTexture {
                session,
                mut compositor,
            } => {
                detach(&session)?;
                compositor.close()?;
                destroy(session)
            }
            Self::BorrowedTexture {
                session,
                mut compositor,
                image,
            } => {
                detach(&session)?;
                compositor.close()?;
                drop(image);
                destroy(session)
            }
            Self::Surface { session } => {
                detach(&session)?;
                destroy(session)
            }
        }
    }
}

fn finish_attach(
    attachment: RenderSessionAttachment,
) -> maplibre_native_ffi::Result<RenderSessionHandle> {
    if !attachment.completion.wait(Duration::from_secs(30))? {
        return Err(compositor_error("render attachment timed out"));
    }
    attachment.completion.take()?;
    let session = attachment.session;
    Ok(session)
}

fn wait_core(
    operation: &maplibre_native_ffi::NativeFuture<()>,
    name: &str,
) -> maplibre_native_ffi::Result<()> {
    if !operation.wait(Duration::from_secs(30))? {
        return Err(compositor_error(format!("{name} timed out")));
    }
    operation.take()
}

fn detach(session: &RenderSessionHandle) -> Result<(), Box<dyn StdError>> {
    let operation = session.detach()?;
    if !operation.wait(Duration::from_secs(30))? {
        return Err("render detach timed out".into());
    }
    operation.take()?;
    Ok(())
}

fn destroy(session: RenderSessionHandle) -> Result<(), Box<dyn StdError>> {
    session
        .destroy()
        .map_err(|error| Box::new(error) as Box<dyn StdError>)
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

fn compositor_error(message: impl Into<String>) -> Error {
    Error::new(ErrorKind::NativeError, None, message)
}
