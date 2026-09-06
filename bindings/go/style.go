package maplibre

/*
#include <stdlib.h>
#include "maplibre_native_c.h"
*/
import "C"

import (
	"runtime"
	"unsafe"

	"github.com/maplibre/maplibre-native-ffi/bindings/go/internal/callback"
)

// StyleSourceType identifies a native style source kind.
type StyleSourceType uint32

const (
	StyleSourceTypeUnknown         StyleSourceType = StyleSourceType(C.MLN_STYLE_SOURCE_TYPE_UNKNOWN)
	StyleSourceTypeVector          StyleSourceType = StyleSourceType(C.MLN_STYLE_SOURCE_TYPE_VECTOR)
	StyleSourceTypeRaster          StyleSourceType = StyleSourceType(C.MLN_STYLE_SOURCE_TYPE_RASTER)
	StyleSourceTypeRasterDEM       StyleSourceType = StyleSourceType(C.MLN_STYLE_SOURCE_TYPE_RASTER_DEM)
	StyleSourceTypeGeoJSON         StyleSourceType = StyleSourceType(C.MLN_STYLE_SOURCE_TYPE_GEOJSON)
	StyleSourceTypeImage           StyleSourceType = StyleSourceType(C.MLN_STYLE_SOURCE_TYPE_IMAGE)
	StyleSourceTypeVideo           StyleSourceType = StyleSourceType(C.MLN_STYLE_SOURCE_TYPE_VIDEO)
	StyleSourceTypeAnnotations     StyleSourceType = StyleSourceType(C.MLN_STYLE_SOURCE_TYPE_ANNOTATIONS)
	StyleSourceTypeCustomVector    StyleSourceType = StyleSourceType(C.MLN_STYLE_SOURCE_TYPE_CUSTOM_VECTOR)
	StyleSourceTypeCustomMVTVector StyleSourceType = StyleSourceType(C.MLN_STYLE_SOURCE_TYPE_CUSTOM_MVT_VECTOR)
)

// StyleSourceInfo contains copied metadata for one style source.
type StyleSourceInfo struct {
	Type            StyleSourceType
	IDSize          uint64
	IsVolatile      bool
	HasAttribution  bool
	AttributionSize uint64
	Attribution     *string
	URL             *string
	TileJSON        *StyleSourceTileJSON
	TileSize        *uint32
	VectorEncoding  *StyleVectorTileEncoding
	RasterEncoding  *StyleRasterDEMEncoding
}

// StyleLayerInfo contains copied metadata for one style layer.
type StyleLayerInfo struct {
	// Type is the style-spec layer type string.
	Type string
	// MinZoom is the lowest zoom at which the layer draws, math.Inf(-1) with no
	// lower bound.
	MinZoom float64
	// MaxZoom is the highest zoom at which the layer draws, math.Inf(1) with no
	// upper bound.
	MaxZoom    float64
	Visibility StyleLayerVisibility
	// HasSourceID reports whether the layer carries a source ID. Copy it with
	// StartLayerSourceID.
	HasSourceID bool
	// SourceIDSize is the source ID byte length, 0 when HasSourceID is false.
	SourceIDSize uint64
	SourceID     *string
	// HasSourceLayer reports whether the layer carries a source-layer ID. Copy
	// it with StartLayerSourceLayer.
	HasSourceLayer bool
	// SourceLayerSize is the source-layer byte length, 0 when HasSourceLayer is
	// false.
	SourceLayerSize uint64
	SourceLayer     *string
}

// StyleSourceTileJSON contains the retained TileJSON fields of an inline tile source.
type StyleSourceTileJSON struct {
	TileURLs []string
	MinZoom  float64
	MaxZoom  float64
	Scheme   StyleTileScheme
	Bounds   *LatLngBounds
}

// StyleTileScheme selects tile URL coordinate scheme.
type StyleTileScheme uint32

const (
	StyleTileSchemeXYZ StyleTileScheme = StyleTileScheme(C.MLN_STYLE_TILE_SCHEME_XYZ)
	StyleTileSchemeTMS StyleTileScheme = StyleTileScheme(C.MLN_STYLE_TILE_SCHEME_TMS)
)

// StyleVectorTileEncoding selects vector tile encoding.
type StyleVectorTileEncoding uint32

const (
	StyleVectorTileEncodingMVT StyleVectorTileEncoding = StyleVectorTileEncoding(C.MLN_STYLE_VECTOR_TILE_ENCODING_MVT)
	StyleVectorTileEncodingMLT StyleVectorTileEncoding = StyleVectorTileEncoding(C.MLN_STYLE_VECTOR_TILE_ENCODING_MLT)
)

// StyleRasterDEMEncoding selects raster DEM tile encoding.
type StyleRasterDEMEncoding uint32

const (
	StyleRasterDEMEncodingMapbox    StyleRasterDEMEncoding = StyleRasterDEMEncoding(C.MLN_STYLE_RASTER_DEM_ENCODING_MAPBOX)
	StyleRasterDEMEncodingTerrarium StyleRasterDEMEncoding = StyleRasterDEMEncoding(C.MLN_STYLE_RASTER_DEM_ENCODING_TERRARIUM)
)

// StyleTileSourceOptions configures vector, raster, and raster DEM sources.
type StyleTileSourceOptions struct {
	MinZoom        *float64
	MaxZoom        *float64
	Attribution    *string
	Scheme         *StyleTileScheme
	Bounds         *LatLngBounds
	TileSize       *uint32
	VectorEncoding *StyleVectorTileEncoding
	RasterEncoding *StyleRasterDEMEncoding
}

// Equal reports whether two descriptors hold the same field values.
func (options StyleTileSourceOptions) Equal(other StyleTileSourceOptions) bool {
	return equalPointer(options.MinZoom, other.MinZoom) &&
		equalPointer(options.MaxZoom, other.MaxZoom) &&
		equalPointer(options.Attribution, other.Attribution) &&
		equalPointer(options.Scheme, other.Scheme) &&
		equalPointer(options.Bounds, other.Bounds) &&
		equalPointer(options.TileSize, other.TileSize) &&
		equalPointer(options.VectorEncoding, other.VectorEncoding) &&
		equalPointer(options.RasterEncoding, other.RasterEncoding)
}

// WithTileSize returns a copy that sets raster tile size.
func (options StyleTileSourceOptions) WithTileSize(tileSize uint32) StyleTileSourceOptions {
	options.TileSize = new(uint32)
	*options.TileSize = tileSize
	return options
}

// WithAttribution returns a copy that sets source attribution.
func (options StyleTileSourceOptions) WithAttribution(attribution string) StyleTileSourceOptions {
	options.Attribution = new(string)
	*options.Attribution = attribution
	return options
}

// WithVectorEncoding returns a copy that sets vector tile encoding.
func (options StyleTileSourceOptions) WithVectorEncoding(encoding StyleVectorTileEncoding) StyleTileSourceOptions {
	options.VectorEncoding = new(StyleVectorTileEncoding)
	*options.VectorEncoding = encoding
	return options
}

// WithRasterEncoding returns a copy that sets raster DEM encoding.
func (options StyleTileSourceOptions) WithRasterEncoding(encoding StyleRasterDEMEncoding) StyleTileSourceOptions {
	options.RasterEncoding = new(StyleRasterDEMEncoding)
	*options.RasterEncoding = encoding
	return options
}

type cStyleTileSourceOptions struct {
	raw         C.mln_style_tile_source_options
	attribution cStringView
}

func cStyleTileSourceOptionsPointer(options *StyleTileSourceOptions) (cStyleTileSourceOptions, *C.mln_style_tile_source_options) {
	if options == nil {
		return cStyleTileSourceOptions{}, nil
	}
	raw := cStyleTileSourceOptions{raw: C.mln_style_tile_source_options_default()}
	if options.MinZoom != nil {
		raw.raw.fields |= C.MLN_STYLE_TILE_SOURCE_OPTION_MIN_ZOOM
		raw.raw.min_zoom = C.double(*options.MinZoom)
	}
	if options.MaxZoom != nil {
		raw.raw.fields |= C.MLN_STYLE_TILE_SOURCE_OPTION_MAX_ZOOM
		raw.raw.max_zoom = C.double(*options.MaxZoom)
	}
	if options.Attribution != nil {
		raw.raw.fields |= C.MLN_STYLE_TILE_SOURCE_OPTION_ATTRIBUTION
		raw.attribution = newCStringView(*options.Attribution)
		raw.raw.attribution = raw.attribution.raw()
	}
	if options.Scheme != nil {
		raw.raw.fields |= C.MLN_STYLE_TILE_SOURCE_OPTION_SCHEME
		raw.raw.scheme = C.uint32_t(*options.Scheme)
	}
	if options.Bounds != nil {
		raw.raw.fields |= C.MLN_STYLE_TILE_SOURCE_OPTION_BOUNDS
		raw.raw.bounds = cLatLngBounds(*options.Bounds)
	}
	if options.TileSize != nil {
		raw.raw.fields |= C.MLN_STYLE_TILE_SOURCE_OPTION_TILE_SIZE
		raw.raw.tile_size = C.uint32_t(*options.TileSize)
	}
	if options.VectorEncoding != nil {
		raw.raw.fields |= C.MLN_STYLE_TILE_SOURCE_OPTION_VECTOR_ENCODING
		raw.raw.vector_encoding = C.uint32_t(*options.VectorEncoding)
	}
	if options.RasterEncoding != nil {
		raw.raw.fields |= C.MLN_STYLE_TILE_SOURCE_OPTION_RASTER_ENCODING
		raw.raw.raster_encoding = C.uint32_t(*options.RasterEncoding)
	}
	return raw, &raw.raw
}

func (options cStyleTileSourceOptions) free() {
	options.attribution.free()
}

// StyleGeoJSONSourceOptions configures GeoJSON sources. These options are
// baked into prepared data by NewGeoJSONSourceData and fixed when the source
// is created, so SetGeoJSONSourceURL keeps the options the source was added
// with and SetGeoJSONSourceData requires data prepared with matching options.
type StyleGeoJSONSourceOptions struct {
	MinZoom        *float64
	MaxZoom        *float64
	Tolerance      *float64
	ClusterMaxZoom *float64
	// ClusterProperties holds cluster aggregation expressions as a JSON object
	// in the MapLibre Style Spec clusterProperties form.
	ClusterProperties []byte
	TileSize          *uint32
	Buffer            *uint32
	ClusterRadius     *uint32
	ClusterMinPoints  *uint32
	LineMetrics       *bool
	Cluster           *bool
	// SynchronousTiling slices requested tiles inline during the update pass,
	// so data set through SetGeoJSONSourceData reaches the next rendered frame
	// rather than a later one. SetGeoJSONSourceSynchronousTiling overrides this
	// at runtime.
	SynchronousTiling *bool
}

