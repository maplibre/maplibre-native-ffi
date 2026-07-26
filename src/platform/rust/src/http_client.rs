use std::ffi::CString;
use std::os::raw::{c_char, c_void};
use std::ptr;
use std::slice;
use std::slice::from_raw_parts_mut;
use std::sync::{
    Arc, OnceLock,
    atomic::{AtomicBool, Ordering},
};
use std::time::Duration;

// Matches `mbgl::util::DEFAULT_MAXIMUM_CONCURRENT_REQUESTS` enforced by
// `OnlineFileSource` in maplibre-native's default platform layer.
const HTTP_WORKER_THREADS: usize = 20;
// Upstream default `http_file_source.cpp` does not set `CURLOPT_TIMEOUT`; cap
// connection establishment so hung TCP/TLS handshakes do not occupy a worker
// thread indefinitely, without bounding slow but progressing body downloads.
const HTTP_CONNECT_TIMEOUT_SECONDS: u64 = 30;
// Upstream uses `CURLOPT_FOLLOWLOCATION` without `CURLOPT_MAXREDIRS`; match
// libcurl 8's default redirect limit.
const HTTP_MAX_REDIRECTS: u32 = 30;

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

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mlnffi_rust_http_request_start(
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
    http_thread_pool().execute(move || {
        let response = if thread_canceled.load(Ordering::Acquire) {
            http_error(HTTP_ERROR_OTHER, "HTTP request canceled")
        } else {
            send_http_request(request)
        };
        // SAFETY: The C++ caller keeps `user_data` valid until this callback
        // runs, even when the request is canceled.
        unsafe {
            callback(callback_user_data as *mut c_void, response);
        }
    });

    Box::into_raw(Box::new(HttpRequestHandle { canceled })) as *mut c_void
}

fn http_thread_pool() -> &'static threadpool::ThreadPool {
    static HTTP_THREAD_POOL: OnceLock<threadpool::ThreadPool> = OnceLock::new();
    HTTP_THREAD_POOL.get_or_init(|| threadpool::ThreadPool::new(HTTP_WORKER_THREADS))
}

