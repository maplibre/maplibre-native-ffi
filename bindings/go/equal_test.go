package maplibre

import "testing"

// BND-070: option descriptors compare by field value rather than pointer identity. Each case
// lists one mutator per declared field, so a field left out of Equal fails its mutator assertion.

func optionPtr[T any](value T) *T {
	return &value
}

func assertValueSemantics[T any](
	t *testing.T,
	name string,
	baseline func() T,
	equal func(T, T) bool,
	mutators []func(*T),
) {
	t.Helper()

	if !equal(baseline(), baseline()) {
		t.Fatalf("%s: descriptors with identical fields are not equal", name)
	}
	for index, mutate := range mutators {
		mutated := baseline()
		mutate(&mutated)
		if equal(baseline(), mutated) {
			t.Errorf("%s: field %d is missing from Equal", name, index)
		}
	}
}

func TestMapOptionsEqualComparesFieldValues(t *testing.T) {
	assertValueSemantics(
		t,
		"MapOptions",
		func() MapOptions {
			return MapOptions{
				Width:           100,
				Height:          200,
				ScaleFactor:     2.0,
				Mode:            MapModeContinuous,
				FastPFOREnabled: false,
				EventMask:       RuntimeEventMaskAllMapEvents,
			}
		},
		MapOptions.Equal,
		[]func(*MapOptions){
			func(o *MapOptions) { o.Width = 300 },
			func(o *MapOptions) { o.Height = 400 },
			func(o *MapOptions) { o.ScaleFactor = 3.0 },
			func(o *MapOptions) { o.Mode = MapModeStatic },
			func(o *MapOptions) { o.FastPFOREnabled = true },
			func(o *MapOptions) { o.EventMask = RuntimeEventMaskAll },
		},
	)
}

func TestCameraOptionsEqualComparesFieldValues(t *testing.T) {
	assertValueSemantics(
		t,
		"CameraOptions",
		func() CameraOptions {
			return CameraOptions{
				Center:         optionPtr(LatLng{Latitude: 1, Longitude: 2}),
				CenterAltitude: optionPtr(3.0),
				Padding:        optionPtr(EdgeInsets{Top: 4, Left: 5, Bottom: 6, Right: 7}),
				Anchor:         optionPtr(ScreenPoint{X: 8, Y: 9}),
				Zoom:           optionPtr(10.0),
				Bearing:        optionPtr(11.0),
				Pitch:          optionPtr(12.0),
				Roll:           optionPtr(13.0),
				FieldOfView:    optionPtr(14.0),
			}
		},
		CameraOptions.Equal,
		[]func(*CameraOptions){
			func(o *CameraOptions) { o.Center = optionPtr(LatLng{Latitude: 90, Longitude: 90}) },
			func(o *CameraOptions) { o.CenterAltitude = optionPtr(300.0) },
			func(o *CameraOptions) { o.Padding = optionPtr(EdgeInsets{}) },
			func(o *CameraOptions) { o.Anchor = optionPtr(ScreenPoint{X: 80, Y: 90}) },
			func(o *CameraOptions) { o.Zoom = optionPtr(100.0) },
			func(o *CameraOptions) { o.Bearing = optionPtr(110.0) },
			func(o *CameraOptions) { o.Pitch = optionPtr(120.0) },
			func(o *CameraOptions) { o.Roll = optionPtr(130.0) },
			func(o *CameraOptions) { o.FieldOfView = optionPtr(140.0) },
		},
	)
}

func TestAnimationOptionsEqualComparesFieldValues(t *testing.T) {
	assertValueSemantics(
		t,
		"AnimationOptions",
		func() AnimationOptions {
			return AnimationOptions{
				DurationMS:   optionPtr(1.0),
				Velocity:     optionPtr(2.0),
				MinZoom:      optionPtr(3.0),
				Easing:       optionPtr(UnitBezier{X1: 0.1, Y1: 0.2, X2: 0.3, Y2: 0.4}),
				TransitionID: optionPtr(uint64(4)),
			}
		},
		AnimationOptions.Equal,
		[]func(*AnimationOptions){
			func(o *AnimationOptions) { o.DurationMS = optionPtr(10.0) },
			func(o *AnimationOptions) { o.Velocity = optionPtr(20.0) },
			func(o *AnimationOptions) { o.MinZoom = optionPtr(30.0) },
			func(o *AnimationOptions) {
				o.Easing = optionPtr(UnitBezier{X1: 0.9, Y1: 0.8, X2: 0.7, Y2: 0.6})
			},
			func(o *AnimationOptions) { o.TransitionID = optionPtr(uint64(40)) },
		},
	)
}

