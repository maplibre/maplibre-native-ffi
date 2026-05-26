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
	StyleSourceTypeUnknown      StyleSourceType = StyleSourceType(C.MLN_STYLE_SOURCE_TYPE_UNKNOWN)
	StyleSourceTypeVector       StyleSourceType = StyleSourceType(C.MLN_STYLE_SOURCE_TYPE_VECTOR)
	StyleSourceTypeRaster       StyleSourceType = StyleSourceType(C.MLN_STYLE_SOURCE_TYPE_RASTER)
	StyleSourceTypeRasterDEM    StyleSourceType = StyleSourceType(C.MLN_STYLE_SOURCE_TYPE_RASTER_DEM)
	StyleSourceTypeGeoJSON      StyleSourceType = StyleSourceType(C.MLN_STYLE_SOURCE_TYPE_GEOJSON)
	StyleSourceTypeImage        StyleSourceType = StyleSourceType(C.MLN_STYLE_SOURCE_TYPE_IMAGE)
	StyleSourceTypeVideo        StyleSourceType = StyleSourceType(C.MLN_STYLE_SOURCE_TYPE_VIDEO)
	StyleSourceTypeAnnotations  StyleSourceType = StyleSourceType(C.MLN_STYLE_SOURCE_TYPE_ANNOTATIONS)
	StyleSourceTypeCustomVector StyleSourceType = StyleSourceType(C.MLN_STYLE_SOURCE_TYPE_CUSTOM_VECTOR)
)

// StyleSourceInfo contains fixed metadata for one style source.
type StyleSourceInfo struct {
	Type            StyleSourceType
	IDSize          uint64
	IsVolatile      bool
	HasAttribution  bool
	AttributionSize uint64
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

// CustomGeometryTileCallback receives custom geometry tile requests. Native
// code may invoke it on worker threads and may race it with owner-thread map
// calls. The callback must be thread-safe, must not call MapLibre map APIs
// directly, and should queue SetCustomGeometrySourceTileData or invalidation
// work back to the map owner thread. Panics are recovered and ignored.
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

// StyleImageOptions configures a runtime style image.
type StyleImageOptions struct {
	PixelRatio *float32
	SDF        *bool
}

func cStyleImageOptions(options StyleImageOptions) C.mln_style_image_options {
	raw := C.mln_style_image_options_default()
	if options.PixelRatio != nil {
		raw.fields |= C.MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO
		raw.pixel_ratio = C.float(*options.PixelRatio)
	}
	if options.SDF != nil {
		raw.fields |= C.MLN_STYLE_IMAGE_OPTION_SDF
		raw.sdf = C.bool(*options.SDF)
	}
	return raw
}

// StyleImageInfo contains copied runtime style image metadata.
type StyleImageInfo struct {
	Width      uint32
	Height     uint32
	Stride     uint32
	ByteLength uint64
	PixelRatio float32
	SDF        bool
}

type cPremultipliedRGBA8Image struct {
	raw        C.mln_premultiplied_rgba8_image
	allocation unsafe.Pointer
}

func newCPremultipliedRGBA8Image(image PremultipliedRGBA8Image) cPremultipliedRGBA8Image {
	raw := C.mln_premultiplied_rgba8_image_default()
	raw.width = C.uint32_t(image.Width)
	raw.height = C.uint32_t(image.Height)
	raw.stride = C.uint32_t(image.Stride)
	var allocation unsafe.Pointer
	if len(image.Pixels) > 0 {
		allocation = C.CBytes(image.Pixels)
		raw.pixels = (*C.uint8_t)(allocation)
	}
	raw.byte_length = C.size_t(len(image.Pixels))
	return cPremultipliedRGBA8Image{raw: raw, allocation: allocation}
}

func (image cPremultipliedRGBA8Image) free() {
	C.free(image.allocation)
}

func styleImageInfoFromC(info C.mln_style_image_info) StyleImageInfo {
	return StyleImageInfo{
		Width:      uint32(info.width),
		Height:     uint32(info.height),
		Stride:     uint32(info.stride),
		ByteLength: uint64(info.byte_length),
		PixelRatio: float32(info.pixel_ratio),
		SDF:        bool(info.sdf),
	}
}

// LocationIndicatorImageKind identifies an image-name slot on a location indicator layer.
type LocationIndicatorImageKind uint32

const (
	LocationIndicatorImageKindTop     LocationIndicatorImageKind = LocationIndicatorImageKind(C.MLN_LOCATION_INDICATOR_IMAGE_KIND_TOP)
	LocationIndicatorImageKindBearing LocationIndicatorImageKind = LocationIndicatorImageKind(C.MLN_LOCATION_INDICATOR_IMAGE_KIND_BEARING)
	LocationIndicatorImageKindShadow  LocationIndicatorImageKind = LocationIndicatorImageKind(C.MLN_LOCATION_INDICATOR_IMAGE_KIND_SHADOW)
)

func styleSourceInfoFromC(info C.mln_style_source_info) StyleSourceInfo {
	return StyleSourceInfo{
		Type:            StyleSourceType(info._type),
		IDSize:          uint64(info.id_size),
		IsVolatile:      bool(info.is_volatile),
		HasAttribution:  bool(info.has_attribution),
		AttributionSize: uint64(info.attribution_size),
	}
}

// AddGeoJSONSourceURL adds a GeoJSON source that loads from a URL.
func (m *MapHandle) AddGeoJSONSourceURL(sourceID string, url string) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	sourceView := newCStringView(sourceID)
	defer sourceView.free()
	urlView := newCStringView(url)
	defer urlView.free()
	return checkNative(func() int32 {
		return int32(C.mln_map_add_geojson_source_url((*C.mln_map)(unsafe.Pointer(ptr)), sourceView.raw(), urlView.raw()))
	})
}

