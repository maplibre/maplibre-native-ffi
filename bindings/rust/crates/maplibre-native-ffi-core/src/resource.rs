use std::ffi::{CString, c_char, c_void};
use std::fmt;
use std::marker::PhantomData;
use std::panic::{AssertUnwindSafe, catch_unwind};
use std::ptr;
use std::sync::{Arc, Mutex, Weak};

use maplibre_native_ffi_sys as sys;

use crate::enums::{
    resource_kind_from_raw, resource_loading_method_from_raw, resource_priority_from_raw,
    resource_storage_policy_from_raw, resource_usage_from_raw,
};
use crate::{
    Error, ErrorKind, ResourceErrorReason, ResourceKind, ResourceLoadingMethod, ResourcePriority,
    ResourceResponseStatus, ResourceStoragePolicy, ResourceUsage, Result,
};

/// Byte range requested for a network resource.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub struct ByteRange {
    pub start: u64,
    pub end: u64,
}

/// Copied request passed to a runtime-scoped resource provider callback.
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub struct ResourceRequest {
    /// URL entering the network layer, preserving configured scheme aliases.
    pub requested_url: String,
    /// URL to fetch, after tile server normalization.
    pub resolved_url: String,
    pub kind: ResourceKind,
    pub raw_kind: u32,
    pub loading_method: ResourceLoadingMethod,
    pub raw_loading_method: u32,
    pub priority: ResourcePriority,
    pub raw_priority: u32,
    pub usage: ResourceUsage,
    pub raw_usage: u32,
    pub storage_policy: ResourceStoragePolicy,
    pub raw_storage_policy: u32,
    pub range: Option<ByteRange>,
    pub prior_modified_unix_ms: Option<i64>,
    pub prior_expires_unix_ms: Option<i64>,
    pub prior_etag: Option<String>,
    pub prior_data: Vec<u8>,
}

/// Copies a borrowed native resource request into owned Rust data.
///
/// # Safety
///
/// `raw` and all nested pointers must remain valid for the duration of this
/// call. Resource provider trampolines typically receive this storage from the
/// C callback and copy it before returning.
pub unsafe fn copy_resource_request(raw: &sys::mln_resource_request) -> Result<ResourceRequest> {
    let prior_data = if raw.prior_data_size == 0 {
        Vec::new()
    } else if raw.prior_data.is_null() {
        return Err(Error::invalid_argument(
            "resource request prior_data must not be null when prior_data_size is nonzero",
        ));
    } else {
        // SAFETY: The caller promised raw and nested request storage are valid
        // for this call; copy the borrowed bytes immediately.
        unsafe { std::slice::from_raw_parts(raw.prior_data, raw.prior_data_size) }.to_vec()
    };

    let prior_etag = if raw.prior_etag.is_null() {
        None
    } else {
        // SAFETY: The caller promised raw points to callback-duration storage.
        Some(unsafe { crate::string::copy_c_string(raw.prior_etag) }?)
    };

    Ok(ResourceRequest {
        // SAFETY: The caller promised raw points to callback-duration storage.
        requested_url: unsafe { crate::string::copy_c_string(raw.requested_url) }?,
        // SAFETY: The caller promised raw points to callback-duration storage.
        resolved_url: unsafe { crate::string::copy_c_string(raw.resolved_url) }?,
        kind: resource_kind_from_raw(raw.kind),
        raw_kind: raw.kind,
        loading_method: resource_loading_method_from_raw(raw.loading_method),
        raw_loading_method: raw.loading_method,
        priority: resource_priority_from_raw(raw.priority),
        raw_priority: raw.priority,
        usage: resource_usage_from_raw(raw.usage),
        raw_usage: raw.usage,
        storage_policy: resource_storage_policy_from_raw(raw.storage_policy),
        raw_storage_policy: raw.storage_policy,
        range: raw.has_range.then_some(ByteRange {
            start: raw.range_start,
            end: raw.range_end,
        }),
        prior_modified_unix_ms: raw.has_prior_modified.then_some(raw.prior_modified_unix_ms),
        prior_expires_unix_ms: raw.has_prior_expires.then_some(raw.prior_expires_unix_ms),
        prior_etag,
        prior_data,
    })
}

/// Decision returned by a resource provider callback.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum ResourceProviderDecision {
    /// Let native OnlineFileSource handle the request.
    PassThrough,
    /// Keep ownership of the request handle and complete or release it later.
    Handle,
}

/// Response used to complete a handled resource request.
#[derive(Debug, Clone, PartialEq, Eq)]
#[non_exhaustive]
pub struct ResourceResponse {
    pub status: ResourceResponseStatus,
    pub error_reason: ResourceErrorReason,
    pub bytes: Vec<u8>,
    pub error_message: Option<String>,
    pub must_revalidate: bool,
    pub modified_unix_ms: Option<i64>,
    pub expires_unix_ms: Option<i64>,
    pub etag: Option<String>,
    pub retry_after_unix_ms: Option<i64>,
}

impl ResourceResponse {
    pub fn ok(bytes: impl Into<Vec<u8>>) -> Self {
        Self {
            status: ResourceResponseStatus::Ok,
            bytes: bytes.into(),
            ..Self::default()
        }
    }

    pub fn no_content() -> Self {
        Self {
            status: ResourceResponseStatus::NoContent,
            ..Self::default()
        }
    }

    pub fn not_modified() -> Self {
        Self {
            status: ResourceResponseStatus::NotModified,
            ..Self::default()
        }
    }

    pub fn error(reason: ResourceErrorReason, message: impl Into<String>) -> Self {
        Self {
            status: ResourceResponseStatus::Error,
            error_reason: reason,
            error_message: Some(message.into()),
            ..Self::default()
        }
    }
}

impl Default for ResourceResponse {
    fn default() -> Self {
        Self {
            status: ResourceResponseStatus::Ok,
            error_reason: ResourceErrorReason::None,
            bytes: Vec::new(),
            error_message: None,
            must_revalidate: false,
            modified_unix_ms: None,
            expires_unix_ms: None,
            etag: None,
            retry_after_unix_ms: None,
        }
    }
}

pub struct NativeResourceResponse<'a> {
    raw: sys::mln_resource_response,
    _response: PhantomData<&'a ResourceResponse>,
    _error_message: Option<CString>,
    _etag: Option<CString>,
}

