package main

import (
	"errors"
	"fmt"
	"os"
	"runtime"
	"strings"
	"time"

	"github.com/jfreymuth/go-sdl3/sdl"
	maplibre "github.com/maplibre/maplibre-native-ffi/bindings/go"
)

func main() {
	runtime.LockOSThread()
	defer runtime.UnlockOSThread()

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

func run(mode renderTargetMode) error {
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
	_ = sdl.GL_SetSwapInterval(1)

	view := currentViewport(window)
	view.log("initial viewport")

	state, err := newMapState(window, view, mode)
	if err != nil {
		return err
	}
	defer func() { _ = state.Close() }()
	defer func() { _ = state.finishFrame() }()

	fmt.Printf("render target: %s\n", mode)
	fmt.Printf("render target status: %s\n", mode.statusLine())
	logControls()

	running := true
	hasPresentedFrame := false
	renderPending := true
	input := inputController{}
	for running {
		didWork := false
		var event sdl.Event
		for sdl.PollEvent(&event) {
			didWork = true
			switch event.Type() {
			case sdl.EventQuit, sdl.EventWindowCloseRequested:
				running = false
			case sdl.EventWindowResized, sdl.EventWindowPixelSizeChanged, sdl.EventWindowDisplayScaleChanged:
				view = currentViewport(window)
				view.log("resized viewport")
				if err := state.resize(window, view, mode); err != nil {
					return err
				}
				renderPending = true
			default:
				changed, err := input.handleEvent(&event, state.mapRef, view)
				if err != nil {
					return err
				}
				renderPending = renderPending || changed
			}
		}

		if err := state.runtime.RunOnce(); err != nil {
			return err
		}
		renderUpdateAvailable, err := drainEvents(state.runtime)
		if err != nil {
			return err
		}
		renderPending = renderPending || renderUpdateAvailable
		didWork = didWork || renderUpdateAvailable

		if err := state.finishFrame(); err != nil {
			return err
		}
		if renderPending {
			rendered, err := state.renderUpdate()
			if err != nil {
				return err
			}
			if rendered {
				renderPending = false
				didWork = true
				hasPresentedFrame = true
			}
		}

		if !didWork {
			if hasPresentedFrame {
				time.Sleep(8 * time.Millisecond)
			} else {
				time.Sleep(time.Millisecond)
			}
		}
	}
	return nil
}

func validateNativeRenderBackend() error {
	backends := maplibre.SupportedRenderBackends()
	fmt.Printf("native render backends: %s\n", renderBackendSupportLabel(backends))
	if !backends.Has(maplibre.RenderBackendOpenGL) {
		return errors.New("loaded native library does not support OpenGL")
	}
	providers := maplibre.SupportedOpenGLContextProviders()
	required := maplibre.OpenGLContextProviderEGL
	if runtime.GOOS == "windows" {
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