// SetGeoJSONSourceURL updates a GeoJSON source to load from a URL.
func (m *MapHandle) SetGeoJSONSourceURL(sourceID string, url string) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	sourceView := newCStringView(sourceID)
	defer sourceView.free()
	urlView := newCStringView(url)
	defer urlView.free()
	return checkNative(func() int32 {
		return int32(C.mln_map_set_geojson_source_url((*C.mln_map)(unsafe.Pointer(ptr)), sourceView.raw(), urlView.raw()))
	})
}

// AddGeoJSONSourceData adds a GeoJSON source with inline data. Accepted data is
// copied into MapLibre Native before the call returns.
func (m *MapHandle) AddGeoJSONSourceData(sourceID string, data GeoJSON) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	sourceView := newCStringView(sourceID)
	defer sourceView.free()
	materializer := newCGeoJSONMaterializer()
	defer materializer.free()
	rawData, err := materializer.geoJSON(data)
	if err != nil {
		return newBindingError(ErrInvalidArgument, err.Error())
	}
	return checkNative(func() int32 {
		return int32(C.mln_map_add_geojson_source_data((*C.mln_map)(unsafe.Pointer(ptr)), sourceView.raw(), &rawData))
	})
}

// SetGeoJSONSourceData updates a GeoJSON source with inline data. Accepted data
// is copied into MapLibre Native before the call returns.
func (m *MapHandle) SetGeoJSONSourceData(sourceID string, data GeoJSON) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	sourceView := newCStringView(sourceID)
	defer sourceView.free()
	materializer := newCGeoJSONMaterializer()
	defer materializer.free()
	rawData, err := materializer.geoJSON(data)
	if err != nil {
		return newBindingError(ErrInvalidArgument, err.Error())
	}
	return checkNative(func() int32 {
		return int32(C.mln_map_set_geojson_source_data((*C.mln_map)(unsafe.Pointer(ptr)), sourceView.raw(), &rawData))
	})
}

// AddCustomGeometrySource adds a custom geometry source to the current style.
// Callback state remains valid until source removal, style replacement, or map
// close. URL style replacement completes asynchronously; after SetStyleURL,
// detached custom geometry callback state is released when RuntimeHandle.PollEvent
// observes RuntimeEventMapStyleLoaded for this map.
func (m *MapHandle) AddCustomGeometrySource(sourceID string, options CustomGeometrySourceOptions) error {
	if options.FetchTile == nil {
		return newBindingError(ErrInvalidArgument, "CustomGeometrySourceOptions.FetchTile is nil")
	}
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()

	var replacement *callback.CustomGeometrySourceState
	if err := checkNative(func() int32 {
		state, status := callback.AddCustomGeometrySource(unsafe.Pointer(ptr), sourceID, options.toCallback())
		replacement = state
		return status
	}); err != nil {
		return err
	}

	m.customGeometryMu.Lock()
	if m.customGeometrySources == nil {
		m.customGeometrySources = make(map[string]*callback.CustomGeometrySourceState)
	}
	previous := m.customGeometrySources[sourceID]
	m.customGeometrySources[sourceID] = replacement
	m.customGeometryMu.Unlock()
	previous.Release()
	return nil
}

// SetCustomGeometrySourceTileData sets custom geometry data for one tile.
func (m *MapHandle) SetCustomGeometrySourceTileData(sourceID string, tileID CanonicalTileID, data GeoJSON) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	sourceView := newCStringView(sourceID)
	defer sourceView.free()
	materializer := newCGeoJSONMaterializer()
	defer materializer.free()
	rawData, err := materializer.geoJSON(data)
	if err != nil {
		return newBindingError(ErrInvalidArgument, err.Error())
	}
	return checkNative(func() int32 {
		return int32(C.mln_map_set_custom_geometry_source_tile_data(
			(*C.mln_map)(unsafe.Pointer(ptr)),
			sourceView.raw(),
			cCanonicalTileID(tileID),
			&rawData,
		))
	})
}

// InvalidateCustomGeometrySourceTile invalidates custom geometry data for one tile.
func (m *MapHandle) InvalidateCustomGeometrySourceTile(sourceID string, tileID CanonicalTileID) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	sourceView := newCStringView(sourceID)
	defer sourceView.free()
	return checkNative(func() int32 {
		return int32(C.mln_map_invalidate_custom_geometry_source_tile((*C.mln_map)(unsafe.Pointer(ptr)), sourceView.raw(), cCanonicalTileID(tileID)))
	})
}

