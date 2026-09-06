package maplibre

import "testing"

func TestRenderExtentPhysicalSizeRejectsInvalidScale(t *testing.T) {
	if _, _, err := (RenderTargetExtent{Width: 1, Height: 1}).PhysicalSize(); err == nil {
		t.Fatal("zero scale factor accepted")
	}
}

func TestFrameBatchCloseIsIdempotent(t *testing.T) {
	var batch *RenderFrameBatch
	batch.Close()
}

// A Vulkan non-dispatchable handle is 64 bits wide on every platform, so the
// binding must carry one that a pointer-width field would truncate. Both
// handles below have a bit set above the low 32 bits, and neither survives a
// 32-bit carrier: the image would lose its top bit and the image view would
// read as VK_NULL_HANDLE.
func TestVulkanDescriptorsKeepHighHandleBits(t *testing.T) {
	const (
		image     = VulkanHandle(0x8000_0000_0000_0001)
		imageView = VulkanHandle(0x0000_0001_0000_0000)
		surface   = VulkanHandle(0xFEDC_BA98_7654_3210)
	)
	context := VulkanContextDescriptor{
		Instance:       NativePointer(0x30),
		PhysicalDevice: NativePointer(0x40),
		Device:         NativePointer(0x50),
		GraphicsQueue:  NativePointer(0x60),
	}
	extent := RenderTargetExtent{Width: 64, Height: 32, ScaleFactor: 2}

	texture := VulkanBorrowedTextureDescriptor{
		Extent:         extent,
		PhysicalWidth:  128,
		PhysicalHeight: 64,
		Context:        context,
		Image:          image,
		ImageView:      imageView,
		Format:         44,
		InitialLayout:  1,
		FinalLayout:    2,
	}.toC()
	if got := VulkanHandle(texture.image); got != image {
		t.Errorf("image = %#x, want %#x", got, image)
	}
	if got := VulkanHandle(texture.image_view); got != imageView {
		t.Errorf("image_view = %#x, want %#x", got, imageView)
	}
	if got := uint32(texture.physical_width); got != 128 {
		t.Errorf("physical_width = %d, want 128", got)
	}

	raw := VulkanSurfaceDescriptor{Extent: extent, Context: context, Surface: surface}.toC()
	if got := VulkanHandle(raw.surface); got != surface {
		t.Errorf("surface = %#x, want %#x", got, surface)
	}
}
