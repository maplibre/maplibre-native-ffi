//! The runtime and map fixture, the waits, and the GPU fixtures that the suite
//! shares.
//!
//! Every wait here blocks on a signal that native raises: a runtime's event
//! wake, a session's frame and driver-work wakes, or a completion. The
//! deadline only bounds a hang.

#[cfg(not(target_os = "emscripten"))]
pub mod graphics;

use std::future::Future;
use std::pin::pin;
use std::sync::atomic::{AtomicU64, Ordering};
use std::sync::{Arc, Condvar, Mutex, MutexGuard, OnceLock, mpsc};
use std::task::{Context, Poll, Wake, Waker};
use std::time::{Duration, Instant};

use maplibre_native_ffi::*;

/// A style that loads with no request: one background layer.
pub const BACKGROUND_STYLE_JSON: &str = r##"{"version":8,"sources":{},"layers":[{"id":"background","type":"background","paint":{"background-color":"#d8f1ff"}}]}"##;
/// The color BACKGROUND_STYLE_JSON paints, as premultiplied RGBA8.
pub const BACKGROUND_RGBA: [u8; 4] = [0xd8, 0xf1, 0xff, 0xff];

/// How long a wait lasts before the test fails. Emulators and software
/// renderers stretch it through MLN_TEST_TIMEOUT_SCALE.
pub fn timeout() -> Duration {
    static SCALE: OnceLock<u32> = OnceLock::new();
    let scale = *SCALE.get_or_init(|| {
        std::env::var("MLN_TEST_TIMEOUT_SCALE")
            .ok()
            .and_then(|value| value.parse().ok())
            .unwrap_or(1)
    });
    Duration::from_secs(10) * scale
}

/// Serializes the tests that change process-global state: the leak reporter,
/// the log callback, and the network status. Each one restores the default
/// before it releases the guard.
pub fn global_state() -> MutexGuard<'static, ()> {
    static LOCK: Mutex<()> = Mutex::new(());
    LOCK.lock().unwrap_or_else(|poisoned| poisoned.into_inner())
}

/// A count of wakes that a waiter can block on.
#[derive(Default)]
pub struct Signal {
    count: Mutex<u64>,
    changed: Condvar,
}

impl Signal {
    pub fn raise(&self) {
        *self.count.lock().unwrap() += 1;
        self.changed.notify_all();
    }

    /// The current count, read before looking at the state a wake announces.
    pub fn seen(&self) -> u64 {
        *self.count.lock().unwrap()
    }

    /// Blocks until a wake arrives after `seen`, failing the test at the
    /// deadline.
    pub fn wait_past(&self, seen: u64, deadline: Instant, what: &str) {
        let mut count = self.count.lock().unwrap();
        while *count == seen {
            let now = Instant::now();
            assert!(now < deadline, "timed out waiting for {what}");
            count = self.changed.wait_timeout(count, deadline - now).unwrap().0;
        }
    }

    pub fn native_wake(self: &Arc<Self>) -> maplibre_native_ffi::Wake {
        let signal = Arc::clone(self);
        maplibre_native_ffi::Wake::new(move || signal.raise())
    }
}

impl Wake for Signal {
    fn wake(self: Arc<Self>) {
        self.raise();
    }
}

/// Blocks until `condition` holds, for state that changes with no wake to
/// announce it. As in the C suite's harness, the thread parks between checks
/// until the deadline.
///
/// This poll is for CB-08 only: a request takes a single cancel registration,
/// which that test spends, so nothing is left to wake the test when MapLibre
/// cancels the request. Every other wait uses a wake, through [`Signal`] or a
/// channel.
pub fn await_condition(what: &str, mut condition: impl FnMut() -> bool) {
    const RECHECK: Duration = Duration::from_millis(5);
    let deadline = Instant::now() + timeout();
    while !condition() {
        let now = Instant::now();
        assert!(now < deadline, "timed out waiting for {what}");
        std::thread::park_timeout(RECHECK.min(deadline - now));
    }
}

/// Blocks until a submitted operation completes and returns its value.
#[track_caller]
pub fn wait_for<T>(submitted: Result<NativeFuture<T>>) -> T {
    let future = submitted.expect("the submission failed");
    assert!(
        future.wait(timeout()).unwrap(),
        "the operation never completed"
    );
    future.take().expect("the operation failed")
}

