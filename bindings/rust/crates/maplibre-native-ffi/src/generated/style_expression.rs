// Generated from C headers by tools/bindgen. Do not edit.
use super::*;

native_owner! {
    /// Owns one `mln_style_expression` native handle.
    ///
    /// A parsed style expression, immutable and independent of any map.
    ///
    /// See `mln_style_expression` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
    pub struct StyleExpressionHandle(mln_style_expression) dispose |raw| { unsafe { sys::mln_style_expression_release(raw) }; Ok(()) };
    /// Returns the native handle value.
    fn id;
}

impl StyleExpressionHandle {
    /// Evaluates a parsed expression and writes its result as UTF-8 JSON.
    ///
    /// See `mln_style_expression_evaluate` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
    pub fn evaluate(&self, inputs: Option<&StyleExpressionInputs>) -> Result<String> {
        let mut call = self.inner.call();
        let mut out_json = Vec::new();
        let inputs = call.optional_reference(inputs.as_ref())?;
        call.status(|expression, out_diagnostic| unsafe {
            convert::caller_sized_text(&mut out_json, |out_json, json_capacity, out_json_length| {
                sys::mln_style_expression_evaluate(
                    expression,
                    inputs,
                    out_json,
                    json_capacity,
                    out_json_length,
                    out_diagnostic,
                )
            })
        })?;
        Ok(convert::utf8(out_json)?)
    }

    /// Copies what a parsed expression depends on into out_info.
    ///
    /// See `mln_style_expression_get_info` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
    pub fn get_info(&self) -> Result<StyleExpressionInfo> {
        let mut call = self.inner.call();
        let mut out_info: sys::mln_style_expression_info = unsafe { std::mem::zeroed() };
        out_info.size = std::mem::size_of::<sys::mln_style_expression_info>() as _;
        call.status(|expression, out_diagnostic| unsafe {
            sys::mln_style_expression_get_info(expression, &mut out_info, out_diagnostic)
        })?;
        Ok(unsafe { from_native(out_info) }?)
    }

    /// Releases a style expression.
    ///
    /// See `mln_style_expression_release` in the
    /// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
    pub fn release(self) -> std::result::Result<(), (Self, Error)> {
        self.inner
            .close(|expression| {
                let mut call = Call::new(expression);
                call.run(|expression| unsafe { sys::mln_style_expression_release(expression) });
                Ok(())
            })
            .map_err(|error| (self, error))
    }
}
