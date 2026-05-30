use maplibre_native::{
    Error, ErrorKind, MapHandle, RenderSessionHandle, RenderTargetExtent,
    VulkanBorrowedTextureDescriptor, VulkanContextDescriptor, VulkanOwnedTextureDescriptor,
    VulkanSurfaceDescriptor,
};
use std::error::Error as StdError;

use crate::graphics::GraphicsContext;
#[cfg(target_os = "macos")]
use crate::metal::{MetalBorrowedTexture, MetalContext, MetalTextureCompositor};
use crate::viewport::Viewport;
use crate::vulkan::{BorrowedImage, VulkanContext};
use crate::vulkan_texture_compositor::VulkanTextureCompositor;

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum Mode {
    OwnedTexture,
    BorrowedTexture,
    NativeSurface,
}

impl Mode {
    pub fn cli_name(self) -> &'static str {
        match self {
            Self::OwnedTexture => "owned-texture",
            Self::BorrowedTexture => "borrowed-texture",
            Self::NativeSurface => "native-surface",
        }
    }

    pub fn status(self) -> &'static str {
        match self {
            Self::OwnedTexture => "samples MapLibre-owned texture frames into the host swapchain",
            Self::BorrowedTexture => {
                "renders into a host-owned texture, then samples it into the host swapchain"
            }
            Self::NativeSurface => "renders directly to the host window surface",
        }
    }

    pub fn parse(value: &str) -> Result<Self, String> {
        match value {
            "owned-texture" => Ok(Self::OwnedTexture),
            "borrowed-texture" => Ok(Self::BorrowedTexture),
            "native-surface" => Ok(Self::NativeSurface),
            _ => Err(format!("unknown render target '{value}'")),
        }
    }
}

pub enum RenderTarget {
    VulkanOwnedTexture {
        session: RenderSessionHandle,
        compositor: Box<VulkanTextureCompositor>,
    },
    VulkanBorrowedTexture {
        session: RenderSessionHandle,
        compositor: Box<VulkanTextureCompositor>,
        image: Box<BorrowedImage>,
    },
    VulkanNativeSurface {
        session: RenderSessionHandle,
    },
    #[cfg(target_os = "macos")]
    MetalOwnedTexture {
        session: RenderSessionHandle,
        compositor: Box<MetalTextureCompositor>,
    },
    #[cfg(target_os = "macos")]
    MetalBorrowedTexture {
        session: RenderSessionHandle,
        compositor: Box<MetalTextureCompositor>,
        texture: Box<MetalBorrowedTexture>,
    },
    #[cfg(target_os = "macos")]
    MetalNativeSurface {
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
        match graphics {
            #[cfg(target_os = "macos")]
            GraphicsContext::Metal(metal) => match mode {
                Mode::OwnedTexture => Self::attach_metal_owned_texture(map, metal, viewport),
                Mode::BorrowedTexture => Self::attach_metal_borrowed_texture(map, metal, viewport),
                Mode::NativeSurface => Self::attach_metal_surface(map, metal, viewport),
            },
            GraphicsContext::Vulkan(vulkan) => match mode {
                Mode::OwnedTexture => Self::attach_vulkan_owned_texture(map, vulkan, viewport),
                Mode::BorrowedTexture => {
                    Self::attach_vulkan_borrowed_texture(map, vulkan, viewport)
                }
                Mode::NativeSurface => Self::attach_vulkan_surface(map, vulkan, viewport),
            },
        }
    }

    pub fn needs_reattach_on_resize(&self) -> bool {
        match self {
            Self::VulkanBorrowedTexture { .. } => true,
            #[cfg(target_os = "macos")]
            Self::MetalBorrowedTexture { .. } => true,
            _ => false,
        }
    }

