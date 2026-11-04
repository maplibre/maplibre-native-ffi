// Ports maplibre-native-ffi src/resources/custom_resource_provider.cpp and the
// resource provider half of src/runtime/runtime.cpp (set_resource_provider,
// clear_resource_provider, queue_provider_request): a runtime's network
// requests that a host serves through a callback.
//
// A provider request has two halves. `ProviderRequest`, the AsyncRequest that
// the requesting file source holds, lives on the requesting thread: it owns
// the requester's callback (in a `FileSourceRequest`) and the native request
// that a pass-through starts. `ResourceRequestObject`, which the handle
// table holds, is what the host's thread reaches: it posts the answer to the
// requesting thread through the two halves' mailboxes, which close when the
// requester cancels the request.

use crate::delivery::{Completion, UserData, deliver};
use crate::diagnostics::{Failure, boundary};
use crate::handles::{self, Kind as HandleKind, Object};
use crate::resource_loader::{Registration, error_response, kind_to_abi};
use crate::types::*;
use crate::url_rules::{RouteTable, copy_routes, validate_c_string_view};
use maplibre_native_next::mln_actor::{ActorRef, Mailbox, scheduler};
use maplibre_native_next::mln_storage::file_source::Callback;
use maplibre_native_next::mln_storage::file_source_request::FileSourceRequestState;
use maplibre_native_next::mln_storage::resource::{Priority, StoragePolicy, Usage};
use maplibre_native_next::mln_storage::response::{Data, Error, Reason};
use maplibre_native_next::mln_storage::{AsyncRequest, FileSourceRequest, LoadingMethod, Resource, Response};
use maplibre_native_next::mln_util::chrono::Timestamp;
use std::any::Any;
use std::cell::RefCell;
use std::rc::Rc;
use std::sync::{Arc, Condvar, Mutex};
use std::thread::ThreadId;

/// A registered resource provider and the requests it serves.
pub struct ProviderRegistration {
    registration: Registration<mln_resource_provider_callback>,
    routes: RouteTable,
}

impl ProviderRegistration {
    /// Whether the provider's routes claim a request.
    pub fn matches(&self, kind: u32, resolved_url: &str) -> bool {
        self.routes.matches(kind, resolved_url)
    }
}

/// Starts the native request of a pass-through, on the requesting thread.
pub type StartNative = Box<dyn FnOnce(Callback) -> Box<dyn AsyncRequest>>;

/// Why a request failed without an answer (C++ `ProviderRequest::Failure`).
#[derive(Clone, Copy)]
enum Failure_ {
    Released,
}

impl Failure_ {
    fn message(self) -> &'static str {
        match self {
            Failure_::Released => "resource request was released without a response",
        }
    }
}

/// The requesting thread's half of a pass-through: the target of the
/// request's control mailbox.
struct ProviderLoader {
    start_native: RefCell<Option<StartNative>>,
    native: RefCell<Option<Box<dyn AsyncRequest>>>,
    response: ActorRef<FileSourceRequestState>,
}

impl ProviderLoader {
    /// `ProviderRequest::pass_through`.
    fn pass_through(&self) {
        let Some(start) = self.start_native.borrow_mut().take() else {
            return;
        };
        let target = self.response.clone();
        let native = start(Box::new(move |response: Response| {
            target.invoke(move |state| state.set_response(response));
        }));
        *self.native.borrow_mut() = Some(native);
    }
}

/// A registered cancel callback.
struct CancelRegistration {
    callback: mln_resource_request_cancel_callback,
    user_data: UserData,
    release_user_data: mln_user_data_release,
    queue: u64,
}

impl CancelRegistration {
    fn release(self) {
        if let Some(release) = self.release_user_data {
            let user_data = self.user_data;
            deliver(
                self.queue,
                Box::new(move || {
                    // SAFETY: the registration's release, called once.
                    unsafe { release(user_data.get()) };
                }),
            );
        }
    }
}

#[derive(Default)]
struct RequestState {
    cancelled: bool,
    completed: bool,
    retired: bool,
    /// The provider callback is running.
    deciding: bool,
    /// The provider callback returned holding the handle.
    claimed: bool,
    released_while_deciding: bool,
    cancel: Option<CancelRegistration>,
    cancel_callback_registered: bool,
    cancel_callback_running: bool,
    /// Set when the cancel callback released its own request: the callback's
    /// wrapper retires the handle once the callback returns.
    remove_after_cancel_callback: bool,
    cancel_callback_thread: Option<ThreadId>,
}

