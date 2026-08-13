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

func (input *inputController) handleEvent(event *sdl.Event, commands *cameraController, v viewport) bool {
	switch event.Type() {
	case sdl.EventMouseButtonDown:
		return input.handleMouseButtonDown(event.MouseButton(), commands, v)
	case sdl.EventMouseButtonUp:
		return input.handleMouseButtonUp(event.MouseButton(), commands, v)
	case sdl.EventMouseMotion:
		return input.handleMouseMotion(event.MouseMotion(), commands, v)
	case sdl.EventMouseWheel:
		return handleMouseWheel(event.MouseWheel(), commands, v)
	case sdl.EventKeyDown:
		return handleKeyDown(event.Keyboard(), commands, v)
	default:
		return false
	}
}

func (input *inputController) handleMouseButtonDown(event *sdl.MouseButtonEvent, commands *cameraController, v viewport) bool {
	if event == nil {
		return false
	}
	// A second button pressed during a live drag joins it, leaving the drag
	// baseline alone.
	if input.dragMode != dragNone {
		return false
	}
	mode := dragModeForButton(event.Button)
	if mode == dragNone {
		return false
	}
	cursor := logicalPoint(float64(event.X), float64(event.Y), v)
	input.lastX = cursor.X
	input.lastY = cursor.Y
	input.dragMode = mode
	input.dragButton = event.Button
	// Cancel first, so the running transition stops before the first delta.
	cancelQueued := enqueueCameraCommand(commands, cameraCommand{kind: commandCancelTransitions})
	gestureQueued := enqueueCameraCommand(commands, cameraCommand{kind: commandSetGestureInProgress, inProgress: true})
	return cancelQueued || gestureQueued
}

// handleMouseButtonUp ends the drag only for the button that started it, so the
// gesture bracket stays paired.
func (input *inputController) handleMouseButtonUp(event *sdl.MouseButtonEvent, commands *cameraController, v viewport) bool {
	if event == nil || (event.Button != sdl.ButtonLeft && event.Button != sdl.ButtonRight) {
		return false
	}
	if input.dragMode == dragNone || event.Button != input.dragButton {
		return false
	}
	cursor := logicalPoint(float64(event.X), float64(event.Y), v)
	input.dragMode = dragNone
	input.dragButton = 0
	input.lastX = cursor.X
	input.lastY = cursor.Y
	return enqueueCameraCommand(commands, cameraCommand{kind: commandSetGestureInProgress, inProgress: false})
}

func (input *inputController) handleMouseMotion(event *sdl.MouseMotionEvent, commands *cameraController, v viewport) bool {
	if event == nil || input.dragMode == dragNone {
		return false
	}
	cursor := logicalPoint(float64(event.X), float64(event.Y), v)
	dx := cursor.X - input.lastX
	dy := cursor.Y - input.lastY
	input.lastX = cursor.X
	input.lastY = cursor.Y
	if dx == 0 && dy == 0 {
		return false
	}

	switch input.dragMode {
	case dragPan:
		return enqueueCameraCommand(commands, cameraCommand{kind: commandMoveBy, deltaX: dx, deltaY: dy})
	case dragRotate:
		bearingQueued := enqueueCameraCommand(commands, cameraCommand{kind: commandAdjustBearing, deltaX: dx * 0.5})
		pitchQueued := enqueueCameraCommand(commands, cameraCommand{kind: commandPitchBy, deltaY: dy * 0.5})
		return bearingQueued || pitchQueued
	}
	return false
}

func handleMouseWheel(event *sdl.MouseWheelEvent, commands *cameraController, v viewport) bool {
	if event == nil || event.Y == 0 {
		return false
	}
	anchor := logicalPoint(float64(event.MouseX), float64(event.MouseY), v)
	return enqueueCameraCommand(commands, cameraCommand{kind: commandScaleBy, scale: math.Pow(2, float64(event.Y)*0.25), anchor: anchor})
}

func handleKeyDown(event *sdl.KeyboardEvent, commands *cameraController, v viewport) bool {
	if event == nil {
		return false
	}
	const (
		panStep     = 120.0
		zoomStep    = 1.25
		bearingStep = 10.0
		pitchStep   = 5.0
	)
	center := maplibre.ScreenPoint{X: float64(v.logicalWidth) / 2, Y: float64(v.logicalHeight) / 2}
	command := cameraCommand{durationMS: 160}

	switch event.Scancode {
	case sdl.ScancodeLeft, sdl.ScancodeA:
		command.kind, command.deltaX = commandMoveByAnimated, panStep
	case sdl.ScancodeRight, sdl.ScancodeD:
		command.kind, command.deltaX = commandMoveByAnimated, -panStep
	case sdl.ScancodeUp, sdl.ScancodeW:
		command.kind, command.deltaY = commandMoveByAnimated, panStep
	case sdl.ScancodeDown, sdl.ScancodeS:
		command.kind, command.deltaY = commandMoveByAnimated, -panStep
	case sdl.ScancodeEquals, sdl.ScancodeKPPlus:
		command.kind, command.scale, command.anchor = commandScaleByAnimated, zoomStep, center
	case sdl.ScancodeMinus, sdl.ScancodeKPMinus:
		command.kind, command.scale, command.anchor = commandScaleByAnimated, 1/zoomStep, center
	case sdl.ScancodeQ:
		command.kind, command.deltaX = commandAdjustBearingAnimated, -bearingStep
	case sdl.ScancodeE:
		command.kind, command.deltaX = commandAdjustBearingAnimated, bearingStep
	case sdl.ScancodeRightbracket:
		command.kind, command.deltaY = commandAdjustPitchAnimated, pitchStep
	case sdl.ScancodeLeftbracket:
		command.kind, command.deltaY = commandAdjustPitchAnimated, -pitchStep
	case sdl.Scancode0:
		command.kind = commandResetOrientation
	default:
		return false
	}
	return enqueueCameraCommand(commands, command)
}

func enqueueCameraCommand(commands *cameraController, command cameraCommand) bool {
	return commands.submit(command)
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