// Equal reports whether two descriptors hold the same field values.
func (options StyleGeoJSONSourceOptions) Equal(other StyleGeoJSONSourceOptions) bool {
	return equalPointer(options.MinZoom, other.MinZoom) &&
		equalPointer(options.MaxZoom, other.MaxZoom) &&
		equalPointer(options.Tolerance, other.Tolerance) &&
		equalPointer(options.ClusterMaxZoom, other.ClusterMaxZoom) &&
		equalOptionalBytes(options.ClusterProperties, other.ClusterProperties) &&
		equalPointer(options.TileSize, other.TileSize) &&
		equalPointer(options.Buffer, other.Buffer) &&
		equalPointer(options.ClusterRadius, other.ClusterRadius) &&
		equalPointer(options.ClusterMinPoints, other.ClusterMinPoints) &&
		equalPointer(options.LineMetrics, other.LineMetrics) &&
		equalPointer(options.Cluster, other.Cluster) &&
		equalPointer(options.SynchronousTiling, other.SynchronousTiling)
}

// Clone returns an independent deep copy of this descriptor.
func (options StyleGeoJSONSourceOptions) Clone() StyleGeoJSONSourceOptions {
	cloned := options
	cloned.MinZoom = clonePointer(options.MinZoom)
	cloned.MaxZoom = clonePointer(options.MaxZoom)
	cloned.Tolerance = clonePointer(options.Tolerance)
	cloned.ClusterMaxZoom = clonePointer(options.ClusterMaxZoom)
	cloned.ClusterProperties = cloneBytes(options.ClusterProperties)
	cloned.TileSize = clonePointer(options.TileSize)
	cloned.Buffer = clonePointer(options.Buffer)
	cloned.ClusterRadius = clonePointer(options.ClusterRadius)
	cloned.ClusterMinPoints = clonePointer(options.ClusterMinPoints)
	cloned.LineMetrics = clonePointer(options.LineMetrics)
	cloned.Cluster = clonePointer(options.Cluster)
	cloned.SynchronousTiling = clonePointer(options.SynchronousTiling)
	return cloned
}

// WithMinZoom returns a copy that sets the minimum tiling zoom.
func (options StyleGeoJSONSourceOptions) WithMinZoom(minZoom float64) StyleGeoJSONSourceOptions {
	options = options.Clone()
	options.MinZoom = new(float64)
	*options.MinZoom = minZoom
	return options
}

// WithMaxZoom returns a copy that sets the maximum tiling zoom.
func (options StyleGeoJSONSourceOptions) WithMaxZoom(maxZoom float64) StyleGeoJSONSourceOptions {
	options = options.Clone()
	options.MaxZoom = new(float64)
	*options.MaxZoom = maxZoom
	return options
}

// WithTolerance returns a copy that sets the Douglas-Peucker simplification tolerance.
func (options StyleGeoJSONSourceOptions) WithTolerance(tolerance float64) StyleGeoJSONSourceOptions {
	options = options.Clone()
	options.Tolerance = new(float64)
	*options.Tolerance = tolerance
	return options
}

// WithClusterMaxZoom returns a copy that sets the highest zoom that clusters points.
func (options StyleGeoJSONSourceOptions) WithClusterMaxZoom(clusterMaxZoom float64) StyleGeoJSONSourceOptions {
	options = options.Clone()
	options.ClusterMaxZoom = new(float64)
	*options.ClusterMaxZoom = clusterMaxZoom
	return options
}

// WithClusterProperties returns a copy that sets cluster aggregation expressions.
func (options StyleGeoJSONSourceOptions) WithClusterProperties(clusterProperties []byte) StyleGeoJSONSourceOptions {
	options = options.Clone()
	options.ClusterProperties = cloneBytes(clusterProperties)
	return options
}

// WithTileSize returns a copy that sets the tile extent in pixels.
func (options StyleGeoJSONSourceOptions) WithTileSize(tileSize uint32) StyleGeoJSONSourceOptions {
	options = options.Clone()
	options.TileSize = new(uint32)
	*options.TileSize = tileSize
	return options
}

// WithBuffer returns a copy that sets the tile buffer in pixels.
func (options StyleGeoJSONSourceOptions) WithBuffer(buffer uint32) StyleGeoJSONSourceOptions {
	options = options.Clone()
	options.Buffer = new(uint32)
	*options.Buffer = buffer
	return options
}

// WithClusterRadius returns a copy that sets the cluster radius in pixels.
func (options StyleGeoJSONSourceOptions) WithClusterRadius(clusterRadius uint32) StyleGeoJSONSourceOptions {
	options = options.Clone()
	options.ClusterRadius = new(uint32)
	*options.ClusterRadius = clusterRadius
	return options
}

// WithClusterMinPoints returns a copy that sets the points required to form a cluster.
func (options StyleGeoJSONSourceOptions) WithClusterMinPoints(clusterMinPoints uint32) StyleGeoJSONSourceOptions {
	options = options.Clone()
	options.ClusterMinPoints = new(uint32)
	*options.ClusterMinPoints = clusterMinPoints
	return options
}

// WithLineMetrics returns a copy that sets whether line distance metrics are added.
func (options StyleGeoJSONSourceOptions) WithLineMetrics(lineMetrics bool) StyleGeoJSONSourceOptions {
	options = options.Clone()
	options.LineMetrics = new(bool)
	*options.LineMetrics = lineMetrics
	return options
}

// WithCluster returns a copy that sets whether point features cluster.
func (options StyleGeoJSONSourceOptions) WithCluster(cluster bool) StyleGeoJSONSourceOptions {
	options = options.Clone()
	options.Cluster = new(bool)
	*options.Cluster = cluster
	return options
}

// WithSynchronousTiling returns a copy that sets whether requested tiles are
// sliced inline during the update pass.
func (options StyleGeoJSONSourceOptions) WithSynchronousTiling(synchronousTiling bool) StyleGeoJSONSourceOptions {
	options = options.Clone()
	options.SynchronousTiling = new(bool)
	*options.SynchronousTiling = synchronousTiling
	return options
}

// cStyleGeoJSONSourceOptions keeps the native options and cluster-property
// bytes alive for the native call.
type cStyleGeoJSONSourceOptions struct {
	raw               *C.mln_geojson_source_options
	clusterProperties cBufferView
}

func newCStyleGeoJSONSourceOptions(options *StyleGeoJSONSourceOptions) (*cStyleGeoJSONSourceOptions, error) {
	if options == nil {
		return nil, nil
	}
	raw := &cStyleGeoJSONSourceOptions{
		raw: (*C.mln_geojson_source_options)(C.malloc(C.size_t(unsafe.Sizeof(C.mln_geojson_source_options{})))),
	}
	*raw.raw = C.mln_geojson_source_options_default()
	if options.MinZoom != nil {
		raw.raw.fields |= C.MLN_GEOJSON_SOURCE_OPTION_MIN_ZOOM
		raw.raw.min_zoom = C.double(*options.MinZoom)
	}
	if options.MaxZoom != nil {
		raw.raw.fields |= C.MLN_GEOJSON_SOURCE_OPTION_MAX_ZOOM
		raw.raw.max_zoom = C.double(*options.MaxZoom)
	}
	if options.Tolerance != nil {
		raw.raw.fields |= C.MLN_GEOJSON_SOURCE_OPTION_TOLERANCE
		raw.raw.tolerance = C.double(*options.Tolerance)
	}
	if options.ClusterMaxZoom != nil {
		raw.raw.fields |= C.MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM
		raw.raw.cluster_max_zoom = C.double(*options.ClusterMaxZoom)
	}
	if options.ClusterProperties != nil {
		raw.clusterProperties = newCBufferView(options.ClusterProperties)
		raw.raw.fields |= C.MLN_GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES
		raw.raw.cluster_properties = raw.clusterProperties.raw()
	}
	if options.TileSize != nil {
		raw.raw.fields |= C.MLN_GEOJSON_SOURCE_OPTION_TILE_SIZE
		raw.raw.tile_size = C.uint32_t(*options.TileSize)
	}
	if options.Buffer != nil {
		raw.raw.fields |= C.MLN_GEOJSON_SOURCE_OPTION_BUFFER
		raw.raw.buffer = C.uint32_t(*options.Buffer)
	}
	if options.ClusterRadius != nil {
		raw.raw.fields |= C.MLN_GEOJSON_SOURCE_OPTION_CLUSTER_RADIUS
		raw.raw.cluster_radius = C.uint32_t(*options.ClusterRadius)
	}
	if options.ClusterMinPoints != nil {
		raw.raw.fields |= C.MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MIN_POINTS
		raw.raw.cluster_min_points = C.uint32_t(*options.ClusterMinPoints)
	}
	if options.LineMetrics != nil {
		raw.raw.fields |= C.MLN_GEOJSON_SOURCE_OPTION_LINE_METRICS
		raw.raw.line_metrics = C.bool(*options.LineMetrics)
	}
	if options.Cluster != nil {
		raw.raw.fields |= C.MLN_GEOJSON_SOURCE_OPTION_CLUSTER
		raw.raw.cluster = C.bool(*options.Cluster)
	}
	if options.SynchronousTiling != nil {
		raw.raw.fields |= C.MLN_GEOJSON_SOURCE_OPTION_SYNCHRONOUS_TILING
		raw.raw.synchronous_tiling = C.bool(*options.SynchronousTiling)
	}
	return raw, nil
}

func (options *cStyleGeoJSONSourceOptions) ptr() *C.mln_geojson_source_options {
	if options == nil {
		return nil
	}
	return options.raw
}

func (options *cStyleGeoJSONSourceOptions) free() {
	if options == nil {
		return
	}
	options.clusterProperties.free()
	if options.raw != nil {
		C.free(unsafe.Pointer(options.raw))
	}
}

// CustomGeometryTileCallback receives custom geometry tile requests. Native code
// may invoke it concurrently on worker threads, so it must be thread-safe and
// must not call MapLibre map APIs directly. Queue tile-data and invalidation
// commands for later submission. Panics are recovered and ignored.
type CustomGeometryTileCallback func(CanonicalTileID)

// CustomGeometrySourceOptions configures a custom geometry source. CancelTile is
// best-effort and may be repeated or race with FetchTile.
type CustomGeometrySourceOptions struct {
	FetchTile  CustomGeometryTileCallback
	CancelTile CustomGeometryTileCallback
	MinZoom    *float64
	MaxZoom    *float64
	Tolerance  *float64
	TileSize   *uint32
	Buffer     *uint32
	Clip       *bool
	Wrap       *bool
}

// CustomMVTVectorTileCallback receives custom MVT vector tile requests. Native
// code may invoke it concurrently on worker threads, so it must be thread-safe
// and must not call MapLibre map APIs directly. Queue tile-data, tile-error,
// and invalidation commands for later submission. Panics are recovered and
// ignored.
type CustomMVTVectorTileCallback func(CanonicalTileID)

// CustomMVTVectorSourceOptions configures a custom MVT vector source. CancelTile
// is best-effort and may be repeated or race with FetchTile.
type CustomMVTVectorSourceOptions struct {
	FetchTile  CustomMVTVectorTileCallback
	CancelTile CustomMVTVectorTileCallback
	MinZoom    *float64
	MaxZoom    *float64
}

