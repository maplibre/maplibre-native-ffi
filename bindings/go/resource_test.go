package maplibre

import (
	"errors"
	"testing"
)

func TestRuntimeResourceProviderLifecycle(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	if err := runtime.SetResourceProvider(func(ResourceRequest, *ResourceRequestHandle) ResourceProviderDecision {
		return ResourceProviderDecisionPassThrough
	}); err != nil {
		_ = runtime.Close()
		t.Fatalf("SetResourceProvider(): %v", err)
	}
	if err := runtime.SetResourceProvider(func(ResourceRequest, *ResourceRequestHandle) ResourceProviderDecision {
		return ResourceProviderDecisionPassThrough
	}); err != nil {
		_ = runtime.Close()
		t.Fatalf("SetResourceProvider(replace): %v", err)
	}
	if err := runtime.Close(); err != nil {
		t.Fatalf("Close(): %v", err)
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

	if err := runtime.SetResourceProvider(nil); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetResourceProvider(nil) error = %v, want ErrInvalidArgument", err)
	}
}
func TestRuntimeResourceProviderRequiresNoLiveMaps(t *testing.T) {
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

	if err := runtime.SetResourceProvider(func(ResourceRequest, *ResourceRequestHandle) ResourceProviderDecision {
		return ResourceProviderDecisionPassThrough
	}); !errors.Is(err, ErrInvalidState) {
		t.Fatalf("SetResourceProvider() with live map error = %v, want ErrInvalidState", err)
	}
}
func TestRuntimeResourceTransformLifecycle(t *testing.T) {
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
