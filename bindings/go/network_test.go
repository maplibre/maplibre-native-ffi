package maplibre

import (
	"errors"
	"testing"
)

func TestNetworkStatusRoundTripsThroughNativeABI(t *testing.T) {
	original, err := NetworkStatusGet()
	if err != nil {
		t.Fatalf("NetworkStatusGet() original: %v", err)
	}
	t.Cleanup(func() {
		if err := NetworkStatusSet(original); err != nil {
			t.Fatalf("restore network status: %v", err)
		}
	})

	if err := NetworkStatusSet(NetworkStatusOffline); err != nil {
		t.Fatalf("NetworkStatusSet(offline): %v", err)
	}
	if got, err := NetworkStatusGet(); err != nil || got != NetworkStatusOffline {
		t.Fatalf("NetworkStatusGet() = %v, %v; want offline, nil", got, err)
	}

	if err := NetworkStatusSet(NetworkStatusOnline); err != nil {
		t.Fatalf("NetworkStatusSet(online): %v", err)
	}
	if got, err := NetworkStatusGet(); err != nil || got != NetworkStatusOnline {
		t.Fatalf("NetworkStatusGet() = %v, %v; want online, nil", got, err)
	}
}

func TestInvalidNetworkStatusReportsNativeError(t *testing.T) {
	err := NetworkStatusSet(NetworkStatus(999_999))
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