func (options CustomMVTVectorSourceOptions) toCallback() callback.CustomMVTVectorSourceOptions {
	raw := callback.CustomMVTVectorSourceOptions{
		FetchTile: func(tileID callback.CanonicalTileID) {
			if options.FetchTile != nil {
				options.FetchTile(CanonicalTileID{Z: tileID.Z, X: tileID.X, Y: tileID.Y})
			}
		},
	}
	if options.CancelTile != nil {
		raw.CancelTile = func(tileID callback.CanonicalTileID) {
			options.CancelTile(CanonicalTileID{Z: tileID.Z, X: tileID.X, Y: tileID.Y})
		}
	}
	if options.MinZoom != nil {
		raw.Fields |= C.MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MIN_ZOOM
		raw.MinZoom = *options.MinZoom
	}
	if options.MaxZoom != nil {
		raw.Fields |= C.MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MAX_ZOOM
		raw.MaxZoom = *options.MaxZoom
	}
	return raw
}

func (options CustomGeometrySourceOptions) toCallback() callback.CustomGeometrySourceOptions {
	raw := callback.CustomGeometrySourceOptions{
		FetchTile: func(tileID callback.CanonicalTileID) {
			if options.FetchTile != nil {
				options.FetchTile(CanonicalTileID{Z: tileID.Z, X: tileID.X, Y: tileID.Y})
			}
		},
	}
	if options.CancelTile != nil {
		raw.CancelTile = func(tileID callback.CanonicalTileID) {
			options.CancelTile(CanonicalTileID{Z: tileID.Z, X: tileID.X, Y: tileID.Y})
		}
	}
	if options.MinZoom != nil {
		raw.Fields |= C.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM
		raw.MinZoom = *options.MinZoom
	}
	if options.MaxZoom != nil {
		raw.Fields |= C.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM
		raw.MaxZoom = *options.MaxZoom
	}
	if options.Tolerance != nil {
		raw.Fields |= C.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TOLERANCE
		raw.Tolerance = *options.Tolerance
	}
	if options.TileSize != nil {
		raw.Fields |= C.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TILE_SIZE
		raw.TileSize = *options.TileSize
	}
	if options.Buffer != nil {
		raw.Fields |= C.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_BUFFER
		raw.Buffer = *options.Buffer
	}
	if options.Clip != nil {
		raw.Fields |= C.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_CLIP
		raw.Clip = *options.Clip
	}
	if options.Wrap != nil {
		raw.Fields |= C.MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_WRAP
		raw.Wrap = *options.Wrap
	}
	return raw
}

// PremultipliedRGBA8Image contains caller-owned premultiplied RGBA8 pixels.
type PremultipliedRGBA8Image struct {
	Width      uint32
	Height     uint32
	Stride     uint32
	Pixels     []byte
	ByteLength uint64
}

// ImageStretch is one stretchable interval along an image axis, in image pixels.
type ImageStretch struct {
	From float32
	To   float32
}

// ImageContent holds content-box insets in image pixels, from the image's top-left.
type ImageContent struct {
	Left   float32
	Top    float32
	Right  float32
	Bottom float32
}

// StyleImageTextFit reports how a stretchable image fits text along one axis.
type StyleImageTextFit uint32

// Style image text-fit values.
const (
	StyleImageTextFitStretchOrShrink StyleImageTextFit = StyleImageTextFit(C.MLN_STYLE_IMAGE_TEXT_FIT_STRETCH_OR_SHRINK)
	StyleImageTextFitStretchOnly     StyleImageTextFit = StyleImageTextFit(C.MLN_STYLE_IMAGE_TEXT_FIT_STRETCH_ONLY)
	StyleImageTextFitProportional    StyleImageTextFit = StyleImageTextFit(C.MLN_STYLE_IMAGE_TEXT_FIT_PROPORTIONAL)
)

// StyleImageOptions configures a runtime style image.
type StyleImageOptions struct {
	PixelRatio *float32
	SDF        *bool
	// StretchX and StretchY are the stretchable intervals along each axis. A
	// present empty slice stays distinguishable from an absent one.
	StretchX []ImageStretch
	StretchY []ImageStretch
	// Content is the content box used when icon-text-fit applies.
	Content       *ImageContent
	TextFitWidth  *StyleImageTextFit
	TextFitHeight *StyleImageTextFit
}

// Equal reports whether two descriptors hold the same field values.
func (options StyleImageOptions) Equal(other StyleImageOptions) bool {
	return equalPointer(options.PixelRatio, other.PixelRatio) &&
		equalPointer(options.SDF, other.SDF) &&
		equalStretches(options.StretchX, other.StretchX) &&
		equalStretches(options.StretchY, other.StretchY) &&
		equalPointer(options.Content, other.Content) &&
		equalPointer(options.TextFitWidth, other.TextFitWidth) &&
		equalPointer(options.TextFitHeight, other.TextFitHeight)
}

// equalStretches compares stretch slices by content, keeping a present empty
// slice distinct from an absent one.
func equalStretches(left, right []ImageStretch) bool {
	if (left == nil) != (right == nil) || len(left) != len(right) {
		return false
	}
	for index := range left {
		if left[index] != right[index] {
			return false
		}
	}
	return true
}

// Clone returns an independent deep copy of this descriptor.
func (options StyleImageOptions) Clone() StyleImageOptions {
	cloned := options
	cloned.PixelRatio = clonePointer(options.PixelRatio)
	cloned.SDF = clonePointer(options.SDF)
	cloned.StretchX = cloneStretches(options.StretchX)
	cloned.StretchY = cloneStretches(options.StretchY)
	cloned.Content = clonePointer(options.Content)
	cloned.TextFitWidth = clonePointer(options.TextFitWidth)
	cloned.TextFitHeight = clonePointer(options.TextFitHeight)
	return cloned
}

// cloneStretches copies a stretch slice, keeping a present empty slice distinct
// from an absent one.
func cloneStretches(stretches []ImageStretch) []ImageStretch {
	if stretches == nil {
		return nil
	}
	cloned := make([]ImageStretch, len(stretches))
	copy(cloned, stretches)
	return cloned
}

// cStyleImageOptionsScope keeps the stretch arrays in C storage, which the
// native options struct borrows for the duration of the call.
type cStyleImageOptionsScope struct {
	raw      C.mln_style_image_options
	stretchX unsafe.Pointer
	stretchY unsafe.Pointer
}

func newCStyleImageOptions(options StyleImageOptions) cStyleImageOptionsScope {
	scope := cStyleImageOptionsScope{raw: C.mln_style_image_options_default()}
	if options.PixelRatio != nil {
		scope.raw.fields |= C.MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO
		scope.raw.pixel_ratio = C.float(*options.PixelRatio)
	}
	if options.SDF != nil {
		scope.raw.fields |= C.MLN_STYLE_IMAGE_OPTION_SDF
		scope.raw.sdf = C.bool(*options.SDF)
	}
	if options.StretchX != nil {
		scope.raw.fields |= C.MLN_STYLE_IMAGE_OPTION_STRETCH_X
		scope.stretchX = allocCStretches(options.StretchX)
		scope.raw.stretch_x = (*C.mln_image_stretch)(scope.stretchX)
		scope.raw.stretch_x_count = C.size_t(len(options.StretchX))
	}
	if options.StretchY != nil {
		scope.raw.fields |= C.MLN_STYLE_IMAGE_OPTION_STRETCH_Y
		scope.stretchY = allocCStretches(options.StretchY)
		scope.raw.stretch_y = (*C.mln_image_stretch)(scope.stretchY)
		scope.raw.stretch_y_count = C.size_t(len(options.StretchY))
	}
	if options.Content != nil {
		scope.raw.fields |= C.MLN_STYLE_IMAGE_OPTION_CONTENT
		scope.raw.content.left = C.float(options.Content.Left)
		scope.raw.content.top = C.float(options.Content.Top)
		scope.raw.content.right = C.float(options.Content.Right)
		scope.raw.content.bottom = C.float(options.Content.Bottom)
	}
	if options.TextFitWidth != nil {
		scope.raw.fields |= C.MLN_STYLE_IMAGE_OPTION_TEXT_FIT_WIDTH
		scope.raw.text_fit_width = C.uint32_t(*options.TextFitWidth)
	}
	if options.TextFitHeight != nil {
		scope.raw.fields |= C.MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT
		scope.raw.text_fit_height = C.uint32_t(*options.TextFitHeight)
	}
	return scope
}

func (scope *cStyleImageOptionsScope) free() {
	if scope.stretchX != nil {
		C.free(scope.stretchX)
		scope.stretchX = nil
	}
	if scope.stretchY != nil {
		C.free(scope.stretchY)
		scope.stretchY = nil
	}
}

func allocCStretches(stretches []ImageStretch) unsafe.Pointer {
	if len(stretches) == 0 {
		// A zero count needs no pointer, so an empty slice stays null.
		return nil
	}
	size := C.size_t(len(stretches)) * C.size_t(unsafe.Sizeof(C.mln_image_stretch{}))
	allocation := C.malloc(size)
	raw := unsafe.Slice((*C.mln_image_stretch)(allocation), len(stretches))
	for index, stretch := range stretches {
		raw[index].from = C.float(stretch.From)
		raw[index].to = C.float(stretch.To)
	}
	return allocation
}

// StyleTransitionOptions configures how the style animates paint property
// changes and whether symbol placement changes cross-fade. These are distinct
// from camera animation options and from the per-property transitions a style
// declares.
type StyleTransitionOptions struct {
	// DurationMS is the transition duration in milliseconds. An absent value
	// falls back to the duration the style declares per property.
	DurationMS *float64
	// DelayMS is the transition delay in milliseconds. An absent value falls
	// back to the delay the style declares per property.
	DelayMS *float64
	// EnablePlacementTransitions reports whether symbol placement changes
	// cross-fade, which an absent value leaves on. Clearing it makes symbol
	// placement changes apply to the next rendered frame. Reading the options
	// always reports a value.
	EnablePlacementTransitions *bool
}

// Equal reports whether two descriptors hold the same field values.
func (options StyleTransitionOptions) Equal(other StyleTransitionOptions) bool {
	return equalPointer(options.DurationMS, other.DurationMS) &&
		equalPointer(options.DelayMS, other.DelayMS) &&
		equalPointer(options.EnablePlacementTransitions, other.EnablePlacementTransitions)
}

// Clone returns an independent deep copy of this descriptor.
func (options StyleTransitionOptions) Clone() StyleTransitionOptions {
	cloned := options
	cloned.DurationMS = clonePointer(options.DurationMS)
	cloned.DelayMS = clonePointer(options.DelayMS)
	cloned.EnablePlacementTransitions = clonePointer(options.EnablePlacementTransitions)
	return cloned
}

func newCStyleTransitionOptions(options StyleTransitionOptions) C.mln_style_transition_options {
	raw := C.mln_style_transition_options_default()
	if options.EnablePlacementTransitions != nil {
		raw.fields |= C.MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS
		raw.enable_placement_transitions = C.bool(*options.EnablePlacementTransitions)
	}
	if options.DurationMS != nil {
		raw.fields |= C.MLN_STYLE_TRANSITION_OPTION_DURATION
		raw.duration_ms = C.double(*options.DurationMS)
	}
	if options.DelayMS != nil {
		raw.fields |= C.MLN_STYLE_TRANSITION_OPTION_DELAY
		raw.delay_ms = C.double(*options.DelayMS)
	}
	return raw
}

