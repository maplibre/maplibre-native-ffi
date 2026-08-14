package maplibre

import (
	"errors"
	"testing"
)

func TestPhase3NilHandles(t *testing.T) {
	var session *RenderSessionHandle
	if err := session.RequestFrame(NewFrameDemand()); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("RequestFrame error = %v", err)
	}
	if _, err := session.Snapshot(); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("Snapshot error = %v", err)
	}
	if _, err := session.ServiceDriverWork(1); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("ServiceDriverWork error = %v", err)
	}
	if _, err := session.BarrierStart(0); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("BarrierStart error = %v", err)
	}
	var frame *AcquiredFrame
	if _, err := frame.Result(); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("frame Result error = %v", err)
	}
}

func TestPhase3Defaults(t *testing.T) {
	options := NewRenderSessionAttachOptions()
	if options.Driver != RenderDriverCoreWorker {
		t.Fatalf("driver = %v", options.Driver)
	}
	demand := NewFrameDemand()
	if demand.Flags != FrameDemandIfNeeded {
		t.Fatalf("flags = %v", demand.Flags)
	}
	if RenderResultDeadlineMissed == RenderResultSuperseded {
		t.Fatal("deadline disposition aliases superseded")
	}
}
