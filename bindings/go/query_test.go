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
	filter := JSONArray(JSONString("=="), JSONString("kind"), JSONString("park"))
	if _, err := session.QueryRenderedFeatures(point, &RenderedFeatureQueryOptions{LayerIDs: []string{"layer"}, Filter: &filter}); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("QueryRenderedFeatures(nil session) error = %v, want ErrInvalidArgument", err)
	}
	if _, err := session.QuerySourceFeatures("source", &SourceFeatureQueryOptions{SourceLayerIDs: []string{"layer"}, Filter: &filter}); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("QuerySourceFeatures(nil session) error = %v, want ErrInvalidArgument", err)
	}
	featureID := "id"
	stateKey := "selected"
	selector := FeatureStateSelector{SourceID: "source", FeatureID: &featureID, StateKey: &stateKey}
	state := JSONObject(JSONMember{Name: "hover", Value: JSONBool(true)})
	if err := session.SetFeatureState(selector, state); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetFeatureState(nil session) error = %v, want ErrInvalidArgument", err)
	}
	if _, err := session.FeatureState(selector); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("FeatureState(nil session) error = %v, want ErrInvalidArgument", err)
	}
	if err := session.RemoveFeatureState(selector); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("RemoveFeatureState(nil session) error = %v, want ErrInvalidArgument", err)
	}
	feature := Feature{
		Geometry:   PointGeometry(LatLng{Latitude: 1, Longitude: 2}),
		Properties: JSONMembers{{Name: "kind", Value: JSONString("park")}},
		Identifier: "id",
	}
	arguments := JSONObject(JSONMember{Name: "limit", Value: JSONUint(1)})
	if _, err := session.QueryFeatureExtensions("source", feature, "supercluster", "children", &arguments); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("QueryFeatureExtensions(nil session) error = %v, want ErrInvalidArgument", err)
	}
}

func TestQueryMaterializersPassCgoPointerChecks(t *testing.T) {
	filter := JSONArray(JSONString("=="), JSONString("kind"), JSONString("park"))

	renderedQueryGeometryPassesCgoPointerCheckForTest(RenderedQueryLineString([]ScreenPoint{{X: 1, Y: 2}, {X: 3, Y: 4}}))

	if err := renderedFeatureQueryOptionsPassesCgoPointerCheckForTest(&RenderedFeatureQueryOptions{LayerIDs: []string{"layer"}, Filter: &filter}); err != nil {
		t.Fatalf("rendered feature query options: %v", err)
	}
	if err := sourceFeatureQueryOptionsPassesCgoPointerCheckForTest(&SourceFeatureQueryOptions{SourceLayerIDs: []string{"layer"}, Filter: &filter}); err != nil {
		t.Fatalf("source feature query options: %v", err)
	}
}

func TestFeatureExtensionUnknownResultTypePreserved(t *testing.T) {
	result := featureExtensionResultForTest(0x7fff_0002)
	if result.Type != FeatureExtensionResultType(0x7fff_0002) {
		t.Fatalf("result type = %d, want unknown raw type", result.Type)
	}
	if result.Value.Type != JSONValueTypeNull || len(result.Value.Array) != 0 || len(result.Value.Object) != 0 || result.Features != nil {
		t.Fatalf("unknown result data = (%v, %v), want empty", result.Value, result.Features)
	}
}

func TestQueryOperationsRejectActiveFrame(t *testing.T) {
	session := newActiveFrameTestSession(t)
	featureID := "id"
	stateKey := "selected"
	selector := FeatureStateSelector{SourceID: "source", FeatureID: &featureID, StateKey: &stateKey}
	state := JSONObject(JSONMember{Name: "hover", Value: JSONBool(true)})
	point := RenderedQueryPoint(ScreenPoint{X: 1, Y: 2})
	filter := JSONArray(JSONString("=="), JSONString("kind"), JSONString("park"))
	feature := Feature{
		Geometry:   PointGeometry(LatLng{Latitude: 1, Longitude: 2}),
		Properties: JSONMembers{{Name: "kind", Value: JSONString("park")}},
		Identifier: "id",
	}
	arguments := JSONObject(JSONMember{Name: "limit", Value: JSONUint(1)})

	if err := session.SetFeatureState(selector, state); !errors.Is(err, ErrInvalidState) {
		t.Fatalf("SetFeatureState() error = %v, want ErrInvalidState", err)
	}
	if _, err := session.FeatureState(selector); !errors.Is(err, ErrInvalidState) {
		t.Fatalf("FeatureState() error = %v, want ErrInvalidState", err)
	}
	if err := session.RemoveFeatureState(selector); !errors.Is(err, ErrInvalidState) {
		t.Fatalf("RemoveFeatureState() error = %v, want ErrInvalidState", err)
	}
	if _, err := session.QueryRenderedFeatures(point, &RenderedFeatureQueryOptions{LayerIDs: []string{"layer"}, Filter: &filter}); !errors.Is(err, ErrInvalidState) {
		t.Fatalf("QueryRenderedFeatures() error = %v, want ErrInvalidState", err)
	}
	if _, err := session.QuerySourceFeatures("source", &SourceFeatureQueryOptions{SourceLayerIDs: []string{"layer"}, Filter: &filter}); !errors.Is(err, ErrInvalidState) {
		t.Fatalf("QuerySourceFeatures() error = %v, want ErrInvalidState", err)
	}
	if _, err := session.QueryFeatureExtensions("source", feature, "supercluster", "children", &arguments); !errors.Is(err, ErrInvalidState) {
		t.Fatalf("QueryFeatureExtensions() error = %v, want ErrInvalidState", err)
	}
}
