//go:build !cgo || !(mln_metal || mln_vulkan || mln_egl)

package maplibre

// An untagged build compiles no render tests, and names no backend that the
// linked library must report.
const buildBackend RenderBackendFlag = 0
