package maplibre

import "testing"

func TestCVersionUsesNativeABI(t *testing.T) {
	if got := CVersion(); got != 0 {
		t.Fatalf("CVersion() = %d, want 0 while ABI is unstable", got)
	}
}
func TestSupportedRenderBackendsUsesNativeABIConstants(t *testing.T) {
	mask := SupportedRenderBackends()
	if mask == 0 {
		t.Fatal("SupportedRenderBackends() returned empty mask")
	}
	if mask.Has(RenderBackendMetal) && uint32(RenderBackendMetal) == 0 {
		t.Fatal("RenderBackendMetal has zero ABI value")
	}
	if mask.Has(RenderBackendVulkan) && uint32(RenderBackendVulkan) == 0 {
		t.Fatal("RenderBackendVulkan has zero ABI value")
	}
}
func TestNativePointerIsOpaqueValue(t *testing.T) {
	var pointer NativePointer = 0x1234
	if uintptr(pointer) != 0x1234 {
		t.Fatalf("NativePointer preserved address value %x", uintptr(pointer))
	}
}