// InvalidateCustomGeometrySourceRegion invalidates custom geometry data inside one geographic region.
func (m *MapHandle) InvalidateCustomGeometrySourceRegion(sourceID string, bounds LatLngBounds) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	sourceView := newCStringView(sourceID)
	defer sourceView.free()
	return checkNative(func() int32 {
		return int32(C.mln_map_invalidate_custom_geometry_source_region((*C.mln_map)(unsafe.Pointer(ptr)), sourceView.raw(), cLatLngBounds(bounds)))
	})
}

// SetStyleImage sets or replaces one runtime style image.
func (m *MapHandle) SetStyleImage(imageID string, image PremultipliedRGBA8Image, options StyleImageOptions) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	imageView := newCStringView(imageID)
	defer imageView.free()
	rawImage := newCPremultipliedRGBA8Image(image)
	defer rawImage.free()
	rawOptions := cStyleImageOptions(options)
	return checkNative(func() int32 {
		return int32(C.mln_map_set_style_image((*C.mln_map)(unsafe.Pointer(ptr)), imageView.raw(), &rawImage.raw, &rawOptions))
	})
}

// RemoveStyleImage removes one runtime style image and reports whether it existed.
func (m *MapHandle) RemoveStyleImage(imageID string) (bool, error) {
	ptr, err := m.ptr()
	if err != nil {
		return false, err
	}
	defer m.state.KeepAlive()
	imageView := newCStringView(imageID)
	defer imageView.free()
	var removed C.bool
	if err := checkNative(func() int32 {
		return int32(C.mln_map_remove_style_image((*C.mln_map)(unsafe.Pointer(ptr)), imageView.raw(), &removed))
	}); err != nil {
		return false, err
	}
	return bool(removed), nil
}

// StyleImageExists reports whether one runtime style image exists.
func (m *MapHandle) StyleImageExists(imageID string) (bool, error) {
	ptr, err := m.ptr()
	if err != nil {
		return false, err
	}
	defer m.state.KeepAlive()
	imageView := newCStringView(imageID)
	defer imageView.free()
	var exists C.bool
	if err := checkNative(func() int32 {
		return int32(C.mln_map_style_image_exists((*C.mln_map)(unsafe.Pointer(ptr)), imageView.raw(), &exists))
	}); err != nil {
		return false, err
	}
	return bool(exists), nil
}

// StyleImageInfo returns copied metadata for one runtime style image.
func (m *MapHandle) StyleImageInfo(imageID string) (StyleImageInfo, bool, error) {
	ptr, err := m.ptr()
	if err != nil {
		return StyleImageInfo{}, false, err
	}
	defer m.state.KeepAlive()
	imageView := newCStringView(imageID)
	defer imageView.free()
	info := C.mln_style_image_info_default()
	var found C.bool
	if err := checkNative(func() int32 {
		return int32(C.mln_map_get_style_image_info((*C.mln_map)(unsafe.Pointer(ptr)), imageView.raw(), &info, &found))
	}); err != nil {
		return StyleImageInfo{}, false, err
	}
	return styleImageInfoFromC(info), bool(found), nil
}

// StyleImagePremultipliedRGBA8 returns copied tightly packed premultiplied RGBA8 pixels.
func (m *MapHandle) StyleImagePremultipliedRGBA8(imageID string) ([]byte, bool, error) {
	info, found, err := m.StyleImageInfo(imageID)
	if err != nil || !found {
		return nil, found, err
	}
	pixels := make([]byte, int(info.ByteLength))
	byteLength, found, err := m.StyleImagePremultipliedRGBA8Into(imageID, pixels)
	if err != nil {
		return nil, found, err
	}
	return pixels[:int(byteLength)], found, nil
}

// StyleImagePremultipliedRGBA8Into copies tightly packed premultiplied RGBA8 pixels into buffer.
func (m *MapHandle) StyleImagePremultipliedRGBA8Into(imageID string, buffer []byte) (uint64, bool, error) {
	ptr, err := m.ptr()
	if err != nil {
		return 0, false, err
	}
	defer m.state.KeepAlive()
	imageView := newCStringView(imageID)
	defer imageView.free()
	var rawBuffer *C.uint8_t
	if len(buffer) > 0 {
		rawBuffer = (*C.uint8_t)(unsafe.Pointer(&buffer[0]))
	}
	var byteLength C.size_t
	var found C.bool
	if err := checkNative(func() int32 {
		return int32(C.mln_map_copy_style_image_premultiplied_rgba8(
			(*C.mln_map)(unsafe.Pointer(ptr)),
			imageView.raw(),
			rawBuffer,
			C.size_t(len(buffer)),
			&byteLength,
			&found,
		))
	}); err != nil {
		runtime.KeepAlive(buffer)
		return uint64(byteLength), bool(found), err
	}
	runtime.KeepAlive(buffer)
	return uint64(byteLength), bool(found), nil
}

// AddImageSourceURL adds an image source that loads its image from a URL.
func (m *MapHandle) AddImageSourceURL(sourceID string, coordinates []LatLng, url string) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
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
	return checkNative(func() int32 {
		return int32(C.mln_map_add_image_source_url(
			(*C.mln_map)(unsafe.Pointer(ptr)),
			sourceView.raw(),
			rawCoordinatesPtr,
			C.size_t(len(rawCoordinates)),
			urlView.raw(),
		))
	})
}

