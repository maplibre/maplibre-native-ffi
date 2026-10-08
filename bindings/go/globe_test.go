package maplibre

import (
	"encoding/json"
	"testing"
)

func TestRuntimeProjectionControlsOcclusionAndResetsWithTheStyle(t *testing.T) {
	lockOSThreadForTest(t)
	check := func(err error) {
		t.Helper()
		if err != nil {
			t.Fatal(err)
		}
	}
	runtime, err := NewRuntime()
	check(err)
	m, err := runtime.NewMapWithOptions(NewMapOptions(512, 512, 1))
	check(err)
	defer func() { check(m.Close()); check(runtime.Close()) }()
	front := LatLng{Latitude: 0, Longitude: 0}
	back := LatLng{Latitude: 0, Longitude: 180}
	style := []byte(`{"version":8,"sources":{},"layers":[]}`)
	check(m.SetStyleJSON(style))
	check(m.SetStyleProjectionJSON([]byte(`{"type":"globe"}`)))
	snapshot, err := m.StyleProjectionProperty("type")
	check(err)
	check(m.JumpTo(CameraOptions{}.WithCenter(front).WithZoom(0)))
	occluded, err := m.IsLocationOccluded(front)
	check(err)
	if occluded {
		t.Fatal("the camera center is occluded")
	}
	occluded, err = m.IsLocationOccluded(back)
	check(err)
	if !occluded {
		t.Fatal("the globe must hide the far-side location")
	}
	projection, err := m.NewProjection()
	check(err)
	defer func() { check(projection.Close()) }()
	check(m.SetStyleProjectionProperty("type", []byte(`["interpolate",["linear"],["zoom"],1,"vertical-perspective",3,"mercator"]`)))
	check(m.JumpTo(CameraOptions{}.WithZoom(4)))
	occluded, err = m.IsLocationOccluded(back)
	check(err)
	if occluded {
		t.Fatal("the expression should select Mercator at zoom 4")
	}
	check(m.JumpTo(CameraOptions{}.WithZoom(0)))
	occluded, err = m.IsLocationOccluded(back)
	check(err)
	if !occluded {
		t.Fatal("the expression should select the globe at zoom 0")
	}
	check(m.SetStyleProjectionJSON([]byte("{}")))
	occluded, err = m.IsLocationOccluded(back)
	check(err)
	if occluded {
		t.Fatal("the empty projection should restore Mercator")
	}
	check(m.SetStyleProjectionJSON([]byte(`{"type":"globe"}`)))
	check(m.SetStyleJSON(style))
	occluded, err = m.IsLocationOccluded(back)
	check(err)
	if occluded {
		t.Fatal("a style load should replace the projection override")
	}
	check(m.Close())
	occluded, err = projection.IsLocationOccluded(back)
	check(err)
	if !occluded {
		t.Fatal("the snapshot must retain the globe after map closure")
	}
	check(projection.SetCamera(CameraOptions{}.WithCenter(back)))
	occluded, err = projection.IsLocationOccluded(back)
	check(err)
	if occluded {
		t.Fatal("the helper camera center must be visible")
	}
	occluded, err = projection.IsLocationOccluded(front)
	check(err)
	if !occluded {
		t.Fatal("the helper camera must hide the opposite side")
	}
	var copied string
	check(json.Unmarshal(snapshot, &copied))
	if copied != "globe" {
		t.Fatalf("copied projection property = %q", copied)
	}
}
