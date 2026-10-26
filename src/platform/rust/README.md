# MapLibre Native FFI platform services

`maplibre-native-ffi-platform` holds the image decoding, HTTP, and locale
services that the C library of
[MapLibre Native FFI](https://maplibre.org/maplibre-native-ffi/) calls on Linux,
Windows, Android, and the browser. It is published only because
[`maplibre-native-ffi-sys`](https://crates.io/crates/maplibre-native-ffi-sys)
depends on it, and its API carries no compatibility promise. Depend on
[`maplibre-native-ffi`](https://crates.io/crates/maplibre-native-ffi) instead.
