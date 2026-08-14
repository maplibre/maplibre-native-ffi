package maplibre

import "testing"

func TestCallerDriverIsExplicitDarwin(t *testing.T) {
	options := NewRenderSessionAttachOptions()
	options.Driver = RenderDriverCallerGraphicsThread
	if options.Driver != RenderDriverCallerGraphicsThread {
		t.Fatal("caller driver selection lost")
	}
}
