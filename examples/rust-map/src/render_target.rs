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

use std::collections::VecDeque;
use std::error::Error as StdError;

use maplibre_native_ffi::{
    AcquiredFrameHandle, Error, ErrorKind, FrameDemand, FrameDemandFlag, GpuSync, NativeFuture,
    RenderResult, RenderSessionAttachOptions, RenderSessionHandle, RenderTargetExtent,
};

use crate::shell::{AppEvent, DriverWait, Wakes};
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

/// What one drain of the frame-result queue found.
#[derive(Clone, Copy, Debug, Default)]
pub struct FrameResults {
    /// A demand rendered a frame.
    pub rendered: bool,
    /// The map asked for another frame while it rendered one.
    pub needs_repaint: bool,
    /// The target could not produce a frame, so the loop retries later.
    pub target_not_ready: bool,
}

pub fn extent(viewport: Viewport) -> RenderTargetExtent {
    RenderTargetExtent::new(
        viewport.logical_width,
        viewport.logical_height,
        viewport.scale_factor,
    )
}

/// Caller-driver attach options whose wakes post frame-result and driver-work
/// events to the winit loop.
pub fn attach_options(wakes: &Wakes, mode: Mode) -> RenderSessionAttachOptions {
    RenderSessionAttachOptions {
        driver: maplibre_native_ffi::RenderDriverKind::CallerGraphicsThread,
        requested_texture_ring_depth: if mode == Mode::OwnedTexture { 2 } else { 0 },
        frame_wake: wakes.wake(AppEvent::FrameResults),
        driver_work_wake: wakes.wake(AppEvent::DriverWork),
    }
}

/// A caller-graphics-thread render session plus the monotonic demand tokens
/// that tie each frame result back to the demand that produced it.
pub struct Session {
    session: RenderSessionHandle,
    presents: bool,
    next_token: u64,
    /// The newest demand token with a rendered result.
    rendered_token: u64,
    driver_wait: DriverWait,
}

impl Session {
    /// Services driver work until the attachment resolves.
    pub fn new(
        attachment: (RenderSessionHandle, NativeFuture<()>),
        presents: bool,
        wakes: &Wakes,
    ) -> maplibre_native_ffi::Result<Self> {
        let session = Self {
            session: attachment.0,
            presents,
            next_token: 0,
            rendered_token: 0,
            driver_wait: wakes.driver_wait(),
        };
        session.service_until(&attachment.1)?;
        Ok(session)
    }

    pub fn handle(&self) -> &RenderSessionHandle {
        &self.session
    }

    /// Services every queued caller-driver item on the graphics thread.
    pub fn service(&self) -> maplibre_native_ffi::Result<()> {
        self.session.service_driver_work(0)?;
        Ok(())
    }

    /// Demands a frame. A forced demand renders even without a newer map
    /// update, which a retry after an undrawn frame needs.
    pub fn request_frame(&mut self, force: bool) -> maplibre_native_ffi::Result<()> {
        self.next_token += 1;
        let mut flags = FrameDemandFlag::empty();
        flags.set(FrameDemandFlag::IF_NEEDED, !force);
        flags.set(FrameDemandFlag::PRESENT, self.presents);
        self.session.request_frame(&FrameDemand {
            flags,
            token: self.next_token,
            ..FrameDemand::default()
        })
    }

    /// Drains every queued frame result.
    pub fn drain_results(&mut self) -> maplibre_native_ffi::Result<FrameResults> {
        let batch = match self.session.drain_frame_results() {
            Ok(batch) => batch,
            Err(error) if error.kind() == ErrorKind::NotReady => return Ok(FrameResults::default()),
            Err(error) => return Err(error),
        };
        let mut results = FrameResults::default();
        for index in 0..batch.count()? {
            let result = batch.get(index)?;
            // No update and size pending wait for the map's next update,
            // superseded demands have a newer one behind them, and no demand
            // carries a timeout.
            match result.disposition {
                RenderResult::Rendered => {
                    results.rendered = true;
                    results.needs_repaint = result.needs_repaint;
                    self.rendered_token = self.rendered_token.max(result.token);
                }
                RenderResult::TargetNotReady => results.target_not_ready = true,
                _ => {}
            }
        }
        Ok(results)
    }

