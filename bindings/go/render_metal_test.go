//go:build cgo && mln_metal

package maplibre

import "github.com/maplibre/maplibre-native-ffi/bindings/go/internal/testsupport"

const (
	buildBackend    = RenderBackendFlagMetal
	graphicsBackend = testsupport.BackendMetal
)

func attachOwnedTexture(
	m *MapHandle, context testsupport.Context, extent LogicalExtent, options RenderSessionAttachOptions,
) (*RenderSessionHandle, *Future[struct{}], error) {
	attachment, err := m.AttachMetalOwnedTexture(MetalOwnedTextureDescriptor{
		Extent:  extent,
		Context: MetalContextDescriptor{Device: context.MetalDevice},
	}, options)
	return attachment.Session, attachment.Completion, err
}

func withFrameView(frame *AcquiredFrameHandle, use func(frameView) error) error {
	return frame.WithMetalTexture(func(view MetalOwnedTextureFrameView) error { return use(view) })
}
