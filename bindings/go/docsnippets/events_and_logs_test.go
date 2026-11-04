// Snippets for the docs site's Events and logs page. They compile with the
// tests, which never run them.
package docsnippets

import (
	"context"

	"github.com/maplibre/maplibre-native-ffi/bindings/go/maplibre"
)

func onStyleLoaded() {}

func onLoadingFailed(message string) {}

func writeLog(severity maplibre.LogSeverity, message string) {}

func eventsAndLogs(ctx context.Context, runtime *maplibre.RuntimeHandle) error {
	var m *maplibre.MapHandle

	// #region handler
	// Runs on a library thread.
	handleEvents := func(events []maplibre.RuntimeEvent) {
		for _, event := range events {
			if event.Source != m.ID() {
				continue
			}

			switch event.Type {
			case maplibre.RuntimeEventTypeMapStyleLoaded:
				onStyleLoaded()
			case maplibre.RuntimeEventTypeMapLoadingFailed:
				onLoadingFailed(event.Message)
			}
		}
	}

	err := runtime.Discarding().SetEventHandler(
		maplibre.RuntimeEventHandler{Callback: handleEvents},
	)
	if err != nil {
		return err
	}
	// #endregion handler

	// #region mask
	m, created, err := runtime.CreateMap(maplibre.MapOptions{
		InitialExtent: maplibre.LogicalExtent{
			Width: 512, Height: 512, ScaleFactor: 1,
		},
		EventMask: new(maplibre.RuntimeEventFlagMapStyleLoaded |
			maplibre.RuntimeEventFlagMapLoadingFailed),
	})
	if err != nil {
		return err
	}
	// #endregion mask
	if _, err := created.Await(ctx); err != nil {
		return err
	}

	// #region logs
	err = maplibre.SetLogCallback(maplibre.LogHandler{
		// Runs on a library thread, and can't call the library.
		Callback: func(
			severity maplibre.LogSeverity,
			_ maplibre.LogEvent,
			_ int64,
			message string,
		) {
			writeLog(severity, message)
		},
	})
	if err != nil {
		return err
	}
	// #endregion logs
	return nil
}

// An example without an output comment compiles but never runs.
func Example_eventsAndLogs() {
	_ = eventsAndLogs(context.Background(), nil)
}
