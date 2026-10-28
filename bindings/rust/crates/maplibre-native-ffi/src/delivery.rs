//! Queue delivery: native copies each completion and callback into a
//! [`DeliveryQueue`], and the host runs them on a thread it chooses.
//!
//! A queue is the one owner of the host handlers that name it. A call made
//! through an owner's `queued` view stores its completion handler, and the
//! handler of each registration it carries, in the queue before it enters C,
//! since native may queue a delivery before the call returns. Native carries
//! only the handler's id, and a drained [`DeliveryBatch`] resolves the ids
//! that it holds, so only the queue that stored a handler can run it.
//!
//! Every handler reaches exactly one end:
//!
//! - A rejected call drops its handlers before it returns, unrun.
//! - A completion handler runs once, with its result, or with
//!   [`Status::Cancelled`] when its queue closes before the result arrives or
//!   its delivery is dropped unrun.
//! - A registration's handler runs for each of its deliveries, and drops once
//!   its `RETIRED` delivery has run and no batch still holds it.
//! - Closing a queue, by [`DeliveryQueue::close`] or by dropping it, drops the
//!   handlers of registrations that haven't retired once no batch holds them.
//!   Those registrations keep working as `delivery.h` describes, without the
//!   host. A later call through the queue fails with [`Status::InvalidState`]
//!   before it reaches native. A call that native accepts while the queue
//!   closes ends as if the queue had closed after it.
//!
//! [`Dispatcher`] drains a queue on a thread of its own and runs `Send`
//! handlers there. [`LocalDispatcher`] runs handlers that need not be `Send`
//! on the one thread that owns it. A registration's handler is its record's
//! [`Callback`]s: a record of kind [`Shared`], which callback mode and a
//! shared queue take, holds `Send` and `Sync` callbacks, and one of kind
//! [`Local`], built by its `local` constructor, holds callbacks that need not
//! be either. A synchronous round trip through a `Dispatcher` takes one thread
//! hop more than callback mode or a `LocalDispatcher` drained by the waiting
//! thread.

use std::collections::HashMap;
use std::ffi::c_void;
use std::marker::PhantomData;
use std::panic::{AssertUnwindSafe, catch_unwind};
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::{Arc, Condvar, Mutex};
use std::thread::{JoinHandle, ThreadId};
use std::time::Duration;

use maplibre_native_ffi_sys as sys;

use crate::call::Call;
use crate::completion::{self, CommandCompletion};
use crate::convert::{
    Handler, InputArena, LocalRegistrationHandler, QueueSink, RegistrationHandler,
};
use crate::handle::lock;
use crate::{Error, Result, Status, TextureImageInfo};

/// Whether a queue's handlers must be `Send`: [`Shared`] for a queue that
/// any thread drains, and [`Local`] for one that only the thread that owns it
/// drains.
pub trait QueueKind: private::Sealed + Clone + Copy + Default + std::fmt::Debug + 'static {}

/// A queue that any thread may drain.
#[derive(Debug, Clone, Copy, Default)]
pub struct Shared;

/// A queue that only the thread that owns it drains.
#[derive(Debug, Clone, Copy, Default)]
pub struct Local;

impl QueueKind for Shared {}
impl QueueKind for Local {}

mod private {
    use super::*;

    /// A batch's native handle and view, which [`super::BatchBytes`] and
    /// taken deliveries keep alive.
    pub struct BatchOwner {
        pub(super) raw: sys::mln_delivery_batch,
        pub(super) view: sys::mln_delivery_batch_view,
        pub(super) native: &'static NativeQueue,
    }

    pub trait Sealed {
        /// A completion handler, erased.
        type Completion: ?Sized;
        /// A registration's handler.
        type Registration: Clone;

        fn complete(
            handler: Box<Self::Completion>,
            result: Result<(&sys::mln_completion_result, &Arc<BatchOwner>)>,
        );

        /// Whether a registration of this kind needs a queue: its callbacks
        /// can't run on native threads.
        const LOCAL: bool;

        /// The handler that a queue of this kind holds, or `None` for one
        /// it can't run.
        fn registration(handler: Handler) -> Option<Self::Registration>;

        /// Erases a registration's handler.
        ///
        /// # Safety
        ///
        /// For [`Shared`], `deliver` must be `Send` and `Sync`.
        unsafe fn handler(
            deliver: impl Fn(*const c_void, &dyn Fn() -> Result<()>) + 'static,
        ) -> Handler;

        fn deliver(
            handler: &Self::Registration,
            delivery: *const c_void,
            take: &dyn Fn() -> Result<()>,
        );
    }

    impl Sealed for Shared {
        type Completion =
            dyn FnOnce(Result<(&sys::mln_completion_result, &Arc<BatchOwner>)>) + Send;
        type Registration = RegistrationHandler;

        fn complete(
            handler: Box<Self::Completion>,
            result: Result<(&sys::mln_completion_result, &Arc<BatchOwner>)>,
        ) {
            handler(result)
        }

        const LOCAL: bool = false;

        unsafe fn handler(
            deliver: impl Fn(*const c_void, &dyn Fn() -> Result<()>) + 'static,
        ) -> Handler {
            /// A handler that the caller vouches is `Send` and `Sync`.
            struct Vouched<F>(F);
            // SAFETY: the caller guarantees it.
            unsafe impl<F> Send for Vouched<F> {}
            // SAFETY: as above.
            unsafe impl<F> Sync for Vouched<F> {}
            impl<F: Fn(*const c_void, &dyn Fn() -> Result<()>)> Vouched<F> {
                fn run(&self, delivery: *const c_void, take: &dyn Fn() -> Result<()>) {
                    (self.0)(delivery, take)
                }
            }
            let deliver = Vouched(deliver);
            Handler::Shared(Arc::new(move |delivery, take| deliver.run(delivery, take)))
        }

        fn registration(handler: Handler) -> Option<RegistrationHandler> {
            match handler {
                Handler::Shared(handler) => Some(handler),
                Handler::Local(_) => None,
            }
        }

        fn deliver(
            handler: &RegistrationHandler,
            delivery: *const c_void,
            take: &dyn Fn() -> Result<()>,
        ) {
            handler(delivery, take)
        }
    }

    impl Sealed for Local {
        type Completion = dyn FnOnce(Result<(&sys::mln_completion_result, &Arc<BatchOwner>)>);
        type Registration = LocalRegistrationHandler;

        fn complete(
            handler: Box<Self::Completion>,
            result: Result<(&sys::mln_completion_result, &Arc<BatchOwner>)>,
        ) {
            handler(result)
        }

        const LOCAL: bool = true;

        unsafe fn handler(
            deliver: impl Fn(*const c_void, &dyn Fn() -> Result<()>) + 'static,
        ) -> Handler {
            Handler::Local(Arc::new(deliver))
        }

        fn registration(handler: Handler) -> Option<LocalRegistrationHandler> {
            Some(match handler {
                Handler::Shared(handler) => handler,
                Handler::Local(handler) => handler,
            })
        }

        fn deliver(
            handler: &LocalRegistrationHandler,
            delivery: *const c_void,
            take: &dyn Fn() -> Result<()>,
        ) {
            handler(delivery, take)
        }
    }

    /// A value that native copies into a completion's result.
    pub trait CopyFrom<N>: Sized {
        fn copy(result: &sys::mln_completion_result) -> Result<Self>;
        fn copy_optional(result: &sys::mln_completion_result) -> Result<Option<Self>>;
        fn copy_list(result: &sys::mln_completion_result) -> Result<Vec<Self>>;
    }

    impl<N: Copy, T: crate::convert::FromNative<N>> CopyFrom<N> for T {
        fn copy(result: &sys::mln_completion_result) -> Result<T> {
            completion::value::<N, T>(result)
        }
        fn copy_optional(result: &sys::mln_completion_result) -> Result<Option<T>> {
            completion::optional::<N, T>(result)
        }
        fn copy_list(result: &sys::mln_completion_result) -> Result<Vec<T>> {
            completion::list::<N, T>(result)
        }
    }
}

use private::BatchOwner;

/// Whether a registration of kind `K` needs a queue.
pub(crate) fn local<K: QueueKind>() -> bool {
    K::LOCAL
}

/// Erases the handler of a registration of kind `K`, which runs its record's
/// callbacks for a delivery.
///
/// # Safety
///
/// For [`Shared`], `deliver` must be `Send` and `Sync`.
pub(crate) unsafe fn registration<K: QueueKind>(
    deliver: impl Fn(*const c_void, &dyn Fn() -> Result<()>) + 'static,
) -> Handler {
    // SAFETY: the caller guarantees it.
    unsafe { K::handler(deliver) }
}

