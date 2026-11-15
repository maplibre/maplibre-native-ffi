//go:build cgo && mln_vulkan

package maplibre

import "maplibre.org/maplibre-native-ffi/bindings/go/internal/testsupport"

const (
	buildBackend    = RenderBackendFlagVulkan
	graphicsBackend = testsupport.BackendVulkan
	// The backend of a device that the tests make without the test graphics:
	// the library builds every backend, so a device left to choose may pick
	// another one.
	deviceBackend = RenderBackendVulkan
	// A Vulkan device's sessions take the queue lock around their queue calls.
	queueLockSupported = true
)

// wrapDevice wraps the test graphics' Vulkan device and queue, with lock.
func wrapDevice(context testsupport.Context, lock QueueLock) (*GPUDeviceHandle, error) {
	return WrapVulkanGPUDevice(VulkanDeviceDescriptor{
		Context: VulkanContextDescriptor{
			Instance:                 context.VulkanInstance,
			PhysicalDevice:           context.VulkanPhysicalDevice,
			Device:                   context.VulkanDevice,
			GraphicsQueue:            context.VulkanQueue,
			GraphicsQueueFamilyIndex: context.VulkanQueueFamilyIndex,
			GetInstanceProcAddr:      context.VulkanGetInstanceProcAddr,
			GetDeviceProcAddr:        context.VulkanGetDeviceProcAddr,
		},
		QueueLock:         lock,
		EnabledExtensions: context.VulkanDeviceExtensions,
	})
}

func attachBorrowedTexture(
	m *MapHandle, device *GPUDeviceHandle, size LogicalSize, textures []*testsupport.Texture,
	options RenderSessionAttachOptions,
) (*RenderSessionHandle, *Future[struct{}], error) {
	ring := make([]VulkanBorrowedTexture, len(textures))
	for index, texture := range textures {
		ring[index] = VulkanBorrowedTexture{Image: texture.Info.VulkanImage, ImageView: texture.Info.VulkanImageView}
	}
	info := textures[0].Info
	return m.Attach(RenderTargetVulkanBorrowedTexturesVariant{Value: VulkanBorrowedTexturesTarget{
		LogicalSize:    size,
		PhysicalWidth:  info.Width,
		PhysicalHeight: info.Height,
		Textures:       ring,
		Format:         info.Format,
		FinalLayout:    info.VulkanFinalLayout,
	}}, device, options)
}

func withFrameView(frame *AcquiredFrameHandle, use func(frameView) error) error {
	return frame.WithVulkanTexture(func(view VulkanTextureFrameView) error { return use(view) })
}

func readTexture(view frameView) error {
	_, err := view.(VulkanTextureFrameView).Image()
	return err
}
