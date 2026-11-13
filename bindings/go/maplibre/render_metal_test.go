//go:build cgo && mln_metal

package maplibre

import "maplibre.org/maplibre-native-ffi/bindings/go/internal/testsupport"

const (
	buildBackend    = RenderBackendFlagMetal
	graphicsBackend = testsupport.BackendMetal
	// Only a Vulkan device takes a queue lock.
	queueLockSupported = false
)

// wrapDevice wraps the test graphics' Metal device. Only a Vulkan device takes
// a queue lock.
func wrapDevice(context testsupport.Context, _ QueueLock) (*GPUDeviceHandle, error) {
	return WrapMetalGPUDevice(MetalDeviceDescriptor{Device: context.MetalDevice})
}

func attachBorrowedTexture(
	m *MapHandle, device *GPUDeviceHandle, size LogicalSize, textures []*testsupport.Texture,
	options RenderSessionAttachOptions,
) (*RenderSessionHandle, *Future[struct{}], error) {
	ring := make([]MetalBorrowedTexture, len(textures))
	for index, texture := range textures {
		ring[index] = MetalBorrowedTexture{Texture: texture.Info.MetalTexture}
	}
	info := textures[0].Info
	return m.Attach(RenderTargetMetalBorrowedTexturesVariant{Value: MetalBorrowedTexturesTarget{
		LogicalSize:    size,
		PhysicalWidth:  info.Width,
		PhysicalHeight: info.Height,
		Textures:       ring,
	}}, device, options)
}

func withFrameView(frame *AcquiredFrameHandle, use func(frameView) error) error {
	return frame.WithMetalTexture(func(view MetalTextureFrameView) error { return use(view) })
}

func readTexture(view frameView) error {
	_, err := view.(MetalTextureFrameView).UnsafeTexture()
	return err
}
