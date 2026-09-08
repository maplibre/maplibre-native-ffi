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

use std::error::Error as StdError;

use maplibre_native_ffi::{
    AcquiredFrameHandle, Error, ErrorKind, FrameDemand, FrameDisposition, FrameGpuSync,
    NativeFuture, RenderSessionAttachment, RenderSessionHandle, RenderTargetExtent,
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

/// One frame demand's outcome: whether the session rendered the demand, and
/// whether the map asked for another frame while it rendered this one.
#[derive(Clone, Copy, Debug, Default)]
pub struct FrameOutcome {
    pub rendered: bool,
    pub needs_repaint: bool,
}

pub fn extent(viewport: Viewport) -> RenderTargetExtent {
    RenderTargetExtent::new(
        viewport.logical_width,
        viewport.logical_height,
        viewport.scale_factor,
    )
}

/// A caller-graphics-thread render session plus the monotonic demand tokens
/// that tie each frame result back to the demand that produced it.
///
/// Every ordered session submission runs on the graphics thread that attached,
/// so this type also owns the loop that services driver work while one is
/// outstanding.
pub struct FrameDriver {
    session: RenderSessionHandle,
    next_token: u64,
}

impl FrameDriver {
    /// Services driver work until the attachment resolves.
    pub fn new(attachment: RenderSessionAttachment) -> maplibre_native_ffi::Result<Self> {
        let driver = Self {
            session: attachment.session,
            next_token: 0,
        };
        driver.drive(&attachment.completion)?;
        Ok(driver)
    }

    pub fn session(&self) -> &RenderSessionHandle {
        &self.session
    }

    /// Services driver work until one ordered submission resolves.
    pub fn drive(&self, operation: &NativeFuture<()>) -> maplibre_native_ffi::Result<()> {
        while !operation.is_ready() {
            if self.session.service_driver_work(0)? == 0 {
                std::thread::yield_now();
            }
        }
        operation.take()
    }

    /// Carries a new logical extent to the map through the attached session,
    /// which is the only extent authority while it stays attached.
    pub fn resize(&self, viewport: Viewport) -> maplibre_native_ffi::Result<()> {
        let operation = self.session.resize(&extent(viewport))?;
        self.drive(&operation)
    }

    /// Submits one demand, services driver work, and reports the outcome of
    /// the result carrying this demand's token.
    pub fn render_frame(&mut self, present: bool) -> maplibre_native_ffi::Result<FrameOutcome> {
        self.next_token += 1;
        let token = self.next_token;
        self.session.request_frame(FrameDemand {
            present,
            token,
            ..FrameDemand::default()
        })?;
        self.session.service_driver_work(0)?;
        Ok(self
            .session
            .drain_frame_results()?
            .into_iter()
            .find(|result| result.token == token)
            .map(|result| FrameOutcome {
                rendered: result.disposition == FrameDisposition::Rendered,
                needs_repaint: result.needs_repaint,
            })
            .unwrap_or_default())
    }

    /// Leases the rendered frame, reporting `None` while the ring holds none.
    pub fn acquire_frame(&self) -> maplibre_native_ffi::Result<Option<AcquiredFrameHandle>> {
        match self.session.acquire_frame() {
            Ok(frame) => Ok(Some(frame)),
            Err(error) if error.kind() == ErrorKind::NotReady => Ok(None),
            Err(error) => Err(error),
        }
    }

    /// Detaches on the graphics thread, then destroys the session.
    pub fn close(self) -> Result<(), Box<dyn StdError>> {
        let operation = self.session.detach()?;
        self.drive(&operation)?;
        self.session
            .destroy()
            .map_err(|error| Box::new(error) as Box<dyn StdError>)
    }
}

pub fn require_cpu_complete_producer(
    frame: &AcquiredFrameHandle,
) -> maplibre_native_ffi::Result<()> {
    let producer = frame.producer_sync()?;
    if matches!(producer, FrameGpuSync::CpuComplete) {
        return Ok(());
    }
    Err(Error::new(
        ErrorKind::InvalidState,
        None,
        format!("rust-map cannot consume a {producer:?} producer synchronization payload"),
    ))
}

pub fn compositor_error(message: impl Into<String>) -> Error {
    Error::new(ErrorKind::NativeError, None, message)
}
