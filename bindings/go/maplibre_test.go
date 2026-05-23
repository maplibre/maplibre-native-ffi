package maplibre

import (
	"errors"
	stdruntime "runtime"
	"testing"
	"time"
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

func TestRuntimeAmbientCacheOperationDiscard(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	defer func() {
		if err := runtime.Close(); err != nil {
			t.Errorf("Close(): %v", err)
		}
	}()

	operation, err := runtime.StartAmbientCacheOperation(AmbientCacheOperationClear)
	if err != nil {
		t.Fatalf("StartAmbientCacheOperation(): %v", err)
	}
	if operation.ID() == 0 {
		t.Fatal("operation ID is zero")
	}
	if err := operation.Discard(); err != nil {
		t.Fatalf("Discard(): %v", err)
	}
	if err := operation.Discard(); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("second Discard() error = %v, want ErrInvalidArgument", err)
	}
}

func TestRuntimeAmbientCacheOperationRejectsUnknownOperation(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	defer func() {
		if err := runtime.Close(); err != nil {
			t.Errorf("Close(): %v", err)
		}
	}()

	_, err = runtime.StartAmbientCacheOperation(AmbientCacheOperation(999_999))
	if !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("StartAmbientCacheOperation(unknown) error = %v, want ErrInvalidArgument", err)
	}
}

func testOfflineTileDefinition() OfflineTilePyramidRegionDefinition {
	return OfflineTilePyramidRegionDefinition{
		StyleURL: "http://example.com/offline-style.json",
		Bounds: LatLngBounds{
			Southwest: LatLng{Latitude: -1, Longitude: -2},
			Northeast: LatLng{Latitude: 1, Longitude: 2},
		},
		MinZoom:           0,
		MaxZoom:           1,
		PixelRatio:        1,
		IncludeIdeographs: true,
	}
}

func requireDiscardOfflineOperation[T any](t *testing.T, operation *OfflineOperationHandle[T], kind OfflineOperationKind, resultKind OfflineOperationResultKind) {
	t.Helper()
	if operation.ID() == 0 {
		t.Fatal("operation ID is zero")
	}
	if operation.Kind() != kind || operation.ResultKind() != resultKind {
		t.Fatalf("operation kind/result = %v/%v, want %v/%v", operation.Kind(), operation.ResultKind(), kind, resultKind)
	}
	if err := operation.Discard(); err != nil {
		t.Fatalf("Discard(): %v", err)
	}
}

func TestOfflineRegionStartOperationsReturnTypedHandles(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	defer func() {
		if err := runtime.Close(); err != nil {
			t.Errorf("Close(): %v", err)
		}
	}()

	create, err := runtime.StartCreateOfflineRegion(testOfflineTileDefinition(), []byte{1, 2, 3})
	if err != nil {
		t.Fatalf("StartCreateOfflineRegion(): %v", err)
	}
	requireDiscardOfflineOperation(t, create, OfflineOperationRegionCreate, OfflineOperationResultRegion)

	get, err := runtime.StartOfflineRegion(1)
	if err != nil {
		t.Fatalf("StartOfflineRegion(): %v", err)
	}
	requireDiscardOfflineOperation(t, get, OfflineOperationRegionGet, OfflineOperationResultOptionalRegion)

	list, err := runtime.StartOfflineRegions()
	if err != nil {
		t.Fatalf("StartOfflineRegions(): %v", err)
	}
	requireDiscardOfflineOperation(t, list, OfflineOperationRegionsList, OfflineOperationResultRegionList)

	update, err := runtime.StartUpdateOfflineRegionMetadata(1, []byte{4, 5, 6})
	if err != nil {
		t.Fatalf("StartUpdateOfflineRegionMetadata(): %v", err)
	}
	requireDiscardOfflineOperation(t, update, OfflineOperationRegionUpdateMetadata, OfflineOperationResultRegion)

	status, err := runtime.StartOfflineRegionStatus(1)
	if err != nil {
		t.Fatalf("StartOfflineRegionStatus(): %v", err)
	}
	requireDiscardOfflineOperation(t, status, OfflineOperationRegionGetStatus, OfflineOperationResultRegionStatus)

	observed, err := runtime.StartSetOfflineRegionObserved(1, true)
	if err != nil {
		t.Fatalf("StartSetOfflineRegionObserved(): %v", err)
	}
	requireDiscardOfflineOperation(t, observed, OfflineOperationRegionSetObserved, OfflineOperationResultNone)

	download, err := runtime.StartSetOfflineRegionDownloadState(1, OfflineRegionDownloadInactive)
	if err != nil {
		t.Fatalf("StartSetOfflineRegionDownloadState(): %v", err)
	}
	requireDiscardOfflineOperation(t, download, OfflineOperationRegionSetDownloadState, OfflineOperationResultNone)

	invalidate, err := runtime.StartInvalidateOfflineRegion(1)
	if err != nil {
		t.Fatalf("StartInvalidateOfflineRegion(): %v", err)
	}
	requireDiscardOfflineOperation(t, invalidate, OfflineOperationRegionInvalidate, OfflineOperationResultNone)

	deleteOperation, err := runtime.StartDeleteOfflineRegion(1)
	if err != nil {
		t.Fatalf("StartDeleteOfflineRegion(): %v", err)
	}
	requireDiscardOfflineOperation(t, deleteOperation, OfflineOperationRegionDelete, OfflineOperationResultNone)
}

func waitTakeOfflineOperation[T any](t *testing.T, runtime *RuntimeHandle, operation *OfflineOperationHandle[T]) T {
	t.Helper()
	for range make([]struct{}, 5000) {
		if err := runtime.RunOnce(); err != nil {
			t.Fatalf("RunOnce(): %v", err)
		}
		result, err := operation.Take()
		if err == nil {
			return result
		}
		if !errors.Is(err, ErrInvalidState) {
			t.Fatalf("Take(): %v", err)
		}
		time.Sleep(time.Millisecond)
	}
	t.Fatal("offline operation did not complete")
	var zero T
	return zero
}

