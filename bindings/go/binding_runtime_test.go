//go:build mlntest

package maplibre

import (
	"errors"
	"math"
	"testing"
	"unsafe"
)

// A byte input is borrowed, not copied: native reads the Go slice's own
// memory. The arena pins that memory, so cgo's pointer check lets a native
// call take the image that points into it.
func TestByteInputsArePinnedAndBorrowed(t *testing.T) {
	pixels := []byte{1, 2, 3, 4}
	arena := &bindingArena{}
	defer arena.close()
	image := nativePremultipliedRgba8Image(PremultipliedRgba8Image{Width: 1, Height: 1, Stride: 4, Pixels: pixels}, arena)
	if unsafe.Pointer(image.pixels) != unsafe.Pointer(&pixels[0]) {
		t.Fatal("the native image does not borrow the slice")
	}
	if err := passImageToCForTest(&image); err != nil {
		t.Fatalf("passing the image to C: %v", err)
	}
}

// expectBindingError runs call as a binding call and fails unless it fails
// with want.
func expectBindingError(t *testing.T, want error, what string, call func()) {
	t.Helper()
	if _, err := bindingCall(func() struct{} { call(); return struct{}{} }); !errors.Is(err, want) {
		t.Fatalf("%s: %v, want %v", what, err, want)
	}
}

// A count narrows to its C type only when the value fits. Vulkan's
// non-dispatchable handles and GPU synchronization objects are 64 bits wide on
// every platform, so each handle below has a bit set above the low 32 that a
// pointer-width carrier would drop on a 32-bit target.
func TestIntegerCarriersKeepTheirValues(t *testing.T) {
	expectBindingError(t, ErrInvalidArgument, "a count above uint16", func() { bindingCount[uint16](math.MaxUint16 + 1) })
	expectBindingError(t, ErrInvalidArgument, "a negative count", func() { bindingCount[uint64](-1) })
	if got := bindingCount[uint16](math.MaxUint16); got != math.MaxUint16 {
		t.Fatalf("bindingCount[uint16](max) = %d", got)
	}

	const (
		image     = uint64(0x8000_0000_0000_0001)
		imageView = uint64(0x0000_0001_0000_0000)
		surface   = uint64(0xFEDC_BA98_7654_3210)
		semaphore = uint64(0xFEED_FACE_0000_0007)
	)
	arena := &bindingArena{}
	defer arena.close()
	extent := RenderTargetExtent{Width: 64, Height: 32, ScaleFactor: 2}
	context := VulkanContextDescriptor{Instance: 0x30, PhysicalDevice: 0x40, Device: 0x50, GraphicsQueue: 0x60}
	descriptor := nativeVulkanBorrowedTextureDescriptor(VulkanBorrowedTextureDescriptor{
		Extent: extent, PhysicalWidth: 128, PhysicalHeight: 64, Context: context,
		Textures: []VulkanBorrowedTexture{{Image: image, ImageView: imageView}},
	}, arena)
	if descriptor.texture_count != 1 {
		t.Fatalf("texture count = %d, want 1", descriptor.texture_count)
	}
	texture := unsafe.Slice(descriptor.textures, 1)[0]
	if uint64(texture.image) != image || uint64(texture.image_view) != imageView {
		t.Fatalf("texture handles = %#x, %#x; want %#x, %#x", uint64(texture.image), uint64(texture.image_view), image, imageView)
	}
	if raw := nativeVulkanSurfaceDescriptor(VulkanSurfaceDescriptor{Extent: extent, Context: context, Surface: surface}, arena); uint64(raw.surface) != surface {
		t.Fatalf("surface = %#x, want %#x", uint64(raw.surface), surface)
	}
	sync := nativeGpuSync(GpuSync{Kind: GpuSyncKindVulkanTimelineSemaphore, Object: semaphore, Value: math.MaxUint64}, arena)
	if uint64(sync.object) != semaphore || uint64(sync.value) != math.MaxUint64 {
		t.Fatalf("sync = %#x, %d; want %#x, %d", uint64(sync.object), uint64(sync.value), semaphore, uint64(math.MaxUint64))
	}
}

// The layout of a native array or message range is checked before the binding
// reads it.
func TestNativeLayoutsAreChecked(t *testing.T) {
	expect := func(want error, what string, call func()) {
		t.Helper()
		expectBindingError(t, want, what, call)
	}
	var storage [4]uint64
	base := unsafe.Pointer(&storage[0])
	size, align := unsafe.Sizeof(storage[0]), unsafe.Alignof(storage[0])
	expect(ErrNative, "a null array", func() { bindingElement(nil, 0, uint64(size), size, align) })
	expect(ErrNative, "a stride below the element size", func() { bindingElement(base, 1, uint64(size-align), size, align) })
	expect(ErrNative, "a misaligned stride", func() { bindingElement(base, 1, uint64(size+1), size, align) })
	if got := bindingElement(base, 2, uint64(size), size, align); got != unsafe.Pointer(&storage[2]) {
		t.Fatalf("bindingElement(2) = %p, want %p", got, &storage[2])
	}

	arena := []byte("abcdef")
	pointer := unsafe.Pointer(&arena[0])
	expect(ErrNative, "an offset past the arena", func() { bindingArenaString(pointer, 6, 7, 0) })
	expect(ErrNative, "a range past the arena", func() { bindingArenaString(pointer, 6, 4, 3) })
	expect(ErrNative, "a message in a null arena", func() { bindingArenaString(nil, 6, 0, 1) })
	expect(ErrNative, "bytes from a null buffer", func() { bindingBytes(nil, 1) })
	if got := bindingArenaString(pointer, 6, 2, 3); got != "cde" {
		t.Fatalf("bindingArenaString(2, 3) = %q, want cde", got)
	}
}

// The binding's own exports run on native threads, so a panic inside one is
// recovered rather than aborting the process.
func TestExportsRecoverFromPanics(t *testing.T) {
	stale, free := staleCallbackCellForTest()
	defer free()
	if owner := mlnGoCallbackOwner(stale); owner != 0 {
		t.Fatalf("mlnGoCallbackOwner(stale) = %d, want 0", owner)
	}
	mlnGoCallbackRelease(stale)
}
