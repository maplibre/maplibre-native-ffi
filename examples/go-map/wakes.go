package main

import (
	"github.com/jfreymuth/go-sdl3/sdl"
	maplibre "github.com/maplibre/maplibre-native-ffi/bindings/go"
)

// loopWake is a native wake for the SDL thread. The wake marks its work and
// pushes an SDL user event, so it returns at once and the SDL thread does the
// work after its wait ends.
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