func TestOfflineCreateAndListTakeResultsCopyNativeData(t *testing.T) {
	stdruntime.LockOSThread()
	defer stdruntime.UnlockOSThread()

	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	defer func() {
		if err := runtime.Close(); err != nil {
			t.Errorf("Close(): %v", err)
		}
	}()

	metadata := []byte{9, 8, 7}
	create, err := runtime.StartCreateOfflineRegion(testOfflineTileDefinition(), metadata)
	if err != nil {
		t.Fatalf("StartCreateOfflineRegion(): %v", err)
	}
	info := waitTakeOfflineOperation(t, runtime, create)
	if info.ID == 0 {
		t.Fatal("created offline region ID is zero")
	}
	if !errors.Is(create.Discard(), ErrInvalidArgument) {
		t.Fatal("Discard() after Take did not report closed operation")
	}
	if got := info.Metadata; len(got) != len(metadata) || got[0] != metadata[0] || got[1] != metadata[1] || got[2] != metadata[2] {
		t.Fatalf("metadata = %v, want %v", got, metadata)
	}
	tile, ok := info.Definition.(OfflineTilePyramidRegionDefinition)
	if !ok {
		t.Fatalf("definition = %T, want OfflineTilePyramidRegionDefinition", info.Definition)
	}
	if tile.StyleURL != testOfflineTileDefinition().StyleURL {
		t.Fatalf("StyleURL = %q, want %q", tile.StyleURL, testOfflineTileDefinition().StyleURL)
	}

	list, err := runtime.StartOfflineRegions()
	if err != nil {
		t.Fatalf("StartOfflineRegions(): %v", err)
	}
	regions := waitTakeOfflineOperation(t, runtime, list)
	if len(regions) == 0 {
		t.Fatal("offline region list is empty after creating a region")
	}
}

func TestOfflineOperationCompletedEventCopiesPayload(t *testing.T) {
	stdruntime.LockOSThread()
	defer stdruntime.UnlockOSThread()

	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	defer func() {
		if err := runtime.Close(); err != nil {
			t.Errorf("Close(): %v", err)
		}
	}()

	operation, err := runtime.StartOfflineRegions()
	if err != nil {
		t.Fatalf("StartOfflineRegions(): %v", err)
	}
	for range make([]struct{}, 5000) {
		if err := runtime.RunOnce(); err != nil {
			t.Fatalf("RunOnce(): %v", err)
		}
		event, err := runtime.PollEvent()
		if err != nil {
			t.Fatalf("PollEvent(): %v", err)
		}
		if event == nil {
			time.Sleep(time.Millisecond)
			continue
		}
		payload, ok := event.Payload.(RuntimeEventOfflineOperationCompletedPayload)
		if !ok {
			continue
		}
		if payload.OperationID != operation.ID() {
			continue
		}
		if payload.OperationKind != OfflineOperationRegionsList || payload.ResultKind != OfflineOperationResultRegionList || payload.ResultStatus != 0 {
			t.Fatalf("payload = %#v", payload)
		}
		if err := operation.Discard(); err != nil {
			t.Fatalf("Discard(): %v", err)
		}
		return
	}
	t.Fatal("offline completion event was not reported")
}

func TestOfflineRegionStartOperationsValidateGoInputs(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	defer func() {
		if err := runtime.Close(); err != nil {
			t.Errorf("Close(): %v", err)
		}
	}()

	definition := testOfflineTileDefinition()
	definition.StyleURL = "http://example.com/\x00style.json"
	if _, err := runtime.StartCreateOfflineRegion(definition, nil); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("StartCreateOfflineRegion embedded NUL error = %v, want ErrInvalidArgument", err)
	}
	if _, err := runtime.StartMergeOfflineRegionsDatabase("/tmp/\x00side.db"); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("StartMergeOfflineRegionsDatabase embedded NUL error = %v, want ErrInvalidArgument", err)
	}
	if _, err := runtime.StartSetOfflineRegionDownloadState(1, OfflineRegionDownloadState(999_999)); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("StartSetOfflineRegionDownloadState unknown error = %v, want ErrInvalidArgument", err)
	}
}

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

const minimalStyleJSON = `{
  "version": 8,
  "name": "go-binding-style-test",
  "sources": {},
  "layers": [
    {"id":"background","type":"background","paint":{"background-color":"#d8f1ff"}}
  ]
}`

func TestMapCommandsAndStyleLoadingUseNativeABI(t *testing.T) {
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

	if err := m.RequestRepaint(); err != nil {
		t.Fatalf("RequestRepaint(): %v", err)
	}
	if err := m.RequestStillImage(); !errors.Is(err, ErrInvalidState) {
		t.Fatalf("RequestStillImage() on continuous map error = %v, want ErrInvalidState", err)
	}
	if err := m.SetStyleJSON(minimalStyleJSON); err != nil {
		t.Fatalf("SetStyleJSON(): %v", err)
	}
	if err := m.SetStyleURL("http://example.com/style.json"); err != nil {
		t.Fatalf("SetStyleURL(): %v", err)
	}
}

func TestMapDebugAndStatusHelpersUseNativeABI(t *testing.T) {
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

	options := MapDebugTileBorders | MapDebugCollision
	if err := m.SetDebugOptions(options); err != nil {
		t.Fatalf("SetDebugOptions(): %v", err)
	}
	got, err := m.DebugOptions()
	if err != nil {
		t.Fatalf("DebugOptions(): %v", err)
	}
	if !got.Has(options) {
		t.Fatalf("DebugOptions() = %v, want bits %v", got, options)
	}
	if err := m.SetRenderingStatsViewEnabled(true); err != nil {
		t.Fatalf("SetRenderingStatsViewEnabled(true): %v", err)
	}
	if got, err := m.RenderingStatsViewEnabled(); err != nil || !got {
		t.Fatalf("RenderingStatsViewEnabled() = %v, %v; want true, nil", got, err)
	}
	if _, err := m.IsFullyLoaded(); err != nil {
		t.Fatalf("IsFullyLoaded(): %v", err)
	}
	if err := m.DumpDebugLogs(); err != nil {
		t.Fatalf("DumpDebugLogs(): %v", err)
	}
}