/// `ResourceRequestObject`: the handle table's half of a provider request.
pub struct ResourceRequestObject {
    handle: u64,
    state: Mutex<RequestState>,
    state_changed: Condvar,
    response: ActorRef<FileSourceRequestState>,
    control: ActorRef<ProviderLoader>,
    /// The cached bytes that the cache withheld from the requester until this
    /// request revalidates them.
    prior_data: Option<Data>,
    prior_modified: Option<Timestamp>,
    prior_etag: Option<String>,
}

impl ResourceRequestObject {
    /// `fail_locked`: answers a request that the provider didn't answer with
    /// an error.
    fn fail_locked(&self, state: &mut RequestState, failure: Failure_) {
        if state.completed || state.cancelled {
            return;
        }
        let response = error_response(Reason::Other, failure.message());
        self.response.invoke(move |request| request.set_response(response));
        state.completed = true;
    }
}

/// `ProviderRequest`: the AsyncRequest that the requester holds. Dropping it
/// cancels the request.
struct ProviderRequest {
    object: Arc<ResourceRequestObject>,
    control: Arc<Mailbox>,
    loader: Rc<ProviderLoader>,
    response: Option<FileSourceRequest>,
}

impl AsyncRequest for ProviderRequest {}

impl Drop for ProviderRequest {
    fn drop(&mut self) {
        self.object.state.lock().unwrap().cancelled = true;
        self.control.close();
        let native = self.loader.native.borrow_mut().take();
        drop(native);
        // Runs the request's cancel callback.
        drop(self.response.take());
    }
}

/// `run_cancel_callback`: runs the registered cancel callback, if any, of a
/// request that wasn't answered. Called without a lock: the callback may
/// call back into the handle.
fn run_cancel_callback(object: &ResourceRequestObject) {
    let cancel = {
        let mut state = object.state.lock().unwrap();
        if state.completed {
            return;
        }
        let Some(cancel) = state.cancel.take() else {
            return;
        };
        if cancel.queue != 0 {
            // A queue handler's call runs when the host dispatches it.
            drop(state);
            let handle = object.handle;
            let CancelRegistration { callback, user_data, release_user_data, queue } = cancel;
            deliver(
                queue,
                Box::new(move || {
                    if let Some(callback) = callback {
                        // SAFETY: the caller registered this callback and user_data.
                        unsafe { callback(user_data.get(), handle.into()) };
                    }
                    if let Some(release) = release_user_data {
                        // SAFETY: the registration's release, called once.
                        unsafe { release(user_data.get()) };
                    }
                }),
            );
            return;
        }
        state.cancel_callback_running = true;
        state.cancel_callback_thread = Some(std::thread::current().id());
        cancel
    };
    if let Some(callback) = cancel.callback {
        // SAFETY: the caller registered this callback and user_data.
        unsafe { callback(cancel.user_data.get(), object.handle.into()) };
    }
    // The callback runs at most once, so its registration retires as it
    // returns.
    if let Some(release) = cancel.release_user_data {
        // SAFETY: the registration's release, called once.
        unsafe { release(cancel.user_data.get()) };
    }
    let remove = {
        let mut state = object.state.lock().unwrap();
        state.cancel_callback_running = false;
        std::mem::take(&mut state.remove_after_cancel_callback)
    };
    object.state_changed.notify_all();
    if remove {
        handles::remove(object.handle);
    }
}

/// `retire_request`: retires the handle so no later call reaches the
/// request. A release on another thread waits for a running cancel callback;
/// a release from inside it leaves the retirement to the callback.
fn retire_request(handle: u64) {
    let Some(Object::ResourceRequest(object)) = handles::get(handle) else {
        return;
    };
    let cancel = {
        let mut state = object.state.lock().unwrap();
        state.retired = true;
        // A registration whose callback never ran retires with the request.
        let cancel = state.cancel.take();
        if state.cancel_callback_running {
            if state.cancel_callback_thread == Some(std::thread::current().id()) {
                state.remove_after_cancel_callback = true;
                return;
            }
            let _state = object.state_changed.wait_while(state, |state| state.cancel_callback_running).unwrap();
        }
        cancel
    };
    if let Some(cancel) = cancel {
        cancel.release();
    }
    handles::remove(handle);
}

/// Lent copies of a request's text, which the request view points at.
struct RequestText {
    requested_url: LentText,
    resolved_url: LentText,
    prior_etag: LentText,
    prior_data: Option<Data>,
}

