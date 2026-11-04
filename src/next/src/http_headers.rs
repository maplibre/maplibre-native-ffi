// Ports the HTTP header transform of maplibre-native-ffi
// src/runtime/runtime.cpp (set_http_header_transform, validate_http_header,
// http_header_transform_response_set, clear_http_header_transform,
// invoke_http_header_transform): the headers that a runtime adds to its HTTP
// requests, from a callback or a table of rules.

use crate::delivery::{Completion, UserData};
use crate::diagnostics::{Failure, boundary};
use crate::handles;
use crate::resource_loader::{Registration, resource_state};
use crate::types::*;
use crate::url_rules::{HeaderRuleTable, copy_header_rules, validate_c_string_view};
use std::sync::Arc;

/// A registered HTTP header transform.
pub enum HeaderTransform {
    Callback(Registration<mln_http_header_transform_callback>),
    /// Owns its strings; a rules transform carries no user_data.
    Rules(HeaderRuleTable),
}

/// The headers that the library manages itself or leaves to the transport.
const MANAGED_HEADERS: [&str; 15] = [
    "Host",
    "Content-Length",
    "Transfer-Encoding",
    "Connection",
    "Proxy-Connection",
    "Proxy-Authorization",
    "Keep-Alive",
    "TE",
    "Trailer",
    "Upgrade",
    "Range",
    "If-None-Match",
    "If-Modified-Since",
    "Accept-Encoding",
    "User-Agent",
];

fn is_field_name_character(character: u8) -> bool {
    character.is_ascii_alphanumeric() || b"!#$%&'*+-.^_`|~".contains(&character)
}

/// `validate_http_header`: the name and value, when they make a header the
/// library may send.
pub fn validate_http_header<'a>(
    name: mln_buffer_view,
    value: mln_buffer_view,
) -> Result<(&'a [u8], &'a [u8]), Failure> {
    let name = validate_c_string_view(name, "HTTP header name")?;
    let value = validate_c_string_view(value, "HTTP header value")?;
    if name.is_empty() || !name.iter().all(|&character| is_field_name_character(character)) {
        return Err(Failure::invalid_argument("HTTP header name is not a valid field name"));
    }
    if !value.iter().all(|&byte| byte == b'\t' || (byte >= 0x20 && byte != 0x7F)) {
        return Err(Failure::invalid_argument("HTTP header value contains a disallowed control byte"));
    }
    if std::str::from_utf8(value).is_err() {
        return Err(Failure::invalid_argument("HTTP header value must be valid UTF-8"));
    }
    if MANAGED_HEADERS.iter().any(|managed| managed.as_bytes().eq_ignore_ascii_case(name)) {
        return Err(Failure::invalid_argument(format!(
            "HTTP header \"{}\" is managed by MapLibre or the platform transport",
            String::from_utf8_lossy(name)
        )));
    }
    Ok((name, value))
}

/// Sets `name` to `value`, replacing a header of the same name in any case.
pub fn set_header(headers: &mut Vec<(String, String)>, name: &str, value: &str) {
    let header = (name.to_owned(), value.to_owned());
    match headers.iter_mut().find(|(existing, _)| existing.eq_ignore_ascii_case(name)) {
        Some(existing) => *existing = header,
        None => headers.push(header),
    }
}

/// `invoke_http_header_transform`: the headers that the transform of the
/// runtime with platform context `token` adds to a request for `url`. A
/// callback that fails adds none.
pub fn invoke_http_header_transform(token: usize, kind: u32, url: &str) -> Vec<(String, String)> {
    let mut headers = Vec::new();
    let Some(transform) = resource_state(token).and_then(|state| state.header_transform()) else {
        return headers;
    };
    match &*transform {
        HeaderTransform::Rules(table) => table.apply(kind, url, &mut headers),
        HeaderTransform::Callback(registration) => {
            let Some(callback) = registration.callback else {
                return headers;
            };
            let text = LentText::new(url);
            let mut response = mln_http_header_transform_response {
                size: size_of::<mln_http_header_transform_response>() as u32,
                context: (&raw mut headers).cast(),
            };
            // SAFETY: the caller registered this callback and user_data;
            // `text` and `response` live until it returns.
            let status =
                unsafe { callback(registration.user_data.get(), kind, mln_buffer_view::of_text(&text), &mut response) };
            if status != MLN_STATUS_OK {
                return Vec::new();
            }
        }
    }
    headers
}

