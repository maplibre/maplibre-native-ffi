package maplibre

import (
	"errors"
	"net/http"
	"net/http/httptest"
	"runtime"
	"testing"
)

func TestGeneratedCallbacksEnforceResponseScopeAndAllowedReentry(t *testing.T) {
	host, m := newRuntimeAndMap(t, nil)
	server := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")
		_, _ = w.Write([]byte(emptyStyleJSON))
	}))
	defer server.Close()
	responses := make(chan *ResourceTransformResponseScope, 1)
	failures := make(chan error, 4)
	transform := ResourceTransform{Callback: func(_ ResourceKind, url string, response *ResourceTransformResponseScope) Status {
		if _, err := host.Close(); !errors.Is(err, ErrInvalidState) {
			failures <- err
		}
		wrongThread := make(chan error, 1)
		go func() { wrongThread <- response.SetURL(url) }()
		if err := <-wrongThread; !errors.Is(err, ErrInvalidState) {
			failures <- err
		}
		responses <- response
		if err := response.SetURL(server.URL); err != nil {
			failures <- err
			return StatusInvalidState
		}
		return StatusOK
	}}
	if _, err := awaitForTest(host.SetResourceTransform(transform)); err != nil {
		t.Fatal(err)
	}
	if _, err := m.SetStyleURL("https://example.test/generated-scope.json"); err != nil {
		t.Fatal(err)
	}
	waitForRuntimeEvent(t, host, RuntimeEventTypeMapStyleLoaded)
	response := <-responses
	if err := response.SetURL("expired"); !errors.Is(err, ErrInvalidState) {
		t.Fatalf("expired response: %v", err)
	}
	if _, err := awaitForTest(host.ClearResourceTransform()); err != nil {
		t.Fatal(err)
	}
	retired := make(chan struct{}, 1)
	provider := ResourceProvider{Callback: func(request ResourceRequest, handle *ResourceRequestHandle) ResourceProviderDecision {
		if _, err := host.Close(); !errors.Is(err, ErrInvalidState) {
			failures <- err
		}
		if err := handle.Complete(ResourceResponse{Status: ResourceResponseStatusOK, Bytes: []byte(emptyStyleJSON)}); err != nil {
			failures <- err
		}
		dispose := handle.state.dispose
		handle.state.dispose = func(raw uint64) { dispose(raw); retired <- struct{}{} }
		for range 2 {
			if err := handle.Close(); err != nil {
				failures <- err
			}
		}
		// Accepted completion claims the decision even when host code returns pass-through.
		return ResourceProviderDecisionPassThrough
	}}
	if _, err := awaitForTest(host.SetResourceProvider(provider)); err != nil {
		t.Fatal(err)
	}
	if _, err := m.SetStyleURL("generated-provider://style.json"); err != nil {
		t.Fatal(err)
	}
	waitForRuntimeEvent(t, host, RuntimeEventTypeMapStyleLoaded)
	waitForResourceSignalValue(t, retired, "inline request retirement")
	provider = ResourceProvider{Callback: func(_ ResourceRequest, handle *ResourceRequestHandle) ResourceProviderDecision {
		dispose := handle.state.dispose
		handle.state.dispose = func(raw uint64) { dispose(raw); retired <- struct{}{} }
		for range 2 {
			if err := handle.Close(); err != nil {
				failures <- err
			}
		}
		return ResourceProviderDecisionPassThrough
	}}
	if _, err := awaitForTest(host.SetResourceProvider(provider)); err != nil {
		t.Fatal(err)
	}
	if _, err := m.SetStyleURL(server.URL + "/closed-provider"); err != nil {
		t.Fatal(err)
	}
	waitForResourceSignalValue(t, retired, "closed provider decision retirement")
	select {
	case err := <-failures:
		t.Fatalf("callback contract: %v", err)
	default:
	}
}

func discardCallbackCycle(t *testing.T) <-chan struct{} {
	t.Helper()
	host, err := RuntimeCreate(DefaultRuntimeOptions())
	if err != nil {
		t.Fatal(err)
	}
	disposed := make(chan struct{})
	original := host.state.dispose
	host.state.dispose = func(raw uint64) { original(raw); close(disposed) }
	provider := ResourceProvider{Callback: func(ResourceRequest, *ResourceRequestHandle) ResourceProviderDecision {
		// This capture makes a cycle through the registration, its owner, and its closure.
		runtime.KeepAlive(host)
		return ResourceProviderDecisionPassThrough
	}}
	if _, err := awaitForTest(host.SetResourceProvider(provider)); err != nil {
		t.Fatal(err)
	}
	return disposed
}

func TestGeneratedCallbackOwnerCycleIsCollectible(t *testing.T) {
	disposed := discardCallbackCycle(t)
	waitForCompletionCollection(t, disposed)
}
