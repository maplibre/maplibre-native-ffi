package maplibre

import (
	"errors"
	"strings"
	"sync/atomic"
	"testing"
	"time"
)

// countingResourceProvider passes every request through and counts the calls that
// native code makes from its file source threads.
func countingResourceProvider(calls *atomic.Int64) func(ResourceRequest, *ResourceRequestHandle) ResourceProviderDecision {
	return func(ResourceRequest, *ResourceRequestHandle) ResourceProviderDecision {
		calls.Add(1)
		return ResourceProviderDecisionPassThrough
	}
}

// loadProbeStyle requests a style URL that no file source serves; the failure
// event naming that URL proves the request reached the network file source.
func loadProbeStyle(t *testing.T, runtime *RuntimeHandle, m *MapHandle, styleURL string) {
	t.Helper()
	if _, err := m.SetStyleUrl(styleURL); err != nil {
		t.Fatalf("SetStyleUrl(%q): %v", styleURL, err)
	}
	for range make([]struct{}, 5000) {
		drained, err := drainEventsForTest(runtime)
		if err != nil {
			t.Fatalf("Close(): %v", err)
		}
		for _, event := range drained {
			if event.Type == RuntimeEventTypeMapLoadingFailed && strings.Contains(event.Message, styleURL) {
				return
			}
		}
		time.Sleep(time.Millisecond)
	}
	t.Fatalf("timed out waiting for a map loading failure naming %q", styleURL)
}

func TestRuntimeResourceProviderInstallsReplacesAndClears(t *testing.T) {
	runtime, m := newRuntimeAndMap(t, nil)

	var firstCalls, secondCalls atomic.Int64
	if _, err := runtime.SetResourceProvider(ResourceProvider{Callback: countingResourceProvider(&firstCalls)}); err != nil {
		t.Fatalf("SetResourceProvider(): %v", err)
	}
	loadProbeStyle(t, runtime, m, "jar:file:/packaged/first.json")
	if got := firstCalls.Load(); got == 0 {
		t.Fatalf("installed provider calls = %d, want at least 1", got)
	}

	if _, err := runtime.SetResourceProvider(ResourceProvider{Callback: countingResourceProvider(&secondCalls)}); err != nil {
		t.Fatalf("SetResourceProvider(replace): %v", err)
	}
	firstCallsAfterReplace := firstCalls.Load()
	loadProbeStyle(t, runtime, m, "jar:file:/packaged/second.json")
	if got := secondCalls.Load(); got == 0 {
		t.Fatalf("replacement provider calls = %d, want at least 1", got)
	}
	if got := firstCalls.Load(); got != firstCallsAfterReplace {
		t.Fatalf("replaced provider calls = %d, want %d", got, firstCallsAfterReplace)
	}

	if _, err := runtime.ClearResourceProvider(); err != nil {
		t.Fatalf("ClearResourceProvider(): %v", err)
	}
	secondCallsAfterClear := secondCalls.Load()
	loadProbeStyle(t, runtime, m, "jar:file:/packaged/third.json")
	if got := firstCalls.Load(); got != firstCallsAfterReplace {
		t.Fatalf("replaced provider calls after clear = %d, want %d", got, firstCallsAfterReplace)
	}
	if got := secondCalls.Load(); got != secondCallsAfterClear {
		t.Fatalf("cleared provider calls = %d, want %d", got, secondCallsAfterClear)
	}

	if _, err := runtime.ClearResourceProvider(); err != nil {
		t.Fatalf("second ClearResourceProvider(): %v", err)
	}
}

// a style URL using the default tile server's maplibre: scheme alias
// reaches the provider as the alias, alongside the HTTPS URL the built-in
// network path would have fetched.
func TestResourceProviderSeesSchemeAliasAndItsResolvedURL(t *testing.T) {
	var resolvedURL atomic.Value

	runtime, err := RuntimeCreate(DefaultRuntimeOptions())
	if err != nil {
		t.Fatalf("RuntimeCreate(DefaultRuntimeOptions()): %v", err)
	}
	if _, err := runtime.SetResourceProvider(ResourceProvider{Callback: func(request ResourceRequest, handle *ResourceRequestHandle) ResourceProviderDecision {
		if *request.RequestedUrl != "maplibre://maps/style" {
			return ResourceProviderDecisionPassThrough
		}
		resolvedURL.Store(*request.ResolvedUrl)
		if err := handle.Complete(ResourceResponse{Status: ResourceResponseStatusOk, Bytes: []byte(emptyStyleJSON)}); err != nil {
			return ResourceProviderDecisionPassThrough
		}
		return ResourceProviderDecisionHandle
	}}); err != nil {
		_ = closeRuntimeForTest(runtime)
		t.Fatalf("SetResourceProvider(): %v", err)
	}
	m, err := awaitForTest(runtime.MapCreate(DefaultMapOptions()))
	if err != nil {
		_ = closeRuntimeForTest(runtime)
		t.Fatalf("NewMap(): %v", err)
	}
	defer func() {
		if err := closeMapForTest(m); err != nil {
			t.Errorf("Map Close(): %v", err)
		}
		if err := closeRuntimeForTest(runtime); err != nil {
			t.Errorf("Runtime Close(): %v", err)
		}
	}()

	if _, err := m.SetStyleUrl("maplibre://maps/style"); err != nil {
		t.Fatalf("SetStyleUrl(): %v", err)
	}
	waitForRuntimeEvent(t, runtime, RuntimeEventTypeMapStyleLoaded)

	if got := resolvedURL.Load(); got != "https://demotiles.maplibre.org/style.json" {
		t.Fatalf("resolved URL = %v, want https://demotiles.maplibre.org/style.json", got)
	}
}

