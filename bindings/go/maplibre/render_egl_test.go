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
func attachOwnedTexture(
	m *MapHandle, context testsupport.Context, extent LogicalExtent, options RenderSessionAttachOptions,
) (*RenderSessionHandle, *Future[struct{}], error) {
	return m.AttachOpenGlOwnedTexture(OpenGlOwnedTextureDescriptor{
		Extent: extent,
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
