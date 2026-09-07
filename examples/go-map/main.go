package main

import (
	"errors"
	"fmt"
	"math"
	"os"
	stdruntime "runtime"
	"strings"

	"github.com/jfreymuth/go-sdl3/sdl"
	maplibre "github.com/maplibre/maplibre-native-ffi/bindings/go"
)

func main() {
	// SDL and OpenGL keep the render-session graphics calls on this thread.
	stdruntime.LockOSThread()
	defer stdruntime.UnlockOSThread()

	mode, ok := parseArgs(os.Args[1:])
	if !ok {
		return
	}
	if err := run(mode); err != nil {
		fmt.Fprintf(os.Stderr, "%v\n", err)
		os.Exit(1)
	}
}

func parseArgs(args []string) (renderTargetMode, bool) {
	if len(args) == 1 && args[0] == "--help" {
		printUsage()
		return 0, false
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
	return mode, true
}

func printUsage() {
	fmt.Print(`Usage: go-map <mode>

Modes:
  owned-texture     session-owned texture render target
  borrowed-texture  caller-owned texture render target
  native-surface    native surface render target
`)
}

func run(mode renderTargetMode) (result error) {
	if err := validateNativeRenderBackend(); err != nil {
		return err
	}
	if err := maplibre.SetLogCallback(func(record maplibre.LogRecord) bool {
		fmt.Printf("maplibre[%s/%s] %d: %s\n", logSeverity(record.Severity), logEvent(record.Event), record.Code, record.Message)
		return true
	}); err != nil {
		return err
	}
	defer func() { _ = maplibre.ClearLogCallback() }()

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

	window, err := sdl.CreateWindow("MapLibre Go SDL3 Map", initialWindowWidth, initialWindowHeight, sdl.WindowOpenGL|sdl.WindowResizable|sdl.WindowHighPixelDensity)
	if err != nil {
		return fmt.Errorf("SDL_CreateWindow failed: %w", err)
	}
	defer window.Destroy()
	_ = window.Raise()

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

	mapState, err := newRuntimeMapState(view)
	if err != nil {
		_ = graphics.Close()
		return err
	}
	state, err := newRenderMapState(graphics, mapState.mapRef, view, mode)
	if err != nil {
		return errors.Join(
			fmt.Errorf("render target attach failed: %w", err),
			mapState.Close(),
			graphics.Close(),
		)
	}
	defer func() {
		result = errors.Join(
			result,
			state.finishFrame(),
			state.closeTarget(),
			mapState.Close(),
			graphics.Close(),
		)
	}()

	fmt.Printf("render target: %s\n", mode)
	fmt.Printf("render target status: %s\n", mode.statusLine())
	logControls()

	running := true
	renderRequested := true
	viewportDirty := false
	input := inputController{}
	handleEvent := func(event *sdl.Event) error {
		switch event.Type() {
		case sdl.EventQuit, sdl.EventWindowCloseRequested:
			running = false
		case sdl.EventWindowResized, sdl.EventWindowPixelSizeChanged, sdl.EventWindowDisplayScaleChanged:
			view = currentViewport(window)
			view.log("resized viewport")
			if view.empty() {
				return nil
			}
			viewportDirty = true
			renderRequested = true
		default:
			if view.empty() {
				return nil
			}
			changed, err := input.handleEvent(event, mapState, view)
			if err != nil {
				return err
			}
			if changed {
				renderRequested = true
			}
		}
		return nil
	}
	for running {
		didWork := false
		var event sdl.Event
		for sdl.PollEvent(&event) {
			didWork = true
			if err := handleEvent(&event); err != nil {
				return err
			}
		}
		requested, err := drainEvents(mapState.runtime, mapState.mapID)
		if err != nil {
			return err
		}
		if requested {
			renderRequested = true
			didWork = true
		}

		targetPending, err := state.pollPending()
		if err != nil {
			return err
		}
		if !targetPending && viewportDirty && !view.empty() {
			viewportDirty = false
			// The session resize carries the new logical extent to the map, so
			// this loop starts one and never resizes the map itself. Starting
			// it here instead of from the resize event coalesces a live resize
			// into one outstanding submission.
			if err := state.resize(view); err != nil {
				return err
			}
			targetPending = true
		}
		if !targetPending && renderRequested && !view.empty() && running {
			renderRequested = false
			outcome, err := state.driveFrame()
			if err != nil {
				return err
			}
			if outcome.rendered {
				didWork = true
			}
			if !outcome.rendered || outcome.needsRepaint {
				renderRequested = true
			}
		}
		if err := state.finishFrame(); err != nil {
			return err
		}

		if !didWork && running {
			if sdl.WaitEventTimeout(&event, displayRefreshTimeoutMS(window)) {
				if err := handleEvent(&event); err != nil {
					return err
				}
			}
		}
	}
	return nil
}

func displayRefreshTimeoutMS(window *sdl.Window) int32 {
	display, err := sdl.GetDisplayForWindow(window)
	if err != nil {
		return 16
	}
	mode, err := display.CurrentDisplayMode()
	if err != nil {
		return 16
	}
	hz := float64(mode.RefreshRate)
	if mode.RefreshRateNumerator > 0 && mode.RefreshRateDenominator > 0 {
		hz = float64(mode.RefreshRateNumerator) / float64(mode.RefreshRateDenominator)
	}
	if hz <= 0 {
		return 16
	}
	timeout := int32(math.Floor(1000 / hz))
	if timeout < 1 {
		return 1
	}
	return timeout
}

func validateNativeRenderBackend() error {
	backends := maplibre.SupportedRenderBackends()
	fmt.Printf("native render backends: %s\n", renderBackendSupportLabel(backends))
	if !backends.Has(maplibre.RenderBackendOpenGL) {
		return errors.New("loaded native library does not support OpenGL")
	}
	providers := maplibre.SupportedOpenGLContextProviders()
	required := maplibre.OpenGLContextProviderEGL
	if stdruntime.GOOS == "windows" {
		required = maplibre.OpenGLContextProviderWGL
	}
	if !providers.Has(required) {
		return fmt.Errorf("loaded native library does not support required OpenGL context provider: %s", openGLProviderLabel(required))
	}
	return nil
}

func renderBackendSupportLabel(mask maplibre.RenderBackendMask) string {
	var labels []string
	if mask.Has(maplibre.RenderBackendMetal) {
		labels = append(labels, "metal")
	}
	if mask.Has(maplibre.RenderBackendOpenGL) {
		labels = append(labels, "opengl")
	}
	if mask.Has(maplibre.RenderBackendVulkan) {
		labels = append(labels, "vulkan")
	}
	if len(labels) == 0 {
		return "none"
	}
	return strings.Join(labels, ",")
}

func openGLProviderLabel(provider maplibre.OpenGLContextProviderMask) string {
	switch provider {
	case maplibre.OpenGLContextProviderWGL:
		return "wgl"
	case maplibre.OpenGLContextProviderEGL:
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
	case maplibre.LogEventOpenGL:
		return "opengl"
	case maplibre.LogEventRender:
		return "render"
	case maplibre.LogEventHTTPRequest:
		return "http"
	case maplibre.LogEventParseStyle:
		return "style-parse"
	case maplibre.LogEventParseTile:
		return "tile-parse"
	default:
		return "general"
	}
}