func TestRuntimeResourceProviderRejectsNilCallback(t *testing.T) {
	runtime, err := RuntimeCreate(DefaultRuntimeOptions())
	if err != nil {
		t.Fatalf("RuntimeCreate(DefaultRuntimeOptions()): %v", err)
	}
	defer func() {
		if err := closeRuntimeForTest(runtime); err != nil {
			t.Errorf("Close(): %v", err)
		}
	}()

	if _, err := runtime.SetResourceProvider(ResourceProvider{Callback: nil}); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetResourceProvider(nil) error = %v, want ErrInvalidArgument", err)
	}
}

func TestRuntimeResourceTransformLifecycle(t *testing.T) {
	runtime, err := RuntimeCreate(DefaultRuntimeOptions())
	if err != nil {
		t.Fatalf("RuntimeCreate(DefaultRuntimeOptions()): %v", err)
	}
	if _, err := runtime.SetResourceTransform(ResourceTransform{Callback: func(_ ResourceKind, url string, response *ResourceTransformResponseScope) Status {
		if err := response.SetUrl(url + "?first"); err != nil {
			return StatusInvalidArgument
		}
		return StatusOk
	}}); err != nil {
		_ = closeRuntimeForTest(runtime)
		t.Fatalf("SetResourceTransform(): %v", err)
	}
	if _, err := runtime.SetResourceTransform(ResourceTransform{Callback: func(ResourceKind, string, *ResourceTransformResponseScope) Status { return StatusOk }}); err != nil {
		_ = closeRuntimeForTest(runtime)
		t.Fatalf("SetResourceTransform(replace): %v", err)
	}
	if _, err := runtime.ClearResourceTransform(); err != nil {
		_ = closeRuntimeForTest(runtime)
		t.Fatalf("ClearResourceTransform(): %v", err)
	}
	if _, err := runtime.ClearResourceTransform(); err != nil {
		_ = closeRuntimeForTest(runtime)
		t.Fatalf("second ClearResourceTransform(): %v", err)
	}
	if err := closeRuntimeForTest(runtime); err != nil {
		t.Fatalf("Close(): %v", err)
	}
}

func TestRuntimeResourceTransformRejectsNilCallback(t *testing.T) {
	runtime, err := RuntimeCreate(DefaultRuntimeOptions())
	if err != nil {
		t.Fatalf("RuntimeCreate(DefaultRuntimeOptions()): %v", err)
	}
	defer func() {
		if err := closeRuntimeForTest(runtime); err != nil {
			t.Errorf("Close(): %v", err)
		}
	}()

	if _, err := runtime.SetResourceTransform(ResourceTransform{}); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetResourceTransform(nil) error = %v, want ErrInvalidArgument", err)
	}
}

