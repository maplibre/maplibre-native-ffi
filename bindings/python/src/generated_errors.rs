// Generated from C headers by tools/bindgen. Do not edit.
mod py_errors {
    pyo3::import_exception!(maplibre_native_ffi.errors, InvalidArgumentError);
    pyo3::import_exception!(maplibre_native_ffi.errors, InvalidStateError);
    pyo3::import_exception!(maplibre_native_ffi.errors, WrongThreadError);
    pyo3::import_exception!(maplibre_native_ffi.errors, UnsupportedFeatureError);
    pyo3::import_exception!(maplibre_native_ffi.errors, NativeError);
    pyo3::import_exception!(maplibre_native_ffi.errors, CancelledError);
    pyo3::import_exception!(maplibre_native_ffi.errors, BusyError);
    pyo3::import_exception!(maplibre_native_ffi.errors, TargetLostError);
    pyo3::import_exception!(maplibre_native_ffi.errors, NotReadyError);
    pyo3::import_exception!(maplibre_native_ffi.errors, NotFoundError);
    pyo3::import_exception!(maplibre_native_ffi.errors, UnknownStatusError);
}
fn map_error(error: Error) -> PyErr {
    let diagnostic = error.diagnostic().to_owned();
    let raw_status = error.raw_status();
    match error.kind() {
        ErrorKind::InvalidArgument => {
            py_errors::InvalidArgumentError::new_err((diagnostic, raw_status))
        }
        ErrorKind::InvalidState => py_errors::InvalidStateError::new_err((diagnostic, raw_status)),
        ErrorKind::WrongThread => py_errors::WrongThreadError::new_err((diagnostic, raw_status)),
        ErrorKind::Unsupported => {
            py_errors::UnsupportedFeatureError::new_err((diagnostic, raw_status))
        }
        ErrorKind::NativeError => py_errors::NativeError::new_err((diagnostic, raw_status)),
        ErrorKind::Cancelled => py_errors::CancelledError::new_err((diagnostic, raw_status)),
        ErrorKind::Busy => py_errors::BusyError::new_err((diagnostic, raw_status)),
        ErrorKind::TargetLost => py_errors::TargetLostError::new_err((diagnostic, raw_status)),
        ErrorKind::NotReady => py_errors::NotReadyError::new_err((diagnostic, raw_status)),
        ErrorKind::NotFound => py_errors::NotFoundError::new_err((diagnostic, raw_status)),
        ErrorKind::UnknownStatus => {
            py_errors::UnknownStatusError::new_err((diagnostic, raw_status.unwrap_or_default()))
        }
        ErrorKind::AbiVersionMismatch => {
            py_errors::UnsupportedFeatureError::new_err((diagnostic, raw_status))
        }
        _ => py_errors::NativeError::new_err((diagnostic, raw_status)),
    }
}
