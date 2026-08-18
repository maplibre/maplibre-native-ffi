package maplibre

import (
	"runtime"
	"runtime/cgo"
	"testing"
	"time"
)

type completionRetentionProbe struct {
	finalized chan struct{}
}

func TestCompletionBridgeRetainsValueAfterFutureIsDropped(t *testing.T) {
	finalized := make(chan struct{})
	probe := &completionRetentionProbe{finalized: finalized}
	runtime.SetFinalizer(probe, func(probe *completionRetentionProbe) {
		close(probe.finalized)
	})

	state := &futureState[struct{}]{ready: make(chan struct{})}
	bridge := &completionBridge[struct{}]{state: state}
	handle := cgo.NewHandle(completionReceiver(bridge))
	future := &Future[struct{}]{state: state}
	future.retain(probe)

	probe = nil
	future = nil
	state = nil
	bridge = nil
	runtime.GC()
	select {
	case <-finalized:
		t.Fatal("retained value was finalized while native still owned the completion bridge")
	default:
	}

	receiver := handle.Value().(completionReceiver)
	receiver.release()
	handle.Delete()
	receiver = nil

	deadline := time.Now().Add(5 * time.Second)
	for {
		runtime.GC()
		select {
		case <-finalized:
			return
		default:
			if time.Now().After(deadline) {
				t.Fatal("retained value was not released with the native completion bridge")
			}
			runtime.Gosched()
		}
	}
}