func TestMapDebugOptionsRejectUnknownBits(t *testing.T) {
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

	if err := m.SetDebugOptions(MapDebugOptions(1 << 31)); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetDebugOptions(unknown) error = %v, want ErrInvalidArgument", err)
	}
}

func TestMapCameraCommandsUseNativeABI(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	m, err := runtime.NewMapWithOptions(NewMapOptions(512, 512, 1))
	if err != nil {
		_ = runtime.Close()
		t.Fatalf("NewMapWithOptions(): %v", err)
	}
	defer func() {
		if err := m.Close(); err != nil {
			t.Errorf("Map Close(): %v", err)
		}
		if err := runtime.Close(); err != nil {
			t.Errorf("Runtime Close(): %v", err)
		}
	}()

	camera := CameraOptions{}.
		WithCenter(LatLng{Latitude: 10, Longitude: 20}).
		WithZoom(2).
		WithBearing(15).
		WithPitch(20)
	if err := m.JumpTo(camera); err != nil {
		t.Fatalf("JumpTo(): %v", err)
	}
	gotCamera, err := m.Camera()
	if err != nil {
		t.Fatalf("Camera(): %v", err)
	}
	if gotCamera.Center == nil || gotCamera.Zoom == nil {
		t.Fatalf("Camera() missing expected fields: %#v", gotCamera)
	}
	if err := m.MoveBy(ScreenPoint{X: 1, Y: 2}); err != nil {
		t.Fatalf("MoveBy(): %v", err)
	}
	anchor := ScreenPoint{X: 256, Y: 256}
	if err := m.ScaleBy(1.1, &anchor); err != nil {
		t.Fatalf("ScaleBy(): %v", err)
	}
	if err := m.RotateBy(ScreenPoint{X: 100, Y: 100}, ScreenPoint{X: 120, Y: 110}); err != nil {
		t.Fatalf("RotateBy(): %v", err)
	}
	if err := m.PitchBy(1); err != nil {
		t.Fatalf("PitchBy(): %v", err)
	}
	if err := m.CancelTransitions(); err != nil {
		t.Fatalf("CancelTransitions(): %v", err)
	}
}

func TestMapAnimatedCameraCommandsUseOptionalAnimationOptions(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	m, err := runtime.NewMapWithOptions(NewMapOptions(512, 512, 1))
	if err != nil {
		_ = runtime.Close()
		t.Fatalf("NewMapWithOptions(): %v", err)
	}
	defer func() {
		if err := m.CancelTransitions(); err != nil {
			t.Errorf("CancelTransitions(): %v", err)
		}
		if err := m.Close(); err != nil {
			t.Errorf("Map Close(): %v", err)
		}
		if err := runtime.Close(); err != nil {
			t.Errorf("Runtime Close(): %v", err)
		}
	}()

	camera := CameraOptions{}.
		WithCenter(LatLng{Latitude: 1, Longitude: 2}).
		WithZoom(1)
	animation := AnimationOptions{}.
		WithDurationMS(0).
		WithVelocity(1.2).
		WithMinZoom(0).
		WithEasing(UnitBezier{X1: 0.25, Y1: 0.1, X2: 0.25, Y2: 1})
	if err := m.EaseTo(camera, &animation); err != nil {
		t.Fatalf("EaseTo(): %v", err)
	}
	if err := m.FlyTo(camera, nil); err != nil {
		t.Fatalf("FlyTo(nil animation): %v", err)
	}
	if err := m.MoveByAnimated(ScreenPoint{X: 1, Y: 1}, &animation); err != nil {
		t.Fatalf("MoveByAnimated(): %v", err)
	}
	anchor := ScreenPoint{X: 256, Y: 256}
	if err := m.ScaleByAnimated(1.01, &anchor, nil); err != nil {
		t.Fatalf("ScaleByAnimated(nil animation): %v", err)
	}
	if err := m.RotateByAnimated(ScreenPoint{X: 100, Y: 100}, ScreenPoint{X: 110, Y: 100}, &animation); err != nil {
		t.Fatalf("RotateByAnimated(): %v", err)
	}
	if err := m.PitchByAnimated(0.5, &animation); err != nil {
		t.Fatalf("PitchByAnimated(): %v", err)
	}
}