impl<'a> NativeResourceResponse<'a> {
    fn new(response: &'a ResourceResponse) -> Result<Self> {
        let error_message = response
            .error_message
            .as_deref()
            .map(crate::string::c_string)
            .transpose()?;
        let etag = response
            .etag
            .as_deref()
            .map(crate::string::c_string)
            .transpose()?;
        Ok(Self {
            raw: sys::mln_resource_response {
                size: std::mem::size_of::<sys::mln_resource_response>() as u32,
                status: response.status.as_raw(),
                error_reason: response.error_reason.raw_value(),
                bytes: if response.bytes.is_empty() {
                    ptr::null()
                } else {
                    response.bytes.as_ptr()
                },
                byte_count: response.bytes.len(),
                error_message: error_message
                    .as_ref()
                    .map_or(ptr::null(), |message| message.as_ptr()),
                must_revalidate: response.must_revalidate,
                has_modified: response.modified_unix_ms.is_some(),
                modified_unix_ms: response.modified_unix_ms.unwrap_or_default(),
                has_expires: response.expires_unix_ms.is_some(),
                expires_unix_ms: response.expires_unix_ms.unwrap_or_default(),
                etag: etag.as_ref().map_or(ptr::null(), |etag| etag.as_ptr()),
                has_retry_after: response.retry_after_unix_ms.is_some(),
                retry_after_unix_ms: response.retry_after_unix_ms.unwrap_or_default(),
            },
            _response: PhantomData,
            _error_message: error_message,
            _etag: etag,
        })
    }

    pub fn as_ptr(&self) -> *const sys::mln_resource_response {
        &self.raw
    }
}

impl AsRef<sys::mln_resource_response> for NativeResourceResponse<'_> {
    fn as_ref(&self) -> &sys::mln_resource_response {
        &self.raw
    }
}

pub fn resource_response_to_native(
    response: &ResourceResponse,
) -> Result<NativeResourceResponse<'_>> {
    NativeResourceResponse::new(response)
}

pub type ResourceProviderCallbackFn = unsafe extern "C" fn(
    *mut c_void,
    *const sys::mln_resource_request,
    sys::mln_resource_request_handle,
) -> u32;

pub fn resource_provider_descriptor(
    callback: Option<ResourceProviderCallbackFn>,
    user_data: *mut c_void,
) -> sys::mln_resource_provider {
    sys::mln_resource_provider {
        size: std::mem::size_of::<sys::mln_resource_provider>() as u32,
        callback,
        user_data,
    }
}

pub type ResourceTransformCallbackFn = unsafe extern "C" fn(
    *mut c_void,
    u32,
    *const c_char,
    *mut sys::mln_resource_transform_response,
) -> sys::mln_status;

pub fn resource_transform_descriptor(
    callback: Option<ResourceTransformCallbackFn>,
    user_data: *mut c_void,
) -> sys::mln_resource_transform {
    sys::mln_resource_transform {
        size: std::mem::size_of::<sys::mln_resource_transform>() as u32,
        callback,
        user_data,
    }
}

pub type HttpHeaderTransformCallbackFn = unsafe extern "C" fn(
    *mut c_void,
    u32,
    *const c_char,
    *mut sys::mln_http_header_transform_response,
) -> sys::mln_status;

pub fn http_header_transform_descriptor(
    callback: Option<HttpHeaderTransformCallbackFn>,
    user_data: *mut c_void,
) -> sys::mln_http_header_transform {
    sys::mln_http_header_transform {
        size: std::mem::size_of::<sys::mln_http_header_transform>() as u32,
        callback,
        user_data,
    }
}

/// Initializes an HTTP header transform callback response.
///
/// # Safety
///
/// `out_response` must be null or point to writable callback-duration storage.
pub unsafe fn initialize_http_header_transform_response(
    out_response: *mut sys::mln_http_header_transform_response,
) -> sys::mln_status {
    if out_response.is_null() {
        return sys::MLN_STATUS_INVALID_ARGUMENT;
    }
    // SAFETY: The caller promised writable callback-duration storage.
    unsafe {
        (*out_response).size =
            std::mem::size_of::<sys::mln_http_header_transform_response>() as u32;
    }
    sys::MLN_STATUS_OK
}

/// Initializes a resource transform callback response to an empty replacement.
///
/// # Safety
///
/// `out_response` must be null or point to writable callback-duration storage.
pub unsafe fn initialize_resource_transform_response(
    out_response: *mut sys::mln_resource_transform_response,
) -> sys::mln_status {
    if out_response.is_null() {
        return sys::MLN_STATUS_INVALID_ARGUMENT;
    }
    // SAFETY: The caller promised writable callback-duration storage, and the
    // null check above guards the write.
    unsafe {
        (*out_response).size = std::mem::size_of::<sys::mln_resource_transform_response>() as u32;
        (*out_response).url = ptr::null();
    }
    sys::MLN_STATUS_OK
}

pub fn status_for_error(error: &Error) -> sys::mln_status {
    if let Some(status) = error.raw_status() {
        return status;
    }
    match error.kind() {
        ErrorKind::InvalidArgument => sys::MLN_STATUS_INVALID_ARGUMENT,
        ErrorKind::InvalidState => sys::MLN_STATUS_INVALID_STATE,
        ErrorKind::WrongThread => sys::MLN_STATUS_WRONG_THREAD,
        ErrorKind::Unsupported => sys::MLN_STATUS_UNSUPPORTED,
        ErrorKind::NativeError | ErrorKind::AbiVersionMismatch | ErrorKind::UnknownStatus => {
            sys::MLN_STATUS_NATIVE_ERROR
        }
    }
}

pub const UNKNOWN_PROVIDER_DECISION: u32 = u32::MAX;

pub type CompleteRequestFn = unsafe extern "C" fn(
    sys::mln_resource_request_handle,
    *const sys::mln_resource_response,
) -> sys::mln_status;
pub type CancelledRequestFn =
    unsafe extern "C" fn(sys::mln_resource_request_handle, *mut bool) -> sys::mln_status;
