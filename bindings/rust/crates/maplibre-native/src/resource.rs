use std::cell::Cell;
use std::collections::HashMap;
use std::ffi::CString;
use std::fmt;
use std::marker::PhantomData;
use std::os::raw::{c_char, c_void};
use std::panic::{AssertUnwindSafe, catch_unwind};
use std::ptr;
use std::sync::{Arc, Mutex};
use std::thread::ThreadId;

use maplibre_native_support as support;
use maplibre_native_sys as sys;

use crate::{Error, ErrorKind, HandleOperationError, ResourceErrorReason, Result};

const UNKNOWN_PROVIDER_DECISION: u32 = u32::MAX;

/// Network resource kind passed to resource callbacks.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum ResourceKind {
    Unknown,
    Style,
    Source,
    Tile,
    Glyphs,
    SpriteImage,
    SpriteJson,
    Image,
    UnknownRaw(u32),
}

impl ResourceKind {
    pub(crate) fn from_raw(raw: u32) -> Self {
        match raw {
            sys::MLN_RESOURCE_KIND_UNKNOWN => Self::Unknown,
            sys::MLN_RESOURCE_KIND_STYLE => Self::Style,
            sys::MLN_RESOURCE_KIND_SOURCE => Self::Source,
            sys::MLN_RESOURCE_KIND_TILE => Self::Tile,
            sys::MLN_RESOURCE_KIND_GLYPHS => Self::Glyphs,
            sys::MLN_RESOURCE_KIND_SPRITE_IMAGE => Self::SpriteImage,
            sys::MLN_RESOURCE_KIND_SPRITE_JSON => Self::SpriteJson,
            sys::MLN_RESOURCE_KIND_IMAGE => Self::Image,
            _ => Self::UnknownRaw(raw),
        }
    }
}

/// Resource loading method requested by MapLibre Native.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum ResourceLoadingMethod {
    All,
    CacheOnly,
    NetworkOnly,
    Unknown(u32),
}

impl ResourceLoadingMethod {
    fn from_raw(raw: u32) -> Self {
        match raw {
            sys::MLN_RESOURCE_LOADING_METHOD_ALL => Self::All,
            sys::MLN_RESOURCE_LOADING_METHOD_CACHE_ONLY => Self::CacheOnly,
            sys::MLN_RESOURCE_LOADING_METHOD_NETWORK_ONLY => Self::NetworkOnly,
            _ => Self::Unknown(raw),
        }
    }
}

/// Resource request priority.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum ResourcePriority {
    Regular,
    Low,
    Unknown(u32),
}

impl ResourcePriority {
    fn from_raw(raw: u32) -> Self {
        match raw {
            sys::MLN_RESOURCE_PRIORITY_REGULAR => Self::Regular,
            sys::MLN_RESOURCE_PRIORITY_LOW => Self::Low,
            _ => Self::Unknown(raw),
        }
    }
}

/// Resource request usage.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum ResourceUsage {
    Online,
    Offline,
    Unknown(u32),
}

impl ResourceUsage {
    fn from_raw(raw: u32) -> Self {
        match raw {
            sys::MLN_RESOURCE_USAGE_ONLINE => Self::Online,
            sys::MLN_RESOURCE_USAGE_OFFLINE => Self::Offline,
            _ => Self::Unknown(raw),
        }
    }
}

/// Resource cache storage policy.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum ResourceStoragePolicy {
    Permanent,
    Volatile,
    Unknown(u32),
}

impl ResourceStoragePolicy {
    fn from_raw(raw: u32) -> Self {
        match raw {
            sys::MLN_RESOURCE_STORAGE_POLICY_PERMANENT => Self::Permanent,
            sys::MLN_RESOURCE_STORAGE_POLICY_VOLATILE => Self::Volatile,
            _ => Self::Unknown(raw),
        }
    }
}

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
    pub url: String,
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