    /// Starts the session resize that carries the new logical extent to the
    /// map. A later resize supersedes this one, so a live resize needs no
    /// pacing.
    pub fn resize(&self, viewport: Viewport) -> maplibre_native_ffi::Result<()> {
        self.session.resize(&extent(viewport))?;
        Ok(())
    }

    /// Acquires the newest rendered frame, releasing any older one unsampled.
    /// Reports `None` while the ring holds none.
    pub fn acquire_newest(&self) -> maplibre_native_ffi::Result<Option<AcquiredFrameHandle>> {
        let mut newest = None;
        loop {
            match self.session.acquire_frame() {
                Ok(frame) => {
                    if let Some(older) = newest.replace(frame) {
                        AcquiredFrameHandle::release(&older, &GpuSync::default())?;
                    }
                }
                Err(error) if error.kind() == ErrorKind::NotReady => return Ok(newest),
                Err(error) => return Err(error),
            }
        }
    }

    /// Detaches on the graphics thread, then destroys the session.
    pub fn close(self) -> Result<(), Box<dyn StdError>> {
        let operation = self.session.detach()?;
        self.service_until(&operation)?;
        self.session
            .destroy()
            .map_err(|error| Box::new(error) as Box<dyn StdError>)
    }

    /// Services driver work until a lifecycle submission resolves. Startup and
    /// shutdown block here, between driver wakes. A caller driver completes
    /// the submission inside a service call.
    fn service_until<T>(&self, operation: &NativeFuture<T>) -> maplibre_native_ffi::Result<T> {
        loop {
            // A wake that arrives after the clear ends the next wait at once.
            self.driver_wait.clear();
            self.session.service_driver_work(0)?;
            if operation.is_ready() {
                return operation.take();
            }
            self.driver_wait.wait();
        }
    }
}

/// The caller-owned textures a borrowed-texture target hands over on resize,
/// oldest first. The session renders into a texture until its replacement
/// completes, so each outgoing texture stays alive until then.
pub struct Replacements<T> {
    entries: VecDeque<Replacement<T>>,
}

struct Replacement<T> {
    completion: NativeFuture<()>,
    texture: T,
    /// The demand whose rendered frame shows the replacement, once its
    /// set_target has completed.
    shown_token: u64,
}

impl<T> Default for Replacements<T> {
    fn default() -> Self {
        Self {
            entries: VecDeque::new(),
        }
    }
}

impl<T> Replacements<T> {
    /// Queues the texture a set_target call handed over, with that call's
    /// completion.
    pub fn push(&mut self, completion: NativeFuture<()>, texture: T) {
        self.entries.push_back(Replacement {
            completion,
            texture,
            shown_token: 0,
        });
    }

    /// Takes the oldest replacement that a rendered frame has drawn into, or
    /// `None` when none has. A completed replacement holds no frame yet, so
    /// the first call that finds it demands one. A failed replacement reports
    /// its error and stays queued: the session may still render into it or the
    /// texture before it, so neither is released before the session detaches.
    pub fn take_shown(&mut self, session: &mut Session) -> maplibre_native_ffi::Result<Option<T>> {
        let Some(oldest) = self.entries.front_mut() else {
            return Ok(None);
        };
        if !oldest.completion.is_ready() {
            return Ok(None);
        }
        if oldest.shown_token == 0 {
            oldest.completion.take()?;
            session.request_frame(true)?;
            oldest.shown_token = session.next_token;
        }
        if session.rendered_token < oldest.shown_token {
            return Ok(None);
        }
        Ok(self.entries.pop_front().map(|entry| entry.texture))
    }

    /// Takes every replacement whatever its state, for teardown after the
    /// session detached. Only OpenGL textures need an explicit close.
    #[cfg(maplibre_render_backend = "opengl")]
    pub fn take_all(&mut self) -> impl Iterator<Item = T> + '_ {
        self.entries.drain(..).map(|entry| entry.texture)
    }
}

pub fn require_cpu_complete_producer(
    frame: &AcquiredFrameHandle,
) -> maplibre_native_ffi::Result<()> {
    frame.get_producer_sync(|producer| {
        if producer.kind == maplibre_native_ffi::GpuSyncKind::CpuComplete {
            return Ok(());
        }
        Err(Error::new(
            ErrorKind::InvalidState,
            None,
            format!("rust-map cannot consume a {producer:?} producer synchronization payload"),
        ))
    })?
}

pub fn compositor_error(message: impl Into<String>) -> Error {
    Error::new(ErrorKind::NativeError, None, message)
}
