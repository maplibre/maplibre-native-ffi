pub use maplibre_native_core::json::{JsonMember, JsonValue};

pub(crate) use maplibre_native_core::json::NativeJsonValue;

use crate::Result;
use crate::sys;

pub(crate) trait JsonValueNativeExt {
    fn try_to_native(&self) -> Result<NativeJsonValue>;

    /// Copies a borrowed native JSON value into an owned Rust tree.
    ///
    /// # Safety
    ///
    /// `raw` and all nested pointers must be valid for the duration of this call.
    unsafe fn from_native(raw: &sys::mln_json_value) -> Result<JsonValue>;
}

impl JsonValueNativeExt for JsonValue {
    fn try_to_native(&self) -> Result<NativeJsonValue> {
        maplibre_native_core::json::json_value_try_to_native(self)
    }

    unsafe fn from_native(raw: &sys::mln_json_value) -> Result<JsonValue> {
        // SAFETY: The caller promises raw and nested pointers are valid for this call.
        unsafe { maplibre_native_core::json::json_value_from_native(raw) }
    }
}
