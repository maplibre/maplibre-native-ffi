package maplibre

import (
	"errors"
	"fmt"
	"strings"
	"testing"
)

func TestCVersionUsesNativeABI(t *testing.T) {
	if got := CVersion(); got != ExpectedCABIVersion {
		t.Fatalf("CVersion() = %d, want %d", got, ExpectedCABIVersion)
	}
}
func TestABIVersionMismatchUsesStableBindingError(t *testing.T) {
	actualVersion := ExpectedCABIVersion + 1
	err := checkCompatibleCABI(actualVersion)
	if !errors.Is(err, ErrABIVersionMismatch) {
		t.Fatalf("checkCompatibleCABI() error = %v, want ErrABIVersionMismatch", err)
	}

	var bindingErr *Error
	if !errors.As(err, &bindingErr) {
		t.Fatalf("checkCompatibleCABI() error = %T, want *Error", err)
	}
	diagnostic := bindingErr.Diagnostic()
	if !strings.Contains(diagnostic, "C ABI version") ||
		!strings.Contains(diagnostic, fmt.Sprintf("expected %d", ExpectedCABIVersion)) ||
		!strings.Contains(diagnostic, fmt.Sprintf("version %d", actualVersion)) {
		t.Fatalf("ABI mismatch diagnostic = %q", diagnostic)
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
