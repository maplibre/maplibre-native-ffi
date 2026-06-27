use maplibre_native::{
    Error, MapHandle, RenderSessionHandle, WebGPUContextDescriptor, WebGPUOwnedTextureDescriptor,
};

use crate::viewport::Viewport;

pub fn attach(
    map: &MapHandle,
    viewport: &Viewport,
    context: &WebGPUContextDescriptor,
) -> Result<RenderSessionHandle, Error> {
    map.attach_webgpu_owned_texture(&WebGPUOwnedTextureDescriptor::new(
        viewport.extent(),
        context.clone(),
    ))
}