func TestCameraFitOptionsEqualComparesFieldValues(t *testing.T) {
	assertValueSemantics(
		t,
		"CameraFitOptions",
		func() CameraFitOptions {
			return CameraFitOptions{
				Padding: optionPtr(EdgeInsets{Top: 1, Left: 2, Bottom: 3, Right: 4}),
				Bearing: optionPtr(5.0),
				Pitch:   optionPtr(6.0),
			}
		},
		CameraFitOptions.Equal,
		[]func(*CameraFitOptions){
			func(o *CameraFitOptions) { o.Padding = optionPtr(EdgeInsets{}) },
			func(o *CameraFitOptions) { o.Bearing = optionPtr(50.0) },
			func(o *CameraFitOptions) { o.Pitch = optionPtr(60.0) },
		},
	)
}

func TestBoundOptionsEqualComparesFieldValues(t *testing.T) {
	assertValueSemantics(
		t,
		"BoundOptions",
		func() BoundOptions {
			return BoundOptions{
				Bounds: optionPtr(BoundsConstraint{
					Kind: BoundsConstraintBounded,
					Bounds: LatLngBounds{
						Southwest: LatLng{Latitude: 0, Longitude: 0},
						Northeast: LatLng{Latitude: 1, Longitude: 1},
					},
				}),
				MinZoom:  optionPtr(2.0),
				MaxZoom:  optionPtr(3.0),
				MinPitch: optionPtr(4.0),
				MaxPitch: optionPtr(5.0),
			}
		},
		BoundOptions.Equal,
		[]func(*BoundOptions){
			func(o *BoundOptions) {
				o.Bounds = optionPtr(BoundsConstraint{
					Kind: BoundsConstraintBounded,
					Bounds: LatLngBounds{
						Southwest: LatLng{Latitude: -1, Longitude: -1},
						Northeast: LatLng{Latitude: 2, Longitude: 2},
					},
				})
			},
			func(o *BoundOptions) {
				o.Bounds = optionPtr(BoundsConstraint{Kind: BoundsConstraintUnbounded})
			},
			func(o *BoundOptions) { o.MinZoom = optionPtr(20.0) },
			func(o *BoundOptions) { o.MaxZoom = optionPtr(30.0) },
			func(o *BoundOptions) { o.MinPitch = optionPtr(40.0) },
			func(o *BoundOptions) { o.MaxPitch = optionPtr(50.0) },
		},
	)
}

func TestFreeCameraOptionsEqualComparesFieldValues(t *testing.T) {
	assertValueSemantics(
		t,
		"FreeCameraOptions",
		func() FreeCameraOptions {
			return FreeCameraOptions{
				Position:    optionPtr(Vec3{X: 1, Y: 2, Z: 3}),
				Orientation: optionPtr(Quaternion{X: 0, Y: 0, Z: 0, W: 1}),
			}
		},
		FreeCameraOptions.Equal,
		[]func(*FreeCameraOptions){
			func(o *FreeCameraOptions) { o.Position = optionPtr(Vec3{X: 9, Y: 9, Z: 9}) },
			func(o *FreeCameraOptions) {
				o.Orientation = optionPtr(Quaternion{X: 1, Y: 0, Z: 0, W: 0})
			},
		},
	)
}

func TestViewportOptionsEqualComparesFieldValues(t *testing.T) {
	assertValueSemantics(
		t,
		"ViewportOptions",
		func() ViewportOptions {
			return ViewportOptions{
				NorthOrientation: optionPtr(NorthOrientationUp),
				ConstrainMode:    optionPtr(ConstrainModeNone),
				ViewportMode:     optionPtr(ViewportModeDefault),
				FrustumOffset:    optionPtr(EdgeInsets{Top: 1, Left: 2, Bottom: 3, Right: 4}),
			}
		},
		ViewportOptions.Equal,
		[]func(*ViewportOptions){
			func(o *ViewportOptions) { o.NorthOrientation = optionPtr(NorthOrientationDown) },
			func(o *ViewportOptions) { o.ConstrainMode = optionPtr(ConstrainModeScreen) },
			func(o *ViewportOptions) { o.ViewportMode = optionPtr(ViewportModeFlippedY) },
			func(o *ViewportOptions) { o.FrustumOffset = optionPtr(EdgeInsets{}) },
		},
	)
}