fn to_unix_ms(timestamp: Timestamp) -> i64 {
    timestamp.secs_since_epoch().saturating_mul(1000)
}

/// `from_unix_ms`: the cache keeps whole seconds, truncated toward zero.
pub fn from_unix_ms(unix_ms: i64) -> Timestamp {
    Timestamp::from_secs(unix_ms / 1000)
}

/// The fields of a request's view, apart from its text.
#[derive(Clone, Copy)]
struct RequestFields {
    fields: u32,
    kind: u32,
    loading_method: u32,
    priority: u32,
    usage: u32,
    storage_policy: u32,
    range: mln_resource_range,
    prior_modified_unix_ms: i64,
    prior_expires_unix_ms: i64,
}

fn request_fields(resource: &Resource) -> RequestFields {
    let mut fields = 0;
    if resource.data_range.is_some() {
        fields |= MLN_RESOURCE_REQUEST_RANGE;
    }
    if resource.prior_modified.is_some() {
        fields |= MLN_RESOURCE_REQUEST_PRIOR_MODIFIED;
    }
    if resource.prior_expires.is_some() {
        fields |= MLN_RESOURCE_REQUEST_PRIOR_EXPIRES;
    }
    let loading_method = if resource.loading_method == LoadingMethod::CacheOnly {
        MLN_RESOURCE_LOADING_METHOD_CACHE_ONLY
    } else if resource.loading_method == LoadingMethod::NetworkOnly {
        MLN_RESOURCE_LOADING_METHOD_NETWORK_ONLY
    } else {
        MLN_RESOURCE_LOADING_METHOD_ALL
    };
    RequestFields {
        fields,
        kind: kind_to_abi(resource.kind),
        loading_method,
        priority: if resource.priority == Priority::Low {
            MLN_RESOURCE_PRIORITY_LOW
        } else {
            MLN_RESOURCE_PRIORITY_REGULAR
        },
        usage: if resource.usage == Usage::Offline { MLN_RESOURCE_USAGE_OFFLINE } else { MLN_RESOURCE_USAGE_ONLINE },
        storage_policy: if resource.storage_policy == StoragePolicy::Volatile {
            MLN_RESOURCE_STORAGE_POLICY_VOLATILE
        } else {
            MLN_RESOURCE_STORAGE_POLICY_PERMANENT
        },
        range: resource
            .data_range
            .map_or(mln_resource_range { start: 0, end: 0 }, |(start, end)| mln_resource_range { start, end }),
        prior_modified_unix_ms: resource.prior_modified.map_or(0, to_unix_ms),
        prior_expires_unix_ms: resource.prior_expires.map_or(0, to_unix_ms),
    }
}

/// `make_request_view`.
fn request_view(fields: &RequestFields, text: &RequestText) -> mln_resource_request_info {
    let prior_data = text.prior_data.as_deref().map_or(&[][..], |data| data.as_slice());
    mln_resource_request_info {
        size: size_of::<mln_resource_request_info>() as u32,
        fields: fields.fields,
        requested_url: mln_buffer_view::of_text(&text.requested_url),
        resolved_url: mln_buffer_view::of_text(&text.resolved_url),
        kind: fields.kind,
        loading_method: fields.loading_method,
        priority: fields.priority,
        usage: fields.usage,
        storage_policy: fields.storage_policy,
        range: fields.range,
        prior_modified_unix_ms: fields.prior_modified_unix_ms,
        prior_expires_unix_ms: fields.prior_expires_unix_ms,
        prior_etag: mln_buffer_view::of_text(&text.prior_etag),
        prior_data: if prior_data.is_empty() { std::ptr::null() } else { prior_data.as_ptr() },
        prior_data_size: prior_data.len(),
    }
}

/// A queued provider call. A call that the queue drops without running
/// fails its request (C++ `queue_provider_request`).
struct QueuedProviderCall {
    handle: u64,
    ran: bool,
}

impl Drop for QueuedProviderCall {
    fn drop(&mut self) {
        if !self.ran {
            // SAFETY: the handle is this call's to release.
            unsafe { mln_resource_request_release(self.handle.into()) };
        }
    }
}

