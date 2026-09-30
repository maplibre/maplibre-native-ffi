package maplibre

import (
	"os"
	"os/exec"
	"strings"
	"testing"
)

// The library the build linked is the one the preset built: it reports the C
// ABI version this binding targets, and the render backend that the test
// task's build tag names.
func TestLinkedLibraryMatchesTheBuild(t *testing.T) {
	if version, err := CVersion(); err != nil || version != bindingCABIVersion {
		t.Fatalf("CVersion() = %d, %v; want %d", version, err, bindingCABIVersion)
	}
	backends, err := SupportedRenderBackendMask()
	if err != nil {
		t.Fatal(err)
	}
	if !backends.Has(buildBackend) {
		t.Fatalf("SupportedRenderBackendMask() = %#x, want the build's backend %#x", uint32(backends), uint32(buildBackend))
	}
}

const exitHelperEnv = "MLN_GO_TEST_EXIT_WITH_LIVE_HANDLES"

// A process that exits while a runtime, a map, and callbacks are live exits
// with its own status. The test runs its own binary again as that process.
func TestProcessExitsCleanlyWithLiveHandles(t *testing.T) {
	if os.Getenv(exitHelperEnv) != "" {
		exitWithLiveHandles(t)
	}
	command := exec.Command(os.Args[0], "-test.run=^TestProcessExitsCleanlyWithLiveHandles$", "-test.count=1")
	command.Env = append(os.Environ(), exitHelperEnv+"=1")
	output, err := command.CombinedOutput()
	if err != nil || !strings.Contains(string(output), "exiting with live handles") {
		t.Fatalf("the exiting process: %v\n%s", err, output)
	}
}

func exitWithLiveHandles(t *testing.T) {
	if err := LogSetCallback(func(LogSeverity, LogEvent, int64, string) uint32 { return 0 }); err != nil {
		t.Fatal(err)
	}
	f := &fixture{events: make(chan struct{}, 1)}
	options := DefaultRuntimeOptions()
	options.EventWake = Wake{Callback: func() { notify(f.events) }}
	runtime, err := RuntimeCreate(options)
	if err != nil {
		t.Fatal(err)
	}
	f.runtime = runtime
	f.m = await(t, submitted(runtime.MapCreate(DefaultMapOptions())))
	f.serveStyle(t, "custom://style.json")
	if _, err := f.m.SetStyleUrl("custom://style.json"); err != nil {
		t.Fatal(err)
	}
	f.awaitEvent(t, "the style load", isStyleLoaded)
	awaitCommitted(t, submitted(f.m.AddCustomGeometrySource("live", CustomGeometrySourceOptions{FetchTile: func(CanonicalTileId) {}})))
	_, _ = os.Stdout.WriteString("exiting with live handles\n")
	os.Exit(0)
}

// The collector reclaims a handle nobody closed: its cleanup disposes the
// native map, which then no longer holds its runtime open.
func TestCollectorReclaimsAnAbandonedHandle(t *testing.T) {
	f := newRuntimeFixture(t)
	m := await(t, submitted(f.runtime.MapCreate(DefaultMapOptions())))
	disposed := disposalSignal(m.bindingOwner)
	m = nil
	awaitCollected(t, disposed, "the abandoned map's disposal")
	// The barrier orders the map's retirement, which disposal only schedules.
	await(t, submitted(f.runtime.Barrier()))
	await(t, submitted(f.runtime.Close()))
}