func TestTileOptionsEqualComparesFieldValues(t *testing.T) {
	assertValueSemantics(
		t,
		"TileOptions",
		func() TileOptions {
			return TileOptions{
				PrefetchZoomDelta: optionPtr(uint32(1)),
				LODMinRadius:      optionPtr(2.0),
				LODScale:          optionPtr(3.0),
				LODPitchThreshold: optionPtr(4.0),
				LODZoomShift:      optionPtr(5.0),
				LODMode:           optionPtr(TileLODModeDefault),
			}
		},
		TileOptions.Equal,
		[]func(*TileOptions){
			func(o *TileOptions) { o.PrefetchZoomDelta = optionPtr(uint32(7)) },
			func(o *TileOptions) { o.LODMinRadius = optionPtr(20.0) },
			func(o *TileOptions) { o.LODScale = optionPtr(30.0) },
			func(o *TileOptions) { o.LODPitchThreshold = optionPtr(40.0) },
			func(o *TileOptions) { o.LODZoomShift = optionPtr(50.0) },
			func(o *TileOptions) { o.LODMode = optionPtr(TileLODModeDistance) },
		},
	)
}

func TestProjectionModeOptionsEqualComparesFieldValues(t *testing.T) {
	assertValueSemantics(
		t,
		"ProjectionModeOptions",
		func() ProjectionModeOptions {
			return ProjectionModeOptions{
				Axonometric: optionPtr(true),
				XSkew:       optionPtr(1.0),
				YSkew:       optionPtr(2.0),
			}
		},
		ProjectionModeOptions.Equal,
		[]func(*ProjectionModeOptions){
			func(o *ProjectionModeOptions) { o.Axonometric = optionPtr(false) },
			func(o *ProjectionModeOptions) { o.XSkew = optionPtr(10.0) },
			func(o *ProjectionModeOptions) { o.YSkew = optionPtr(20.0) },
		},
	)
}

func TestRuntimeOptionsEqualComparesFieldValues(t *testing.T) {
	assertValueSemantics(
		t,
		"RuntimeOptions",
		func() RuntimeOptions {
			return RuntimeOptions{
				AssetPath: "assets",
				CachePath: "cache",
				EventMask: RuntimeEventMaskAllRuntimeEvents,
			}
		},
		RuntimeOptions.Equal,
		[]func(*RuntimeOptions){
			func(o *RuntimeOptions) { o.AssetPath = "other-assets" },
			func(o *RuntimeOptions) { o.CachePath = "other-cache" },
			func(o *RuntimeOptions) { o.EventMask = RuntimeEventMaskAll },
		},
	)
}

func TestStyleTileSourceOptionsEqualComparesFieldValues(t *testing.T) {
	assertValueSemantics(
		t,
		"StyleTileSourceOptions",
		func() StyleTileSourceOptions {
			return StyleTileSourceOptions{
				MinZoom:     optionPtr(1.0),
				MaxZoom:     optionPtr(2.0),
				Attribution: optionPtr("attribution"),
				Scheme:      optionPtr(StyleTileSchemeXYZ),
				Bounds: optionPtr(LatLngBounds{
					Southwest: LatLng{Latitude: 0, Longitude: 0},
					Northeast: LatLng{Latitude: 1, Longitude: 1},
				}),
				TileSize:       optionPtr(uint32(256)),
				VectorEncoding: optionPtr(StyleVectorTileEncodingMVT),
				RasterEncoding: optionPtr(StyleRasterDEMEncodingMapbox),
			}
		},
		StyleTileSourceOptions.Equal,
		[]func(*StyleTileSourceOptions){
			func(o *StyleTileSourceOptions) { o.MinZoom = optionPtr(10.0) },
			func(o *StyleTileSourceOptions) { o.MaxZoom = optionPtr(20.0) },
			func(o *StyleTileSourceOptions) { o.Attribution = optionPtr("other") },
			func(o *StyleTileSourceOptions) { o.Scheme = optionPtr(StyleTileSchemeTMS) },
			func(o *StyleTileSourceOptions) {
				o.Bounds = optionPtr(LatLngBounds{
					Southwest: LatLng{Latitude: -1, Longitude: -1},
					Northeast: LatLng{Latitude: 2, Longitude: 2},
				})
			},
			func(o *StyleTileSourceOptions) { o.TileSize = optionPtr(uint32(512)) },
			func(o *StyleTileSourceOptions) {
				o.VectorEncoding = optionPtr(StyleVectorTileEncodingMLT)
			},
			func(o *StyleTileSourceOptions) {
				o.RasterEncoding = optionPtr(StyleRasterDEMEncodingTerrarium)
			},
		},
	)
}

