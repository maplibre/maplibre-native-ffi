//go:build mlntest

package maplibre

import (
	"errors"
	"strings"
	"testing"
)

// A native failure becomes an *Error that wraps its category sentinel and
// carries the raw status and the call's diagnostic. A diagnostic that fills
// its buffer with no terminating NUL is read to the buffer's end and no
// further. A status this binding does not know keeps its raw value, and a call
// that writes no diagnostic reports none even after an earlier call filled the
// shared diagnostic buffer.
func TestNativeStatusMapsToTypedErrors(t *testing.T) {
	err := NetworkSetStatus(NetworkStatus(999_999))
	var native *Error
	if !errors.As(err, &native) || !errors.Is(err, StatusInvalidArgument) {
		t.Fatalf("NetworkSetStatus(invalid) = %v, want an *Error wrapping StatusInvalidArgument", err)
	}
	if native.Status != StatusInvalidArgument {
		t.Fatalf("status = %v", native.Status)
	}
	if want := "maplibre: invalid argument: " + native.Diagnostic; native.Diagnostic == "" || native.Error() != want {
		t.Fatalf("Error() = %q, want %q", native.Error(), want)
	}

	capacity, err := failWithFullDiagnosticForTest(-1)
	if !errors.As(err, &native) {
		t.Fatalf("a full diagnostic buffer = %v, want an *Error", err)
	}
	if diagnostic := native.Diagnostic; diagnostic != strings.Repeat("x", capacity) {
		t.Fatalf("a full diagnostic buffer read as %d bytes, want its %d", len(diagnostic), capacity)
	}

	err = failWithoutDiagnosticForTest(-12345)
	if !errors.As(err, &native) || !errors.Is(err, Status(-12345)) {
		t.Fatalf("unknown status = %v, want an *Error wrapping Status(-12345)", err)
	}
	if native.Status != Status(-12345) {
		t.Fatalf("status = %v", native.Status)
	}
	if native.Diagnostic != "" {
		t.Fatalf("Diagnostic = %q, want the earlier call's message cleared", native.Diagnostic)
	}
	if want := "maplibre: status -12345"; native.Error() != want {
		t.Fatalf("Error() = %q, want %q", native.Error(), want)
	}
}

// The package's init runs this check on the linked library and panics on a
// mismatch, so no call reaches a library with another C ABI version.
func TestMismatchedCABIVersionIsRejected(t *testing.T) {
	err := checkCABIVersion(bindingCABIVersion + 1)
	if !errors.Is(err, StatusUnsupported) || !strings.Contains(err.Error(), "C ABI version") {
		t.Fatalf("checkCABIVersion(other) = %v, want StatusUnsupported naming the version", err)
	}
}
