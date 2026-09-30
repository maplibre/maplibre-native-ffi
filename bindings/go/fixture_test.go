package maplibre

import (
	"context"
	"errors"
	"io"
	"net/http"
	"net/http/httptest"
	"os"
	stdruntime "runtime"
	"strconv"
	"strings"
	"testing"
	"time"
	"weak"
)

// The suite's shared fixture: one runtime and one map per test, a resource
// provider that denies every request, and waits that block on the runtime's
// event wake. Native semantics are tested once, in tests/native; these tests
// cover what the binding adds on top.
//
// Most tests reach no server at all. The resource transform tests, which only
// the built-in HTTP stack calls, use serveLoopback: an httptest server on the
// loopback interface that answers the paths a test names.

const emptyStyle = `{"version":8,"sources":{},"layers":[]}`

// testTimeout bounds each wait. MLN_TEST_TIMEOUT_SCALE stretches it on
// emulators and software renderers, as it does for the C suite.
func testTimeout() time.Duration {
	scale, err := strconv.Atoi(os.Getenv("MLN_TEST_TIMEOUT_SCALE"))
	if err != nil || scale < 1 {
		scale = 1
	}
	return 10 * time.Second * time.Duration(scale)
}

// submitted folds a submission's error into the future it returned, so that
// one call can await what a submitting call returns:
//
//	await(t, submitted(m.Close()))
func submitted[T any](future *Future[T], err error) *Future[T] {
	if err == nil {
		return future
	}
	state := &futureState[T]{ready: make(chan struct{}), completed: true}
	state.result.err = err
	close(state.ready)
	return &Future[T]{state: state}
}

// await waits for future and fails the test on its error. A cleanup may call
// it, so it never uses t.Context(), which ends before cleanups run.
func await[T any](t *testing.T, future *Future[T]) T {
	t.Helper()
	ctx, cancel := context.WithTimeout(context.Background(), testTimeout())
	defer cancel()
	value, err := future.Await(ctx)
	if err != nil {
		t.Fatal(err)
	}
	return value
}

// awaitCommitted waits for a command and fails unless it committed.
func awaitCommitted(t *testing.T, future *Future[CommandCompletion]) {
	t.Helper()
	if completion := await(t, future); completion.Disposition != CommandDispositionCommitted {
		t.Fatalf("command %+v, want committed", completion)
	}
}

type fixture struct {
	runtime *RuntimeHandle
	m       *MapHandle
	// events receives the runtime's event wake, which fires when the event
	// queue becomes nonempty.
	events chan struct{}
}

// newFixture creates a runtime with an in-memory cache and a denying resource
// provider, and one map, and closes both when the test ends.
func newFixture(t *testing.T) *fixture {
	t.Helper()
	return newFixtureWith(t, DefaultMapOptions())
}

// newFixtureWith is newFixture with the map created from options.
func newFixtureWith(t *testing.T, options MapOptions) *fixture {
	t.Helper()
	f := newRuntimeFixture(t)
	f.m = await(t, submitted(f.runtime.MapCreate(options)))
	// A test that closed the map already gets a completed future here.
	t.Cleanup(func() { await(t, submitted(f.m.Close())) })
	return f
}

// newRuntimeFixture is newFixture without the map.
func newRuntimeFixture(t *testing.T) *fixture {
	t.Helper()
	f := &fixture{events: make(chan struct{}, 1)}
	options := DefaultRuntimeOptions()
	cache := ":memory:"
	options.CachePath = &cache
	options.EventWake = Wake{Callback: func() { notify(f.events) }}
	runtime, err := RuntimeCreate(options)
	if err != nil {
		t.Fatalf("RuntimeCreate: %v", err)
	}
	f.runtime = runtime
	t.Cleanup(func() { await(t, submitted(f.runtime.Close())) })
	await(t, submitted(runtime.SetResourceProvider(denyingProvider())))
	return f
}

// denyingProvider fails every request, so no test reaches the network.
func denyingProvider() ResourceProvider {
	return ResourceProvider{Callback: func(_ ResourceRequest, request *ResourceRequestHandle) ResourceProviderDecision {
		message := "the test fixture serves no resources"
		_ = request.Complete(ResourceResponse{
			Status:       ResourceResponseStatusError,
			ErrorReason:  ResourceErrorReasonNotFound,
			ErrorMessage: &message,
		})
		_ = request.Close()
		return ResourceProviderDecisionHandle
	}}
}

