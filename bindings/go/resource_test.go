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
		_ = runtime.Close()
		t.Fatalf("SetResourceProvider(): %v", err)
	}
	m, err := awaitForTest(runtime.NewMap())
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
		if err := runtime.Close(); err != nil {
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
		_ = runtime.Close()
		t.Fatalf("SetResourceTransform(): %v", err)
	}
	if _, err := runtime.SetResourceTransform(func(request ResourceTransformRequest) (string, bool) {
		return "", false
	}); err != nil {
		_ = runtime.Close()
		t.Fatalf("SetResourceTransform(replace): %v", err)
	}
	if _, err := runtime.ClearResourceTransform(); err != nil {
		_ = runtime.Close()
		t.Fatalf("ClearResourceTransform(): %v", err)
	}
	if _, err := runtime.ClearResourceTransform(); err != nil {
		_ = runtime.Close()
		t.Fatalf("second ClearResourceTransform(): %v", err)
	}
	if err := runtime.Close(); err != nil {
		t.Fatalf("Close(): %v", err)
	}
}

func TestRuntimeResourceTransformRejectsNilCallback(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	defer func() {
		if err := runtime.Close(); err != nil {
			t.Errorf("Close(): %v", err)
		}
	}()

	if _, err := runtime.SetResourceTransform(nil); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetResourceTransform(nil) error = %v, want ErrInvalidArgument", err)
	}
}