func TestMapCameraFitAndBoundsHelpers(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	m, err := runtime.NewMapWithOptions(NewMapOptions(512, 512, 1))
	if err != nil {
		_ = runtime.Close()
		t.Fatalf("NewMapWithOptions(): %v", err)
	}
	defer func() {
		if err := m.Close(); err != nil {
			t.Errorf("Map Close(): %v", err)
		}
		if err := runtime.Close(); err != nil {
			t.Errorf("Runtime Close(): %v", err)
		}
	}()

	bounds := LatLngBounds{
		Southwest: LatLng{Latitude: -10, Longitude: -20},
		Northeast: LatLng{Latitude: 10, Longitude: 20},
	}
	fitOptions := CameraFitOptions{}.
		WithPadding(EdgeInsets{Top: 4, Left: 3, Bottom: 2, Right: 1}).
		WithBearing(0).
		WithPitch(0)
	camera, err := m.CameraForLatLngBounds(bounds, &fitOptions)
	if err != nil {
		t.Fatalf("CameraForLatLngBounds(): %v", err)
	}
	if camera.Center == nil || camera.Zoom == nil {
		t.Fatalf("CameraForLatLngBounds() missing expected fields: %#v", camera)
	}
	camera, err = m.CameraForLatLngs([]LatLng{bounds.Southwest, bounds.Northeast}, nil)
	if err != nil {
		t.Fatalf("CameraForLatLngs(): %v", err)
	}
	wrapped, err := m.LatLngBoundsForCamera(camera)
	if err != nil {
		t.Fatalf("LatLngBoundsForCamera(): %v", err)
	}
	if wrapped.Southwest.Latitude > wrapped.Northeast.Latitude {
		t.Fatalf("LatLngBoundsForCamera() inverted latitude bounds: %#v", wrapped)
	}
	if _, err := m.LatLngBoundsForCameraUnwrapped(camera); err != nil {
		t.Fatalf("LatLngBoundsForCameraUnwrapped(): %v", err)
	}

	constraints := BoundOptions{}.
		WithBounds(bounds).
		WithMinZoom(0).
		WithMaxZoom(20).
		WithMinPitch(0).
		WithMaxPitch(60)
	if err := m.SetBounds(constraints); err != nil {
		t.Fatalf("SetBounds(): %v", err)
	}
	gotConstraints, err := m.Bounds()
	if err != nil {
		t.Fatalf("Bounds(): %v", err)
	}
	if gotConstraints.Bounds == nil || *gotConstraints.Bounds != bounds {
		t.Fatalf("Bounds().Bounds = %#v, want %#v", gotConstraints.Bounds, bounds)
	}
}

func TestMapFreeCameraOptionsRoundTripCurrentValues(t *testing.T) {
	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	m, err := runtime.NewMapWithOptions(NewMapOptions(512, 512, 1))
	if err != nil {
		_ = runtime.Close()
		t.Fatalf("NewMapWithOptions(): %v", err)
	}
	defer func() {
		if err := m.Close(); err != nil {
			t.Errorf("Map Close(): %v", err)
		}
		if err := runtime.Close(); err != nil {
			t.Errorf("Runtime Close(): %v", err)
		}
	}()

	freeCamera, err := m.FreeCameraOptions()
	if err != nil {
		t.Fatalf("FreeCameraOptions(): %v", err)
	}
	if freeCamera.Position == nil || freeCamera.Orientation == nil {
		t.Fatalf("FreeCameraOptions() missing expected fields: %#v", freeCamera)
	}
	if err := m.SetFreeCameraOptions(FreeCameraOptions{}.
		WithPosition(*freeCamera.Position).
		WithOrientation(*freeCamera.Orientation)); err != nil {
		t.Fatalf("SetFreeCameraOptions(current values): %v", err)
	}
}

func TestMapCameraCommandsReportNativeValidation(t *testing.T) {
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

	if err := m.ScaleBy(0, nil); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("ScaleBy(0) error = %v, want ErrInvalidArgument", err)
	}
	invalidAnimation := AnimationOptions{}.WithDurationMS(-1)
	if err := m.MoveByAnimated(ScreenPoint{}, &invalidAnimation); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("MoveByAnimated(invalid animation) error = %v, want ErrInvalidArgument", err)
	}
	if _, err := m.CameraForLatLngs(nil, nil); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("CameraForLatLngs(nil) error = %v, want ErrInvalidArgument", err)
	}
	invalidBounds := BoundOptions{}.WithMinZoom(3).WithMaxZoom(2)
	if err := m.SetBounds(invalidBounds); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetBounds(invalid min/max) error = %v, want ErrInvalidArgument", err)
	}
	invalidFreeCamera := FreeCameraOptions{}.WithOrientation(Quaternion{})
	if err := m.SetFreeCameraOptions(invalidFreeCamera); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetFreeCameraOptions(invalid orientation) error = %v, want ErrInvalidArgument", err)
	}
}

func TestMapViewportTileAndProjectionOptionsRoundTrip(t *testing.T) {
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

	viewport := ViewportOptions{}.
		WithNorthOrientation(NorthOrientationUp).
		WithConstrainMode(ConstrainModeWidthAndHeight).
		WithViewportMode(ViewportModeDefault).
		WithFrustumOffset(EdgeInsets{Top: 1, Left: 2, Bottom: 3, Right: 4})
	if err := m.SetViewportOptions(viewport); err != nil {
		t.Fatalf("SetViewportOptions(): %v", err)
	}
	gotViewport, err := m.ViewportOptions()
	if err != nil {
		t.Fatalf("ViewportOptions(): %v", err)
	}
	if gotViewport.NorthOrientation == nil || *gotViewport.NorthOrientation != NorthOrientationUp {
		t.Fatalf("ViewportOptions().NorthOrientation = %v", gotViewport.NorthOrientation)
	}
	if gotViewport.FrustumOffset == nil || *gotViewport.FrustumOffset != (EdgeInsets{Top: 1, Left: 2, Bottom: 3, Right: 4}) {
		t.Fatalf("ViewportOptions().FrustumOffset = %#v", gotViewport.FrustumOffset)
	}

	tileOptions := TileOptions{}.
		WithPrefetchZoomDelta(2).
		WithLODMode(TileLODModeDefault)
	if err := m.SetTileOptions(tileOptions); err != nil {
		t.Fatalf("SetTileOptions(): %v", err)
	}
	gotTileOptions, err := m.TileOptions()
	if err != nil {
		t.Fatalf("TileOptions(): %v", err)
	}
	if gotTileOptions.PrefetchZoomDelta == nil || *gotTileOptions.PrefetchZoomDelta != 2 {
		t.Fatalf("TileOptions().PrefetchZoomDelta = %v", gotTileOptions.PrefetchZoomDelta)
	}

	projectionMode := ProjectionModeOptions{}.
		WithAxonometric(true).
		WithSkew(0.5, 0.25)
	if err := m.SetProjectionMode(projectionMode); err != nil {
		t.Fatalf("SetProjectionMode(): %v", err)
	}
	gotProjectionMode, err := m.ProjectionMode()
	if err != nil {
		t.Fatalf("ProjectionMode(): %v", err)
	}
	if gotProjectionMode.Axonometric == nil || !*gotProjectionMode.Axonometric {
		t.Fatalf("ProjectionMode().Axonometric = %v", gotProjectionMode.Axonometric)
	}
}