func styleTransitionOptionsFromC(raw C.mln_style_transition_options) StyleTransitionOptions {
	options := StyleTransitionOptions{}
	if raw.fields&C.MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS != 0 {
		enable := bool(raw.enable_placement_transitions)
		options.EnablePlacementTransitions = &enable
	}
	if raw.fields&C.MLN_STYLE_TRANSITION_OPTION_DURATION != 0 {
		duration := float64(raw.duration_ms)
		options.DurationMS = &duration
	}
	if raw.fields&C.MLN_STYLE_TRANSITION_OPTION_DELAY != 0 {
		delay := float64(raw.delay_ms)
		options.DelayMS = &delay
	}
	return options
}

// StyleImageInfo contains copied runtime style image metadata.
type StyleImageInfo struct {
	Width      uint32
	Height     uint32
	Stride     uint32
	ByteLength uint64
	PixelRatio float32
	SDF        bool
	// StretchXCount and StretchYCount report the interval counts. Read the
	// intervals themselves with StyleImageStretches.
	StretchXCount uint64
	StretchYCount uint64
	// Content is the content box, absent when the image carries none.
	Content       *ImageContent
	TextFitWidth  *StyleImageTextFit
	TextFitHeight *StyleImageTextFit
}

// StyleImage is a complete copied runtime style image.
type StyleImage struct {
	Info     StyleImageInfo
	Pixels   []byte
	StretchX []ImageStretch
	StretchY []ImageStretch
}

type cPremultipliedRGBA8Image struct {
	raw    C.mln_premultiplied_rgba8_image
	pinner runtime.Pinner
}

func newCPremultipliedRGBA8Image(image PremultipliedRGBA8Image) *cPremultipliedRGBA8Image {
	result := &cPremultipliedRGBA8Image{raw: C.mln_premultiplied_rgba8_image_default()}
	result.raw.width = C.uint32_t(image.Width)
	result.raw.height = C.uint32_t(image.Height)
	result.raw.stride = C.uint32_t(image.Stride)
	if len(image.Pixels) > 0 {
		// The descriptor contains a Go pointer, so pin its backing array until C returns.
		result.pinner.Pin(&image.Pixels[0])
		result.raw.pixels = (*C.uint8_t)(unsafe.Pointer(&image.Pixels[0]))
	}
	result.raw.byte_length = C.size_t(len(image.Pixels))
	return result
}

func (image *cPremultipliedRGBA8Image) free() {
	image.pinner.Unpin()
}

func styleImageInfoFromC(info C.mln_style_image_info) StyleImageInfo {
	result := StyleImageInfo{
		Width:         uint32(info.width),
		Height:        uint32(info.height),
		Stride:        uint32(info.stride),
		ByteLength:    uint64(info.byte_length),
		PixelRatio:    float32(info.pixel_ratio),
		SDF:           bool(info.sdf),
		StretchXCount: uint64(info.stretch_x_count),
		StretchYCount: uint64(info.stretch_y_count),
	}
	if bool(info.has_content) {
		result.Content = &ImageContent{
			Left:   float32(info.content.left),
			Top:    float32(info.content.top),
			Right:  float32(info.content.right),
			Bottom: float32(info.content.bottom),
		}
	}
	if bool(info.has_text_fit_width) {
		fit := StyleImageTextFit(info.text_fit_width)
		result.TextFitWidth = &fit
	}
	if bool(info.has_text_fit_height) {
		fit := StyleImageTextFit(info.text_fit_height)
		result.TextFitHeight = &fit
	}
	return result
}

// StyleImageStretches returns one runtime style image's stretchable intervals
// and whether the image exists.

func stretchesFromC(raw []C.mln_image_stretch) []ImageStretch {
	stretches := make([]ImageStretch, len(raw))
	for index, stretch := range raw {
		stretches[index] = ImageStretch{From: float32(stretch.from), To: float32(stretch.to)}
	}
	return stretches
}

// LocationIndicatorImageKind identifies an image-name slot on a location indicator layer.
type LocationIndicatorImageKind uint32

const (
	LocationIndicatorImageKindTop     LocationIndicatorImageKind = LocationIndicatorImageKind(C.MLN_LOCATION_INDICATOR_IMAGE_KIND_TOP)
	LocationIndicatorImageKindBearing LocationIndicatorImageKind = LocationIndicatorImageKind(C.MLN_LOCATION_INDICATOR_IMAGE_KIND_BEARING)
	LocationIndicatorImageKindShadow  LocationIndicatorImageKind = LocationIndicatorImageKind(C.MLN_LOCATION_INDICATOR_IMAGE_KIND_SHADOW)
)

func styleSourceInfoFromC(info C.mln_style_source_info) StyleSourceInfo {
	result := StyleSourceInfo{
		Type:            StyleSourceType(info._type),
		IDSize:          uint64(info.id_size),
		IsVolatile:      bool(info.is_volatile),
		HasAttribution:  bool(info.has_attribution),
		AttributionSize: uint64(info.attribution_size),
	}
	if info.fields&C.MLN_STYLE_SOURCE_INFO_TILEJSON != 0 {
		result.TileJSON = &StyleSourceTileJSON{
			MinZoom: float64(info.min_zoom),
			MaxZoom: float64(info.max_zoom),
			Scheme:  StyleTileScheme(info.scheme),
		}
		if info.fields&C.MLN_STYLE_SOURCE_INFO_BOUNDS != 0 {
			bounds := goLatLngBounds(info.bounds)
			result.TileJSON.Bounds = &bounds
		}
	}
	if info.fields&C.MLN_STYLE_SOURCE_INFO_TILE_SIZE != 0 {
		value := uint32(info.tile_size)
		result.TileSize = &value
	}
	if info.fields&C.MLN_STYLE_SOURCE_INFO_VECTOR_ENCODING != 0 {
		value := StyleVectorTileEncoding(info.vector_encoding)
		result.VectorEncoding = &value
	}
	if info.fields&C.MLN_STYLE_SOURCE_INFO_RASTER_ENCODING != 0 {
		value := StyleRasterDEMEncoding(info.raster_encoding)
		result.RasterEncoding = &value
	}
	return result
}

func styleLayerInfoFromC(info C.mln_style_layer_info) StyleLayerInfo {
	return StyleLayerInfo{
		Type:            goStringView(info._type),
		MinZoom:         float64(info.min_zoom),
		MaxZoom:         float64(info.max_zoom),
		Visibility:      StyleLayerVisibility(info.visibility),
		HasSourceID:     info.fields&C.MLN_STYLE_LAYER_INFO_SOURCE_ID != 0,
		SourceIDSize:    uint64(info.source_id_size),
		HasSourceLayer:  info.fields&C.MLN_STYLE_LAYER_INFO_SOURCE_LAYER != 0,
		SourceLayerSize: uint64(info.source_layer_size),
	}
}

// AddGeoJSONSourceURL adds a GeoJSON source that loads from a URL. Later
// SetGeoJSONSourceURL calls keep the options passed here, and later
// SetGeoJSONSourceData calls require data prepared with matching options.
func (m *MapHandle) AddGeoJSONSourceURL(sourceID string, url string, options *StyleGeoJSONSourceOptions) (*Future[CommandCompletion], error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}

	defer m.state.KeepAlive()
	sourceView := newCStringView(sourceID)
	defer sourceView.free()
	urlView := newCStringView(url)
	defer urlView.free()
	rawOptions, err := newCStyleGeoJSONSourceOptions(options)
	if err != nil {
		return nil, newBindingError(ErrInvalidArgument, err.Error())
	}
	defer rawOptions.free()
	return startCompletion(func(completion *C.mln_completion) int32 {
		return int32(C.mln_map_add_geojson_source_url(C.mln_map(ptr), sourceView.raw(), urlView.raw(), rawOptions.ptr(), completion))
	}, completionCommand)
}

// SetGeoJSONSourceURL updates a GeoJSON source to load from a URL.
func (m *MapHandle) SetGeoJSONSourceURL(sourceID string, url string) (*Future[CommandCompletion], error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}

	defer m.state.KeepAlive()
	sourceView := newCStringView(sourceID)
	defer sourceView.free()
	urlView := newCStringView(url)
	defer urlView.free()
	return startCompletion(func(completion *C.mln_completion) int32 {
		return int32(C.mln_map_set_geojson_source_url(C.mln_map(ptr), sourceView.raw(), urlView.raw(), completion))
	}, completionCommand)
}

// AddGeoJSONSourceData adds a GeoJSON source with prepared inline data. The
// call borrows the handle and retains the prepared index, and the source
// adopts the options the data was prepared with, fixed for the lifetime of the
// source.
func (m *MapHandle) AddGeoJSONSourceData(sourceID string, data *GeoJSONSourceDataHandle) (*Future[CommandCompletion], error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}

	defer m.state.KeepAlive()
	dataPtr, err := data.ptr()
	if err != nil {
		return nil, err
	}

	defer data.state.KeepAlive()
	sourceView := newCStringView(sourceID)
	defer sourceView.free()
	return startCompletion(func(completion *C.mln_completion) int32 {
		return int32(C.mln_map_add_geojson_source_data(C.mln_map(ptr), sourceView.raw(), C.mln_geojson_source_data(dataPtr), completion))
	}, completionCommand)
}

// SetGeoJSONSourceData updates a GeoJSON source with prepared inline data. The
// call borrows the handle and retains the prepared index. The data must have
// been prepared with options equal to the options the source was added with,
// ClusterProperties excepted.
func (m *MapHandle) SetGeoJSONSourceData(sourceID string, data *GeoJSONSourceDataHandle) (*Future[CommandCompletion], error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}

	defer m.state.KeepAlive()
	dataPtr, err := data.ptr()
	if err != nil {
		return nil, err
	}

	defer data.state.KeepAlive()
	sourceView := newCStringView(sourceID)
	defer sourceView.free()
	return startCompletion(func(completion *C.mln_completion) int32 {
		return int32(C.mln_map_set_geojson_source_data(C.mln_map(ptr), sourceView.raw(), C.mln_geojson_source_data(dataPtr), completion))
	}, completionCommand)
}

// SetGeoJSONSourceSynchronousTiling overrides a GeoJSON source's synchronous
// tiling at runtime. While enabled is true, the source slices requested tiles
// inline during the update pass, as if its options had set SynchronousTiling;
// false restores the option the source was added with.
func (m *MapHandle) SetGeoJSONSourceSynchronousTiling(sourceID string, enabled bool) (*Future[CommandCompletion], error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}

	defer m.state.KeepAlive()
	sourceView := newCStringView(sourceID)
	defer sourceView.free()
	return startCompletion(func(completion *C.mln_completion) int32 {
		return int32(C.mln_map_set_geojson_source_synchronous_tiling(C.mln_map(ptr), sourceView.raw(), C.bool(enabled), completion))
	}, completionCommand)
}

