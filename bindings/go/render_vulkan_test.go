//go:build cgo && mln_vulkan

package maplibre

import "github.com/maplibre/maplibre-native-ffi/bindings/go/internal/testsupport"

const (
	buildBackend    = RenderBackendFlagVulkan
	graphicsBackend = testsupport.BackendVulkan
)

func attachOwnedTexture(
	m *MapHandle, context testsupport.Context, extent RenderTargetExtent, options RenderSessionAttachOptions,
) (*RenderSessionHandle, *Future[struct{}], error) {
	attachment, err := m.VulkanOwnedTextureAttach(VulkanOwnedTextureDescriptor{
		Extent: extent,
		Context: VulkanContextDescriptor{
			Instance:                 context.VulkanInstance,
			PhysicalDevice:           context.VulkanPhysicalDevice,
			Device:                   context.VulkanDevice,
			GraphicsQueue:            context.VulkanQueue,
			GraphicsQueueFamilyIndex: context.VulkanQueueFamilyIndex,
			GetInstanceProcAddr:      context.VulkanGetInstanceProcAddr,
			GetDeviceProcAddr:        context.VulkanGetDeviceProcAddr,
		},
	}, options)
	return attachment.Session, attachment.Completion, err
}

func withFrameView(frame *AcquiredFrameHandle, use func(frameView) error) error {
	return frame.WithVulkanTexture(func(view VulkanOwnedTextureFrameView) error { return use(view) })
}