func TestTileOptionsRejectInvalidPrefetchZoomDelta(t *testing.T) {
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

	if err := m.SetTileOptions(TileOptions{}.WithPrefetchZoomDelta(256)); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetTileOptions(invalid prefetch) error = %v, want ErrInvalidArgument", err)
	}
}

func TestMapStyleStringsRejectEmbeddedNUL(t *testing.T) {
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

	if err := m.SetStyleURL("http://example.com/\x00style.json"); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetStyleURL embedded NUL error = %v, want ErrInvalidArgument", err)
	}
	if err := m.SetStyleJSON("{\x00}"); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetStyleJSON embedded NUL error = %v, want ErrInvalidArgument", err)
	}
}

func TestRenderSessionNilHandleAndInvalidSurfaceDescriptor(t *testing.T) {
	var nilSession *RenderSessionHandle
	if err := nilSession.Close(); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("nil RenderSessionHandle Close() error = %v, want ErrInvalidArgument", err)
	}
	if err := nilSession.RenderUpdate(); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("nil RenderSessionHandle RenderUpdate() error = %v, want ErrInvalidArgument", err)
	}
	if _, err := nilSession.ReadPremultipliedRGBA8Into(nil); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("nil RenderSessionHandle ReadPremultipliedRGBA8Into() error = %v, want ErrInvalidArgument", err)
	}
	var nilMetalFrame *MetalOwnedTextureFrame
	if err := nilMetalFrame.Close(); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("nil MetalOwnedTextureFrame Close() error = %v, want ErrInvalidArgument", err)
	}
	var nilVulkanFrame *VulkanOwnedTextureFrame
	if err := nilVulkanFrame.Close(); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("nil VulkanOwnedTextureFrame Close() error = %v, want ErrInvalidArgument", err)
	}

	runtime, err := NewRuntime()
	if err != nil {
		t.Fatalf("NewRuntime(): %v", err)
	}
	m, err := runtime.NewMapWithOptions(NewMapOptions(64, 64, 1))
	if err != nil {
		_ = runtime.Close()
		t.Fatalf("NewMapWithOptions(): %v", err)
	}
	defer func() {
		if err := m.Close(); err != nil {
			t.Errorf("Map Close(): %v", err)
		}
		if err := runtime.Close(); err != nil {
			t.Errorf("Runtime Close(): %v", err)
		}
	}()

	_, err = m.AttachMetalSurface(MetalSurfaceDescriptor{
		Extent: RenderTargetExtent{Width: 64, Height: 64, ScaleFactor: 1},
	})
	if !errors.Is(err, ErrInvalidArgument) && !errors.Is(err, ErrUnsupported) {
		t.Fatalf("AttachMetalSurface(invalid descriptor) error = %v, want ErrInvalidArgument or ErrUnsupported", err)
	}
	_, err = m.AttachMetalOwnedTexture(MetalOwnedTextureDescriptor{
		Extent: RenderTargetExtent{Width: 64, Height: 64, ScaleFactor: 1},
	})
	if !errors.Is(err, ErrInvalidArgument) && !errors.Is(err, ErrUnsupported) {
		t.Fatalf("AttachMetalOwnedTexture(invalid descriptor) error = %v, want ErrInvalidArgument or ErrUnsupported", err)
	}
	_, err = m.AttachMetalBorrowedTexture(MetalBorrowedTextureDescriptor{
		Extent: RenderTargetExtent{Width: 64, Height: 64, ScaleFactor: 1},
	})
	if !errors.Is(err, ErrInvalidArgument) && !errors.Is(err, ErrUnsupported) {
		t.Fatalf("AttachMetalBorrowedTexture(invalid descriptor) error = %v, want ErrInvalidArgument or ErrUnsupported", err)
	}
	_, err = m.AttachVulkanOwnedTexture(VulkanOwnedTextureDescriptor{
		Extent: RenderTargetExtent{Width: 64, Height: 64, ScaleFactor: 1},
	})
	if !errors.Is(err, ErrInvalidArgument) && !errors.Is(err, ErrUnsupported) {
		t.Fatalf("AttachVulkanOwnedTexture(invalid descriptor) error = %v, want ErrInvalidArgument or ErrUnsupported", err)
	}
	_, err = m.AttachVulkanBorrowedTexture(VulkanBorrowedTextureDescriptor{
		Extent: RenderTargetExtent{Width: 64, Height: 64, ScaleFactor: 1},
	})
	if !errors.Is(err, ErrInvalidArgument) && !errors.Is(err, ErrUnsupported) {
		t.Fatalf("AttachVulkanBorrowedTexture(invalid descriptor) error = %v, want ErrInvalidArgument or ErrUnsupported", err)
	}
}

func TestStyleSourceMetadataForMissingSources(t *testing.T) {
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

	if err := m.SetStyleJSON(`{"version":8,"sources":{},"layers":[]}`); err != nil {
		t.Fatalf("SetStyleJSON(empty style): %v", err)
	}
	ids, err := m.StyleSourceIDs()
	if err != nil {
		t.Fatalf("StyleSourceIDs(): %v", err)
	}
	for _, id := range ids {
		if id == "missing" {
			t.Fatalf("StyleSourceIDs() unexpectedly contains missing source: %v", ids)
		}
	}
	exists, err := m.StyleSourceExists("missing")
	if err != nil {
		t.Fatalf("StyleSourceExists(): %v", err)
	}
	if exists {
		t.Fatalf("StyleSourceExists(missing) = true, want false")
	}
	sourceType, found, err := m.StyleSourceType("missing")
	if err != nil {
		t.Fatalf("StyleSourceType(): %v", err)
	}
	if found || sourceType != StyleSourceTypeUnknown {
		t.Fatalf("StyleSourceType(missing) = (%v, %v), want (unknown, false)", sourceType, found)
	}
	_, found, err = m.StyleSourceInfo("missing")
	if err != nil {
		t.Fatalf("StyleSourceInfo(): %v", err)
	}
	if found {
		t.Fatalf("StyleSourceInfo(missing) found = true, want false")
	}
	attribution, found, err := m.StyleSourceAttribution("missing")
	if err != nil {
		t.Fatalf("StyleSourceAttribution(): %v", err)
	}
	if found || attribution != "" {
		t.Fatalf("StyleSourceAttribution(missing) = (%q, %v), want empty false", attribution, found)
	}
	removed, err := m.RemoveStyleSource("missing")
	if err != nil {
		t.Fatalf("RemoveStyleSource(): %v", err)
	}
	if removed {
		t.Fatalf("RemoveStyleSource(missing) = true, want false")
	}
	if _, err := m.StyleSourceExists(""); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("StyleSourceExists(empty) error = %v, want ErrInvalidArgument", err)
	}
}