/// `request_custom_resource`: hands `resource` to the provider, with
/// `resolved_url` as the URL that the native path would load. Runs on the
/// requesting thread.
pub fn request_custom_resource(
    resource: &Resource,
    resolved_url: String,
    provider: Arc<ProviderRegistration>,
    callback: Callback,
    start_native: StartNative,
) -> Box<dyn AsyncRequest> {
    let mut response = FileSourceRequest::new(callback);
    let loader = Rc::new(ProviderLoader {
        start_native: RefCell::new(Some(start_native)),
        native: RefCell::new(None),
        response: response.actor(),
    });
    let control = Mailbox::with_target(&scheduler::get_current(), loader.clone() as Rc<dyn Any>);
    let handle = handles::reserve(HandleKind::ResourceRequest);
    let object = Arc::new(ResourceRequestObject {
        handle,
        state: Mutex::new(RequestState::default()),
        state_changed: Condvar::new(),
        response: response.actor(),
        control: ActorRef::new(&control),
        prior_data: resource.prior_data.clone(),
        prior_modified: resource.prior_modified,
        prior_etag: resource.prior_etag.clone(),
    });
    handles::insert_at(handle, Object::ResourceRequest(object.clone()));
    // Runs on every destruction of the request, also after a response was
    // delivered, so a completed request doesn't report cancellation.
    let cancelled = object.clone();
    response.on_cancel(Box::new(move || {
        cancelled.state.lock().unwrap().cancelled = true;
        run_cancel_callback(&cancelled);
    }));

    let fields = request_fields(resource);
    let text = RequestText {
        requested_url: LentText::new(resource.url.as_str()),
        resolved_url: LentText::new(resolved_url),
        prior_etag: LentText::new(resource.prior_etag.clone().unwrap_or_default()),
        prior_data: resource.prior_data.clone(),
    };
    object.state.lock().unwrap().deciding = true;
    let registration = &provider.registration;
    if let Some(callback) = registration.callback {
        let user_data = registration.user_data;
        if registration.queue == 0 {
            let view = request_view(&fields, &text);
            // SAFETY: the caller registered this callback and user_data;
            // the view and the text it points at live until it returns.
            unsafe { callback(user_data.get(), &view, handle.into()) };
        } else {
            // The queue keeps a copy of the request. The registration's
            // release, queued when the runtime retires it, follows the call
            // (C++ `QueuedRegistration`).
            let call = QueuedProviderCall { handle, ran: false };
            deliver(
                registration.queue,
                Box::new(move || {
                    let mut call = call;
                    call.ran = true;
                    let view = request_view(&fields, &text);
                    // SAFETY: as above.
                    unsafe { callback(user_data.get(), &view, call.handle.into()) };
                }),
            );
        }
    }
    // The provider holds the handle until an answer consumes it or it is
    // released. A release during the callback fails the request only now.
    {
        let mut state = object.state.lock().unwrap();
        state.deciding = false;
        state.claimed = true;
        if state.released_while_deciding {
            object.fail_locked(&mut state, Failure_::Released);
        }
    }
    Box::new(ProviderRequest { object, control, loader, response: Some(response) })
}

fn error_reason_from_abi(reason: u32) -> Reason {
    match reason {
        MLN_RESOURCE_ERROR_REASON_NOT_FOUND => Reason::NotFound,
        MLN_RESOURCE_ERROR_REASON_SERVER => Reason::Server,
        MLN_RESOURCE_ERROR_REASON_CONNECTION => Reason::Connection,
        MLN_RESOURCE_ERROR_REASON_RATE_LIMIT => Reason::RateLimit,
        _ => Reason::Other,
    }
}

