package main

import (
	"github.com/jfreymuth/go-sdl3/sdl"
	maplibre "github.com/maplibre/maplibre-native-ffi/bindings/go"
)

// loopWakes are the native wakes that bring the SDL thread back from its
// wait. Each one marks its receiver's work and pushes an SDL user event, so the
// wake returns at once and the SDL thread does the work.
type loopWakes struct {
	// events reports that the runtime has events to drain.
	events *loopWake
	// frames reports that the render session has frame results to drain.
	frames *loopWake
	// driverWork reports that the render session has driver work for the SDL
	// thread.
	driverWork *loopWake
}

func newLoopWakes() loopWakes {
	eventType := sdl.RegisterEvents(1)
	return loopWakes{
		events:     newLoopWake(eventType),
		frames:     newLoopWake(eventType),
		driverWork: newLoopWake(eventType),
	}
}

// attachOptions returns attach options for a session that the SDL thread
// drives.
func (wakes loopWakes) attachOptions(ringDepth uint32) maplibre.RenderSessionAttachOptions {
	options := maplibre.DefaultRenderSessionAttachOptions()
	options.Driver = maplibre.RenderDriverKindCallerGraphicsThread
	options.RequestedTextureRingDepth = ringDepth
	options.FrameWake = wakes.frames.wake()
	options.DriverWorkWake = wakes.driverWork.wake()
	return options
}

type loopWake struct {
	pending   chan struct{}
	eventType sdl.EventType
}

func newLoopWake(eventType sdl.EventType) *loopWake {
	return &loopWake{pending: make(chan struct{}, 1), eventType: eventType}
}

func (w *loopWake) wake() maplibre.Wake {
	return maplibre.Wake{Callback: func() {
		select {
		case w.pending <- struct{}{}:
		default:
		}
		// A full SDL queue drops the push, but the queue is then full of
		// events that end the wait anyway, and the mark stays set.
		var event sdl.Event
		event.SetUser(&sdl.UserEvent{Type: w.eventType})
		_ = sdl.PushEvent(&event)
	}}
}

// consume reports whether the wake fired since the last call.
func (w *loopWake) consume() bool {
	select {
	case <-w.pending:
		return true
	default:
		return false
	}
}