// SetStyleSourceVolatile submits a command that sets whether one style source
// stores fetched tiles in persistent cache. The change is visible through
// StyleSourceInfo's IsVolatile field. The command fails with ErrNotFound
// reported through its terminal event's Err payload field when no source with
// the ID exists.
func (m *MapHandle) SetStyleSourceVolatile(sourceID string, isVolatile bool) (*Future[CommandCompletion], error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}

	defer m.state.KeepAlive()
	sourceView := newCStringView(sourceID)
	defer sourceView.free()
	return startCompletion(func(completion *C.mln_completion) int32 {
		return int32(C.mln_map_set_style_source_volatile(C.mln_map(ptr), sourceView.raw(), C.bool(isVolatile), completion))
	}, completionCommand)
}

// SetCustomGeometrySourceTileData sets custom geometry data for one tile.
func (m *MapHandle) SetCustomGeometrySourceTileData(sourceID string, tileID CanonicalTileID, data []byte) (*Future[CommandCompletion], error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}

	defer m.state.KeepAlive()
	sourceView := newCStringView(sourceID)
	defer sourceView.free()
	rawData := newCBufferView(data)
	defer rawData.free()
	return startCompletion(func(completion *C.mln_completion) int32 {
		return int32(C.mln_map_set_custom_geometry_source_tile_data(
			C.mln_map(ptr),
			sourceView.raw(),
			cCanonicalTileID(tileID),
			rawData.raw(),
			completion,
		))
	}, completionCommand)
}

// InvalidateCustomGeometrySourceTile invalidates custom geometry data for one tile.
func (m *MapHandle) InvalidateCustomGeometrySourceTile(sourceID string, tileID CanonicalTileID) (*Future[CommandCompletion], error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}

	defer m.state.KeepAlive()
	sourceView := newCStringView(sourceID)
	defer sourceView.free()
	return startCompletion(func(completion *C.mln_completion) int32 {
		return int32(C.mln_map_invalidate_custom_geometry_source_tile(C.mln_map(ptr), sourceView.raw(), cCanonicalTileID(tileID), completion))
	}, completionCommand)
}

// InvalidateCustomGeometrySourceRegion invalidates custom geometry data inside one geographic region.
func (m *MapHandle) InvalidateCustomGeometrySourceRegion(sourceID string, bounds LatLngBounds) (*Future[CommandCompletion], error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}

	defer m.state.KeepAlive()
	sourceView := newCStringView(sourceID)
	defer sourceView.free()
	return startCompletion(func(completion *C.mln_completion) int32 {
		return int32(C.mln_map_invalidate_custom_geometry_source_region(C.mln_map(ptr), sourceView.raw(), cLatLngBounds(bounds), completion))
	}, completionCommand)
}

// AddCustomMVTVectorSource adds a custom MVT vector source to the current style.
// Callback state remains valid until source removal, style replacement, or map
// close, each of which frees it through the C API's release callback. A failed
// add frees it before returning.
func (m *MapHandle) AddCustomMVTVectorSource(sourceID string, options CustomMVTVectorSourceOptions) (*Future[CommandCompletion], error) {
	if sourceID == "" {
		return nil, newBindingError(ErrInvalidArgument, "source ID is empty")
	}
	if options.FetchTile == nil {
		return nil, newBindingError(ErrInvalidArgument, "CustomMVTVectorSourceOptions.FetchTile is nil")
	}
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}
	defer m.state.KeepAlive()
	return startCompletion(func(completion *C.mln_completion) int32 {
		return callback.AddCustomMVTVectorSource(
			uint64(ptr), sourceID, options.toCallback(), unsafe.Pointer(completion),
		)
	}, completionCommand)
}

// SetCustomMVTVectorSourceTileData sets custom MVT vector data for one tile.
func (m *MapHandle) SetCustomMVTVectorSourceTileData(sourceID string, tileID CanonicalTileID, data []byte) (*Future[CommandCompletion], error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}

	defer m.state.KeepAlive()
	sourceView := newCStringView(sourceID)
	defer sourceView.free()
	rawData := newCBufferView(data)
	defer rawData.free()
	return startCompletion(func(completion *C.mln_completion) int32 {
		return int32(C.mln_map_set_custom_mvt_vector_source_tile_data(
			C.mln_map(ptr),
			sourceView.raw(),
			cCanonicalTileID(tileID),
			rawData.raw(),
			completion,
		))
	}, completionCommand)
}

// SetCustomMVTVectorSourceTileError reports a custom MVT vector source error for one tile.
func (m *MapHandle) SetCustomMVTVectorSourceTileError(sourceID string, tileID CanonicalTileID, message string) (*Future[CommandCompletion], error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}

	defer m.state.KeepAlive()
	sourceView := newCStringView(sourceID)
	defer sourceView.free()
	messageView := newCStringView(message)
	defer messageView.free()
	return startCompletion(func(completion *C.mln_completion) int32 {
		return int32(C.mln_map_set_custom_mvt_vector_source_tile_error(
			C.mln_map(ptr),
			sourceView.raw(),
			cCanonicalTileID(tileID),
			messageView.raw(),
			completion,
		))
	}, completionCommand)
}

// InvalidateCustomMVTVectorSourceTile invalidates custom MVT vector data for one tile.
func (m *MapHandle) InvalidateCustomMVTVectorSourceTile(sourceID string, tileID CanonicalTileID) (*Future[CommandCompletion], error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}

	defer m.state.KeepAlive()
	sourceView := newCStringView(sourceID)
	defer sourceView.free()
	return startCompletion(func(completion *C.mln_completion) int32 {
		return int32(C.mln_map_invalidate_custom_mvt_vector_source_tile(C.mln_map(ptr), sourceView.raw(), cCanonicalTileID(tileID), completion))
	}, completionCommand)
}

// SetStyleImage sets or replaces one runtime style image.
func (m *MapHandle) SetStyleImage(imageID string, image PremultipliedRGBA8Image, options StyleImageOptions) (*Future[CommandCompletion], error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}

	defer m.state.KeepAlive()
	imageView := newCStringView(imageID)
	defer imageView.free()
	rawImage := newCPremultipliedRGBA8Image(image)
	defer rawImage.free()
	rawOptions := newCStyleImageOptions(options)
	defer rawOptions.free()
	return startCompletion(func(completion *C.mln_completion) int32 {
		return int32(C.mln_map_set_style_image(C.mln_map(ptr), imageView.raw(), &rawImage.raw, &rawOptions.raw, completion))
	}, completionCommand)
}

// RemoveStyleImage submits a command that removes one runtime style image. The
// command commits when an image with the ID existed and was removed, and fails
// with ErrNotFound reported through its terminal event's Err payload field when
// none does. Check existence with StyleImageInfo's found flag.
func (m *MapHandle) RemoveStyleImage(imageID string) (*Future[CommandCompletion], error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}

	defer m.state.KeepAlive()
	imageView := newCStringView(imageID)
	defer imageView.free()
	return startCompletion(func(completion *C.mln_completion) int32 {
		return int32(C.mln_map_remove_style_image(C.mln_map(ptr), imageView.raw(), completion))
	}, completionCommand)
}

// StyleImageInfo returns copied metadata for one runtime style image.

// StyleImagePremultipliedRGBA8 returns copied tightly packed premultiplied RGBA8 pixels.

// StyleImagePremultipliedRGBA8Into copies tightly packed premultiplied RGBA8 pixels into buffer.

// AddImageSourceURL adds an image source that loads its image from a URL.
func (m *MapHandle) AddImageSourceURL(sourceID string, coordinates []LatLng, url string) (*Future[CommandCompletion], error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}

	defer m.state.KeepAlive()
	sourceView := newCStringView(sourceID)
	defer sourceView.free()
	urlView := newCStringView(url)
	defer urlView.free()
	rawCoordinates := cLatLngSlice(coordinates)
	var rawCoordinatesPtr *C.mln_lat_lng
	if len(rawCoordinates) > 0 {
		rawCoordinatesPtr = &rawCoordinates[0]
	}
	return startCompletion(func(completion *C.mln_completion) int32 {
		return int32(C.mln_map_add_image_source_url(
			C.mln_map(ptr),
			sourceView.raw(),
			rawCoordinatesPtr,
			C.size_t(len(rawCoordinates)),
			urlView.raw(),
			completion,
		))
	}, completionCommand)
}

// AddImageSourceImage adds an image source with inline image pixels.
func (m *MapHandle) AddImageSourceImage(sourceID string, coordinates []LatLng, image PremultipliedRGBA8Image) (*Future[CommandCompletion], error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}

	defer m.state.KeepAlive()
	sourceView := newCStringView(sourceID)
	defer sourceView.free()
	rawCoordinates := cLatLngSlice(coordinates)
	var rawCoordinatesPtr *C.mln_lat_lng
	if len(rawCoordinates) > 0 {
		rawCoordinatesPtr = &rawCoordinates[0]
	}
	rawImage := newCPremultipliedRGBA8Image(image)
	defer rawImage.free()
	return startCompletion(func(completion *C.mln_completion) int32 {
		return int32(C.mln_map_add_image_source_image(
			C.mln_map(ptr),
			sourceView.raw(),
			rawCoordinatesPtr,
			C.size_t(len(rawCoordinates)),
			&rawImage.raw,
			completion,
		))
	}, completionCommand)
}

// SetImageSourceURL updates an image source to load its image from a URL.
func (m *MapHandle) SetImageSourceURL(sourceID string, url string) (*Future[CommandCompletion], error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}

	defer m.state.KeepAlive()
	sourceView := newCStringView(sourceID)
	defer sourceView.free()
	urlView := newCStringView(url)
	defer urlView.free()
	return startCompletion(func(completion *C.mln_completion) int32 {
		return int32(C.mln_map_set_image_source_url(C.mln_map(ptr), sourceView.raw(), urlView.raw(), completion))
	}, completionCommand)
}

// SetImageSourceImage updates an image source with inline image pixels.
func (m *MapHandle) SetImageSourceImage(sourceID string, image PremultipliedRGBA8Image) (*Future[CommandCompletion], error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}

	defer m.state.KeepAlive()
	sourceView := newCStringView(sourceID)
	defer sourceView.free()
	rawImage := newCPremultipliedRGBA8Image(image)
	defer rawImage.free()
	return startCompletion(func(completion *C.mln_completion) int32 {
		return int32(C.mln_map_set_image_source_image(C.mln_map(ptr), sourceView.raw(), &rawImage.raw, completion))
	}, completionCommand)
}

// SetImageSourceCoordinates updates image source coordinates.
func (m *MapHandle) SetImageSourceCoordinates(sourceID string, coordinates []LatLng) (*Future[CommandCompletion], error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}

	defer m.state.KeepAlive()
	sourceView := newCStringView(sourceID)
	defer sourceView.free()
	rawCoordinates := cLatLngSlice(coordinates)
	var rawCoordinatesPtr *C.mln_lat_lng
	if len(rawCoordinates) > 0 {
		rawCoordinatesPtr = &rawCoordinates[0]
	}
	return startCompletion(func(completion *C.mln_completion) int32 {
		return int32(C.mln_map_set_image_source_coordinates(
			C.mln_map(ptr),
			sourceView.raw(),
			rawCoordinatesPtr,
			C.size_t(len(rawCoordinates)),
			completion,
		))
	}, completionCommand)
}

