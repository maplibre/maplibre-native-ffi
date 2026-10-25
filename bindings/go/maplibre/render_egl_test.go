//go:build cgo && mln_egl

package maplibre

import "github.com/maplibre/maplibre-native-ffi/bindings/go/internal/testsupport"

const (
	buildBackend    = RenderBackendFlagOpengl
	graphicsBackend = testsupport.BackendEGL
	// Only Vulkan sessions take a queue lock.
	queueLockSupported = false
)

// A shared EGL context takes the caller driver, which runs on the thread the
// fixture made that context current on.
func attachOwnedTexture(
	m *MapHandle, context testsupport.Context, extent LogicalExtent, options RenderSessionAttachOptions,
) (*RenderSessionHandle, *Future[struct{}], error) {
	return m.AttachOpenglOwnedTexture(OpenglOwnedTextureDescriptor{
		Extent: extent,
		Context: OpenglContextDescriptor{
			Ownership: OpenglContextOwnershipShared,
			Data: OpenglContextDescriptorDataEglVariant{Value: EglContextDescriptor{
				Display:      context.EGLDisplay,
				Config:       context.EGLConfig,
				ShareContext: context.EGLContext,
			}},
		},
	}, options)
}

func withFrameView(frame *AcquiredFrameHandle, use func(frameView) error) error {
	return frame.WithOpenglTexture(func(view OpenglTextureFrameView) error { return use(view) })
}

func readTexture(view frameView) error {
	_, err := view.(OpenglTextureFrameView).Texture()
	return err
}