func TestStyleSourceURLAndTileBindings(t *testing.T) {
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

	if err := m.SetStyleJSON(`{"version":8,"sources":{},"layers":[]}`); err != nil {
		t.Fatalf("SetStyleJSON(empty style): %v", err)
	}
	if err := m.AddGeoJSONSourceURL("geojson-url", "asset://fixtures/points.geojson"); err != nil {
		t.Fatalf("AddGeoJSONSourceURL(): %v", err)
	}
	if err := m.SetGeoJSONSourceURL("geojson-url", "asset://fixtures/points-2.geojson"); err != nil {
		t.Fatalf("SetGeoJSONSourceURL(): %v", err)
	}
	tileOptions := StyleTileSourceOptions{}.
		WithTileSize(256).
		WithAttribution("unit attribution")
	if err := m.AddVectorSourceTiles("vector-tiles", []string{"https://example.com/vector/{z}/{x}/{y}.pbf"}, &tileOptions); err != nil {
		t.Fatalf("AddVectorSourceTiles(): %v", err)
	}
	if err := m.AddRasterSourceURL("raster-url", "https://example.com/raster.json", &tileOptions); err != nil {
		t.Fatalf("AddRasterSourceURL(): %v", err)
	}
	demOptions := StyleTileSourceOptions{}.
		WithTileSize(512).
		WithRasterEncoding(StyleRasterDEMEncodingTerrarium)
	if err := m.AddRasterDEMSourceTiles("dem-tiles", []string{"https://example.com/dem/{z}/{x}/{y}.png"}, &demOptions); err != nil {
		t.Fatalf("AddRasterDEMSourceTiles(): %v", err)
	}
	checks := map[string]StyleSourceType{
		"geojson-url":  StyleSourceTypeGeoJSON,
		"vector-tiles": StyleSourceTypeVector,
		"raster-url":   StyleSourceTypeRaster,
		"dem-tiles":    StyleSourceTypeRasterDEM,
	}
	for id, wantType := range checks {
		gotType, found, err := m.StyleSourceType(id)
		if err != nil {
			t.Fatalf("StyleSourceType(%s): %v", id, err)
		}
		if !found || gotType != wantType {
			t.Fatalf("StyleSourceType(%s) = (%v, %v), want %v true", id, gotType, found, wantType)
		}
	}
	if err := m.AddVectorSourceTiles("bad-vector", nil, nil); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("AddVectorSourceTiles(nil) error = %v, want ErrInvalidArgument", err)
	}
	if err := m.AddGeoJSONSourceURL("", "asset://fixtures/points.geojson"); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("AddGeoJSONSourceURL(empty id) error = %v, want ErrInvalidArgument", err)
	}
}

func TestGeoJSONSourceDataDescriptors(t *testing.T) {
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

	if err := m.SetStyleJSON(`{"version":8,"sources":{},"layers":[]}`); err != nil {
		t.Fatalf("SetStyleJSON(empty style): %v", err)
	}
	points := []LatLng{{Latitude: 1, Longitude: 2}, {Latitude: 3, Longitude: 4}}
	properties := map[string]any{"name": "before", "rank": int64(7)}
	data := GeoJSONFeatureCollection([]Feature{{
		Geometry:   LineStringGeometry(points),
		Properties: properties,
		Identifier: "feature-1",
	}})
	if err := m.AddGeoJSONSourceData("geojson-data", data); err != nil {
		t.Fatalf("AddGeoJSONSourceData(): %v", err)
	}
	points[0] = LatLng{Latitude: 90, Longitude: 90}
	properties["name"] = "after"
	if err := m.SetGeoJSONSourceData("geojson-data", GeoJSON{Type: GeoJSONTypeGeometry, Geometry: PointGeometry(LatLng{Latitude: 5, Longitude: 6})}); err != nil {
		t.Fatalf("SetGeoJSONSourceData(): %v", err)
	}
	sourceType, found, err := m.StyleSourceType("geojson-data")
	if err != nil {
		t.Fatalf("StyleSourceType(geojson-data): %v", err)
	}
	if !found || sourceType != StyleSourceTypeGeoJSON {
		t.Fatalf("StyleSourceType(geojson-data) = (%v, %v), want GeoJSON true", sourceType, found)
	}
	badID := GeoJSONFeatureCollection([]Feature{{Geometry: PointGeometry(LatLng{}), Identifier: struct{}{}}})
	if err := m.AddGeoJSONSourceData("bad-geojson-data", badID); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("AddGeoJSONSourceData(unsupported id) error = %v, want ErrInvalidArgument", err)
	}
	badGeometry := GeoJSON{Type: GeoJSONTypeGeometry, Geometry: Geometry{Type: GeometryType(999)}}
	if err := m.AddGeoJSONSourceData("bad-geometry", badGeometry); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("AddGeoJSONSourceData(unsupported geometry) error = %v, want ErrInvalidArgument", err)
	}
}

