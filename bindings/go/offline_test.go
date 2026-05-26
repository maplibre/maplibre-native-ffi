package maplibre

import (
	"errors"
	stdruntime "runtime"
	"testing"
	"time"
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
