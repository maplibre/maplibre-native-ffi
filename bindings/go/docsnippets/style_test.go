// Snippets for the docs site's Style page. They compile with the tests, which
// never run them.
package docsnippets

import (
	"context"
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
func style(ctx context.Context, m *maplibre.MapHandle, pixels []byte) error {
	// #region load
	loaded, err := m.SetStyleUrl(
		"https://demotiles.maplibre.org/style.json",
	)
	if err != nil {
		return err
	}
	if _, err := loaded.Await(ctx); err != nil {
		return err
	}
	// #endregion load

	// #region add
	err = m.Discarding().AddStyleSourceJson("satellite", `{
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
	data, err := maplibre.NewGeoJsonSourceData(
		`{"type": "Point", "coordinates": [-122.4, 37.8]}`, nil,
	)
	if err != nil {
		return err
	}
	// The source keeps its own reference to the data.
	defer data.Close()
	err = m.Discarding().AddGeoJsonSourceData("points", data)
	if err != nil {
		return err
	}

	// Later, prepare new data with the same options, and set it.
	next, err := maplibre.NewGeoJsonSourceData(
		`{"type": "Point", "coordinates": [2.35, 48.86]}`, nil,
	)
	if err != nil {
		return err
	}
	defer next.Close()
	err = m.Discarding().SetGeoJsonSourceData("points", next)
	if err != nil {
		return err
	}
	// #endregion geojson

	// #region set-property
	// Show both changes in the same frame.
	if err := m.Discarding().BeginCommandGroup(); err != nil {
		return err
	}
	err = m.Discarding().SetStyleLayerProperty(
		"satellite", "visibility", `"none"`,
	)
	if err != nil {
		return err
	}
	err = m.Discarding().SetStyleLayerProperty(
		"countries-boundary",
		"line-color",
		`["interpolate", ["linear"], ["zoom"], 5, "#9cf", 12, "#06c"]`,
	)
	if err != nil {
		return err
	}
	if err := m.Discarding().EndCommandGroup(); err != nil {
		return err
	}
	// #endregion set-property

	// #region image
	image := maplibre.PremultipliedRgba8Image{
		Width: 16, Height: 16, Stride: 16 * 4, Pixels: pixels,
	}
	err = m.Discarding().SetStyleImage("marker", image, nil)
	if err != nil {
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
	// Runs on a library thread.
	handleEvents := func(events []maplibre.RuntimeEvent) {
		for _, event := range events {
			if event.Source != m.Id() {
				continue
			}
			missing, ok := event.Payload.(maplibre.RuntimeEventPayloadStyleImageVariant)
			if !ok {
				continue
			}
			id := missing.Value.ImageId
			err := m.Discarding().SetStyleImage(id, drawImage(id), nil)
			if err != nil {
				log.Print(err)
			}
		}
	}
	err := runtime.Discarding().SetEventHandler(
		maplibre.RuntimeEventHandler{Callback: handleEvents},
	)
	if err != nil {
		return err
	}
	// #endregion missing-image
	return nil
}

// An example without an output comment compiles but never runs.
func Example_style() {
	_ = style(context.Background(), nil, nil)
	_ = styleEvents(nil, nil)
}
