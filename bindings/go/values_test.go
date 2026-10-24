package maplibre

import (
	"errors"
	"slices"
	"strings"
	"testing"
)

// Host strings cross as views, so native sees and rejects a NUL in a URL, and
// native-lent strings decode from terminated storage.
func TestHostStringsUseViewsAndNativeLentStringsDecode(t *testing.T) {
	f := newFixture(t)
	if _, err := f.m.SetStyleUrl("custom://style\x00.json"); !errors.Is(err, StatusInvalidArgument) {
		t.Fatalf("SetStyleUrl with a NUL = %v, want StatusInvalidArgument", err)
	}
	const url = "custom://named-style.json"
	f.serveStyle(t, url)
	if _, err := f.m.SetStyleUrl(url); err != nil {
		t.Fatal(err)
	}
	if got := await(t, submitted(f.m.GetStyleUrl())); got != url {
		t.Fatalf("GetStyleUrl() = %q, want %q", got, url)
	}
}

// Optional inputs cross only when present, and optional outputs come back nil
// when native reports them absent.
func TestPresenceFieldsRoundTrip(t *testing.T) {
	f := newFixture(t)
	await(t, submitted(f.m.SetStyleJson([]byte(emptyStyle))))
	fit := StyleImageTextFitProportional
	await(t, submitted(f.m.SetStyleImage("patch", PremultipliedRgba8Image{Width: 2, Height: 2, Stride: 8, Pixels: make([]byte, 16)}, &StyleImageOptions{
		StretchX:      []ImageStretch{{From: 0, To: 1}},
		Content:       &ImageContent{Left: 0.5, Top: 0.5, Right: 1.5, Bottom: 1.5},
		TextFitHeight: &fit,
	})))
	image := await(t, submitted(f.m.GetStyleImage("patch")))
	if image == nil {
		t.Fatal("GetStyleImage(patch) found nothing")
	}
	if image.Content == nil || *image.Content != (ImageContent{Left: 0.5, Top: 0.5, Right: 1.5, Bottom: 1.5}) {
		t.Fatalf("Content = %+v, want the one set", image.Content)
	}
	if image.TextFitWidth != nil {
		t.Fatalf("TextFitWidth = %v, want absent", *image.TextFitWidth)
	}
	if image.TextFitHeight == nil || *image.TextFitHeight != fit {
		t.Fatalf("TextFitHeight = %v, want proportional", image.TextFitHeight)
	}
	if missing := await(t, submitted(f.m.GetStyleImage("missing"))); missing != nil {
		t.Fatalf("GetStyleImage(missing) = %+v, want nil", missing)
	}

	// Go cannot default a struct field to true, so the placement switch is an
	// optional field too: a literal that sets only a duration leaves it on.
	duration := 250.0
	await(t, submitted(f.m.SetStyleTransitionOptions(StyleTransitionOptions{DurationMs: &duration})))
	transitions := await(t, submitted(f.m.GetStyleTransitionOptions()))
	if transitions.DurationMs == nil || *transitions.DurationMs != duration || transitions.DelayMs != nil {
		t.Fatalf("transition options = %+v, want only the duration set", transitions)
	}
	if transitions.EnablePlacementTransitions == nil || !*transitions.EnablePlacementTransitions {
		t.Fatal("a duration-only literal turned the placement cross-fade off")
	}
}

