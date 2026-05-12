use std::collections::HashMap;
use std::ffi::CString;
use std::fmt;
use std::os::raw::{c_char, c_void};
use std::panic::{AssertUnwindSafe, catch_unwind};
use std::ptr;
use std::sync::Mutex;
use std::thread::ThreadId;

use maplibre_native_support as support;
use maplibre_native_sys as sys;

use crate::{Error, ErrorKind};

/// Network resource kind passed to a resource transform callback.
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
    use std::sync::Arc;
    use std::sync::atomic::{AtomicUsize, Ordering};

    use super::*;

    fn response() -> sys::mln_resource_transform_response {
        sys::mln_resource_transform_response {
            size: std::mem::size_of::<sys::mln_resource_transform_response>() as u32,
            url: ptr::null(),
        }
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
    fn no_op_transform_descriptor_discards_without_state() {
        let descriptor = noop_resource_transform_descriptor();
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
        assert!(response.url.is_null());
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