/// # Safety
/// C API contract (runtime.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_http_header_transform_response_set_header(
    response: *mut mln_http_header_transform_response,
    name: mln_buffer_view,
    value: mln_buffer_view,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            if response.is_null() {
                return Err(Failure::invalid_argument("HTTP header transform response must not be null"));
            }
            if ((*response).size as usize) < size_of::<mln_http_header_transform_response>() {
                return Err(Failure::invalid_argument("mln_http_header_transform_response.size is too small"));
            }
            let (name, value) = validate_http_header(name, value)?;
            let headers = (*response).context.cast::<Vec<(String, String)>>();
            if headers.is_null() {
                return Err(Failure::invalid_state(
                    "HTTP headers can only be set during an HTTP header transform callback",
                ));
            }
            // Validated as UTF-8 (the value) and ASCII (the name).
            set_header(&mut *headers, &String::from_utf8_lossy(name), &String::from_utf8_lossy(value));
            Ok(())
        })
    }
}

/// Swaps the runtime's header transform on its thread and completes, as
/// C++ `submit_resource_configuration_command` does.
fn replace(
    runtime: mln_runtime,
    completion: Option<Completion>,
    next: Option<Arc<HeaderTransform>>,
) -> Result<(), Failure> {
    let shared = handles::runtime(runtime)?;
    let state = shared.resources.clone();
    shared.post(move || {
        let previous = state.replace_header_transform(next);
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
pub unsafe extern "C" fn mln_runtime_set_http_header_transform(
    runtime: mln_runtime,
    transform: *const mln_http_header_transform,
    completion: *const mln_completion,
    out_diagnostic: *mut mln_diagnostic,
) -> mln_status {
    // SAFETY: C API contract.
    unsafe {
        boundary(out_diagnostic, || {
            handles::runtime(runtime)?;
            if transform.is_null() {
                return Err(Failure::invalid_argument("HTTP header transform must not be null"));
            }
            let Some(transform) = read_sized(transform) else {
                return Err(Failure::invalid_argument("mln_http_header_transform.size is too small"));
            };
            let rules = if transform.callback.is_none() {
                if transform.rule_count == 0 {
                    return Err(Failure::invalid_argument(
                        "a transform requires a callback or rules; use the clear function",
                    ));
                }
                if !transform.user_data.is_null() || transform.release_user_data.is_some() {
                    return Err(Failure::invalid_argument(
                        "a rules HTTP header transform must not carry user_data or release_user_data",
                    ));
                }
                Some(copy_header_rules(transform.rules, transform.rule_count)?)
            } else {
                if transform.rule_count != 0 {
                    return Err(Failure::invalid_argument("an HTTP header transform callback takes no rules"));
                }
                None
            };
            if cfg!(target_os = "emscripten") {
                return Err(Failure::new(
                    MLN_STATUS_UNSUPPORTED,
                    "HTTP header transforms are unsupported in the browser because emscripten_fetch follows \
                     redirects itself and cannot drop transformed headers when the origin changes; serve the \
                     request with mln_runtime_set_resource_provider() instead",
                ));
            }
            let completion = Completion::optional(completion)?;
            let next = match rules {
                Some(rules) => HeaderTransform::Rules(rules),
                None => HeaderTransform::Callback(Registration::new(
                    transform.callback,
                    UserData(transform.user_data),
                    transform.release_user_data,
                    0,
                )),
            };
            replace(runtime, completion, Some(Arc::new(next)))
        })
    }
}

/// # Safety
/// C API contract (runtime.h).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_runtime_clear_http_header_transform(
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
