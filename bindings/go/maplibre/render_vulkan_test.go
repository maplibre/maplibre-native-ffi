//go:build cgo && mln_vulkan

package maplibre

import "github.com/maplibre/maplibre-native-ffi/bindings/go/internal/testsupport"

const (
	buildBackend    = RenderBackendFlagVulkan
	graphicsBackend = testsupport.BackendVulkan
	// A Vulkan session takes the queue lock around its queue calls.
	queueLockSupported = true
)

func attachBorrowedTexture(
	m *MapHandle, context testsupport.Context, extent LogicalExtent, textures []*testsupport.Texture,
	options RenderSessionAttachOptions,
) (*RenderSessionHandle, *Future[struct{}], error) {
	ring := make([]VulkanBorrowedTexture, len(textures))
	for index, texture := range textures {
		ring[index] = VulkanBorrowedTexture{Image: texture.Info.VulkanImage, ImageView: texture.Info.VulkanImageView}
	}
	info := textures[0].Info
	return m.AttachVulkanBorrowedTexture(VulkanBorrowedTextureDescriptor{
		Extent:         extent,
		PhysicalWidth:  info.Width,
		PhysicalHeight: info.Height,
		Textures:       ring,
		Format:         info.Format,
		FinalLayout:    info.VulkanFinalLayout,
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
}

func withFrameView(frame *AcquiredFrameHandle, use func(frameView) error) error {
	return frame.WithVulkanTexture(func(view VulkanTextureFrameView) error { return use(view) })
}

func readTexture(view frameView) error {
	_, err := view.(VulkanTextureFrameView).Image()
	return err
}
