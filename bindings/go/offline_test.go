package maplibre

import (
	"errors"
	stdruntime "runtime"
	"sync/atomic"
	"testing"
	"time"

	"github.com/maplibre/maplibre-native-ffi/bindings/go/internal/handle"
)

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

func testOfflineGeometryDefinition() OfflineGeometryRegionDefinition {
	return OfflineGeometryRegionDefinition{
		StyleURL: "http://example.com/offline-geometry-style.json",
		Geometry: PolygonGeometry([][]LatLng{{
			{Latitude: -1, Longitude: -2},
			{Latitude: -1, Longitude: 2},
			{Latitude: 1, Longitude: 2},
			{Latitude: 1, Longitude: -2},
			{Latitude: -1, Longitude: -2},
		}}),
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

	create, err := runtime.StartCreateOfflineRegion(testOfflineTileDefinition(), []byte{1, 2, 3})
	if err != nil {
		t.Fatalf("StartCreateOfflineRegion(): %v", err)
	}
	requireDiscardOfflineOperation(t, create, OfflineOperationRegionCreate, OfflineOperationResultRegion)

	createGeometry, err := runtime.StartCreateOfflineRegion(testOfflineGeometryDefinition(), []byte{1, 2, 3})
	if err != nil {
		t.Fatalf("StartCreateOfflineRegion(geometry): %v", err)
	}
	requireDiscardOfflineOperation(t, createGeometry, OfflineOperationRegionCreate, OfflineOperationResultRegion)

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

	definition := testOfflineTileDefinition()
	definition.StyleURL = "http://example.com/\x00style.json"
	if _, err := runtime.StartCreateOfflineRegion(definition, nil); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("StartCreateOfflineRegion embedded NUL error = %v, want ErrInvalidArgument", err)
	}
	geometryDefinition := testOfflineGeometryDefinition()
	geometryDefinition.StyleURL = "http://example.com/\x00style.json"
	if _, err := runtime.StartCreateOfflineRegion(geometryDefinition, nil); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("StartCreateOfflineRegion geometry embedded NUL error = %v, want ErrInvalidArgument", err)
	}
	geometryDefinition = testOfflineGeometryDefinition()
	geometryDefinition.Geometry = Geometry{Type: GeometryType(999_999)}
	if _, err := runtime.StartCreateOfflineRegion(geometryDefinition, nil); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("StartCreateOfflineRegion bad geometry error = %v, want ErrInvalidArgument", err)
	}
	if _, err := runtime.StartMergeOfflineRegionsDatabase("/tmp/\x00side.db"); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("StartMergeOfflineRegionsDatabase embedded NUL error = %v, want ErrInvalidArgument", err)
	}
	if _, err := runtime.StartSetOfflineRegionDownloadState(1, OfflineRegionDownloadState(999_999)); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("StartSetOfflineRegionDownloadState unknown error = %v, want ErrInvalidArgument", err)
	}
}

func TestOfflineGeometryDefinitionMaterializesAndCopies(t *testing.T) {
	definition := testOfflineGeometryDefinition()
	raw, err := newCOfflineGeometryRegionDefinition(definition)
	if err != nil {
		t.Fatalf("newCOfflineGeometryRegionDefinition(): %v", err)
	}
	defer raw.free()

	copiedDefinition, err := raw.copyDefinition()
	if err != nil {
		t.Fatalf("copyDefinition(): %v", err)
	}
	copied, ok := copiedDefinition.(OfflineGeometryRegionDefinition)
	if !ok {
		t.Fatalf("copyDefinition() = %T, want OfflineGeometryRegionDefinition", copiedDefinition)
	}
	if copied.StyleURL != definition.StyleURL || copied.MinZoom != definition.MinZoom || copied.MaxZoom != definition.MaxZoom || copied.PixelRatio != definition.PixelRatio || copied.IncludeIdeographs != definition.IncludeIdeographs {
		t.Fatalf("copied scalar fields = %#v, want %#v", copied, definition)
	}
	if copied.Geometry.Type != GeometryTypePolygon || len(copied.Geometry.Lines) != 1 || len(copied.Geometry.Lines[0]) != len(definition.Geometry.Lines[0]) {
		t.Fatalf("copied geometry = %#v, want polygon with %d coordinates", copied.Geometry, len(definition.Geometry.Lines[0]))
	}
}

