package main

import (
	"errors"
	"fmt"
	"os"
	stdruntime "runtime"
	"strings"
	"time"

	"github.com/jfreymuth/go-sdl3/sdl"
	maplibre "github.com/maplibre/maplibre-native-ffi/bindings/go"
)

func main() {
	// SDL and OpenGL keep the render-session graphics calls on this thread.
	stdruntime.LockOSThread()
	defer stdruntime.UnlockOSThread()

	mode, smoke, ok := parseArgs(os.Args[1:])
	if !ok {
		return
	}
	if err := run(mode, smoke); err != nil {
		fmt.Fprintf(os.Stderr, "%v\n", err)
		os.Exit(1)
	}
}

func parseArgs(args []string) (renderTargetMode, bool, bool) {
	if len(args) == 1 && args[0] == "--help" {
		printUsage()
		return 0, false, false
	}
	if len(args) != 1 || strings.HasPrefix(args[0], "-") {
		printUsage()
		os.Exit(1)
	}
	mode, ok := parseRenderTargetMode(args[0])
	if !ok {
		printUsage()
		os.Exit(1)
	}
	return mode, smokeMode(), true
}

// smokeMode reports whether MLN_EXAMPLE_SMOKE=1 selects a smoke run, which
// renders one frame of a local style in a hidden window and exits.
func smokeMode() bool {
	return os.Getenv("MLN_EXAMPLE_SMOKE") == "1"
}

func printUsage() {
	fmt.Print(`Usage: go-map <mode>

Modes:
  owned-texture     session-owned texture render target
  borrowed-texture  caller-owned texture render target
  native-surface    native surface render target
`)
}

// smokeTimeout bounds a smoke run, which exits with an error when no frame
// renders in time.
const smokeTimeout = 60 * time.Second

func run(mode renderTargetMode, smoke bool) (result error) {
	if err := validateNativeRenderBackend(); err != nil {
		return err
	}
	if err := maplibre.LogSetCallback(func(severity maplibre.LogSeverity, event maplibre.LogEvent, code int64, message string) uint32 {
		fmt.Printf("maplibre[%s/%s] %d: %s\n", logSeverity(severity), logEvent(event), code, message)
		return 1
	}); err != nil {
		return err
	}
	defer func() { _ = maplibre.LogClearCallback() }()

	if usesEGL() {
		_ = sdl.SetHint(sdl.HintVideoForceEgl, "1")
	}
	if err := sdl.Init(sdl.InitVideo); err != nil {
		return fmt.Errorf("SDL_Init failed: %w", err)
	}
	defer sdl.Quit()

	if usesEGL() {
		if err := sdl.GL_SetAttribute(sdl.GLContextProfileMask, int32(sdl.GLContextProfileEs)); err != nil {
			return err
		}
		if err := sdl.GL_SetAttribute(sdl.GLContextMajorVersion, 3); err != nil {
			return err
		}
		if err := sdl.GL_SetAttribute(sdl.GLContextMinorVersion, 0); err != nil {
			return err
		}
	}

	windowFlags := sdl.WindowOpenGL | sdl.WindowResizable | sdl.WindowHighPixelDensity
	if smoke {
		windowFlags |= sdl.WindowHidden
	}
	window, err := sdl.CreateWindow("MapLibre Go SDL3 Map", initialWindowWidth, initialWindowHeight, windowFlags)
	if err != nil {
		return fmt.Errorf("SDL_CreateWindow failed: %w", err)
	}
	defer window.Destroy()
	if !smoke {
		_ = window.Raise()
	}

	view := currentViewport(window)
	view.log("initial viewport")
	if view.empty() {
		return errors.New("initial viewport is empty")
	}

	graphics, err := newOpenGLContext(window)
	if err != nil {
		return err
	}
	_ = sdl.GL_SetSwapInterval(1)

	wakes := newLoopWakes()
	mapState, err := newRuntimeMapState(view, smoke, wakes.events.wake())
	if err != nil {
		_ = graphics.Close()
		return err
	}
	target, err := newOpenGLRenderTarget(graphics, view, mode, mapState.mapRef, wakes)
	if err != nil {
		return errors.Join(
			fmt.Errorf("render target attach failed: %w", err),
			mapState.Close(),
			graphics.Close(),
		)
	}
	defer func() {
		result = errors.Join(result, target.Close(), mapState.Close(), graphics.Close())
	}()

	fmt.Printf("render target: %s\n", mode)
	fmt.Printf("render target status: %s\n", mode.statusLine())
	logControls()

	// The SDL thread sleeps until input or a native wake arrives. Input
	// becomes map commands, a map update becomes a frame demand, and each wake
	// has the thread drain events, service driver work, or drain frame results.
	input := inputController{}
	smokeDeadline := time.Now().Add(smokeTimeout)
	for {
		viewportChanged := false
		var event sdl.Event
		deadline, hasDeadline := nextWake(target, smoke, smokeDeadline)
		ok := waitEvent(&event, deadline, hasDeadline)
		if smoke && !time.Now().Before(smokeDeadline) {
			return fmt.Errorf("smoke: no frame rendered within %s", smokeTimeout)
		}
		for ; ok; ok = sdl.PollEvent(&event) {
			switch event.Type() {
			case sdl.EventQuit, sdl.EventWindowCloseRequested:
				return nil
			case sdl.EventWindowResized, sdl.EventWindowPixelSizeChanged, sdl.EventWindowDisplayScaleChanged:
				view = currentViewport(window)
				view.log("resized viewport")
				viewportChanged = true
			default:
				if !view.empty() {
					if err := input.handleEvent(&event, mapState, view); err != nil {
						return err
					}
				}
			}
		}
		// A live resize delivers several window events at once, and this
		// resizes once for all of them.
		if viewportChanged && !view.empty() {
			if err := target.Resize(view); err != nil {
				return err
			}
		}
		if wakes.events.consume() {
			update, err := mapState.drainRenderUpdates()
			if err != nil {
				return err
			}
			if update && !view.empty() {
				if err := target.RequestFrame(false); err != nil {
					return err
				}
			}
		}
		if wakes.driverWork.consume() {
			if err := target.ServiceDriverWork(); err != nil {
				return err
			}
		}
		if err := target.RetryIfDue(); err != nil {
			return err
		}
		if wakes.frames.consume() {
			presented, err := target.DrainFrameResults()
			if err != nil {
				return err
			}
			if smoke && presented {
				fmt.Println("smoke: rendered one frame")
				return nil
			}
		}
	}
}