// a request the provider handled but never completed reports one
// cancellation when the map that asked for it goes away, the callback can
// close that request from inside itself, a second registration on the same
// request reports invalid state, and native code releases the registration
// once the callback returns.
func TestResourceRequestCancelCallbackReportsDiscardedRequest(t *testing.T) {
	const styleURL = "jar:file:/packaged/cancelled-style.json"
	requested := make(chan struct{}, 1)
	cancelled := make(chan struct{}, 4)
	var cancelCalls atomic.Int64
	var registerErr, secondRegisterErr atomic.Value

	runtime, err := RuntimeCreate(DefaultRuntimeOptions())
	if err != nil {
		t.Fatalf("RuntimeCreate(DefaultRuntimeOptions()): %v", err)
	}
	defer func() {
		if err := closeRuntimeForTest(runtime); err != nil {
			t.Errorf("Runtime Close(): %v", err)
		}
	}()
	if _, err := runtime.SetResourceProvider(ResourceProvider{Callback: func(request ResourceRequest, handle *ResourceRequestHandle) ResourceProviderDecision {
		if *request.RequestedUrl != styleURL {
			return ResourceProviderDecisionPassThrough
		}
		if _, err := handle.SetCancelCallback(func() {
			cancelCalls.Add(1)
			handle.Close()
			cancelled <- struct{}{}
		}); err != nil {
			registerErr.Store(err)
		}
		if _, err := handle.SetCancelCallback(func() { cancelCalls.Add(1) }); err != nil {
			secondRegisterErr.Store(err)
		}
		select {
		case requested <- struct{}{}:
		default:
		}
		// The request stays open, so only cancellation retires it.
		return ResourceProviderDecisionHandle
	}}); err != nil {
		_ = closeRuntimeForTest(runtime)
		t.Fatalf("SetResourceProvider(): %v", err)
	}

	m, err := awaitForTest(runtime.MapCreate(DefaultMapOptions()))
	if err != nil {
		t.Fatalf("NewMap(): %v", err)
	}
	baseline := bindingCallbackCount.Load()
	if _, err := m.SetStyleUrl(styleURL); err != nil {
		t.Fatalf("SetStyleUrl(): %v", err)
	}
	waitForResourceSignalValue(t, requested, "the provider to receive the style request")
	if got := bindingCallbackCount.Load() - baseline; got != 1 {
		t.Fatalf("live cancel registrations before cancellation = %d, want 1", got)
	}
	// Map teardown discards the request the provider never completed, and the
	// cancel callback runs on the thread that discards it.
	teardown, err := m.Close()
	if err != nil {
		t.Fatalf("Map Close(): %v", err)
	}
	waitForResourceSignalValue(t, cancelled, "the cancel callback to run")
	if _, err := awaitForTest(teardown, nil); err != nil {
		t.Fatalf("map teardown: %v", err)
	}

	if err, ok := registerErr.Load().(error); ok {
		t.Fatalf("SetCancelCallback(): %v", err)
	}
	if err, ok := secondRegisterErr.Load().(error); !ok || !errors.Is(err, ErrInvalidState) {
		t.Fatalf("second SetCancelCallback() error = %v, want ErrInvalidState", err)
	}
	if _, err := awaitForTest(runtime.Barrier()); err != nil {
		t.Fatalf("Barrier(): %v", err)
	}
	if got := cancelCalls.Load(); got != 1 {
		t.Fatalf("cancel callback calls = %d, want 1", got)
	}
	if got := bindingCallbackCount.Load() - baseline; got != 0 {
		t.Fatalf("live cancel registrations after the callback = %d, want 0", got)
	}
}

// registering on a request MapLibre already cancelled reports the
// cancellation without storing or running the callback, and a closed request
// rejects registration as closed.
func TestResourceRequestCancelCallbackRunsForAlreadyCancelledRequest(t *testing.T) {
	const styleURL = "jar:file:/packaged/late-cancel-style.json"
	handles := make(chan *ResourceRequestHandle, 1)

	runtime, err := RuntimeCreate(DefaultRuntimeOptions())
	if err != nil {
		t.Fatalf("RuntimeCreate(DefaultRuntimeOptions()): %v", err)
	}
	defer func() {
		if err := closeRuntimeForTest(runtime); err != nil {
			t.Errorf("Runtime Close(): %v", err)
		}
	}()
	if _, err := runtime.SetResourceProvider(ResourceProvider{Callback: func(request ResourceRequest, handle *ResourceRequestHandle) ResourceProviderDecision {
		if *request.RequestedUrl != styleURL {
			return ResourceProviderDecisionPassThrough
		}
		select {
		case handles <- handle:
		default:
		}
		return ResourceProviderDecisionHandle
	}}); err != nil {
		_ = closeRuntimeForTest(runtime)
		t.Fatalf("SetResourceProvider(): %v", err)
	}

	m, err := awaitForTest(runtime.MapCreate(DefaultMapOptions()))
	if err != nil {
		t.Fatalf("NewMap(): %v", err)
	}
	if _, err := m.SetStyleUrl(styleURL); err != nil {
		t.Fatalf("SetStyleUrl(): %v", err)
	}
	handle := waitForResourceSignalValue(t, handles, "the provider to receive the style request")
	if err := closeMapForTest(m); err != nil {
		t.Fatalf("Map Close(): %v", err)
	}
	waitForResourceRequestCancelled(t, handle)

	var calls int
	baseline := bindingCallbackCount.Load()
	cancelled, err := handle.SetCancelCallback(func() { calls++ })
	if err != nil || !cancelled || calls != 0 {
		t.Fatalf("already cancelled registration: %v, %v, calls %d", cancelled, err, calls)
	}
	// The binding frees the unstored registration itself, and native code never
	// releases it.
	if got := bindingCallbackCount.Load() - baseline; got != 0 {
		t.Fatalf("live cancel registrations after an already cancelled registration = %d, want 0", got)
	}

	handle.Close()
	if _, err := handle.SetCancelCallback(func() { calls++ }); !errors.Is(err, ErrInvalidState) {
		t.Fatalf("SetCancelCallback() after Close error = %v, want ErrInvalidState", err)
	}
	if calls != 0 {
		t.Fatalf("cancel callback calls after Close = %d, want 0", calls)
	}
	if got := bindingCallbackCount.Load() - baseline; got != 0 {
		t.Fatalf("live cancel registrations after Close = %d, want 0", got)
	}
}

