package maplibre

import (
	"context"
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
	snapshot, err := m.Snapshot()
	if err != nil {
		t.Fatalf("Snapshot(): %v", err)
	}
	return snapshot.EventMask
}

const emptyStyleJSON = `{"version":8,"sources":{},"layers":[]}`

// newRuntimeAndMap creates a runtime and one map, and registers their close.
func newRuntimeAndMap(t *testing.T, options *MapOptions) (*RuntimeHandle, *MapHandle) {
	t.Helper()

	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	var m *MapHandle
	if options == nil {
		m, err = awaitForTest(runtime.NewMap())
	} else {
		m, err = awaitForTest(runtime.NewMapWithOptions(*options))
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