pub type ReleaseRequestFn = unsafe extern "C" fn(sys::mln_resource_request_handle);
pub type SetCancelCallbackFn = unsafe extern "C" fn(
    sys::mln_resource_request_handle,
    sys::mln_resource_request_cancel_callback,
    *mut c_void,
    *mut bool,
) -> sys::mln_status;

/// Host callback that runs once when MapLibre cancels a handled request.
pub type CancelCallback = dyn FnOnce() + Send + 'static;

#[derive(Clone, Copy, Debug)]
pub struct ResourceRequestHandleFns {
    complete: CompleteRequestFn,
    cancelled: CancelledRequestFn,
    set_cancel_callback: SetCancelCallbackFn,
    release: ReleaseRequestFn,
}

impl ResourceRequestHandleFns {
    pub const NATIVE: Self = Self {
        complete: sys::mln_resource_request_complete,
        cancelled: sys::mln_resource_request_cancelled,
        set_cancel_callback: sys::mln_resource_request_set_cancel_callback,
        release: sys::mln_resource_request_release,
    };

    /// Creates a function table for a native resource request handle.
    ///
    /// # Safety
    ///
    /// The functions must implement the same ownership contract as the C API:
    /// `complete`, `cancelled`, and `set_cancel_callback` operate on the
    /// matching handle type, `set_cancel_callback` never invokes the callback
    /// and stops using its `user_data` once `release` returns, and `release`
    /// releases a provider-owned handle exactly once.
    pub const unsafe fn new(
        complete: CompleteRequestFn,
        cancelled: CancelledRequestFn,
        set_cancel_callback: SetCancelCallbackFn,
        release: ReleaseRequestFn,
    ) -> Self {
        Self {
            complete,
            cancelled,
            set_cancel_callback,
            release,
        }
    }
}

/// Runs the host cancel callback for a request.
///
/// # Safety
///
/// `user_data` must be the pointer `ResourceRequestHandleState` registered
/// with the C API, which the C API stops using once the request's release
/// returns.
unsafe extern "C" fn cancel_callback_trampoline(user_data: *mut c_void) {
    let Some(token) = ptr::NonNull::new(user_data.cast::<ResourceRequestHandleState>()) else {
        return;
    };
    // SAFETY: The token came from Weak::into_raw in set_cancel_callback, and
    // the state reclaims it only after native release returns, when the C API
    // no longer invokes this callback. Handing the weak reference back through
    // into_raw leaves the registration's count untouched.
    let weak = unsafe { Weak::from_raw(token.as_ptr()) };
    let state = weak.upgrade();
    let _ = Weak::into_raw(weak);
    if let Some(state) = state {
        state.run_cancel_callback();
    }
}

/// Runs a host cancel callback with no binding lock held. A panic is contained
/// here: unwinding into C is undefined behavior, and the cancel path has no
/// status to report the failure through.
fn run_cancel_callback_contained(callback: Box<CancelCallback>) {
    let _ = catch_unwind(AssertUnwindSafe(callback));
}

struct ResourceRequestHandleInner {
    handle: u64,
    decision_finalized: bool,
    provider_owned: bool,
    release_accounted_for: bool,
    closed: bool,
    completed: bool,
    /// The registered host callback. Taken before it runs, so it runs once.
    cancel_callback: Option<Box<CancelCallback>>,
    /// `Weak<ResourceRequestHandleState>` handed to the C API as `user_data`,
    /// or zero before the first registration. Reclaimed when the state drops.
    cancel_token: usize,
}

impl fmt::Debug for ResourceRequestHandleInner {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("ResourceRequestHandleInner")
            .field("handle", &self.handle)
            .field("decision_finalized", &self.decision_finalized)
            .field("provider_owned", &self.provider_owned)
            .field("release_accounted_for", &self.release_accounted_for)
            .field("closed", &self.closed)
            .field("completed", &self.completed)
            .field("cancel_registered", &self.cancel_callback.is_some())
            .finish()
    }
}

/// Shared state behind a resource request handle.
///
/// The `inner` lock is never held while host code runs or while native release
/// runs: native release waits for a cancel callback running on another thread,
/// and that callback may call back into this same state.
#[derive(Debug)]
pub struct ResourceRequestHandleState {
    inner: Mutex<ResourceRequestHandleInner>,
    fns: ResourceRequestHandleFns,
}

impl ResourceRequestHandleState {
    /// Takes ownership of a native resource request handle state machine.
    ///
    /// # Safety
    ///
    /// `handle` must be a live native resource request handle borrowed from a
    /// provider callback, and `fns` must match that handle.
    pub unsafe fn new(
        handle: sys::mln_resource_request_handle,
        fns: ResourceRequestHandleFns,
    ) -> Result<Arc<Self>> {
        if handle.0 == 0 {
            return Err(Error::invalid_argument(
                "resource request handle must not be the null handle",
            ));
        }
        Ok(Arc::new(Self {
            inner: Mutex::new(ResourceRequestHandleInner {
                handle: handle.0,
                decision_finalized: false,
                provider_owned: false,
                release_accounted_for: false,
                closed: false,
                completed: false,
                cancel_callback: None,
                cancel_token: 0,
            }),
            fns,
        }))
    }

    fn native_handle(inner: &ResourceRequestHandleInner) -> sys::mln_resource_request_handle {
        sys::mln_resource_request_handle(inner.handle)
    }

    pub fn complete(&self, response: &ResourceResponse) -> Result<()> {
        let native = resource_response_to_native(response)?;
        let mut inner = self.lock_inner()?;
        if inner.completed {
            return Err(Error::new(
                ErrorKind::InvalidState,
                None,
                "ResourceRequestHandle is already completed",
            ));
        }
        if inner.closed {
            return Err(Error::invalid_argument("ResourceRequestHandle is closed"));
        }

        inner.completed = true;
        inner.closed = true;
        let handle = Self::native_handle(&inner);
        // SAFETY: handle is live while not closed/released, and native response
        // points to storage retained for this call. The C API copies contents.
        let status = unsafe { (self.fns.complete)(handle, native.as_ptr()) };
        // A completed request never runs its cancel callback.
        let callback = inner.cancel_callback.take();
        let release = inner.decision_finalized
            && inner.provider_owned
            && Self::take_release_locked(&mut inner);
        drop(inner);
        self.release_now(release, handle);
        drop(callback);
        crate::check(status)
    }

