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
	dragMode dragMode
	lastX    float64
	lastY    float64
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

func (input *inputController) handleEvent(event *sdl.Event, m *maplibre.MapHandle, v viewport) (bool, error) {
	switch event.Type() {
	case sdl.EventMouseButtonDown:
		return input.handleMouseButtonDown(event.MouseButton(), m, v)
	case sdl.EventMouseButtonUp:
		return input.handleMouseButtonUp(event.MouseButton(), v), nil
	case sdl.EventMouseMotion:
		return input.handleMouseMotion(event.MouseMotion(), m, v)
	case sdl.EventMouseWheel:
		return handleMouseWheel(event.MouseWheel(), m, v)
	case sdl.EventKeyDown:
		return handleKeyDown(event.Keyboard(), m, v)
	default:
		return false, nil
	}
}

func (input *inputController) handleMouseButtonDown(event *sdl.MouseButtonEvent, m *maplibre.MapHandle, v viewport) (bool, error) {
	if event == nil {
		return false, nil
	}
	cursor := logicalPoint(float64(event.X), float64(event.Y), v)
	input.lastX = cursor.X
	input.lastY = cursor.Y
	mode := dragModeForButton(event.Button)
	if mode == dragNone {
		return false, nil
	}
	if err := m.CancelTransitions(); err != nil {
		return false, fmt.Errorf("cancel camera transitions failed: %w", err)
	}
	input.dragMode = mode
	return false, nil
}

func (input *inputController) handleMouseButtonUp(event *sdl.MouseButtonEvent, v viewport) bool {
	if event == nil || (event.Button != sdl.ButtonLeft && event.Button != sdl.ButtonRight) {
		return false
	}
	cursor := logicalPoint(float64(event.X), float64(event.Y), v)
	input.dragMode = dragNone
	input.lastX = cursor.X
	input.lastY = cursor.Y
	return false
}

func (input *inputController) handleMouseMotion(event *sdl.MouseMotionEvent, m *maplibre.MapHandle, v viewport) (bool, error) {
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
		if err := m.MoveBy(maplibre.ScreenPoint{X: dx, Y: dy}); err != nil {
			return false, fmt.Errorf("camera pan failed: %w", err)
		}
	case dragRotate:
		if err := adjustBearing(m, dx*0.5); err != nil {
			return false, err
		}
		if err := m.PitchBy(dy * 0.5); err != nil {
			return false, fmt.Errorf("camera pitch failed: %w", err)
		}
	}
	return true, nil
}

func handleMouseWheel(event *sdl.MouseWheelEvent, m *maplibre.MapHandle, v viewport) (bool, error) {
	if event == nil || event.Y == 0 {
		return false, nil
	}
	anchor := logicalPoint(float64(event.MouseX), float64(event.MouseY), v)
	if err := m.ScaleBy(math.Pow(2, float64(event.Y)*0.25), &anchor); err != nil {
		return false, fmt.Errorf("camera zoom failed: %w", err)
	}
	return true, nil
}

func handleKeyDown(event *sdl.KeyboardEvent, m *maplibre.MapHandle, v viewport) (bool, error) {
	if event == nil {
		return false, nil
	}
	const (
		panStep     = 120.0
		zoomStep    = 1.25
		bearingStep = 10.0
		pitchStep   = 5.0
	)
	animation := maplibre.AnimationOptions{}.WithDurationMS(160)
	center := maplibre.ScreenPoint{X: float64(v.logicalWidth) / 2, Y: float64(v.logicalHeight) / 2}

	switch event.Scancode {
	case sdl.ScancodeLeft, sdl.ScancodeA:
		return true, m.MoveByAnimated(maplibre.ScreenPoint{X: panStep}, &animation)
	case sdl.ScancodeRight, sdl.ScancodeD:
		return true, m.MoveByAnimated(maplibre.ScreenPoint{X: -panStep}, &animation)
	case sdl.ScancodeUp, sdl.ScancodeW:
		return true, m.MoveByAnimated(maplibre.ScreenPoint{Y: panStep}, &animation)
	case sdl.ScancodeDown, sdl.ScancodeS:
		return true, m.MoveByAnimated(maplibre.ScreenPoint{Y: -panStep}, &animation)
	case sdl.ScancodeEquals, sdl.ScancodeKPPlus:
		return true, m.ScaleByAnimated(zoomStep, &center, &animation)
	case sdl.ScancodeMinus, sdl.ScancodeKPMinus:
		return true, m.ScaleByAnimated(1/zoomStep, &center, &animation)
	case sdl.ScancodeQ:
		return true, adjustBearingAnimated(m, -bearingStep, &animation)
	case sdl.ScancodeE:
		return true, adjustBearingAnimated(m, bearingStep, &animation)
	case sdl.ScancodeRightbracket:
		return true, adjustPitchAnimated(m, pitchStep, &animation)
	case sdl.ScancodeLeftbracket:
		return true, adjustPitchAnimated(m, -pitchStep, &animation)
	case sdl.Scancode0:
		resetAnimation := maplibre.AnimationOptions{}.WithDurationMS(220)
		return true, m.EaseTo(maplibre.CameraOptions{}.WithBearing(0).WithPitch(0), &resetAnimation)
	default:
		return false, nil
	}
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

func adjustBearing(m *maplibre.MapHandle, delta float64) error {
	camera, err := m.Camera()
	if err != nil {
		return fmt.Errorf("camera snapshot failed: %w", err)
	}
	current := 0.0
	if camera.Bearing != nil {
		current = *camera.Bearing
	}
	if err := m.JumpTo(maplibre.CameraOptions{}.WithBearing(current + delta)); err != nil {
		return fmt.Errorf("camera rotate failed: %w", err)
	}
	return nil
}

func adjustBearingAnimated(m *maplibre.MapHandle, delta float64, animation *maplibre.AnimationOptions) error {
	camera, err := m.Camera()
	if err != nil {
		return fmt.Errorf("camera snapshot failed: %w", err)
	}
	current := 0.0
	if camera.Bearing != nil {
		current = *camera.Bearing
	}
	if err := m.EaseTo(maplibre.CameraOptions{}.WithBearing(current+delta), animation); err != nil {
		return fmt.Errorf("keyboard rotate failed: %w", err)
	}
	return nil
}

func adjustPitchAnimated(m *maplibre.MapHandle, delta float64, animation *maplibre.AnimationOptions) error {
	camera, err := m.Camera()
	if err != nil {
		return fmt.Errorf("camera snapshot failed: %w", err)
	}
	current := 0.0
	if camera.Pitch != nil {
		current = *camera.Pitch
	}
	if err := m.EaseTo(maplibre.CameraOptions{}.WithPitch(clamp(current+delta, 0, 60)), animation); err != nil {
		return fmt.Errorf("keyboard pitch failed: %w", err)
	}
	return nil
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