impl ResourceRequest {
    unsafe fn from_native(raw: &sys::mln_resource_request) -> Result<Self> {
        let prior_data = if raw.prior_data_size == 0 {
            Vec::new()
        } else if raw.prior_data.is_null() {
            return Err(Error::invalid_argument(
                "resource request prior_data must not be null when prior_data_size is nonzero",
            ));
        } else {
            // SAFETY: The caller promised raw and nested request storage are
            // valid for this callback; copy the borrowed bytes immediately.
            unsafe { std::slice::from_raw_parts(raw.prior_data, raw.prior_data_size) }.to_vec()
        };

        let prior_etag = if raw.prior_etag.is_null() {
            None
        } else {
            // SAFETY: The caller promised raw points to callback-duration storage.
            Some(unsafe { support::string::copy_c_string(raw.prior_etag) }?)
        };

        Ok(Self {
            // SAFETY: The caller promised raw points to callback-duration storage.
            url: unsafe { support::string::copy_c_string(raw.url) }?,
            kind: ResourceKind::from_raw(raw.kind),
            raw_kind: raw.kind,
            loading_method: ResourceLoadingMethod::from_raw(raw.loading_method),
            raw_loading_method: raw.loading_method,
            priority: ResourcePriority::from_raw(raw.priority),
            raw_priority: raw.priority,
            usage: ResourceUsage::from_raw(raw.usage),
            raw_usage: raw.usage,
            storage_policy: ResourceStoragePolicy::from_raw(raw.storage_policy),
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
}

/// Decision returned by a resource provider callback.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum ResourceProviderDecision {
    /// Let native OnlineFileSource handle the request.
    ///
    /// If the callback has already completed the request inline with
    /// [`ResourceRequestHandle::complete`], the wrapper returns native `Handle`
    /// instead. This prevents native code from also handling the same request.
    PassThrough,
    /// Keep ownership of the request handle and complete or release it later.
    Handle,
}

/// Status for a resource provider response.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum ResourceResponseStatus {
    Ok,
    Error,
    NoContent,
    NotModified,
}

impl ResourceResponseStatus {
    fn as_raw(self) -> u32 {
        match self {
            Self::Ok => sys::MLN_RESOURCE_RESPONSE_STATUS_OK,
            Self::Error => sys::MLN_RESOURCE_RESPONSE_STATUS_ERROR,
            Self::NoContent => sys::MLN_RESOURCE_RESPONSE_STATUS_NO_CONTENT,
            Self::NotModified => sys::MLN_RESOURCE_RESPONSE_STATUS_NOT_MODIFIED,
        }
    }
}

impl ResourceErrorReason {
    pub(crate) fn as_raw(self) -> u32 {
        match self {
            Self::None => sys::MLN_RESOURCE_ERROR_REASON_NONE,
            Self::NotFound => sys::MLN_RESOURCE_ERROR_REASON_NOT_FOUND,
            Self::Server => sys::MLN_RESOURCE_ERROR_REASON_SERVER,
            Self::Connection => sys::MLN_RESOURCE_ERROR_REASON_CONNECTION,
            Self::RateLimit => sys::MLN_RESOURCE_ERROR_REASON_RATE_LIMIT,
            Self::Other => sys::MLN_RESOURCE_ERROR_REASON_OTHER,
            Self::Unknown(raw) => raw,
        }
    }
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

    pub fn with_must_revalidate(mut self, must_revalidate: bool) -> Self {
        self.must_revalidate = must_revalidate;
        self
    }

    pub fn with_modified_unix_ms(mut self, modified_unix_ms: i64) -> Self {
        self.modified_unix_ms = Some(modified_unix_ms);
        self
    }

    pub fn with_expires_unix_ms(mut self, expires_unix_ms: i64) -> Self {
        self.expires_unix_ms = Some(expires_unix_ms);
        self
    }

    pub fn with_etag(mut self, etag: impl Into<String>) -> Self {
        self.etag = Some(etag.into());
        self
    }

    pub fn with_retry_after_unix_ms(mut self, retry_after_unix_ms: i64) -> Self {
        self.retry_after_unix_ms = Some(retry_after_unix_ms);
        self
    }

