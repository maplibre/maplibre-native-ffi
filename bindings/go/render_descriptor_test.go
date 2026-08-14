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