func TestStyleGeoJSONSourceOptionsEqualComparesFieldValues(t *testing.T) {
	assertValueSemantics(
		t,
		"StyleGeoJSONSourceOptions",
		func() StyleGeoJSONSourceOptions {
			return StyleGeoJSONSourceOptions{
				MinZoom:           optionPtr(1.0),
				MaxZoom:           optionPtr(2.0),
				Tolerance:         optionPtr(0.5),
				ClusterMaxZoom:    optionPtr(14.0),
				ClusterProperties: []byte(`{"total":["+",["get","rank"]]}`),
				TileSize:          optionPtr(uint32(256)),
				Buffer:            optionPtr(uint32(64)),
				ClusterRadius:     optionPtr(uint32(0)),
				ClusterMinPoints:  optionPtr(uint32(2)),
				LineMetrics:       optionPtr(true),
				Cluster:           optionPtr(true),
				SynchronousTiling: optionPtr(true),
			}
		},
		StyleGeoJSONSourceOptions.Equal,
		[]func(*StyleGeoJSONSourceOptions){
			func(o *StyleGeoJSONSourceOptions) { o.MinZoom = optionPtr(10.0) },
			func(o *StyleGeoJSONSourceOptions) { o.MaxZoom = optionPtr(20.0) },
			func(o *StyleGeoJSONSourceOptions) { o.Tolerance = optionPtr(0.25) },
			func(o *StyleGeoJSONSourceOptions) { o.ClusterMaxZoom = optionPtr(15.0) },
			func(o *StyleGeoJSONSourceOptions) {
				o.ClusterProperties = []byte(`{"total":["+",["get","score"]]}`)
			},
			func(o *StyleGeoJSONSourceOptions) { o.TileSize = optionPtr(uint32(512)) },
			func(o *StyleGeoJSONSourceOptions) { o.Buffer = optionPtr(uint32(128)) },
			func(o *StyleGeoJSONSourceOptions) { o.ClusterRadius = optionPtr(uint32(50)) },
			func(o *StyleGeoJSONSourceOptions) { o.ClusterMinPoints = optionPtr(uint32(3)) },
			func(o *StyleGeoJSONSourceOptions) { o.LineMetrics = optionPtr(false) },
			func(o *StyleGeoJSONSourceOptions) { o.Cluster = optionPtr(false) },
			func(o *StyleGeoJSONSourceOptions) { o.SynchronousTiling = optionPtr(false) },
		},
	)
}

func TestStyleGeoJSONSourceOptionsBuildersDeepCopyRetainedFields(t *testing.T) {
	original := StyleGeoJSONSourceOptions{}.
		WithMinZoom(1).
		WithClusterProperties([]byte(`{"total":["+",1]}`))
	updated := original.WithMaxZoom(2)

	*updated.MinZoom = 9
	updated.ClusterProperties[0] = 'x'

	if *original.MinZoom != 1 {
		t.Fatalf("original MinZoom = %v, want 1", *original.MinZoom)
	}
	if got := string(original.ClusterProperties); got != `{"total":["+",1]}` {
		t.Fatalf("original cluster properties = %q", got)
	}
}