func TestOfflineOperationDiscardIsIdempotentAfterSuccess(t *testing.T) {
	runtime := newFakeRuntimeHandle(t)
	defer closeFakeRuntimeHandle(t, runtime)

	var calls int
	restore := replaceOfflineOperationDiscardForTest(func(ptr *nativeRuntime, id uint64) int32 {
		calls++
		return 0
	})
	defer restore()

	operation := newOfflineOperationHandle[struct{}](runtime, 1, OfflineOperationRegionSetObserved, OfflineOperationResultNone)
	if err := operation.Discard(); err != nil {
		t.Fatalf("Discard(): %v", err)
	}
	if err := operation.Discard(); err != nil {
		t.Fatalf("second Discard(): %v", err)
	}
	if calls != 1 {
		t.Fatalf("discard calls = %d, want 1", calls)
	}
}

func TestOfflineOperationDiscardFailureLeavesHandleRetryable(t *testing.T) {
	runtime := newFakeRuntimeHandle(t)
	defer closeFakeRuntimeHandle(t, runtime)

	statuses := []int32{-2, 0, 0}
	restore := replaceOfflineOperationDiscardForTest(func(ptr *nativeRuntime, id uint64) int32 {
		status := statuses[0]
		statuses = statuses[1:]
		return status
	})
	defer restore()

	operation := newOfflineOperationHandle[struct{}](runtime, 1, OfflineOperationRegionSetObserved, OfflineOperationResultNone)
	if err := operation.Discard(); !errors.Is(err, ErrInvalidState) {
		t.Fatalf("Discard() failure = %v, want ErrInvalidState", err)
	}
	if err := operation.Discard(); err != nil {
		t.Fatalf("Discard() retry: %v", err)
	}
}

func TestOfflineOperationBlocksRuntimeCloseUntilReleased(t *testing.T) {
	runtime := newFakeRuntimeHandle(t)
	defer closeFakeRuntimeHandle(t, runtime)

	restore := replaceOfflineOperationDiscardForTest(func(ptr *nativeRuntime, id uint64) int32 {
		return 0
	})
	defer restore()
	restoreRuntimeDestroy := replaceRuntimeDestroyForTest(func(ptr *nativeRuntime) int32 {
		return 0
	})
	defer restoreRuntimeDestroy()

	operation := newOfflineOperationHandle[struct{}](runtime, 1, OfflineOperationRegionSetObserved, OfflineOperationResultNone)
	if err := runtime.Close(); !errors.Is(err, ErrInvalidState) {
		t.Fatalf("Runtime Close() with live offline operation = %v, want ErrInvalidState", err)
	}
	if err := operation.Discard(); err != nil {
		t.Fatalf("Discard(): %v", err)
	}
	if err := runtime.Close(); err != nil {
		t.Fatalf("Runtime Close() after Discard(): %v", err)
	}
}

func TestOfflineOperationDiscardSerializesConcurrentCalls(t *testing.T) {
	runtime := newFakeRuntimeHandle(t)
	defer closeFakeRuntimeHandle(t, runtime)

	entered := make(chan struct{})
	release := make(chan struct{})
	var calls atomic.Int32
	restore := replaceOfflineOperationDiscardForTest(func(ptr *nativeRuntime, id uint64) int32 {
		if calls.Add(1) == 1 {
			close(entered)
			<-release
		}
		return 0
	})
	defer restore()

	operation := newOfflineOperationHandle[struct{}](runtime, 1, OfflineOperationRegionSetObserved, OfflineOperationResultNone)
	firstDone := make(chan error, 1)
	secondDone := make(chan error, 1)
	go func() { firstDone <- operation.Discard() }()
	<-entered
	secondStarted := make(chan struct{})
	go func() {
		close(secondStarted)
		secondDone <- operation.Discard()
	}()
	<-secondStarted
	select {
	case err := <-secondDone:
		t.Fatalf("second Discard() completed while first discard was in flight: %v", err)
	case <-time.After(25 * time.Millisecond):
	}
	close(release)
	if err := <-firstDone; err != nil {
		t.Fatalf("first Discard(): %v", err)
	}
	if err := <-secondDone; err != nil {
		t.Fatalf("second Discard(): %v", err)
	}
	if calls.Load() != 1 {
		t.Fatalf("native discard calls = %d, want 1", calls.Load())
	}
}

