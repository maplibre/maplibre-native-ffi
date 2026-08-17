#[cfg(maplibre_render_backend = "metal")]
mod metal_target;
#[cfg(maplibre_render_backend = "opengl")]
mod opengl_target;
#[cfg(maplibre_render_backend = "vulkan")]
mod vulkan_target;

#[cfg(maplibre_render_backend = "metal")]
pub use metal_target::RenderTarget;
#[cfg(maplibre_render_backend = "opengl")]
pub use opengl_target::RenderTarget;
#[cfg(maplibre_render_backend = "vulkan")]
pub use vulkan_target::RenderTarget;

use maplibre_native_ffi::{
    AcquiredFrameHandle, Error, ErrorKind, FrameDemand, FrameDisposition, GpuSyncKind,
    RenderSessionHandle, RenderTargetExtent,
};

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

fn request_render_frame(
    session: &RenderSessionHandle,
    present: bool,
    service_caller_driver: bool,
) -> maplibre_native_ffi::Result<bool> {
    session.request_frame(FrameDemand {
        present,
        ..FrameDemand::default()
    })?;
    if service_caller_driver {
        session.service_driver_work(usize::MAX)?;
    }
    Ok(session
        .drain_frame_results()?
        .copy_results()?
        .into_iter()
        .any(|result| result.disposition == FrameDisposition::Rendered))
}

fn require_cpu_complete_producer(frame: &AcquiredFrameHandle) -> maplibre_native_ffi::Result<()> {
    let producer = frame.producer_sync()?;
    if producer.kind == GpuSyncKind::CpuComplete {
        return Ok(());
    }
    Err(Error::new(
        ErrorKind::InvalidState,
        None,
        format!(
            "rust-map cannot consume a {:?} producer synchronization payload",
            producer.kind
        ),
    ))
}
