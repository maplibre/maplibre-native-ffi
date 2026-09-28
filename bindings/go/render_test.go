package maplibre

import (
	"errors"
	"math"
	"testing"
)

func TestRenderTargetExtentUsesNativeValidation(t *testing.T) {
	for _, scale := range []float64{0, -1, math.NaN(), math.Inf(1)} {
		if _, err := RenderTargetExtentPhysicalSize(RenderTargetExtent{Width: 64, Height: 32, ScaleFactor: scale}); !errors.Is(err, ErrInvalidArgument) {
			t.Fatalf("invalid scale %v: %v", scale, err)
		}
	}
	extent, err := RenderTargetExtentPhysicalSize(RenderTargetExtent{Width: 64, Height: 32, ScaleFactor: 2})
	if err != nil || extent.Width != 128 || extent.Height != 64 {
		t.Fatalf("physical extent: %#v, %v", extent, err)
	}
}

func TestZeroOwnerReportsBindingError(t *testing.T) {
	for _, owner := range []*RenderSessionHandle{nil, {}} {
		if _, err := owner.GetCapabilities(); !errors.Is(err, ErrInvalidState) {
			t.Fatalf("zero session: %v", err)
		}
	}
	for _, frame := range []*AcquiredFrameHandle{nil, {}} {
		if err := frame.WithMetalTexture(func(MetalOwnedTextureFrameView) error { t.Fatal("invalid frame callback ran"); return nil }); !errors.Is(err, ErrInvalidState) {
			t.Fatalf("zero frame: %v", err)
		}
	}
}
