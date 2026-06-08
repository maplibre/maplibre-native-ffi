package maplibre

import (
	"errors"
	"testing"
)

func TestQueryDescriptorsAndHandleErrors(t *testing.T) {
	point := RenderedQueryPoint(ScreenPoint{X: 1, Y: 2})
	if point.Type != RenderedQueryGeometryTypePoint || point.Point != (ScreenPoint{X: 1, Y: 2}) {
		t.Fatalf("RenderedQueryPoint() = %+v, want point descriptor", point)
	}
	box := RenderedQueryBox(ScreenBox{Min: ScreenPoint{X: 1, Y: 2}, Max: ScreenPoint{X: 3, Y: 4}})
	if box.Type != RenderedQueryGeometryTypeBox || box.Box.Max != (ScreenPoint{X: 3, Y: 4}) {
		t.Fatalf("RenderedQueryBox() = %+v, want box descriptor", box)
	}
	line := RenderedQueryLineString([]ScreenPoint{{X: 1, Y: 2}, {X: 3, Y: 4}})
	if line.Type != RenderedQueryGeometryTypeLineString || len(line.Points) != 2 {
		t.Fatalf("RenderedQueryLineString() = %+v, want copied line descriptor", line)
	}
	var session *RenderSessionHandle
	if _, err := session.QueryRenderedFeatures(point, &RenderedFeatureQueryOptions{LayerIDs: []string{"layer"}, Filter: []any{"==", "kind", "park"}}); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("QueryRenderedFeatures(nil session) error = %v, want ErrInvalidArgument", err)
	}
	if _, err := session.QuerySourceFeatures("source", &SourceFeatureQueryOptions{SourceLayerIDs: []string{"layer"}, Filter: []any{"==", "kind", "park"}}); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("QuerySourceFeatures(nil session) error = %v, want ErrInvalidArgument", err)
	}
	featureID := "id"
	stateKey := "selected"
	selector := FeatureStateSelector{SourceID: "source", FeatureID: &featureID, StateKey: &stateKey}
	if err := session.SetFeatureState(selector, map[string]any{"hover": true}); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetFeatureState(nil session) error = %v, want ErrInvalidArgument", err)
	}
	if _, err := session.FeatureState(selector); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("FeatureState(nil session) error = %v, want ErrInvalidArgument", err)
	}
	if err := session.RemoveFeatureState(selector); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("RemoveFeatureState(nil session) error = %v, want ErrInvalidArgument", err)
	}
	feature := Feature{Geometry: PointGeometry(LatLng{Latitude: 1, Longitude: 2}), Properties: map[string]any{"kind": "park"}, Identifier: "id"}
	if _, err := session.QueryFeatureExtensions("source", feature, "supercluster", "children", map[string]any{"limit": uint64(1)}); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("QueryFeatureExtensions(nil session) error = %v, want ErrInvalidArgument", err)
	}
}

func TestFeatureExtensionUnknownResultTypePreserved(t *testing.T) {
	result := featureExtensionResultForTest(0x7fff_0002)
	if result.Type != FeatureExtensionResultType(0x7fff_0002) {
		t.Fatalf("result type = %d, want unknown raw type", result.Type)
	}
	if result.Value != nil || result.Features != nil {
		t.Fatalf("unknown result data = (%v, %v), want empty", result.Value, result.Features)
	}
}
