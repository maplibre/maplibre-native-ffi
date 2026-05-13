pub use maplibre_native_core::json::{JsonMember, JsonValue};

pub(crate) use maplibre_native_core::json::NativeJsonValue;

use crate::Result;

pub(crate) trait JsonValueNativeExt {
    fn try_to_native(&self) -> Result<NativeJsonValue>;
}

impl JsonValueNativeExt for JsonValue {
    fn try_to_native(&self) -> Result<NativeJsonValue> {
        maplibre_native_core::json::json_value_try_to_native(self)
    }
}
