// Snippets for the docs site's Query page. They compile with the tests, which
// never run them.
package docsnippets

import (
	"context"

	"github.com/maplibre/maplibre-native-ffi/bindings/go/maplibre"
)

func useFeature(geojson string) {}

func query(ctx context.Context, session *maplibre.RenderSessionHandle, point maplibre.ScreenPoint) error {
	// #region rendered
	box := maplibre.RenderedQueryGeometryBoxVariant{Value: maplibre.ScreenBox{
		Min: maplibre.ScreenPoint{X: point.X - 5, Y: point.Y - 5},
		Max: maplibre.ScreenPoint{X: point.X + 5, Y: point.Y + 5},
	}}
	rendered, err := session.QueryRenderedFeatures(box, &maplibre.RenderedFeatureQueryOptions{
		LayerIds: []string{"poi"},
	})
	if err != nil {
		return err
	}
	hits, err := rendered.Await(ctx)
	if err != nil {
		return err
	}
	for _, hit := range hits {
		// hit.Feature is the feature's GeoJSON.
		useFeature(hit.Feature)
	}
	// #endregion rendered

	// #region source
	source, err := session.QuerySourceFeatures("openmaptiles", &maplibre.SourceFeatureQueryOptions{
		SourceLayerIds: []string{"transportation"},
		// A filter from the MapLibre style spec, as JSON.
		Filter: `["==", ["get", "class"], "motorway"]`,
	})
	if err != nil {
		return err
	}
	features, err := source.Await(ctx)
	if err != nil {
		return err
	}
	for _, feature := range features {
		useFeature(feature.Feature)
	}
	// #endregion source
	return nil
}

// An example without an output comment compiles but never runs.
func Example_query() {
	_ = query(context.Background(), nil, maplibre.ScreenPoint{})
}
