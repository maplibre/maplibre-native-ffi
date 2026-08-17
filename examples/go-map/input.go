package main

import (
	"fmt"
	"math"

	"github.com/jfreymuth/go-sdl3/sdl"
	maplibre "github.com/maplibre/maplibre-native-ffi/bindings/go"
)

type dragMode int

const (
	dragNone dragMode = iota
	dragPan
	dragRotate
)

type inputController struct {
	dragMode   dragMode
	dragButton uint8
	lastX      float64
	lastY      float64
}

func logControls() {
	fmt.Println(`Controls:
  left drag: pan
  right drag or Ctrl+left drag: rotate with X, pitch with Y
  scroll: zoom at cursor
  arrows or WASD: pan
  + / -: zoom at center
  Q / E: rotate
  ] / [: pitch
  0: reset pitch and bearing`)
}

func (input *inputController) handleEvent(event *sdl.Event, state *runtimeMapState, v viewport) (bool, error) {
	switch event.Type() {
	case sdl.EventMouseButtonDown:
		return input.handleMouseButtonDown(event.MouseButton(), state, v)
	case sdl.EventMouseButtonUp:
		return input.handleMouseButtonUp(event.MouseButton(), state, v)
	case sdl.EventMouseMotion:
		return input.handleMouseMotion(event.MouseMotion(), state, v)
	case sdl.EventMouseWheel:
		return handleMouseWheel(event.MouseWheel(), state, v)
	case sdl.EventKeyDown:
		return handleKeyDown(event.Keyboard(), state, v)
	default:
		return false, nil
	}
}

func (input *inputController) handleMouseButtonDown(event *sdl.MouseButtonEvent, state *runtimeMapState, v viewport) (bool, error) {
	if event == nil {
		return false, nil
	}
	// A second button pressed during a live drag joins it, leaving the drag
	// baseline alone.
	if input.dragMode != dragNone {
		return false, nil
	}
	mode := dragModeForButton(event.Button)
	if mode == dragNone {
		return false, nil
	}
	cursor := logicalPoint(float64(event.X), float64(event.Y), v)
	input.lastX = cursor.X
	input.lastY = cursor.Y
	input.dragMode = mode
	input.dragButton = event.Button
	if err := state.cancelTransitions(); err != nil {
		return false, err
	}
	if err := state.setGestureInProgress(true); err != nil {
		return false, err
	}
	return true, nil
}

// handleMouseButtonUp ends the drag only for the button that started it, so the
// gesture bracket stays paired.
func (input *inputController) handleMouseButtonUp(event *sdl.MouseButtonEvent, state *runtimeMapState, v viewport) (bool, error) {
	if event == nil || (event.Button != sdl.ButtonLeft && event.Button != sdl.ButtonRight) {
		return false, nil
	}
	if input.dragMode == dragNone || event.Button != input.dragButton {
		return false, nil
	}
	cursor := logicalPoint(float64(event.X), float64(event.Y), v)
	input.dragMode = dragNone
	input.dragButton = 0
	input.lastX = cursor.X
	input.lastY = cursor.Y
	if err := state.setGestureInProgress(false); err != nil {
		return false, err
	}
	return true, nil
}

func (input *inputController) handleMouseMotion(event *sdl.MouseMotionEvent, state *runtimeMapState, v viewport) (bool, error) {
	if event == nil || input.dragMode == dragNone {
		return false, nil
	}
	cursor := logicalPoint(float64(event.X), float64(event.Y), v)
	dx := cursor.X - input.lastX
	dy := cursor.Y - input.lastY
	input.lastX = cursor.X
	input.lastY = cursor.Y
	if dx == 0 && dy == 0 {
		return false, nil
	}

	switch input.dragMode {
	case dragPan:
		return true, state.moveBy(dx, dy, nil)
	case dragRotate:
		if err := state.adjustBearing(dx*0.5, nil); err != nil {
			return false, err
		}
		return true, state.adjustPitch(dy*0.5, nil)
	}
	return false, nil
}

func handleMouseWheel(event *sdl.MouseWheelEvent, state *runtimeMapState, v viewport) (bool, error) {
	if event == nil || event.Y == 0 {
		return false, nil
	}
	anchor := logicalPoint(float64(event.MouseX), float64(event.MouseY), v)
	return true, state.scaleBy(math.Pow(2, float64(event.Y)*0.25), anchor, nil)
}

func handleKeyDown(event *sdl.KeyboardEvent, state *runtimeMapState, v viewport) (bool, error) {
	if event == nil {
		return false, nil
	}
	const (
		panStep     = 120.0
		zoomStep    = 1.25
		bearingStep = 10.0
		pitchStep   = 5.0
	)
	center := maplibre.ScreenPoint{X: float64(v.logicalWidth) / 2, Y: float64(v.logicalHeight) / 2}
	durationMS := 160.0
	var err error

	switch event.Scancode {
	case sdl.ScancodeLeft, sdl.ScancodeA:
		err = state.moveBy(panStep, 0, &durationMS)
	case sdl.ScancodeRight, sdl.ScancodeD:
		err = state.moveBy(-panStep, 0, &durationMS)
	case sdl.ScancodeUp, sdl.ScancodeW:
		err = state.moveBy(0, panStep, &durationMS)
	case sdl.ScancodeDown, sdl.ScancodeS:
		err = state.moveBy(0, -panStep, &durationMS)
	case sdl.ScancodeEquals, sdl.ScancodeKPPlus:
		err = state.scaleBy(zoomStep, center, &durationMS)
	case sdl.ScancodeMinus, sdl.ScancodeKPMinus:
		err = state.scaleBy(1/zoomStep, center, &durationMS)
	case sdl.ScancodeQ:
		err = state.adjustBearing(-bearingStep, &durationMS)
	case sdl.ScancodeE:
		err = state.adjustBearing(bearingStep, &durationMS)
	case sdl.ScancodeRightbracket:
		err = state.adjustPitch(pitchStep, &durationMS)
	case sdl.ScancodeLeftbracket:
		err = state.adjustPitch(-pitchStep, &durationMS)
	case sdl.Scancode0:
		err = state.resetOrientation(220)
	default:
		return false, nil
	}
	return err == nil, err
}

func dragModeForButton(button byte) dragMode {
	if button == sdl.ButtonRight {
		return dragRotate
	}
	if button != sdl.ButtonLeft {
		return dragNone
	}
	if sdl.GetModState()&sdl.ModCtrl != 0 {
		return dragRotate
	}
	return dragPan
}

func logicalPoint(x, y float64, v viewport) maplibre.ScreenPoint {
	return maplibre.ScreenPoint{
		X: logicalCoordinate(x, v.windowWidth, v.logicalWidth),
		Y: logicalCoordinate(y, v.windowHeight, v.logicalHeight),
	}
}

func logicalCoordinate(value float64, windowSize uint32, logicalSize uint32) float64 {
	if windowSize == 0 {
		return value
	}
	return value * float64(logicalSize) / float64(windowSize)
}

func clamp(value, min, max float64) float64 {
	if value < min {
		return min
	}
	if value > max {
		return max
	}
	return value
}
