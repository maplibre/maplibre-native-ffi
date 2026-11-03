// A custom source's callback borrows its map, so it can't release it.
use maplibre_native_ffi::CustomGeometrySourceOptions;

fn main() {
    let _ = CustomGeometrySourceOptions::new(|map, _source_id, _tile| {
        let _ = map.release();
    });
}