    fn to_native(&self) -> Result<NativeResourceResponse> {
        NativeResourceResponse::new(self)
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

struct NativeResourceResponse {
    raw: sys::mln_resource_response,
    _error_message: Option<CString>,
    _etag: Option<CString>,
}

impl NativeResourceResponse {
    fn new(response: &ResourceResponse) -> Result<Self> {
        let error_message = response
            .error_message
            .as_deref()
            .map(support::string::c_string)
            .transpose()?;
        let etag = response
            .etag
            .as_deref()
            .map(support::string::c_string)
            .transpose()?;
        Ok(Self {
            raw: sys::mln_resource_response {
                size: std::mem::size_of::<sys::mln_resource_response>() as u32,
                status: response.status.as_raw(),
                error_reason: response.error_reason.as_raw(),
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
            _error_message: error_message,
            _etag: etag,
        })
    }

    fn as_ptr(&self) -> *const sys::mln_resource_response {
        &self.raw
    }
}

type CompleteRequestFn = unsafe extern "C" fn(
    *mut sys::mln_resource_request_handle,
    *const sys::mln_resource_response,
) -> sys::mln_status;
type CancelledRequestFn =
    unsafe extern "C" fn(*const sys::mln_resource_request_handle, *mut bool) -> sys::mln_status;
type ReleaseRequestFn = unsafe extern "C" fn(*mut sys::mln_resource_request_handle);

#[derive(Clone, Copy, Debug)]
struct ResourceRequestHandleFns {
    complete: CompleteRequestFn,
    cancelled: CancelledRequestFn,
    release: ReleaseRequestFn,
}

impl ResourceRequestHandleFns {
    const NATIVE: Self = Self {
        complete: sys::mln_resource_request_complete,
        cancelled: sys::mln_resource_request_cancelled,
        release: sys::mln_resource_request_release,
    };
}

#[derive(Debug)]
struct ResourceRequestHandleInner {
    handle: usize,
    decision_finalized: bool,
    provider_owned: bool,
    release_accounted_for: bool,
    closed: bool,
    completed: bool,
}

#[derive(Debug)]
struct ResourceRequestHandleState {
    inner: Mutex<ResourceRequestHandleInner>,
    fns: ResourceRequestHandleFns,
}

impl ResourceRequestHandleState {
    fn new(
        handle: *mut sys::mln_resource_request_handle,
        fns: ResourceRequestHandleFns,
    ) -> Result<Arc<Self>> {
        if handle.is_null() {
            return Err(Error::invalid_argument(
                "resource request handle must not be null",
            ));
        }
        Ok(Arc::new(Self {
            inner: Mutex::new(ResourceRequestHandleInner {
                handle: handle as usize,
                decision_finalized: false,
                provider_owned: false,
                release_accounted_for: false,
                closed: false,
                completed: false,
            }),
            fns,
        }))
    }

    fn handle_ptr(inner: &ResourceRequestHandleInner) -> *mut sys::mln_resource_request_handle {
        inner.handle as *mut sys::mln_resource_request_handle
    }

    fn complete(&self, response: &ResourceResponse) -> Result<()> {
        let native = response.to_native()?;
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

        // SAFETY: handle is live while not closed/released, and native response
        // points to storage retained for this call. The C API copies contents.
        support::check(unsafe { (self.fns.complete)(Self::handle_ptr(&inner), native.as_ptr()) })?;
        inner.completed = true;
        inner.closed = true;
        if inner.decision_finalized && inner.provider_owned {
            self.release_if_owned_locked(&mut inner);
        }
        Ok(())
    }

    fn is_cancelled(&self) -> Result<bool> {
        let inner = self.lock_inner()?;
        if inner.closed {
            return Err(Error::invalid_argument("ResourceRequestHandle is closed"));
        }
        let mut cancelled = false;
        // SAFETY: handle is live while not closed/released, and cancelled points
        // to writable bool storage.
        support::check(unsafe { (self.fns.cancelled)(Self::handle_ptr(&inner), &mut cancelled) })?;
        Ok(cancelled)
    }

    fn close(&self) {
        let Ok(mut inner) = self.inner.lock() else {
            return;
        };
        if inner.closed {
            return;
        }
        inner.closed = true;
        if inner.decision_finalized && inner.provider_owned {
            self.release_if_owned_locked(&mut inner);
        }
    }

    fn finish_provider_decision(&self, decision: ResourceProviderDecision) -> u32 {
        let Ok(mut inner) = self.inner.lock() else {
            return UNKNOWN_PROVIDER_DECISION;
        };
        if inner.completed || matches!(decision, ResourceProviderDecision::Handle) {
            inner.decision_finalized = true;
            inner.provider_owned = true;
            if inner.closed {
                self.release_if_owned_locked(&mut inner);
            }
            sys::MLN_RESOURCE_PROVIDER_DECISION_HANDLE
        } else {
            inner.decision_finalized = true;
            inner.release_accounted_for = true;
            inner.closed = true;
            sys::MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH
        }
    }

    fn finish_provider_exception(&self) -> u32 {
        let completed = self
            .inner
            .lock()
            .map(|inner| inner.completed)
            .unwrap_or(false);
        if completed {
            self.finish_provider_decision(ResourceProviderDecision::Handle)
        } else {
            if let Ok(mut inner) = self.inner.lock() {
                inner.decision_finalized = true;
                inner.release_accounted_for = true;
                inner.closed = true;
            }
            UNKNOWN_PROVIDER_DECISION
        }
    }

    fn release_if_owned_locked(&self, inner: &mut ResourceRequestHandleInner) {
        if !inner.release_accounted_for {
            inner.release_accounted_for = true;
            // SAFETY: release is called exactly once for provider-owned handles.
            unsafe { (self.fns.release)(Self::handle_ptr(inner)) };
        }
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
        let Ok(mut inner) = self.inner.lock() else {
            return;
        };
        if inner.provider_owned {
            self.release_if_owned_locked(&mut inner);
        }
    }
}

/// Owned handle for a resource provider request selected for handling.
///
/// The handle may be sent to another thread for deferred completion. It is
/// one-shot: call `complete` once to provide a response, or `close`/drop it to
/// release the provider's reference without completing.
pub struct ResourceRequestHandle {
    state: Arc<ResourceRequestHandleState>,
    _not_sync: PhantomData<Cell<()>>,
}

impl fmt::Debug for ResourceRequestHandle {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("ResourceRequestHandle")
            .finish_non_exhaustive()
    }
}

impl ResourceRequestHandle {
    fn from_state(state: Arc<ResourceRequestHandleState>) -> Self {
        Self {
            state,
            _not_sync: PhantomData,
        }
    }

    fn from_raw_with_fns(
        handle: *mut sys::mln_resource_request_handle,
        fns: ResourceRequestHandleFns,
    ) -> Result<Self> {
        ResourceRequestHandleState::new(handle, fns).map(Self::from_state)
    }

    /// Completes the request. Successful completion releases this handle once
    /// the provider callback has returned `Handle` to native code.
    ///
    /// When a provider callback completes inline and then returns
    /// [`ResourceProviderDecision::PassThrough`], the wrapper still returns the
    /// native `Handle` decision. Native code must not also pass the completed
    /// request through to its own networking path.
    pub fn complete(
        self,
        response: ResourceResponse,
    ) -> std::result::Result<(), HandleOperationError<Self>> {
        self.state
            .complete(&response)
            .map_err(|error| HandleOperationError::new(error, self))
    }

    /// Reports whether native code has cancelled the request.
    pub fn is_cancelled(&self) -> Result<bool> {
        self.state.is_cancelled()
    }

    /// Releases the provider-owned request handle without completing it.
    pub fn close(self) {
        self.state.close();
    }
}

impl Drop for ResourceRequestHandle {
    fn drop(&mut self) {
        self.state.close();
    }
}

type ResourceProviderCallback = dyn Fn(ResourceRequest, ResourceRequestHandle) -> ResourceProviderDecision
    + Send
    + Sync
    + 'static;

pub(crate) struct ResourceProviderState {
    callback: Box<ResourceProviderCallback>,
    handle_fns: ResourceRequestHandleFns,
}

impl fmt::Debug for ResourceProviderState {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("ResourceProviderState")
            .finish_non_exhaustive()
    }
}

impl ResourceProviderState {
    pub(crate) fn new<F>(callback: F) -> Box<Self>
    where
        F: Fn(ResourceRequest, ResourceRequestHandle) -> ResourceProviderDecision
            + Send
            + Sync
            + 'static,
    {
        Box::new(Self {
            callback: Box::new(callback),
            handle_fns: ResourceRequestHandleFns::NATIVE,
        })
    }

