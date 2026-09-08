package maplibre

import (
	"errors"
	stdruntime "runtime"
	"testing"
	"unsafe"
)

func TestStyleImageBorrowsPixelsAcrossGC(t *testing.T) {
	pixels := []byte{1, 2, 3, 4}
	image := newCPremultipliedRGBA8Image(PremultipliedRGBA8Image{Width: 1, Height: 1, Stride: 4, Pixels: pixels})
	defer image.free()
	stdruntime.GC()
	if unsafe.Pointer(image.raw.pixels) != unsafe.Pointer(&pixels[0]) {
		t.Fatal("native image does not borrow the backing pixels")
	}
	if got := unsafe.Slice((*byte)(unsafe.Pointer(image.raw.pixels)), 4); got[0] != 1 || got[3] != 4 {
		t.Fatalf("borrowed pixels after GC = %v", got)
	}
}

func TestNinePatchStyleImageRoundTripsStretchContentAndTextFit(t *testing.T) {
	_, m := newRuntimeAndMap(t, nil)

	if _, err := m.SetStyleJSON([]byte(emptyStyleJSON)); err != nil {
		t.Fatalf("SetStyleJSON(): %v", err)
	}

	image := PremultipliedRGBA8Image{Width: 2, Height: 2, Stride: 8, Pixels: make([]byte, 16)}
	textFit := StyleImageTextFitProportional
	options := StyleImageOptions{
		StretchX:      []ImageStretch{{From: 0, To: 1}},
		StretchY:      []ImageStretch{{From: 0, To: 1}, {From: 1, To: 2}},
		Content:       &ImageContent{Left: 0.5, Top: 0.5, Right: 1.5, Bottom: 1.5},
		TextFitHeight: &textFit,
	}
	if _, err := m.SetStyleImage("patch", image, options); err != nil {
		t.Fatalf("SetStyleImage(): %v", err)
	}

	styleImage, found, err := takeOptionalStyleOperationForTest(m.StyleImage("patch"))
	if err != nil || !found {
		t.Fatalf("StyleImage(patch) = (%+v, %v, %v)", styleImage, found, err)
	}
	info := styleImage.Info
	if info.StretchXCount != 1 || info.StretchYCount != 2 {
		t.Fatalf("stretch counts = (%d, %d), want (1, 2)", info.StretchXCount, info.StretchYCount)
	}
	if info.Content == nil || info.Content.Right != 1.5 {
		t.Fatalf("Content = %+v, want right 1.5", info.Content)
	}
	// An absent text fit stays distinguishable from a present default.
	if info.TextFitWidth != nil {
		t.Fatalf("TextFitWidth = %v, want nil", info.TextFitWidth)
	}
	if info.TextFitHeight == nil || *info.TextFitHeight != StyleImageTextFitProportional {
		t.Fatalf("TextFitHeight = %v, want proportional", info.TextFitHeight)
	}

	stretchX, stretchY := styleImage.StretchX, styleImage.StretchY
	if len(stretchX) != 1 || stretchX[0] != (ImageStretch{From: 0, To: 1}) {
		t.Fatalf("stretchX = %v, want [{0 1}]", stretchX)
	}
	if len(stretchY) != 2 || stretchY[1] != (ImageStretch{From: 1, To: 2}) {
		t.Fatalf("stretchY = %v, want [{0 1} {1 2}]", stretchY)
	}

	// The narrow copy reports the same intervals the aggregate carries.
	stretches, found, err := takeOptionalStyleOperationForTest(m.StyleImageStretches("patch"))
	if err != nil || !found || !equalStretches(stretches.X, stretchX) || !equalStretches(stretches.Y, stretchY) {
		t.Fatalf("StyleImageStretches(patch) = (%+v, %v, %v), want %v and %v", stretches, found, err, stretchX, stretchY)
	}
	if _, found, err := takeOptionalStyleOperationForTest(m.StyleImageStretches("missing")); err != nil || found {
		t.Fatalf("StyleImageStretches(missing) = (%v, %v), want (false, nil)", found, err)
	}

	if _, found, err = takeOptionalStyleOperationForTest(m.StyleImage("missing")); err != nil || found {
		t.Fatalf("StyleImage(missing) = (%v, %v), want (false, nil)", found, err)
	}

	// A backwards interval is rejected by C.
	bad := StyleImageOptions{StretchX: []ImageStretch{{From: 2, To: 1}}}
	if _, err := m.SetStyleImage("bad", image, bad); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetStyleImage(backwards) error = %v; want ErrInvalidArgument", err)
	}
}