/// A registration record's callback. A record of kind [`Shared`] holds one
/// that is `Send` and `Sync`, which native threads or any thread that drains
/// its queue may run. One of kind [`Local`] holds one that need not be either,
/// which only a [`LocalDispatcher`]'s thread runs.
pub struct Callback<K: QueueKind, F: ?Sized> {
    callback: Arc<F>,
    kind: PhantomData<K>,
}

// SAFETY: `shared` builds a shared callback only from a `Send` and `Sync`
// closure.
unsafe impl<F: ?Sized> Send for Callback<Shared, F> {}
// SAFETY: as above.
unsafe impl<F: ?Sized> Sync for Callback<Shared, F> {}

impl<F: ?Sized> Callback<Shared, F> {
    /// Erases a `Send` and `Sync` closure; `erase` only coerces it.
    pub(crate) fn shared<C: Send + Sync + 'static>(
        callback: C,
        erase: fn(Arc<C>) -> Arc<F>,
    ) -> Self {
        Self {
            callback: erase(Arc::new(callback)),
            kind: PhantomData,
        }
    }
}

impl<F: ?Sized> Callback<Local, F> {
    /// Erases a closure; `erase` only coerces it.
    pub(crate) fn local<C: 'static>(callback: C, erase: fn(Arc<C>) -> Arc<F>) -> Self {
        Self {
            callback: erase(Arc::new(callback)),
            kind: PhantomData,
        }
    }
}

impl<K: QueueKind, F: ?Sized> Clone for Callback<K, F> {
    fn clone(&self) -> Self {
        Self {
            callback: Arc::clone(&self.callback),
            kind: PhantomData,
        }
    }
}

impl<K: QueueKind, F: ?Sized> std::ops::Deref for Callback<K, F> {
    type Target = F;

    fn deref(&self) -> &F {
        &self.callback
    }
}

impl<K: QueueKind, F: ?Sized> std::fmt::Debug for Callback<K, F> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("Callback").finish_non_exhaustive()
    }
}

/// What a queued completion lends its handler. Each operation's queued form
/// names it; a handler only names the [`QueuedResult`] it receives.
pub trait QueuedValue: 'static {
    /// The value, which may borrow the batch for `'b`.
    type View<'b>;
    #[doc(hidden)]
    fn view<'b>(
        result: &'b sys::mln_completion_result,
        batch: &'b Arc<BatchOwner>,
    ) -> Result<Self::View<'b>>;
}

/// A completion with no value.
#[derive(Debug)]
pub enum Unit {}

impl QueuedValue for Unit {
    type View<'b> = ();
    fn view<'b>(result: &'b sys::mln_completion_result, _: &'b Arc<BatchOwner>) -> Result<()> {
        completion::unit(result)
    }
}

/// A command's completion.
#[derive(Debug)]
pub enum Command {}

impl QueuedValue for Command {
    type View<'b> = CommandCompletion;
    fn view<'b>(
        result: &'b sys::mln_completion_result,
        _: &'b Arc<BatchOwner>,
    ) -> Result<CommandCompletion> {
        completion::command(result)
    }
}

/// A result of C type `N` that the handler receives as `T`, an owned copy.
pub struct Copied<N, T>(PhantomData<fn() -> (N, T)>);

impl<N: 'static, T: private::CopyFrom<N> + 'static> QueuedValue for Copied<N, T> {
    type View<'b> = T;
    fn view<'b>(result: &'b sys::mln_completion_result, _: &'b Arc<BatchOwner>) -> Result<T> {
        T::copy(result)
    }
}

/// An optional result, copied.
pub struct CopiedOptional<N, T>(PhantomData<fn() -> (N, T)>);

impl<N: 'static, T: private::CopyFrom<N> + 'static> QueuedValue for CopiedOptional<N, T> {
    type View<'b> = Option<T>;
    fn view<'b>(
        result: &'b sys::mln_completion_result,
        _: &'b Arc<BatchOwner>,
    ) -> Result<Option<T>> {
        T::copy_optional(result)
    }
}

/// A list result, copied.
pub struct CopiedList<N, T>(PhantomData<fn() -> (N, T)>);

impl<N: 'static, T: private::CopyFrom<N> + 'static> QueuedValue for CopiedList<N, T> {
    type View<'b> = Vec<T>;
    fn view<'b>(result: &'b sys::mln_completion_result, _: &'b Arc<BatchOwner>) -> Result<Vec<T>> {
        T::copy_list(result)
    }
}

/// A texture readback whose pixels the handler reads from its batch.
#[derive(Debug)]
pub enum Readback {}

impl QueuedValue for Readback {
    type View<'b> = TextureReadbackView<'b>;
    fn view<'b>(
        result: &'b sys::mln_completion_result,
        batch: &'b Arc<BatchOwner>,
    ) -> Result<TextureReadbackView<'b>> {
        let value = completion::copy_value::<sys::mln_texture_readback_result>(result)?;
        // SAFETY: the batch owns the pixels until it is released, and `batch`
        // keeps it for 'b.
        let data = unsafe { crate::ptr::slice(value.data.data.cast::<u8>(), value.data.size) }?;
        Ok(TextureReadbackView {
            data,
            // SAFETY: the record holds no pointers.
            info: unsafe { crate::convert::from_native(value.info) }?,
            batch,
        })
    }
}

/// A queued texture readback, whose pixels stay in the batch they arrived
/// in, so reading them copies nothing.
#[derive(Clone, Copy)]
pub struct TextureReadbackView<'b> {
    data: &'b [u8],
    pub info: TextureImageInfo,
    batch: &'b Arc<BatchOwner>,
}

impl std::fmt::Debug for TextureReadbackView<'_> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("TextureReadbackView")
            .field("len", &self.data.len())
            .field("info", &self.info)
            .finish()
    }
}

impl<'b> TextureReadbackView<'b> {
    /// Pixel bytes, valid while the batch lives.
    pub fn data(&self) -> &'b [u8] {
        self.data
    }

    /// Keeps the pixels past the handler by keeping their batch, without a
    /// copy.
    pub fn keep(&self) -> BatchBytes {
        BatchBytes {
            _batch: Arc::clone(self.batch),
            data: self.data.as_ptr(),
            len: self.data.len(),
        }
    }
}

/// Bytes that a delivery batch owns, which keep the batch alive until they
/// drop.
pub struct BatchBytes {
    _batch: Arc<BatchOwner>,
    data: *const u8,
    len: usize,
}

// SAFETY: the bytes are immutable and owned by the kept batch, which any
// thread may release.
unsafe impl Send for BatchBytes {}
// SAFETY: as above.
unsafe impl Sync for BatchBytes {}

impl std::ops::Deref for BatchBytes {
    type Target = [u8];

    fn deref(&self) -> &[u8] {
        // SAFETY: the kept batch owns len bytes at data.
        unsafe { std::slice::from_raw_parts(self.data, self.len) }
    }
}

impl AsRef<[u8]> for BatchBytes {
    fn as_ref(&self) -> &[u8] {
        self
    }
}

impl std::fmt::Debug for BatchBytes {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("BatchBytes")
            .field("len", &self.len)
            .finish()
    }
}

/// The result that a queued completion lends its handler until it returns:
/// the value, or the call's failure with its status, diagnostic, and
/// generation.
pub type QueuedResult<'b, V> = Result<<V as QueuedValue>::View<'b>>;

fn queued_view<'b, V: QueuedValue>(
    result: Result<(&'b sys::mln_completion_result, &'b Arc<BatchOwner>)>,
) -> QueuedResult<'b, V> {
    let (raw, batch) = result?;
    if raw.status != sys::MLN_STATUS_OK {
        return Err(completion::failure(raw));
    }
    V::view(raw, batch)
}

/// A completion handler that a queue of kind `K` holds: a closure that
/// receives the [`QueuedResult`], and is `Send` for a [`Shared`] queue.
pub trait CompletionHandler<K: QueueKind, V: QueuedValue>:
    for<'b> FnOnce(QueuedResult<'b, V>) + 'static
{
    #[doc(hidden)]
    fn erase(self) -> Box<<K as private::Sealed>::Completion>;
}

impl<V: QueuedValue, F> CompletionHandler<Shared, V> for F
where
    F: for<'b> FnOnce(QueuedResult<'b, V>) + Send + 'static,
{
    fn erase(self) -> Box<<Shared as private::Sealed>::Completion> {
        Box::new(move |result| self(queued_view::<V>(result)))
    }
}

impl<V: QueuedValue, F> CompletionHandler<Local, V> for F
where
    F: for<'b> FnOnce(QueuedResult<'b, V>) + 'static,
{
    fn erase(self) -> Box<<Local as private::Sealed>::Completion> {
        Box::new(move |result| self(queued_view::<V>(result)))
    }
}