    pub(crate) fn descriptor(&self) -> sys::mln_resource_provider {
        sys::mln_resource_provider {
            size: std::mem::size_of::<sys::mln_resource_provider>() as u32,
            callback: Some(resource_provider_trampoline),
            user_data: ptr::from_ref(self).cast_mut().cast::<c_void>(),
        }
    }

    fn invoke(
        &self,
        request: *const sys::mln_resource_request,
        handle: *mut sys::mln_resource_request_handle,
    ) -> u32 {
        let Some(raw_request) = ptr::NonNull::new(request.cast_mut()) else {
            return UNKNOWN_PROVIDER_DECISION;
        };
        let handle = match ResourceRequestHandle::from_raw_with_fns(handle, self.handle_fns) {
            Ok(handle) => handle,
            Err(_) => return UNKNOWN_PROVIDER_DECISION,
        };
        let state = Arc::clone(&handle.state);

        // SAFETY: raw_request is non-null and borrowed for the callback duration.
        let request = match unsafe { ResourceRequest::from_native(raw_request.as_ref()) } {
            Ok(request) => request,
            Err(_) => return state.finish_provider_exception(),
        };

        match catch_unwind(AssertUnwindSafe(|| (self.callback)(request, handle))) {
            Ok(decision) => state.finish_provider_decision(decision),
            Err(_) => state.finish_provider_exception(),
        }
    }
}

unsafe extern "C" fn resource_provider_trampoline(
    user_data: *mut c_void,
    request: *const sys::mln_resource_request,
    handle: *mut sys::mln_resource_request_handle,
) -> u32 {
    let Some(state) = ptr::NonNull::new(user_data.cast::<ResourceProviderState>()) else {
        return UNKNOWN_PROVIDER_DECISION;
    };
    // SAFETY: user_data is installed from ResourceProviderState::descriptor and
    // remains valid until replacement or runtime teardown. The callback state is
    // Send + Sync because native may invoke it from worker/network threads.
    unsafe { state.as_ref() }.invoke(request, handle)
}

/// Copied request passed to a runtime-scoped resource transform callback.
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub struct ResourceTransformRequest {
    pub kind: ResourceKind,
    pub raw_kind: u32,
    pub url: String,
}

type ResourceTransformCallback =
    dyn Fn(ResourceTransformRequest) -> Option<String> + Send + Sync + 'static;

pub(crate) struct ResourceTransformState {
    callback: Box<ResourceTransformCallback>,
    replacement_urls: Mutex<HashMap<ThreadId, CString>>,
}

impl fmt::Debug for ResourceTransformState {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("ResourceTransformState")
            .finish_non_exhaustive()
    }
}

impl ResourceTransformState {
    pub(crate) fn new<F>(callback: F) -> Box<Self>
    where
        F: Fn(ResourceTransformRequest) -> Option<String> + Send + Sync + 'static,
    {
        Box::new(Self {
            callback: Box::new(callback),
            replacement_urls: Mutex::new(HashMap::new()),
        })
    }

    pub(crate) fn descriptor(&self) -> sys::mln_resource_transform {
        sys::mln_resource_transform {
            size: std::mem::size_of::<sys::mln_resource_transform>() as u32,
            callback: Some(resource_transform_trampoline),
            user_data: ptr::from_ref(self).cast_mut().cast::<c_void>(),
        }
    }

