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

// loadProbeStyle points the map at a style URL that no file source serves, so the
// failure event naming that URL proves the request reached the network file
// source after any installed provider passed it through.
func loadProbeStyle(t *testing.T, runtime *RuntimeHandle, m *MapHandle, styleURL string) {
	t.Helper()
	if err := m.SetStyleURL(styleURL); err != nil {
		t.Fatalf("SetStyleURL(%q): %v", styleURL, err)
	}
	for range make([]struct{}, 5000) {
		if err := runtime.RunOnce(); err != nil {
			t.Fatalf("RunOnce(): %v", err)
		}
		event, err := runtime.PollEvent()
		if err != nil {
			t.Fatalf("PollEvent(): %v", err)
		}
		if event != nil && event.Type == RuntimeEventMapLoadingFailed && strings.Contains(event.Message, styleURL) {
			return
		}
		time.Sleep(time.Millisecond)
	}
	t.Fatalf("timed out waiting for a map loading failure naming %q", styleURL)
}

// This covers the runtime-scoped provider lifecycle end to end: an installed
// provider is consulted, a replacement takes over while a map is live, and a
// cleared provider stops being consulted while requests keep reaching the
// network file source.
func TestRuntimeResourceProviderInstallsReplacesAndClears(t *testing.T) {
	lockOSThreadForTest(t)

	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	m, err := runtime.NewMap()
	if err != nil {
		_ = runtime.Close()
		t.Fatalf("NewMap(): %v", err)
	}
	defer func() {
		if err := m.Close(); err != nil {
			t.Errorf("Map Close(): %v", err)
		}
		if err := runtime.Close(); err != nil {
			t.Errorf("Runtime Close(): %v", err)
		}
	}()

	var firstCalls, secondCalls atomic.Int64
	if err := runtime.SetResourceProvider(countingResourceProvider(&firstCalls)); err != nil {
		t.Fatalf("SetResourceProvider(): %v", err)
	}
	loadProbeStyle(t, runtime, m, "jar:file:/packaged/first.json")
	if got := firstCalls.Load(); got == 0 {
		t.Fatalf("installed provider calls = %d, want at least 1", got)
	}

	// Replacing the provider while a map is live is part of the contract.
	if err := runtime.SetResourceProvider(countingResourceProvider(&secondCalls)); err != nil {
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

	if err := runtime.ClearResourceProvider(); err != nil {
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

	// Clearing an already cleared provider stays a successful no-op.
	if err := runtime.ClearResourceProvider(); err != nil {
		t.Fatalf("second ClearResourceProvider(): %v", err)
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
	lockOSThreadForTest(t)

	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	defer func() {
		if err := runtime.Close(); err != nil {
			t.Errorf("Close(): %v", err)
		}
	}()

	if err := runtime.SetResourceProvider(nil); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetResourceProvider(nil) error = %v, want ErrInvalidArgument", err)
	}
}

func TestRuntimeResourceTransformLifecycle(t *testing.T) {
	lockOSThreadForTest(t)

	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	if err := runtime.SetResourceTransform(func(request ResourceTransformRequest) (string, bool) {
		return request.URL + "?first", true
	}); err != nil {
		_ = runtime.Close()
		t.Fatalf("SetResourceTransform(): %v", err)
	}
	if err := runtime.SetResourceTransform(func(request ResourceTransformRequest) (string, bool) {
		return "", false
	}); err != nil {
		_ = runtime.Close()
		t.Fatalf("SetResourceTransform(replace): %v", err)
	}
	if err := runtime.ClearResourceTransform(); err != nil {
		_ = runtime.Close()
		t.Fatalf("ClearResourceTransform(): %v", err)
	}
	if err := runtime.ClearResourceTransform(); err != nil {
		_ = runtime.Close()
		t.Fatalf("second ClearResourceTransform(): %v", err)
	}
	if err := runtime.Close(); err != nil {
		t.Fatalf("Close(): %v", err)
	}
}

func TestRuntimeResourceTransformRejectsNilCallback(t *testing.T) {
	lockOSThreadForTest(t)

	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	defer func() {
		if err := runtime.Close(); err != nil {
			t.Errorf("Close(): %v", err)
		}
	}()

	if err := runtime.SetResourceTransform(nil); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetResourceTransform(nil) error = %v, want ErrInvalidArgument", err)
	}
}
