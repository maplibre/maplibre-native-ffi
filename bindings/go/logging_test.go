package maplibre

import (
	"errors"
	"testing"
)

// Installing a log callback replaces the previous registration, and native
// runs the release callback for the state it dropped. Clearing does the same
// for the last one, so a host's callback state does not accumulate.
func TestLogCallbackReplacementReleasesThePreviousState(t *testing.T) {
	liveCallbacks := func() int {
		bindingGlobalRoots.Lock()
		defer bindingGlobalRoots.Unlock()
		return len(bindingGlobalRoots.values)
	}
	baseline := liveCallbacks()

	if err := LogSetCallback(func(LogSeverity, LogEvent, int64, string) uint32 { return 0 }); err != nil {
		t.Fatalf("LogSetCallback(): %v", err)
	}
	if live := liveCallbacks() - baseline; live != 1 {
		t.Fatalf("live log callback states after the install = %d, want 1", live)
	}

	if err := LogSetCallback(func(LogSeverity, LogEvent, int64, string) uint32 { return 1 }); err != nil {
		_ = LogClearCallback()
		t.Fatalf("LogSetCallback(replace): %v", err)
	}
	if live := liveCallbacks() - baseline; live != 1 {
		_ = LogClearCallback()
		t.Fatalf("live log callback states after the replacement = %d, want 1", live)
	}

	if err := LogClearCallback(); err != nil {
		t.Fatalf("LogClearCallback(): %v", err)
	}
	if live := liveCallbacks() - baseline; live != 0 {
		t.Fatalf("live log callback states after the clear = %d, want 0", live)
	}
}

func TestLoggingConfigurationUsesNativeABI(t *testing.T) {
	if err := LogSetAsyncSeverityMask(LogSeverityMaskDefault); err != nil {
		t.Fatalf("LogSetAsyncSeverityMask(default): %v", err)
	}
	if err := LogSetAsyncSeverityMask(LogSeverityMask(1 << 31)); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("LogSetAsyncSeverityMask(invalid) error = %v, want ErrInvalidArgument", err)
	}
	if err := LogSetCallback(func(LogSeverity, LogEvent, int64, string) uint32 { return 0 }); err != nil {
		t.Fatalf("LogSetCallback(): %v", err)
	}
	if err := LogSetCallback(func(LogSeverity, LogEvent, int64, string) uint32 { return 1 }); err != nil {
		_ = LogClearCallback()
		t.Fatalf("LogSetCallback(replace): %v", err)
	}
	if err := LogClearCallback(); err != nil {
		t.Fatalf("LogClearCallback(): %v", err)
	}
	if err := LogClearCallback(); err != nil {
		t.Fatalf("second LogClearCallback(): %v", err)
	}
}