// AddImageSourceImage adds an image source with inline image pixels.
func (m *MapHandle) AddImageSourceImage(sourceID string, coordinates []LatLng, image PremultipliedRGBA8Image) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
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
	return checkNative(func() int32 {
		return int32(C.mln_map_add_image_source_image(
			(*C.mln_map)(unsafe.Pointer(ptr)),
			sourceView.raw(),
			rawCoordinatesPtr,
			C.size_t(len(rawCoordinates)),
			&rawImage.raw,
		))
	})
}

// SetImageSourceURL updates an image source to load its image from a URL.
func (m *MapHandle) SetImageSourceURL(sourceID string, url string) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	sourceView := newCStringView(sourceID)
	defer sourceView.free()
	urlView := newCStringView(url)
	defer urlView.free()
	return checkNative(func() int32 {
		return int32(C.mln_map_set_image_source_url((*C.mln_map)(unsafe.Pointer(ptr)), sourceView.raw(), urlView.raw()))
	})
}

// SetImageSourceImage updates an image source with inline image pixels.
func (m *MapHandle) SetImageSourceImage(sourceID string, image PremultipliedRGBA8Image) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	sourceView := newCStringView(sourceID)
	defer sourceView.free()
	rawImage := newCPremultipliedRGBA8Image(image)
	defer rawImage.free()
	return checkNative(func() int32 {
		return int32(C.mln_map_set_image_source_image((*C.mln_map)(unsafe.Pointer(ptr)), sourceView.raw(), &rawImage.raw))
	})
}

// SetImageSourceCoordinates updates image source coordinates.
func (m *MapHandle) SetImageSourceCoordinates(sourceID string, coordinates []LatLng) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	sourceView := newCStringView(sourceID)
	defer sourceView.free()
	rawCoordinates := cLatLngSlice(coordinates)
	var rawCoordinatesPtr *C.mln_lat_lng
	if len(rawCoordinates) > 0 {
		rawCoordinatesPtr = &rawCoordinates[0]
	}
	return checkNative(func() int32 {
		return int32(C.mln_map_set_image_source_coordinates(
			(*C.mln_map)(unsafe.Pointer(ptr)),
			sourceView.raw(),
			rawCoordinatesPtr,
			C.size_t(len(rawCoordinates)),
		))
	})
}

// ImageSourceCoordinates returns copied image source coordinates.
func (m *MapHandle) ImageSourceCoordinates(sourceID string) ([]LatLng, bool, error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, false, err
	}
	defer m.state.KeepAlive()
	sourceView := newCStringView(sourceID)
	defer sourceView.free()
	rawCoordinates := make([]C.mln_lat_lng, 4)
	var count C.size_t
	var found C.bool
	if err := checkNative(func() int32 {
		return int32(C.mln_map_get_image_source_coordinates(
			(*C.mln_map)(unsafe.Pointer(ptr)),
			sourceView.raw(),
			&rawCoordinates[0],
			C.size_t(len(rawCoordinates)),
			&count,
			&found,
		))
	}); err != nil {
		return nil, false, err
	}
	return goLatLngSlice(rawCoordinates[:int(count)]), bool(found), nil
}

// AddVectorSourceURL adds a vector source with a TileJSON URL.
func (m *MapHandle) AddVectorSourceURL(sourceID string, url string, options *StyleTileSourceOptions) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	sourceView := newCStringView(sourceID)
	defer sourceView.free()
	urlView := newCStringView(url)
	defer urlView.free()
	rawOptions, rawOptionsPtr := cStyleTileSourceOptionsPointer(options)
	defer rawOptions.free()
	return checkNative(func() int32 {
		return int32(C.mln_map_add_vector_source_url((*C.mln_map)(unsafe.Pointer(ptr)), sourceView.raw(), urlView.raw(), rawOptionsPtr))
	})
}

// AddVectorSourceTiles adds a vector source with inline tile URLs.
func (m *MapHandle) AddVectorSourceTiles(sourceID string, tiles []string, options *StyleTileSourceOptions) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	sourceView := newCStringView(sourceID)
	defer sourceView.free()
	rawTiles := newCStringViewArray(tiles)
	defer rawTiles.free()
	rawOptions, rawOptionsPtr := cStyleTileSourceOptionsPointer(options)
	defer rawOptions.free()
	return checkNative(func() int32 {
		return int32(C.mln_map_add_vector_source_tiles((*C.mln_map)(unsafe.Pointer(ptr)), sourceView.raw(), rawTiles.ptr(), rawTiles.count(), rawOptionsPtr))
	})
}

// AddRasterSourceURL adds a raster source with a TileJSON URL.
func (m *MapHandle) AddRasterSourceURL(sourceID string, url string, options *StyleTileSourceOptions) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	sourceView := newCStringView(sourceID)
	defer sourceView.free()
	urlView := newCStringView(url)
	defer urlView.free()
	rawOptions, rawOptionsPtr := cStyleTileSourceOptionsPointer(options)
	defer rawOptions.free()
	return checkNative(func() int32 {
		return int32(C.mln_map_add_raster_source_url((*C.mln_map)(unsafe.Pointer(ptr)), sourceView.raw(), urlView.raw(), rawOptionsPtr))
	})
}

