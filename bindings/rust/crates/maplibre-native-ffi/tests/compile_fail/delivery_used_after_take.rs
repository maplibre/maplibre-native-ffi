// Taking a delivery moves it out of its batch, so it can't be taken again.
use maplibre_native_ffi::{DeliveryBatch, TakenDelivery};

fn take_twice(batch: &mut DeliveryBatch) -> (TakenDelivery, TakenDelivery) {
    let delivery = batch.deliveries().next().unwrap();
    let first = delivery.take();
    (first, delivery.take())
}

fn main() {
    let _ = take_twice;
}