// ImageSourceCoordinates returns copied image source coordinates.

// AddVectorSourceURL adds a vector source with a TileJSON URL.
func (m *MapHandle) AddVectorSourceURL(sourceID string, url string, options *StyleTileSourceOptions) (*Future[CommandCompletion], error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}

	defer m.state.KeepAlive()
	sourceView := newCStringView(sourceID)
	defer sourceView.free()
	urlView := newCStringView(url)
	defer urlView.free()
	rawOptions, rawOptionsPtr := cStyleTileSourceOptionsPointer(options)
	defer rawOptions.free()
	return startCompletion(func(completion *C.mln_completion) int32 {
		return int32(C.mln_map_add_vector_source_url(C.mln_map(ptr), sourceView.raw(), urlView.raw(), rawOptionsPtr, completion))
	}, completionCommand)
}

// AddVectorSourceTiles adds a vector source with inline tile URLs.
func (m *MapHandle) AddVectorSourceTiles(sourceID string, tiles []string, options *StyleTileSourceOptions) (*Future[CommandCompletion], error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}

	defer m.state.KeepAlive()
	sourceView := newCStringView(sourceID)
	defer sourceView.free()
	rawTiles := newCStringViewArray(tiles)
	defer rawTiles.free()
	rawOptions, rawOptionsPtr := cStyleTileSourceOptionsPointer(options)
	defer rawOptions.free()
	return startCompletion(func(completion *C.mln_completion) int32 {
		return int32(C.mln_map_add_vector_source_tiles(C.mln_map(ptr), sourceView.raw(), rawTiles.ptr(), rawTiles.count(), rawOptionsPtr, completion))
	}, completionCommand)
}

// AddRasterSourceURL adds a raster source with a TileJSON URL.
func (m *MapHandle) AddRasterSourceURL(sourceID string, url string, options *StyleTileSourceOptions) (*Future[CommandCompletion], error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}

	defer m.state.KeepAlive()
	sourceView := newCStringView(sourceID)
	defer sourceView.free()
	urlView := newCStringView(url)
	defer urlView.free()
	rawOptions, rawOptionsPtr := cStyleTileSourceOptionsPointer(options)
	defer rawOptions.free()
	return startCompletion(func(completion *C.mln_completion) int32 {
		return int32(C.mln_map_add_raster_source_url(C.mln_map(ptr), sourceView.raw(), urlView.raw(), rawOptionsPtr, completion))
	}, completionCommand)
}

// AddRasterSourceTiles adds a raster source with inline tile URLs.
func (m *MapHandle) AddRasterSourceTiles(sourceID string, tiles []string, options *StyleTileSourceOptions) (*Future[CommandCompletion], error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}

	defer m.state.KeepAlive()
	sourceView := newCStringView(sourceID)
	defer sourceView.free()
	rawTiles := newCStringViewArray(tiles)
	defer rawTiles.free()
	rawOptions, rawOptionsPtr := cStyleTileSourceOptionsPointer(options)
	defer rawOptions.free()
	return startCompletion(func(completion *C.mln_completion) int32 {
		return int32(C.mln_map_add_raster_source_tiles(C.mln_map(ptr), sourceView.raw(), rawTiles.ptr(), rawTiles.count(), rawOptionsPtr, completion))
	}, completionCommand)
}

// AddRasterDEMSourceURL adds a raster DEM source with a TileJSON URL.
func (m *MapHandle) AddRasterDEMSourceURL(sourceID string, url string, options *StyleTileSourceOptions) (*Future[CommandCompletion], error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}

	defer m.state.KeepAlive()
	sourceView := newCStringView(sourceID)
	defer sourceView.free()
	urlView := newCStringView(url)
	defer urlView.free()
	rawOptions, rawOptionsPtr := cStyleTileSourceOptionsPointer(options)
	defer rawOptions.free()
	return startCompletion(func(completion *C.mln_completion) int32 {
		return int32(C.mln_map_add_raster_dem_source_url(C.mln_map(ptr), sourceView.raw(), urlView.raw(), rawOptionsPtr, completion))
	}, completionCommand)
}

// AddRasterDEMSourceTiles adds a raster DEM source with inline tile URLs.
func (m *MapHandle) AddRasterDEMSourceTiles(sourceID string, tiles []string, options *StyleTileSourceOptions) (*Future[CommandCompletion], error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}

	defer m.state.KeepAlive()
	sourceView := newCStringView(sourceID)
	defer sourceView.free()
	rawTiles := newCStringViewArray(tiles)
	defer rawTiles.free()
	rawOptions, rawOptionsPtr := cStyleTileSourceOptionsPointer(options)
	defer rawOptions.free()
	return startCompletion(func(completion *C.mln_completion) int32 {
		return int32(C.mln_map_add_raster_dem_source_tiles(C.mln_map(ptr), sourceView.raw(), rawTiles.ptr(), rawTiles.count(), rawOptionsPtr, completion))
	}, completionCommand)
}

// AddStyleSourceJSON adds one style source from a style-spec source JSON object.
func (m *MapHandle) AddStyleSourceJSON(sourceID string, sourceJSON []byte) (*Future[CommandCompletion], error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}

	defer m.state.KeepAlive()
	sourceView := newCStringView(sourceID)
	defer sourceView.free()
	rawJSON := newCBufferView(sourceJSON)
	defer rawJSON.free()
	return startCompletion(func(completion *C.mln_completion) int32 {
		return int32(C.mln_map_add_style_source_json(C.mln_map(ptr), sourceView.raw(), rawJSON.raw(), completion))
	}, completionCommand)
}

// RemoveStyleSource submits a command that removes one style source by ID. The
// command commits when a source with the ID existed and was removed. It fails
// with ErrNotFound reported through its terminal event's Err payload field when
// none does, and with ErrInvalidState when a layer still uses the source. Check
// existence with StyleSourceInfo's found flag.
func (m *MapHandle) RemoveStyleSource(sourceID string) (*Future[CommandCompletion], error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}

	defer m.state.KeepAlive()
	sourceView := newCStringView(sourceID)
	defer sourceView.free()
	return startCompletion(func(completion *C.mln_completion) int32 {
		return int32(C.mln_map_remove_style_source(C.mln_map(ptr), sourceView.raw(), completion))
	}, completionCommand)
}

func styleIDListStrings(list C.mln_style_id_list) ([]string, error) {
	defer C.mln_style_id_list_destroy(list)
	var count C.size_t
	if err := checkNative(func() int32 { return int32(C.mln_style_id_list_count(list, &count)) }); err != nil {
		return nil, err
	}
	ids := make([]string, int(count))
	for i := range ids {
		var view C.mln_buffer_view
		if err := checkNative(func() int32 { return int32(C.mln_style_id_list_get(list, C.size_t(i), &view)) }); err != nil {
			return nil, err
		}
		ids[i] = goStringView(view)
	}
	return ids, nil
}

func styleStringListStrings(list C.mln_style_string_list) ([]string, error) {
	defer C.mln_style_string_list_destroy(list)
	var count C.size_t
	if err := checkNative(func() int32 { return int32(C.mln_style_string_list_count(list, &count)) }); err != nil {
		return nil, err
	}
	values := make([]string, int(count))
	for i := range values {
		var view C.mln_buffer_view
		if err := checkNative(func() int32 {
			return int32(C.mln_style_string_list_get(list, C.size_t(i), &view))
		}); err != nil {
			return nil, err
		}
		values[i] = goStringView(view)
	}
	return values, nil
}

// AddHillshadeLayer adds a hillshade layer for a raster DEM source. Passing an
// empty beforeLayerID appends the layer.
func (m *MapHandle) AddHillshadeLayer(layerID string, sourceID string, beforeLayerID string) (*Future[CommandCompletion], error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}

	defer m.state.KeepAlive()
	layerView := newCStringView(layerID)
	defer layerView.free()
	sourceView := newCStringView(sourceID)
	defer sourceView.free()
	beforeView := newCStringView(beforeLayerID)
	defer beforeView.free()
	return startCompletion(func(completion *C.mln_completion) int32 {
		return int32(C.mln_map_add_hillshade_layer(C.mln_map(ptr), layerView.raw(), sourceView.raw(), beforeView.raw(), completion))
	}, completionCommand)
}

// AddColorReliefLayer adds a color-relief layer for a raster DEM source.
// Passing an empty beforeLayerID appends the layer.
func (m *MapHandle) AddColorReliefLayer(layerID string, sourceID string, beforeLayerID string) (*Future[CommandCompletion], error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}

	defer m.state.KeepAlive()
	layerView := newCStringView(layerID)
	defer layerView.free()
	sourceView := newCStringView(sourceID)
	defer sourceView.free()
	beforeView := newCStringView(beforeLayerID)
	defer beforeView.free()
	return startCompletion(func(completion *C.mln_completion) int32 {
		return int32(C.mln_map_add_color_relief_layer(C.mln_map(ptr), layerView.raw(), sourceView.raw(), beforeView.raw(), completion))
	}, completionCommand)
}

// AddLocationIndicatorLayer adds a source-free location indicator layer. Passing
// an empty beforeLayerID appends the layer.
func (m *MapHandle) AddLocationIndicatorLayer(layerID string, beforeLayerID string) (*Future[CommandCompletion], error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}

	defer m.state.KeepAlive()
	layerView := newCStringView(layerID)
	defer layerView.free()
	beforeView := newCStringView(beforeLayerID)
	defer beforeView.free()
	return startCompletion(func(completion *C.mln_completion) int32 {
		return int32(C.mln_map_add_location_indicator_layer(C.mln_map(ptr), layerView.raw(), beforeView.raw(), completion))
	}, completionCommand)
}

// SetLocationIndicatorLocation sets a location indicator layer location.
func (m *MapHandle) SetLocationIndicatorLocation(layerID string, coordinate LatLng, altitude float64) (*Future[CommandCompletion], error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}

	defer m.state.KeepAlive()
	layerView := newCStringView(layerID)
	defer layerView.free()
	return startCompletion(func(completion *C.mln_completion) int32 {
		return int32(C.mln_map_set_location_indicator_location(C.mln_map(ptr), layerView.raw(), cLatLng(coordinate), C.double(altitude), completion))
	}, completionCommand)
}

// SetLocationIndicatorBearing sets a location indicator layer bearing in degrees.
func (m *MapHandle) SetLocationIndicatorBearing(layerID string, bearing float64) (*Future[CommandCompletion], error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}

	defer m.state.KeepAlive()
	layerView := newCStringView(layerID)
	defer layerView.free()
	return startCompletion(func(completion *C.mln_completion) int32 {
		return int32(C.mln_map_set_location_indicator_bearing(C.mln_map(ptr), layerView.raw(), C.double(bearing), completion))
	}, completionCommand)
}