// a request the provider completed is not reported as cancelled, even
// once the map that asked for it goes away, and closing the request releases
// the registration whose callback never ran.
func TestResourceRequestCancelCallbackSkipsCompletedRequest(t *testing.T) {
	const styleURL = "jar:file:/packaged/completed-style.json"
	var cancelCalls atomic.Int64
	var providerErr atomic.Value
	handles := make(chan *ResourceRequestHandle, 1)

	runtime, err := RuntimeCreate(DefaultRuntimeOptions())
	if err != nil {
		t.Fatalf("RuntimeCreate(DefaultRuntimeOptions()): %v", err)
	}
	defer func() {
		if err := closeRuntimeForTest(runtime); err != nil {
			t.Errorf("Runtime Close(): %v", err)
		}
	}()
	if _, err := runtime.SetResourceProvider(ResourceProvider{Callback: func(request ResourceRequest, handle *ResourceRequestHandle) ResourceProviderDecision {
		if *request.RequestedUrl != styleURL {
			return ResourceProviderDecisionPassThrough
		}
		if _, err := handle.SetCancelCallback(func() { cancelCalls.Add(1) }); err != nil {
			providerErr.Store(err)
		}
		if err := handle.Complete(ResourceResponse{
			Status: ResourceResponseStatusOk,
			Bytes:  []byte(minimalStyleJSON),
		}); err != nil {
			providerErr.Store(err)
		}
		select {
		case handles <- handle:
		default:
		}
		return ResourceProviderDecisionHandle
	}}); err != nil {
		_ = closeRuntimeForTest(runtime)
		t.Fatalf("SetResourceProvider(): %v", err)
	}

	m, err := awaitForTest(runtime.MapCreate(DefaultMapOptions()))
	if err != nil {
		t.Fatalf("NewMap(): %v", err)
	}
	baseline := bindingCallbackCount.Load()
	if _, err := m.SetStyleUrl(styleURL); err != nil {
		t.Fatalf("SetStyleUrl(): %v", err)
	}
	waitForRuntimeEvent(t, runtime, RuntimeEventTypeMapStyleLoaded)
	handle := waitForResourceSignalValue(t, handles, "the provider to receive the style request")
	teardown, err := m.Close()
	if err != nil {
		t.Fatalf("Map Close(): %v", err)
	}
	if _, err := awaitForTest(teardown, nil); err != nil {
		t.Fatalf("map teardown: %v", err)
	}

	if err, ok := providerErr.Load().(error); ok {
		t.Fatalf("provider error: %v", err)
	}
	if got := cancelCalls.Load(); got != 0 {
		t.Fatalf("cancel callback calls for a completed request = %d, want 0", got)
	}
	if got := bindingCallbackCount.Load() - baseline; got != 1 {
		t.Fatalf("live cancel registrations before Close = %d, want 1", got)
	}
	handle.Close()
	if got := bindingCallbackCount.Load() - baseline; got != 0 {
		t.Fatalf("live cancel registrations after Close = %d, want 0", got)
	}
}

// waitForResourceSignalValue waits for a signal that native code raises from a
// MapLibre thread.
func waitForResourceSignalValue[T any](t *testing.T, signal <-chan T, what string) T {
	t.Helper()
	select {
	case value := <-signal:
		return value
	case <-time.After(30 * time.Second):
		t.Fatalf("timed out waiting for %s", what)
	}
	var zero T
	return zero
}

// waitForResourceRequestCancelled polls until native code reports the request
// as cancelled.
func waitForResourceRequestCancelled(t *testing.T, handle *ResourceRequestHandle) {
	t.Helper()
	for range make([]struct{}, 30000) {
		cancelled, err := handle.Cancelled()
		if err != nil {
			t.Fatalf("Cancelled(): %v", err)
		}
		if cancelled {
			return
		}
		time.Sleep(time.Millisecond)
	}
	t.Fatal("timed out waiting for the request to be cancelled")
}
