package maplibre

import "testing"

func TestMetalCoreWorkerOptionsDarwin(t *testing.T) {
	options := NewRenderSessionAttachOptions()
	options.RequestedTextureRingDepth = 3
	if options.Driver != RenderDriverCoreWorker || options.RequestedTextureRingDepth != 3 {
		t.Fatalf("options = %+v", options)
	}
}
