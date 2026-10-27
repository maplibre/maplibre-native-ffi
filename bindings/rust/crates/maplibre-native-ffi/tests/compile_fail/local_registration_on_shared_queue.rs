// A local registration's callbacks may hold state that isn't Send, so
// neither a shared queue nor callback mode takes it.
use std::cell::Cell;
use std::rc::Rc;

use maplibre_native_ffi::{DeliveryQueue, ResourceProvider, RuntimeHandle};

fn not_send(runtime: &RuntimeHandle, queue: &DeliveryQueue) {
    let count = Rc::new(Cell::new(0));
    let provider = ResourceProvider::local(move |_, _| count.set(count.get() + 1));
    runtime
        .queued(queue)
        .set_resource_provider(provider.clone(), |_| {})
        .unwrap();
    runtime.set_resource_provider(provider).unwrap();
}

fn main() {
    let _ = not_send;
}
