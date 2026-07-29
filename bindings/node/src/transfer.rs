use std::{collections::HashMap, io, sync::Mutex, thread, time::Duration};

const TRANSFER_EXPIRY: Duration = Duration::from_secs(5 * 60);

pub(crate) fn schedule_expiry<T: Send + 'static>(
    registry: &'static Mutex<HashMap<String, T>>,
    token: String,
) -> io::Result<()> {
    thread::Builder::new()
        .name("maplibre-node-transfer-expiry".to_owned())
        .spawn(move || {
            thread::sleep(TRANSFER_EXPIRY);
            if let Ok(mut transfers) = registry.lock() {
                transfers.remove(&token);
            }
        })
        .map(|_| ())
}