func TestCustomGeometrySourceDescriptors(t *testing.T) {
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

	if err := m.SetStyleJSON(`{"version":8,"sources":{},"layers":[]}`); err != nil {
		t.Fatalf("SetStyleJSON(empty style): %v", err)
	}
	minZoom := 0.0
	maxZoom := 2.0
	tolerance := 0.375
	tileSize := uint32(512)
	buffer := uint32(64)
	clip := true
	wrap := false
	fetches := 0
	cancels := 0
	if err := m.AddCustomGeometrySource("custom", CustomGeometrySourceOptions{
		FetchTile:  func(CanonicalTileID) { fetches++ },
		CancelTile: func(CanonicalTileID) { cancels++ },
		MinZoom:    &minZoom,
		MaxZoom:    &maxZoom,
		Tolerance:  &tolerance,
		TileSize:   &tileSize,
		Buffer:     &buffer,
		Clip:       &clip,
		Wrap:       &wrap,
	}); err != nil {
		t.Fatalf("AddCustomGeometrySource(): %v", err)
	}
	if fetches != 0 || cancels != 0 {
		t.Fatalf("callbacks invoked during registration: fetches=%d cancels=%d", fetches, cancels)
	}
	tileID := CanonicalTileID{Z: 0, X: 0, Y: 0}
	if err := m.SetCustomGeometrySourceTileData("custom", tileID, GeoJSONFeatureCollection(nil)); err != nil {
		t.Fatalf("SetCustomGeometrySourceTileData(): %v", err)
	}
	if err := m.InvalidateCustomGeometrySourceTile("custom", tileID); err != nil {
		t.Fatalf("InvalidateCustomGeometrySourceTile(): %v", err)
	}
	if err := m.InvalidateCustomGeometrySourceRegion("custom", LatLngBounds{Southwest: LatLng{Latitude: -1, Longitude: -1}, Northeast: LatLng{Latitude: 1, Longitude: 1}}); err != nil {
		t.Fatalf("InvalidateCustomGeometrySourceRegion(): %v", err)
	}
	removed, err := m.RemoveStyleSource("custom")
	if err != nil {
		t.Fatalf("RemoveStyleSource(custom): %v", err)
	}
	if !removed {
		t.Fatal("RemoveStyleSource(custom) removed=false, want true")
	}
	if err := m.AddCustomGeometrySource("bad-custom", CustomGeometrySourceOptions{}); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("AddCustomGeometrySource(nil fetch) error = %v, want ErrInvalidArgument", err)
	}
}

func TestAddStyleSourceJSONCopiesGoJSONDescriptor(t *testing.T) {
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

	if err := m.SetStyleJSON(`{"version":8,"sources":{},"layers":[]}`); err != nil {
		t.Fatalf("SetStyleJSON(empty style): %v", err)
	}
	source := map[string]any{
		"type": "geojson",
		"data": map[string]any{
			"type":     "FeatureCollection",
			"features": []any{},
		},
		"attribution": "unit-test",
	}
	if err := m.AddStyleSourceJSON("go-json-source", source); err != nil {
		t.Fatalf("AddStyleSourceJSON(): %v", err)
	}
	source["type"] = "mutated-after-call"
	exists, err := m.StyleSourceExists("go-json-source")
	if err != nil {
		t.Fatalf("StyleSourceExists(): %v", err)
	}
	if !exists {
		t.Fatalf("StyleSourceExists(go-json-source) = false, want true")
	}
	sourceType, found, err := m.StyleSourceType("go-json-source")
	if err != nil {
		t.Fatalf("StyleSourceType(): %v", err)
	}
	if !found || sourceType != StyleSourceTypeGeoJSON {
		t.Fatalf("StyleSourceType(go-json-source) = (%v, %v), want GeoJSON true", sourceType, found)
	}
	info, found, err := m.StyleSourceInfo("go-json-source")
	if err != nil {
		t.Fatalf("StyleSourceInfo(): %v", err)
	}
	if !found || info.IDSize != uint64(len("go-json-source")) {
		t.Fatalf("StyleSourceInfo(go-json-source) = (%#v, %v), want copied ID size", info, found)
	}
	if err := m.AddStyleSourceJSON("bad-json-source", map[string]any{"type": make(chan int)}); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("AddStyleSourceJSON(unsupported Go value) error = %v, want ErrInvalidArgument", err)
	}
}

