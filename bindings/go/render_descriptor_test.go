package maplibre

import "testing"

func TestRenderExtentPhysicalSizeRejectsInvalidScale(t *testing.T) {
	if _, err := RenderTargetExtentPhysicalSize(RenderTargetExtent{Width: 1, Height: 1}); err == nil {
		t.Fatal("zero scale factor accepted")
	}
}

// A Vulkan non-dispatchable handle is 64 bits wide on every platform, so the
// binding must carry one that a pointer-width field would truncate. Both
// handles below have a bit set above the low 32 bits, and neither survives a
// 32-bit carrier: the image would lose its top bit and the image view would
// read as VK_NULL_HANDLE.
func TestVulkanDescriptorsKeepHighHandleBits(t *testing.T) {
	const (
		image     = uint64(0x8000_0000_0000_0001)
		imageView = uint64(0x0000_0001_0000_0000)
		surface   = uint64(0xFEDC_BA98_7654_3210)
	)
	context := VulkanContextDescriptor{
		Instance:       uintptr(0x30),
		PhysicalDevice: uintptr(0x40),
		Device:         uintptr(0x50),
		GraphicsQueue:  uintptr(0x60),
	}
	extent := RenderTargetExtent{Width: 64, Height: 32, ScaleFactor: 2}

	arena := &bindingArena{}
	defer arena.close()
	texture := nativeVulkanBorrowedTextureDescriptor(VulkanBorrowedTextureDescriptor{
		Extent:         extent,
		PhysicalWidth:  128,
		PhysicalHeight: 64,
		Context:        context,
		Image:          image,
		ImageView:      imageView,
		Format:         44,
		InitialLayout:  1,
		FinalLayout:    2,
	}, arena)
	if got := uint64(texture.image); got != image {
		t.Errorf("image = %#x, want %#x", got, image)
	}
	if got := uint64(texture.image_view); got != imageView {
		t.Errorf("image_view = %#x, want %#x", got, imageView)
	}
	if got := uint32(texture.physical_width); got != 128 {
		t.Errorf("physical_width = %d, want 128", got)
	}

	raw := nativeVulkanSurfaceDescriptor(VulkanSurfaceDescriptor{Extent: extent, Context: context, Surface: surface}, arena)
	if got := uint64(raw.surface); got != surface {
		t.Errorf("surface = %#x, want %#x", got, surface)
	}
}

// The GPU synchronization object shares that 64-bit carrier: a Vulkan timeline
// semaphore with a bit set above the low 32 bits must reach the C struct whole.
func TestGPUSyncKeepsHighSemaphoreBits(t *testing.T) {
	const semaphore = uint64(0xFEED_FACE_0000_0007)

	arena := &bindingArena{}
	defer arena.close()
	raw := nativeGpuSync(GpuSync{
		Kind:   GpuSyncKindVulkanTimelineSemaphore,
		Object: uint64(semaphore),
		Value:  9,
	}, arena)

	if got := uint64(raw.object); got != semaphore {
		t.Errorf("object = %#x, want %#x", got, semaphore)
	}
	if got := uint64(raw.value); got != 9 {
		t.Errorf("value = %d, want 9", got)
	}
}
