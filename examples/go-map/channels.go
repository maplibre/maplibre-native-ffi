package main

import (
	"sync"
	"sync/atomic"

	maplibre "github.com/maplibre/maplibre-native-ffi/bindings/go"
)

// cameraCommand is decoded on the render loop and applied on the runtime
// loop. Commands that depend on the current camera carry deltas so the read and
// write stay together on the map's owner thread.
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
// runtime loop has not applied yet.
//
// The queue grows rather than dropping, which is why it is not a buffered
// channel: a buffered channel is bounded, and sending on a full one either
// blocks the render loop or discards a command. Its commands are deltas and a
// gesture bracket, and neither survives being discarded: a dropped delta is
// motion the drag never gets back, and a dropped bracket leaves every delta
// after it attributed to no gesture. Only a stalled runtime loop grows it.
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

// drain is called on the runtime loop. It hands over everything queued so far
// and keeps the backing array for the next batch.
func (queue *commandQueue) drain(out []cameraCommand) []cameraCommand {
	queue.mu.Lock()
	defer queue.mu.Unlock()
	out = append(out, queue.pending...)
	queue.pending = queue.pending[:0]
	return out
}

// sharedState is the small cross-thread state surface between the render and
// runtime loops. The camera queue and one-time map publication use channels;
// this carries the render request, shutdown, and first failure.
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
