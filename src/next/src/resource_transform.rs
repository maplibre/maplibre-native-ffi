// Ports maplibre-native-ffi src/resources/resource_transform.cpp and the
// resource transform half of src/runtime/runtime.cpp (set/clear
// resource_transform, queue_resource_transform, invoke_resource_transform):
// a callback that rewrites the URL of each network request, now or later,
// from any thread.

use crate::delivery::{Completion, UserData, deliver};
use crate::diagnostics::{Failure, boundary};
use crate::handles::{self, Object};
use crate::resource_loader::{Registration, TransformRegistration, resource_state};
use crate::resource_provider::from_unix_ms;
use crate::types::*;
use crate::url_rules::validate_c_string_view;
use maplibre_native_next::mln_storage::response::{Error, Reason};
use std::sync::{Arc, Mutex};

/// Answers a transformed request: the URL to load, or why it fails.
pub type TransformFinished = Box<dyn FnOnce(Result<String, Error>) + Send>;

/// `ResourceTransformRequestObject`. The finished callback posts to the
/// waiting request's mailbox, which drops the answer once the request is
/// cancelled, so the host may answer from any thread at any time.
pub struct ResourceTransformRequestObject {
    /// Empty once the request is answered.
    finished: Mutex<Option<TransformFinished>>,
}

fn unanswered_error() -> Error {
    Error::new(Reason::Other, "the resource transform released the request without an answer")
}

/// A queued transform call. A call that the queue drops without running
/// fails its request.
struct QueuedTransformCall {
    request: u64,
    ran: bool,
}

impl Drop for QueuedTransformCall {
    fn drop(&mut self) {
        if !self.ran {
            // SAFETY: the request is this call's to release.
            unsafe { mln_resource_transform_request_release(self.request.into()) };
        }
    }
}

/// `request_resource_transform`: hands `url` to the transform with a request
/// handle, which answers `finished` once.
fn request_resource_transform(
    registration: Arc<TransformRegistration>,
    kind: u32,
    url: &str,
    finished: TransformFinished,
) {
    let object = Arc::new(ResourceTransformRequestObject { finished: Mutex::new(Some(finished)) });
    let request = handles::insert(Object::ResourceTransformRequest(object));
    let Some(callback) = registration.callback else {
        // SAFETY: the request is ours to release.
        unsafe { mln_resource_transform_request_release(request.into()) };
        return;
    };
    let user_data = registration.user_data;
    let text = LentText::new(url);
    if registration.queue == 0 {
        // SAFETY: the caller registered this callback and user_data; the
        // URL lives until it returns.
        unsafe { callback(user_data.get(), kind, mln_buffer_view::of_text(&text), request.into()) };
        return;
    }
    // The queue keeps a copy of the URL. The registration's release, queued
    // when the runtime retires it, follows the call.
    let queue = registration.queue;
    let call = QueuedTransformCall { request, ran: false };
    deliver(
        queue,
        Box::new(move || {
            let mut call = call;
            call.ran = true;
            // SAFETY: as above; the queue keeps a copy of the URL.
            unsafe { callback(user_data.get(), kind, mln_buffer_view::of_text(&text), call.request.into()) };
        }),
    );
}

/// `invoke_resource_transform`: answers `finished` with the URL that the
/// transform of the runtime with platform context `token` makes of `url`,
/// now or once the host answers.
pub fn invoke_resource_transform(token: usize, kind: u32, url: &str, finished: TransformFinished) {
    match resource_state(token).and_then(|state| state.transform()) {
        Some(registration) => request_resource_transform(registration, kind, url, finished),
        None => finished(Ok(url.to_owned())),
    }
}

/// Answers a live request once and consumes its handle.
fn answer(request: mln_resource_transform_request, result: Result<String, Error>) -> Result<(), Failure> {
    let object = handles::resource_transform_request(request)?;
    let Some(finished) = object.finished.lock().unwrap().take() else {
        return Err(Failure::invalid_state("resource transform request is already answered"));
    };
    finished(result);
    handles::remove(request);
    Ok(())
}

