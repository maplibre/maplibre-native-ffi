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
func countingResourceProvider(calls *atomic.Int64) ResourceProviderCallback {
	return func(ResourceRequest, *ResourceRequestHandle) ResourceProviderDecision {
		calls.Add(1)
		return ResourceProviderDecisionPassThrough
	}
}

// loadProbeStyle requests a style URL that no file source serves; the failure
// event naming that URL proves the request reached the network file source.
func loadProbeStyle(t *testing.T, runtime *RuntimeHandle, m *MapHandle, styleURL string) {
	t.Helper()
	if _, err := m.SetStyleURL(styleURL); err != nil {
		t.Fatalf("SetStyleURL(%q): %v", styleURL, err)
	}
	for range make([]struct{}, 5000) {
		time.Sleep(time.Millisecond)
		batch, err := runtime.DrainEvents()
		if err != nil {
			t.Fatalf("DrainEvents(): %v", err)
		}
		for _, event := range batch.Events {
			if event.Type == RuntimeEventMapLoadingFailed && strings.Contains(event.Message, styleURL) {
				return
			}
		}
		time.Sleep(time.Millisecond)
	}
	t.Fatalf("timed out waiting for a map loading failure naming %q", styleURL)
}

func TestRuntimeResourceProviderInstallsReplacesAndClears(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	m, err := awaitForTest(runtime.NewMap())
	if err != nil {
		_ = closeRuntimeForTest(runtime)
		t.Fatalf("NewMap(): %v", err)
	}
	defer func() {
		if err := m.Close(); err != nil {
			t.Errorf("Map Close(): %v", err)
		}
		if err := closeRuntimeForTest(runtime); err != nil {
			t.Errorf("Runtime Close(): %v", err)
		}
	}()

	var firstCalls, secondCalls atomic.Int64
	if _, err := runtime.SetResourceProvider(countingResourceProvider(&firstCalls)); err != nil {
		t.Fatalf("SetResourceProvider(): %v", err)
	}
	loadProbeStyle(t, runtime, m, "jar:file:/packaged/first.json")
	if got := firstCalls.Load(); got == 0 {
		t.Fatalf("installed provider calls = %d, want at least 1", got)
	}

	if _, err := runtime.SetResourceProvider(countingResourceProvider(&secondCalls)); err != nil {
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

// BND-155: a style URL using the default tile server's maplibre: scheme alias
// reaches the provider as the alias, alongside the HTTPS URL the built-in
// network path would have fetched.
func TestResourceProviderSeesSchemeAliasAndItsResolvedURL(t *testing.T) {
	const emptyStyle = `{"version":8,"sources":{},"layers":[]}`
	var resolvedURL atomic.Value

	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	if _, err := runtime.SetResourceProvider(func(request ResourceRequest, handle *ResourceRequestHandle) ResourceProviderDecision {
		if request.RequestedURL != "maplibre://maps/style" {
			return ResourceProviderDecisionPassThrough
		}
		resolvedURL.Store(request.ResolvedURL)
		if err := handle.Complete(ResourceResponse{Status: ResourceResponseStatusOK, Bytes: []byte(emptyStyle)}); err != nil {
			return ResourceProviderDecisionPassThrough
		}
		return ResourceProviderDecisionHandle
	}); err != nil {
		_ = closeRuntimeForTest(runtime)
		t.Fatalf("SetResourceProvider(): %v", err)
	}
	m, err := awaitForTest(runtime.NewMap())
	if err != nil {
		_ = closeRuntimeForTest(runtime)
		t.Fatalf("NewMap(): %v", err)
	}
	defer func() {
		if err := m.Close(); err != nil {
			t.Errorf("Map Close(): %v", err)
		}
		if err := closeRuntimeForTest(runtime); err != nil {
			t.Errorf("Runtime Close(): %v", err)
		}
	}()

	if _, err := m.SetStyleURL("maplibre://maps/style"); err != nil {
		t.Fatalf("SetStyleURL(): %v", err)
	}
	waitForRuntimeEvent(t, runtime, RuntimeEventMapStyleLoaded)

	if got := resolvedURL.Load(); got != "https://demotiles.maplibre.org/style.json" {
		t.Fatalf("resolved URL = %v, want https://demotiles.maplibre.org/style.json", got)
	}
}

func TestResourceResponseRejectsEmbeddedNULStrings(t *testing.T) {
	if err := validateResourceResponse(ResourceResponse{ErrorMessage: "bad\x00tail"}); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("ErrorMessage embedded NUL error = %v, want ErrInvalidArgument", err)
	}
	if err := validateResourceResponse(ResourceResponse{ETag: "etag\x00tail"}); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("ETag embedded NUL error = %v, want ErrInvalidArgument", err)
	}
	if err := validateResourceResponse(ResourceResponse{ErrorMessage: "bad", ETag: "etag"}); err != nil {
		t.Fatalf("valid resource response error = %v", err)
	}
}

