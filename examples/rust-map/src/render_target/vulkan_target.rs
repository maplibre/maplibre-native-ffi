use std::error::Error as StdError;

use maplibre_native::{
    Error, ErrorKind, MapHandle, RenderSessionHandle, VulkanBorrowedTextureDescriptor,
    VulkanContextDescriptor, VulkanOwnedTextureDescriptor, VulkanSurfaceDescriptor,
};

use crate::graphics::GraphicsContext;
use crate::render_target::{Mode, extent};
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
    ) -> maplibre_native::Result<Self> {
        let vulkan = graphics.vulkan();
        match mode {
            Mode::OwnedTexture => attach_owned_texture(map, vulkan, viewport),
            Mode::BorrowedTexture => attach_borrowed_texture(map, vulkan, viewport),
            Mode::NativeSurface => attach_surface(map, vulkan, viewport),
        }
    }

    pub fn needs_reattach_on_resize(&self) -> bool {
        matches!(self, Self::BorrowedTexture { .. })
    }

    pub fn resize(&mut self, viewport: Viewport) -> maplibre_native::Result<()> {
        match self {
            Self::OwnedTexture {
                session,
                compositor,
            } => {
                compositor.resize(viewport).map_err(|error| {
                    compositor_error(format!(
                        "Vulkan texture compositor resize failed: {error:?}"
                    ))
                })?;
                session.resize(
                    viewport.logical_width,
                    viewport.logical_height,
                    viewport.scale_factor,
                )
            }
            Self::BorrowedTexture { .. } => Err(compositor_error(
                "borrowed texture resize requires render target reattachment",
            )),
            Self::Surface { session } => session.resize(
                viewport.logical_width,
                viewport.logical_height,
                viewport.scale_factor,
            ),
        }
    }

    pub fn render_update(&mut self, _graphics: &GraphicsContext) -> maplibre_native::Result<()> {
        match self {
            Self::OwnedTexture {
                session,
                compositor,
            } => {
                session.render_update()?;
                let frame = session.acquire_vulkan_owned_texture_frame()?;
                let draw_result = compositor.draw(&frame);
                let close_result = frame.close().map_err(|error| error.into_error());
                match (draw_result, close_result) {
                    (Ok(()), Ok(())) => Ok(()),
                    (Err(draw_error), Ok(())) => Err(draw_error),
                    (Ok(()), Err(close_error)) => Err(close_error),
                    (Err(draw_error), Err(close_error)) => Err(Error::new(
                        draw_error.kind(),
                        draw_error.raw_status(),
                        format!("{draw_error}; frame cleanup failed: {close_error}"),
                    )),
                }
            }
            Self::BorrowedTexture {
                session,
                compositor,
                image,
            } => {
                session.render_update()?;
                compositor.draw_image_view(image.view()).map_err(|error| {
                    compositor_error(format!("Vulkan texture compositor draw failed: {error:?}"))
                })
            }
            Self::Surface { session } => session.render_update(),
        }
    }

    pub fn close(self, _graphics: &GraphicsContext) -> Result<(), Box<dyn StdError>> {
        match self {
            Self::OwnedTexture {
                session,
                mut compositor,
            } => {
                let mut close_error = compositor
                    .close()
                    .err()
                    .map(|error| format!("Vulkan texture compositor close failed: {error:?}"));
                if let Err(error) = session.close() {
                    append_error(
                        &mut close_error,
                        format!("render session close failed: {error}"),
                    );
                }
                match close_error {
                    Some(error) => Err(Box::new(compositor_error(error))),
                    None => Ok(()),
                }
            }
            Self::BorrowedTexture {
                session,
                mut compositor,
                image,
            } => {
                let mut close_error = compositor
                    .close()
                    .err()
                    .map(|error| format!("Vulkan texture compositor close failed: {error:?}"));
                if let Err(error) = session.close() {
                    append_error(
                        &mut close_error,
                        format!("render session close failed: {error}"),
                    );
                }
                drop(image);
                match close_error {
                    Some(error) => Err(Box::new(compositor_error(error))),
                    None => Ok(()),
                }
            }
            Self::Surface { session } => session
                .close()
                .map_err(|error| Box::new(error) as Box<dyn StdError>),
        }
    }
}

