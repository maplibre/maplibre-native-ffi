// A header transform's response is valid only until its callback returns.
use maplibre_native_ffi::{HttpHeaderTransformResponse, ResourceKind, Result};

fn keep(_: &'static mut HttpHeaderTransformResponse<'static>) {}

fn transform(
    _: ResourceKind,
    _: &str,
    response: &mut HttpHeaderTransformResponse<'_>,
) -> Result<()> {
    keep(response);
    Ok(())
}

fn main() {
    let _ = maplibre_native_ffi::HttpHeaderTransform::new(transform);
}
