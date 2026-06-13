use maplibre_native::{
    Error, ErrorKind, MapHandle, RenderSessionHandle, RenderTargetExtent,
    VulkanBorrowedTextureDescriptor, VulkanContextDescriptor, VulkanOwnedTextureDescriptor,
    VulkanSurfaceDescriptor,
};
use std::error::Error as StdError;

#[cfg(any(target_os = "linux", target_os = "windows"))]
mod opengl_target;

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
    OwnedVulkanTexture {
        session: RenderSessionHandle,
        compositor: Box<VulkanTextureCompositor>,
    },
    BorrowedVulkanTexture {
        session: RenderSessionHandle,
        compositor: Box<VulkanTextureCompositor>,
        image: Box<BorrowedImage>,
    },
    VulkanSurface {
        session: RenderSessionHandle,
    },
    #[cfg(any(target_os = "linux", target_os = "windows"))]
    OpenGL(opengl_target::RenderTarget),
    #[cfg(target_os = "macos")]
    OwnedMetalTexture {
        session: RenderSessionHandle,
        compositor: Box<MetalTextureCompositor>,
    },
    #[cfg(target_os = "macos")]
    BorrowedMetalTexture {
        session: RenderSessionHandle,
        compositor: Box<MetalTextureCompositor>,
        texture: Box<MetalBorrowedTexture>,
    },
    #[cfg(target_os = "macos")]
    MetalSurface {
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
            #[cfg(any(target_os = "linux", target_os = "windows"))]
            GraphicsContext::OpenGL(opengl) => match mode {
                Mode::OwnedTexture => {
                    opengl_target::RenderTarget::attach_owned_texture(map, opengl, viewport)
                        .map(Self::OpenGL)
                }
                Mode::BorrowedTexture => {
                    opengl_target::RenderTarget::attach_borrowed_texture(map, opengl, viewport)
                        .map(Self::OpenGL)
                }
                Mode::NativeSurface => {
                    opengl_target::RenderTarget::attach_surface(map, opengl, viewport)
                        .map(Self::OpenGL)
                }
            },
        }
    }

    pub fn needs_reattach_on_resize(&self) -> bool {
        match self {
            Self::BorrowedVulkanTexture { .. } => true,
            #[cfg(any(target_os = "linux", target_os = "windows"))]
            Self::OpenGL(target) => target.needs_reattach_on_resize(),
            #[cfg(target_os = "macos")]
            Self::BorrowedMetalTexture { .. } => true,
            _ => false,
        }
    }

    pub fn resize(&mut self, viewport: Viewport) -> maplibre_native::Result<()> {
        match self {
            Self::OwnedVulkanTexture {
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
            Self::BorrowedVulkanTexture { .. } => Err(compositor_error(
                "borrowed texture resize requires render target reattachment",
            )),
            #[cfg(target_os = "macos")]
            Self::BorrowedMetalTexture { .. } => Err(compositor_error(
                "borrowed texture resize requires render target reattachment",
            )),
            Self::VulkanSurface { session } => session.resize(
                viewport.logical_width,
                viewport.logical_height,
                viewport.scale_factor,
            ),
            #[cfg(any(target_os = "linux", target_os = "windows"))]
            Self::OpenGL(target) => target.resize(viewport),
            #[cfg(target_os = "macos")]
            Self::MetalSurface { session } => session.resize(
                viewport.logical_width,
                viewport.logical_height,
                viewport.scale_factor,
            ),
            #[cfg(target_os = "macos")]
            Self::OwnedMetalTexture { session, .. } => session.resize(
                viewport.logical_width,
                viewport.logical_height,
                viewport.scale_factor,
            ),
        }
    }

    pub fn render_update(&mut self, graphics: &GraphicsContext) -> maplibre_native::Result<()> {
        #[cfg(target_os = "macos")]
        let _ = graphics;

        match self {
            Self::OwnedVulkanTexture {
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
            Self::BorrowedVulkanTexture {
                session,
                compositor,
                image,
            } => {
                session.render_update()?;
                compositor.draw_image_view(image.view()).map_err(|error| {
                    compositor_error(format!("Vulkan texture compositor draw failed: {error:?}"))
                })
            }
            Self::VulkanSurface { session } => session.render_update(),
            #[cfg(any(target_os = "linux", target_os = "windows"))]
            Self::OpenGL(target) => {
                let GraphicsContext::OpenGL(context) = graphics else {
                    return Err(compositor_error(
                        "OpenGL render target requires OpenGL context",
                    ));
                };
                target.render_update(context)
            }
            #[cfg(target_os = "macos")]
            Self::OwnedMetalTexture {
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
            Self::BorrowedMetalTexture {
                session,
                compositor,
                texture,
            } => {
                session.render_update()?;
                compositor.draw_texture(texture.texture())
            }
            #[cfg(target_os = "macos")]
            Self::MetalSurface { session } => session.render_update(),
        }
    }

    pub fn close(self, graphics: Option<&GraphicsContext>) -> Result<(), Box<dyn StdError>> {
        #[cfg(target_os = "macos")]
        let _ = graphics;

        match self {
            Self::OwnedVulkanTexture {
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
            Self::BorrowedVulkanTexture {
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
            Self::VulkanSurface { session } => session
                .close()
                .map_err(|error| Box::new(error) as Box<dyn StdError>),
            #[cfg(any(target_os = "linux", target_os = "windows"))]
            Self::OpenGL(target) => {
                let opengl = match graphics {
                    Some(GraphicsContext::OpenGL(context)) => Some(context.as_ref()),
                    _ => None,
                };
                target.close(opengl)
            }
            #[cfg(target_os = "macos")]
            Self::OwnedMetalTexture {
                session,
                compositor,
            } => {
                drop(compositor);
                session
                    .close()
                    .map_err(|error| Box::new(error) as Box<dyn StdError>)
            }
            #[cfg(target_os = "macos")]
            Self::BorrowedMetalTexture {
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
            Self::MetalSurface { session } => session
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
        Ok(Self::OwnedVulkanTexture {
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
        Ok(Self::BorrowedVulkanTexture {
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
        Ok(Self::VulkanSurface {
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
        Ok(Self::OwnedMetalTexture {
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
        Ok(Self::BorrowedMetalTexture {
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
        Ok(Self::MetalSurface {
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
