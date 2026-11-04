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
	if err := SetLogCallback(LogHandler{Callback: func(LogSeverity, LogEvent, int64, string) {}}); err != nil {
		t.Fatal(err)
	}
	// The process exits before the fixture's cleanups can close anything.
	f := newFixture(t)
	f.serveStyle(t, "custom://style.json")
	if _, err := f.m.SetStyleURL("custom://style.json"); err != nil {
		t.Fatal(err)
	}
	f.awaitEvent(t, "the style load", isStyleLoaded)
	await(t, submitted(f.m.AddCustomGeometrySource("live", CustomGeometrySourceOptions{FetchTile: func(*MapHandle, string, CanonicalTileID) {}})))
	_, _ = os.Stdout.WriteString("exiting with live handles\n")
	os.Exit(0)
}