fn http_agent() -> &'static ureq::Agent {
    static AGENT: OnceLock<ureq::Agent> = OnceLock::new();
    AGENT.get_or_init(|| {
        let mut tls_builder =
            ureq::tls::TlsConfig::builder().root_certs(ureq::tls::RootCerts::PlatformVerifier);
        #[cfg(target_os = "windows")]
        {
            tls_builder = tls_builder.provider(ureq::tls::TlsProvider::NativeTls);
        }
        #[cfg(not(target_os = "windows"))]
        {
            tls_builder = tls_builder.provider(ureq::tls::TlsProvider::Rustls);
        }

        let mut config_builder = ureq::config::Config::builder()
            .http_status_as_error(false)
            // Redirects are handled manually so each hop can be checked on Android.
            .max_redirects(0)
            .timeout_connect(Some(Duration::from_secs(HTTP_CONNECT_TIMEOUT_SECONDS)))
            .tls_config(tls_builder.build());
        if let Some(proxy) = ureq::Proxy::try_from_env() {
            config_builder = config_builder.proxy(Some(proxy));
        }
        config_builder.build().into()
    })
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mlnffi_rust_http_request_cancel(handle: *mut c_void) {
    if handle.is_null() {
        return;
    }

    // SAFETY: `handle` was created by `mlnffi_rust_http_request_start`.
    let handle = unsafe { &*(handle as *mut HttpRequestHandle) };
    handle.canceled.store(true, Ordering::Release);
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mlnffi_rust_http_request_free(handle: *mut c_void) {
    if handle.is_null() {
        return;
    }

    // SAFETY: `handle` was returned by `mlnffi_rust_http_request_start` and is
    // freed exactly once by the C++ owner.
    unsafe {
        drop(Box::from_raw(handle as *mut HttpRequestHandle));
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn mlnffi_rust_http_response_free(response: MlnRustHttpResponse) {
    free_http_response(response);
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
    let mut url = request.url;

    for _ in 0..=HTTP_MAX_REDIRECTS {
        if is_https_url(&url) && !crate::android::tls_verifier_initialized() {
            return http_error(
                HTTP_ERROR_OTHER,
                "Android TLS verifier is not initialized; call mln_android_init before HTTPS requests",
            );
        }

        let agent = http_agent();
        let mut builder = agent.get(&url);
        let has_accept_encoding = request
            .headers
            .iter()
            .any(|(name, _)| name.eq_ignore_ascii_case("accept-encoding"));
        let has_range = request
            .headers
            .iter()
            .any(|(name, _)| name.eq_ignore_ascii_case("range"));
        for (name, value) in &request.headers {
            builder = builder.header(name, value);
        }
        if !has_accept_encoding && has_range {
            builder = builder.header("Accept-Encoding", "identity");
        }

        match builder.call() {
            Ok(mut response) => {
                let status_code = response.status().as_u16();
                if is_redirect_status(status_code) {
                    let Some(location) = response_header(&response, "location") else {
                        return http_error(
                            HTTP_ERROR_OTHER,
                            "HTTP redirect missing Location header",
                        );
                    };
                    if location.is_empty() {
                        return http_error(
                            HTTP_ERROR_OTHER,
                            "HTTP redirect Location header is empty",
                        );
                    }
                    url = match redirect_url(&url, &location) {
                        Some(url) => url,
                        None => {
                            return http_error(HTTP_ERROR_OTHER, "unsupported HTTP redirect URL");
                        }
                    };
                    continue;
                }
                return http_response(&mut response, has_range);
            }
            Err(error) => return http_error(map_transport_error(&error), &error.to_string()),
        }
    }

    http_error(HTTP_ERROR_OTHER, "too many HTTP redirects")
}

fn http_response(
    response: &mut http::Response<ureq::Body>,
    ranged_request: bool,
) -> MlnRustHttpResponse {
    let status_code = response.status().as_u16();
    if ranged_request && (status_code == 200 || status_code == 206) {
        if let Some(encoding) = response_header(response, "content-encoding")
            && !encoding.eq_ignore_ascii_case("identity")
        {
            return http_error(
                HTTP_ERROR_OTHER,
                &format!("unsupported HTTP content encoding: {encoding}"),
            );
        }
    }

    let etag = response_header(response, "etag");
    let modified = response_header(response, "last-modified");
    let cache_control = response_header(response, "cache-control");
    let expires = response_header(response, "expires");
    let retry_after = response_header(response, "retry-after");
    let x_rate_limit_reset = response_header(response, "x-rate-limit-reset");

    // libcurl/minreq had no response body cap; ureq's convenience read_to_vec() defaults to 10 MiB.
    let body = match response
        .body_mut()
        .with_config()
        .limit(u64::MAX)
        .read_to_vec()
    {
        Ok(body) => body,
        Err(error) => {
            return http_error(HTTP_ERROR_CONNECTION, &error.to_string());
        }
    };

    let data_len = body.len();
    let data = if body.is_empty() {
        ptr::null_mut()
    } else {
        let body = body.into_boxed_slice();
        Box::into_raw(body) as *mut u8
    };

    MlnRustHttpResponse {
        status_code,
        error_reason: 0,
        data,
        data_len,
        error: ptr::null_mut(),
        etag: optional_c_string_ptr(etag),
        modified: optional_c_string_ptr(modified),
        cache_control: optional_c_string_ptr(cache_control),
        expires: optional_c_string_ptr(expires),
        retry_after: optional_c_string_ptr(retry_after),
        x_rate_limit_reset: optional_c_string_ptr(x_rate_limit_reset),
    }
}

fn response_header(response: &http::Response<ureq::Body>, name: &str) -> Option<String> {
    response
        .headers()
        .get(name)
        .and_then(|value| value.to_str().ok())
        .map(str::to_owned)
}

fn is_redirect_status(status_code: u16) -> bool {
    matches!(status_code, 301 | 302 | 303 | 307 | 308)
}

fn redirect_url(current_url: &str, location: &str) -> Option<String> {
    if location
        .get(..7)
        .is_some_and(|url| url.eq_ignore_ascii_case("http://"))
        || location
            .get(..8)
            .is_some_and(|url| url.eq_ignore_ascii_case("https://"))
    {
        return Some(location.to_owned());
    }

    if has_url_scheme(location) {
        return None;
    }

    let scheme_end = current_url.find("://")?;
    let scheme = &current_url[..scheme_end];
    if !scheme.eq_ignore_ascii_case("http") && !scheme.eq_ignore_ascii_case("https") {
        return None;
    }
    if location.starts_with("//") {
        return Some(format!("{scheme}:{location}"));
    }

    let authority_start = scheme_end + 3;
    let authority_len = current_url[authority_start..]
        .find(['/', '?', '#'])
        .unwrap_or(current_url.len() - authority_start);
    let origin_end = authority_start + authority_len;
    let origin = &current_url[..origin_end];
    if location.starts_with('/') {
        let (path, suffix) = split_url_path_suffix(location);
        return Some(format!("{origin}{}{suffix}", normalize_url_path(path)));
    }

    if location.starts_with('?') {
        let path_end = current_url.find(['?', '#']).unwrap_or(current_url.len());
        return Some(format!("{}{location}", &current_url[..path_end]));
    }

    if location.starts_with('#') {
        let fragment_start = current_url.find('#').unwrap_or(current_url.len());
        return Some(format!("{}{location}", &current_url[..fragment_start]));
    }

    let path_end = current_url.find(['?', '#']).unwrap_or(current_url.len());
    let current_path = &current_url[origin_end..path_end];
    let directory_path = current_path
        .rfind('/')
        .map_or("/", |index| &current_path[..=index]);
    let (path, suffix) = split_url_path_suffix(location);
    Some(format!(
        "{origin}{}{suffix}",
        normalize_url_path(&format!("{directory_path}{path}"))
    ))
}

fn has_url_scheme(value: &str) -> bool {
    let scheme_end = value.find([':', '/', '?', '#']);
    matches!(scheme_end, Some(index) if value.as_bytes()[index] == b':')
}

fn split_url_path_suffix(value: &str) -> (&str, &str) {
    match value.find(['?', '#']) {
        Some(index) => (&value[..index], &value[index..]),
        None => (value, ""),
    }
}

fn normalize_url_path(path: &str) -> String {
    if !path
        .split('/')
        .any(|segment| segment == "." || segment == "..")
    {
        return path.to_owned();
    }

    let is_absolute = path.starts_with('/');
    let has_trailing_slash = path.ends_with('/') || path.ends_with("/.") || path.ends_with("/..");
    let mut segments = Vec::new();
    for segment in path.split('/') {
        match segment {
            "" | "." => {}
            ".." => {
                segments.pop();
            }
            segment => segments.push(segment),
        }
    }

    let mut normalized = String::new();
    if is_absolute {
        normalized.push('/');
    }
    normalized.push_str(&segments.join("/"));
    if has_trailing_slash && !normalized.ends_with('/') {
        normalized.push('/');
    }
    if normalized.is_empty() {
        normalized.push('/');
    }
    normalized
}

fn map_transport_error(error: &ureq::Error) -> u8 {
    match error {
        ureq::Error::HostNotFound
        | ureq::Error::ConnectionFailed
        | ureq::Error::Timeout(_)
        | ureq::Error::Io(_)
        | ureq::Error::ConnectProxyFailed(_) => HTTP_ERROR_CONNECTION,
        _ => HTTP_ERROR_OTHER,
    }
}

fn is_https_url(url: &str) -> bool {
    url.get(..8)
        .is_some_and(|scheme| scheme.eq_ignore_ascii_case("https://"))
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
        // SAFETY: `http_response` returns `data` from `Box<[u8]>`, and the
        // caller returns ownership here with the original length.
        unsafe {
            drop(Box::from_raw(from_raw_parts_mut(
                response.data,
                response.data_len,
            )));
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

fn optional_c_string_ptr(value: Option<String>) -> *mut c_char {
    value
        .and_then(|value| c_string_ptr(&value))
        .unwrap_or(ptr::null_mut())
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
