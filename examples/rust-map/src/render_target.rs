use maplibre_native::{
    Error, ErrorKind, MapHandle, RenderSessionHandle, VulkanOwnedTextureDescriptor,
    VulkanSurfaceDescriptor,
};
use std::error::Error as StdError;

use crate::viewport::Viewport;
use crate::vulkan::VulkanContext;
use crate::vulkan_texture_compositor::VulkanTextureCompositor;

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum Mode {
    OwnedTexture,
    NativeSurface,
}

impl Mode {
    pub fn cli_name(self) -> &'static str {
        match self {
            Self::OwnedTexture => "owned-texture",
            Self::NativeSurface => "native-surface",
        }
    }

    pub fn status(self) -> &'static str {
        match self {
            Self::OwnedTexture => "samples MapLibre-owned Vulkan frames into the winit swapchain",
            Self::NativeSurface => "renders directly to the winit Vulkan surface",
        }
    }

    pub fn parse(value: &str) -> Result<Self, String> {
        match value {
            "owned-texture" => Ok(Self::OwnedTexture),
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
    VulkanNativeSurface {
        session: RenderSessionHandle,
    },
}

impl RenderTarget {
    pub fn attach(
        mode: Mode,
        map: &MapHandle,
        vulkan: &VulkanContext,
        viewport: Viewport,
    ) -> maplibre_native::Result<Self> {
        match mode {
            Mode::OwnedTexture => Self::attach_vulkan_owned_texture(map, vulkan, viewport),
            Mode::NativeSurface => Self::attach_vulkan_surface(map, vulkan, viewport),
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
            Self::VulkanNativeSurface { session } => session.resize(
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
                compositor.draw(&frame)?;
                match frame.close() {
                    Ok(()) => Ok(()),
                    Err(error) => error
                        .into_handle()
                        .close()
                        .map_err(|error| error.into_error()),
                }
            }
            Self::VulkanNativeSurface { session } => session.render_update(),
        }
    }

    pub fn close(self) -> Result<(), Box<dyn StdError>> {
        match self {
            Self::VulkanOwnedTexture {
                session,
                mut compositor,
            } => {
                session
                    .close()
                    .map_err(|error| Box::new(error) as Box<dyn StdError>)?;
                compositor.close().map_err(|error| {
                    compositor_error(format!("Vulkan texture compositor close failed: {error:?}"))
                })
            }
            Self::VulkanNativeSurface { session } => session
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
            viewport.logical_width,
            viewport.logical_height,
            viewport.scale_factor,
            vulkan.instance_pointer(),
            vulkan.physical_device_pointer(),
            vulkan.device_pointer(),
            vulkan.graphics_queue_pointer(),
            vulkan.graphics_queue_family_index(),
        );
        let session = map.attach_vulkan_owned_texture(&descriptor)?;
        let compositor = match VulkanTextureCompositor::new(vulkan, viewport) {
            Ok(compositor) => compositor,
            Err(error) => {
                let _ = session.close();
                return Err(compositor_error(format!(
                    "Vulkan texture compositor creation failed: {error:?}"
                )));
            }
        };
        Ok(Self::VulkanOwnedTexture {
            session,
            compositor: Box::new(compositor),
        })
    }

    fn attach_vulkan_surface(
        map: &MapHandle,
        vulkan: &VulkanContext,
        viewport: Viewport,
    ) -> maplibre_native::Result<Self> {
        let descriptor = VulkanSurfaceDescriptor::new(
            viewport.logical_width,
            viewport.logical_height,
            viewport.scale_factor,
            vulkan.instance_pointer(),
            vulkan.physical_device_pointer(),
            vulkan.device_pointer(),
            vulkan.graphics_queue_pointer(),
            vulkan.graphics_queue_family_index(),
            vulkan.surface_pointer(),
        );
        Ok(Self::VulkanNativeSurface {
            session: map.attach_vulkan_surface(&descriptor)?,
        })
    }
}

fn compositor_error(message: impl Into<String>) -> Error {
    Error::new(ErrorKind::NativeError, None, message)
}