/// `response_from_abi`. A response that the C API can't read answers the
/// request with an error that says why.
fn response_from_abi(response: &mln_resource_response) -> Response {
    if (response.size as usize) < size_of::<mln_resource_response>() {
        return error_response(Reason::Other, "mln_resource_response.size is too small");
    }
    let known_fields =
        MLN_RESOURCE_RESPONSE_MODIFIED | MLN_RESOURCE_RESPONSE_EXPIRES | MLN_RESOURCE_RESPONSE_RETRY_AFTER;
    if response.fields & !known_fields != 0 {
        return error_response(Reason::Other, "mln_resource_response.fields contains unknown bits");
    }
    if !matches!(
        response.status,
        MLN_RESOURCE_RESPONSE_STATUS_OK
            | MLN_RESOURCE_RESPONSE_STATUS_ERROR
            | MLN_RESOURCE_RESPONSE_STATUS_NO_CONTENT
            | MLN_RESOURCE_RESPONSE_STATUS_NOT_MODIFIED
    ) {
        return error_response(Reason::Other, "resource provider returned an unknown response status");
    }
    if response.byte_count != 0 && response.bytes.is_null() {
        return error_response(Reason::Other, "resource provider returned a null byte buffer");
    }
    let text = |view: mln_buffer_view| {
        // SAFETY: validated by the caller.
        String::from_utf8_lossy(unsafe { view.bytes() }.unwrap_or_default()).into_owned()
    };
    let mut native = Response::new();
    native.no_content = response.status == MLN_RESOURCE_RESPONSE_STATUS_NO_CONTENT;
    native.not_modified = response.status == MLN_RESOURCE_RESPONSE_STATUS_NOT_MODIFIED;
    native.must_revalidate = response.must_revalidate;
    if response.fields & MLN_RESOURCE_RESPONSE_MODIFIED != 0 {
        native.modified = Some(from_unix_ms(response.modified_unix_ms));
    }
    if response.fields & MLN_RESOURCE_RESPONSE_EXPIRES != 0 {
        native.expires = Some(from_unix_ms(response.expires_unix_ms));
    }
    if response.etag.size != 0 {
        native.etag = Some(text(response.etag));
    }
    if response.status == MLN_RESOURCE_RESPONSE_STATUS_ERROR {
        let message = if response.error_message.size == 0 {
            "resource provider failed".to_owned()
        } else {
            text(response.error_message)
        };
        let retry_after = (response.fields & MLN_RESOURCE_RESPONSE_RETRY_AFTER != 0)
            .then(|| from_unix_ms(response.retry_after_unix_ms));
        native.error =
            Some(Box::new(Error::with_retry_after(error_reason_from_abi(response.error_reason), message, retry_after)));
        return native;
    }
    if !native.not_modified && !native.no_content {
        let bytes = if response.byte_count == 0 {
            Vec::new()
        } else {
            // SAFETY: C API contract: `bytes` holds `byte_count` bytes.
            unsafe { std::slice::from_raw_parts(response.bytes, response.byte_count) }.to_vec()
        };
        native.data = Some(Arc::new(bytes));
    }
    native
}

/// The live request that `handle` names. A request that its own state
/// retired is stale, as the table reports once it removes it.
fn live_request(handle: mln_resource_request) -> Result<Arc<ResourceRequestObject>, Failure> {
    handles::resource_request(handle)
}

fn stale(handle: u64) -> Failure {
    handles::stale(HandleKind::ResourceRequest, handle)
}

/// # Safety
/// C API contract (runtime.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_resource_request_complete(
    handle: mln_resource_request,
    response: *const mln_resource_response,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            if response.is_null() {
                return Err(Failure::invalid_argument("response must not be null"));
            }
            if (*response.cast::<u32>()) as usize >= size_of::<mln_resource_response>() {
                validate_c_string_view((*response).etag, "etag")?;
                validate_c_string_view((*response).error_message, "error_message")?;
            }
            let object = live_request(handle)?;
            {
                let mut state = object.state.lock().unwrap();
                if state.retired {
                    return Err(stale(handle.0));
                }
                if state.completed {
                    return Err(Failure::invalid_state("resource request is already completed"));
                }
                if state.cancelled {
                    return Err(Failure::invalid_state("resource request is cancelled"));
                }
                // A response smaller than the struct reads only its size.
                let mut native = if (*response.cast::<u32>()) as usize >= size_of::<mln_resource_response>() {
                    response_from_abi(&*response)
                } else {
                    error_response(Reason::Other, "mln_resource_response.size is too small")
                };
                // Revalidation delivers the cached bytes that the cache
                // withheld, as the online file source does for a 304.
                if native.not_modified && object.prior_data.is_some() {
                    native.data = object.prior_data.clone();
                    if native.modified.is_none() {
                        native.modified = object.prior_modified;
                    }
                    if native.etag.is_none() {
                        native.etag = object.prior_etag.clone();
                    }
                    native.not_modified = false;
                }
                state.completed = true;
                object.response.invoke(move |request| request.set_response(native));
            }
            // An answered request consumes its handle.
            retire_request(handle.0);
            Ok(())
        })
    }
}

/// # Safety
/// C API contract (runtime.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_resource_request_pass_through(
    handle: mln_resource_request,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let object = live_request(handle)?;
            {
                let mut state = object.state.lock().unwrap();
                if state.retired {
                    return Err(stale(handle.0));
                }
                if state.completed || state.cancelled {
                    return Err(Failure::invalid_state("resource request is completed or cancelled"));
                }
                object.control.invoke(|loader| loader.pass_through());
                state.completed = true;
            }
            retire_request(handle.0);
            Ok(())
        })
    }
}

