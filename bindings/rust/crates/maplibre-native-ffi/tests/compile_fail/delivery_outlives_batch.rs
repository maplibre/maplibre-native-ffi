// A delivery borrows the batch it arrived in, which owns its payload.
use maplibre_native_ffi::{Delivery, DeliveryQueue, Shared};

fn delivery_after_batch(queue: &DeliveryQueue) -> Delivery<'_, Shared> {
    let mut batch = queue.drain().unwrap().unwrap();
    batch.deliveries().next().unwrap()
}

fn main() {
    let _ = delivery_after_batch;
}
