//go:build cgo && mln_metal

package maplibre

import "github.com/maplibre/maplibre-native-ffi/bindings/go/internal/testsupport"

const (
	buildBackend    = RenderBackendFlagMetal
	graphicsBackend = testsupport.BackendMetal
	// Only Vulkan sessions take a queue lock.
	queueLockSupported = false
)

func attachOwnedTexture(
	m *MapHandle, context testsupport.Context, extent LogicalExtent, options RenderSessionAttachOptions,
) (*RenderSessionHandle, *Future[struct{}], error) {
	return m.AttachMetalOwnedTexture(MetalOwnedTextureDescriptor{
		Extent:  extent,
		Context: MetalContextDescriptor{Device: context.MetalDevice},
		// MTLPixelFormatBGRA8Unorm.
		PixelFormat: 80,
	}, options)
}

func withFrameView(frame *AcquiredFrameHandle, use func(frameView) error) error {
	return frame.WithMetalTexture(func(view MetalTextureFrameView) error { return use(view) })
}
