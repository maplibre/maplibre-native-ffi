package maplibre

import (
	"errors"
	"slices"
	"testing"
)

// A string the C API reads NUL-terminated rejects an embedded NUL before the
// call, and one it reads as an explicit-length view carries the NUL to native,
// which decides what it means.
func TestStringsCrossInTheirDeclaredShape(t *testing.T) {
	asset := "asset\x00root"
	options := DefaultRuntimeOptions()
	options.AssetPath = &asset
	if _, err := RuntimeCreate(options); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("RuntimeCreate with a NUL in AssetPath = %v, want ErrInvalidArgument", err)
	}

	f := newFixture(t)
	if _, err := f.m.SetStyleUrl("custom://style\x00.json"); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("SetStyleUrl with a NUL = %v, want ErrInvalidArgument", err)
	}
	if completion := await(t, submitted(f.m.SetStyleJson([]byte("{\x00}")))); completion.Disposition != CommandDispositionFailed {
		t.Fatalf("SetStyleJson with a NUL = %+v, want native's parse failure", completion)
	}
	const url = "custom://named-style.json"
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
	awaitCommitted(t, submitted(f.m.SetStyleJson([]byte(emptyStyle))))
	fit := StyleImageTextFitProportional
	awaitCommitted(t, submitted(f.m.SetStyleImage("patch", PremultipliedRgba8Image{Width: 2, Height: 2, Stride: 8, Pixels: make([]byte, 16)}, &StyleImageOptions{
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
	if !slices.Equal(image.StretchX, []ImageStretch{{From: 0, To: 1}}) || len(image.StretchY) != 0 {
		t.Fatalf("stretches = %v, %v, want the one X interval", image.StretchX, image.StretchY)
	}
	if missing := await(t, submitted(f.m.GetStyleImage("missing"))); missing != nil {
		t.Fatalf("GetStyleImage(missing) = %+v, want nil", missing)
	}

	// Go cannot default a struct field to true, so the placement switch is an
	// optional field too: a literal that sets only a duration leaves it on.
	duration := 250.0
	awaitCommitted(t, submitted(f.m.SetStyleTransitionOptions(StyleTransitionOptions{DurationMs: &duration})))
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
	awaitCommitted(t, submitted(f.m.SetStyleJson([]byte(emptyStyle))))
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
	awaitCommitted(t, submitted(added, err))
	awaitCommitted(t, submitted(marker, markerErr))

	if got := await(t, submitted(f.m.GetImageSourceCoordinates("image"))); len(got) != 4 || got[0] != (LatLng{Latitude: 1, Longitude: 1}) {
		t.Fatalf("image source coordinates = %v, want the submitted ones", got)
	}
	if got := await(t, submitted(f.m.GetStyleImage("marker"))); got == nil || !slices.Equal(got.Pixels, []byte{0, 255, 0, 255}) {
		t.Fatalf("style image = %+v, want the submitted pixels", got)
	}
}