// AddRasterSourceTiles adds a raster source with inline tile URLs.
func (m *MapHandle) AddRasterSourceTiles(sourceID string, tiles []string, options *StyleTileSourceOptions) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	sourceView := newCStringView(sourceID)
	defer sourceView.free()
	rawTiles := newCStringViewArray(tiles)
	defer rawTiles.free()
	rawOptions, rawOptionsPtr := cStyleTileSourceOptionsPointer(options)
	defer rawOptions.free()
	return checkNative(func() int32 {
		return int32(C.mln_map_add_raster_source_tiles((*C.mln_map)(unsafe.Pointer(ptr)), sourceView.raw(), rawTiles.ptr(), rawTiles.count(), rawOptionsPtr))
	})
}

// AddRasterDEMSourceURL adds a raster DEM source with a TileJSON URL.
func (m *MapHandle) AddRasterDEMSourceURL(sourceID string, url string, options *StyleTileSourceOptions) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	sourceView := newCStringView(sourceID)
	defer sourceView.free()
	urlView := newCStringView(url)
	defer urlView.free()
	rawOptions, rawOptionsPtr := cStyleTileSourceOptionsPointer(options)
	defer rawOptions.free()
	return checkNative(func() int32 {
		return int32(C.mln_map_add_raster_dem_source_url((*C.mln_map)(unsafe.Pointer(ptr)), sourceView.raw(), urlView.raw(), rawOptionsPtr))
	})
}

// AddRasterDEMSourceTiles adds a raster DEM source with inline tile URLs.
func (m *MapHandle) AddRasterDEMSourceTiles(sourceID string, tiles []string, options *StyleTileSourceOptions) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	sourceView := newCStringView(sourceID)
	defer sourceView.free()
	rawTiles := newCStringViewArray(tiles)
	defer rawTiles.free()
	rawOptions, rawOptionsPtr := cStyleTileSourceOptionsPointer(options)
	defer rawOptions.free()
	return checkNative(func() int32 {
		return int32(C.mln_map_add_raster_dem_source_tiles((*C.mln_map)(unsafe.Pointer(ptr)), sourceView.raw(), rawTiles.ptr(), rawTiles.count(), rawOptionsPtr))
	})
}

// AddStyleSourceJSON adds one style source from a style-spec source JSON object.
// sourceJSON may contain nil, bool, string, integer, finite float, []any, and
// map[string]any values.
func (m *MapHandle) AddStyleSourceJSON(sourceID string, sourceJSON any) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	sourceView := newCStringView(sourceID)
	defer sourceView.free()
	materializer := newCJSONMaterializer()
	defer materializer.free()
	rawJSON, err := materializer.value(sourceJSON)
	if err != nil {
		return newBindingError(ErrInvalidArgument, err.Error())
	}
	return checkNative(func() int32 {
		return int32(C.mln_map_add_style_source_json((*C.mln_map)(unsafe.Pointer(ptr)), sourceView.raw(), &rawJSON))
	})
}

// RemoveStyleSource removes one style source by ID and reports whether it was
// present.
func (m *MapHandle) RemoveStyleSource(sourceID string) (bool, error) {
	ptr, err := m.ptr()
	if err != nil {
		return false, err
	}
	defer m.state.KeepAlive()
	sourceView := newCStringView(sourceID)
	defer sourceView.free()
	var removed C.bool
	if err := checkNative(func() int32 {
		return int32(C.mln_map_remove_style_source((*C.mln_map)(unsafe.Pointer(ptr)), sourceView.raw(), &removed))
	}); err != nil {
		return false, err
	}
	if bool(removed) {
		m.releaseCustomGeometrySource(sourceID)
	}
	return bool(removed), nil
}

// StyleSourceExists reports whether one style source ID exists.
func (m *MapHandle) StyleSourceExists(sourceID string) (bool, error) {
	ptr, err := m.ptr()
	if err != nil {
		return false, err
	}
	defer m.state.KeepAlive()
	sourceView := newCStringView(sourceID)
	defer sourceView.free()
	var exists C.bool
	if err := checkNative(func() int32 {
		return int32(C.mln_map_style_source_exists((*C.mln_map)(unsafe.Pointer(ptr)), sourceView.raw(), &exists))
	}); err != nil {
		return false, err
	}
	return bool(exists), nil
}

// StyleSourceType returns a source type and whether the source exists.
func (m *MapHandle) StyleSourceType(sourceID string) (StyleSourceType, bool, error) {
	ptr, err := m.ptr()
	if err != nil {
		return StyleSourceTypeUnknown, false, err
	}
	defer m.state.KeepAlive()
	sourceView := newCStringView(sourceID)
	defer sourceView.free()
	var sourceType C.uint32_t
	var found C.bool
	if err := checkNative(func() int32 {
		return int32(C.mln_map_get_style_source_type((*C.mln_map)(unsafe.Pointer(ptr)), sourceView.raw(), &sourceType, &found))
	}); err != nil {
		return StyleSourceTypeUnknown, false, err
	}
	return StyleSourceType(sourceType), bool(found), nil
}

