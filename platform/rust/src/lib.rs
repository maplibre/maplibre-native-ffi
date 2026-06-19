use std::ffi::CString;
use std::os::raw::{c_char, c_void};
use std::ptr;
use std::slice;
use std::sync::{
    Arc,
    atomic::{AtomicBool, Ordering},
};
use std::thread;

#[repr(C)]
pub struct MlnRustHttpHeader {
    pub name: *const c_char,
    pub value: *const c_char,
}

#[repr(C)]
pub struct MlnRustHttpResponse {
    pub status_code: u16,
    pub error_reason: u8,
    pub data: *mut u8,
    pub data_len: usize,
    pub error: *mut c_char,
    pub etag: *mut c_char,
    pub modified: *mut c_char,
    pub cache_control: *mut c_char,
    pub expires: *mut c_char,
    pub retry_after: *mut c_char,
    pub x_rate_limit_reset: *mut c_char,
}

type MlnRustHttpCallback = unsafe extern "C" fn(*mut c_void, MlnRustHttpResponse);

const HTTP_ERROR_CONNECTION: u8 = 1;
const HTTP_ERROR_OTHER: u8 = 2;

struct HttpRequestHandle {
    canceled: Arc<AtomicBool>,
}

#[repr(C)]
pub struct MlnRustDecodedImage {
    pub width: u32,
    pub height: u32,
    pub data: *mut u8,
    pub data_len: usize,
    pub error: *mut c_char,
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_rust_http_request_start(
    url: *const c_char,
    headers: *const MlnRustHttpHeader,
    header_count: usize,
    callback: MlnRustHttpCallback,
    user_data: *mut c_void,
) -> *mut c_void {
    let Some(request) = copy_http_request(url, headers, header_count) else {
        let response = http_error(HTTP_ERROR_OTHER, "invalid HTTP request");
        // SAFETY: The callback and user data are supplied by the C++ caller.
        unsafe {
            callback(user_data, response);
        }
        return ptr::null_mut();
    };

    let canceled = Arc::new(AtomicBool::new(false));
    let thread_canceled = Arc::clone(&canceled);
    let callback_user_data = user_data as usize;
    thread::spawn(move || {
        let response = send_http_request(request);
        let _ = thread_canceled.load(Ordering::Acquire);
        // SAFETY: The C++ caller keeps `user_data` valid until this callback
        // runs, even when the request is canceled.
        unsafe {
            callback(callback_user_data as *mut c_void, response);
        }
    });

    Box::into_raw(Box::new(HttpRequestHandle { canceled })) as *mut c_void
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_rust_http_request_cancel(handle: *mut c_void) {
    if handle.is_null() {
        return;
    }

    // SAFETY: `handle` was created by `mln_rust_http_request_start`.
    let handle = unsafe { &*(handle as *mut HttpRequestHandle) };
    handle.canceled.store(true, Ordering::Release);
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_rust_http_request_free(handle: *mut c_void) {
    if handle.is_null() {
        return;
    }

    // SAFETY: `handle` was returned by `mln_rust_http_request_start` and is
    // freed exactly once by the C++ owner.
    unsafe {
        drop(Box::from_raw(handle as *mut HttpRequestHandle));
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_rust_http_response_free(response: MlnRustHttpResponse) {
    free_http_response(response);
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_rust_decode_image(
    data: *const u8,
    data_len: usize,
) -> MlnRustDecodedImage {
    if data.is_null() {
        return decode_error("image input pointer is null");
    }

    // SAFETY: The C++ caller passes a pointer/length pair valid for this call.
    let encoded = unsafe { slice::from_raw_parts(data, data_len) };
    match decode_image(encoded) {
        Ok(decoded) => decoded,
        Err(message) => decode_error(&message),
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_rust_decoded_image_free(image: MlnRustDecodedImage) {
    if !image.data.is_null() && image.data_len > 0 {
        // SAFETY: `mln_rust_decode_image` returns `data` from a Vec with
        // capacity equal to length, and ownership is transferred back here.
        unsafe {
            Vec::from_raw_parts(image.data, image.data_len, image.data_len);
        }
    }

    if !image.error.is_null() {
        // SAFETY: `decode_error` returns `error` from `CString::into_raw`.
        unsafe {
            drop(CString::from_raw(image.error));
        }
    }
}

fn decode_image(encoded: &[u8]) -> Result<MlnRustDecodedImage, String> {
    let image = image::load_from_memory(encoded).map_err(|error| error.to_string())?;
    let rgba = image.to_rgba8();
    let (width, height) = rgba.dimensions();
    let mut data = rgba.into_raw();

    premultiply_rgba(&mut data);

    let data_len = data.len();
    let data_ptr = data.as_mut_ptr();
    std::mem::forget(data);

    Ok(MlnRustDecodedImage {
        width,
        height,
        data: data_ptr,
        data_len,
        error: ptr::null_mut(),
    })
}

fn premultiply_rgba(data: &mut [u8]) {
    for pixel in data.chunks_exact_mut(4) {
        let alpha = u16::from(pixel[3]);
        pixel[0] = premultiply_channel(pixel[0], alpha);
        pixel[1] = premultiply_channel(pixel[1], alpha);
        pixel[2] = premultiply_channel(pixel[2], alpha);
    }
}

fn premultiply_channel(channel: u8, alpha: u16) -> u8 {
    ((u16::from(channel) * alpha + 127) / 255) as u8
}

fn decode_error(message: &str) -> MlnRustDecodedImage {
    let error = match CString::new(message) {
        Ok(error) => error.into_raw(),
        Err(_) => CString::new("image decode failed")
            .expect("static string has no interior nul")
            .into_raw(),
    };

    MlnRustDecodedImage {
        width: 0,
        height: 0,
        data: ptr::null_mut(),
        data_len: 0,
        error,
    }
}

struct HttpRequest {
    url: String,
    headers: Vec<(String, String)>,
}

fn copy_http_request(
    url: *const c_char,
    headers: *const MlnRustHttpHeader,
    header_count: usize,
) -> Option<HttpRequest> {
    if url.is_null() {
        return None;
    }

    let url = unsafe_c_string_to_string(url)?;
    let headers = if headers.is_null() || header_count == 0 {
        Vec::new()
    } else {
        // SAFETY: The C++ caller passes a valid array for this call.
        let headers = unsafe { slice::from_raw_parts(headers, header_count) };
        let mut copied = Vec::with_capacity(headers.len());
        for header in headers {
            copied.push((
                unsafe_c_string_to_string(header.name)?,
                unsafe_c_string_to_string(header.value)?,
            ));
        }
        copied
    };

    Some(HttpRequest { url, headers })
}

fn send_http_request(request: HttpRequest) -> MlnRustHttpResponse {
    let mut minreq = minreq::get(request.url);
    for (name, value) in request.headers {
        minreq = minreq.with_header(name, value);
    }

    match minreq.send() {
        Ok(response) => http_response(response),
        Err(error) => {
            let reason = match error {
                minreq::Error::AddressNotFound | minreq::Error::IoError(_) => HTTP_ERROR_CONNECTION,
                _ => HTTP_ERROR_OTHER,
            };
            http_error(reason, &error.to_string())
        }
    }
}

fn http_response(response: minreq::Response) -> MlnRustHttpResponse {
    let status_code = response.status_code;
    let etag = response
        .header("etag")
        .and_then(c_string_ptr)
        .unwrap_or(ptr::null_mut());
    let modified = response
        .header("last-modified")
        .and_then(c_string_ptr)
        .unwrap_or(ptr::null_mut());
    let cache_control = response
        .header("cache-control")
        .and_then(c_string_ptr)
        .unwrap_or(ptr::null_mut());
    let expires = response
        .header("expires")
        .and_then(c_string_ptr)
        .unwrap_or(ptr::null_mut());
    let retry_after = response
        .header("retry-after")
        .and_then(c_string_ptr)
        .unwrap_or(ptr::null_mut());
    let x_rate_limit_reset = response
        .header("x-rate-limit-reset")
        .and_then(c_string_ptr)
        .unwrap_or(ptr::null_mut());

    let mut body = response.into_bytes();
    let data_len = body.len();
    let data = if body.is_empty() {
        ptr::null_mut()
    } else {
        let data = body.as_mut_ptr();
        std::mem::forget(body);
        data
    };

    MlnRustHttpResponse {
        status_code,
        error_reason: 0,
        data,
        data_len,
        error: ptr::null_mut(),
        etag,
        modified,
        cache_control,
        expires,
        retry_after,
        x_rate_limit_reset,
    }
}

fn http_error(reason: u8, message: &str) -> MlnRustHttpResponse {
    MlnRustHttpResponse {
        status_code: 0,
        error_reason: reason,
        data: ptr::null_mut(),
        data_len: 0,
        error: c_string_ptr(message).unwrap_or_else(|| {
            CString::new("HTTP request failed")
                .expect("static string has no interior nul")
                .into_raw()
        }),
        etag: ptr::null_mut(),
        modified: ptr::null_mut(),
        cache_control: ptr::null_mut(),
        expires: ptr::null_mut(),
        retry_after: ptr::null_mut(),
        x_rate_limit_reset: ptr::null_mut(),
    }
}

fn free_http_response(response: MlnRustHttpResponse) {
    if !response.data.is_null() && response.data_len > 0 {
        // SAFETY: `http_response` returns `data` from a Vec with capacity equal
        // to length, and ownership is transferred back here.
        unsafe {
            drop(Vec::from_raw_parts(
                response.data,
                response.data_len,
                response.data_len,
            ));
        }
    }

    free_c_string(response.error);
    free_c_string(response.etag);
    free_c_string(response.modified);
    free_c_string(response.cache_control);
    free_c_string(response.expires);
    free_c_string(response.retry_after);
    free_c_string(response.x_rate_limit_reset);
}

fn c_string_ptr(value: &str) -> Option<*mut c_char> {
    CString::new(value).ok().map(CString::into_raw)
}

fn free_c_string(value: *mut c_char) {
    if value.is_null() {
        return;
    }

    // SAFETY: These pointers are returned by `CString::into_raw`.
    unsafe {
        drop(CString::from_raw(value));
    }
}

fn unsafe_c_string_to_string(value: *const c_char) -> Option<String> {
    if value.is_null() {
        return None;
    }

    // SAFETY: The C++ caller passes nul-terminated strings valid for this call.
    unsafe { std::ffi::CStr::from_ptr(value) }
        .to_str()
        .ok()
        .map(str::to_owned)
}
