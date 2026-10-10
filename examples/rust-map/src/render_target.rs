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
use std::task::{Context, Poll, Waker};
use std::time::Duration;

use maplibre_native_ffi::{
    AcquiredFrameHandle, Error, ErrorKind, FrameDemand, FrameDemandFlag, GpuSync, NativeFuture,
    RenderDriverKind, RenderResult, RenderSessionAttachOptions, RenderSessionHandle,
    RenderTargetExtent,
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
    /// The drain found at least one result.
    pub any: bool,
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

pub fn driver_label(driver: RenderDriverKind) -> &'static str {
    match driver {
        RenderDriverKind::CoreWorker => "core-worker",
        _ => "caller-graphics-thread",
    }
}

/// Attach options for `driver` whose wakes post frame-result events and, for a
/// caller driver, driver-work events to the winit loop. Only an owned texture
/// has a ring.
pub fn attach_options(
    wakes: &Wakes,
    mode: Mode,
    driver: RenderDriverKind,
) -> RenderSessionAttachOptions {
    let mut options = RenderSessionAttachOptions {
        driver,
        requested_texture_ring_depth: if mode == Mode::OwnedTexture { 2 } else { 0 },
        frame_wake: wakes.wake(AppEvent::FrameResults),
        ..RenderSessionAttachOptions::default()
    };
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
    /// Whether the session and the host take turns with one texture, as a
    /// core-worker borrowed texture does. The session owns it from a demand
    /// until its result, and the host owns it until the compositor's reads
    /// finish, so at most one demand is outstanding.
    takes_turns: bool,
    /// Whether a turn-taking session has a demand outstanding.
    demand_outstanding: bool,
    /// A demand that arrived while one was outstanding, sent once the
    /// compositor is done. A forced one renders without a newer map update.
    wanted: Option<bool>,
    next_token: u64,
    /// The newest demand token with a rendered result.
    rendered_token: u64,
    /// The newest owned-texture frame, held until a newer one replaces it.
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
            // A caller driver renders and composes on one thread, in order.
            takes_turns: mode == Mode::BorrowedTexture
                && options.driver == RenderDriverKind::CoreWorker,
            demand_outstanding: false,
            wanted: None,
            next_token: 0,
            rendered_token: 0,
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

    /// Demands a frame and returns the token whose result shows it. A forced
    /// demand renders even without a newer map update, which a retry after an
    /// undrawn frame needs. While a turn-taking session has a demand
    /// outstanding, the demand waits for [`Session::compositor_done`].
    pub fn request_frame(&mut self, force: bool) -> maplibre_native_ffi::Result<u64> {
        if self.demand_outstanding {
            self.wanted = Some(force || self.wanted.unwrap_or(false));
            return Ok(self.next_token + 1);
        }
        self.next_token += 1;
        let mut flags = FrameDemandFlag::empty();
        flags.set(FrameDemandFlag::IF_NEEDED, !force);
        flags.set(FrameDemandFlag::PRESENT, self.presents);
        self.session.request_frame(&FrameDemand {
            flags,
            token: self.next_token,
            ..FrameDemand::default()
        })?;
        self.demand_outstanding = self.takes_turns;
        Ok(self.next_token)
    }

    /// Ends the host's turn with a turn-taking session's texture after a drain
    /// that found results, sending any demand that waited for it.
    pub fn compositor_done(&mut self) -> maplibre_native_ffi::Result<()> {
        self.demand_outstanding = false;
        if let Some(force) = self.wanted.take() {
            self.request_frame(force)?;
        }
        Ok(())
    }

    /// Drains every queued frame result.
    pub fn drain_results(&mut self) -> maplibre_native_ffi::Result<FrameResults> {
        let Some(batch) = self.session.drain_frame_results()? else {
            return Ok(FrameResults::default());
        };
        let count = batch.count()?;
        let mut results = FrameResults {
            any: count > 0,
            ..FrameResults::default()
        };
        for index in 0..count {
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
    /// pacing. A session resizes only while the host holds none of its frames.
    pub fn resize(&mut self, viewport: Viewport) -> maplibre_native_ffi::Result<()> {
        self.release_held()?;
        self.session.resize(&extent(viewport))?;
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

    fn release_held(&mut self) -> maplibre_native_ffi::Result<()> {
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

    /// Ends the session's graphics work without graphics calls, which
    /// completes any pending lifecycle submission with target loss.
    pub fn abandon(&self) {
        match self.session.abandon() {
            Ok(result) if result.quarantined_resource_count > 0 => eprintln!(
                "render session abandon quarantined {} resource groups",
                result.quarantined_resource_count
            ),
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

/// The caller-owned textures a borrowed-texture target hands over on resize,
/// oldest first. The session renders into a texture until its replacement
/// completes, so each outgoing texture stays alive until then. Each completion
/// wakes the event loop with [`AppEvent::TargetReplaced`].
pub struct Replacements<T> {
    entries: VecDeque<Replacement<T>>,
}

struct Replacement<T> {
    completion: NativeFuture<()>,
    /// The completion's result, once it has arrived.
    outcome: Option<maplibre_native_ffi::Result<()>>,
    texture: T,
    /// The demand whose rendered frame shows the replacement, once its
    /// set_target has completed.
    shown_token: u64,
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
    /// Queues the texture a set_target call handed over, with that call's
    /// completion.
    pub fn push(&mut self, completion: NativeFuture<()>, texture: T, wakes: &Wakes) {
        let mut replacement = Replacement {
            completion,
            outcome: None,
            texture,
            shown_token: 0,
        };
        replacement.poll(&wakes.waker(AppEvent::TargetReplaced));
        self.entries.push_back(replacement);
    }

    /// Takes the oldest replacement that a rendered frame has drawn into, or
    /// `None` when none has. A completed replacement holds no frame yet, so
    /// the first call that finds it demands one. A failed replacement reports
    /// its error and stays queued: the session may still render into it or the
    /// texture before it, so neither is released before the session detaches.
    pub fn take_shown(
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
        if oldest.shown_token == 0 {
            oldest.shown_token = session.request_frame(true)?;
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
