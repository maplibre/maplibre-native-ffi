use std::ffi::{CString, c_char, c_void};
use std::marker::PhantomData;
use std::ptr;
use std::sync::{Arc, Mutex};

use maplibre_native_sys as sys;

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
        url: unsafe { crate::string::copy_c_string(raw.url) }?,
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
    *mut sys::mln_resource_request_handle,
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
    *mut sys::mln_resource_request_handle,
    *const sys::mln_resource_response,
) -> sys::mln_status;
pub type CancelledRequestFn =
    unsafe extern "C" fn(*const sys::mln_resource_request_handle, *mut bool) -> sys::mln_status;
pub type ReleaseRequestFn = unsafe extern "C" fn(*mut sys::mln_resource_request_handle);

#[derive(Clone, Copy, Debug)]
pub struct ResourceRequestHandleFns {
    complete: CompleteRequestFn,
    cancelled: CancelledRequestFn,
    release: ReleaseRequestFn,
}

impl ResourceRequestHandleFns {
    pub const NATIVE: Self = Self {
        complete: sys::mln_resource_request_complete,
        cancelled: sys::mln_resource_request_cancelled,
        release: sys::mln_resource_request_release,
    };

    /// Creates a function table for a native resource request handle.
    ///
    /// # Safety
    ///
    /// The functions must implement the same ownership contract as the C API:
    /// `complete` and `cancelled` operate on the matching handle type, and
    /// `release` releases a provider-owned handle exactly once.
    pub const unsafe fn new(
        complete: CompleteRequestFn,
        cancelled: CancelledRequestFn,
        release: ReleaseRequestFn,
    ) -> Self {
        Self {
            complete,
            cancelled,
            release,
        }
    }
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
    /// provider callback, and `fns` must match that handle. This state machine
    /// coordinates completion, cancellation checks, provider decision
    /// finalization, and provider-owned release.
    pub unsafe fn new(
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
        // SAFETY: handle is live while not closed/released, and native response
        // points to storage retained for this call. The C API copies contents.
        let status = unsafe { (self.fns.complete)(Self::handle_ptr(&inner), native.as_ptr()) };
        let result = crate::check(status);
        if inner.decision_finalized && inner.provider_owned {
            self.release_if_owned_locked(&mut inner);
        }
        result
    }

    pub fn is_cancelled(&self) -> Result<bool> {
        let inner = self.lock_inner()?;
        if inner.closed {
            return Err(Error::invalid_argument("ResourceRequestHandle is closed"));
        }
        let mut cancelled = false;
        // SAFETY: handle is live while not closed/released, and cancelled points
        // to writable bool storage.
        crate::check(unsafe { (self.fns.cancelled)(Self::handle_ptr(&inner), &mut cancelled) })?;
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
        if inner.decision_finalized && inner.provider_owned {
            self.release_if_owned_locked(&mut inner);
        }
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

    pub fn finish_provider_exception(&self) -> u32 {
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

/// Copied request passed to a runtime-scoped resource transform callback.
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub struct ResourceTransformRequest {
    pub kind: ResourceKind,
    pub raw_kind: u32,
    pub url: String,
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
        let sleep_ms = CANCELLED_SLEEP_MS.load(Ordering::SeqCst);
        if sleep_ms != 0 {
            CANCELLED_STARTED.store(true, Ordering::SeqCst);
            std::thread::sleep(Duration::from_millis(sleep_ms as u64));
            CANCELLED_FINISHED.store(true, Ordering::SeqCst);
        }
        // SAFETY: out_cancelled is non-null and points to caller-owned output storage.
        unsafe { *out_cancelled = false };
        sys::MLN_STATUS_OK
    }

    unsafe extern "C" fn fake_release(_handle: *mut sys::mln_resource_request_handle) {
        RELEASE_COUNT.fetch_add(1, Ordering::SeqCst);
    }

    fn fake_fns() -> ResourceRequestHandleFns {
        // SAFETY: These fake functions implement the native handle contract for tests.
        unsafe { ResourceRequestHandleFns::new(fake_complete, fake_cancelled, fake_release) }
    }

    fn fake_state() -> Arc<ResourceRequestHandleState> {
        COMPLETE_COUNT.store(0, Ordering::SeqCst);
        RELEASE_COUNT.store(0, Ordering::SeqCst);
        COMPLETE_STATUS.store(sys::MLN_STATUS_OK, Ordering::SeqCst);
        CANCELLED_SLEEP_MS.store(0, Ordering::SeqCst);
        CANCELLED_STARTED.store(false, Ordering::SeqCst);
        CANCELLED_FINISHED.store(false, Ordering::SeqCst);
        // SAFETY: Non-null sentinel handle is used only by fake functions.
        unsafe {
            ResourceRequestHandleState::new(
                0x1234usize as *mut sys::mln_resource_request_handle,
                fake_fns(),
            )
        }
        .unwrap()
    }

    #[test]
    fn resource_request_copies_nested_storage() {
        let mut url = CString::new("https://example.test/tile").unwrap();
        let mut etag = CString::new("abc").unwrap();
        let mut prior_data = [1_u8, 2, 3];
        let raw = sys::mln_resource_request {
            size: std::mem::size_of::<sys::mln_resource_request>() as u32,
            url: url.as_ptr(),
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
        url = CString::new("https://changed.test").unwrap();
        etag = CString::new("changed").unwrap();
        prior_data.fill(9);

        assert_eq!(url.as_bytes(), b"https://changed.test");
        assert_eq!(etag.as_bytes(), b"changed");
        assert_eq!(copied.url, "https://example.test/tile");
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
            url: url.as_ptr(),
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