// Array and byte inputs are copied when the call submits them, so changing
// them afterwards leaves what native received alone.
func TestArrayInputsAreCopiedAtSubmission(t *testing.T) {
	f := newFixture(t)
	await(t, submitted(f.m.SetStyleJson([]byte(emptyStyle))))
	coordinates := []LatLng{
		{Latitude: 1, Longitude: 1},
		{Latitude: 1, Longitude: 2},
		{Latitude: 0, Longitude: 2},
		{Latitude: 0, Longitude: 1},
	}
	pixels := []byte{0, 255, 0, 255}
	image := PremultipliedRgba8Image{Width: 1, Height: 1, Stride: 4, Pixels: pixels}
	added, err := f.m.AddImageSourceImage("image", coordinates, image)
	marker, markerErr := f.m.SetStyleImage("marker", image, nil)
	coordinates[0] = LatLng{Latitude: 9, Longitude: 9}
	pixels[1] = 0
	await(t, submitted(added, err))
	await(t, submitted(marker, markerErr))

	if got := await(t, submitted(f.m.GetImageSourceCoordinates("image"))); len(got) != 4 || got[0] != (LatLng{Latitude: 1, Longitude: 1}) {
		t.Fatalf("image source coordinates = %v, want the submitted ones", got)
	}
	if got := await(t, submitted(f.m.GetStyleImage("marker"))); got == nil || !slices.Equal(got.Pixels, []byte{0, 255, 0, 255}) {
		t.Fatalf("style image = %+v, want the submitted pixels", got)
	}
}

func TestSynchronousCallsFillCallerSizedOutputsInInputOrder(t *testing.T) {
	f := newFixture(t)
	snapshot, err := f.m.GetSnapshot()
	if err != nil {
		t.Fatal(err)
	}
	projection, err := MapProjectionCreate(snapshot.View)
	if err != nil {
		t.Fatal(err)
	}
	defer projection.Close()
	coordinates := []LatLng{{Latitude: 0, Longitude: 0}, {Latitude: 10, Longitude: 20}, {Latitude: -12, Longitude: -30}}
	points, err := projection.PixelsForLatLngs(coordinates)
	if err != nil {
		t.Fatal(err)
	}
	if len(points) != len(coordinates) {
		t.Fatalf("got %d points", len(points))
	}
	wrapped, err := projection.LatLngsForPixels(points)
	if err != nil {
		t.Fatal(err)
	}
	unwrapped, err := projection.LatLngsForPixelsUnwrapped(points)
	if err != nil {
		t.Fatal(err)
	}
	for index, coordinate := range coordinates {
		point, err := projection.PixelForLatLng(coordinate)
		if err != nil || point != points[index] {
			t.Fatalf("point %d: %+v, %v", index, point, err)
		}
		single, err := projection.LatLngForPixel(point)
		if err != nil || single != wrapped[index] {
			t.Fatalf("coordinate %d: %+v, %v", index, single, err)
		}
		single, err = projection.LatLngForPixelUnwrapped(point)
		if err != nil || single != unwrapped[index] {
			t.Fatalf("unwrapped coordinate %d: %+v, %v", index, single, err)
		}
	}
	emptyPoints, err := projection.PixelsForLatLngs(nil)
	if err != nil || len(emptyPoints) != 0 {
		t.Fatalf("empty points: %+v, %v", emptyPoints, err)
	}
	emptyCoordinates, err := projection.LatLngsForPixels(nil)
	if err != nil || len(emptyCoordinates) != 0 {
		t.Fatalf("empty coordinates: %+v, %v", emptyCoordinates, err)
	}
}

func TestCallerSizedTextReturnsResultsLongerThanItsFirstStorage(t *testing.T) {
	zoom, err := StyleExpressionCreate([]byte(`["interpolate", ["linear"], ["zoom"], 0, 0, 10, 100]`), nil)
	if err != nil {
		t.Fatal(err)
	}
	defer zoom.Close()
	level := 5.0
	got, err := zoom.Evaluate(&StyleExpressionInputs{Zoom: &level})
	if err != nil {
		t.Fatal(err)
	}
	if string(got) != "50.0" {
		t.Fatalf("zoom result = %s, want 50.0", got)
	}

	// The result outgrows the runtime's first storage, so the call runs again.
	text := `"` + strings.Repeat("a", 1000) + `"`
	long, err := StyleExpressionCreate([]byte(text), nil)
	if err != nil {
		t.Fatal(err)
	}
	defer long.Close()
	got, err = long.Evaluate(nil)
	if err != nil {
		t.Fatal(err)
	}
	if string(got) != text {
		t.Fatalf("long result has %d bytes, want %d", len(got), len(text))
	}
}
