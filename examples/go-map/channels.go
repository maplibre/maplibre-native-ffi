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
