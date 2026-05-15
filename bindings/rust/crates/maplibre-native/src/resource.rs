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

use maplibre_native_core as maplibre_core;
use maplibre_native_sys as sys;

use crate::{HandleOperationError, Result};

pub use maplibre_core::resource::{
    ByteRange, ResourceProviderDecision, ResourceRequest, ResourceResponse,
    ResourceTransformRequest,
};

use maplibre_core::resource::{
    ResourceRequestHandleFns, ResourceRequestHandleState, UNKNOWN_PROVIDER_DECISION,
};

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
        // SAFETY: handle is received from the resource-provider C callback and
        // fns matches that native handle type.
        unsafe { ResourceRequestHandleState::new(handle, fns) }.map(Self::from_state)
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
        maplibre_core::resource::resource_provider_descriptor(
            Some(resource_provider_trampoline),
            ptr::from_ref(self).cast_mut().cast::<c_void>(),
        )
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
        let request =
            match unsafe { maplibre_core::resource::copy_resource_request(raw_request.as_ref()) } {
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
        maplibre_core::resource::resource_transform_descriptor(
            Some(resource_transform_trampoline),
            ptr::from_ref(self).cast_mut().cast::<c_void>(),
        )
    }

    fn invoke(
        &self,
        raw_kind: u32,
        url: *const c_char,
        out_response: *mut sys::mln_resource_transform_response,
    ) -> sys::mln_status {
        // SAFETY: out_response is callback-duration output storage provided by
        // native; core validates null before initializing it.
        let status = unsafe {
            maplibre_core::resource::initialize_resource_transform_response(out_response)
        };
        if status != sys::MLN_STATUS_OK {
            return status;
        }

        // SAFETY: url is borrowed for the callback duration by the C API.
        let request = match unsafe {
            maplibre_core::resource::copy_resource_transform_request(raw_kind, url)
        } {
            Ok(request) => request,
            Err(error) => return maplibre_core::resource::status_for_error(&error),
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

#[cfg(test)]
mod tests {
    use std::ffi::CStr;
    use std::sync::Mutex as StdMutex;
    use std::sync::atomic::{AtomicBool, AtomicI32, AtomicUsize, Ordering};

    use static_assertions::{assert_impl_all, assert_not_impl_any};

    use super::*;
    use crate::{
        ErrorKind, ResourceKind, ResourceLoadingMethod, ResourcePriority, ResourceStoragePolicy,
        ResourceUsage,
    };

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
        // SAFETY: These fake functions implement the same ownership contract as
        // the native handle functions for tests.
        unsafe { ResourceRequestHandleFns::new(fake_complete, fake_cancelled, fake_release) }
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
                let range = request.range.unwrap();
                assert_eq!((range.start, range.end), (7, 11));
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