/// Awaits a future, running `service` each time it is still pending and
/// blocking on `signal` when `service` finds nothing to do. The future wakes
/// `signal` itself, so a completion from any thread ends the wait.
pub fn drive<F: Future>(
    signal: &Arc<Signal>,
    future: F,
    mut service: impl FnMut() -> usize,
) -> F::Output {
    let waker = Waker::from(Arc::clone(signal));
    let mut context = Context::from_waker(&waker);
    let mut future = pin!(future);
    let deadline = Instant::now() + timeout();
    loop {
        let seen = signal.seen();
        if let Poll::Ready(output) = future.as_mut().poll(&mut context) {
            return output;
        }
        if service() == 0 {
            signal.wait_past(seen, deadline, "an operation to complete");
        }
    }
}

/// A resource provider that fails every request, so no test reaches the
/// network. A test that serves resources installs its own.
pub fn denying_provider() -> ResourceProvider {
    ResourceProvider::new(|request, handle| {
        let response = ResourceResponse {
            status: ResourceResponseStatus::Error,
            error_reason: ResourceErrorReason::NotFound,
            error_message: Some(format!(
                "the test fixture denies {}",
                request.requested_url.unwrap_or_default()
            )),
            ..Default::default()
        };
        handle
            .complete(&response)
            .expect("the denial was not accepted");
        ResourceProviderDecision::Handle
    })
}

/// A resource provider that serves `body` at `url` and denies everything else.
pub fn serving_provider(url: &'static str, body: &'static str) -> ResourceProvider {
    let deny = denying_provider().callback.unwrap();
    ResourceProvider::new(move |request, handle| {
        if request.requested_url.as_deref() != Some(url) {
            return deny(request, handle);
        }
        handle
            .complete(&ok_response(body.as_bytes()))
            .expect("the response was not accepted");
        ResourceProviderDecision::Handle
    })
}

/// Signals once it drops, so a test can wait for native to release the
/// callback that captured it.
pub struct ReleaseProbe(mpsc::Sender<()>);

impl Drop for ReleaseProbe {
    fn drop(&mut self) {
        let _ = self.0.send(());
    }
}

/// A probe to capture in a callback, and the receiver that its release
/// signals.
pub fn release_probe() -> (ReleaseProbe, mpsc::Receiver<()>) {
    let (sender, receiver) = mpsc::channel();
    (ReleaseProbe(sender), receiver)
}

/// Blocks until the callback that captured a probe has been released.
#[track_caller]
pub fn await_release(released: &mpsc::Receiver<()>) {
    released
        .recv_timeout(timeout())
        .expect("native never released the callback");
}

/// A response that serves `bytes`.
pub fn ok_response(bytes: impl Into<Vec<u8>>) -> ResourceResponse {
    ResourceResponse {
        status: ResourceResponseStatus::Ok,
        bytes: bytes.into(),
        ..Default::default()
    }
}

/// One runtime with a map, both released when the fixture drops, even when the
/// test fails.
pub struct Fixture {
    runtime: Option<Arc<RuntimeHandle>>,
    map: Option<MapHandle>,
    events: Arc<Signal>,
}

impl Fixture {
    /// A 64 by 64 map on a runtime that denies every resource request.
    pub fn new() -> Self {
        Self::with_map_options(MapOptions {
            initial_extent: LogicalExtent::new(64, 64, 1.0),
            ..Default::default()
        })
    }

    pub fn with_map_options(map_options: MapOptions) -> Self {
        let events = Arc::new(Signal::default());
        let runtime = runtime_create(&RuntimeOptions {
            event_wake: events.native_wake(),
            ..Default::default()
        })
        .unwrap();
        wait_for(runtime.set_resource_provider(denying_provider()));
        let map = wait_for(runtime.map_create(&map_options));
        Self {
            runtime: Some(Arc::new(runtime)),
            map: Some(map),
            events,
        }
    }

    pub fn runtime(&self) -> &Arc<RuntimeHandle> {
        self.runtime.as_ref().unwrap()
    }

    pub fn map(&self) -> &MapHandle {
        self.map.as_ref().unwrap()
    }

    /// Hands the runtime and map to a test that ends their lives itself.
    pub fn into_parts(mut self) -> (Arc<RuntimeHandle>, MapHandle) {
        (self.runtime.take().unwrap(), self.map.take().unwrap())
    }

    /// Blocks until every runtime submission accepted so far has finished.
    pub fn barrier(&self) {
        wait_for(self.runtime().barrier());
    }

    /// Drains events until one from this fixture's map matches, and returns it.
    pub fn await_event(
        &self,
        what: &str,
        mut matches: impl FnMut(&RuntimeEvent) -> bool,
    ) -> RuntimeEvent {
        let deadline = Instant::now() + timeout();
        let source = self.map().id();
        loop {
            let seen = self.events.seen();
            let batch = self.runtime().drain_events().unwrap();
            if let Some(event) = batch
                .get()
                .unwrap()
                .events
                .into_iter()
                .find(|event| event.source == source && matches(event))
            {
                return event;
            }
            self.events.wait_past(seen, deadline, what);
        }
    }