    /// Registers the host callback that runs when MapLibre cancels the request.
    ///
    /// A request accepts one registration. When the C API reports that the
    /// request was already cancelled, the callback runs before this returns.
    pub fn set_cancel_callback(self: &Arc<Self>, callback: Box<CancelCallback>) -> Result<()> {
        let mut inner = self.lock_inner()?;
        if inner.closed {
            return Err(Error::invalid_argument("ResourceRequestHandle is closed"));
        }
        if inner.cancel_callback.is_some() {
            return Err(Error::new(
                ErrorKind::InvalidState,
                None,
                "ResourceRequestHandle already has a cancel callback",
            ));
        }
        inner.cancel_callback = Some(callback);
        if inner.cancel_token == 0 {
            inner.cancel_token = Weak::into_raw(Arc::downgrade(self)) as usize;
        }
        let mut cancelled = false;
        // The native setter never blocks or calls back into the host, so the
        // lock stays held across it and a concurrent close waits for this
        // registration like any other in-flight use.
        // SAFETY: handle is live while not closed. user_data is a weak
        // reference this state reclaims only after native release returns, and
        // cancelled points to writable bool storage for this call.
        let status = unsafe {
            (self.fns.set_cancel_callback)(
                Self::native_handle(&inner),
                Some(cancel_callback_trampoline),
                inner.cancel_token as *mut c_void,
                &mut cancelled,
            )
        };
        if let Err(error) = crate::check(status) {
            // Native stored nothing, so the slot goes back to empty.
            drop(inner.cancel_callback.take());
            return Err(error);
        }
        // Native stored nothing for a request MapLibre already cancelled, so
        // this call runs the callback itself. Taking it back under the lock
        // keeps a racing close from dropping it unrun.
        let inline = if cancelled {
            inner.cancel_callback.take()
        } else {
            None
        };
        drop(inner);
        if let Some(callback) = inline {
            run_cancel_callback_contained(callback);
        }
        Ok(())
    }

    pub fn is_cancelled(&self) -> Result<bool> {
        let inner = self.lock_inner()?;
        if inner.closed {
            return Err(Error::invalid_argument("ResourceRequestHandle is closed"));
        }
        let mut cancelled = false;
        // SAFETY: handle is live while not closed/released, and cancelled points
        // to writable bool storage.
        crate::check(unsafe { (self.fns.cancelled)(Self::native_handle(&inner), &mut cancelled) })?;
        Ok(cancelled)
    }

    pub fn close(&self) {
        let Ok(mut inner) = self.inner.lock() else {
            return;
        };
        if inner.closed {
            return;
        }
        inner.closed = true;
        let callback = inner.cancel_callback.take();
        let handle = Self::native_handle(&inner);
        let release = inner.decision_finalized
            && inner.provider_owned
            && Self::take_release_locked(&mut inner);
        drop(inner);
        self.release_now(release, handle);
        drop(callback);
    }

    pub fn finish_provider_decision(&self, decision: ResourceProviderDecision) -> u32 {
        let Ok(mut inner) = self.inner.lock() else {
            return UNKNOWN_PROVIDER_DECISION;
        };
        if inner.decision_finalized {
            return if inner.provider_owned {
                sys::MLN_RESOURCE_PROVIDER_DECISION_HANDLE
            } else {
                sys::MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH
            };
        }
        if inner.completed || matches!(decision, ResourceProviderDecision::Handle) {
            inner.decision_finalized = true;
            inner.provider_owned = true;
            let handle = Self::native_handle(&inner);
            let release = inner.closed && Self::take_release_locked(&mut inner);
            drop(inner);
            self.release_now(release, handle);
            sys::MLN_RESOURCE_PROVIDER_DECISION_HANDLE
        } else {
            // The C API releases a passed-through request itself, and its
            // release retires any cancel registration.
            let callback = Self::finish_unowned_locked(&mut inner);
            drop(inner);
            drop(callback);
            sys::MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH
        }
    }

    pub fn finish_provider_exception(&self) -> u32 {
        let completed = self
            .inner
            .lock()
            .map(|inner| inner.completed)
            .unwrap_or(false);
        if completed {
            return self.finish_provider_decision(ResourceProviderDecision::Handle);
        }
        if let Ok(mut inner) = self.inner.lock() {
            // The C API releases the request it gets no decision for.
            let callback = Self::finish_unowned_locked(&mut inner);
            drop(inner);
            drop(callback);
        }
        UNKNOWN_PROVIDER_DECISION
    }

    /// Records a decision that leaves the release to the C API, returning the
    /// cancel callback that can no longer run.
    fn finish_unowned_locked(
        inner: &mut ResourceRequestHandleInner,
    ) -> Option<Box<CancelCallback>> {
        inner.decision_finalized = true;
        inner.release_accounted_for = true;
        inner.closed = true;
        inner.cancel_callback.take()
    }

    fn take_release_locked(inner: &mut ResourceRequestHandleInner) -> bool {
        if inner.release_accounted_for {
            return false;
        }
        inner.release_accounted_for = true;
        true
    }

    fn take_cancel_callback(&self) -> Option<Box<CancelCallback>> {
        self.inner
            .lock()
            .unwrap_or_else(|poisoned| poisoned.into_inner())
            .cancel_callback
            .take()
    }

    /// Runs the registered callback once, outside the handle lock, so it can
    /// complete or close this same request.
    fn run_cancel_callback(&self) {
        if let Some(callback) = self.take_cancel_callback() {
            run_cancel_callback_contained(callback);
        }
    }

    /// Calls native release with no lock held. Release waits for a cancel
    /// callback running on another thread, and that callback may take the lock.
    fn release_now(&self, release: bool, handle: sys::mln_resource_request_handle) {
        if !release {
            return;
        }
        // SAFETY: take_release_locked grants this call exactly once per handle.
        unsafe { (self.fns.release)(handle) };
    }

    fn lock_inner(&self) -> Result<std::sync::MutexGuard<'_, ResourceRequestHandleInner>> {
        self.inner.lock().map_err(|_| {
            Error::new(
                ErrorKind::NativeError,
                None,
                "ResourceRequestHandle lock poisoned",
            )
        })
    }
}

