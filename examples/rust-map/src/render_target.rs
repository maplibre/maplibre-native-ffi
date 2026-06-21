#[cfg(feature = "metal")]
mod metal_target;
#[cfg(feature = "opengl")]
mod opengl_target;
#[cfg(feature = "vulkan")]
mod vulkan_target;

#[cfg(feature = "metal")]
pub use metal_target::RenderTarget;
#[cfg(feature = "opengl")]
pub use opengl_target::RenderTarget;
#[cfg(feature = "vulkan")]
pub use vulkan_target::RenderTarget;

use maplibre_native::RenderTargetExtent;

use crate::viewport::Viewport;

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

fn extent(viewport: Viewport) -> RenderTargetExtent {
    RenderTargetExtent::new(
        viewport.logical_width,
        viewport.logical_height,
        viewport.scale_factor,
    )
}