func TestOfflineOperationTakeRejectsNoResultOperationWithoutDiscarding(t *testing.T) {
	runtime := newFakeRuntimeHandle(t)
	defer closeFakeRuntimeHandle(t, runtime)

	var calls int
	restore := replaceOfflineOperationDiscardForTest(func(ptr *nativeRuntime, id uint64) int32 {
		calls++
		return 0
	})
	defer restore()

	operation := newOfflineOperationHandle[struct{}](runtime, 1, OfflineOperationRegionSetObserved, OfflineOperationResultNone)
	if _, err := operation.Take(); !errors.Is(err, ErrInvalidState) {
		t.Fatalf("Take() no-result operation = %v, want ErrInvalidState", err)
	}
	if err := operation.Discard(); err != nil {
		t.Fatalf("Discard() after rejected Take: %v", err)
	}
	if calls != 1 {
		t.Fatalf("discard calls = %d, want 1", calls)
	}
}

func TestOfflineOperationTakePreConsumeMismatchRemainsRetryable(t *testing.T) {
	runtime := newFakeRuntimeHandle(t)
	defer closeFakeRuntimeHandle(t, runtime)

	restore := replaceOfflineOperationDiscardForTest(func(ptr *nativeRuntime, id uint64) int32 {
		return 0
	})
	defer restore()

	preConsume := newOfflineOperationHandle[struct{}](runtime, 2, OfflineOperationRegionSetObserved, OfflineOperationResultKind(999_999))
	if _, err := preConsume.Take(); !errors.Is(err, ErrInvalidState) {
		t.Fatalf("Take() pre-consume mismatch = %v, want ErrInvalidState", err)
	}
	if err := preConsume.Discard(); err != nil {
		t.Fatalf("Discard() after pre-consume mismatch: %v", err)
	}
}

func TestStartOfflineOperationRejectsZeroID(t *testing.T) {
	runtime := newFakeRuntimeHandle(t)
	defer closeFakeRuntimeHandle(t, runtime)

	operation, err := startOfflineOperationZeroIDForTest(runtime)
	if !errors.Is(err, ErrInvalidState) {
		t.Fatalf("startOfflineOperation() error = %v, want ErrInvalidState", err)
	}
	if operation != nil {
		t.Fatalf("startOfflineOperation() operation = %#v, want nil", operation)
	}
}

func newFakeRuntimeHandle(t *testing.T) *RuntimeHandle {
	t.Helper()
	state, err := handle.New(&nativeRuntime{}, "RuntimeHandle")
	if err != nil {
		t.Fatal(err)
	}
	return &RuntimeHandle{state: state}
}

func closeFakeRuntimeHandle(t *testing.T, runtime *RuntimeHandle) {
	t.Helper()
	if status := runtime.state.Close(func(ptr *nativeRuntime) int32 { return 0 }); status != 0 {
		t.Fatalf("fake runtime close status = %d, want 0", status)
	}
}

func replaceOfflineOperationDiscardForTest(discard func(*nativeRuntime, uint64) int32) func() {
	previous := offlineOperationDiscard
	offlineOperationDiscard = discard
	return func() {
		offlineOperationDiscard = previous
	}
}

func replaceRuntimeDestroyForTest(destroy func(*nativeRuntime) int32) func() {
	previous := destroyRuntimeHandle
	destroyRuntimeHandle = destroy
	return func() {
		destroyRuntimeHandle = previous
	}
}