func TestResourceResponseAllowsUnknownEnumsForNativeValidation(t *testing.T) {
	if err := validateResourceResponse(ResourceResponse{Status: ResourceResponseStatus(99)}); err != nil {
		t.Fatalf("unknown status error = %v, want nil", err)
	}
	if err := validateResourceResponse(ResourceResponse{Status: ResourceResponseStatusOK, ErrorReason: ResourceErrorReason(99)}); err != nil {
		t.Fatalf("unknown error reason error = %v, want nil", err)
	}
}

func TestResourceResponseAcceptsKnownEnums(t *testing.T) {
	statuses := []ResourceResponseStatus{
		ResourceResponseStatusOK,
		ResourceResponseStatusError,
		ResourceResponseStatusNoContent,
		ResourceResponseStatusNotModified,
	}
	reasons := []ResourceErrorReason{
		ResourceErrorReasonNone,
		ResourceErrorReasonNotFound,
		ResourceErrorReasonServer,
		ResourceErrorReasonConnection,
		ResourceErrorReasonRateLimit,
		ResourceErrorReasonOther,
	}
	for _, status := range statuses {
		for _, reason := range reasons {
			if err := validateResourceResponse(ResourceResponse{Status: status, ErrorReason: reason}); err != nil {
				t.Fatalf("validateResourceResponse(%v, %v) error = %v", status, reason, err)
			}
		}
	}
}

func TestRuntimeResourceProviderRejectsNilCallback(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	defer func() {
		if err := closeRuntimeForTest(runtime); err != nil {
			t.Errorf("Close(): %v", err)
		}
	}()

	if _, err := runtime.SetResourceProvider(nil); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetResourceProvider(nil) error = %v, want ErrInvalidArgument", err)
	}
}

func TestRuntimeResourceTransformLifecycle(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	if _, err := runtime.SetResourceTransform(func(request ResourceTransformRequest) (string, bool) {
		return request.URL + "?first", true
	}); err != nil {
		_ = closeRuntimeForTest(runtime)
		t.Fatalf("SetResourceTransform(): %v", err)
	}
	if _, err := runtime.SetResourceTransform(func(request ResourceTransformRequest) (string, bool) {
		return "", false
	}); err != nil {
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
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	defer func() {
		if err := closeRuntimeForTest(runtime); err != nil {
			t.Errorf("Close(): %v", err)
		}
	}()

	if _, err := runtime.SetResourceTransform(nil); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetResourceTransform(nil) error = %v, want ErrInvalidArgument", err)
	}
}

// BND-198: a request the provider handled but never completed reports one
// cancellation when the map that asked for it goes away, the callback can
// close that request from inside itself, and a second registration on the same
// request reports invalid state.
func TestResourceRequestCancelCallbackReportsDiscardedRequest(t *testing.T) {
	const styleURL = "jar:file:/packaged/cancelled-style.json"
	requested := make(chan struct{}, 1)
	cancelled := make(chan struct{}, 4)
	var cancelCalls atomic.Int64
	var registerErr, secondRegisterErr atomic.Value

	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	defer func() {
		if err := closeRuntimeForTest(runtime); err != nil {
			t.Errorf("Runtime Close(): %v", err)
		}
	}()
	if _, err := runtime.SetResourceProvider(func(request ResourceRequest, handle *ResourceRequestHandle) ResourceProviderDecision {
		if request.RequestedURL != styleURL {
			return ResourceProviderDecisionPassThrough
		}
		if err := handle.SetCancelCallback(func() {
			cancelCalls.Add(1)
			handle.Close()
			cancelled <- struct{}{}
		}); err != nil {
			registerErr.Store(err)
		}
		if err := handle.SetCancelCallback(func() { cancelCalls.Add(1) }); err != nil {
			secondRegisterErr.Store(err)
		}
		select {
		case requested <- struct{}{}:
		default:
		}
		// The request stays open, so only cancellation retires it.
		return ResourceProviderDecisionHandle
	}); err != nil {
		_ = closeRuntimeForTest(runtime)
		t.Fatalf("SetResourceProvider(): %v", err)
	}

	m, err := awaitForTest(runtime.NewMap())
	if err != nil {
		t.Fatalf("NewMap(): %v", err)
	}
	if _, err := m.SetStyleURL(styleURL); err != nil {
		t.Fatalf("SetStyleURL(): %v", err)
	}
	waitForResourceSignal(t, requested, "the provider to receive the style request")
	// Map teardown discards the request the provider never completed, and the
	// cancel callback runs on the thread that discards it.
	teardown, err := m.CloseAsync()
	if err != nil {
		t.Fatalf("Map CloseAsync(): %v", err)
	}
	waitForResourceSignal(t, cancelled, "the cancel callback to run")
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
}