func TestStyleImageOptionsCloneKeepsStretchPresence(t *testing.T) {
	options := StyleImageOptions{StretchX: []ImageStretch{}, StretchY: nil}
	cloned := options.Clone()

	// A present empty slice and an absent one stay distinguishable across a clone.
	if cloned.StretchX == nil {
		t.Fatalf("cloned StretchX = nil, want a present empty slice")
	}
	if cloned.StretchY != nil {
		t.Fatalf("cloned StretchY = %v, want nil", cloned.StretchY)
	}
	if !options.Equal(cloned) {
		t.Fatalf("clone %+v is not equal to its source %+v", cloned, options)
	}

	source := StyleImageOptions{StretchX: []ImageStretch{{From: 0, To: 1}}}
	independent := source.Clone()
	source.StretchX[0] = ImageStretch{From: 5, To: 6}
	if independent.StretchX[0] != (ImageStretch{From: 0, To: 1}) {
		t.Fatalf("clone StretchX = %v, want the values at clone time", independent.StretchX)
	}
}

func TestStyleImageCopiesPixelsAndMetadata(t *testing.T) {
	_, m := newRuntimeAndMap(t, nil)

	if _, err := m.SetStyleJSON([]byte(emptyStyleJSON)); err != nil {
		t.Fatalf("SetStyleJSON(empty style): %v", err)
	}
	pixels := []byte{255, 0, 0, 255}
	pixelRatio := float32(2)
	sdf := true
	if _, err := m.SetStyleImage("marker", PremultipliedRGBA8Image{Width: 1, Height: 1, Stride: 4, Pixels: pixels}, StyleImageOptions{PixelRatio: &pixelRatio, SDF: &sdf}); err != nil {
		t.Fatalf("SetStyleImage(): %v", err)
	}
	pixels[0] = 0
	styleImage, found, err := takeOptionalStyleOperationForTest(m.StyleImage("marker"))
	if err != nil {
		t.Fatalf("StyleImageInfo(marker): %v", err)
	}
	info := styleImage.Info
	if !found || info.Width != 1 || info.Height != 1 || info.Stride != 4 || info.ByteLength != 4 || info.PixelRatio != pixelRatio || info.SDF != sdf {
		t.Fatalf("StyleImage(marker) = (%+v, %v), want copied 1x1 image metadata", styleImage, found)
	}
	copied := styleImage.Pixels
	if !found || len(copied) != 4 || copied[0] != 255 || copied[1] != 0 || copied[2] != 0 || copied[3] != 255 {
		t.Fatalf("StyleImage(marker) pixels = (%v, %v), want original copied pixels", copied, found)
	}
	// The narrow copy reports the same pixels the aggregate carries.
	copiedPixels, found, err := takeOptionalStyleOperationForTest(m.StyleImagePremultipliedRGBA8("marker"))
	if err != nil || !found || len(copiedPixels) != 4 || copiedPixels[0] != 255 || copiedPixels[3] != 255 {
		t.Fatalf("StyleImagePremultipliedRGBA8(marker) = (%v, %v, %v), want original copied pixels", copiedPixels, found, err)
	}
	completion, err := m.RemoveStyleImage("marker")
	requireCommandCommitted(t, completion, err)
	if _, found, err := takeOptionalStyleOperationForTest(m.StyleImage("marker")); err != nil || found {
		t.Fatalf("StyleImageInfo(marker) after removal = (%v, %v), want (false, nil)", found, err)
	}
	if _, found, err := takeOptionalStyleOperationForTest(m.StyleImagePremultipliedRGBA8("marker")); err != nil || found {
		t.Fatalf("StyleImagePremultipliedRGBA8(marker) after removal = (%v, %v), want (false, nil)", found, err)
	}
	completion, err = m.RemoveStyleImage("marker")
	requireCommandFailedWith(t, completion, err, ErrNotFound)
	if _, err := m.SetStyleImage("bad-marker", PremultipliedRGBA8Image{Width: 1, Height: 1, Stride: 4}, StyleImageOptions{}); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetStyleImage(empty pixels) error = %v, want ErrInvalidArgument", err)
	}
}

