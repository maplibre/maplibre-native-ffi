// A shared queue may run its handlers on any thread, so each must be Send.
use maplibre_native_ffi::{DeliveryQueue, MapHandle};

fn not_send(map: &MapHandle, queue: &DeliveryQueue) {
    let local = std::rc::Rc::new(());
    map.queued(queue)
        .get_style_url(move |_| drop(local))
        .unwrap();
}

fn main() {
    let _ = not_send;
}