/// # Safety
/// C API contract (runtime.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_resource_transform_request_complete(
    request: mln_resource_transform_request,
    url: mln_buffer_view,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            let url = validate_c_string_view(url, "url")?;
            if url.is_empty() {
                return Err(Failure::invalid_argument("url must not be empty"));
            }
            answer(request, Ok(String::from_utf8_lossy(url).into_owned()))
        })
    }
}

/// # Safety
/// C API contract (runtime.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_resource_transform_request_fail(
    request: mln_resource_transform_request,
    error: *const mln_resource_transform_error,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            if error.is_null() {
                return Err(Failure::invalid_argument("error must not be null"));
            }
            let Some(error) = read_sized(error) else {
                return Err(Failure::invalid_argument("mln_resource_transform_error.size is too small"));
            };
            if error.fields & !MLN_RESOURCE_TRANSFORM_ERROR_RETRY_AFTER != 0 {
                return Err(Failure::invalid_argument("mln_resource_transform_error.fields has unknown bits"));
            }
            let reason = match error.reason {
                MLN_RESOURCE_ERROR_REASON_NOT_FOUND => Reason::NotFound,
                MLN_RESOURCE_ERROR_REASON_SERVER => Reason::Server,
                MLN_RESOURCE_ERROR_REASON_CONNECTION => Reason::Connection,
                MLN_RESOURCE_ERROR_REASON_RATE_LIMIT => Reason::RateLimit,
                MLN_RESOURCE_ERROR_REASON_OTHER => Reason::Other,
                _ => {
                    return Err(Failure::invalid_argument("mln_resource_transform_error.reason must name an error"));
                }
            };
            let message = validate_c_string_view(error.message, "message")?;
            let retry_after = (error.fields & MLN_RESOURCE_TRANSFORM_ERROR_RETRY_AFTER != 0)
                .then(|| from_unix_ms(error.retry_after_unix_ms));
            let message = if message.is_empty() {
                "resource transform failed".to_owned()
            } else {
                String::from_utf8_lossy(message).into_owned()
            };
            answer(request, Err(Error::with_retry_after(reason, message, retry_after)))
        })
    }
}

/// # Safety
/// C API contract (runtime.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_resource_transform_request_release(request: mln_resource_transform_request) {
    if let Some(Object::ResourceTransformRequest(object)) = handles::remove(request) {
        let finished = object.finished.lock().unwrap().take();
        if let Some(finished) = finished {
            finished(Err(unanswered_error()));
        }
    }
}

/// Swaps the runtime's transform on its thread and completes.
fn replace(
    runtime: mln_runtime,
    completion: Option<Completion>,
    next: Option<Arc<TransformRegistration>>,
) -> Result<(), Failure> {
    let shared = handles::runtime(runtime)?;
    let state = shared.resources.clone();
    shared.post(move || {
        let previous = state.replace_transform(next);
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
pub unsafe extern "C" fn mln_runtime_set_resource_transform(
    runtime: mln_runtime,
    transform: *const mln_resource_transform,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            handles::runtime(runtime)?;
            if transform.is_null() {
                return Err(Failure::invalid_argument("resource transform must not be null"));
            }
            let Some(transform) = read_sized(transform) else {
                return Err(Failure::invalid_argument("mln_resource_transform.size is too small"));
            };
            if transform.callback.is_none() {
                return Err(Failure::invalid_argument("resource transform requires a callback"));
            }
            if transform.queue.0 != 0 {
                handles::queue(transform.queue)?;
            }
            let completion = Completion::optional(completion)?;
            let registration = Registration::new(
                transform.callback,
                UserData(transform.user_data),
                transform.release_user_data,
                transform.queue.0,
            );
            replace(runtime, completion, Some(Arc::new(registration)))
        })
    }
}

/// # Safety
/// C API contract (runtime.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_runtime_clear_resource_transform(
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
