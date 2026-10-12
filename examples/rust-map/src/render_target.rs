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
use std::future::Future;
use std::pin::Pin;
use std::sync::atomic::{AtomicBool, Ordering};
use std::task::{Context, Poll, Waker};
use std::time::Duration;

use maplibre_native_ffi::{
    AcquiredFrameHandle, Error, ErrorKind, FrameDemand, FrameDemandFlag, GpuSync, LogicalExtent,
    NativeFuture, RenderDriverKind, RenderResult, RenderSessionAttachOptions, RenderSessionHandle,
};

use crate::shell::{AppEvent, DriverWait, Wakes};
use crate::viewport::Viewport;

static GRAPHICS_KEPT: AtomicBool = AtomicBool::new(false);

/// Whether an abandon kept graphics objects until the process exits. A kept
/// Vulkan object is a child of the host's device, and a kept swapchain of its
/// surface, so a Vulkan host then keeps those until the process exits too.
#[cfg_attr(not(maplibre_render_backend = "vulkan"), allow(dead_code))]
pub fn graphics_kept() -> bool {
    GRAPHICS_KEPT.load(Ordering::Acquire)
}

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

pub fn extent(viewport: Viewport) -> LogicalExtent {
    LogicalExtent::new(
        viewport.logical_width,
        viewport.logical_height,
        viewport.scale_factor,
    )
}

/// The slot count of a texture ring, owned or borrowed. The host holds the
/// newest frame until a newer one arrives, and the session never renders into
/// a held frame's texture, so it renders into the other one meanwhile.
pub const RING_DEPTH: usize = 2;

pub fn driver_label(driver: RenderDriverKind) -> &'static str {
    match driver {
        RenderDriverKind::CoreWorker => "core-worker",
        _ => "caller-graphics-thread",
    }
}

/// Attach options for `driver` whose wakes post frame-result events and, for a
/// caller driver, driver-work events to the winit loop. An owned texture asks
/// for a ring of [`RING_DEPTH`] slots; a borrowed ring's depth is its
/// texture count.
pub fn attach_options(
    wakes: &Wakes,
    mode: Mode,
    driver: RenderDriverKind,
) -> RenderSessionAttachOptions {
    let mut options = RenderSessionAttachOptions {
        driver,
        frame_wake: wakes.wake(AppEvent::FrameResults),
        ..RenderSessionAttachOptions::default()
    };
    if mode == Mode::OwnedTexture {
        options.requested_texture_ring_depth = RING_DEPTH as u32;
    }
    if driver == RenderDriverKind::CallerGraphicsThread {
        options.driver_work_wake = wakes.wake(AppEvent::DriverWork);
    }
    options
}

/// A render session plus the monotonic demand tokens that tie each frame
/// result back to the demand that produced it.
pub struct Session {
    session: RenderSessionHandle,
    driver: RenderDriverKind,
    presents: bool,
    next_token: u64,
    /// The newest texture frame, held until a newer one replaces it.
    held: Option<AcquiredFrameHandle>,
    driver_wait: DriverWait,
}

impl Session {
    /// Awaits the attachment, servicing a caller driver meanwhile.
    pub fn new(
        attachment: (RenderSessionHandle, NativeFuture<()>),
        options: &RenderSessionAttachOptions,
        mode: Mode,
        wakes: &Wakes,
    ) -> maplibre_native_ffi::Result<Self> {
        let session = Self {
            session: attachment.0,
            driver: options.driver,
            presents: mode == Mode::NativeSurface,
            next_token: 0,
            held: None,
            driver_wait: wakes.driver_wait(),
        };
        if let Err(error) = session.wait_for(&attachment.1) {
            session.abandon();
            return Err(error);
        }
        Ok(session)
    }

    pub fn handle(&self) -> &RenderSessionHandle {
        &self.session
    }

    pub fn driver(&self) -> RenderDriverKind {
        self.driver
    }

    /// Services every queued caller-driver item on the graphics thread.
    pub fn service(&self) -> maplibre_native_ffi::Result<()> {
        self.session.service_driver_work(0)?;
        Ok(())
    }

    /// Demands a frame. A forced demand renders even without a newer map
    /// update, which a retry after a frame that missed the window needs.
    pub fn request_frame(&mut self, force: bool) -> maplibre_native_ffi::Result<()> {
        self.next_token += 1;
        let mut flags = FrameDemandFlag::empty();
        flags.set(FrameDemandFlag::IF_NEEDED, !force);
        flags.set(FrameDemandFlag::PRESENT, self.presents);
        self.session.request_frame(&FrameDemand {
            flags,
            token: self.next_token,
            ..FrameDemand::default()
        })?;
        Ok(())
    }

    /// Drains every queued frame result.
    pub fn drain_results(&mut self) -> maplibre_native_ffi::Result<FrameResults> {
        let Some(batch) = self.session.drain_frame_results()? else {
            return Ok(FrameResults::default());
        };
        let view = batch.get()?;
        let mut results = FrameResults::default();
        for result in view.results {
            // No update and size pending wait for the map's next update,
            // superseded demands have a newer one behind them, and no demand
            // carries a timeout.
            match result.disposition {
                RenderResult::Rendered => {
                    results.rendered = true;
                    results.needs_repaint = result.needs_repaint;
                }
                RenderResult::TargetNotReady => results.target_not_ready = true,
                _ => {}
            }
        }
        Ok(results)
    }