impl Drop for ResourceRequestHandleState {
    fn drop(&mut self) {
        let inner = self
            .inner
            .get_mut()
            .unwrap_or_else(|poisoned| poisoned.into_inner());
        let handle = Self::native_handle(inner);
        let release = inner.provider_owned && Self::take_release_locked(inner);
        let token = inner.cancel_token;
        self.release_now(release, handle);
        if token != 0 {
            // SAFETY: The token came from Weak::into_raw in set_cancel_callback.
            // Every path that lets this state drop has released the native
            // request first, and the C API stops using user_data once release
            // returns, so no cancel callback can still be arriving.
            drop(unsafe { Weak::from_raw(token as *const Self) });
        }
    }
}

/// Copied request passed to a runtime-scoped resource transform callback.
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub struct ResourceTransformRequest {
    pub kind: ResourceKind,
    pub raw_kind: u32,
    pub url: String,
}

/// Copied request passed to an outgoing HTTP header transform callback.
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub struct HttpHeaderTransformRequest {
    pub kind: ResourceKind,
    pub raw_kind: u32,
    pub url: String,
}

/// One owned outgoing HTTP request header.
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub struct HttpHeader {
    pub name: String,
    pub value: String,
}

impl HttpHeader {
    pub fn new(name: impl Into<String>, value: impl Into<String>) -> Self {
        Self {
            name: name.into(),
            value: value.into(),
        }
    }
}

/// Copies an HTTP header transform request into owned Rust data.
///
/// # Safety
///
/// `url` must point to a valid NUL-terminated string for this call.
pub unsafe fn copy_http_header_transform_request(
    raw_kind: u32,
    url: *const c_char,
) -> Result<HttpHeaderTransformRequest> {
    // SAFETY: The caller promises url follows the C callback contract.
    let request_url = unsafe { crate::string::copy_c_string(url) }?;
    Ok(HttpHeaderTransformRequest {
        kind: resource_kind_from_raw(raw_kind),
        raw_kind,
        url: request_url,
    })
}

/// Copies a resource transform callback request into owned Rust data.
///
/// # Safety
///
/// `url` must be null or point to a valid NUL-terminated string for the
/// duration of this call.
pub unsafe fn copy_resource_transform_request(
    raw_kind: u32,
    url: *const c_char,
) -> Result<ResourceTransformRequest> {
    // SAFETY: The caller promises url follows the C callback contract.
    let request_url = unsafe { crate::string::copy_c_string(url) }?;
    Ok(ResourceTransformRequest {
        kind: resource_kind_from_raw(raw_kind),
        raw_kind,
        url: request_url,
    })
}

#[cfg(test)]
mod tests {
    use std::ffi::CString;
    use std::sync::Mutex as StdMutex;
    use std::sync::atomic::{AtomicBool, AtomicI32, AtomicUsize, Ordering};
    use std::time::{Duration, Instant};

    use super::*;

    static HANDLE_TEST_LOCK: StdMutex<()> = StdMutex::new(());
    static COMPLETE_COUNT: AtomicUsize = AtomicUsize::new(0);
    static RELEASE_COUNT: AtomicUsize = AtomicUsize::new(0);
    static COMPLETE_STATUS: AtomicI32 = AtomicI32::new(sys::MLN_STATUS_OK);
    static CANCELLED_SLEEP_MS: AtomicUsize = AtomicUsize::new(0);
    static CANCELLED_STARTED: AtomicBool = AtomicBool::new(false);
    static CANCELLED_FINISHED: AtomicBool = AtomicBool::new(false);
    static SET_CANCEL_COUNT: AtomicUsize = AtomicUsize::new(0);
    static ALREADY_CANCELLED: AtomicBool = AtomicBool::new(false);
    /// The registration the fake C API stored, as (callback, user_data).
    static REGISTERED_CANCEL: StdMutex<Option<(sys::mln_resource_request_cancel_callback, usize)>> =
        StdMutex::new(None);
    /// When set, the fake release waits for this flag like the C API waits for
    /// a cancel callback running on another thread.
    static RELEASE_WAITS_FOR_CALLBACK: AtomicBool = AtomicBool::new(false);
    static CALLBACK_FINISHED: AtomicBool = AtomicBool::new(false);

    unsafe extern "C" fn fake_complete(
        _handle: sys::mln_resource_request_handle,
        _response: *const sys::mln_resource_response,
    ) -> sys::mln_status {
        COMPLETE_COUNT.fetch_add(1, Ordering::SeqCst);
        COMPLETE_STATUS.load(Ordering::SeqCst)
    }

