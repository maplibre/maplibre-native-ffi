// `keep` holds the batch that lent the readback's pixels, so a view can't
// swap in other bytes for it to keep.
use maplibre_native_ffi::{BatchBytes, TextureReadbackView};

fn escape(view: TextureReadbackView<'_>) -> BatchBytes {
    let replacement = vec![42u8; 16];
    let mut shortened = view;
    shortened.data = &replacement;
    shortened.keep()
}

fn main() {
    let _ = escape;
}