    fn invoke(
        &self,
        raw_kind: u32,
        url: *const c_char,
        out_response: *mut sys::mln_resource_transform_response,
    ) -> sys::mln_status {
        if out_response.is_null() {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }

        // SAFETY: out_response was checked non-null and is borrowed for the
        // callback duration by the C API.
        unsafe {
            (*out_response).size =
                std::mem::size_of::<sys::mln_resource_transform_response>() as u32;
            (*out_response).url = ptr::null();
        }

        let request_url = match unsafe { support::string::copy_c_string(url) } {
            Ok(url) => url,
            Err(error) => return status_for_error(&error),
        };
        let request = ResourceTransformRequest {
            kind: ResourceKind::from_raw(raw_kind),
            raw_kind,
            url: request_url,
        };

        let replacement = match catch_unwind(AssertUnwindSafe(|| (self.callback)(request))) {
            Ok(replacement) => replacement,
            Err(_) => return sys::MLN_STATUS_NATIVE_ERROR,
        };

        match replacement {
            Some(replacement) if !replacement.is_empty() => {
                let replacement = match CString::new(replacement) {
                    Ok(replacement) => replacement,
                    Err(_) => return sys::MLN_STATUS_INVALID_ARGUMENT,
                };
                let replacement_ptr = replacement.as_ptr();
                let mut replacements = match self.replacement_urls.lock() {
                    Ok(replacements) => replacements,
                    Err(_) => return sys::MLN_STATUS_NATIVE_ERROR,
                };
                replacements.insert(std::thread::current().id(), replacement);
                // SAFETY: out_response was checked non-null above. The stored
                // CString is retained in replacement_urls until the next
                // callback on this thread or until state teardown, so it
                // remains live after this trampoline returns while C copies it.
                unsafe {
                    (*out_response).url = replacement_ptr;
                }
                sys::MLN_STATUS_OK
            }
            _ => {
                if let Ok(mut replacements) = self.replacement_urls.lock() {
                    replacements.remove(&std::thread::current().id());
                    sys::MLN_STATUS_OK
                } else {
                    sys::MLN_STATUS_NATIVE_ERROR
                }
            }
        }
    }
}

pub(crate) fn noop_resource_transform_descriptor() -> sys::mln_resource_transform {
    sys::mln_resource_transform {
        size: std::mem::size_of::<sys::mln_resource_transform>() as u32,
        callback: Some(noop_resource_transform),
        user_data: ptr::null_mut(),
    }
}

unsafe extern "C" fn noop_resource_transform(
    _user_data: *mut c_void,
    _kind: u32,
    _url: *const c_char,
    out_response: *mut sys::mln_resource_transform_response,
) -> sys::mln_status {
    if out_response.is_null() {
        return sys::MLN_STATUS_INVALID_ARGUMENT;
    }
    // SAFETY: out_response was checked non-null and is borrowed for this callback.
    unsafe {
        (*out_response).size = std::mem::size_of::<sys::mln_resource_transform_response>() as u32;
        (*out_response).url = ptr::null();
    }
    sys::MLN_STATUS_OK
}

unsafe extern "C" fn resource_transform_trampoline(
    user_data: *mut c_void,
    kind: u32,
    url: *const c_char,
    out_response: *mut sys::mln_resource_transform_response,
) -> sys::mln_status {
    let Some(state) = ptr::NonNull::new(user_data.cast::<ResourceTransformState>()) else {
        return sys::MLN_STATUS_INVALID_ARGUMENT;
    };
    // SAFETY: user_data is installed from ResourceTransformState::descriptor
    // and remains valid until the runtime replaces/clears the transform or is
    // destroyed. The callback state itself is Send + Sync.
    unsafe { state.as_ref() }.invoke(kind, url, out_response)
}

fn status_for_error(error: &Error) -> sys::mln_status {
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
        _ => sys::MLN_STATUS_NATIVE_ERROR,
    }
}

#[cfg(test)]
mod tests {
    use std::ffi::CStr;
    use std::sync::Mutex as StdMutex;
    use std::sync::atomic::{AtomicBool, AtomicI32, AtomicUsize, Ordering};

    use static_assertions::{assert_impl_all, assert_not_impl_any};

    use super::*;

    static FAKE_HANDLE_TEST_LOCK: StdMutex<()> = StdMutex::new(());
    static COMPLETE_COUNT: AtomicUsize = AtomicUsize::new(0);
    static RELEASE_COUNT: AtomicUsize = AtomicUsize::new(0);
    static CANCELLED_VALUE: AtomicBool = AtomicBool::new(false);
    static COMPLETE_STATUS: AtomicI32 = AtomicI32::new(sys::MLN_STATUS_OK);

    unsafe extern "C" fn fake_complete(
        _handle: *mut sys::mln_resource_request_handle,
        _response: *const sys::mln_resource_response,
    ) -> sys::mln_status {
        COMPLETE_COUNT.fetch_add(1, Ordering::SeqCst);
        COMPLETE_STATUS.load(Ordering::SeqCst)
    }

