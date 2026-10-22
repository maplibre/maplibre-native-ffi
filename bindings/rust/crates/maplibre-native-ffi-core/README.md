# MapLibre Native FFI binding runtime

`maplibre-native-ffi-core` holds the error, ownership, callback, and memory
runtime that the Rust and Python bindings of
[MapLibre Native FFI](https://maplibre.org/maplibre-native-ffi/) share. It is
published only because
[`maplibre-native-ffi`](https://crates.io/crates/maplibre-native-ffi) depends on
it, and its API carries no compatibility promise. Depend on
`maplibre-native-ffi` instead.
