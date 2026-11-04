//go:build cgo && mln_wgl

package maplibre

import "github.com/maplibre/maplibre-native-ffi/bindings/go/internal/testsupport"

const (
	buildBackend    = RenderBackendFlagOpenGL
	graphicsBackend = testsupport.BackendWGL
	// Only Vulkan sessions take a queue lock.
	queueLockSupported = false
)

// A shared WGL context takes the caller driver, which runs on the thread the
// fixture made that context current on.
func attachBorrowedTexture(
	m *MapHandle, context testsupport.Context, size LogicalSize, textures []*testsupport.Texture,
	options RenderSessionAttachOptions,
) (*RenderSessionHandle, *Future[struct{}], error) {
	ring := make([]OpenGLBorrowedTexture, len(textures))
	for index, texture := range textures {
		ring[index] = OpenGLBorrowedTexture{Texture: texture.Info.OpenGLTexture}
	}
	info := textures[0].Info
	return m.AttachOpenGLBorrowedTexture(OpenGLBorrowedTextureDescriptor{
		LogicalSize:    size,
		PhysicalWidth:  info.Width,
		PhysicalHeight: info.Height,
		Textures:       ring,
		Target:         info.OpenGLTarget,
		Context: OpenGLContextDescriptor{
			Ownership: OpenGLContextOwnershipShared,
			Data: OpenGLContextDescriptorDataWGLVariant{Value: WGLContextDescriptor{
				DeviceContext: context.WGLDeviceContext,
				ShareContext:  context.WGLContext,
			}},
		},
	}, options)
}

func withFrameView(frame *AcquiredFrameHandle, use func(frameView) error) error {
	return frame.WithOpenGLTexture(func(view OpenGLTextureFrameView) error { return use(view) })
}

func readTexture(view frameView) error {
	_, err := view.(OpenGLTextureFrameView).Texture()
	return err
}
