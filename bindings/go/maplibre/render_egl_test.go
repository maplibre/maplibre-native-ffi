//go:build cgo && mln_egl

package maplibre

import "github.com/maplibre/maplibre-native-ffi/bindings/go/internal/testsupport"

const (
	buildBackend    = RenderBackendFlagOpenGl
	graphicsBackend = testsupport.BackendEgl
	// Only Vulkan sessions take a queue lock.
	queueLockSupported = false
)

// A shared EGL context takes the caller driver, which runs on the thread the
// fixture made that context current on.
func attachBorrowedTexture(
	m *MapHandle, context testsupport.Context, extent LogicalExtent, textures []*testsupport.Texture,
	options RenderSessionAttachOptions,
) (*RenderSessionHandle, *Future[struct{}], error) {
	ring := make([]OpenGlBorrowedTexture, len(textures))
	for index, texture := range textures {
		ring[index] = OpenGlBorrowedTexture{Texture: texture.Info.OpenGlTexture}
	}
	info := textures[0].Info
	return m.AttachOpenGlBorrowedTexture(OpenGlBorrowedTextureDescriptor{
		Extent:         extent,
		PhysicalWidth:  info.Width,
		PhysicalHeight: info.Height,
		Textures:       ring,
		Target:         info.OpenGlTarget,
		Context: OpenGlContextDescriptor{
			Ownership: OpenGlContextOwnershipShared,
			Data: OpenGlContextDescriptorDataEglVariant{Value: EglContextDescriptor{
				Display:      context.EglDisplay,
				Config:       context.EglConfig,
				ShareContext: context.EglContext,
			}},
		},
	}, options)
}

func withFrameView(frame *AcquiredFrameHandle, use func(frameView) error) error {
	return frame.WithOpenGlTexture(func(view OpenGlTextureFrameView) error { return use(view) })
}

func readTexture(view frameView) error {
	_, err := view.(OpenGlTextureFrameView).Texture()
	return err
}