// SetLocationIndicatorAccuracyRadius sets a location indicator layer accuracy
// radius in meters.
func (m *MapHandle) SetLocationIndicatorAccuracyRadius(layerID string, radius float64) (*Future[CommandCompletion], error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}

	defer m.state.KeepAlive()
	layerView := newCStringView(layerID)
	defer layerView.free()
	return startCompletion(func(completion *C.mln_completion) int32 {
		return int32(C.mln_map_set_location_indicator_accuracy_radius(C.mln_map(ptr), layerView.raw(), C.double(radius), completion))
	}, completionCommand)
}

// SetLocationIndicatorImageName sets one location indicator image-name property.
func (m *MapHandle) SetLocationIndicatorImageName(layerID string, imageKind LocationIndicatorImageKind, imageID string) (*Future[CommandCompletion], error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}

	defer m.state.KeepAlive()
	layerView := newCStringView(layerID)
	defer layerView.free()
	imageView := newCStringView(imageID)
	defer imageView.free()
	return startCompletion(func(completion *C.mln_completion) int32 {
		return int32(C.mln_map_set_location_indicator_image_name(C.mln_map(ptr), layerView.raw(), C.uint32_t(imageKind), imageView.raw(), completion))
	}, completionCommand)
}

// AddStyleLayerJSON adds one style layer from a style-spec layer JSON object.
// Passing an empty beforeLayerID appends the layer.
func (m *MapHandle) AddStyleLayerJSON(layerJSON []byte, beforeLayerID string) (*Future[CommandCompletion], error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}

	defer m.state.KeepAlive()
	beforeView := newCStringView(beforeLayerID)
	defer beforeView.free()
	rawJSON := newCBufferView(layerJSON)
	defer rawJSON.free()
	return startCompletion(func(completion *C.mln_completion) int32 {
		return int32(C.mln_map_add_style_layer_json(C.mln_map(ptr), rawJSON.raw(), beforeView.raw(), completion))
	}, completionCommand)
}

// RemoveStyleLayer submits a command that removes one style layer by ID. The
// command commits when a layer with the ID existed and was removed, and fails
// with ErrNotFound reported through its terminal event's Err payload field when
// none does. Check existence with StyleLayerInfo's found flag.
func (m *MapHandle) RemoveStyleLayer(layerID string) (*Future[CommandCompletion], error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}

	defer m.state.KeepAlive()
	layerView := newCStringView(layerID)
	defer layerView.free()
	return startCompletion(func(completion *C.mln_completion) int32 {
		return int32(C.mln_map_remove_style_layer(C.mln_map(ptr), layerView.raw(), completion))
	}, completionCommand)
}

// StyleLayerInfo returns copied layer metadata and whether the layer exists.

// StyleLayerIDs returns copied layer IDs in style order.

// MoveStyleLayer moves one style layer before another layer. Passing an empty
// beforeLayerID moves layerID to the top of the style order.
func (m *MapHandle) MoveStyleLayer(layerID string, beforeLayerID string) (*Future[CommandCompletion], error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}

	defer m.state.KeepAlive()
	layerView := newCStringView(layerID)
	defer layerView.free()
	beforeView := newCStringView(beforeLayerID)
	defer beforeView.free()
	return startCompletion(func(completion *C.mln_completion) int32 {
		return int32(C.mln_map_move_style_layer(C.mln_map(ptr), layerView.raw(), beforeView.raw(), completion))
	}, completionCommand)
}

// StyleLayerJSON returns one copied style layer as a style-spec JSON object and
// whether the layer exists.

// SetStyleLightJSON sets the style light from a style-spec light JSON object.
func (m *MapHandle) SetStyleLightJSON(lightJSON []byte) (*Future[CommandCompletion], error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}

	defer m.state.KeepAlive()
	rawJSON := newCBufferView(lightJSON)
	defer rawJSON.free()
	return startCompletion(func(completion *C.mln_completion) int32 {
		return int32(C.mln_map_set_style_light_json(C.mln_map(ptr), rawJSON.raw(), completion))
	}, completionCommand)
}

// SetStyleLightProperty sets one style light property.
func (m *MapHandle) SetStyleLightProperty(propertyName string, value []byte) (*Future[CommandCompletion], error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}

	defer m.state.KeepAlive()
	propertyView := newCStringView(propertyName)
	defer propertyView.free()
	rawValue := newCBufferView(value)
	defer rawValue.free()
	return startCompletion(func(completion *C.mln_completion) int32 {
		return int32(C.mln_map_set_style_light_property(C.mln_map(ptr), propertyView.raw(), rawValue.raw(), completion))
	}, completionCommand)
}

// StyleLightProperty returns one copied style light property as a style-spec
// JSON value.

// SetStyleTransitionOptions replaces the style's global transition options
// rather than merging into them, so absent duration and delay clear the
// style-wide override. Loading a style replaces these options with the ones
// that style declares, so apply an override after the style loads.
func (m *MapHandle) SetStyleTransitionOptions(options StyleTransitionOptions) (*Future[CommandCompletion], error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}

	defer m.state.KeepAlive()
	raw := newCStyleTransitionOptions(options)
	return startCompletion(func(completion *C.mln_completion) int32 {
		return int32(C.mln_map_set_style_transition_options(C.mln_map(ptr), &raw, completion))
	}, completionCommand)
}

// StyleTransitionOptions returns the style's copied global transition options.

// SetLayerProperty sets one style layer property.
func (m *MapHandle) SetLayerProperty(layerID string, propertyName string, value []byte) (*Future[CommandCompletion], error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}

	defer m.state.KeepAlive()
	layerView := newCStringView(layerID)
	defer layerView.free()
	propertyView := newCStringView(propertyName)
	defer propertyView.free()
	rawValue := newCBufferView(value)
	defer rawValue.free()
	return startCompletion(func(completion *C.mln_completion) int32 {
		return int32(C.mln_map_set_layer_property(C.mln_map(ptr), layerView.raw(), propertyView.raw(), rawValue.raw(), completion))
	}, completionCommand)
}

// LayerProperty returns one copied style layer property as a style-spec JSON
// value.

// StyleLayerVisibility reports whether a style layer draws.
type StyleLayerVisibility uint32

// Style layer visibility values.
const (
	StyleLayerVisibilityVisible StyleLayerVisibility = StyleLayerVisibility(C.MLN_STYLE_LAYER_VISIBILITY_VISIBLE)
	StyleLayerVisibilityNone    StyleLayerVisibility = StyleLayerVisibility(C.MLN_STYLE_LAYER_VISIBILITY_NONE)
)

// SetLayerSourceLayer sets one layer's source-layer ID. Layer types that take no
// source, such as background, are rejected.

// LayerSourceLayer returns one layer's source-layer ID, empty when the layer
// carries none.

// SetLayerSourceID sets one layer's source ID. Layer types that take no source,
// such as background, are rejected. The named source need not exist yet.

// LayerSourceID returns one layer's source ID, empty when the layer carries
// none.

// SetLayerMinZoom sets the lowest zoom at which one layer draws. Pass
// math.Inf(-1) for no lower bound. Read the committed range with
// StyleLayerInfo.

// SetLayerMaxZoom sets the highest zoom at which one layer draws. Pass
// math.Inf(1) for no upper bound. Read the committed range with
// StyleLayerInfo.

// SetLayerVisibility sets whether one layer draws. Read the committed value
// with StyleLayerInfo.
func (m *MapHandle) SetLayerVisibility(layerID string, visibility StyleLayerVisibility) (*Future[CommandCompletion], error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}

	defer m.state.KeepAlive()
	layerView := newCStringView(layerID)
	defer layerView.free()
	return startCompletion(func(completion *C.mln_completion) int32 {
		return int32(C.mln_map_set_layer_visibility(C.mln_map(ptr), layerView.raw(), C.uint32_t(visibility), completion))
	}, completionCommand)
}

// copyMapText probes the required length, then copies. A null buffer with zero
// capacity is a size probe the C API answers with OK.
func (m *MapHandle) copyMapText(copy func(C.mln_map, *C.char, C.size_t, *C.size_t) int32) (string, error) {
	ptr, err := m.ptr()
	if err != nil {
		return "", err
	}

	defer m.state.KeepAlive()

	var required C.size_t
	if err := checkNative(func() int32 {
		return copy(C.mln_map(ptr), nil, 0, &required)
	}); err != nil {
		return "", err
	}
	if required == 0 {
		return "", nil
	}

	buffer := make([]byte, int(required))
	var size C.size_t
	if err := checkNative(func() int32 {
		return copy(C.mln_map(ptr), (*C.char)(unsafe.Pointer(&buffer[0])), C.size_t(len(buffer)), &size)
	}); err != nil {
		return "", err
	}
	return string(buffer[:int(size)]), nil
}

func (m *MapHandle) copyMapBytes(copy func(C.mln_map, *C.uint8_t, C.size_t, *C.size_t) int32) ([]byte, error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}

	defer m.state.KeepAlive()

	var required C.size_t
	if err := checkNative(func() int32 { return copy(C.mln_map(ptr), nil, 0, &required) }); err != nil {
		return nil, err
	}
	if required == 0 {
		return []byte{}, nil
	}
	buffer := make([]byte, int(required))
	var size C.size_t
	if err := checkNative(func() int32 {
		return copy(C.mln_map(ptr), (*C.uint8_t)(unsafe.Pointer(&buffer[0])), C.size_t(len(buffer)), &size)
	}); err != nil {
		return nil, err
	}
	return buffer[:int(size)], nil
}

// SetLayerFilter sets or clears one style layer filter. Passing nil clears the
// filter.
func (m *MapHandle) SetLayerFilter(layerID string, filter []byte) (*Future[CommandCompletion], error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}

	defer m.state.KeepAlive()
	layerView := newCStringView(layerID)
	defer layerView.free()
	var rawFilter *C.mln_buffer_view
	var filterView cBufferView
	if filter != nil {
		filterView = newCBufferView(filter)
		defer filterView.free()
		rawFilter = filterView.ptr()
	}
	return startCompletion(func(completion *C.mln_completion) int32 {
		return int32(C.mln_map_set_layer_filter(C.mln_map(ptr), layerView.raw(), rawFilter, completion))
	}, completionCommand)
}

// LayerFilter returns one copied style layer filter as a style-spec JSON value.

// StyleOptional is a copied style read that may not find its requested value.
type StyleOptional[T any] struct {
	Value T
	Found bool
}

// StyleImageStretchResult contains both stretch axes for one style image.
type StyleImageStretchResult struct {
	X []ImageStretch
	Y []ImageStretch
}

func copyStyleViews(raw *C.mln_buffer_view, count C.size_t) ([]string, error) {
	if count == 0 {
		return []string{}, nil
	}
	if raw == nil {
		return nil, newBindingError(ErrNative, "native style string array is invalid")
	}
	views := unsafe.Slice(raw, int(count))
	values := make([]string, len(views))
	for index := range views {
		values[index] = goStringView(views[index])
	}
	return values, nil
}

