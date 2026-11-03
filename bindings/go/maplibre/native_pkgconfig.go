//go:build mln_pkgconfig

package maplibre

// With the mln_pkgconfig tag, the package compiles against a C library that
// pkg-config finds, such as your own build, and links it without a preset
// module.

// #cgo pkg-config: maplibre-native-c
import "C"
