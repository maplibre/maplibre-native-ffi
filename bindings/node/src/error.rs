use napi::{Error as NapiError, Status};

const NATIVE_ERROR_PREFIX: &str = "MaplibreNativeError:";

pub(crate) fn from_core(error: maplibre_native_core::Error) -> NapiError {
    let kind = match error.kind() {
        maplibre_native_core::ErrorKind::InvalidArgument => "InvalidArgument",
        maplibre_native_core::ErrorKind::InvalidState => "InvalidState",
        maplibre_native_core::ErrorKind::WrongThread => "WrongThread",
        maplibre_native_core::ErrorKind::Unsupported => "Unsupported",
        maplibre_native_core::ErrorKind::NativeError => "NativeError",
        maplibre_native_core::ErrorKind::AbiVersionMismatch => "AbiVersionMismatch",
        maplibre_native_core::ErrorKind::UnknownStatus => "UnknownStatus",
        _ => "UnknownStatus",
    };
    let native_status_code = error
        .raw_status()
        .map_or_else(|| "null".to_owned(), |status| status.to_string());
    let diagnostic = json_string(error.diagnostic());
    NapiError::new(
        Status::GenericFailure,
        format!(
            "{NATIVE_ERROR_PREFIX}{{\"kind\":\"{kind}\",\"nativeStatusCode\":{native_status_code},\"diagnostic\":{diagnostic}}}"
        ),
    )
}

pub(crate) fn invalid_argument(message: impl Into<String>) -> NapiError {
    from_core(maplibre_native_core::Error::invalid_argument(message))
}

pub(crate) fn invalid_state(message: impl Into<String>) -> NapiError {
    from_core(maplibre_native_core::Error::new(
        maplibre_native_core::ErrorKind::InvalidState,
        None,
        message,
    ))
}

fn json_string(value: &str) -> String {
    let mut out = String::with_capacity(value.len() + 2);
    out.push('"');
    for ch in value.chars() {
        match ch {
            '"' => out.push_str("\\\""),
            '\\' => out.push_str("\\\\"),
            '\n' => out.push_str("\\n"),
            '\r' => out.push_str("\\r"),
            '\t' => out.push_str("\\t"),
            '\u{08}' => out.push_str("\\b"),
            '\u{0c}' => out.push_str("\\f"),
            ch if ch.is_control() => out.push_str(&format!("\\u{:04x}", ch as u32)),
            ch => out.push(ch),
        }
    }
    out.push('"');
    out
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn json_string_escapes_control_characters() {
        assert_eq!(json_string("a\"b\\c\n"), "\"a\\\"b\\\\c\\n\"");
    }
}
