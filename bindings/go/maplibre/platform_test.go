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
	if version := CVersion(); version != bindingCAbiVersion {
		t.Fatalf("CVersion() = %d, want %d", version, bindingCAbiVersion)
	}
	if backends := SupportedRenderBackendMask(); !backends.Has(buildBackend) {
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
	if err := SetLogCallback(LogHandler{Callback: func(LogSeverity, LogEvent, int64, string) bool { return false }}); err != nil {
		t.Fatal(err)
	}
	f := &fixture{pendingEvents: make(chan RuntimeEvent, 4096)}
	runtime, err := NewRuntime(RuntimeOptions{})
	if err != nil {
		t.Fatal(err)
	}
	f.runtime = runtime
	// A handler accepted before CreateMap receives the map's first events.
	if err := runtime.Discarding().SetEventHandler(RuntimeEventHandler{Callback: func(events []RuntimeEvent) {
		for _, event := range events {
			f.pendingEvents <- event
		}
	}}); err != nil {
		t.Fatal(err)
	}
	f.m = createMap(t, runtime, testMapOptions())
	f.serveStyle(t, "custom://style.json")
	if _, err := f.m.SetStyleUrl("custom://style.json"); err != nil {
		t.Fatal(err)
	}
	f.awaitEvent(t, "the style load", isStyleLoaded)
	await(t, submitted(f.m.AddCustomGeometrySource("live", CustomGeometrySourceOptions{FetchTile: func(CanonicalTileId) {}})))
	_, _ = os.Stdout.WriteString("exiting with live handles\n")
	os.Exit(0)
}
