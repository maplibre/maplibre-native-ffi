package maplibre

import "testing"

func TestQueryDescriptorsCopyBufferValues(t *testing.T) {
	point := RenderedQueryPoint(ScreenPoint{X: 1, Y: 2})
	if point.Type != RenderedQueryGeometryTypePoint || point.Point != (ScreenPoint{X: 1, Y: 2}) {
		t.Fatalf("RenderedQueryPoint() = %#v", point)
	}

	filter := []byte(`["==","kind","park"]`)
	rendered := RenderedFeatureQueryOptions{LayerIDs: []string{"roads"}, Filter: filter}
	source := SourceFeatureQueryOptions{SourceLayerIDs: []string{"landuse"}, Filter: filter}
	if !rendered.Equal(RenderedFeatureQueryOptions{LayerIDs: []string{"roads"}, Filter: append([]byte(nil), filter...)}) {
		t.Fatal("rendered query options do not compare filters by content")
	}
	if !source.Equal(SourceFeatureQueryOptions{SourceLayerIDs: []string{"landuse"}, Filter: append([]byte(nil), filter...)}) {
		t.Fatal("source query options do not compare filters by content")
	}
}
