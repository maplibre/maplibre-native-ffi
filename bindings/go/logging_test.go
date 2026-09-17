package maplibre

import (
	"errors"
	"testing"

	"github.com/maplibre/maplibre-native-ffi/bindings/go/internal/callback"
)

// Installing a log callback replaces the previous registration, and native
// runs the release callback for the state it dropped. Clearing does the same
// for the last one, so a host's callback state does not accumulate.
func TestLogCallbackReplacementReleasesThePreviousState(t *testing.T) {
	baseline := callback.LogCallbackLiveCountForTest()

	if err := SetLogCallback(func(LogRecord) bool { return false }); err != nil {
		t.Fatalf("SetLogCallback(): %v", err)
	}
	if live := callback.LogCallbackLiveCountForTest() - baseline; live != 1 {
		t.Fatalf("live log callback states after the install = %d, want 1", live)
	}

	if err := SetLogCallback(func(LogRecord) bool { return true }); err != nil {
		_ = ClearLogCallback()
		t.Fatalf("SetLogCallback(replace): %v", err)
	}
	if live := callback.LogCallbackLiveCountForTest() - baseline; live != 1 {
		_ = ClearLogCallback()
		t.Fatalf("live log callback states after the replacement = %d, want 1", live)
	}

	if err := ClearLogCallback(); err != nil {
		t.Fatalf("ClearLogCallback(): %v", err)
	}
	if live := callback.LogCallbackLiveCountForTest() - baseline; live != 0 {
		t.Fatalf("live log callback states after the clear = %d, want 0", live)
	}
}

func TestLoggingConfigurationUsesNativeABI(t *testing.T) {
	if err := SetAsyncLogSeverityMask(LogSeverityMaskDefault); err != nil {
		t.Fatalf("SetAsyncLogSeverityMask(default): %v", err)
	}
	if err := SetAsyncLogSeverityMask(LogSeverityMask(1 << 31)); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetAsyncLogSeverityMask(invalid) error = %v, want ErrInvalidArgument", err)
	}
	if err := SetLogCallback(func(LogRecord) bool { return false }); err != nil {
		t.Fatalf("SetLogCallback(): %v", err)
	}
	if err := SetLogCallback(func(LogRecord) bool { return true }); err != nil {
		_ = ClearLogCallback()
		t.Fatalf("SetLogCallback(replace): %v", err)
	}
	if err := ClearLogCallback(); err != nil {
		t.Fatalf("ClearLogCallback(): %v", err)
	}
	if err := ClearLogCallback(); err != nil {
		t.Fatalf("second ClearLogCallback(): %v", err)
	}
}