func TestStyleLayerJSONAndPropertySnapshots(t *testing.T) {
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

	if err := m.SetStyleJSON(`{"version":8,"sources":{},"layers":[]}`); err != nil {
		t.Fatalf("SetStyleJSON(empty style): %v", err)
	}
	if err := m.AddStyleSourceJSON("points", map[string]any{
		"type": "geojson",
		"data": map[string]any{"type": "FeatureCollection", "features": []any{}},
	}); err != nil {
		t.Fatalf("AddStyleSourceJSON(points): %v", err)
	}
	layerJSON := map[string]any{
		"id":     "points-layer",
		"type":   "circle",
		"source": "points",
		"paint":  map[string]any{"circle-radius": float64(2)},
	}
	if err := m.AddStyleLayerJSON(layerJSON, ""); err != nil {
		t.Fatalf("AddStyleLayerJSON(): %v", err)
	}
	layerJSON["type"] = "mutated-after-call"
	layerType, found, err := m.StyleLayerType("points-layer")
	if err != nil {
		t.Fatalf("StyleLayerType(): %v", err)
	}
	if !found || layerType != "circle" {
		t.Fatalf("StyleLayerType(points-layer) = (%q, %v), want circle true", layerType, found)
	}
	copiedLayer, found, err := m.StyleLayerJSON("points-layer")
	if err != nil {
		t.Fatalf("StyleLayerJSON(): %v", err)
	}
	if !found {
		t.Fatalf("StyleLayerJSON(points-layer) found = false, want true")
	}
	copiedLayerObject, ok := copiedLayer.(map[string]any)
	if !ok || copiedLayerObject["type"] != "circle" {
		t.Fatalf("StyleLayerJSON(points-layer) = %#v, want copied circle object", copiedLayer)
	}
	if err := m.SetLayerProperty("points-layer", "circle-radius", float64(5)); err != nil {
		t.Fatalf("SetLayerProperty(circle-radius): %v", err)
	}
	property, err := m.LayerProperty("points-layer", "circle-radius")
	if err != nil {
		t.Fatalf("LayerProperty(circle-radius): %v", err)
	}
	if property != float64(5) {
		t.Fatalf("LayerProperty(circle-radius) = %#v, want 5", property)
	}
	filter := []any{"==", []any{"get", "kind"}, "unit"}
	if err := m.SetLayerFilter("points-layer", filter); err != nil {
		t.Fatalf("SetLayerFilter(): %v", err)
	}
	gotFilter, err := m.LayerFilter("points-layer")
	if err != nil {
		t.Fatalf("LayerFilter(): %v", err)
	}
	if gotFilter == nil {
		t.Fatalf("LayerFilter() = nil, want copied filter")
	}
	if err := m.SetLayerFilter("points-layer", nil); err != nil {
		t.Fatalf("SetLayerFilter(nil): %v", err)
	}
	if err := m.SetLayerProperty("points-layer", "circle-radius", make(chan int)); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetLayerProperty(unsupported value) error = %v, want ErrInvalidArgument", err)
	}
	if _, err := m.LayerProperty("missing", "circle-radius"); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("LayerProperty(missing layer) error = %v, want ErrInvalidArgument", err)
	}
}

func TestStyleLightPropertyJSONSnapshots(t *testing.T) {
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

	if err := m.SetStyleJSON(`{"version":8,"sources":{},"layers":[]}`); err != nil {
		t.Fatalf("SetStyleJSON(empty style): %v", err)
	}
	if err := m.SetStyleLightJSON(map[string]any{"anchor": "viewport", "color": "#ffffff", "intensity": float64(0.5)}); err != nil {
		t.Fatalf("SetStyleLightJSON(): %v", err)
	}
	if err := m.SetStyleLightProperty("intensity", float64(0.75)); err != nil {
		t.Fatalf("SetStyleLightProperty(intensity): %v", err)
	}
	intensity, err := m.StyleLightProperty("intensity")
	if err != nil {
		t.Fatalf("StyleLightProperty(intensity): %v", err)
	}
	if intensity != float64(0.75) {
		t.Fatalf("StyleLightProperty(intensity) = %#v, want 0.75", intensity)
	}
	if err := m.SetStyleLightProperty("intensity", make(chan int)); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetStyleLightProperty(unsupported value) error = %v, want ErrInvalidArgument", err)
	}
}

func TestStyleLayerMetadataForMissingLayers(t *testing.T) {
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

	if err := m.SetStyleJSON(`{"version":8,"sources":{},"layers":[]}`); err != nil {
		t.Fatalf("SetStyleJSON(empty style): %v", err)
	}
	ids, err := m.StyleLayerIDs()
	if err != nil {
		t.Fatalf("StyleLayerIDs(): %v", err)
	}
	for _, id := range ids {
		if id == "missing" {
			t.Fatalf("StyleLayerIDs() unexpectedly contains missing layer: %v", ids)
		}
	}
	exists, err := m.StyleLayerExists("missing")
	if err != nil {
		t.Fatalf("StyleLayerExists(): %v", err)
	}
	if exists {
		t.Fatalf("StyleLayerExists(missing) = true, want false")
	}
	layerType, found, err := m.StyleLayerType("missing")
	if err != nil {
		t.Fatalf("StyleLayerType(): %v", err)
	}
	if found || layerType != "" {
		t.Fatalf("StyleLayerType(missing) = (%q, %v), want empty false", layerType, found)
	}
	removed, err := m.RemoveStyleLayer("missing")
	if err != nil {
		t.Fatalf("RemoveStyleLayer(): %v", err)
	}
	if removed {
		t.Fatalf("RemoveStyleLayer(missing) = true, want false")
	}
	if err := m.MoveStyleLayer("missing", ""); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("MoveStyleLayer(missing) error = %v, want ErrInvalidArgument", err)
	}
	if _, err := m.StyleLayerExists(""); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("StyleLayerExists(empty) error = %v, want ErrInvalidArgument", err)
	}
}

func TestMapProjectionCameraAndVisibleCoordinates(t *testing.T) {
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
	defer func() {
		if err := projection.Close(); err != nil {
			t.Errorf("Projection Close(): %v", err)
		}
		if err := m.Close(); err != nil {
			t.Errorf("Map Close(): %v", err)
		}
		if err := runtime.Close(); err != nil {
			t.Errorf("Runtime Close(): %v", err)
		}
	}()

	camera := CameraOptions{}.
		WithCenter(LatLng{Latitude: 2, Longitude: 3}).
		WithZoom(2)
	if err := projection.SetCamera(camera); err != nil {
		t.Fatalf("SetCamera(): %v", err)
	}
	gotCamera, err := projection.Camera()
	if err != nil {
		t.Fatalf("Camera(): %v", err)
	}
	if gotCamera.Center == nil || gotCamera.Zoom == nil {
		t.Fatalf("Camera() missing expected fields: %#v", gotCamera)
	}
	if err := projection.SetVisibleCoordinates([]LatLng{{Latitude: -1, Longitude: -1}, {Latitude: 1, Longitude: 1}}, EdgeInsets{}); err != nil {
		t.Fatalf("SetVisibleCoordinates(): %v", err)
	}
	if _, err := projection.Camera(); err != nil {
		t.Fatalf("Camera() after SetVisibleCoordinates(): %v", err)
	}
	if err := projection.SetVisibleCoordinates(nil, EdgeInsets{}); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetVisibleCoordinates(nil) error = %v, want ErrInvalidArgument", err)
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