    /// Starts the session resize that carries the new logical extent to the
    /// map. A later resize supersedes this one, so a live resize needs no
    /// pacing. A session resizes only while the host holds none of its frames.
    pub fn resize(&mut self, viewport: Viewport) -> maplibre_native_ffi::Result<()> {
        self.release_held()?;
        self.session.resize(extent(viewport))?;
        Ok(())
    }

    /// Holds the newest rendered frame, releasing every older one, and returns
    /// it when it is new. The compositor waits for its GPU work before
    /// returning, so CPU-complete release is accurate.
    pub fn acquire_newest(&mut self) -> maplibre_native_ffi::Result<Option<&AcquiredFrameHandle>> {
        let mut acquired = false;
        while let Some(frame) = self.session.acquire_frame()? {
            self.release_held()?;
            self.held = Some(frame);
            acquired = true;
        }
        Ok(if acquired { self.held.as_ref() } else { None })
    }

    /// Releases the held frame, if any. A session resizes or takes a
    /// replacement ring only while the host holds none of its frames.
    pub fn release_held(&mut self) -> maplibre_native_ffi::Result<()> {
        match self.held.take() {
            Some(frame) => frame.release(&GpuSync::default()),
            None => Ok(()),
        }
    }

    /// Detaches, abandoning instead when that fails, then destroys the
    /// session.
    pub fn close(mut self) -> Result<(), Box<dyn StdError>> {
        let detached = self
            .release_held()
            .and_then(|()| self.session.detach())
            .and_then(|operation| self.wait_for(&operation));
        if let Err(error) = detached {
            eprintln!("render session detach failed: {error}");
            self.abandon();
        }
        self.session
            .destroy()
            .map_err(|error| Box::new(error) as Box<dyn StdError>)
    }

    /// Ends the session's graphics work at once, which completes any pending
    /// lifecycle submission with target loss.
    pub fn abandon(&self) {
        match self.session.abandon() {
            Ok(result) if result.quarantined_resource_count > 0 => {
                GRAPHICS_KEPT.store(true, Ordering::Release);
                eprintln!(
                    "render session abandon kept {} resource groups until exit",
                    result.quarantined_resource_count
                );
            }
            Ok(_) => {}
            // A session that already released its target reports invalid
            // state.
            Err(error) if error.kind() == ErrorKind::InvalidState => {}
            Err(error) => eprintln!("render session abandon failed: {error}"),
        }
    }

    /// Waits for a lifecycle submission. A core worker needs nothing from this
    /// thread, so the wait blocks until the submission completes. A caller
    /// driver completes the submission inside a service call, so startup and
    /// shutdown service it here, between driver wakes.
    fn wait_for<T>(&self, operation: &NativeFuture<T>) -> maplibre_native_ffi::Result<T> {
        if self.driver == RenderDriverKind::CoreWorker {
            operation.wait(Duration::MAX)?;
            return operation.take();
        }
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

/// The rings of caller-owned textures that a borrowed-texture target retires
/// on resize, oldest first. The session renders into a ring until the
/// replacement that retires it completes, so each retired ring stays alive
/// until then. Each completion wakes the event loop with
/// [`AppEvent::TargetReplaced`].
pub struct Replacements<T> {
    entries: VecDeque<Replacement<T>>,
}

struct Replacement<T> {
    completion: NativeFuture<()>,
    /// The completion's result, once it has arrived.
    outcome: Option<maplibre_native_ffi::Result<()>>,
    retired: T,
}

impl<T> Replacement<T> {
    /// Polls the completion, which registers the waker for its arrival.
    fn poll(&mut self, waker: &Waker) {
        if self.outcome.is_none()
            && let Poll::Ready(outcome) =
                Pin::new(&mut self.completion).poll(&mut Context::from_waker(waker))
        {
            self.outcome = Some(outcome);
        }
    }
}

impl<T> Default for Replacements<T> {
    fn default() -> Self {
        Self {
            entries: VecDeque::new(),
        }
    }
}

impl<T> Replacements<T> {
    /// Queues the ring a set_target call retired, with that call's completion.
    pub fn push(&mut self, completion: NativeFuture<()>, retired: T, wakes: &Wakes) {
        let mut replacement = Replacement {
            completion,
            outcome: None,
            retired,
        };
        replacement.poll(&wakes.waker(AppEvent::TargetReplaced));
        self.entries.push_back(replacement);
    }

    /// Takes the oldest retired ring whose replacement has completed, or
    /// `None` when none has. A replacement publishes no map update, and a
    /// frame rendered before it can no longer be acquired, so taking one
    /// demands a forced frame for the new ring. A failed replacement reports
    /// its error and stays queued: the session may still render into the
    /// retired ring or the new one, so neither is released before the session
    /// detaches.
    pub fn take_completed(
        &mut self,
        session: &mut Session,
        wakes: &Wakes,
    ) -> maplibre_native_ffi::Result<Option<T>> {
        let Some(oldest) = self.entries.front_mut() else {
            return Ok(None);
        };
        oldest.poll(&wakes.waker(AppEvent::TargetReplaced));
        match &oldest.outcome {
            None => return Ok(None),
            Some(Err(error)) => return Err(error.clone()),
            Some(Ok(())) => {}
        }
        let retired = self.entries.pop_front().map(|entry| entry.retired);
        session.request_frame(true)?;
        Ok(retired)
    }

    /// Takes every retired ring whatever its state, for teardown after the
    /// session detached. Only OpenGL textures need an explicit close.
    #[cfg(maplibre_render_backend = "opengl")]
    pub fn take_all(&mut self) -> impl Iterator<Item = T> + '_ {
        self.entries.drain(..).map(|entry| entry.retired)
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
