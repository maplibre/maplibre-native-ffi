package maplibre

import (
	"errors"
	"testing"
)

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

func TestNetworkStatusRoundTripsThroughNativeABI(t *testing.T) {
	original, err := CurrentNetworkStatus()
	if err != nil {
		t.Fatalf("CurrentNetworkStatus() original: %v", err)
	}
	t.Cleanup(func() {
		if err := networkStatusSetRaw(uint32(original)); err != nil {
			t.Fatalf("restore network status: %v", err)
		}
	})

	if err := SetNetworkStatus(NetworkStatusOffline); err != nil {
		t.Fatalf("SetNetworkStatus(offline): %v", err)
	}
	if got, err := CurrentNetworkStatus(); err != nil || got != NetworkStatusOffline {
		t.Fatalf("CurrentNetworkStatus() = %v, %v; want offline, nil", got, err)
	}

	if err := SetNetworkStatus(NetworkStatusOnline); err != nil {
		t.Fatalf("SetNetworkStatus(online): %v", err)
	}
	if got, err := CurrentNetworkStatus(); err != nil || got != NetworkStatusOnline {
		t.Fatalf("CurrentNetworkStatus() = %v, %v; want online, nil", got, err)
	}
}

func TestInvalidNetworkStatusReportsNativeError(t *testing.T) {
	err := networkStatusSetRaw(999_999)
	if !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("networkStatusSetRaw invalid error = %v, want ErrInvalidArgument", err)
	}

	var nativeErr *Error
	if !errors.As(err, &nativeErr) {
		t.Fatalf("error %T does not expose *Error", err)
	}
	if status, ok := nativeErr.RawStatus(); !ok || status != -1 {
		t.Fatalf("RawStatus() = %d, %v; want -1, true", status, ok)
	}
	if got := nativeErr.Diagnostic(); got == "" {
		t.Fatal("Diagnostic() is empty")
	}
}

func TestUnknownNetworkStatusRejectedBeforeNativeCall(t *testing.T) {
	err := SetNetworkStatus(NetworkStatus(999_999))
	if !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetNetworkStatus unknown error = %v, want ErrInvalidArgument", err)
	}

	var bindingErr *Error
	if !errors.As(err, &bindingErr) {
		t.Fatalf("error %T does not expose *Error", err)
	}
	if _, ok := bindingErr.RawStatus(); ok {
		t.Fatal("RawStatus() reported native status for binding validation error")
	}
}
