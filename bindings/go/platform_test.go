package maplibre

import (
	"context"
	"log/slog"
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
	f.m = await(t, submitted(runtime.CreateMap(DefaultMapOptions())))
	f.serveStyle(t, "custom://style.json")
	if _, err := f.m.SetStyleUrl("custom://style.json"); err != nil {
		t.Fatal(err)
	}
	f.awaitEvent(t, "the style load", isStyleLoaded)
	awaitCommitted(t, submitted(f.m.AddCustomGeometrySource("live", CustomGeometrySourceOptions{FetchTile: func(CanonicalTileId) {}})))
	_, _ = os.Stdout.WriteString("exiting with live handles\n")
	os.Exit(0)
}

// The collector disposes a map nobody closed and logs the leak once through
// the default slog logger, though the disposal succeeds.
func TestCollectorDisposesAndLogsALeakedHandle(t *testing.T) {
	leaks := captureLeakLogs(t)
	f := newRuntimeFixture(t)
	dropped := await(t, submitted(f.runtime.CreateMap(DefaultMapOptions())))
	id, err := dropped.Id()
	if err != nil {
		t.Fatal(err)
	}
	dropped = nil
	closeOnceCollected(t, f.runtime, "the disposal of the dropped map")
	for {
		leak := receive(t, leaks, "the dropped map's leak log")
		if leak.handle != id {
			continue
		}
		if leak.typeName != "MapHandle" {
			t.Fatalf("logged %+v", leak)
		}
		break
	}
	select {
	case leak := <-leaks:
		if leak.handle == id {
			t.Fatalf("logged the map again: %+v", leak)
		}
	default:
	}
}

// leakedHandle is one leak that the binding logged.
type leakedHandle struct {
	typeName string
	handle   uint64
}

// captureLeakLogs routes the default slog logger's leak warnings to the
// returned channel until the test ends.
func captureLeakLogs(t *testing.T) <-chan leakedHandle {
	leaks := make(chan leakedHandle, 64)
	previous := slog.Default()
	slog.SetDefault(slog.New(leakLogHandler(leaks)))
	t.Cleanup(func() { slog.SetDefault(previous) })
	return leaks
}

type leakLogHandler chan<- leakedHandle

func (leakLogHandler) Enabled(context.Context, slog.Level) bool   { return true }
func (handler leakLogHandler) WithAttrs([]slog.Attr) slog.Handler { return handler }
func (handler leakLogHandler) WithGroup(string) slog.Handler      { return handler }

func (handler leakLogHandler) Handle(_ context.Context, record slog.Record) error {
	if record.Level != slog.LevelWarn || record.Message != "maplibre: leaked handle; close it explicitly" {
		return nil
	}
	var logged leakedHandle
	record.Attrs(func(attr slog.Attr) bool {
		switch attr.Key {
		case "type":
			logged.typeName = attr.Value.String()
		case "handle":
			logged.handle = attr.Value.Uint64()
		}
		return true
	})
	select {
	case handler <- logged:
	default:
	}
	return nil
}

// The collector retires a map nobody closed, whether the test dropped its
// handle or never claimed it from the creation's future: the map's cleanup
// disposes it, and the runtime then closes.
func TestCollectorRetiresUnclosedMaps(t *testing.T) {
	f := newRuntimeFixture(t)
	dropped := await(t, submitted(f.runtime.CreateMap(DefaultMapOptions())))
	awaitCommitted(t, submitted(dropped.SetStyleJson([]byte(emptyStyle))))
	unclaimed, err := f.runtime.CreateMap(DefaultMapOptions())
	if err != nil {
		t.Fatal(err)
	}
	receive(t, unclaimed.Done(), "the unclaimed map's creation")
	dropped, unclaimed = nil, nil
	closeOnceCollected(t, f.runtime, "the disposal of both maps")
}
