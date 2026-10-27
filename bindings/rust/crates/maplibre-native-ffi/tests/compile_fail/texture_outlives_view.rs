// A backend texture that a view lends is valid only while the view lives.
use maplibre_native_ffi::{AcquiredFrameHandle, Lent, OpenGlTextureFrame};

fn texture_after_view(frame: &AcquiredFrameHandle) -> Lent<'_, OpenGlTextureFrame> {
    let view = frame.view().unwrap();
    view.get_open_gl_texture().unwrap()
}

fn main() {
    let _ = texture_after_view;
}
