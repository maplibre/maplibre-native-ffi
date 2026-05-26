package maplibre

import (
	"errors"
	"testing"
)

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