func completionOptionalBuffer(result *C.mln_completion_result) (StyleOptional[[]byte], error) {
	if result.value_count == 0 {
		return StyleOptional[[]byte]{}, nil
	}
	value, err := completionBuffer(result)
	return StyleOptional[[]byte]{Value: value, Found: err == nil}, err
}

func completionStyleImage(result *C.mln_completion_result) (StyleOptional[StyleImage], error) {
	if result.value_count == 0 {
		return StyleOptional[StyleImage]{}, nil
	}
	raw, err := completionValue[C.mln_style_image_result](result)
	if err != nil {
		return StyleOptional[StyleImage]{}, err
	}
	pixels, ok := goByteSlice(raw.pixels.data, raw.pixels.size)
	if !ok {
		return StyleOptional[StyleImage]{}, newBindingError(ErrNative, "native style image pixels are invalid")
	}
	x := append([]C.mln_image_stretch(nil), unsafe.Slice(raw.stretch_x, int(raw.stretch_x_count))...)
	y := append([]C.mln_image_stretch(nil), unsafe.Slice(raw.stretch_y, int(raw.stretch_y_count))...)
	return StyleOptional[StyleImage]{
		Found: true,
		Value: StyleImage{
			Info: styleImageInfoFromC(raw.info), Pixels: pixels,
			StretchX: stretchesFromC(x), StretchY: stretchesFromC(y),
		},
	}, nil
}

func completionStyleSource(result *C.mln_completion_result) (StyleOptional[StyleSourceInfo], error) {
	if result.value_count == 0 {
		return StyleOptional[StyleSourceInfo]{}, nil
	}
	raw, err := completionValue[C.mln_style_source_result](result)
	if err != nil {
		return StyleOptional[StyleSourceInfo]{}, err
	}
	info := styleSourceInfoFromC(raw.info)
	if bool(raw.info.has_attribution) {
		value := goStringView(raw.attribution)
		info.Attribution = &value
	}
	if raw.info.fields&C.MLN_STYLE_SOURCE_INFO_URL != 0 {
		value := goStringView(raw.url)
		info.URL = &value
	}
	if info.TileJSON != nil {
		info.TileJSON.TileURLs, err = copyStyleViews(raw.tile_urls, raw.tile_url_count)
		if err != nil {
			return StyleOptional[StyleSourceInfo]{}, err
		}
	}
	return StyleOptional[StyleSourceInfo]{Value: info, Found: true}, nil
}

func completionStyleLayer(result *C.mln_completion_result) (StyleOptional[StyleLayerInfo], error) {
	if result.value_count == 0 {
		return StyleOptional[StyleLayerInfo]{}, nil
	}
	raw, err := completionValue[C.mln_style_layer_result](result)
	if err != nil {
		return StyleOptional[StyleLayerInfo]{}, err
	}
	info := styleLayerInfoFromC(raw.info)
	if info.HasSourceID {
		value := goStringView(raw.source_id)
		info.SourceID = &value
	}
	if info.HasSourceLayer {
		value := goStringView(raw.source_layer)
		info.SourceLayer = &value
	}
	return StyleOptional[StyleLayerInfo]{Value: info, Found: true}, nil
}

func completionStyleIDs(result *C.mln_completion_result) ([]string, error) {
	return copyStyleViews((*C.mln_buffer_view)(result.value), C.size_t(result.value_count))
}

func (m *MapHandle) AddCustomGeometrySource(sourceID string, options CustomGeometrySourceOptions) (*Future[CommandCompletion], error) {
	if sourceID == "" {
		return nil, newBindingError(ErrInvalidArgument, "source ID is empty")
	}
	if options.FetchTile == nil {
		return nil, newBindingError(ErrInvalidArgument, "CustomGeometrySourceOptions.FetchTile is nil")
	}
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}
	defer m.state.KeepAlive()
	return startCompletion(func(completion *C.mln_completion) int32 {
		return callback.AddCustomGeometrySource(
			uint64(ptr), sourceID, options.toCallback(), unsafe.Pointer(completion),
		)
	}, completionCommand)
}

func (m *MapHandle) StyleImage(imageID string) (*Future[StyleOptional[StyleImage]], error) {
	view := newCStringView(imageID)
	defer view.free()
	return startMapCompletion(m, func(raw C.mln_map, completion *C.mln_completion) int32 {
		return int32(C.mln_map_get_style_image_info(raw, view.raw(), completion))
	}, completionStyleImage)
}

func (m *MapHandle) ImageSourceCoordinates(sourceID string) (*Future[StyleOptional[[]LatLng]], error) {
	view := newCStringView(sourceID)
	defer view.free()
	return startMapCompletion(m, func(raw C.mln_map, completion *C.mln_completion) int32 {
		return int32(C.mln_map_get_image_source_coordinates(raw, view.raw(), completion))
	}, func(result *C.mln_completion_result) (StyleOptional[[]LatLng], error) {
		raw, err := completionSlice[C.mln_lat_lng](result)
		if err != nil {
			return StyleOptional[[]LatLng]{}, err
		}
		if len(raw) == 0 {
			return StyleOptional[[]LatLng]{}, nil
		}
		return StyleOptional[[]LatLng]{Value: goLatLngSlice(raw), Found: true}, nil
	})
}

func (m *MapHandle) StyleSourceInfo(sourceID string) (*Future[StyleOptional[StyleSourceInfo]], error) {
	view := newCStringView(sourceID)
	defer view.free()
	return startMapCompletion(m, func(raw C.mln_map, completion *C.mln_completion) int32 {
		return int32(C.mln_map_get_style_source_info(raw, view.raw(), completion))
	}, completionStyleSource)
}

func (m *MapHandle) StyleSourceIDs() (*Future[[]string], error) {
	return startMapCompletion(m, func(raw C.mln_map, completion *C.mln_completion) int32 {
		return int32(C.mln_map_list_style_source_ids(raw, completion))
	}, completionStyleIDs)
}

func (m *MapHandle) StyleLayerInfo(layerID string) (*Future[StyleOptional[StyleLayerInfo]], error) {
	view := newCStringView(layerID)
	defer view.free()
	return startMapCompletion(m, func(raw C.mln_map, completion *C.mln_completion) int32 {
		return int32(C.mln_map_get_style_layer_info(raw, view.raw(), completion))
	}, completionStyleLayer)
}

func (m *MapHandle) StyleLayerIDs() (*Future[[]string], error) {
	return startMapCompletion(m, func(raw C.mln_map, completion *C.mln_completion) int32 {
		return int32(C.mln_map_list_style_layer_ids(raw, completion))
	}, completionStyleIDs)
}

func (m *MapHandle) StyleLayerJSON(layerID string) (*Future[StyleOptional[[]byte]], error) {
	view := newCStringView(layerID)
	defer view.free()
	return startMapCompletion(m, func(raw C.mln_map, completion *C.mln_completion) int32 {
		return int32(C.mln_map_get_style_layer_json(raw, view.raw(), completion))
	}, completionOptionalBuffer)
}

func (m *MapHandle) StyleLightProperty(name string) (*Future[[]byte], error) {
	view := newCStringView(name)
	defer view.free()
	return startMapCompletion(m, func(raw C.mln_map, completion *C.mln_completion) int32 {
		return int32(C.mln_map_get_style_light_property(raw, view.raw(), completion))
	}, completionNullableBuffer)
}

func (m *MapHandle) StyleTransitionOptions() (*Future[StyleTransitionOptions], error) {
	return startMapCompletion(m, func(raw C.mln_map, completion *C.mln_completion) int32 {
		return int32(C.mln_map_get_style_transition_options(raw, completion))
	}, func(result *C.mln_completion_result) (StyleTransitionOptions, error) {
		raw, err := completionValue[C.mln_style_transition_options](result)
		return styleTransitionOptionsFromC(raw), err
	})
}

func (m *MapHandle) LayerProperty(layerID, name string) (*Future[[]byte], error) {
	layer := newCStringView(layerID)
	defer layer.free()
	property := newCStringView(name)
	defer property.free()
	return startMapCompletion(m, func(raw C.mln_map, completion *C.mln_completion) int32 {
		return int32(C.mln_map_get_layer_property(raw, layer.raw(), property.raw(), completion))
	}, completionNullableBuffer)
}

func (m *MapHandle) LayerFilter(layerID string) (*Future[StyleOptional[[]byte]], error) {
	view := newCStringView(layerID)
	defer view.free()
	return startMapCompletion(m, func(raw C.mln_map, completion *C.mln_completion) int32 {
		return int32(C.mln_map_get_layer_filter(raw, view.raw(), completion))
	}, completionOptionalBuffer)
}

func submitLayerTextCommand(m *MapHandle, layerID, text string, submit func(C.mln_map, C.mln_buffer_view, C.mln_buffer_view, *C.mln_completion) int32) (*Future[CommandCompletion], error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}

	defer m.state.KeepAlive()
	layer := newCStringView(layerID)
	defer layer.free()
	value := newCStringView(text)
	defer value.free()
	return startCompletion(func(completion *C.mln_completion) int32 {
		return submit(C.mln_map(ptr), layer.raw(), value.raw(), completion)
	}, completionCommand)
}

func (m *MapHandle) SetLayerSourceLayer(layerID, sourceLayer string) (*Future[CommandCompletion], error) {
	return submitLayerTextCommand(m, layerID, sourceLayer, func(raw C.mln_map, layer, value C.mln_buffer_view, completion *C.mln_completion) int32 {
		return int32(C.mln_map_set_layer_source_layer(raw, layer, value, completion))
	})
}

func (m *MapHandle) SetLayerSourceID(layerID, sourceID string) (*Future[CommandCompletion], error) {
	return submitLayerTextCommand(m, layerID, sourceID, func(raw C.mln_map, layer, value C.mln_buffer_view, completion *C.mln_completion) int32 {
		return int32(C.mln_map_set_layer_source_id(raw, layer, value, completion))
	})
}

func submitLayerZoomCommand(m *MapHandle, layerID string, zoom float64, submit func(C.mln_map, C.mln_buffer_view, C.double, *C.mln_completion) int32) (*Future[CommandCompletion], error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}

	defer m.state.KeepAlive()
	layer := newCStringView(layerID)
	defer layer.free()
	return startCompletion(func(completion *C.mln_completion) int32 {
		return submit(C.mln_map(ptr), layer.raw(), C.double(zoom), completion)
	}, completionCommand)
}

func (m *MapHandle) SetLayerMinZoom(layerID string, zoom float64) (*Future[CommandCompletion], error) {
	return submitLayerZoomCommand(m, layerID, zoom, func(raw C.mln_map, layer C.mln_buffer_view, value C.double, completion *C.mln_completion) int32 {
		return int32(C.mln_map_set_layer_min_zoom(raw, layer, value, completion))
	})
}

func (m *MapHandle) SetLayerMaxZoom(layerID string, zoom float64) (*Future[CommandCompletion], error) {
	return submitLayerZoomCommand(m, layerID, zoom, func(raw C.mln_map, layer C.mln_buffer_view, value C.double, completion *C.mln_completion) int32 {
		return int32(C.mln_map_set_layer_max_zoom(raw, layer, value, completion))
	})
}
