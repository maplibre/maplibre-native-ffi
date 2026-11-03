//go:build cgo && mln_metal

package maplibre

import "maplibre.org/maplibre-native-ffi/go/internal/testsupport"

const (
	buildBackend    = RenderBackendFlagMetal
	graphicsBackend = testsupport.BackendMetal
	// Only Vulkan sessions take a queue lock.
	queueLockSupported = false
)

func attachBorrowedTexture(
	m *MapHandle, _ testsupport.Context, size LogicalSize, textures []*testsupport.Texture,
	options RenderSessionAttachOptions,
) (*RenderSessionHandle, *Future[struct{}], error) {
	ring := make([]MetalBorrowedTexture, len(textures))
	for index, texture := range textures {
		ring[index] = MetalBorrowedTexture{Texture: texture.Info.MetalTexture}
	}
	info := textures[0].Info
	return m.AttachMetalBorrowedTexture(MetalBorrowedTextureDescriptor{
		LogicalSize:    size,
		PhysicalWidth:  info.Width,
		PhysicalHeight: info.Height,
		Textures:       ring,
	}, options)
}

func withFrameView(frame *AcquiredFrameHandle, use func(frameView) error) error {
	return frame.WithMetalTexture(func(view MetalTextureFrameView) error { return use(view) })
}

func readTexture(view frameView) error {
	_, err := view.(MetalTextureFrameView).UnsafeTexture()
	return err
}