// StyleSourceInfo returns source metadata and whether the source exists.
func (m *MapHandle) StyleSourceInfo(sourceID string) (StyleSourceInfo, bool, error) {
	ptr, err := m.ptr()
	if err != nil {
		return StyleSourceInfo{}, false, err
	}
	defer m.state.KeepAlive()
	sourceView := newCStringView(sourceID)
	defer sourceView.free()
	info := C.mln_style_source_info{size: C.uint32_t(unsafe.Sizeof(C.mln_style_source_info{}))}
	var found C.bool
	if err := checkNative(func() int32 {
		return int32(C.mln_map_get_style_source_info((*C.mln_map)(unsafe.Pointer(ptr)), sourceView.raw(), &info, &found))
	}); err != nil {
		return StyleSourceInfo{}, false, err
	}
	return styleSourceInfoFromC(info), bool(found), nil
}

// StyleSourceAttribution returns copied source attribution and whether the
// source exists.
func (m *MapHandle) StyleSourceAttribution(sourceID string) (string, bool, error) {
	info, found, err := m.StyleSourceInfo(sourceID)
	if err != nil || !found || !info.HasAttribution || info.AttributionSize == 0 {
		return "", found, err
	}
	ptr, err := m.ptr()
	if err != nil {
		return "", false, err
	}
	defer m.state.KeepAlive()
	sourceView := newCStringView(sourceID)
	defer sourceView.free()
	buffer := make([]byte, int(info.AttributionSize))
	var size C.size_t
	var rawFound C.bool
	if err := checkNative(func() int32 {
		return int32(C.mln_map_copy_style_source_attribution(
			(*C.mln_map)(unsafe.Pointer(ptr)),
			sourceView.raw(),
			(*C.char)(unsafe.Pointer(&buffer[0])),
			C.size_t(len(buffer)),
			&size,
			&rawFound,
		))
	}); err != nil {
		return "", false, err
	}
	return string(buffer[:int(size)]), bool(rawFound), nil
}

// StyleSourceIDs returns copied source IDs in style order.
func (m *MapHandle) StyleSourceIDs() ([]string, error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}
	defer m.state.KeepAlive()
	var list *C.mln_style_id_list
	if err := checkNative(func() int32 {
		return int32(C.mln_map_list_style_source_ids((*C.mln_map)(unsafe.Pointer(ptr)), &list))
	}); err != nil {
		return nil, err
	}
	return styleIDListStrings(list)
}

func styleIDListStrings(list *C.mln_style_id_list) ([]string, error) {
	defer C.mln_style_id_list_destroy(list)
	var count C.size_t
	if err := checkNative(func() int32 { return int32(C.mln_style_id_list_count(list, &count)) }); err != nil {
		return nil, err
	}
	ids := make([]string, int(count))
	for i := range ids {
		var view C.mln_string_view
		if err := checkNative(func() int32 { return int32(C.mln_style_id_list_get(list, C.size_t(i), &view)) }); err != nil {
			return nil, err
		}
		ids[i] = goStringView(view)
	}
	return ids, nil
}

// AddHillshadeLayer adds a hillshade layer for a raster DEM source. Passing an
// empty beforeLayerID appends the layer.
func (m *MapHandle) AddHillshadeLayer(layerID string, sourceID string, beforeLayerID string) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	layerView := newCStringView(layerID)
	defer layerView.free()
	sourceView := newCStringView(sourceID)
	defer sourceView.free()
	beforeView := newCStringView(beforeLayerID)
	defer beforeView.free()
	return checkNative(func() int32 {
		return int32(C.mln_map_add_hillshade_layer((*C.mln_map)(unsafe.Pointer(ptr)), layerView.raw(), sourceView.raw(), beforeView.raw()))
	})
}

// AddColorReliefLayer adds a color-relief layer for a raster DEM source.
// Passing an empty beforeLayerID appends the layer.
func (m *MapHandle) AddColorReliefLayer(layerID string, sourceID string, beforeLayerID string) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	layerView := newCStringView(layerID)
	defer layerView.free()
	sourceView := newCStringView(sourceID)
	defer sourceView.free()
	beforeView := newCStringView(beforeLayerID)
	defer beforeView.free()
	return checkNative(func() int32 {
		return int32(C.mln_map_add_color_relief_layer((*C.mln_map)(unsafe.Pointer(ptr)), layerView.raw(), sourceView.raw(), beforeView.raw()))
	})
}

// AddLocationIndicatorLayer adds a source-free location indicator layer. Passing
// an empty beforeLayerID appends the layer.
func (m *MapHandle) AddLocationIndicatorLayer(layerID string, beforeLayerID string) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	layerView := newCStringView(layerID)
	defer layerView.free()
	beforeView := newCStringView(beforeLayerID)
	defer beforeView.free()
	return checkNative(func() int32 {
		return int32(C.mln_map_add_location_indicator_layer((*C.mln_map)(unsafe.Pointer(ptr)), layerView.raw(), beforeView.raw()))
	})
}

// SetLocationIndicatorLocation sets a location indicator layer location.
func (m *MapHandle) SetLocationIndicatorLocation(layerID string, coordinate LatLng, altitude float64) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	layerView := newCStringView(layerID)
	defer layerView.free()
	return checkNative(func() int32 {
		return int32(C.mln_map_set_location_indicator_location((*C.mln_map)(unsafe.Pointer(ptr)), layerView.raw(), cLatLng(coordinate), C.double(altitude)))
	})
}

