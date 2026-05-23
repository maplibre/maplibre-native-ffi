package maplibre

import (
	"errors"
	stdruntime "runtime"
	"testing"
)

func TestCVersionUsesNativeABI(t *testing.T) {
	if got := CVersion(); got != 0 {
		t.Fatalf("CVersion() = %d, want 0 while ABI is unstable", got)
	}
}

func TestSupportedRenderBackendsUsesNativeABIConstants(t *testing.T) {
	mask := SupportedRenderBackends()
	if mask == 0 {
		t.Fatal("SupportedRenderBackends() returned empty mask")
	}
	if mask.Has(RenderBackendMetal) && uint32(RenderBackendMetal) == 0 {
		t.Fatal("RenderBackendMetal has zero ABI value")
	}
	if mask.Has(RenderBackendVulkan) && uint32(RenderBackendVulkan) == 0 {
		t.Fatal("RenderBackendVulkan has zero ABI value")
	}
}

func TestNativePointerIsOpaqueValue(t *testing.T) {
	var pointer NativePointer = 0x1234
	if uintptr(pointer) != 0x1234 {
		t.Fatalf("NativePointer preserved address value %x", uintptr(pointer))
	}
}

func TestNetworkStatusRoundTripsThroughNativeABI(t *testing.T) {
	original, err := CurrentNetworkStatus()
	if err != nil {
		t.Fatalf("CurrentNetworkStatus() original: %v", err)
	}
	t.Cleanup(func() {
		if err := networkStatusSetRaw(uint32(original)); err != nil {
			t.Fatalf("restore network status: %v", err)
		}
	})

	if err := SetNetworkStatus(NetworkStatusOffline); err != nil {
		t.Fatalf("SetNetworkStatus(offline): %v", err)
	}
	if got, err := CurrentNetworkStatus(); err != nil || got != NetworkStatusOffline {
		t.Fatalf("CurrentNetworkStatus() = %v, %v; want offline, nil", got, err)
	}

	if err := SetNetworkStatus(NetworkStatusOnline); err != nil {
		t.Fatalf("SetNetworkStatus(online): %v", err)
	}
	if got, err := CurrentNetworkStatus(); err != nil || got != NetworkStatusOnline {
		t.Fatalf("CurrentNetworkStatus() = %v, %v; want online, nil", got, err)
	}
}

func TestLoggingConfigurationUsesNativeABI(t *testing.T) {
	if err := SetAsyncLogSeverityMask(LogSeverityMaskDefault); err != nil {
		t.Fatalf("SetAsyncLogSeverityMask(default): %v", err)
	}
	if err := SetAsyncLogSeverityMask(LogSeverityMask(1 << 31)); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetAsyncLogSeverityMask(invalid) error = %v, want ErrInvalidArgument", err)
	}
	if err := SetLogCallback(func(LogRecord) bool { return false }); err != nil {
		t.Fatalf("SetLogCallback(): %v", err)
	}
	if err := SetLogCallback(func(LogRecord) bool { return true }); err != nil {
		_ = ClearLogCallback()
		t.Fatalf("SetLogCallback(replace): %v", err)
	}
	if err := ClearLogCallback(); err != nil {
		t.Fatalf("ClearLogCallback(): %v", err)
	}
	if err := ClearLogCallback(); err != nil {
		t.Fatalf("second ClearLogCallback(): %v", err)
	}
}

func TestProjectedMetersHelpersRoundTrip(t *testing.T) {
	coordinate := LatLng{Latitude: 45, Longitude: -122}
	meters, err := ProjectedMetersForLatLng(coordinate)
	if err != nil {
		t.Fatalf("ProjectedMetersForLatLng(): %v", err)
	}
	roundTripped, err := LatLngForProjectedMeters(meters)
	if err != nil {
		t.Fatalf("LatLngForProjectedMeters(): %v", err)
	}
	if diff := roundTripped.Latitude - coordinate.Latitude; diff < -1e-9 || diff > 1e-9 {
		t.Fatalf("latitude round trip = %f, want %f", roundTripped.Latitude, coordinate.Latitude)
	}
	if diff := roundTripped.Longitude - coordinate.Longitude; diff < -1e-9 || diff > 1e-9 {
		t.Fatalf("longitude round trip = %f, want %f", roundTripped.Longitude, coordinate.Longitude)
	}
}

func TestRuntimeCreateWithOptions(t *testing.T) {
	runtime, err := NewRuntimeWithOptions(RuntimeOptions{CachePath: ":memory:"}.WithMaximumCacheSize(0))
	if err != nil {
		t.Fatalf("NewRuntimeWithOptions(): %v", err)
	}
	if err := runtime.Close(); err != nil {
		t.Fatalf("Close(): %v", err)
	}
}

func TestRuntimeOptionsRejectEmbeddedNUL(t *testing.T) {
	_, err := NewRuntimeWithOptions(RuntimeOptions{AssetPath: "asset\x00root"})
	if !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("NewRuntimeWithOptions embedded NUL error = %v, want ErrInvalidArgument", err)
	}
}