    unsafe extern "C" fn fake_cancelled(
        _handle: *const sys::mln_resource_request_handle,
        out_cancelled: *mut bool,
    ) -> sys::mln_status {
        if out_cancelled.is_null() {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        // SAFETY: out_cancelled is non-null and points to caller-owned output storage.
        unsafe { *out_cancelled = CANCELLED_VALUE.load(Ordering::SeqCst) };
        sys::MLN_STATUS_OK
    }

    unsafe extern "C" fn fake_release(_handle: *mut sys::mln_resource_request_handle) {
        RELEASE_COUNT.fetch_add(1, Ordering::SeqCst);
    }

    fn fake_fns() -> ResourceRequestHandleFns {
        ResourceRequestHandleFns {
            complete: fake_complete,
            cancelled: fake_cancelled,
            release: fake_release,
        }
    }

    fn reset_fake_handle_state() {
        COMPLETE_COUNT.store(0, Ordering::SeqCst);
        RELEASE_COUNT.store(0, Ordering::SeqCst);
        CANCELLED_VALUE.store(false, Ordering::SeqCst);
        COMPLETE_STATUS.store(sys::MLN_STATUS_OK, Ordering::SeqCst);
    }

    fn fake_handle() -> ResourceRequestHandle {
        reset_fake_handle_state();
        ResourceRequestHandle::from_raw_with_fns(
            0x1234usize as *mut sys::mln_resource_request_handle,
            fake_fns(),
        )
        .unwrap()
    }

    fn request() -> sys::mln_resource_request {
        sys::mln_resource_request {
            size: std::mem::size_of::<sys::mln_resource_request>() as u32,
            url: c"https://example.test/tile.pbf".as_ptr(),
            kind: sys::MLN_RESOURCE_KIND_TILE,
            loading_method: sys::MLN_RESOURCE_LOADING_METHOD_NETWORK_ONLY,
            priority: sys::MLN_RESOURCE_PRIORITY_LOW,
            usage: sys::MLN_RESOURCE_USAGE_OFFLINE,
            storage_policy: sys::MLN_RESOURCE_STORAGE_POLICY_VOLATILE,
            has_range: true,
            range_start: 7,
            range_end: 11,
            has_prior_modified: true,
            prior_modified_unix_ms: 123,
            has_prior_expires: true,
            prior_expires_unix_ms: 456,
            prior_etag: c"etag".as_ptr(),
            prior_data: [1u8, 2, 3].as_ptr(),
            prior_data_size: 3,
        }
    }

    fn response() -> sys::mln_resource_transform_response {
        sys::mln_resource_transform_response {
            size: std::mem::size_of::<sys::mln_resource_transform_response>() as u32,
            url: ptr::null(),
        }
    }

    #[test]
    fn resource_request_handle_is_send_but_not_sync() {
        assert_impl_all!(ResourceRequestHandle: Send);
        assert_not_impl_any!(ResourceRequestHandle: Sync);
    }

    #[test]
    fn provider_callback_copies_request_and_pass_through_does_not_release() {
        let _guard = FAKE_HANDLE_TEST_LOCK.lock().unwrap();
        reset_fake_handle_state();
        let state = ResourceProviderState {
            callback: Box::new(|request, handle| {
                assert_eq!(request.url, "https://example.test/tile.pbf");
                assert_eq!(request.kind, ResourceKind::Tile);
                assert_eq!(request.raw_kind, sys::MLN_RESOURCE_KIND_TILE);
                assert_eq!(request.loading_method, ResourceLoadingMethod::NetworkOnly);
                assert_eq!(request.priority, ResourcePriority::Low);
                assert_eq!(request.usage, ResourceUsage::Offline);
                assert_eq!(request.storage_policy, ResourceStoragePolicy::Volatile);
                assert_eq!(request.range, Some(ByteRange { start: 7, end: 11 }));
                assert_eq!(request.prior_modified_unix_ms, Some(123));
                assert_eq!(request.prior_expires_unix_ms, Some(456));
                assert_eq!(request.prior_etag.as_deref(), Some("etag"));
                assert_eq!(request.prior_data, vec![1, 2, 3]);
                drop(handle);
                ResourceProviderDecision::PassThrough
            }),
            handle_fns: fake_fns(),
        };
        let raw_request = request();

        let decision = state.invoke(
            &raw_request,
            0x1234usize as *mut sys::mln_resource_request_handle,
        );

        assert_eq!(decision, sys::MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH);
        assert_eq!(RELEASE_COUNT.load(Ordering::SeqCst), 0);
    }

    #[test]
    fn inline_completion_returns_handle_and_releases_once() {
        let _guard = FAKE_HANDLE_TEST_LOCK.lock().unwrap();
        reset_fake_handle_state();
        let state = ResourceProviderState {
            callback: Box::new(|_, handle| {
                handle.complete(ResourceResponse::ok([1, 2, 3])).unwrap();
                ResourceProviderDecision::PassThrough
            }),
            handle_fns: fake_fns(),
        };
        let raw_request = request();

        let decision = state.invoke(
            &raw_request,
            0x1234usize as *mut sys::mln_resource_request_handle,
        );

        assert_eq!(decision, sys::MLN_RESOURCE_PROVIDER_DECISION_HANDLE);
        assert_eq!(COMPLETE_COUNT.load(Ordering::SeqCst), 1);
        assert_eq!(RELEASE_COUNT.load(Ordering::SeqCst), 1);
    }

    #[test]
    fn deferred_completion_from_another_thread_releases_once() {
        let _guard = FAKE_HANDLE_TEST_LOCK.lock().unwrap();
        let handle = fake_handle();
        assert_eq!(
            handle
                .state
                .finish_provider_decision(ResourceProviderDecision::Handle),
            sys::MLN_RESOURCE_PROVIDER_DECISION_HANDLE
        );

        let thread = std::thread::spawn(move || {
            handle
                .complete(ResourceResponse::ok(vec![4, 5, 6]))
                .unwrap();
        });
        thread.join().unwrap();

        assert_eq!(COMPLETE_COUNT.load(Ordering::SeqCst), 1);
        assert_eq!(RELEASE_COUNT.load(Ordering::SeqCst), 1);
    }

    #[test]
    fn failed_completion_returns_handle_for_retry() {
        let _guard = FAKE_HANDLE_TEST_LOCK.lock().unwrap();
        let handle = fake_handle();
        COMPLETE_STATUS.store(sys::MLN_STATUS_INVALID_STATE, Ordering::SeqCst);

        let error = handle
            .complete(ResourceResponse::ok(Vec::new()))
            .unwrap_err();
        assert_eq!(error.kind(), ErrorKind::InvalidState);
        assert_eq!(COMPLETE_COUNT.load(Ordering::SeqCst), 1);

        COMPLETE_STATUS.store(sys::MLN_STATUS_OK, Ordering::SeqCst);
        let handle = error.into_handle();
        handle.complete(ResourceResponse::no_content()).unwrap();
        assert_eq!(COMPLETE_COUNT.load(Ordering::SeqCst), 2);
    }

    #[test]
    fn drop_releases_uncompleted_handled_request_once() {
        let _guard = FAKE_HANDLE_TEST_LOCK.lock().unwrap();
        let handle = fake_handle();
        assert_eq!(
            handle
                .state
                .finish_provider_decision(ResourceProviderDecision::Handle),
            sys::MLN_RESOURCE_PROVIDER_DECISION_HANDLE
        );
        drop(handle);

        assert_eq!(RELEASE_COUNT.load(Ordering::SeqCst), 1);
    }

    #[test]
    fn close_before_handle_decision_releases_after_decision() {
        let _guard = FAKE_HANDLE_TEST_LOCK.lock().unwrap();
        let handle = fake_handle();
        let state = Arc::clone(&handle.state);
        handle.close();
        assert_eq!(RELEASE_COUNT.load(Ordering::SeqCst), 0);
        assert_eq!(
            state.finish_provider_decision(ResourceProviderDecision::Handle),
            sys::MLN_RESOURCE_PROVIDER_DECISION_HANDLE
        );

        assert_eq!(RELEASE_COUNT.load(Ordering::SeqCst), 1);
    }

    #[test]
    fn cancelled_queries_use_native_function_until_closed() {
        let _guard = FAKE_HANDLE_TEST_LOCK.lock().unwrap();
        let handle = fake_handle();
        CANCELLED_VALUE.store(true, Ordering::SeqCst);

        assert!(handle.is_cancelled().unwrap());
        handle.close();
    }

    #[test]
    fn provider_panics_produce_unknown_decision_without_unwinding() {
        let _guard = FAKE_HANDLE_TEST_LOCK.lock().unwrap();
        reset_fake_handle_state();
        let state = ResourceProviderState {
            callback: Box::new(|_, _| panic!("boom")),
            handle_fns: fake_fns(),
        };
        let raw_request = request();

        let decision = state.invoke(
            &raw_request,
            0x1234usize as *mut sys::mln_resource_request_handle,
        );

        assert_eq!(decision, UNKNOWN_PROVIDER_DECISION);
        assert_eq!(RELEASE_COUNT.load(Ordering::SeqCst), 0);
    }

    #[test]
    fn resource_response_materializes_error_and_cache_fields() {
        let response = ResourceResponse::error(ResourceErrorReason::RateLimit, "slow down")
            .with_retry_after_unix_ms(1_700_000_001)
            .with_must_revalidate(true)
            .with_modified_unix_ms(1_700_000_002)
            .with_expires_unix_ms(1_700_000_003)
            .with_etag("abc123");

        let native = response.to_native().unwrap();
        let raw = &native.raw;

        assert_eq!(raw.status, sys::MLN_RESOURCE_RESPONSE_STATUS_ERROR);
        assert_eq!(raw.error_reason, sys::MLN_RESOURCE_ERROR_REASON_RATE_LIMIT);
        assert!(raw.bytes.is_null());
        assert_eq!(raw.byte_count, 0);
        assert!(raw.must_revalidate);
        assert!(raw.has_modified);
        assert_eq!(raw.modified_unix_ms, 1_700_000_002);
        assert!(raw.has_expires);
        assert_eq!(raw.expires_unix_ms, 1_700_000_003);
        assert!(raw.has_retry_after);
        assert_eq!(raw.retry_after_unix_ms, 1_700_000_001);
        assert_eq!(
            unsafe { CStr::from_ptr(raw.error_message) }
                .to_str()
                .unwrap(),
            "slow down"
        );
        assert_eq!(
            unsafe { CStr::from_ptr(raw.etag) }.to_str().unwrap(),
            "abc123"
        );
    }

    #[test]
    fn transform_callback_copies_request_and_keeps_replacement_url_alive() {
        let state = ResourceTransformState::new(|request| {
            assert_eq!(request.kind, ResourceKind::Style);
            assert_eq!(request.raw_kind, sys::MLN_RESOURCE_KIND_STYLE);
            assert_eq!(request.url, "https://example.test/style.json");
            Some(format!("{}?token=1", request.url))
        });
        let descriptor = state.descriptor();
        let callback = descriptor.callback.unwrap();
        let url = CString::new("https://example.test/style.json").unwrap();
        let mut response = response();

        let status = unsafe {
            callback(
                descriptor.user_data,
                sys::MLN_RESOURCE_KIND_STYLE,
                url.as_ptr(),
                &mut response,
            )
        };

        assert_eq!(status, sys::MLN_STATUS_OK);
        assert!(!response.url.is_null());
        let replacement = unsafe { CStr::from_ptr(response.url) }.to_str().unwrap();
        assert_eq!(replacement, "https://example.test/style.json?token=1");
    }

    #[test]
    fn transform_callback_clears_stale_response_when_keeping_original_url() {
        let state = ResourceTransformState::new(|_| None);
        let descriptor = state.descriptor();
        let callback = descriptor.callback.unwrap();
        let url = CString::new("https://example.test/style.json").unwrap();
        let stale = CString::new("https://stale.test/style.json").unwrap();
        let mut response = response();
        response.url = stale.as_ptr();

        let status = unsafe {
            callback(
                descriptor.user_data,
                sys::MLN_RESOURCE_KIND_STYLE,
                url.as_ptr(),
                &mut response,
            )
        };

        assert_eq!(status, sys::MLN_STATUS_OK);
        assert!(response.url.is_null());
    }

    #[test]
    fn transform_callback_contains_panics() {
        let state = ResourceTransformState::new(|_| panic!("boom"));
        let descriptor = state.descriptor();
        let callback = descriptor.callback.unwrap();
        let url = CString::new("https://example.test/style.json").unwrap();
        let mut response = response();

        let status = unsafe {
            callback(
                descriptor.user_data,
                sys::MLN_RESOURCE_KIND_STYLE,
                url.as_ptr(),
                &mut response,
            )
        };

        assert_eq!(status, sys::MLN_STATUS_NATIVE_ERROR);
        assert!(response.url.is_null());
    }

    #[test]
    fn transform_callback_rejects_embedded_nul_replacements() {
        let state = ResourceTransformState::new(|_| Some("https://example.test/\0bad".to_owned()));
        let descriptor = state.descriptor();
        let callback = descriptor.callback.unwrap();
        let url = CString::new("https://example.test/style.json").unwrap();
        let mut response = response();

        let status = unsafe {
            callback(
                descriptor.user_data,
                sys::MLN_RESOURCE_KIND_STYLE,
                url.as_ptr(),
                &mut response,
            )
        };

        assert_eq!(status, sys::MLN_STATUS_INVALID_ARGUMENT);
        assert!(response.url.is_null());
    }

    #[test]
    fn transform_state_drops_callback_capture() {
        let token = Arc::new(());
        let callback_token = Arc::clone(&token);
        let state = ResourceTransformState::new(move |_| {
            let _ = &callback_token;
            None
        });
        assert_eq!(Arc::strong_count(&token), 2);
        drop(state);
        assert_eq!(Arc::strong_count(&token), 1);
    }

    #[test]
    fn callback_can_run_from_multiple_threads() {
        let calls = Arc::new(AtomicUsize::new(0));
        let callback_calls = Arc::clone(&calls);
        let state = Arc::new(ResourceTransformState {
            callback: Box::new(move |request| {
                callback_calls.fetch_add(1, Ordering::SeqCst);
                Some(format!("{}?thread=1", request.url))
            }),
            replacement_urls: Mutex::new(HashMap::new()),
        });

        let handles = (0..2)
            .map(|_| {
                let state = Arc::clone(&state);
                std::thread::spawn(move || {
                    let descriptor = state.descriptor();
                    let callback = descriptor.callback.unwrap();
                    let url = CString::new("https://example.test/tile").unwrap();
                    let mut response = response();
                    let status = unsafe {
                        callback(
                            descriptor.user_data,
                            sys::MLN_RESOURCE_KIND_TILE,
                            url.as_ptr(),
                            &mut response,
                        )
                    };
                    assert_eq!(status, sys::MLN_STATUS_OK);
                    assert!(!response.url.is_null());
                    let replacement = unsafe { CStr::from_ptr(response.url) }.to_str().unwrap();
                    assert_eq!(replacement, "https://example.test/tile?thread=1");
                })
            })
            .collect::<Vec<_>>();

        for handle in handles {
            handle.join().unwrap();
        }
        assert_eq!(calls.load(Ordering::SeqCst), 2);
    }
}