// SetLocationIndicatorBearing sets a location indicator layer bearing in degrees.
func (m *MapHandle) SetLocationIndicatorBearing(layerID string, bearing float64) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	layerView := newCStringView(layerID)
	defer layerView.free()
	return checkNative(func() int32 {
		return int32(C.mln_map_set_location_indicator_bearing((*C.mln_map)(unsafe.Pointer(ptr)), layerView.raw(), C.double(bearing)))
	})
}

// SetLocationIndicatorAccuracyRadius sets a location indicator layer accuracy radius.
func (m *MapHandle) SetLocationIndicatorAccuracyRadius(layerID string, radius float64) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	layerView := newCStringView(layerID)
	defer layerView.free()
	return checkNative(func() int32 {
		return int32(C.mln_map_set_location_indicator_accuracy_radius((*C.mln_map)(unsafe.Pointer(ptr)), layerView.raw(), C.double(radius)))
	})
}

// SetLocationIndicatorImageName sets one location indicator image-name property.
func (m *MapHandle) SetLocationIndicatorImageName(layerID string, imageKind LocationIndicatorImageKind, imageID string) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	layerView := newCStringView(layerID)
	defer layerView.free()
	imageView := newCStringView(imageID)
	defer imageView.free()
	return checkNative(func() int32 {
		return int32(C.mln_map_set_location_indicator_image_name((*C.mln_map)(unsafe.Pointer(ptr)), layerView.raw(), C.uint32_t(imageKind), imageView.raw()))
	})
}

// AddStyleLayerJSON adds one style layer from a style-spec layer JSON object.
// Passing an empty beforeLayerID appends the layer.
func (m *MapHandle) AddStyleLayerJSON(layerJSON any, beforeLayerID string) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	beforeView := newCStringView(beforeLayerID)
	defer beforeView.free()
	materializer := newCJSONMaterializer()
	defer materializer.free()
	rawJSON, err := materializer.value(layerJSON)
	if err != nil {
		return newBindingError(ErrInvalidArgument, err.Error())
	}
	return checkNative(func() int32 {
		return int32(C.mln_map_add_style_layer_json((*C.mln_map)(unsafe.Pointer(ptr)), &rawJSON, beforeView.raw()))
	})
}

// RemoveStyleLayer removes one style layer by ID and reports whether it was
// present.
func (m *MapHandle) RemoveStyleLayer(layerID string) (bool, error) {
	ptr, err := m.ptr()
	if err != nil {
		return false, err
	}
	defer m.state.KeepAlive()
	layerView := newCStringView(layerID)
	defer layerView.free()
	var removed C.bool
	if err := checkNative(func() int32 {
		return int32(C.mln_map_remove_style_layer((*C.mln_map)(unsafe.Pointer(ptr)), layerView.raw(), &removed))
	}); err != nil {
		return false, err
	}
	return bool(removed), nil
}

// StyleLayerExists reports whether one style layer ID exists.
func (m *MapHandle) StyleLayerExists(layerID string) (bool, error) {
	ptr, err := m.ptr()
	if err != nil {
		return false, err
	}
	defer m.state.KeepAlive()
	layerView := newCStringView(layerID)
	defer layerView.free()
	var exists C.bool
	if err := checkNative(func() int32 {
		return int32(C.mln_map_style_layer_exists((*C.mln_map)(unsafe.Pointer(ptr)), layerView.raw(), &exists))
	}); err != nil {
		return false, err
	}
	return bool(exists), nil
}

// StyleLayerType returns a layer type string and whether the layer exists.
func (m *MapHandle) StyleLayerType(layerID string) (string, bool, error) {
	ptr, err := m.ptr()
	if err != nil {
		return "", false, err
	}
	defer m.state.KeepAlive()
	layerView := newCStringView(layerID)
	defer layerView.free()
	var layerType C.mln_string_view
	var found C.bool
	if err := checkNative(func() int32 {
		return int32(C.mln_map_get_style_layer_type((*C.mln_map)(unsafe.Pointer(ptr)), layerView.raw(), &layerType, &found))
	}); err != nil {
		return "", false, err
	}
	return goStringView(layerType), bool(found), nil
}

// StyleLayerIDs returns copied layer IDs in style order.
func (m *MapHandle) StyleLayerIDs() ([]string, error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}
	defer m.state.KeepAlive()
	var list *C.mln_style_id_list
	if err := checkNative(func() int32 {
		return int32(C.mln_map_list_style_layer_ids((*C.mln_map)(unsafe.Pointer(ptr)), &list))
	}); err != nil {
		return nil, err
	}
	return styleIDListStrings(list)
}

// MoveStyleLayer moves one style layer before another layer. Passing an empty
// beforeLayerID moves layerID to the top of the style order.
func (m *MapHandle) MoveStyleLayer(layerID string, beforeLayerID string) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	layerView := newCStringView(layerID)
	defer layerView.free()
	beforeView := newCStringView(beforeLayerID)
	defer beforeView.free()
	return checkNative(func() int32 {
		return int32(C.mln_map_move_style_layer((*C.mln_map)(unsafe.Pointer(ptr)), layerView.raw(), beforeView.raw()))
	})
}

