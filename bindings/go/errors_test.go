//go:build mlntest

package maplibre

import (
	"errors"
	"strings"
	"testing"
)

// A native failure becomes an *Error that wraps its category sentinel and
// carries the raw status and the call's diagnostic. A status this binding does
// not know keeps its raw value, and a call that writes no diagnostic reports
// none even after an earlier call left one in the shared diagnostic buffer.
func TestNativeStatusMapsToTypedErrors(t *testing.T) {
	err := NetworkStatusSet(NetworkStatus(999_999))
	var native *Error
	if !errors.As(err, &native) || !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("NetworkStatusSet(invalid) = %v, want an *Error wrapping ErrInvalidArgument", err)
	}
	if status, ok := native.RawStatus(); !ok || status != -1 {
		t.Fatalf("RawStatus() = %d, %v; want -1, true", status, ok)
	}
	if native.Diagnostic() == "" || !strings.Contains(native.Error(), native.Diagnostic()) {
		t.Fatalf("diagnostic %q missing from %q", native.Diagnostic(), native.Error())
	}

	err = failWithoutDiagnosticForTest(-12345)
	if !errors.As(err, &native) || !errors.Is(err, ErrUnknownStatus) {
		t.Fatalf("unknown status = %v, want an *Error wrapping ErrUnknownStatus", err)
	}
	if status, _ := native.RawStatus(); status != -12345 {
		t.Fatalf("RawStatus() = %d, want the unknown code", status)
	}
	if native.Diagnostic() != "" {
		t.Fatalf("Diagnostic() = %q, want the earlier call's message cleared", native.Diagnostic())
	}
}

// The package's init runs this check on the linked library and panics on a
// mismatch, so no call reaches a library with another C ABI version.
func TestMismatchedCABIVersionIsRejected(t *testing.T) {
	err := checkCABIVersion(bindingCABIVersion + 1)
	if !errors.Is(err, ErrUnsupported) || !strings.Contains(err.Error(), "C ABI version") {
		t.Fatalf("checkCABIVersion(other) = %v, want ErrUnsupported naming the version", err)
	}
}