/// The native queue functions, which tests replace with a scripted queue.
pub(crate) struct NativeQueue {
    pub(crate) drain: unsafe extern "C" fn(
        sys::mln_delivery_queue,
        *mut sys::mln_delivery_batch,
        *mut sys::mln_delivery_batch_view,
        *mut sys::mln_diagnostic,
    ) -> sys::mln_status,
    pub(crate) take: unsafe extern "C" fn(
        sys::mln_delivery_batch,
        usize,
        *mut sys::mln_diagnostic,
    ) -> sys::mln_status,
    pub(crate) release_batch: unsafe extern "C" fn(sys::mln_delivery_batch),
    pub(crate) release: unsafe extern "C" fn(sys::mln_delivery_queue),
}

static NATIVE: NativeQueue = NativeQueue {
    drain: sys::mln_delivery_queue_drain,
    take: sys::mln_delivery_batch_take,
    release_batch: sys::mln_delivery_batch_release,
    release: sys::mln_delivery_queue_release,
};

enum Entry<K: QueueKind> {
    Completion(Box<<K as private::Sealed>::Completion>),
    Registration(<K as private::Sealed>::Registration),
}

struct Registry<K: QueueKind> {
    entries: HashMap<usize, Entry<K>>,
    /// The handlers whose call is inside native, which a close leaves to the
    /// call: native may still reject it, and then its handlers drop unrun.
    submitting: std::collections::HashSet<usize>,
    next: usize,
    closed: bool,
}

/// A native delivery queue and the host handlers that name it.
///
/// `K` is [`Shared`] for a queue that any thread may drain, which is `Send`
/// and `Sync`, and [`Local`] for one that only the thread that created it
/// drains, whose completion handlers need not be `Send`.
pub struct DeliveryQueue<K: QueueKind = Shared> {
    raw: sys::mln_delivery_queue,
    native: &'static NativeQueue,
    registry: Mutex<Registry<K>>,
}

impl<K: QueueKind> std::fmt::Debug for DeliveryQueue<K> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "DeliveryQueue(id={:#x})", self.raw.0)
    }
}

impl<K: QueueKind> DeliveryQueue<K> {
    /// Creates a queue whose `wake` runs when it goes from empty to nonempty.
    /// A wake may run on any thread, including inside a native call, so it
    /// only schedules a drain. Without one, the host drains on its own
    /// schedule.
    ///
    /// See `mln_delivery_queue_create` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/delivery_8h.html).
    pub fn new(wake: Option<crate::Wake>) -> Result<Self> {
        crate::abi::validate_abi_version()?;
        let mut arena = InputArena::default();
        let options = sys::mln_delivery_queue_options {
            size: std::mem::size_of::<sys::mln_delivery_queue_options>() as u32,
            wake: crate::convert::to_native(&wake.unwrap_or_default(), &mut arena)?,
        };
        let mut raw = sys::mln_delivery_queue(0);
        // SAFETY: the options and the output are valid for the call.
        crate::error::check(|diagnostic| unsafe {
            sys::mln_delivery_queue_create(&options, &mut raw, diagnostic)
        })?;
        arena.accept_registrations();
        Ok(Self::adopt(raw, &NATIVE))
    }

    fn adopt(raw: sys::mln_delivery_queue, native: &'static NativeQueue) -> Self {
        Self {
            raw,
            native,
            registry: Mutex::new(Registry {
                entries: HashMap::new(),
                submitting: std::collections::HashSet::new(),
                next: 1,
                closed: false,
            }),
        }
    }

    /// Returns the native handle value.
    pub fn id(&self) -> u64 {
        self.raw.0
    }

    /// Moves every queued delivery into a new batch, or returns `None` when
    /// nothing is queued. Any thread may drain a shared queue, and each
    /// delivery enters one batch.
    ///
    /// See `mln_delivery_queue_drain` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/delivery_8h.html).
    pub fn drain(&self) -> Result<Option<DeliveryBatch<K>>> {
        // A batch resolves its handlers in drain order, so a registration's
        // RETIRED can't overtake a delivery drained before it.
        let mut registry = lock(&self.registry);
        if registry.closed {
            return Err(closed());
        }
        let mut raw = sys::mln_delivery_batch(0);
        // SAFETY: all zeroes is a valid empty view.
        let mut view: sys::mln_delivery_batch_view = unsafe { std::mem::zeroed() };
        view.size = std::mem::size_of::<sys::mln_delivery_batch_view>() as u32;
        let mut ready = true;
        crate::error::check(|diagnostic| {
            // SAFETY: the outputs are valid for the call.
            let status = unsafe { (self.native.drain)(self.raw, &mut raw, &mut view, diagnostic) };
            if status == sys::MLN_STATUS_NOT_READY {
                ready = false;
                return sys::MLN_STATUS_OK;
            }
            status
        })?;
        if !ready {
            return Ok(None);
        }
        let owner = Arc::new(BatchOwner {
            raw,
            view,
            native: self.native,
        });
        let slots = (0..view.delivery_count)
            .map(|index| match owner.delivery(index) {
                Ok(delivery) => registry.resolve(delivery),
                Err(_) => Slot::EMPTY,
            })
            .collect();
        Ok(Some(DeliveryBatch { owner, slots }))
    }

    /// Closes the queue: native discards what is queued and refuses later
    /// deliveries. Every completion that hasn't been drained runs its handler
    /// with [`Status::Cancelled`], each registration's handler drops once no
    /// batch holds it, and later drains and calls through the queue fail with
    /// [`Status::InvalidState`]. Batches already drained stay readable. A
    /// second close does nothing.
    ///
    /// Dropping the queue closes it.
    ///
    /// See `mln_delivery_queue_release` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/delivery_8h.html).
    pub fn close(&self) {
        let entries = {
            let mut registry = lock(&self.registry);
            if registry.closed {
                return;
            }
            registry.closed = true;
            // Released under the lock, so no drain can use the handle after.
            // SAFETY: the queue releases its handle once.
            unsafe { (self.native.release)(self.raw) };
            let submitting = std::mem::take(&mut registry.submitting);
            let (left, taken) = std::mem::take(&mut registry.entries)
                .into_iter()
                .partition(|(id, _)| submitting.contains(id));
            registry.entries = left;
            registry.submitting = submitting;
            taken
        };
        // Handlers run and drop outside the lock, since they may call in.
        end(entries);
    }

    /// Submits one queue-mode completion: `submit` passes native the
    /// completion that names this queue, after the handler is stored.
    pub(crate) fn enqueue<H, V>(
        &self,
        mut call: Call<'_, H>,
        handler: impl CompletionHandler<K, V>,
        submit: impl FnOnce(H, *const sys::mln_completion, *mut sys::mln_diagnostic) -> sys::mln_status,
    ) -> Result<()>
    where
        H: sys::NativeHandle,
        V: QueuedValue,
    {
        let token = self.hold(Entry::Completion(handler.erase()))?;
        let completion = sys::mln_completion {
            size: std::mem::size_of::<sys::mln_completion>() as u32,
            callback: None,
            user_data: token as *mut c_void,
            release_user_data: None,
            queue: self.raw,
        };
        if let Err(error) =
            call.status(|native, diagnostic| submit(native, &completion, diagnostic))
        {
            // A rejected submission queues nothing, so its handler is ours.
            self.forget(token);
            return Err(error);
        }
        self.settle(token);
        Ok(())
    }

    /// Submits one queue-mode command without a completion. Only its
    /// registrations name this queue, and it fails once the queue is closed,
    /// as a completing call does.
    pub(crate) fn discard<H: sys::NativeHandle>(
        &self,
        call: Call<'_, H>,
        submit: impl FnOnce(H, *const sys::mln_completion, *mut sys::mln_diagnostic) -> sys::mln_status,
    ) -> Result<()> {
        if lock(&self.registry).closed {
            return Err(closed());
        }
        call.discard(submit)
    }

    /// Stores a handler for a call about to enter native.
    fn hold(&self, entry: Entry<K>) -> Result<usize> {
        let mut registry = lock(&self.registry);
        if registry.closed {
            return Err(closed());
        }
        let id = registry.next;
        registry.next += 1;
        registry.entries.insert(id, entry);
        registry.submitting.insert(id);
        Ok(id)
    }

    /// Native accepted the call, so the queue keeps its handler, or ends it
    /// now when the queue closed during the call.
    fn settle(&self, id: usize) {
        let ended = {
            let mut registry = lock(&self.registry);
            registry.submitting.remove(&id);
            if registry.closed {
                registry.entries.remove(&id)
            } else {
                None
            }
        };
        end(ended.map(|entry| (id, entry)));
    }

