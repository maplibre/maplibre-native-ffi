// A queued readback lends its pixels from the batch only while its handler
// runs; `keep` holds them past it.
use std::sync::Mutex;

use maplibre_native_ffi::{DeliveryQueue, RenderSessionHandle};

fn escape<'a>(
    session: &RenderSessionHandle,
    queue: &DeliveryQueue,
    escaped: &'static Mutex<&'a [u8]>,
) {
    session
        .queued(queue)
        .read_texture(move |readback| *escaped.lock().unwrap() = readback.unwrap().data())
        .unwrap();
}

fn main() {
    let _ = escape;
}
