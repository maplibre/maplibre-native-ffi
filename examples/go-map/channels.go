package main

import (
	"sync"
	"sync/atomic"

	maplibre "github.com/maplibre/maplibre-native-ffi/bindings/go"
)

// cameraCommand is decoded and submitted directly from the SDL event loop.
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

// cameraController submits commands directly to the any-thread map API.
type cameraController struct {
	state  *runtimeMapState
	shared *sharedState
}

func (commands *cameraController) submit(command cameraCommand) bool {
	if err := commands.state.applyCommand(command); err != nil {
		commands.shared.fail(err)
		return false
	}
	return true
}

// sharedState carries render requests and the first failure observed by the
// host loop.
type sharedState struct {
	renderRequested atomic.Bool
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