    pub fn resize(&mut self, viewport: Viewport) -> maplibre_native::Result<()> {
        match self {
            Self::VulkanOwnedTexture {
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
            Self::VulkanBorrowedTexture { .. } => Err(compositor_error(
                "borrowed texture resize requires render target reattachment",
            )),
            #[cfg(target_os = "macos")]
            Self::MetalBorrowedTexture { .. } => Err(compositor_error(
                "borrowed texture resize requires render target reattachment",
            )),
            Self::VulkanNativeSurface { session } => session.resize(
                viewport.logical_width,
                viewport.logical_height,
                viewport.scale_factor,
            ),
            #[cfg(target_os = "macos")]
            Self::MetalNativeSurface { session } => session.resize(
                viewport.logical_width,
                viewport.logical_height,
                viewport.scale_factor,
            ),
            #[cfg(target_os = "macos")]
            Self::MetalOwnedTexture { session, .. } => session.resize(
                viewport.logical_width,
                viewport.logical_height,
                viewport.scale_factor,
            ),
        }
    }

    pub fn render_update(&mut self) -> maplibre_native::Result<()> {
        match self {
            Self::VulkanOwnedTexture {
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
            Self::VulkanBorrowedTexture {
                session,
                compositor,
                image,
            } => {
                session.render_update()?;
                compositor.draw_image_view(image.view()).map_err(|error| {
                    compositor_error(format!("Vulkan texture compositor draw failed: {error:?}"))
                })
            }
            Self::VulkanNativeSurface { session } => session.render_update(),
            #[cfg(target_os = "macos")]
            Self::MetalOwnedTexture {
                session,
                compositor,
            } => {
                session.render_update()?;
                let frame = session.acquire_metal_owned_texture_frame()?;
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
            #[cfg(target_os = "macos")]
            Self::MetalBorrowedTexture {
                session,
                compositor,
                texture,
            } => {
                session.render_update()?;
                compositor.draw_texture(texture.texture())
            }
            #[cfg(target_os = "macos")]
            Self::MetalNativeSurface { session } => session.render_update(),
        }
    }

    pub fn close(self) -> Result<(), Box<dyn StdError>> {
        match self {
            Self::VulkanOwnedTexture {
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
            Self::VulkanBorrowedTexture {
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
            Self::VulkanNativeSurface { session } => session
                .close()
                .map_err(|error| Box::new(error) as Box<dyn StdError>),
            #[cfg(target_os = "macos")]
            Self::MetalOwnedTexture {
                session,
                compositor,
            } => {
                drop(compositor);
                session
                    .close()
                    .map_err(|error| Box::new(error) as Box<dyn StdError>)
            }
            #[cfg(target_os = "macos")]
            Self::MetalBorrowedTexture {
                session,
                compositor,
                texture,
            } => {
                drop(compositor);
                let result = session
                    .close()
                    .map_err(|error| Box::new(error) as Box<dyn StdError>);
                drop(texture);
                result
            }
            #[cfg(target_os = "macos")]
            Self::MetalNativeSurface { session } => session
                .close()
                .map_err(|error| Box::new(error) as Box<dyn StdError>),
        }
    }

    fn attach_vulkan_owned_texture(
        map: &MapHandle,
        vulkan: &VulkanContext,
        viewport: Viewport,
    ) -> maplibre_native::Result<Self> {
        let descriptor = VulkanOwnedTextureDescriptor::new(
            RenderTargetExtent::new(
                viewport.logical_width,
                viewport.logical_height,
                viewport.scale_factor,
            ),
            VulkanContextDescriptor::new(
                vulkan.instance_pointer(),
                vulkan.physical_device_pointer(),
                vulkan.device_pointer(),
                vulkan.graphics_queue_pointer(),
                vulkan.graphics_queue_family_index(),
            )
            .with_proc_addresses(
                vulkan.get_instance_proc_addr_pointer(),
                vulkan.get_device_proc_addr_pointer(),
            ),
        );
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
        Ok(Self::VulkanOwnedTexture {
            session,
            compositor: Box::new(compositor),
        })
    }

    fn attach_vulkan_borrowed_texture(
        map: &MapHandle,
        vulkan: &VulkanContext,
        viewport: Viewport,
    ) -> maplibre_native::Result<Self> {
        let image = BorrowedImage::new(vulkan, viewport).map_err(|error| {
            compositor_error(format!("Vulkan borrowed image creation failed: {error:?}"))
        })?;
        let descriptor = VulkanBorrowedTextureDescriptor::new(
            RenderTargetExtent::new(
                viewport.logical_width,
                viewport.logical_height,
                viewport.scale_factor,
            ),
            VulkanContextDescriptor::new(
                vulkan.instance_pointer(),
                vulkan.physical_device_pointer(),
                vulkan.device_pointer(),
                vulkan.graphics_queue_pointer(),
                vulkan.graphics_queue_family_index(),
            )
            .with_proc_addresses(
                vulkan.get_instance_proc_addr_pointer(),
                vulkan.get_device_proc_addr_pointer(),
            ),
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
        Ok(Self::VulkanBorrowedTexture {
            session,
            compositor: Box::new(compositor),
            image: Box::new(image),
        })
    }

    fn attach_vulkan_surface(
        map: &MapHandle,
        vulkan: &VulkanContext,
        viewport: Viewport,
    ) -> maplibre_native::Result<Self> {
        let descriptor = VulkanSurfaceDescriptor::new(
            RenderTargetExtent::new(
                viewport.logical_width,
                viewport.logical_height,
                viewport.scale_factor,
            ),
            VulkanContextDescriptor::new(
                vulkan.instance_pointer(),
                vulkan.physical_device_pointer(),
                vulkan.device_pointer(),
                vulkan.graphics_queue_pointer(),
                vulkan.graphics_queue_family_index(),
            )
            .with_proc_addresses(
                vulkan.get_instance_proc_addr_pointer(),
                vulkan.get_device_proc_addr_pointer(),
            ),
            vulkan.surface_pointer(),
        );
        Ok(Self::VulkanNativeSurface {
            session: map.attach_vulkan_surface(&descriptor)?,
        })
    }

    #[cfg(target_os = "macos")]
    fn attach_metal_owned_texture(
        map: &MapHandle,
        metal: &MetalContext,
        viewport: Viewport,
    ) -> maplibre_native::Result<Self> {
        let descriptor = maplibre_native::MetalOwnedTextureDescriptor::new(
            RenderTargetExtent::new(
                viewport.logical_width,
                viewport.logical_height,
                viewport.scale_factor,
            ),
            metal.context_descriptor(),
        );
        let session = map.attach_metal_owned_texture(&descriptor)?;
        let compositor = match MetalTextureCompositor::new(metal) {
            Ok(compositor) => compositor,
            Err(error) => {
                let mut message = format!("Metal texture compositor creation failed: {error:?}");
                if let Err(close_error) = session.close() {
                    message.push_str(&format!("; render session cleanup failed: {close_error}"));
                }
                return Err(compositor_error(message));
            }
        };
        Ok(Self::MetalOwnedTexture {
            session,
            compositor: Box::new(compositor),
        })
    }

    #[cfg(target_os = "macos")]
    fn attach_metal_borrowed_texture(
        map: &MapHandle,
        metal: &MetalContext,
        viewport: Viewport,
    ) -> maplibre_native::Result<Self> {
        let texture = MetalBorrowedTexture::new(metal, viewport)?;
        let descriptor = maplibre_native::MetalBorrowedTextureDescriptor::new(
            RenderTargetExtent::new(
                viewport.logical_width,
                viewport.logical_height,
                viewport.scale_factor,
            ),
            texture.pointer(),
        );
        let session = map.attach_metal_borrowed_texture(&descriptor)?;
        let compositor = match MetalTextureCompositor::new(metal) {
            Ok(compositor) => compositor,
            Err(error) => {
                let mut message = format!("Metal texture compositor creation failed: {error:?}");
                if let Err(close_error) = session.close() {
                    message.push_str(&format!("; render session cleanup failed: {close_error}"));
                }
                return Err(compositor_error(message));
            }
        };
        Ok(Self::MetalBorrowedTexture {
            session,
            compositor: Box::new(compositor),
            texture: Box::new(texture),
        })
    }

    #[cfg(target_os = "macos")]
    fn attach_metal_surface(
        map: &MapHandle,
        metal: &MetalContext,
        viewport: Viewport,
    ) -> maplibre_native::Result<Self> {
        let descriptor = maplibre_native::MetalSurfaceDescriptor::new(
            RenderTargetExtent::new(
                viewport.logical_width,
                viewport.logical_height,
                viewport.scale_factor,
            ),
            metal.context_descriptor(),
            metal.layer_pointer(),
        );
        Ok(Self::MetalNativeSurface {
            session: map.attach_metal_surface(&descriptor)?,
        })
    }
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
