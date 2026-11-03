// The Go binding lives in bindings/go. The module is rooted here so that it
// compiles against include/, and so that the release tags version it.
// The path has no major version suffix, so an epoch of 2 or more needs the path
// to end in /vN first.
module maplibre.org/maplibre-native-ffi

go 1.26
