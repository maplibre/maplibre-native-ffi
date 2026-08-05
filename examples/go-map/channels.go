package main

import (
	"sync"
	"sync/atomic"

	maplibre "github.com/maplibre/maplibre-native-ffi/bindings/go"
)

// cameraCommand is decoded on the render loop and applied on the runtime loop.
// Commands that depend on the current camera carry deltas so the read and write
// stay together on the map's owner thread.
type cameraCommand struct {
	kind       cameraCommandKind
	deltaX     float64
	deltaY     float64
	scale      float64
	anchor     maplibre.ScreenPoint
	durationMS float64
	inProgress bool
}

type cameraCommandKind int

const (
	commandCancelTransitions cameraCommandKind = iota
	commandSetGestureInProgress
	commandMoveBy
	commandMoveByAnimated
	commandScaleBy
	commandScaleByAnimated
	commandPitchBy
	commandAdjustBearing
	commandAdjustBearingAnimated
	commandAdjustPitchAnimated
	commandResetOrientation
)

// commandQueue holds the camera commands the render loop has decoded and the
// runtime loop has not applied yet. It grows rather than dropping, because
// deltas and gesture brackets are not recoverable once discarded.
type commandQueue struct {
	mu      sync.Mutex
	pending []cameraCommand
}

// push is called on the render loop.
func (queue *commandQueue) push(command cameraCommand) {
	queue.mu.Lock()
	queue.pending = append(queue.pending, command)
	queue.mu.Unlock()
}

// drain is called on the runtime loop. It swaps the caller's buffer in for the
// pending slice, keeping the locked section O(1).
func (queue *commandQueue) drain(out []cameraCommand) []cameraCommand {
	queue.mu.Lock()
	defer queue.mu.Unlock()
	pending := queue.pending
	queue.pending = out[:0]
	return pending
}

// sharedState carries the render request, shutdown, and first failure between
// the render and runtime loops.
type sharedState struct {
	renderRequested atomic.Bool
	shutdown        atomic.Bool
	failureMu       sync.Mutex
	failure         error
}

func newSharedState() *sharedState {
	shared := &sharedState{}
	shared.renderRequested.Store(true)
	return shared
}

func (shared *sharedState) requestRender() {
	shared.renderRequested.Store(true)
}

func (shared *sharedState) consumeRenderRequest() bool {
	return shared.renderRequested.Swap(false)
}

func (shared *sharedState) requestShutdown() {
	shared.shutdown.Store(true)
}

func (shared *sharedState) shutdownRequested() bool {
	return shared.shutdown.Load()
}

func (shared *sharedState) fail(err error) {
	if err == nil {
		return
	}
	shared.failureMu.Lock()
	defer shared.failureMu.Unlock()
	if shared.failure == nil {
		shared.failure = err
	}
}

func (shared *sharedState) firstFailure() error {
	shared.failureMu.Lock()
	defer shared.failureMu.Unlock()
	return shared.failure
}
