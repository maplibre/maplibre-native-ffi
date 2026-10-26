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
// provider that denies every request, and waits that receive copied events.
// Native semantics are tested once, in tests/native; these tests
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
	state := &futureState[T]{ready: make(chan struct{})}
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

// createMap creates a map and waits until it is ready, closing it if its
// creation fails.
func createMap(t *testing.T, runtime *RuntimeHandle, options MapOptions) *MapHandle {
	t.Helper()
	m, ready, err := runtime.CreateMap(options)
	if err != nil {
		t.Fatal(err)
	}
	ctx, cancel := context.WithTimeout(context.Background(), testTimeout())
	defer cancel()
	if _, err := ready.Await(ctx); err != nil {
		_, _ = m.Close()
		t.Fatal(err)
	}
	return m
}

// awaitFailure waits for a future and returns its *Error, failing unless the
// future failed with one.
func awaitFailure[T any](t *testing.T, future *Future[T]) *Error {
	t.Helper()
	ctx, cancel := context.WithTimeout(context.Background(), testTimeout())
	defer cancel()
	value, err := future.Await(ctx)
	var native *Error
	if !errors.As(err, &native) {
		t.Fatalf("Await() = %+v, %v; want an *Error", value, err)
	}
	return native
}

// testMapOptions returns map options with every required member set: a 256
// by 256 initial extent at scale factor 1.
func testMapOptions() MapOptions {
	return MapOptions{InitialExtent: LogicalExtent{Width: 256, Height: 256, ScaleFactor: 1}}
}

type fixture struct {
	runtime       *RuntimeHandle
	m             *MapHandle
	pendingEvents chan RuntimeEvent
}

// newFixture creates a runtime with an in-memory cache and a denying resource
// provider, and one map, and closes both when the test ends.
func newFixture(t *testing.T) *fixture {
	t.Helper()
	return newFixtureWith(t, testMapOptions())
}

// newFixtureWith is newFixture with the map created from options.
func newFixtureWith(t *testing.T, options MapOptions) *fixture {
	t.Helper()
	f := newRuntimeFixture(t)
	f.m = createMap(t, f.runtime, options)
	// A test may already have released its fixture owner.
	t.Cleanup(func() {
		future, err := f.m.Close()
		if !errors.Is(err, StatusInvalidState) {
			await(t, submitted(future, err))
		}
	})
	return f
}

// newRuntimeFixture is newFixture without the map.
func newRuntimeFixture(t *testing.T) *fixture {
	t.Helper()
	f := &fixture{pendingEvents: make(chan RuntimeEvent, 4096)}
	runtime, err := NewRuntime(RuntimeOptions{CachePath: ":memory:"})
	if err != nil {
		t.Fatalf("NewRuntime: %v", err)
	}
	f.runtime = runtime
	// A handler accepted before CreateMap receives the map's first events.
	if err := runtime.Discarding().SetEventHandler(RuntimeEventHandler{Callback: func(events []RuntimeEvent) {
		for _, event := range events {
			f.pendingEvents <- event
		}
	}}); err != nil {
		t.Fatal(err)
	}
	t.Cleanup(func() {
		future, err := f.runtime.Close()
		if !errors.Is(err, StatusInvalidState) {
			await(t, submitted(future, err))
		}
	})
	await(t, submitted(runtime.SetResourceProvider(denyingProvider())))
	return f
}

// denyingProvider fails every request, so no test reaches the network.
func denyingProvider() ResourceProvider {
	return ResourceProvider{Callback: func(_ ResourceRequestInfo, request *ResourceRequestHandle) {
		_ = request.Complete(ResourceResponse{
			Status:       ResourceResponseStatusError,
			ErrorReason:  ResourceErrorReasonNotFound,
			ErrorMessage: "the test fixture serves no resources",
		})
	}}
}

// serveStyle installs a provider that answers url with an empty style and
// denies every other request.
func (f *fixture) serveStyle(t *testing.T, url string) {
	t.Helper()
	deny := denyingProvider().Callback
	await(t, submitted(f.runtime.SetResourceProvider(ResourceProvider{Callback: func(request ResourceRequestInfo, handle *ResourceRequestHandle) {
		if request.RequestedUrl != url {
			deny(request, handle)
			return
		}
		_ = handle.Complete(ResourceResponse{Status: ResourceResponseStatusOk, Bytes: []byte(emptyStyle)})
	}})))
}

// serveLoopback starts an HTTP server on the loopback interface that answers
// each path in bodies, and installs a provider that passes the server's
// requests through to the built-in HTTP stack and denies the rest. It returns
// the server's base URL. Resource transforms apply on that stack only.
//
// Android's stack calls Java, which a test executable runs without, so a test
// that needs it skips there; the Kotlin device suite covers that stack.
func (f *fixture) serveLoopback(t *testing.T, bodies map[string]string) string {
	t.Helper()
	if stdruntime.GOOS == "android" {
		t.Skip("Android's HTTP stack calls Java, which test executables run without")
	}
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
	await(t, submitted(f.runtime.SetResourceProvider(ResourceProvider{Callback: func(request ResourceRequestInfo, handle *ResourceRequestHandle) {
		if strings.HasPrefix(request.RequestedUrl, server.URL+"/") {
			_ = handle.PassThrough()
			return
		}
		deny(request, handle)
	}})))
	return server.URL
}

// awaitEvent receives events until one matches, discarding earlier events.
func (f *fixture) awaitEvent(t *testing.T, what string, match func(RuntimeEvent) bool) RuntimeEvent {
	t.Helper()
	deadline := time.NewTimer(testTimeout())
	defer deadline.Stop()
	for {
		select {
		case event := <-f.pendingEvents:
			if match(event) {
				return event
			}
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
// dropped. A runtime command orders a map's retirement, which disposal only
// schedules.
func closeOnceCollected(t *testing.T, runtime *RuntimeHandle, what string) {
	t.Helper()
	awaitCondition(t, what, func() bool {
		stdruntime.GC()
		await(t, submitted(runtime.ClearResourceTransform()))
		teardown, err := runtime.Close()
		if errors.Is(err, StatusInvalidState) {
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