// nextWake reports when the loop must wake without an event: for a paced
// retry, or at a smoke run's deadline.
func nextWake(target renderTarget, smoke bool, smokeDeadline time.Time) (time.Time, bool) {
	retryAt, retry := target.RetryAt()
	if smoke && (!retry || smokeDeadline.Before(retryAt)) {
		return smokeDeadline, true
	}
	return retryAt, retry
}

// waitEvent sleeps until the next SDL event, or until a deadline when it has
// one, and reports whether an event arrived.
func waitEvent(event *sdl.Event, deadline time.Time, hasDeadline bool) bool {
	if !hasDeadline {
		return sdl.WaitEvent(event) == nil
	}
	remaining := time.Until(deadline)
	return sdl.WaitEventTimeout(event, int32(max(remaining.Milliseconds(), 0))+1)
}

func validateNativeRenderBackend() error {
	backends, err := maplibre.SupportedRenderBackendMask()
	if err != nil {
		return err
	}
	fmt.Printf("native render backends: %s\n", renderBackendSupportLabel(backends))
	if !backends.Has(maplibre.RenderBackendFlagOpengl) {
		return errors.New("loaded native library does not support OpenGL")
	}
	providers, err := maplibre.OpenglSupportedContextProviderMask()
	if err != nil {
		return err
	}
	required := maplibre.OpenglContextProviderFlagEgl
	if stdruntime.GOOS == "windows" {
		required = maplibre.OpenglContextProviderFlagWgl
	}
	if !providers.Has(required) {
		return fmt.Errorf("loaded native library does not support required OpenGL context provider: %s", openGLProviderLabel(required))
	}
	return nil
}

func renderBackendSupportLabel(mask maplibre.RenderBackendFlag) string {
	var labels []string
	if mask.Has(maplibre.RenderBackendFlagMetal) {
		labels = append(labels, "metal")
	}
	if mask.Has(maplibre.RenderBackendFlagOpengl) {
		labels = append(labels, "opengl")
	}
	if mask.Has(maplibre.RenderBackendFlagVulkan) {
		labels = append(labels, "vulkan")
	}
	if len(labels) == 0 {
		return "none"
	}
	return strings.Join(labels, ",")
}

func openGLProviderLabel(provider maplibre.OpenglContextProviderFlag) string {
	switch provider {
	case maplibre.OpenglContextProviderFlagWgl:
		return "wgl"
	case maplibre.OpenglContextProviderFlagEgl:
		return "egl"
	default:
		return "unknown"
	}
}

func logSeverity(severity maplibre.LogSeverity) string {
	switch severity {
	case maplibre.LogSeverityInfo:
		return "info"
	case maplibre.LogSeverityWarning:
		return "warning"
	case maplibre.LogSeverityError:
		return "error"
	default:
		return "unknown"
	}
}

func logEvent(event maplibre.LogEvent) string {
	switch event {
	case maplibre.LogEventGraphicsBackend:
		return "graphics-backend"
	case maplibre.LogEventRender:
		return "render"
	case maplibre.LogEventHttpRequest:
		return "http"
	case maplibre.LogEventParseStyle:
		return "style-parse"
	case maplibre.LogEventParseTile:
		return "tile-parse"
	default:
		return "general"
	}
}