/// # Safety
/// C API contract (runtime.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_resource_request_is_cancelled(
    handle: mln_resource_request,
    out_cancelled: *mut bool,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            if out_cancelled.is_null() {
                return Err(Failure::invalid_argument("out_cancelled must not be null"));
            }
            let object = live_request(handle)?;
            let state = object.state.lock().unwrap();
            if state.retired {
                return Err(stale(handle.0));
            }
            *out_cancelled = state.cancelled && !state.completed;
            Ok(())
        })
    }
}

/// # Safety
/// C API contract (runtime.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_resource_request_set_cancel_callback(
    handle: mln_resource_request,
    handler: *const mln_resource_request_cancel_handler,
    out_cancelled: *mut bool,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            if handler.is_null() {
                return Err(Failure::invalid_argument("cancel handler must not be null"));
            }
            let Some(handler) = read_sized(handler) else {
                return Err(Failure::invalid_argument("mln_resource_request_cancel_handler.size is too small"));
            };
            if out_cancelled.is_null() {
                return Err(Failure::invalid_argument("out_cancelled must not be null"));
            }
            if handler.callback.is_none() {
                return Err(Failure::invalid_argument("cancel handler requires a callback"));
            }
            if handler.queue.0 != 0 {
                handles::queue(handler.queue)?;
            }
            let object = live_request(handle)?;
            let mut state = object.state.lock().unwrap();
            if state.retired {
                return Err(stale(handle.0));
            }
            if state.cancel_callback_registered {
                return Err(Failure::invalid_state("resource request already has a cancel callback"));
            }
            state.cancel_callback_registered = true;
            *out_cancelled = state.cancelled && !state.completed;
            if !*out_cancelled {
                state.cancel = Some(CancelRegistration {
                    callback: handler.callback,
                    user_data: UserData(handler.user_data),
                    release_user_data: handler.release_user_data,
                    queue: handler.queue.0,
                });
            }
            Ok(())
        })
    }
}

/// # Safety
/// C API contract (runtime.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_resource_request_release(handle: mln_resource_request) {
    if let Some(Object::ResourceRequest(object)) = handles::get(handle) {
        let mut state = object.state.lock().unwrap();
        if !state.retired {
            if state.claimed {
                object.fail_locked(&mut state, Failure_::Released);
            } else if state.deciding {
                state.released_while_deciding = true;
            }
        }
    }
    retire_request(handle.0);
}

/// Swaps the runtime's provider on its thread and completes (C++
/// `submit_resource_configuration_command`).
fn replace(
    runtime: mln_runtime,
    completion: Option<Completion>,
    next: Option<Arc<ProviderRegistration>>,
) -> Result<(), Failure> {
    let shared = handles::runtime(runtime)?;
    let state = shared.resources.clone();
    shared.post(move || {
        let previous = state.replace_provider(next);
        drop(previous);
        if let Some(completion) = completion {
            completion.ok();
        }
    });
    Ok(())
}

/// # Safety
/// C API contract (runtime.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_runtime_set_resource_provider(
    runtime: mln_runtime,
    provider: *const mln_resource_provider,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            handles::runtime(runtime)?;
            if provider.is_null() {
                return Err(Failure::invalid_argument("provider must not be null"));
            }
            let Some(provider) = read_sized(provider) else {
                return Err(Failure::invalid_argument("mln_resource_provider.size is too small"));
            };
            if provider.callback.is_none() {
                return Err(Failure::invalid_argument("provider requires a callback"));
            }
            if provider.queue.0 != 0 {
                handles::queue(provider.queue)?;
            }
            let routes = copy_routes(provider.routes, provider.route_count)?;
            let completion = Completion::optional(completion)?;
            let registration = ProviderRegistration {
                registration: Registration::new(
                    provider.callback,
                    UserData(provider.user_data),
                    provider.release_user_data,
                    provider.queue.0,
                ),
                routes,
            };
            replace(runtime, completion, Some(Arc::new(registration)))
        })
    }
}

/// # Safety
/// C API contract (runtime.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_runtime_clear_resource_provider(
    runtime: mln_runtime,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            handles::runtime(runtime)?;
            let completion = Completion::optional(completion)?;
            replace(runtime, completion, None)
        })
    }
}