    /// Native rejected the call, so its handler drops unrun.
    fn forget(&self, id: usize) {
        let entry = {
            let mut registry = lock(&self.registry);
            registry.submitting.remove(&id);
            registry.entries.remove(&id)
        };
        // The handler drops outside the lock, since dropping it may call in.
        drop(entry);
    }
}

fn closed() -> Error {
    Error::new(Status::InvalidState, "delivery queue is closed")
}

/// Ends handlers that a closed queue will never deliver to: a completion
/// runs with [`Status::Cancelled`], and a registration's handler drops.
fn end<K: QueueKind>(entries: impl IntoIterator<Item = (usize, Entry<K>)>) {
    for (_, entry) in entries {
        match entry {
            Entry::Completion(handler) => cancel::<K>(handler, "delivery queue was closed"),
            Entry::Registration(handler) => contain("mln_user_data_release", || drop(handler)),
        }
    }
}

impl<K: QueueKind> QueueSink for DeliveryQueue<K> {
    fn hold(&self, handler: Handler) -> Result<(sys::mln_delivery_queue, *mut c_void)> {
        let handler = K::registration(handler).ok_or_else(|| {
            Error::invalid_argument("a local registration needs a local delivery queue")
        })?;
        let id = DeliveryQueue::hold(self, Entry::Registration(handler))?;
        Ok((self.raw, id as *mut c_void))
    }

    fn accept(&self, user_data: *mut c_void) {
        self.settle(user_data as usize);
    }

    fn forget(&self, user_data: *mut c_void) {
        DeliveryQueue::forget(self, user_data as usize);
    }
}

impl<K: QueueKind> Registry<K> {
    /// The handler that a drained delivery names, which a completion and a
    /// registration's RETIRED take out of the registry.
    fn resolve(&mut self, delivery: &sys::mln_delivery) -> Slot<K> {
        let id = delivery.user_data as usize;
        let runs = match delivery.type_ {
            sys::MLN_DELIVERY_TYPE_COMPLETION => match self.entries.remove(&id) {
                Some(Entry::Completion(handler)) => Some(Runs::Completion(handler)),
                Some(other) => {
                    self.entries.insert(id, other);
                    None
                }
                None => None,
            },
            sys::MLN_DELIVERY_TYPE_RETIRED => match self.entries.remove(&id) {
                Some(Entry::Registration(handler)) => Some(Runs::Retired(handler)),
                Some(other) => {
                    self.entries.insert(id, other);
                    None
                }
                None => None,
            },
            _ => match self.entries.get(&id) {
                Some(Entry::Registration(handler)) => Some(Runs::Registration(handler.clone())),
                _ => None,
            },
        };
        Slot(runs)
    }
}

impl<K: QueueKind> Drop for DeliveryQueue<K> {
    fn drop(&mut self) {
        self.close();
    }
}

/// Runs a completion handler whose result will never arrive.
fn cancel<K: QueueKind>(handler: Box<<K as private::Sealed>::Completion>, why: &str) {
    let error = Error::new(Status::Cancelled, why);
    contain("mln_completion_callback", || {
        K::complete(handler, Err(error))
    });
}

/// Runs host code outside a native callback, reporting a panic.
fn contain(callback: &'static str, body: impl FnOnce()) {
    if let Err(payload) = catch_unwind(AssertUnwindSafe(body)) {
        // No native frame is below, so a payload that panics as it drops
        // unwinds only into this second catch.
        let _ = catch_unwind(AssertUnwindSafe(|| drop(payload)));
        crate::report::report(crate::report::Report::CallbackError {
            callback,
            error: Error::new(Status::NativeError, "callback panicked"),
        });
    }
}

/// What one drained delivery runs, which a delivery that ran or moved out
/// of its batch leaves empty.
struct Slot<K: QueueKind>(Option<Runs<K>>);

enum Runs<K: QueueKind> {
    Completion(Box<<K as private::Sealed>::Completion>),
    Registration(<K as private::Sealed>::Registration),
    Retired(<K as private::Sealed>::Registration),
}

impl<K: QueueKind> Slot<K> {
    const EMPTY: Self = Self(None);

    fn kind(&self) -> Option<DeliveryKind> {
        self.0.as_ref().map(|runs| match runs {
            Runs::Completion(_) => DeliveryKind::Completion,
            Runs::Registration(_) => DeliveryKind::Registration,
            Runs::Retired(_) => DeliveryKind::Retired,
        })
    }

    fn run(mut self, batch: &Arc<BatchOwner>, index: usize) {
        let Some(runs) = self.0.take() else {
            return;
        };
        let Ok(delivery) = batch.delivery(index) else {
            // An unreadable delivery ends as an unrun one does.
            drop(Self(Some(runs)));
            return;
        };
        match runs {
            Runs::Completion(handler) => {
                // SAFETY: the type selects the payload member.
                let result = unsafe { &delivery.payload.completion };
                contain("mln_completion_callback", || {
                    K::complete(handler, Ok((result, batch)))
                });
            }
            Runs::Registration(handler) => {
                let take = || batch.take(index);
                let delivery = std::ptr::from_ref(delivery).cast();
                contain("mln_delivery", || K::deliver(&handler, delivery, &take));
            }
            Runs::Retired(handler) => contain("mln_user_data_release", || drop(handler)),
        }
    }
}

impl<K: QueueKind> Drop for Slot<K> {
    fn drop(&mut self) {
        match self.0.take() {
            Some(Runs::Completion(handler)) => cancel::<K>(handler, "delivery was dropped unrun"),
            Some(Runs::Retired(handler)) => contain("mln_user_data_release", || drop(handler)),
            Some(Runs::Registration(_)) | None => {}
        }
    }
}

impl BatchOwner {
    fn delivery(&self, index: usize) -> Result<&sys::mln_delivery> {
        let view = &self.view;
        let stride = view.delivery_size as usize;
        if index >= view.delivery_count
            || view.deliveries.is_null()
            || stride < std::mem::size_of::<sys::mln_delivery>()
            || index > isize::MAX as usize / stride
        {
            return Err(Error::new(
                Status::NativeError,
                "invalid delivery batch view",
            ));
        }
        // SAFETY: native lays out delivery_count deliveries delivery_size
        // bytes apart, valid until the batch is released.
        Ok(unsafe { &*view.deliveries.cast::<u8>().add(index * stride).cast() })
    }

    /// Transfers a delivery's owned handles to the binding.
    fn take(&self, index: usize) -> Result<()> {
        // SAFETY: the batch is live while self is.
        crate::error::check(|diagnostic| unsafe { (self.native.take)(self.raw, index, diagnostic) })
    }
}

impl Drop for BatchOwner {
    fn drop(&mut self) {
        // SAFETY: the drain published the batch to this owner alone.
        unsafe { (self.native.release_batch)(self.raw) };
    }
}

// SAFETY: the view is immutable and valid until the batch is released, which
// any thread may do.
unsafe impl Send for BatchOwner {}
// SAFETY: as above.
unsafe impl Sync for BatchOwner {}

/// The kind of one drained delivery.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum DeliveryKind {
    /// A completion's result, which runs its handler.
    Completion,
    /// A registration's callback, which runs its record's callback.
    Registration,
    /// A registration's last delivery, which drops its handler.
    Retired,
}

/// One batch of deliveries that a drain moved out of its queue.
///
/// The batch owns its deliveries' payloads, which their views borrow.
/// [`DeliveryBatch::dispatch`] runs them in push order. A host that runs
/// some elsewhere takes them out of the batch through
/// [`DeliveryBatch::deliveries`]. A delivery still in the batch when it
/// drops ends as an unrun one does: a completion's handler runs with
/// [`Status::Cancelled`], an untaken resource request fails, and an untaken
/// transform keeps its URL.
pub struct DeliveryBatch<K: QueueKind = Shared> {
    owner: Arc<BatchOwner>,
    slots: Vec<Slot<K>>,
}

impl<K: QueueKind> std::fmt::Debug for DeliveryBatch<K> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("DeliveryBatch")
            .field("len", &self.len())
            .finish()
    }
}

impl<K: QueueKind> DeliveryBatch<K> {
    /// The number of deliveries.
    pub fn len(&self) -> usize {
        self.slots.len()
    }

    pub fn is_empty(&self) -> bool {
        self.slots.is_empty()
    }