func TestRuntimeCreateRunOnceAndClose(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	if err := runtime.RunOnce(); err != nil {
		t.Fatalf("RunOnce(): %v", err)
	}
	if event, err := runtime.PollEvent(); err != nil {
		t.Fatalf("PollEvent(): %v", err)
	} else if event != nil && event.PayloadSize > 0 && event.PayloadType == RuntimeEventPayloadNone {
		t.Fatalf("PollEvent() payload metadata inconsistent: %#v", event)
	}
	if err := runtime.Close(); err != nil {
		t.Fatalf("Close(): %v", err)
	}
	if err := runtime.Close(); err != nil {
		t.Fatalf("second Close(): %v", err)
	}
	if _, err := runtime.NewMap(); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("NewMap() after Close error = %v, want ErrInvalidArgument", err)
	}
}

func TestRuntimeMapLifecycle(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}

	m, err := runtime.NewMap()
	if err != nil {
		_ = runtime.Close()
		t.Fatalf("NewMap(): %v", err)
	}
	if err := runtime.Close(); !errors.Is(err, ErrInvalidState) {
		_ = m.Close()
		_ = runtime.Close()
		t.Fatalf("Close() with live map error = %v, want ErrInvalidState", err)
	}
	if err := m.Close(); err != nil {
		_ = runtime.Close()
		t.Fatalf("Map Close(): %v", err)
	}
	if err := m.Close(); err != nil {
		_ = runtime.Close()
		t.Fatalf("second Map Close(): %v", err)
	}
	if err := runtime.Close(); err != nil {
		t.Fatalf("Runtime Close(): %v", err)
	}
}

func TestMapProjectionSnapshotOutlivesMap(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	m, err := runtime.NewMapWithOptions(NewMapOptions(512, 512, 1))
	if err != nil {
		_ = runtime.Close()
		t.Fatalf("NewMapWithOptions(): %v", err)
	}
	projection, err := m.NewProjection()
	if err != nil {
		_ = m.Close()
		_ = runtime.Close()
		t.Fatalf("NewProjection(): %v", err)
	}
	if err := m.Close(); err != nil {
		_ = projection.Close()
		_ = runtime.Close()
		t.Fatalf("Map Close(): %v", err)
	}
	if err := runtime.Close(); err != nil {
		_ = projection.Close()
		t.Fatalf("Runtime Close(): %v", err)
	}

	coordinate := LatLng{Latitude: 0, Longitude: 0}
	point, err := projection.PixelForLatLng(coordinate)
	if err != nil {
		_ = projection.Close()
		t.Fatalf("PixelForLatLng(): %v", err)
	}
	roundTripped, err := projection.LatLngForPixel(point)
	if err != nil {
		_ = projection.Close()
		t.Fatalf("LatLngForPixel(): %v", err)
	}
	if diff := roundTripped.Latitude - coordinate.Latitude; diff < -1e-7 || diff > 1e-7 {
		t.Fatalf("latitude round trip = %f, want %f", roundTripped.Latitude, coordinate.Latitude)
	}
	if diff := roundTripped.Longitude - coordinate.Longitude; diff < -1e-7 || diff > 1e-7 {
		t.Fatalf("longitude round trip = %f, want %f", roundTripped.Longitude, coordinate.Longitude)
	}
	if err := projection.Close(); err != nil {
		t.Fatalf("Projection Close(): %v", err)
	}
	if err := projection.Close(); err != nil {
		t.Fatalf("second Projection Close(): %v", err)
	}
	if _, err := projection.PixelForLatLng(coordinate); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("PixelForLatLng() after close error = %v, want ErrInvalidArgument", err)
	}
}

func TestRuntimeCloseWrongThreadLeavesHandleRetryable(t *testing.T) {
	stdruntime.LockOSThread()
	defer stdruntime.UnlockOSThread()

	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}

	errCh := make(chan error, 1)
	go func() {
		errCh <- runtime.Close()
	}()
	if err := <-errCh; !errors.Is(err, ErrWrongThread) {
		_ = runtime.Close()
		t.Fatalf("Close() from another thread error = %v, want ErrWrongThread", err)
	}
	if err := runtime.RunOnce(); err != nil {
		_ = runtime.Close()
		t.Fatalf("RunOnce() after failed close: %v", err)
	}
	if err := runtime.Close(); err != nil {
		t.Fatalf("Close() on owner thread after failed close: %v", err)
	}
}

func TestInvalidNetworkStatusReportsNativeError(t *testing.T) {
	err := networkStatusSetRaw(999_999)
	if !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("networkStatusSetRaw invalid error = %v, want ErrInvalidArgument", err)
	}

	var nativeErr *Error
	if !errors.As(err, &nativeErr) {
		t.Fatalf("error %T does not expose *Error", err)
	}
	if status, ok := nativeErr.RawStatus(); !ok || status != -1 {
		t.Fatalf("RawStatus() = %d, %v; want -1, true", status, ok)
	}
	if got := nativeErr.Diagnostic(); got == "" {
		t.Fatal("Diagnostic() is empty")
	}
}

func TestUnknownNetworkStatusRejectedBeforeNativeCall(t *testing.T) {
	err := SetNetworkStatus(NetworkStatus(999_999))
	if !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetNetworkStatus unknown error = %v, want ErrInvalidArgument", err)
	}

	var bindingErr *Error
	if !errors.As(err, &bindingErr) {
		t.Fatalf("error %T does not expose *Error", err)
	}
	if _, ok := bindingErr.RawStatus(); ok {
		t.Fatal("RawStatus() reported native status for binding validation error")
	}
}
