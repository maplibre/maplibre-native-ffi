// A frame can't be released while its view lends its textures.
use maplibre_native_ffi::{AcquiredFrameHandle, GpuSync};

fn release_during_view(frame: AcquiredFrameHandle) {
    let view = frame.view().unwrap();
    let _ = unsafe { frame.release(&GpuSync::default()) };
    drop(view);
}

fn main() {
    let _ = release_during_view;
}