    /// Runs every delivery in push order: a completion's handler with its
    /// result, a registration's callback with its arguments, and a
    /// registration's `RETIRED`, which drops its handler. A panic or a
    /// callback's error goes to the reporter, as in callback mode.
    pub fn dispatch(mut self) {
        for (index, slot) in std::mem::take(&mut self.slots).into_iter().enumerate() {
            slot.run(&self.owner, index);
        }
    }

    /// The deliveries still in the batch, in push order. Each borrows the
    /// batch until [`Delivery::take`] moves it out.
    pub fn deliveries(&mut self) -> impl Iterator<Item = Delivery<'_, K>> {
        let owner = &self.owner;
        self.slots
            .iter_mut()
            .enumerate()
            .filter_map(move |(index, slot)| {
                slot.kind().map(|kind| Delivery {
                    owner,
                    index,
                    kind,
                    slot,
                })
            })
    }
}

/// One delivery of a batch, which borrows it until [`Delivery::take`] moves
/// the delivery out.
pub struct Delivery<'b, K: QueueKind> {
    owner: &'b Arc<BatchOwner>,
    index: usize,
    kind: DeliveryKind,
    slot: &'b mut Slot<K>,
}

impl<K: QueueKind> std::fmt::Debug for Delivery<'_, K> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("Delivery")
            .field("index", &self.index)
            .field("kind", &self.kind)
            .finish()
    }
}

impl<K: QueueKind> Delivery<'_, K> {
    pub fn kind(&self) -> DeliveryKind {
        self.kind
    }

    /// Moves the delivery out of its batch. The taken delivery keeps the
    /// batch's storage alive until it runs or drops.
    pub fn take(self) -> TakenDelivery<K> {
        TakenDelivery {
            owner: Arc::clone(self.owner),
            index: self.index,
            slot: std::mem::replace(self.slot, Slot::EMPTY),
        }
    }
}

/// A delivery moved out of its batch, which runs once.
pub struct TakenDelivery<K: QueueKind = Shared> {
    owner: Arc<BatchOwner>,
    index: usize,
    slot: Slot<K>,
}

impl<K: QueueKind> std::fmt::Debug for TakenDelivery<K> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("TakenDelivery")
            .field("index", &self.index)
            .field("kind", &self.slot.kind())
            .finish()
    }
}

impl<K: QueueKind> TakenDelivery<K> {
    pub fn kind(&self) -> DeliveryKind {
        self.slot
            .kind()
            .expect("a taken delivery holds what it runs")
    }

    /// Runs the delivery, as [`DeliveryBatch::dispatch`] would.
    pub fn run(mut self) {
        let slot = std::mem::replace(&mut self.slot, Slot::EMPTY);
        slot.run(&self.owner, self.index);
    }
}

/// The signal that a queue's wake raises.
#[derive(Default)]
struct Signal {
    raised: Mutex<bool>,
    ready: Condvar,
    stopped: AtomicBool,
}

impl Signal {
    fn raise(&self) {
        *lock(&self.raised) = true;
        self.ready.notify_all();
    }

    fn stop(&self) {
        self.stopped.store(true, Ordering::Release);
        self.raise();
    }

    /// Waits for a wake for at most `timeout`, and reports whether one came.
    fn wait(&self, timeout: Option<Duration>) -> bool {
        let raised = lock(&self.raised);
        let mut raised = match timeout {
            Some(timeout) => {
                self.ready
                    .wait_timeout_while(raised, timeout, |raised| !*raised)
                    .unwrap_or_else(|poisoned| poisoned.into_inner())
                    .0
            }
            None => self
                .ready
                .wait_while(raised, |raised| !*raised)
                .unwrap_or_else(|poisoned| poisoned.into_inner()),
        };
        std::mem::replace(&mut *raised, false)
    }

    fn wake(self: &Arc<Self>) -> crate::Wake {
        let signal = Arc::clone(self);
        crate::Wake::new(move || signal.raise())
    }
}

/// Drains `queue` and runs every batch until it is empty.
fn drain_all<K: QueueKind>(queue: &DeliveryQueue<K>) -> Result<usize> {
    let mut batches = 0;
    while let Some(batch) = queue.drain()? {
        batch.dispatch();
        batches += 1;
    }
    Ok(batches)
}

/// A thread that drains one shared queue and runs its handlers.
///
/// Closing the dispatcher, by [`Dispatcher::close`] or by dropping it, stops
/// its thread, runs what is already queued on the closing thread, and closes
/// the queue. A handler may close its own dispatcher; the thread then closes
/// the queue once the handler returns.
pub struct Dispatcher {
    queue: Option<Arc<DeliveryQueue<Shared>>>,
    signal: Arc<Signal>,
    thread: Option<JoinHandle<()>>,
    thread_id: ThreadId,
}

impl std::fmt::Debug for Dispatcher {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("Dispatcher")
            .field("queue", &self.queue)
            .finish()
    }
}

impl Dispatcher {
    /// Starts a dispatcher thread over a new queue.
    pub fn new() -> Result<Self> {
        let signal = Arc::new(Signal::default());
        let queue = Arc::new(DeliveryQueue::new(Some(signal.wake()))?);
        let thread = std::thread::Builder::new()
            .name("maplibre-dispatcher".into())
            .spawn({
                let queue = Arc::clone(&queue);
                let signal = Arc::clone(&signal);
                move || {
                    while !signal.stopped.load(Ordering::Acquire) {
                        if let Err(error) = drain_all(&queue) {
                            crate::report::report(crate::report::Report::CallbackError {
                                callback: "mln_delivery_queue_drain",
                                error,
                            });
                        }
                        signal.wait(None);
                    }
                }
            })
            .map_err(|error| {
                Error::new(Status::NativeError, format!("dispatcher thread: {error}"))
            })?;
        let thread_id = thread.thread().id();
        Ok(Self {
            queue: Some(queue),
            signal,
            thread: Some(thread),
            thread_id,
        })
    }

    /// The queue whose handlers this dispatcher runs.
    pub fn queue(&self) -> &DeliveryQueue<Shared> {
        self.queue
            .as_deref()
            .expect("an open dispatcher holds its queue")
    }

    /// Stops the thread, runs what is already queued, and closes the queue,
    /// as [`DeliveryQueue::close`] describes.
    pub fn close(self) {}
}

impl Drop for Dispatcher {
    fn drop(&mut self) {
        self.signal.stop();
        let Some(queue) = self.queue.take() else {
            return;
        };
        let Some(thread) = self.thread.take() else {
            return;
        };
        if std::thread::current().id() == self.thread_id {
            // A handler is closing its own dispatcher; the thread's own
            // reference closes the queue once the handler returns.
            return;
        }
        let _ = thread.join();
        let _ = drain_all(&queue);
    }
}

/// A queue that the thread which owns it drains, whose completion handlers
/// need not be `Send`.
///
/// It runs nothing on its own: [`LocalDispatcher::dispatch`] runs what is
/// queued, and [`LocalDispatcher::wait`] blocks until the queue wakes.
/// Dropping it closes the queue, as [`DeliveryQueue::close`] describes.
pub struct LocalDispatcher {
    queue: DeliveryQueue<Local>,
    signal: Arc<Signal>,
}

impl std::fmt::Debug for LocalDispatcher {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("LocalDispatcher")
            .field("queue", &self.queue)
            .finish()
    }
}

impl LocalDispatcher {
    pub fn new() -> Result<Self> {
        let signal = Arc::new(Signal::default());
        let queue = DeliveryQueue::new(Some(signal.wake()))?;
        Ok(Self { queue, signal })
    }

    /// The queue whose handlers this dispatcher runs.
    pub fn queue(&self) -> &DeliveryQueue<Local> {
        &self.queue
    }

    /// Runs every delivery queued now, and returns how many batches ran.
    pub fn dispatch(&self) -> Result<usize> {
        drain_all(&self.queue)
    }

    /// Blocks for at most `timeout` until the queue wakes, and reports
    /// whether it did. A wake that came since the last wait counts.
    pub fn wait(&self, timeout: Duration) -> bool {
        self.signal.wait(Some(timeout))
    }
}

#[cfg(test)]
mod tests {
    //! Deterministic schedules over a scripted native queue, which pushes,
    //! drains, takes, and releases exactly when a test says so.

    use std::sync::atomic::AtomicU64;
    use std::sync::mpsc;

    use super::*;

    /// One delivery that the scripted queue holds: its type, token, and
    /// payload, which a batch lays out as native does.
    #[derive(Clone, Copy)]
    struct Pushed {
        type_: sys::mln_delivery_type,
        user_data: *mut c_void,
        payload: sys::mln_delivery_payload,
        /// Whether the payload owns a handle that the batch must release
        /// unless the binding takes it.
        owns: bool,
    }