    unsafe extern "C" fn fake_cancelled(
        _handle: sys::mln_resource_request_handle,
        out_cancelled: *mut bool,
    ) -> sys::mln_status {
        if out_cancelled.is_null() {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        let sleep_ms = CANCELLED_SLEEP_MS.load(Ordering::SeqCst);
        if sleep_ms != 0 {
            CANCELLED_STARTED.store(true, Ordering::SeqCst);
            std::thread::sleep(Duration::from_millis(sleep_ms as u64));
            CANCELLED_FINISHED.store(true, Ordering::SeqCst);
        }
        // SAFETY: out_cancelled is non-null and points to caller-owned output storage.
        unsafe { *out_cancelled = ALREADY_CANCELLED.load(Ordering::SeqCst) };
        sys::MLN_STATUS_OK
    }

    unsafe extern "C" fn fake_release(_handle: sys::mln_resource_request_handle) {
        if RELEASE_WAITS_FOR_CALLBACK.load(Ordering::SeqCst) {
            let deadline = Instant::now() + Duration::from_secs(5);
            while !CALLBACK_FINISHED.load(Ordering::SeqCst) && Instant::now() < deadline {
                std::thread::yield_now();
            }
        }
        *REGISTERED_CANCEL.lock().unwrap() = None;
        RELEASE_COUNT.fetch_add(1, Ordering::SeqCst);
    }

    /// Stands in for the C API's registration: it stores the callback for a
    /// live request and reports an already cancelled one without storing it.
    unsafe extern "C" fn fake_set_cancel_callback(
        _handle: sys::mln_resource_request_handle,
        callback: sys::mln_resource_request_cancel_callback,
        user_data: *mut c_void,
        out_cancelled: *mut bool,
    ) -> sys::mln_status {
        SET_CANCEL_COUNT.fetch_add(1, Ordering::SeqCst);
        if callback.is_none() || out_cancelled.is_null() {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        let cancelled = ALREADY_CANCELLED.load(Ordering::SeqCst);
        // SAFETY: out_cancelled is non-null and points to caller-owned output storage.
        unsafe { *out_cancelled = cancelled };
        if !cancelled {
            *REGISTERED_CANCEL.lock().unwrap() = Some((callback, user_data as usize));
        }
        sys::MLN_STATUS_OK
    }

    /// Invokes the stored registration the way the C API does when MapLibre
    /// discards the request.
    fn fire_registered_cancel() {
        let (callback, user_data) = REGISTERED_CANCEL
            .lock()
            .unwrap()
            .expect("a registered cancel callback");
        // SAFETY: The fake registration stored this pair from the state under
        // test, which is still alive and unreleased.
        unsafe { callback.unwrap()(user_data as *mut c_void) };
    }

    fn fake_fns() -> ResourceRequestHandleFns {
        // SAFETY: These fake functions implement the native handle contract for tests.
        unsafe {
            ResourceRequestHandleFns::new(
                fake_complete,
                fake_cancelled,
                fake_set_cancel_callback,
                fake_release,
            )
        }
    }

    fn fake_state() -> Arc<ResourceRequestHandleState> {
        COMPLETE_COUNT.store(0, Ordering::SeqCst);
        RELEASE_COUNT.store(0, Ordering::SeqCst);
        COMPLETE_STATUS.store(sys::MLN_STATUS_OK, Ordering::SeqCst);
        CANCELLED_SLEEP_MS.store(0, Ordering::SeqCst);
        CANCELLED_STARTED.store(false, Ordering::SeqCst);
        CANCELLED_FINISHED.store(false, Ordering::SeqCst);
        SET_CANCEL_COUNT.store(0, Ordering::SeqCst);
        ALREADY_CANCELLED.store(false, Ordering::SeqCst);
        *REGISTERED_CANCEL.lock().unwrap() = None;
        RELEASE_WAITS_FOR_CALLBACK.store(false, Ordering::SeqCst);
        CALLBACK_FINISHED.store(false, Ordering::SeqCst);
        // SAFETY: This synthetic handle reaches only the fake functions above,
        // never the C API.
        unsafe {
            ResourceRequestHandleState::new(
                sys::mln_resource_request_handle(0x0c00_0000_0000_0034),
                fake_fns(),
            )
        }
        .unwrap()
    }

    fn handled_fake_state() -> Arc<ResourceRequestHandleState> {
        let state = fake_state();
        assert_eq!(
            state.finish_provider_decision(ResourceProviderDecision::Handle),
            sys::MLN_RESOURCE_PROVIDER_DECISION_HANDLE
        );
        state
    }

    fn wait_until(flag: &AtomicBool, what: &str) {
        let deadline = Instant::now() + Duration::from_secs(5);
        while !flag.load(Ordering::SeqCst) {
            assert!(Instant::now() < deadline, "timed out waiting for {what}");
            std::thread::yield_now();
        }
    }
    #[test]
    fn resource_request_handle_preserves_all_64_bits() {
        let state = fake_state();
        let inner = state.lock_inner().unwrap();
        assert_eq!(
            ResourceRequestHandleState::native_handle(&inner).0,
            0x0c00_0000_0000_0034
        );
    }

    #[test]
    fn resource_request_copies_nested_storage() {
        let mut requested_url = CString::new("maplibre://tiles/2/1/1.pbf").unwrap();
        let mut resolved_url = CString::new("https://example.test/tile").unwrap();
        let mut etag = CString::new("abc").unwrap();
        let mut prior_data = [1_u8, 2, 3];
        let raw = sys::mln_resource_request {
            size: std::mem::size_of::<sys::mln_resource_request>() as u32,
            requested_url: requested_url.as_ptr(),
            resolved_url: resolved_url.as_ptr(),
            kind: sys::MLN_RESOURCE_KIND_TILE,
            loading_method: sys::MLN_RESOURCE_LOADING_METHOD_NETWORK_ONLY,
            priority: sys::MLN_RESOURCE_PRIORITY_LOW,
            usage: sys::MLN_RESOURCE_USAGE_ONLINE,
            storage_policy: sys::MLN_RESOURCE_STORAGE_POLICY_PERMANENT,
            has_range: true,
            range_start: 5,
            range_end: 10,
            has_prior_modified: true,
            prior_modified_unix_ms: 123,
            has_prior_expires: true,
            prior_expires_unix_ms: 456,
            prior_etag: etag.as_ptr(),
            prior_data: prior_data.as_ptr(),
            prior_data_size: prior_data.len(),
        };

        // SAFETY: raw points to live local backing storage for this call.
        let copied = unsafe { copy_resource_request(&raw) }.unwrap();
        requested_url = CString::new("maplibre://changed").unwrap();
        resolved_url = CString::new("https://changed.test").unwrap();
        etag = CString::new("changed").unwrap();
        prior_data.fill(9);

        assert_eq!(requested_url.as_bytes(), b"maplibre://changed");
        assert_eq!(resolved_url.as_bytes(), b"https://changed.test");
        assert_eq!(etag.as_bytes(), b"changed");
        assert_eq!(copied.requested_url, "maplibre://tiles/2/1/1.pbf");
        assert_eq!(copied.resolved_url, "https://example.test/tile");
        assert_eq!(copied.prior_etag.as_deref(), Some("abc"));
        assert_eq!(copied.prior_data, vec![1, 2, 3]);
        assert_eq!(copied.range, Some(ByteRange { start: 5, end: 10 }));
        assert_eq!(copied.kind, ResourceKind::Tile);
    }

    #[test]
    fn resource_request_rejects_nonempty_null_prior_data() {
        let url = CString::new("https://example.test/tile").unwrap();
        let raw = sys::mln_resource_request {
            size: std::mem::size_of::<sys::mln_resource_request>() as u32,
            requested_url: url.as_ptr(),
            resolved_url: url.as_ptr(),
            kind: sys::MLN_RESOURCE_KIND_TILE,
            loading_method: sys::MLN_RESOURCE_LOADING_METHOD_NETWORK_ONLY,
            priority: sys::MLN_RESOURCE_PRIORITY_LOW,
            usage: sys::MLN_RESOURCE_USAGE_ONLINE,
            storage_policy: sys::MLN_RESOURCE_STORAGE_POLICY_PERMANENT,
            has_range: false,
            range_start: 0,
            range_end: 0,
            has_prior_modified: false,
            prior_modified_unix_ms: 0,
            has_prior_expires: false,
            prior_expires_unix_ms: 0,
            prior_etag: ptr::null(),
            prior_data: ptr::null(),
            prior_data_size: 1,
        };

        // SAFETY: raw points to live local backing storage for this call.
        let Err(error) = (unsafe { copy_resource_request(&raw) }) else {
            panic!("nonempty null prior_data should fail");
        };
        assert!(error.to_string().contains("prior_data must not be null"));
    }

    #[test]
    fn resource_response_materializes_error_and_cache_fields() {
        let mut response = ResourceResponse::error(ResourceErrorReason::RateLimit, "slow down");
        response.must_revalidate = true;
        response.modified_unix_ms = Some(10);
        response.expires_unix_ms = Some(20);
        response.etag = Some("v1".into());
        response.retry_after_unix_ms = Some(30);

        let native = resource_response_to_native(&response).unwrap();
        let raw = native.as_ref();

        assert_eq!(raw.status, sys::MLN_RESOURCE_RESPONSE_STATUS_ERROR);
        assert_eq!(raw.error_reason, sys::MLN_RESOURCE_ERROR_REASON_RATE_LIMIT);
        assert!(raw.must_revalidate);
        assert!(raw.has_modified);
        assert!(raw.has_expires);
        assert!(raw.has_retry_after);
        assert!(!raw.error_message.is_null());
        assert!(!raw.etag.is_null());
    }

    #[test]
    fn resource_response_materializes_nonempty_bytes() {
        let response = ResourceResponse::ok([1, 2, 3]);

        let native = resource_response_to_native(&response).unwrap();
        let raw = native.as_ref();

        assert_eq!(raw.status, sys::MLN_RESOURCE_RESPONSE_STATUS_OK);
        assert_eq!(raw.byte_count, 3);
        assert!(!raw.bytes.is_null());
    }

    #[test]
    fn provider_decision_finalization_is_idempotent_for_owned_handles() {
        let _guard = HANDLE_TEST_LOCK.lock().unwrap();
        let state = fake_state();

        assert_eq!(
            state.finish_provider_decision(ResourceProviderDecision::Handle),
            sys::MLN_RESOURCE_PROVIDER_DECISION_HANDLE
        );
        assert_eq!(
            state.finish_provider_decision(ResourceProviderDecision::PassThrough),
            sys::MLN_RESOURCE_PROVIDER_DECISION_HANDLE
        );
        drop(state);

        assert_eq!(RELEASE_COUNT.load(Ordering::SeqCst), 1);
    }

    #[test]
    fn request_handle_rejects_double_successful_completion() {
        let _guard = HANDLE_TEST_LOCK.lock().unwrap();
        let state = fake_state();

        state.complete(&ResourceResponse::ok([1, 2, 3])).unwrap();
        let error = state
            .complete(&ResourceResponse::ok([4, 5, 6]))
            .unwrap_err();

        assert_eq!(error.kind(), ErrorKind::InvalidState);
        assert_eq!(COMPLETE_COUNT.load(Ordering::SeqCst), 1);
    }

    #[test]
    fn request_handle_completion_that_reaches_c_is_terminal_on_error() {
        let _guard = HANDLE_TEST_LOCK.lock().unwrap();
        let state = fake_state();
        assert_eq!(
            state.finish_provider_decision(ResourceProviderDecision::Handle),
            sys::MLN_RESOURCE_PROVIDER_DECISION_HANDLE
        );
        COMPLETE_STATUS.store(sys::MLN_STATUS_INVALID_STATE, Ordering::SeqCst);

        let error = state
            .complete(&ResourceResponse::ok([1, 2, 3]))
            .unwrap_err();

        assert_eq!(error.kind(), ErrorKind::InvalidState);
        assert_eq!(COMPLETE_COUNT.load(Ordering::SeqCst), 1);
        assert_eq!(RELEASE_COUNT.load(Ordering::SeqCst), 1);
        let error = state
            .complete(&ResourceResponse::ok([4, 5, 6]))
            .unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidState);
        assert_eq!(COMPLETE_COUNT.load(Ordering::SeqCst), 1);
    }

    #[test]
    fn request_release_waits_for_in_flight_cancellation_check() {
        let _guard = HANDLE_TEST_LOCK.lock().unwrap();
        let state = fake_state();
        assert_eq!(
            state.finish_provider_decision(ResourceProviderDecision::Handle),
            sys::MLN_RESOURCE_PROVIDER_DECISION_HANDLE
        );
        CANCELLED_SLEEP_MS.store(50, Ordering::SeqCst);
        let thread_state = Arc::clone(&state);
        let thread = std::thread::spawn(move || {
            thread_state.is_cancelled().unwrap();
        });
        let started_deadline = Instant::now() + Duration::from_secs(5);
        while !CANCELLED_STARTED.load(Ordering::SeqCst) {
            assert!(
                Instant::now() < started_deadline,
                "timed out waiting for cancellation check to start"
            );
            std::thread::yield_now();
        }

        state.close();

        assert!(CANCELLED_FINISHED.load(Ordering::SeqCst));
        assert_eq!(RELEASE_COUNT.load(Ordering::SeqCst), 1);
        thread.join().unwrap();
    }

    #[test]
    // Spec coverage: BND-198. The C API reports a request cancelled before
    // registration instead of storing the callback, so the binding runs the
    // callback itself, with no lock held, before registration returns.
    fn cancel_registration_on_a_cancelled_request_runs_the_callback_before_returning() {
        let _guard = HANDLE_TEST_LOCK.lock().unwrap();
        let state = handled_fake_state();
        ALREADY_CANCELLED.store(true, Ordering::SeqCst);
        COMPLETE_STATUS.store(sys::MLN_STATUS_INVALID_STATE, Ordering::SeqCst);
        let callback_state = Arc::clone(&state);
        let calls = Arc::new(AtomicUsize::new(0));
        let callback_calls = Arc::clone(&calls);

        state
            .set_cancel_callback(Box::new(move || {
                callback_calls.fetch_add(1, Ordering::SeqCst);
                assert_eq!(
                    callback_state
                        .complete(&ResourceResponse::no_content())
                        .unwrap_err()
                        .kind(),
                    ErrorKind::InvalidState
                );
                callback_state.close();
            }))
            .unwrap();

        assert_eq!(calls.load(Ordering::SeqCst), 1);
        assert_eq!(COMPLETE_COUNT.load(Ordering::SeqCst), 1);
        assert_eq!(RELEASE_COUNT.load(Ordering::SeqCst), 1);
    }

    #[test]
    // Spec coverage: BND-198. A closed request rejects registration, and a
    // second registration reports invalid state, both without reaching C.
    fn cancel_registration_rejects_closed_and_registered_requests() {
        let _guard = HANDLE_TEST_LOCK.lock().unwrap();
        let state = handled_fake_state();
        state.set_cancel_callback(Box::new(|| {})).unwrap();
        assert_eq!(SET_CANCEL_COUNT.load(Ordering::SeqCst), 1);

        let error = state.set_cancel_callback(Box::new(|| {})).unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidState);
        assert_eq!(SET_CANCEL_COUNT.load(Ordering::SeqCst), 1);

        state.close();
        let error = state.set_cancel_callback(Box::new(|| {})).unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidArgument);
        assert_eq!(SET_CANCEL_COUNT.load(Ordering::SeqCst), 1);
        assert_eq!(RELEASE_COUNT.load(Ordering::SeqCst), 1);
    }

    #[test]
    // Spec coverage: BND-198. MapLibre's cancel path runs the callback on its
    // own thread, and the callback closes the request it belongs to.
    fn cancel_callback_from_another_thread_may_close_its_request() {
        let _guard = HANDLE_TEST_LOCK.lock().unwrap();
        let state = handled_fake_state();
        let callback_state = Arc::clone(&state);
        let calls = Arc::new(AtomicUsize::new(0));
        let callback_calls = Arc::clone(&calls);
        state
            .set_cancel_callback(Box::new(move || {
                callback_calls.fetch_add(1, Ordering::SeqCst);
                callback_state.close();
            }))
            .unwrap();

        std::thread::spawn(fire_registered_cancel).join().unwrap();

        assert_eq!(calls.load(Ordering::SeqCst), 1);
        assert_eq!(RELEASE_COUNT.load(Ordering::SeqCst), 1);
        state.close();
        drop(state);
        assert_eq!(RELEASE_COUNT.load(Ordering::SeqCst), 1);
    }

    #[test]
    // Spec coverage: BND-153 and BND-198. Native release waits for a cancel
    // callback running on another thread, so close must not hold the handle
    // lock across it when that callback calls back into the same handle.
    fn close_holds_no_lock_while_native_release_waits_for_the_callback() {
        let _guard = HANDLE_TEST_LOCK.lock().unwrap();
        let state = handled_fake_state();
        RELEASE_WAITS_FOR_CALLBACK.store(true, Ordering::SeqCst);
        let callback_started = Arc::new(AtomicBool::new(false));
        let callback_state = Arc::clone(&state);
        let started = Arc::clone(&callback_started);
        let observed_closed = Arc::new(AtomicBool::new(false));
        let callback_observed_closed = Arc::clone(&observed_closed);
        state
            .set_cancel_callback(Box::new(move || {
                started.store(true, Ordering::SeqCst);
                // Let close begin its release before this takes the lock.
                std::thread::sleep(Duration::from_millis(50));
                let closed = callback_state.is_cancelled().is_err();
                callback_observed_closed.store(closed, Ordering::SeqCst);
                CALLBACK_FINISHED.store(true, Ordering::SeqCst);
            }))
            .unwrap();
        let callback_thread = std::thread::spawn(fire_registered_cancel);
        wait_until(&callback_started, "the cancel callback to start");

        state.close();

        assert!(CALLBACK_FINISHED.load(Ordering::SeqCst));
        assert!(observed_closed.load(Ordering::SeqCst));
        assert_eq!(RELEASE_COUNT.load(Ordering::SeqCst), 1);
        callback_thread.join().unwrap();
    }

    #[test]
    // Spec coverage: BND-198. A completed request never runs its callback, and
    // completion drops the callback's captures.
    fn completion_drops_the_cancel_callback() {
        let _guard = HANDLE_TEST_LOCK.lock().unwrap();
        let state = handled_fake_state();
        let token = Arc::new(());
        let callback_token = Arc::clone(&token);
        state
            .set_cancel_callback(Box::new(move || {
                let _ = &callback_token;
                panic!("a completed request must not run its cancel callback");
            }))
            .unwrap();
        assert_eq!(Arc::strong_count(&token), 2);

        state.complete(&ResourceResponse::no_content()).unwrap();

        assert_eq!(Arc::strong_count(&token), 1);
        assert_eq!(RELEASE_COUNT.load(Ordering::SeqCst), 1);
    }

    #[test]
    // Spec coverage: BND-142 and BND-198. The C API releases a passed-through
    // request itself, and that release retires the registration.
    fn cancel_registration_leaves_a_passed_through_release_to_native() {
        let _guard = HANDLE_TEST_LOCK.lock().unwrap();
        let state = fake_state();
        state.set_cancel_callback(Box::new(|| {})).unwrap();

        assert_eq!(
            state.finish_provider_decision(ResourceProviderDecision::PassThrough),
            sys::MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH
        );
        drop(state);

        assert_eq!(RELEASE_COUNT.load(Ordering::SeqCst), 0);
    }

    #[test]
    fn resource_transform_request_copies_url_and_kind() {
        let url = CString::new("https://example.test/style.json").unwrap();
        // SAFETY: url points to live local storage for this call.
        let request =
            unsafe { copy_resource_transform_request(sys::MLN_RESOURCE_KIND_STYLE, url.as_ptr()) }
                .unwrap();

        assert_eq!(request.kind, ResourceKind::Style);
        assert_eq!(request.raw_kind, sys::MLN_RESOURCE_KIND_STYLE);
        assert_eq!(request.url, "https://example.test/style.json");
    }
}
