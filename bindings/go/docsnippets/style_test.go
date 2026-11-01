// Snippets for the docs site's Style page. They compile with the tests, which
// never run them.
package docsnippets

import (
	"log"

	"github.com/maplibre/maplibre-native-ffi/bindings/go/maplibre"
)

func drawImage(imageID string) maplibre.PremultipliedRgba8Image {
	return maplibre.PremultipliedRgba8Image{
		Width:  1,
		Height: 1,
		Stride: 4,
		Pixels: []byte{255, 0, 0, 255},
	}
}

// pixels holds 16 x 16 premultiplied RGBA pixels.
func style(m *maplibre.MapHandle, pixels []byte) error {
	// #region load
	if err := m.Discarding().SetStyleUrl("https://demotiles.maplibre.org/style.json"); err != nil {
		return err
	}
	// #endregion load

	// #region add
	err := m.Discarding().AddStyleSourceJson("satellite", `{
		"type": "raster",
		"tileSize": 256,
		"tiles": ["https://tiles.example.com/{z}/{x}/{y}.png"]
	}`)
	if err != nil {
		return err
	}

	// Draw the layer under the "countries-label" layer.
	err = m.Discarding().AddStyleLayerJson(
		`{"id": "satellite", "type": "raster", "source": "satellite"}`,
		"countries-label",
	)
	if err != nil {
		return err
	}
	// #endregion add

	// #region geojson
	data, err := maplibre.NewGeoJsonSourceData(`{"type": "Point", "coordinates": [-122.4, 37.8]}`, nil)
	if err != nil {
		return err
	}
	// The source keeps its own reference to the data.
	defer data.Close()
	if err := m.Discarding().AddGeoJsonSourceData("points", data); err != nil {
		return err
	}

	// Later, prepare new data with the same options, and set it.
	next, err := maplibre.NewGeoJsonSourceData(`{"type": "Point", "coordinates": [2.35, 48.86]}`, nil)
	if err != nil {
		return err
	}
	defer next.Close()
	if err := m.Discarding().SetGeoJsonSourceData("points", next); err != nil {
		return err
	}
	// #endregion geojson

	// #region set-property
	// Show both changes in the same frame.
	if err := m.Discarding().BeginCommandGroup(); err != nil {
		return err
	}
	if err := m.Discarding().SetStyleLayerProperty("satellite", "visibility", `"none"`); err != nil {
		return err
	}
	err = m.Discarding().SetStyleLayerProperty("countries-boundary", "line-color",
		`["interpolate", ["linear"], ["zoom"], 5, "#9cf", 12, "#06c"]`)
	if err != nil {
		return err
	}
	if err := m.Discarding().EndCommandGroup(); err != nil {
		return err
	}
	// #endregion set-property

	// #region image
	image := maplibre.PremultipliedRgba8Image{Width: 16, Height: 16, Stride: 16 * 4, Pixels: pixels}
	if err := m.Discarding().SetStyleImage("marker", image, nil); err != nil {
		return err
	}

	// An empty layer ID draws the layer on top.
	err = m.Discarding().AddStyleLayerJson(`{
		"id": "markers",
		"type": "symbol",
		"source": "points",
		"layout": {"icon-image": "marker"}
	}`, "")
	if err != nil {
		return err
	}
	// #endregion image
	return nil
}

func styleEvents(runtime *maplibre.RuntimeHandle, m *maplibre.MapHandle) error {
	// #region missing-image
	err := runtime.Discarding().SetEventHandler(maplibre.RuntimeEventHandler{
		// Runs on a library thread.
		Callback: func(events []maplibre.RuntimeEvent) {
			for _, event := range events {
				if event.Source != m.Id() || event.Type != maplibre.RuntimeEventTypeMapStyleImageMissing {
					continue
				}
				// The message holds the image's ID.
				err := m.Discarding().SetStyleImage(event.Message, drawImage(event.Message), nil)
				if err != nil {
					log.Print(err)
				}
			}
		},
	})
	if err != nil {
		return err
	}
	// #endregion missing-image
	return nil
}

// An example without an output comment compiles but never runs.
func Example_style() {
	_ = style(nil, nil)
	_ = styleEvents(nil, nil)
}
