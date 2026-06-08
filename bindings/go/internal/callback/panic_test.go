package callback

import "testing"

const (
	testLogSeverityInfo uint32 = 1
	testLogEventGeneral uint32 = 0
)

func TestLogCallbackTrampolineRecoversPanic(t *testing.T) {
	if consumed := invokeLogCallbackForTest(func(uint32, uint32, int64, string) bool {
		panic("boom")
	}); consumed != 0 {
		t.Fatalf("consumed = %d, want 0", consumed)
	}
}

func TestCustomGeometryTrampolineRecoversPanic(t *testing.T) {
	defer func() {
		if recovered := recover(); recovered != nil {
			t.Fatalf("custom geometry trampoline propagated panic: %v", recovered)
		}
	}()
	invokeCustomGeometryFetchForTest(func(CanonicalTileID) {
		panic("boom")
	})
}

func TestLogCallbackTrampolineReturnsConsumed(t *testing.T) {
	if consumed := invokeLogCallbackForTest(func(severity uint32, event uint32, code int64, message string) bool {
		if severity != testLogSeverityInfo || event != testLogEventGeneral || code != 0 || message != "test message" {
			t.Fatalf("record = (%d, %d, %d, %q)", severity, event, code, message)
		}
		return true
	}); consumed != 1 {
		t.Fatalf("consumed = %d, want 1", consumed)
	}
}

func TestCustomGeometryReleaseWaitsForActiveCallback(t *testing.T) {
	entered := make(chan struct{})
	unblock := make(chan struct{})
	released := make(chan struct{})

	state := newCustomGeometrySourceStateForTest(func(CanonicalTileID) {
		close(entered)
		<-unblock
	})

	go invokeCustomGeometryFetchStateForTest(state)
	<-entered

	go func() {
		state.Release()
		close(released)
	}()

	select {
	case <-released:
		t.Fatal("Release returned while callback was active")
	default:
	}

	close(unblock)
	<-released
	state.Release()
}
