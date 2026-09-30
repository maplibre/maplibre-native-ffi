//go:build cgo && mln_egl

package maplibre

import "github.com/maplibre/maplibre-native-ffi/bindings/go/internal/testsupport"

const (
	buildBackend    = RenderBackendFlagOpengl
	graphicsBackend = testsupport.BackendEGL
)

// A shared EGL context takes the caller driver, which runs on the thread the
// fixture made that context current on.
func attachOwnedTexture(
	m *MapHandle, context testsupport.Context, extent RenderTargetExtent, options RenderSessionAttachOptions,
) (*RenderSessionHandle, *Future[struct{}], error) {
	attachment, err := m.OpenglOwnedTextureAttach(OpenglOwnedTextureDescriptor{
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
	return attachment.Session, attachment.Completion, err
}

func withFrameView(frame *AcquiredFrameHandle, use func(frameView) error) error {
	return frame.WithOpenglTexture(func(view OpenglOwnedTextureFrameView) error { return use(view) })
}
