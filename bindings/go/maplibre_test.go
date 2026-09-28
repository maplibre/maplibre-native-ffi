package maplibre

import (
	"context"
	"errors"
	"testing"
	"time"
)

// awaitTimeout bounds every awaited native operation, so a stalled worker fails
// a test instead of hanging it.
const awaitTimeout = 30 * time.Second

func awaitForTest[T any](future *Future[T], err error) (T, error) {
	var zero T
	if err != nil {
		return zero, err
	}
	ctx, cancel := context.WithTimeout(context.Background(), awaitTimeout)
	defer cancel()
	return future.Await(ctx)
}

// closeRuntimeForTest closes a runtime and waits for its native teardown, so a
// test leaves no native thread running past its own end.
func closeRuntimeForTest(runtime *RuntimeHandle) error {
	_, err := awaitForTest(runtime.Close())
	return err
}

// closeMapForTest closes a map and waits for its native teardown, so a test
// leaves no native work running past its own end.
func closeMapForTest(m *MapHandle) error {
	_, err := awaitForTest(m.Close())
	return err
}

// pointerTo returns a pointer to value, for the optional fields the binding
// carries as pointers.
func pointerTo[T any](value T) *T {
	return &value
}

// mapEventMaskForTest reads the committed map event mask, which the published
// map snapshot carries.
func mapEventMaskForTest(t *testing.T, m *MapHandle) RuntimeEventMask {
	t.Helper()
	snapshot, err := m.SnapshotGet()
	if err != nil {
		t.Fatalf("Snapshot(): %v", err)
	}
	return snapshot.EventMask
}

const emptyStyleJSON = `{"version":8,"sources":{},"layers":[]}`

// newRuntimeAndMap creates a runtime and one map, and registers their close.
func newRuntimeAndMap(t *testing.T, options *MapOptions) (*RuntimeHandle, *MapHandle) {
	t.Helper()

	runtime, err := RuntimeCreate(DefaultRuntimeOptions())
	if err != nil {
		t.Fatalf("RuntimeCreate(DefaultRuntimeOptions()): %v", err)
	}
	var m *MapHandle
	if options == nil {
		m, err = awaitForTest(runtime.MapCreate(DefaultMapOptions()))
	} else {
		m, err = awaitForTest(runtime.MapCreate(*options))
	}
	if err != nil {
		_ = closeRuntimeForTest(runtime)
		t.Fatalf("NewMap(): %v", err)
	}
	t.Cleanup(func() {
		if err := closeMapForTest(m); err != nil {
			t.Errorf("Map Close(): %v", err)
		}
		if err := closeRuntimeForTest(runtime); err != nil {
			t.Errorf("Runtime Close(): %v", err)
		}
	})
	return runtime, m
}

const minimalStyleJSON = `{
  "version": 8,
  "name": "go-binding-style-test",
  "sources": {},
  "layers": [
    {"id":"background","type":"background","paint":{"background-color":"#d8f1ff"}}
  ]
}`

func mapOptionsForTest(width, height uint32, scale float64) MapOptions {
	options := DefaultMapOptions()
	options.InitialExtent = LogicalExtent{Width: width, Height: height, ScaleFactor: scale}
	return options
}

func drainEventsForTest(runtime *RuntimeHandle) ([]RuntimeEvent, error) {
	batch, err := runtime.DrainEvents()
	if err != nil {
		return nil, err
	}
	defer batch.Close()
	view, err := batch.Get()
	return view.Events, err
}

func drainFramesForTest(session *RenderSessionHandle) ([]RenderFrameResult, error) {
	batch, err := session.DrainFrameResults()
	if errors.Is(err, ErrNotReady) {
		return nil, nil
	}
	if err != nil {
		return nil, err
	}
	defer batch.Close()
	count, err := batch.Count()
	if err != nil {
		return nil, err
	}
	results := make([]RenderFrameResult, count)
	for i := range results {
		value, err := batch.Get(uint(i))
		if err != nil {
			return nil, err
		}
		results[i] = value
	}
	return results, nil
}

func jumpForTest(m *MapHandle, camera CameraOptions) (*Future[CommandCompletion], error) {
	return m.UpdateCamera(CameraUpdate{Mode: CameraUpdateModeJump, Camera: camera})
}

func easeForTest(m *MapHandle, camera CameraOptions, animation *AnimationOptions) (*Future[CommandCompletion], error) {
	return m.UpdateCamera(CameraUpdate{Mode: CameraUpdateModeEase, Camera: camera, Animation: *animation})
}

func flyForTest(m *MapHandle, camera CameraOptions, animation *AnimationOptions) (*Future[CommandCompletion], error) {
	return m.UpdateCamera(CameraUpdate{Mode: CameraUpdateModeFly, Camera: camera, Animation: *animation})
}

func runtimeOptionsForTest(asset, cache string) RuntimeOptions {
	options := DefaultRuntimeOptions()
	options.AssetPath = &asset
	options.CachePath = &cache
	return options
}

func offlineDefinitionForTest(value any) OfflineRegionDefinition {
	switch value := value.(type) {
	case OfflineTilePyramidRegionDefinition:
		return OfflineRegionDefinition{Data: OfflineRegionDefinitionDataTilePyramidVariant{Value: value}}
	case OfflineGeometryRegionDefinition:
		return OfflineRegionDefinition{Data: OfflineRegionDefinitionDataGeometryVariant{Value: value}}
	default:
		panic("unsupported offline fixture")
	}
}

func takeOptionalSliceForTest[T any](future *Future[[]T], err error) ([]T, bool, error) {
	result, err := awaitForTest(future, err)
	return result, result != nil, err
}