    /// A drained batch: its deliveries, which own a handle, and which of
    /// those the binding took.
    struct Drained {
        deliveries: Vec<sys::mln_delivery>,
        owns: Vec<bool>,
        taken: Vec<bool>,
    }

    #[derive(Default)]
    struct Script {
        pending: Vec<Pushed>,
        released: bool,
        batches: HashMap<u64, Drained>,
        /// Handles of owned payloads that a batch released untaken.
        discarded: usize,
    }

    // SAFETY: the scripted payloads are plain data that the tests build.
    unsafe impl Send for Script {}

    static SCRIPTS: Mutex<Option<HashMap<u64, Script>>> = Mutex::new(None);
    static NEXT: AtomicU64 = AtomicU64::new(0x5c_0000);

    fn with<R>(queue: u64, body: impl FnOnce(&mut Script) -> R) -> R {
        let mut scripts = lock(&SCRIPTS);
        body(
            scripts
                .get_or_insert_with(HashMap::new)
                .entry(queue)
                .or_default(),
        )
    }

    fn queue_of(batch: u64) -> u64 {
        batch >> 16
    }

    unsafe extern "C" fn drain(
        queue: sys::mln_delivery_queue,
        out_batch: *mut sys::mln_delivery_batch,
        out_view: *mut sys::mln_delivery_batch_view,
        _: *mut sys::mln_diagnostic,
    ) -> sys::mln_status {
        with(queue.0, |script| {
            if script.released {
                return sys::MLN_STATUS_INVALID_STATE;
            }
            if script.pending.is_empty() {
                return sys::MLN_STATUS_NOT_READY;
            }
            let pushed = std::mem::take(&mut script.pending);
            let deliveries: Vec<_> = pushed
                .iter()
                .map(|item| sys::mln_delivery {
                    type_: item.type_,
                    reserved: 0,
                    user_data: item.user_data,
                    payload: item.payload,
                })
                .collect();
            let id = (queue.0 << 16) | (NEXT.fetch_add(1, Ordering::Relaxed) & 0xffff);
            let view = sys::mln_delivery_batch_view {
                size: std::mem::size_of::<sys::mln_delivery_batch_view>() as u32,
                delivery_size: std::mem::size_of::<sys::mln_delivery>() as u32,
                deliveries: deliveries.as_ptr(),
                delivery_count: deliveries.len(),
            };
            let owns = pushed.iter().map(|item| item.owns).collect();
            let taken = vec![false; pushed.len()];
            script.batches.insert(
                id,
                Drained {
                    deliveries,
                    owns,
                    taken,
                },
            );
            // SAFETY: drain receives valid outputs.
            unsafe {
                *out_batch = sys::mln_delivery_batch(id);
                *out_view = view;
            }
            sys::MLN_STATUS_OK
        })
    }

    unsafe extern "C" fn take(
        batch: sys::mln_delivery_batch,
        index: usize,
        _: *mut sys::mln_diagnostic,
    ) -> sys::mln_status {
        with(queue_of(batch.0), |script| {
            let Some(drained) = script.batches.get_mut(&batch.0) else {
                return sys::MLN_STATUS_INVALID_STATE;
            };
            if !drained.owns[index] {
                return sys::MLN_STATUS_OK;
            }
            if drained.taken[index] {
                return sys::MLN_STATUS_INVALID_STATE;
            }
            drained.taken[index] = true;
            sys::MLN_STATUS_OK
        })
    }

    unsafe extern "C" fn release_batch(batch: sys::mln_delivery_batch) {
        with(queue_of(batch.0), |script| {
            let drained = script
                .batches
                .remove(&batch.0)
                .expect("a batch is released once");
            // The view pointed into the deliveries until now.
            drop(drained.deliveries);
            script.discarded += drained
                .owns
                .iter()
                .zip(&drained.taken)
                .filter(|(owns, taken)| **owns && !**taken)
                .count();
        })
    }

    unsafe extern "C" fn release(queue: sys::mln_delivery_queue) {
        with(queue.0, |script| {
            script.released = true;
            script.pending.clear();
        })
    }

    static SCRIPTED: NativeQueue = NativeQueue {
        drain,
        take,
        release_batch,
        release,
    };

    /// A queue over the scripted native queue.
    fn scripted<K: QueueKind>() -> DeliveryQueue<K> {
        let id = NEXT.fetch_add(1, Ordering::Relaxed) + 1;
        with(id, |_| {});
        DeliveryQueue::adopt(sys::mln_delivery_queue(id), &SCRIPTED)
    }

    fn zeroed_payload() -> sys::mln_delivery_payload {
        // SAFETY: all zeroes is a valid payload of every member.
        unsafe { std::mem::zeroed() }
    }

    /// Native pushes the completion that `completion` names.
    fn complete(completion: &sys::mln_completion, status: sys::mln_status, generation: u64) {
        let mut payload = zeroed_payload();
        payload.completion.size = std::mem::size_of::<sys::mln_completion_result>() as u32;
        payload.completion.status = status;
        payload.completion.generation = generation;
        push(
            completion.queue,
            sys::MLN_DELIVERY_TYPE_COMPLETION,
            completion.user_data,
            payload,
            false,
        );
    }

    fn push(
        queue: sys::mln_delivery_queue,
        type_: sys::mln_delivery_type,
        user_data: *mut c_void,
        payload: sys::mln_delivery_payload,
        owns: bool,
    ) {
        with(queue.0, |script| {
            if !script.released {
                script.pending.push(Pushed {
                    type_,
                    user_data,
                    payload,
                    owns,
                });
            }
        });
    }

    fn discarded<K: QueueKind>(queue: &DeliveryQueue<K>) -> usize {
        with(queue.raw.0, |script| script.discarded)
    }