fn attach_owned_texture(
    map: &MapHandle,
    vulkan: &VulkanContext,
    viewport: Viewport,
) -> maplibre_native::Result<RenderTarget> {
    let descriptor =
        VulkanOwnedTextureDescriptor::new(extent(viewport), context_descriptor(vulkan));
    let session = map.attach_vulkan_owned_texture(&descriptor)?;
    let compositor = match VulkanTextureCompositor::new(vulkan, viewport) {
        Ok(compositor) => compositor,
        Err(error) => {
            let mut message = format!("Vulkan texture compositor creation failed: {error:?}");
            if let Err(close_error) = session.close() {
                message.push_str(&format!("; render session cleanup failed: {close_error}"));
            }
            return Err(compositor_error(message));
        }
    };
    Ok(RenderTarget::OwnedTexture {
        session,
        compositor: Box::new(compositor),
    })
}

fn attach_borrowed_texture(
    map: &MapHandle,
    vulkan: &VulkanContext,
    viewport: Viewport,
) -> maplibre_native::Result<RenderTarget> {
    let image = BorrowedImage::new(vulkan, viewport).map_err(|error| {
        compositor_error(format!("Vulkan borrowed image creation failed: {error:?}"))
    })?;
    let descriptor = VulkanBorrowedTextureDescriptor::new(
        extent(viewport),
        context_descriptor(vulkan),
        image.image_pointer(),
        image.view_pointer(),
        ash::vk::Format::R8G8B8A8_UNORM.as_raw() as u32,
        ash::vk::ImageLayout::UNDEFINED.as_raw() as u32,
        ash::vk::ImageLayout::SHADER_READ_ONLY_OPTIMAL.as_raw() as u32,
    );
    let session = map.attach_vulkan_borrowed_texture(&descriptor)?;
    let compositor = match VulkanTextureCompositor::new(vulkan, viewport) {
        Ok(compositor) => compositor,
        Err(error) => {
            let mut message = format!("Vulkan texture compositor creation failed: {error:?}");
            if let Err(close_error) = session.close() {
                message.push_str(&format!("; render session cleanup failed: {close_error}"));
            }
            return Err(compositor_error(message));
        }
    };
    Ok(RenderTarget::BorrowedTexture {
        session,
        compositor: Box::new(compositor),
        image: Box::new(image),
    })
}

fn attach_surface(
    map: &MapHandle,
    vulkan: &VulkanContext,
    viewport: Viewport,
) -> maplibre_native::Result<RenderTarget> {
    let descriptor = VulkanSurfaceDescriptor::new(
        extent(viewport),
        context_descriptor(vulkan),
        vulkan.surface_pointer(),
    );
    Ok(RenderTarget::Surface {
        session: map.attach_vulkan_surface(&descriptor)?,
    })
}

fn context_descriptor(vulkan: &VulkanContext) -> VulkanContextDescriptor {
    let mut descriptor = VulkanContextDescriptor::new(
        vulkan.instance_pointer(),
        vulkan.physical_device_pointer(),
        vulkan.device_pointer(),
        vulkan.graphics_queue_pointer(),
        vulkan.graphics_queue_family_index(),
    );
    descriptor.get_instance_proc_addr = vulkan.get_instance_proc_addr_pointer();
    descriptor.get_device_proc_addr = vulkan.get_device_proc_addr_pointer();
    descriptor
}

fn append_error(message: &mut Option<String>, error: String) {
    match message {
        Some(message) => message.push_str(&format!("; {error}")),
        None => *message = Some(error),
    }
}

fn compositor_error(message: impl Into<String>) -> Error {
    Error::new(ErrorKind::NativeError, None, message)
}