// StyleLayerJSON returns one copied style layer as a style-spec JSON object and
// whether the layer exists.
func (m *MapHandle) StyleLayerJSON(layerID string) (any, bool, error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, false, err
	}
	defer m.state.KeepAlive()
	layerView := newCStringView(layerID)
	defer layerView.free()
	var snapshot *C.mln_json_snapshot
	var found C.bool
	if err := checkNative(func() int32 {
		return int32(C.mln_map_get_style_layer_json((*C.mln_map)(unsafe.Pointer(ptr)), layerView.raw(), &snapshot, &found))
	}); err != nil {
		return nil, false, err
	}
	if !bool(found) {
		return nil, false, nil
	}
	value, err := cJSONSnapshotValue(snapshot)
	if err != nil {
		return nil, false, err
	}
	return value, true, nil
}

// SetStyleLightJSON sets the style light from a style-spec light JSON object.
func (m *MapHandle) SetStyleLightJSON(lightJSON any) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	materializer := newCJSONMaterializer()
	defer materializer.free()
	rawJSON, err := materializer.value(lightJSON)
	if err != nil {
		return newBindingError(ErrInvalidArgument, err.Error())
	}
	return checkNative(func() int32 {
		return int32(C.mln_map_set_style_light_json((*C.mln_map)(unsafe.Pointer(ptr)), &rawJSON))
	})
}

// SetStyleLightProperty sets one style light property.
func (m *MapHandle) SetStyleLightProperty(propertyName string, value any) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	propertyView := newCStringView(propertyName)
	defer propertyView.free()
	materializer := newCJSONMaterializer()
	defer materializer.free()
	rawValue, err := materializer.value(value)
	if err != nil {
		return newBindingError(ErrInvalidArgument, err.Error())
	}
	return checkNative(func() int32 {
		return int32(C.mln_map_set_style_light_property((*C.mln_map)(unsafe.Pointer(ptr)), propertyView.raw(), &rawValue))
	})
}

// StyleLightProperty returns one copied style light property as a style-spec
// JSON value.
func (m *MapHandle) StyleLightProperty(propertyName string) (any, error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}
	defer m.state.KeepAlive()
	propertyView := newCStringView(propertyName)
	defer propertyView.free()
	var snapshot *C.mln_json_snapshot
	if err := checkNative(func() int32 {
		return int32(C.mln_map_get_style_light_property((*C.mln_map)(unsafe.Pointer(ptr)), propertyView.raw(), &snapshot))
	}); err != nil {
		return nil, err
	}
	return cJSONSnapshotValue(snapshot)
}

// SetLayerProperty sets one style layer property.
func (m *MapHandle) SetLayerProperty(layerID string, propertyName string, value any) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	layerView := newCStringView(layerID)
	defer layerView.free()
	propertyView := newCStringView(propertyName)
	defer propertyView.free()
	materializer := newCJSONMaterializer()
	defer materializer.free()
	rawValue, err := materializer.value(value)
	if err != nil {
		return newBindingError(ErrInvalidArgument, err.Error())
	}
	return checkNative(func() int32 {
		return int32(C.mln_map_set_layer_property((*C.mln_map)(unsafe.Pointer(ptr)), layerView.raw(), propertyView.raw(), &rawValue))
	})
}

// LayerProperty returns one copied style layer property as a style-spec JSON
// value.
func (m *MapHandle) LayerProperty(layerID string, propertyName string) (any, error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}
	defer m.state.KeepAlive()
	layerView := newCStringView(layerID)
	defer layerView.free()
	propertyView := newCStringView(propertyName)
	defer propertyView.free()
	var snapshot *C.mln_json_snapshot
	if err := checkNative(func() int32 {
		return int32(C.mln_map_get_layer_property((*C.mln_map)(unsafe.Pointer(ptr)), layerView.raw(), propertyView.raw(), &snapshot))
	}); err != nil {
		return nil, err
	}
	return cJSONSnapshotValue(snapshot)
}

// SetLayerFilter sets or clears one style layer filter. Passing nil clears the
// filter.
func (m *MapHandle) SetLayerFilter(layerID string, filter any) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	layerView := newCStringView(layerID)
	defer layerView.free()
	var rawFilter *C.mln_json_value
	var materializer *cJSONMaterializer
	if filter != nil {
		materializer = newCJSONMaterializer()
		defer materializer.free()
		value, err := materializer.value(filter)
		if err != nil {
			return newBindingError(ErrInvalidArgument, err.Error())
		}
		rawFilter = &value
	}
	return checkNative(func() int32 {
		return int32(C.mln_map_set_layer_filter((*C.mln_map)(unsafe.Pointer(ptr)), layerView.raw(), rawFilter))
	})
}

// LayerFilter returns one copied style layer filter as a style-spec JSON value.
func (m *MapHandle) LayerFilter(layerID string) (any, error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}
	defer m.state.KeepAlive()
	layerView := newCStringView(layerID)
	defer layerView.free()
	var snapshot *C.mln_json_snapshot
	if err := checkNative(func() int32 {
		return int32(C.mln_map_get_layer_filter((*C.mln_map)(unsafe.Pointer(ptr)), layerView.raw(), &snapshot))
	}); err != nil {
		return nil, err
	}
	return cJSONSnapshotValue(snapshot)
}
