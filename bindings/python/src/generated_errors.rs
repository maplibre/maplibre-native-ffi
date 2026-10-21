// Generated from C headers by tools/bindgen. Do not edit.
mod py_errors {
    pyo3::import_exception!(maplibre_native_ffi.errors, MaplibreError);
    pyo3::import_exception!(maplibre_native_ffi.errors, AbiVersionMismatchError);
}
fn map_error(error: Error) -> PyErr {
    match error {
        Error::Status { status, diagnostic } => {
            py_errors::MaplibreError::new_err((status.as_raw(), diagnostic))
        }
        Error::AbiVersionMismatch { expected, actual } => {
            py_errors::AbiVersionMismatchError::new_err((expected, actual))
        }
    }
}