func TestStyleImageOptionsEqualComparesFieldValues(t *testing.T) {
	assertValueSemantics(
		t,
		"StyleImageOptions",
		func() StyleImageOptions {
			return StyleImageOptions{
				PixelRatio:    optionPtr(float32(2)),
				SDF:           optionPtr(true),
				StretchX:      []ImageStretch{{From: 0, To: 1}},
				StretchY:      []ImageStretch{},
				Content:       optionPtr(ImageContent{Left: 0.5, Top: 0.5, Right: 1.5, Bottom: 1.5}),
				TextFitWidth:  optionPtr(StyleImageTextFitStretchOnly),
				TextFitHeight: optionPtr(StyleImageTextFitProportional),
			}
		},
		StyleImageOptions.Equal,
		[]func(*StyleImageOptions){
			func(o *StyleImageOptions) { o.PixelRatio = optionPtr(float32(3)) },
			func(o *StyleImageOptions) { o.SDF = optionPtr(false) },
			func(o *StyleImageOptions) { o.StretchX = []ImageStretch{{From: 0, To: 2}} },
			// A present empty slice stays distinguishable from an absent one.
			func(o *StyleImageOptions) { o.StretchY = nil },
			func(o *StyleImageOptions) { o.Content = nil },
			func(o *StyleImageOptions) { o.TextFitWidth = optionPtr(StyleImageTextFitProportional) },
			func(o *StyleImageOptions) {
				o.TextFitHeight = optionPtr(StyleImageTextFitStretchOrShrink)
			},
		},
	)
}

func TestStyleTransitionOptionsEqualComparesFieldValues(t *testing.T) {
	assertValueSemantics(
		t,
		"StyleTransitionOptions",
		func() StyleTransitionOptions {
			return StyleTransitionOptions{
				DurationMS:                 optionPtr(300.0),
				DelayMS:                    optionPtr(0.0),
				EnablePlacementTransitions: optionPtr(false),
			}
		},
		StyleTransitionOptions.Equal,
		[]func(*StyleTransitionOptions){
			func(o *StyleTransitionOptions) { o.DurationMS = optionPtr(500.0) },
			// A present zero stays distinguishable from an absent field.
			func(o *StyleTransitionOptions) { o.DelayMS = nil },
			// A present false stays distinguishable from an absent field.
			func(o *StyleTransitionOptions) { o.EnablePlacementTransitions = nil },
		},
	)

	original := StyleTransitionOptions{DurationMS: optionPtr(300.0)}
	cloned := original.Clone()
	*cloned.DurationMS = 500
	if *original.DurationMS != 300 {
		t.Fatalf("original DurationMS = %v, want 300", *original.DurationMS)
	}
}

func TestQueryOptionsEqualComparesLayerIDsElementByElement(t *testing.T) {
	assertValueSemantics(
		t,
		"RenderedFeatureQueryOptions",
		func() RenderedFeatureQueryOptions {
			return RenderedFeatureQueryOptions{
				LayerIDs: []string{"a", "b"},
				Filter:   []byte("true"),
			}
		},
		RenderedFeatureQueryOptions.Equal,
		[]func(*RenderedFeatureQueryOptions){
			func(o *RenderedFeatureQueryOptions) { o.LayerIDs = []string{"a"} },
			func(o *RenderedFeatureQueryOptions) { o.Filter = []byte(`"filter"`) },
		},
	)
	assertValueSemantics(
		t,
		"SourceFeatureQueryOptions",
		func() SourceFeatureQueryOptions {
			return SourceFeatureQueryOptions{
				SourceLayerIDs: []string{"a", "b"},
				Filter:         []byte("true"),
			}
		},
		SourceFeatureQueryOptions.Equal,
		[]func(*SourceFeatureQueryOptions){
			func(o *SourceFeatureQueryOptions) { o.SourceLayerIDs = []string{"a"} },
			func(o *SourceFeatureQueryOptions) { o.Filter = []byte(`"filter"`) },
		},
	)
}

func TestQueryOptionsEqualSeparatesAbsentFromEmptyLayerIDs(t *testing.T) {
	// The native field mask distinguishes an absent layer filter from an empty one.
	absent := RenderedFeatureQueryOptions{}
	empty := RenderedFeatureQueryOptions{LayerIDs: []string{}}

	if absent.Equal(empty) {
		t.Error("absent LayerIDs compares equal to an empty LayerIDs list")
	}
}