    pub fn await_event_type(&self, event_type: RuntimeEventType) -> RuntimeEvent {
        self.await_event(&format!("{event_type:?}"), |event| {
            event.r#type == event_type
        })
    }

    /// Publishes a new map update for the next frame to render.
    pub fn repaint(&self) {
        wait_for(self.map().request_repaint());
        self.barrier();
    }

    /// Loads a style document and waits until it has loaded.
    pub fn load_style(&self, style_json: &str) {
        let loaded = self.map().set_style_json(style_json.as_bytes()).unwrap();
        self.await_event_type(RuntimeEventType::MapStyleLoaded);
        assert_eq!(
            wait_for(Ok(loaded)).disposition,
            CommandDisposition::Committed
        );
    }
}

impl Drop for Fixture {
    fn drop(&mut self) {
        // A failed test still releases what it made. Waiting on a release
        // that fails would hide the test's own panic, so failures here are
        // reported only when the test passed.
        let panicking = std::thread::panicking();
        let release = |future: Result<NativeFuture<()>>| match future {
            Ok(future) => {
                if !future.wait(timeout()).unwrap_or(false) && !panicking {
                    panic!("fixture release never completed");
                }
                if let Err(error) = future.take()
                    && !panicking
                {
                    panic!("fixture release failed: {error}");
                }
            }
            Err(error) if !panicking => panic!("fixture release was rejected: {error}"),
            Err(_) => {}
        };
        if let Some(map) = self.map.take() {
            release(map.release());
        }
        if let Some(runtime) = self.runtime.take() {
            release(runtime.release());
        }
    }
}

/// A frame demand token with its high bits set, so a result that carried a
/// truncated token would match no demand.
pub fn next_token() -> u64 {
    static NEXT: AtomicU64 = AtomicU64::new(0xfedc_0000_0000_0001);
    NEXT.fetch_add(1, Ordering::Relaxed)
}

/// A render session together with the wake that its frame results and
/// driver work raise.
pub struct Session {
    pub handle: RenderSessionHandle,
    pub wakes: Arc<Signal>,
}

impl Session {
    /// Attach options that raise this session's wake, for `driver`.
    pub fn attach_options(driver: RenderDriverKind) -> (RenderSessionAttachOptions, Arc<Signal>) {
        let wakes = Arc::new(Signal::default());
        let options = RenderSessionAttachOptions {
            driver,
            requested_texture_ring_depth: 2,
            frame_wake: wakes.native_wake(),
            driver_work_wake: wakes.native_wake(),
        };
        (options, wakes)
    }

    /// Completes an attachment, servicing driver work while it is pending.
    pub fn finish_attach(
        (handle, attached): (RenderSessionHandle, NativeFuture<()>),
        wakes: Arc<Signal>,
    ) -> Self {
        let session = Self { handle, wakes };
        session.drive(attached).expect("the attachment failed");
        session
    }

    /// Awaits one session operation, servicing driver work while it is
    /// pending, as the host's graphics thread would.
    pub fn drive<T>(&self, operation: NativeFuture<T>) -> Result<T> {
        drive(&self.wakes, operation, || self.service())
    }

    fn service(&self) -> usize {
        // A core worker drives itself and rejects service calls.
        self.handle.service_driver_work(64).unwrap_or(0)
    }

    /// Demands one frame and returns its result.
    pub fn render_frame(&self) -> RenderFrameResult {
        let token = next_token();
        self.handle
            .request_frame(&FrameDemand {
                token,
                ..Default::default()
            })
            .unwrap();
        let deadline = Instant::now() + timeout();
        loop {
            let seen = self.wakes.seen();
            self.service();
            match self.handle.drain_frame_results() {
                Ok(batch) => {
                    let found = (0..batch.count().unwrap())
                        .map(|index| batch.get(index).unwrap())
                        .find(|result| result.token == token);
                    if let Some(result) = found {
                        return result;
                    }
                }
                Err(error) if error.kind() == ErrorKind::NotReady => {}
                Err(error) => panic!("draining frame results failed: {error}"),
            }
            self.wakes.wait_past(seen, deadline, "a frame result");
        }
    }

    /// Reads the newest frame back as premultiplied RGBA8.
    pub fn read_back(&self) -> TextureReadbackResult {
        self.drive(self.handle.texture_read_premultiplied_rgba8().unwrap())
            .unwrap()
    }

    /// Detaches and destroys the session.
    pub fn close(self) {
        self.drive(self.handle.detach().unwrap()).unwrap();
        self.handle.destroy().unwrap();
    }
}