// BND-198: registering on a request MapLibre already cancelled runs the
// callback before SetCancelCallback returns, and a closed request rejects
// registration as closed.
func TestResourceRequestCancelCallbackRunsForAlreadyCancelledRequest(t *testing.T) {
	const styleURL = "jar:file:/packaged/late-cancel-style.json"
	handles := make(chan *ResourceRequestHandle, 1)

	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	defer func() {
		if err := closeRuntimeForTest(runtime); err != nil {
			t.Errorf("Runtime Close(): %v", err)
		}
	}()
	if _, err := runtime.SetResourceProvider(func(request ResourceRequest, handle *ResourceRequestHandle) ResourceProviderDecision {
		if request.RequestedURL != styleURL {
			return ResourceProviderDecisionPassThrough
		}
		select {
		case handles <- handle:
		default:
		}
		return ResourceProviderDecisionHandle
	}); err != nil {
		_ = closeRuntimeForTest(runtime)
		t.Fatalf("SetResourceProvider(): %v", err)
	}

	m, err := awaitForTest(runtime.NewMap())
	if err != nil {
		t.Fatalf("NewMap(): %v", err)
	}
	if _, err := m.SetStyleURL(styleURL); err != nil {
		t.Fatalf("SetStyleURL(): %v", err)
	}
	handle := waitForResourceSignalValue(t, handles, "the provider to receive the style request")
	if err := m.Close(); err != nil {
		t.Fatalf("Map Close(): %v", err)
	}
	waitForResourceRequestCancelled(t, handle)

	var calls int
	if err := handle.SetCancelCallback(func() { calls++ }); err != nil {
		t.Fatalf("SetCancelCallback() on a cancelled request: %v", err)
	}
	if calls != 1 {
		t.Fatalf("cancel callback calls = %d, want 1 before SetCancelCallback returned", calls)
	}

	handle.Close()
	if err := handle.SetCancelCallback(func() { calls++ }); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetCancelCallback() after Close error = %v, want ErrInvalidArgument", err)
	}
	if calls != 1 {
		t.Fatalf("cancel callback calls after Close = %d, want 1", calls)
	}
}

// BND-198: a request the provider completed is not reported as cancelled, even
// once the map that asked for it goes away.
func TestResourceRequestCancelCallbackSkipsCompletedRequest(t *testing.T) {
	const styleURL = "jar:file:/packaged/completed-style.json"
	var cancelCalls atomic.Int64
	var providerErr atomic.Value

	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	defer func() {
		if err := closeRuntimeForTest(runtime); err != nil {
			t.Errorf("Runtime Close(): %v", err)
		}
	}()
	if _, err := runtime.SetResourceProvider(func(request ResourceRequest, handle *ResourceRequestHandle) ResourceProviderDecision {
		if request.RequestedURL != styleURL {
			return ResourceProviderDecisionPassThrough
		}
		if err := handle.SetCancelCallback(func() { cancelCalls.Add(1) }); err != nil {
			providerErr.Store(err)
		}
		if err := handle.Complete(ResourceResponse{
			Status: ResourceResponseStatusOK,
			Bytes:  []byte(minimalStyleJSON),
		}); err != nil {
			providerErr.Store(err)
		}
		return ResourceProviderDecisionHandle
	}); err != nil {
		_ = closeRuntimeForTest(runtime)
		t.Fatalf("SetResourceProvider(): %v", err)
	}

	m, err := awaitForTest(runtime.NewMap())
	if err != nil {
		t.Fatalf("NewMap(): %v", err)
	}
	if _, err := m.SetStyleURL(styleURL); err != nil {
		t.Fatalf("SetStyleURL(): %v", err)
	}
	waitForRuntimeEvent(t, runtime, RuntimeEventMapStyleLoaded)
	teardown, err := m.CloseAsync()
	if err != nil {
		t.Fatalf("Map CloseAsync(): %v", err)
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
}

// waitForResourceSignal waits for a signal that native code raises from a
// MapLibre thread.
func waitForResourceSignal(t *testing.T, signal <-chan struct{}, what string) {
	t.Helper()
	waitForResourceSignalValue(t, signal, what)
}

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