// serveStyle installs a provider that answers url with an empty style and
// denies every other request.
func (f *fixture) serveStyle(t *testing.T, url string) {
	t.Helper()
	deny := denyingProvider().Callback
	await(t, submitted(f.runtime.SetResourceProvider(ResourceProvider{Callback: func(request ResourceRequest, handle *ResourceRequestHandle) ResourceProviderDecision {
		if request.RequestedUrl == nil || *request.RequestedUrl != url {
			return deny(request, handle)
		}
		_ = handle.Complete(ResourceResponse{Status: ResourceResponseStatusOk, Bytes: []byte(emptyStyle)})
		_ = handle.Close()
		return ResourceProviderDecisionHandle
	}})))
}

// serveLoopback starts an HTTP server on the loopback interface that answers
// each path in bodies, and installs a provider that passes the server's
// requests through to the built-in HTTP stack and denies the rest. It returns
// the server's base URL. Resource transforms apply on that stack only.
func (f *fixture) serveLoopback(t *testing.T, bodies map[string]string) string {
	t.Helper()
	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		body, ok := bodies[r.URL.Path]
		if !ok {
			http.NotFound(w, r)
			return
		}
		w.Header().Set("Content-Type", "application/json")
		_, _ = io.WriteString(w, body)
	}))
	t.Cleanup(server.Close)
	deny := denyingProvider().Callback
	await(t, submitted(f.runtime.SetResourceProvider(ResourceProvider{Callback: func(request ResourceRequest, handle *ResourceRequestHandle) ResourceProviderDecision {
		if request.RequestedUrl != nil && strings.HasPrefix(*request.RequestedUrl, server.URL+"/") {
			return ResourceProviderDecisionPassThrough
		}
		return deny(request, handle)
	}})))
	return server.URL
}

// awaitEvent drains events until one matches, blocking on the event wake while
// the queue is empty. Events drained before the match are discarded.
func (f *fixture) awaitEvent(t *testing.T, what string, match func(RuntimeEvent) bool) RuntimeEvent {
	t.Helper()
	deadline := time.NewTimer(testTimeout())
	defer deadline.Stop()
	for {
		batch, err := f.runtime.DrainEvents()
		if err != nil {
			t.Fatalf("DrainEvents: %v", err)
		}
		view, err := batch.Get()
		if err != nil {
			t.Fatalf("event batch: %v", err)
		}
		if err := batch.Close(); err != nil {
			t.Fatalf("event batch close: %v", err)
		}
		for _, event := range view.Events {
			if match(event) {
				return event
			}
		}
		select {
		case <-f.events:
		case <-deadline.C:
			t.Fatalf("timed out waiting for %s", what)
		}
	}
}

func (f *fixture) awaitEventType(t *testing.T, eventType RuntimeEventType) RuntimeEvent {
	t.Helper()
	return f.awaitEvent(t, "a runtime event", func(event RuntimeEvent) bool { return event.Type == eventType })
}

// notify raises a wake without blocking. The channel holds one pending wake,
// so a wake between a drain and the next wait is never lost.
func notify(wake chan<- struct{}) {
	select {
	case wake <- struct{}{}:
	default:
	}
}

// awaitUnreachable runs the collector until the object behind pointer is
// unreachable, which is when its cleanup is due to dispose it.
func awaitUnreachable[T any](t *testing.T, pointer weak.Pointer[T], what string) {
	t.Helper()
	awaitCondition(t, what, func() bool {
		stdruntime.GC()
		return pointer.Value() == nil
	})
}

// closeOnceCollected runs the collector until runtime closes, which it refuses
// while a map is live, and so shows that cleanup disposed every map the test
// dropped. The barrier orders a map's retirement, which disposal only
// schedules.
func closeOnceCollected(t *testing.T, runtime *RuntimeHandle, what string) {
	t.Helper()
	awaitCondition(t, what, func() bool {
		stdruntime.GC()
		await(t, submitted(runtime.Barrier()))
		teardown, err := runtime.Close()
		if errors.Is(err, ErrInvalidState) {
			return false
		}
		await(t, submitted(teardown, err))
		return true
	})
}

// awaitCondition re-checks ready until it holds. Only states that raise no
// wake of their own use it: a collection, and a request's cancelled flag.
func awaitCondition(t *testing.T, what string, ready func() bool) {
	t.Helper()
	recheck := time.NewTicker(time.Millisecond)
	defer recheck.Stop()
	deadline := time.NewTimer(testTimeout())
	defer deadline.Stop()
	for !ready() {
		select {
		case <-recheck.C:
		case <-deadline.C:
			t.Fatalf("timed out waiting for %s", what)
		}
	}
}