    /// A handler that sends its result once.
    fn observer() -> (
        impl for<'b> FnOnce(QueuedResult<'b, Command>) + Send + 'static,
        mpsc::Receiver<Result<CommandCompletion>>,
    ) {
        let (sender, results) = mpsc::channel();
        (move |result| sender.send(result).unwrap(), results)
    }

    /// A command submitted on `queue` whose native side runs `native` with
    /// the completion before it returns `status`.
    fn submit<K: QueueKind>(
        queue: &DeliveryQueue<K>,
        handler: impl CompletionHandler<K, Command>,
        status: sys::mln_status,
        native: impl FnOnce(&sys::mln_completion),
    ) -> Result<()> {
        queue.enqueue(Call::global(), handler, |_, completion, _| {
            // SAFETY: the submission lends a complete descriptor.
            native(unsafe { &*completion });
            status
        })
    }

    #[derive(Debug)]
    struct DropProbe(Arc<AtomicBool>);

    impl Drop for DropProbe {
        fn drop(&mut self) {
            self.0.store(true, Ordering::Release);
        }
    }

    fn probe() -> (DropProbe, Arc<AtomicBool>) {
        let dropped = Arc::new(AtomicBool::new(false));
        (DropProbe(Arc::clone(&dropped)), dropped)
    }

    fn dropped(flag: &AtomicBool) -> bool {
        flag.load(Ordering::Acquire)
    }

    /// A registration whose handler counts its deliveries and drops a probe
    /// when the queue frees it.
    fn register<K: QueueKind>(
        queue: &DeliveryQueue<K>,
    ) -> (
        *mut c_void,
        Arc<std::sync::atomic::AtomicUsize>,
        Arc<AtomicBool>,
    ) {
        let runs = Arc::new(std::sync::atomic::AtomicUsize::new(0));
        let (probe, dropped) = probe();
        let counter = Arc::clone(&runs);
        let handler: RegistrationHandler = Arc::new(move |_, take: &dyn Fn() -> Result<()>| {
            let _ = &probe;
            take().unwrap();
            counter.fetch_add(1, Ordering::AcqRel);
        });
        let mut arena = InputArena::default();
        arena.set_queue(queue);
        let (raw, token) = arena
            .queued_registration(false, || Handler::Shared(handler))
            .unwrap()
            .unwrap();
        assert_eq!(raw, queue.raw);
        arena.accept_registrations();
        (token, runs, dropped)
    }

    fn deliver(queue: sys::mln_delivery_queue, token: *mut c_void, owns: bool) {
        push(
            queue,
            sys::MLN_DELIVERY_TYPE_RESOURCE_REQUEST,
            token,
            zeroed_payload(),
            owns,
        );
    }

    fn retire(queue: sys::mln_delivery_queue, token: *mut c_void) {
        push(
            queue,
            sys::MLN_DELIVERY_TYPE_RETIRED,
            token,
            zeroed_payload(),
            false,
        );
    }

    fn dispatch<K: QueueKind>(queue: &DeliveryQueue<K>) {
        while let Some(batch) = queue.drain().unwrap() {
            batch.dispatch();
        }
    }

    #[test]
    fn a_completion_queued_before_its_submission_returns_reaches_its_handler() {
        let queue = scripted::<Shared>();
        let (handler, results) = observer();
        // Native queues the completion inside the submitting call.
        submit(&queue, handler, sys::MLN_STATUS_OK, |completion| {
            assert_eq!(completion.queue, queue.raw);
            assert!(completion.callback.is_none() && completion.release_user_data.is_none());
            complete(completion, sys::MLN_STATUS_OK, 7);
        })
        .unwrap();
        assert!(
            results.try_recv().is_err(),
            "a handler runs only when drained"
        );
        dispatch(&queue);
        assert_eq!(results.try_recv().unwrap().unwrap().generation, 7);
    }

    #[test]
    fn a_failed_completion_reaches_its_handler_with_its_status_and_generation() {
        let queue = scripted::<Shared>();
        let (handler, results) = observer();
        let mut accepted = None;
        submit(&queue, handler, sys::MLN_STATUS_OK, |completion| {
            accepted = Some(*completion);
        })
        .unwrap();
        complete(&accepted.unwrap(), sys::MLN_STATUS_INVALID_ARGUMENT, 3);
        dispatch(&queue);
        let error = results.try_recv().unwrap().unwrap_err();
        assert_eq!(error.status(), Status::InvalidArgument);
        assert_eq!(error.generation(), 3);
    }

    #[test]
    fn a_rejected_submission_drops_its_handler_unrun_and_queues_nothing() {
        let queue = scripted::<Shared>();
        let (probe, dropped_probe) = probe();
        let (handler, results) = observer();
        let error = submit(
            &queue,
            move |result| {
                let _ = &probe;
                handler(result)
            },
            sys::MLN_STATUS_INVALID_STATE,
            |_| {},
        )
        .unwrap_err();
        assert_eq!(error.status(), Status::InvalidState);
        assert!(dropped(&dropped_probe));
        assert!(results.try_recv().is_err());
        assert!(lock(&queue.registry).entries.is_empty());
        assert!(queue.drain().unwrap().is_none());
    }

    #[test]
    fn an_observer_dropped_before_its_result_lets_the_handler_run_and_free_it() {
        let queue = scripted::<Shared>();
        let (future, resolver) = crate::pending::<CommandCompletion>();
        let mut accepted = None;
        submit(
            &queue,
            move |result: QueuedResult<'_, Command>| resolver.resolve(result),
            sys::MLN_STATUS_OK,
            |completion| accepted = Some(*completion),
        )
        .unwrap();
        // Nothing waits for the result any more, but native still delivers
        // it, and the handler still runs once and is freed.
        drop(future);
        complete(&accepted.unwrap(), sys::MLN_STATUS_OK, 1);
        dispatch(&queue);
        assert!(lock(&queue.registry).entries.is_empty());
    }

    #[test]
    fn a_dropped_batch_cancels_the_completions_it_never_ran() {
        let queue = scripted::<Shared>();
        let (handler, results) = observer();
        submit(&queue, handler, sys::MLN_STATUS_OK, |completion| {
            complete(completion, sys::MLN_STATUS_OK, 1)
        })
        .unwrap();
        drop(queue.drain().unwrap().unwrap());
        assert_eq!(
            results.try_recv().unwrap().unwrap_err().status(),
            Status::Cancelled
        );
    }

    #[test]
    fn closing_a_queue_cancels_pending_completions_and_rejects_later_submissions() {
        let queue = scripted::<Shared>();
        let (handler, results) = observer();
        submit(&queue, handler, sys::MLN_STATUS_OK, |_| {}).unwrap();
        let raw = queue.raw;
        queue.close();
        assert_eq!(
            results.try_recv().unwrap().unwrap_err().status(),
            Status::Cancelled
        );
        assert!(with(raw.0, |script| script.released));
    }

    #[test]
    fn a_submission_after_close_fails_before_it_reaches_native() {
        let queue = scripted::<Shared>();
        queue.close();
        assert_eq!(queue.drain().unwrap_err().status(), Status::InvalidState);
        let (probe, dropped_probe) = probe();
        let error = submit(
            &queue,
            move |_: QueuedResult<'_, Command>| {
                let _ = &probe;
            },
            sys::MLN_STATUS_OK,
            |_| panic!("a closed queue's submission reached native"),
        )
        .unwrap_err();
        assert_eq!(error.status(), Status::InvalidState);
        assert!(dropped(&dropped_probe));

        // A registration fails the same way, so its record drops unrun.
        let mut arena = InputArena::default();
        arena.set_queue(&queue);
        let error = arena
            .queued_registration(false, || {
                Handler::Shared(Arc::new(|_, _: &dyn Fn() -> Result<()>| {}))
            })
            .unwrap_err();
        assert_eq!(error.status(), Status::InvalidState);
    }

    #[test]
    fn a_close_during_a_submission_ends_its_handler_as_native_decides() {
        // Native accepts the call after the queue closed, so the handler runs
        // once, cancelled, after the call returns.
        let queue = scripted::<Shared>();
        let (handler, results) = observer();
        submit(&queue, handler, sys::MLN_STATUS_OK, |_| {
            queue.close();
            assert!(
                results.try_recv().is_err(),
                "a call in native keeps its handler"
            );
        })
        .unwrap();
        assert_eq!(
            results.try_recv().unwrap().unwrap_err().status(),
            Status::Cancelled
        );

        // Native rejects it, so the handler drops unrun.
        let queue = scripted::<Shared>();
        let (rejected, dropped_probe) = probe();
        let (handler, results) = observer();
        let error = submit(
            &queue,
            move |result| {
                let _ = &rejected;
                handler(result)
            },
            sys::MLN_STATUS_INVALID_STATE,
            |_| queue.close(),
        )
        .unwrap_err();
        assert_eq!(error.status(), Status::InvalidState);
        assert!(dropped(&dropped_probe));
        assert!(results.try_recv().is_err());

        // A registration that native accepts as its queue closes drops.
        let queue = scripted::<Shared>();
        let (registered, dropped_probe) = probe();
        let mut arena = InputArena::default();
        arena.set_queue(&queue);
        arena
            .queued_registration(false, || {
                Handler::Shared(Arc::new(move |_, _: &dyn Fn() -> Result<()>| {
                    let _ = &registered;
                }))
            })
            .unwrap();
        queue.close();
        assert!(
            !dropped(&dropped_probe),
            "a call in native keeps its handler"
        );
        arena.accept_registrations();
        assert!(dropped(&dropped_probe));
    }

    /// A command submitted on `queue` without a completion, carrying a camera
    /// end handler that holds `probe` when there is one. Its native side runs
    /// `native` with the handler's token before it returns `status`.
    fn discard_command<K: QueueKind>(
        queue: &DeliveryQueue<K>,
        probe: Option<DropProbe>,
        status: sys::mln_status,
        native: impl FnOnce(*mut c_void),
    ) -> Result<()> {
        let mut call = Call::global().queue(queue);
        let token = match probe {
            Some(probe) => {
                let handler = crate::CameraTransitionHandler::new(move |_| {
                    let _ = &probe;
                });
                let raw: sys::mln_camera_transition_handler = call.input(&handler)?;
                assert_eq!(raw.queue, queue.raw);
                raw.user_data
            }
            None => std::ptr::null_mut(),
        };
        queue.discard(call, |_, completion, _| {
            assert!(
                completion.is_null(),
                "a discarded command has no completion"
            );
            native(token);
            status
        })
    }

    #[test]
    fn a_rejected_queued_discard_drops_its_registration_handlers_unrun() {
        let queue = scripted::<Shared>();
        let (probe, dropped_probe) = probe();
        let error = discard_command(
            &queue,
            Some(probe),
            sys::MLN_STATUS_INVALID_ARGUMENT,
            |_| {},
        )
        .unwrap_err();
        assert_eq!(error.status(), Status::InvalidArgument);
        assert!(dropped(&dropped_probe));
        assert!(lock(&queue.registry).entries.is_empty());
    }

    #[test]
    fn an_accepted_queued_discard_keeps_its_registration_handlers_until_retired() {
        let queue = scripted::<Shared>();
        let (probe, dropped_probe) = probe();
        let mut token = std::ptr::null_mut();
        discard_command(&queue, Some(probe), sys::MLN_STATUS_OK, |held| token = held).unwrap();
        assert!(!dropped(&dropped_probe));
        assert_eq!(lock(&queue.registry).entries.len(), 1);
        dispatch(&queue);
        assert!(!dropped(&dropped_probe));

        retire(queue.raw, token);
        dispatch(&queue);
        assert!(dropped(&dropped_probe));
        assert!(lock(&queue.registry).entries.is_empty());
    }

    #[test]
    fn a_closed_queue_rejects_a_queued_discard_with_or_without_registrations() {
        let queue = scripted::<Shared>();
        queue.close();
        let unreached = |_| panic!("a closed queue's discard reached native");
        let error = discard_command(&queue, None, sys::MLN_STATUS_OK, unreached).unwrap_err();
        assert_eq!(error.status(), Status::InvalidState);

        let (probe, dropped_probe) = probe();
        let error =
            discard_command(&queue, Some(probe), sys::MLN_STATUS_OK, unreached).unwrap_err();
        assert_eq!(error.status(), Status::InvalidState);
        assert!(dropped(&dropped_probe));
        assert!(lock(&queue.registry).entries.is_empty());
    }

    #[test]
    fn a_registration_runs_for_each_delivery_and_drops_at_retirement() {
        let queue = scripted::<Shared>();
        let (token, runs, dropped_probe) = register(&queue);
        deliver(queue.raw, token, true);
        deliver(queue.raw, token, true);
        dispatch(&queue);
        assert_eq!(runs.load(Ordering::Acquire), 2);
        assert!(!dropped(&dropped_probe));
        // The handler took both handles, so the batch released none.
        assert_eq!(discarded(&queue), 0);

        retire(queue.raw, token);
        dispatch(&queue);
        assert!(dropped(&dropped_probe));
        assert!(lock(&queue.registry).entries.is_empty());
    }

    #[test]
    fn a_rejected_registration_drops_its_handler_unrun() {
        let queue = scripted::<Shared>();
        let (probe, dropped_probe) = probe();
        {
            let mut arena = InputArena::default();
            arena.set_queue(&queue);
            arena
                .queued_registration(false, || {
                    Handler::Shared(Arc::new(move |_, _: &dyn Fn() -> Result<()>| {
                        let _ = &probe;
                    }))
                })
                .unwrap();
            // Native refused the call, so the arena drops without accepting.
        }
        assert!(dropped(&dropped_probe));
        assert!(lock(&queue.registry).entries.is_empty());
    }

    #[test]
    fn a_retirement_drained_with_its_last_delivery_runs_after_it() {
        let queue = scripted::<Shared>();
        let (token, runs, dropped_probe) = register(&queue);
        // The last delivery and the retirement race into one batch.
        deliver(queue.raw, token, false);
        retire(queue.raw, token);
        let batch = queue.drain().unwrap().unwrap();
        // The batch holds the handler, though the queue no longer does.
        assert!(lock(&queue.registry).entries.is_empty());
        assert!(!dropped(&dropped_probe));
        batch.dispatch();
        assert_eq!(runs.load(Ordering::Acquire), 1);
        assert!(dropped(&dropped_probe));
    }

    #[test]
    fn a_retirement_in_a_later_batch_waits_for_the_batch_that_holds_a_delivery() {
        let queue = scripted::<Shared>();
        let (token, runs, dropped_probe) = register(&queue);
        deliver(queue.raw, token, false);
        let first = queue.drain().unwrap().unwrap();
        retire(queue.raw, token);
        // The retirement runs first, on another drain, while the earlier
        // batch still holds the handler.
        queue.drain().unwrap().unwrap().dispatch();
        assert!(!dropped(&dropped_probe));
        first.dispatch();
        assert_eq!(runs.load(Ordering::Acquire), 1);
        assert!(dropped(&dropped_probe));
    }

    #[test]
    fn closing_a_queue_before_retirement_frees_handlers_once_no_batch_holds_them() {
        let queue = scripted::<Shared>();
        let (token, runs, dropped_probe) = register(&queue);
        let (idle, _, idle_dropped) = register(&queue);
        let _ = idle;
        deliver(queue.raw, token, true);
        let batch = queue.drain().unwrap().unwrap();
        // Native will never push RETIRED to a released queue.
        drop(queue);
        assert!(dropped(&idle_dropped));
        assert!(!dropped(&dropped_probe));
        // The drained batch stays readable and still runs the delivery.
        batch.dispatch();
        assert_eq!(runs.load(Ordering::Acquire), 1);
        assert!(dropped(&dropped_probe));
    }

    #[test]
    fn a_delivery_taken_out_of_its_batch_runs_once_and_an_untaken_one_releases_its_handle() {
        let queue = scripted::<Shared>();
        let (token, runs, _) = register(&queue);
        deliver(queue.raw, token, true);
        deliver(queue.raw, token, true);
        let mut batch = queue.drain().unwrap().unwrap();
        let taken = batch.deliveries().next().unwrap().take();
        assert_eq!(taken.kind(), DeliveryKind::Registration);
        // The second delivery stays in the batch, unrun, and the batch
        // releases its handle, which the taken delivery keeps alive no longer
        // than itself.
        drop(batch);
        assert_eq!(discarded(&queue), 0, "the taken delivery keeps the batch");
        std::thread::spawn(move || taken.run()).join().unwrap();
        assert_eq!(runs.load(Ordering::Acquire), 1);
        assert_eq!(discarded(&queue), 1);
    }

    #[test]
    fn a_token_from_another_queue_runs_nothing() {
        let first = scripted::<Shared>();
        let second = scripted::<Shared>();
        let (token, runs, _) = register(&first);
        // The second queue never issued this token.
        deliver(second.raw, token, true);
        dispatch(&second);
        assert_eq!(runs.load(Ordering::Acquire), 0);
        assert_eq!(discarded(&second), 1);
    }

    #[test]
    fn a_panicking_handler_is_contained_and_the_batch_runs_on() {
        let queue = scripted::<Shared>();
        let (handler, results) = observer();
        submit(
            &queue,
            |_: QueuedResult<'_, Command>| panic!("handler bug"),
            sys::MLN_STATUS_OK,
            |completion| complete(completion, sys::MLN_STATUS_OK, 1),
        )
        .unwrap();
        submit(&queue, handler, sys::MLN_STATUS_OK, |completion| {
            complete(completion, sys::MLN_STATUS_OK, 2)
        })
        .unwrap();
        dispatch(&queue);
        assert_eq!(results.try_recv().unwrap().unwrap().generation, 2);
    }

    #[test]
    fn a_local_queue_runs_handlers_that_are_not_send() {
        let queue = scripted::<Local>();
        let seen = std::rc::Rc::new(std::cell::Cell::new(0));
        let shared = std::rc::Rc::clone(&seen);
        submit(
            &queue,
            move |result: QueuedResult<'_, Command>| shared.set(result.unwrap().generation),
            sys::MLN_STATUS_OK,
            |completion| complete(completion, sys::MLN_STATUS_OK, 9),
        )
        .unwrap();
        dispatch(&queue);
        assert_eq!(seen.get(), 9);
    }

    #[test]
    fn concurrent_drains_run_each_handler_once() {
        let queue = Arc::new(scripted::<Shared>());
        let count = 2_000;
        let (sender, results) = mpsc::channel();
        let mut accepted = Vec::new();
        for _ in 0..count {
            let sender = sender.clone();
            submit(
                &queue,
                move |result: QueuedResult<'_, Command>| {
                    sender.send(result.unwrap().generation).unwrap()
                },
                sys::MLN_STATUS_OK,
                |completion| accepted.push(*completion),
            )
            .unwrap();
        }
        let pushers = accepted
            .chunks(count / 4)
            .map(|chunk| {
                let chunk: Vec<_> = chunk
                    .iter()
                    .map(|c| (c.queue.0, c.user_data as usize))
                    .collect();
                std::thread::spawn(move || {
                    for (queue, user_data) in chunk {
                        let completion = sys::mln_completion {
                            size: 0,
                            callback: None,
                            user_data: user_data as *mut c_void,
                            release_user_data: None,
                            queue: sys::mln_delivery_queue(queue),
                        };
                        complete(&completion, sys::MLN_STATUS_OK, user_data as u64);
                    }
                })
            })
            .collect::<Vec<_>>();
        let drainers = (0..4)
            .map(|_| {
                let queue = Arc::clone(&queue);
                std::thread::spawn(move || {
                    for _ in 0..200 {
                        dispatch(&queue);
                        std::thread::yield_now();
                    }
                })
            })
            .collect::<Vec<_>>();
        for thread in pushers.into_iter().chain(drainers) {
            thread.join().unwrap();
        }
        dispatch(&queue);
        drop(sender);
        let mut seen: Vec<u64> = results.iter().collect();
        seen.sort_unstable();
        seen.dedup();
        assert_eq!(seen.len(), count);
    }
}