func TestImageSourceCopiesPixelsAndCoordinates(t *testing.T) {
	_, m := newRuntimeAndMap(t, nil)

	if _, err := m.SetStyleJSON([]byte(emptyStyleJSON)); err != nil {
		t.Fatalf("SetStyleJSON(empty style): %v", err)
	}
	coordinates := []LatLng{
		{Latitude: 1, Longitude: 1},
		{Latitude: 1, Longitude: 2},
		{Latitude: 0, Longitude: 2},
		{Latitude: 0, Longitude: 1},
	}
	pixels := []byte{0, 255, 0, 255}
	if _, err := m.AddImageSourceImage("image-source", coordinates, PremultipliedRGBA8Image{Width: 1, Height: 1, Stride: 4, Pixels: pixels}); err != nil {
		t.Fatalf("AddImageSourceImage(): %v", err)
	}
	coordinates[0] = LatLng{Latitude: 9, Longitude: 9}
	pixels[1] = 0
	gotCoordinates, found, err := takeOptionalStyleOperationForTest(m.ImageSourceCoordinates("image-source"))
	if err != nil {
		t.Fatalf("ImageSourceCoordinates(image-source): %v", err)
	}
	if !found || len(gotCoordinates) != 4 || gotCoordinates[0] != (LatLng{Latitude: 1, Longitude: 1}) {
		t.Fatalf("ImageSourceCoordinates(image-source) = (%v, %v), want original copied coordinates", gotCoordinates, found)
	}
	updatedCoordinates := []LatLng{
		{Latitude: 2, Longitude: 2},
		{Latitude: 2, Longitude: 3},
		{Latitude: 1, Longitude: 3},
		{Latitude: 1, Longitude: 2},
	}
	if _, err := m.SetImageSourceCoordinates("image-source", updatedCoordinates); err != nil {
		t.Fatalf("SetImageSourceCoordinates(): %v", err)
	}
	if _, err := m.SetImageSourceImage("image-source", PremultipliedRGBA8Image{Width: 1, Height: 1, Stride: 4, Pixels: []byte{0, 0, 255, 255}}); err != nil {
		t.Fatalf("SetImageSourceImage(): %v", err)
	}
	gotCoordinates, found, err = takeOptionalStyleOperationForTest(m.ImageSourceCoordinates("image-source"))
	if err != nil {
		t.Fatalf("ImageSourceCoordinates(image-source after update): %v", err)
	}
	if !found || gotCoordinates[0] != updatedCoordinates[0] {
		t.Fatalf("ImageSourceCoordinates(image-source after update) = (%v, %v), want updated coordinates", gotCoordinates, found)
	}
	if _, err := m.AddImageSourceURL("image-url-source", updatedCoordinates, "asset://fixtures/image.png"); err != nil {
		t.Fatalf("AddImageSourceURL(): %v", err)
	}
	if _, err := m.SetImageSourceURL("image-url-source", "asset://fixtures/image-2.png"); err != nil {
		t.Fatalf("SetImageSourceURL(): %v", err)
	}
	if _, err := m.AddImageSourceImage("bad-image-source", updatedCoordinates[:3], PremultipliedRGBA8Image{Width: 1, Height: 1, Stride: 4, Pixels: []byte{0, 0, 0, 0}}); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("AddImageSourceImage(3 coordinates) error = %v, want ErrInvalidArgument", err)
	}
}
