package maplibre

import (
	"runtime"

	"github.com/maplibre/maplibre-native-ffi/bindings/go/internal/callback"
	"github.com/maplibre/maplibre-native-ffi/bindings/go/internal/capi"
)

// StyleSourceType identifies a native style source kind.
type StyleSourceType uint32

const (
	StyleSourceTypeUnknown      StyleSourceType = StyleSourceType(capi.StyleSourceTypeUnknown)
	StyleSourceTypeVector       StyleSourceType = StyleSourceType(capi.StyleSourceTypeVector)
	StyleSourceTypeRaster       StyleSourceType = StyleSourceType(capi.StyleSourceTypeRaster)
	StyleSourceTypeRasterDEM    StyleSourceType = StyleSourceType(capi.StyleSourceTypeRasterDEM)
	StyleSourceTypeGeoJSON      StyleSourceType = StyleSourceType(capi.StyleSourceTypeGeoJSON)
	StyleSourceTypeImage        StyleSourceType = StyleSourceType(capi.StyleSourceTypeImage)
	StyleSourceTypeVideo        StyleSourceType = StyleSourceType(capi.StyleSourceTypeVideo)
	StyleSourceTypeAnnotations  StyleSourceType = StyleSourceType(capi.StyleSourceTypeAnnotations)
	StyleSourceTypeCustomVector StyleSourceType = StyleSourceType(capi.StyleSourceTypeCustomVector)
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
	StyleTileSchemeXYZ StyleTileScheme = StyleTileScheme(capi.StyleTileSchemeXYZ)
	StyleTileSchemeTMS StyleTileScheme = StyleTileScheme(capi.StyleTileSchemeTMS)
)

// StyleVectorTileEncoding selects vector tile encoding.
type StyleVectorTileEncoding uint32

const (
	StyleVectorTileEncodingMVT StyleVectorTileEncoding = StyleVectorTileEncoding(capi.StyleVectorTileEncodingMVT)
	StyleVectorTileEncodingMLT StyleVectorTileEncoding = StyleVectorTileEncoding(capi.StyleVectorTileEncodingMLT)
)

// StyleRasterDEMEncoding selects raster DEM tile encoding.
type StyleRasterDEMEncoding uint32

const (
	StyleRasterDEMEncodingMapbox    StyleRasterDEMEncoding = StyleRasterDEMEncoding(capi.StyleRasterDEMEncodingMapbox)
	StyleRasterDEMEncodingTerrarium StyleRasterDEMEncoding = StyleRasterDEMEncoding(capi.StyleRasterDEMEncodingTerrarium)
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

func (options StyleTileSourceOptions) toCAPI() capi.StyleTileSourceOptions {
	var raw capi.StyleTileSourceOptions
	if options.MinZoom != nil {
		raw.Fields |= capi.StyleTileSourceOptionMinZoom
		raw.MinZoom = *options.MinZoom
	}
	if options.MaxZoom != nil {
		raw.Fields |= capi.StyleTileSourceOptionMaxZoom
		raw.MaxZoom = *options.MaxZoom
	}
	if options.Attribution != nil {
		raw.Fields |= capi.StyleTileSourceOptionAttribution
		raw.Attribution = *options.Attribution
	}
	if options.Scheme != nil {
		raw.Fields |= capi.StyleTileSourceOptionScheme
		raw.Scheme = uint32(*options.Scheme)
	}
	if options.Bounds != nil {
		raw.Fields |= capi.StyleTileSourceOptionBounds
		raw.Bounds = options.Bounds.toCAPI()
	}
	if options.TileSize != nil {
		raw.Fields |= capi.StyleTileSourceOptionTileSize
		raw.TileSize = *options.TileSize
	}
	if options.VectorEncoding != nil {
		raw.Fields |= capi.StyleTileSourceOptionVectorEncoding
		raw.VectorEncoding = uint32(*options.VectorEncoding)
	}
	if options.RasterEncoding != nil {
		raw.Fields |= capi.StyleTileSourceOptionRasterEncoding
		raw.RasterEncoding = uint32(*options.RasterEncoding)
	}
	return raw
}

func styleTileSourceOptionsToCAPI(options *StyleTileSourceOptions) *capi.StyleTileSourceOptions {
	if options == nil {
		return nil
	}
	raw := options.toCAPI()
	return &raw
}

// CustomGeometryTileCallback receives custom geometry tile requests.
type CustomGeometryTileCallback func(CanonicalTileID)

// CustomGeometrySourceOptions configures a custom geometry source.
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
		raw.Fields |= capi.CustomGeometrySourceOptionMinZoom
		raw.MinZoom = *options.MinZoom
	}
	if options.MaxZoom != nil {
		raw.Fields |= capi.CustomGeometrySourceOptionMaxZoom
		raw.MaxZoom = *options.MaxZoom
	}
	if options.Tolerance != nil {
		raw.Fields |= capi.CustomGeometrySourceOptionTolerance
		raw.Tolerance = *options.Tolerance
	}
	if options.TileSize != nil {
		raw.Fields |= capi.CustomGeometrySourceOptionTileSize
		raw.TileSize = *options.TileSize
	}
	if options.Buffer != nil {
		raw.Fields |= capi.CustomGeometrySourceOptionBuffer
		raw.Buffer = *options.Buffer
	}
	if options.Clip != nil {
		raw.Fields |= capi.CustomGeometrySourceOptionClip
		raw.Clip = *options.Clip
	}
	if options.Wrap != nil {
		raw.Fields |= capi.CustomGeometrySourceOptionWrap
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

func (options StyleImageOptions) toCAPI() capi.StyleImageOptions {
	var raw capi.StyleImageOptions
	if options.PixelRatio != nil {
		raw.Fields |= capi.StyleImageOptionPixelRatio
		raw.PixelRatio = *options.PixelRatio
	}
	if options.SDF != nil {
		raw.Fields |= capi.StyleImageOptionSDF
		raw.SDF = *options.SDF
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

func (image PremultipliedRGBA8Image) toCAPI() capi.PremultipliedRGBA8Image {
	return capi.PremultipliedRGBA8Image{Width: image.Width, Height: image.Height, Stride: image.Stride, Pixels: image.Pixels, ByteLength: image.ByteLength}
}

func styleImageInfoFromCAPI(info capi.StyleImageInfo) StyleImageInfo {
	return StyleImageInfo{Width: info.Width, Height: info.Height, Stride: info.Stride, ByteLength: info.ByteLength, PixelRatio: info.PixelRatio, SDF: info.SDF}
}

// LocationIndicatorImageKind identifies an image-name slot on a location indicator layer.
type LocationIndicatorImageKind uint32

const (
	LocationIndicatorImageKindTop     LocationIndicatorImageKind = LocationIndicatorImageKind(capi.LocationIndicatorImageKindTop)
	LocationIndicatorImageKindBearing LocationIndicatorImageKind = LocationIndicatorImageKind(capi.LocationIndicatorImageKindBearing)
	LocationIndicatorImageKindShadow  LocationIndicatorImageKind = LocationIndicatorImageKind(capi.LocationIndicatorImageKindShadow)
)

func styleSourceInfoFromCAPI(info capi.StyleSourceInfo) StyleSourceInfo {
	return StyleSourceInfo{
		Type:            StyleSourceType(info.Type),
		IDSize:          info.IDSize,
		IsVolatile:      info.IsVolatile,
		HasAttribution:  info.HasAttribution,
		AttributionSize: info.AttributionSize,
	}
}

// AddGeoJSONSourceURL adds a GeoJSON source that loads from a URL.
func (m *MapHandle) AddGeoJSONSourceURL(sourceID string, url string) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	return checkNative(func() capi.Status { return capi.MapAddGeoJSONSourceURL(ptr, sourceID, url) })
}

// SetGeoJSONSourceURL updates a GeoJSON source to load from a URL.
func (m *MapHandle) SetGeoJSONSourceURL(sourceID string, url string) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	return checkNative(func() capi.Status { return capi.MapSetGeoJSONSourceURL(ptr, sourceID, url) })
}

// AddGeoJSONSourceData adds a GeoJSON source with inline data. Accepted data is
// copied into MapLibre Native before the call returns.
func (m *MapHandle) AddGeoJSONSourceData(sourceID string, data GeoJSON) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	var materialErr error
	err = checkNative(func() capi.Status {
		var status capi.Status
		status, materialErr = capi.MapAddGeoJSONSourceData(ptr, sourceID, data.toCAPI())
		return status
	})
	if materialErr != nil {
		return newBindingError(ErrInvalidArgument, materialErr.Error())
	}
	return err
}

// SetGeoJSONSourceData updates a GeoJSON source with inline data. Accepted data
// is copied into MapLibre Native before the call returns.
func (m *MapHandle) SetGeoJSONSourceData(sourceID string, data GeoJSON) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	var materialErr error
	err = checkNative(func() capi.Status {
		var status capi.Status
		status, materialErr = capi.MapSetGeoJSONSourceData(ptr, sourceID, data.toCAPI())
		return status
	})
	if materialErr != nil {
		return newBindingError(ErrInvalidArgument, materialErr.Error())
	}
	return err
}

// AddCustomGeometrySource adds a custom geometry source to the current style.
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
	if err := checkNative(func() capi.Status {
		state, status := callback.AddCustomGeometrySource(ptr, sourceID, options.toCallback())
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
	var materialErr error
	err = checkNative(func() capi.Status {
		var status capi.Status
		status, materialErr = capi.MapSetCustomGeometrySourceTileData(ptr, sourceID, tileID.toCAPI(), data.toCAPI())
		return status
	})
	if materialErr != nil {
		return newBindingError(ErrInvalidArgument, materialErr.Error())
	}
	return err
}

// InvalidateCustomGeometrySourceTile invalidates custom geometry data for one tile.
func (m *MapHandle) InvalidateCustomGeometrySourceTile(sourceID string, tileID CanonicalTileID) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	return checkNative(func() capi.Status { return capi.MapInvalidateCustomGeometrySourceTile(ptr, sourceID, tileID.toCAPI()) })
}

// InvalidateCustomGeometrySourceRegion invalidates custom geometry data inside one geographic region.
func (m *MapHandle) InvalidateCustomGeometrySourceRegion(sourceID string, bounds LatLngBounds) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	return checkNative(func() capi.Status {
		return capi.MapInvalidateCustomGeometrySourceRegion(ptr, sourceID, bounds.toCAPI())
	})
}

// SetStyleImage sets or replaces one runtime style image.
func (m *MapHandle) SetStyleImage(imageID string, image PremultipliedRGBA8Image, options StyleImageOptions) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	return checkNative(func() capi.Status { return capi.MapSetStyleImage(ptr, imageID, image.toCAPI(), options.toCAPI()) })
}

// RemoveStyleImage removes one runtime style image and reports whether it existed.
func (m *MapHandle) RemoveStyleImage(imageID string) (bool, error) {
	ptr, err := m.ptr()
	if err != nil {
		return false, err
	}
	defer m.state.KeepAlive()
	var removed bool
	if err := checkNative(func() capi.Status { return capi.MapRemoveStyleImage(ptr, imageID, &removed) }); err != nil {
		return false, err
	}
	return removed, nil
}

// StyleImageExists reports whether one runtime style image exists.
func (m *MapHandle) StyleImageExists(imageID string) (bool, error) {
	ptr, err := m.ptr()
	if err != nil {
		return false, err
	}
	defer m.state.KeepAlive()
	var exists bool
	if err := checkNative(func() capi.Status { return capi.MapStyleImageExists(ptr, imageID, &exists) }); err != nil {
		return false, err
	}
	return exists, nil
}

// StyleImageInfo returns copied metadata for one runtime style image.
func (m *MapHandle) StyleImageInfo(imageID string) (StyleImageInfo, bool, error) {
	ptr, err := m.ptr()
	if err != nil {
		return StyleImageInfo{}, false, err
	}
	defer m.state.KeepAlive()
	var info capi.StyleImageInfo
	var found bool
	if err := checkNative(func() capi.Status { return capi.MapGetStyleImageInfo(ptr, imageID, &info, &found) }); err != nil {
		return StyleImageInfo{}, false, err
	}
	return styleImageInfoFromCAPI(info), found, nil
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
	var byteLength uint64
	var found bool
	if err := checkNative(func() capi.Status {
		return capi.MapCopyStyleImagePremultipliedRGBA8(ptr, imageID, buffer, &byteLength, &found)
	}); err != nil {
		runtime.KeepAlive(buffer)
		return byteLength, found, err
	}
	runtime.KeepAlive(buffer)
	return byteLength, found, nil
}

// AddImageSourceURL adds an image source that loads its image from a URL.
func (m *MapHandle) AddImageSourceURL(sourceID string, coordinates []LatLng, url string) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	return checkNative(func() capi.Status {
		return capi.MapAddImageSourceURL(ptr, sourceID, latLngSliceToCAPI(coordinates), url)
	})
}

// AddImageSourceImage adds an image source with inline image pixels.
func (m *MapHandle) AddImageSourceImage(sourceID string, coordinates []LatLng, image PremultipliedRGBA8Image) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	return checkNative(func() capi.Status {
		return capi.MapAddImageSourceImage(ptr, sourceID, latLngSliceToCAPI(coordinates), image.toCAPI())
	})
}

// SetImageSourceURL updates an image source to load its image from a URL.
func (m *MapHandle) SetImageSourceURL(sourceID string, url string) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	return checkNative(func() capi.Status { return capi.MapSetImageSourceURL(ptr, sourceID, url) })
}

// SetImageSourceImage updates an image source with inline image pixels.
func (m *MapHandle) SetImageSourceImage(sourceID string, image PremultipliedRGBA8Image) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	return checkNative(func() capi.Status { return capi.MapSetImageSourceImage(ptr, sourceID, image.toCAPI()) })
}

// SetImageSourceCoordinates updates image source coordinates.
func (m *MapHandle) SetImageSourceCoordinates(sourceID string, coordinates []LatLng) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	return checkNative(func() capi.Status {
		return capi.MapSetImageSourceCoordinates(ptr, sourceID, latLngSliceToCAPI(coordinates))
	})
}

// ImageSourceCoordinates returns copied image source coordinates.
func (m *MapHandle) ImageSourceCoordinates(sourceID string) ([]LatLng, bool, error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, false, err
	}
	defer m.state.KeepAlive()
	var coordinates []capi.LatLng
	var found bool
	if err := checkNative(func() capi.Status { return capi.MapGetImageSourceCoordinates(ptr, sourceID, &coordinates, &found) }); err != nil {
		return nil, false, err
	}
	return latLngSliceFromCAPI(coordinates), found, nil
}

// AddVectorSourceURL adds a vector source with a TileJSON URL.
func (m *MapHandle) AddVectorSourceURL(sourceID string, url string, options *StyleTileSourceOptions) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	return checkNative(func() capi.Status {
		return capi.MapAddVectorSourceURL(ptr, sourceID, url, styleTileSourceOptionsToCAPI(options))
	})
}

// AddVectorSourceTiles adds a vector source with inline tile URLs.
func (m *MapHandle) AddVectorSourceTiles(sourceID string, tiles []string, options *StyleTileSourceOptions) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	return checkNative(func() capi.Status {
		return capi.MapAddVectorSourceTiles(ptr, sourceID, tiles, styleTileSourceOptionsToCAPI(options))
	})
}

// AddRasterSourceURL adds a raster source with a TileJSON URL.
func (m *MapHandle) AddRasterSourceURL(sourceID string, url string, options *StyleTileSourceOptions) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	return checkNative(func() capi.Status {
		return capi.MapAddRasterSourceURL(ptr, sourceID, url, styleTileSourceOptionsToCAPI(options))
	})
}

// AddRasterSourceTiles adds a raster source with inline tile URLs.
func (m *MapHandle) AddRasterSourceTiles(sourceID string, tiles []string, options *StyleTileSourceOptions) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	return checkNative(func() capi.Status {
		return capi.MapAddRasterSourceTiles(ptr, sourceID, tiles, styleTileSourceOptionsToCAPI(options))
	})
}

// AddRasterDEMSourceURL adds a raster DEM source with a TileJSON URL.
func (m *MapHandle) AddRasterDEMSourceURL(sourceID string, url string, options *StyleTileSourceOptions) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	return checkNative(func() capi.Status {
		return capi.MapAddRasterDEMSourceURL(ptr, sourceID, url, styleTileSourceOptionsToCAPI(options))
	})
}

// AddRasterDEMSourceTiles adds a raster DEM source with inline tile URLs.
func (m *MapHandle) AddRasterDEMSourceTiles(sourceID string, tiles []string, options *StyleTileSourceOptions) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	return checkNative(func() capi.Status {
		return capi.MapAddRasterDEMSourceTiles(ptr, sourceID, tiles, styleTileSourceOptionsToCAPI(options))
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
	var materialErr error
	err = checkNative(func() capi.Status {
		var status capi.Status
		status, materialErr = capi.MapAddStyleSourceJSON(ptr, sourceID, sourceJSON)
		return status
	})
	if materialErr != nil {
		return newBindingError(ErrInvalidArgument, materialErr.Error())
	}
	return err
}

// RemoveStyleSource removes one style source by ID and reports whether it was
// present.
func (m *MapHandle) RemoveStyleSource(sourceID string) (bool, error) {
	ptr, err := m.ptr()
	if err != nil {
		return false, err
	}
	defer m.state.KeepAlive()
	var removed bool
	if err := checkNative(func() capi.Status { return capi.MapRemoveStyleSource(ptr, sourceID, &removed) }); err != nil {
		return false, err
	}
	if removed {
		m.releaseCustomGeometrySource(sourceID)
	}
	return removed, nil
}

// StyleSourceExists reports whether one style source ID exists.
func (m *MapHandle) StyleSourceExists(sourceID string) (bool, error) {
	ptr, err := m.ptr()
	if err != nil {
		return false, err
	}
	defer m.state.KeepAlive()
	var exists bool
	if err := checkNative(func() capi.Status { return capi.MapStyleSourceExists(ptr, sourceID, &exists) }); err != nil {
		return false, err
	}
	return exists, nil
}

// StyleSourceType returns a source type and whether the source exists.
func (m *MapHandle) StyleSourceType(sourceID string) (StyleSourceType, bool, error) {
	ptr, err := m.ptr()
	if err != nil {
		return StyleSourceTypeUnknown, false, err
	}
	defer m.state.KeepAlive()
	var sourceType uint32
	var found bool
	if err := checkNative(func() capi.Status { return capi.MapGetStyleSourceType(ptr, sourceID, &sourceType, &found) }); err != nil {
		return StyleSourceTypeUnknown, false, err
	}
	return StyleSourceType(sourceType), found, nil
}

// StyleSourceInfo returns source metadata and whether the source exists.
func (m *MapHandle) StyleSourceInfo(sourceID string) (StyleSourceInfo, bool, error) {
	ptr, err := m.ptr()
	if err != nil {
		return StyleSourceInfo{}, false, err
	}
	defer m.state.KeepAlive()
	var info capi.StyleSourceInfo
	var found bool
	if err := checkNative(func() capi.Status { return capi.MapGetStyleSourceInfo(ptr, sourceID, &info, &found) }); err != nil {
		return StyleSourceInfo{}, false, err
	}
	return styleSourceInfoFromCAPI(info), found, nil
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
	var attribution string
	if err := checkNative(func() capi.Status {
		return capi.MapCopyStyleSourceAttribution(ptr, sourceID, int(info.AttributionSize), &attribution, &found)
	}); err != nil {
		return "", false, err
	}
	return attribution, found, nil
}

// StyleSourceIDs returns copied source IDs in style order.
func (m *MapHandle) StyleSourceIDs() ([]string, error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}
	defer m.state.KeepAlive()
	var ids []string
	if err := checkNative(func() capi.Status { return capi.MapListStyleSourceIDs(ptr, &ids) }); err != nil {
		return nil, err
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
	return checkNative(func() capi.Status { return capi.MapAddHillshadeLayer(ptr, layerID, sourceID, beforeLayerID) })
}

// AddColorReliefLayer adds a color-relief layer for a raster DEM source.
// Passing an empty beforeLayerID appends the layer.
func (m *MapHandle) AddColorReliefLayer(layerID string, sourceID string, beforeLayerID string) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	return checkNative(func() capi.Status { return capi.MapAddColorReliefLayer(ptr, layerID, sourceID, beforeLayerID) })
}

// AddLocationIndicatorLayer adds a source-free location indicator layer. Passing
// an empty beforeLayerID appends the layer.
func (m *MapHandle) AddLocationIndicatorLayer(layerID string, beforeLayerID string) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	return checkNative(func() capi.Status { return capi.MapAddLocationIndicatorLayer(ptr, layerID, beforeLayerID) })
}

// SetLocationIndicatorLocation sets a location indicator layer location.
func (m *MapHandle) SetLocationIndicatorLocation(layerID string, coordinate LatLng, altitude float64) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	return checkNative(func() capi.Status {
		return capi.MapSetLocationIndicatorLocation(ptr, layerID, coordinate.toCAPI(), altitude)
	})
}

// SetLocationIndicatorBearing sets a location indicator layer bearing in degrees.
func (m *MapHandle) SetLocationIndicatorBearing(layerID string, bearing float64) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	return checkNative(func() capi.Status { return capi.MapSetLocationIndicatorBearing(ptr, layerID, bearing) })
}

// SetLocationIndicatorAccuracyRadius sets a location indicator layer accuracy radius.
func (m *MapHandle) SetLocationIndicatorAccuracyRadius(layerID string, radius float64) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	return checkNative(func() capi.Status { return capi.MapSetLocationIndicatorAccuracyRadius(ptr, layerID, radius) })
}

// SetLocationIndicatorImageName sets one location indicator image-name property.
func (m *MapHandle) SetLocationIndicatorImageName(layerID string, imageKind LocationIndicatorImageKind, imageID string) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	return checkNative(func() capi.Status {
		return capi.MapSetLocationIndicatorImageName(ptr, layerID, uint32(imageKind), imageID)
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
	var materialErr error
	err = checkNative(func() capi.Status {
		var status capi.Status
		status, materialErr = capi.MapAddStyleLayerJSON(ptr, layerJSON, beforeLayerID)
		return status
	})
	if materialErr != nil {
		return newBindingError(ErrInvalidArgument, materialErr.Error())
	}
	return err
}

// RemoveStyleLayer removes one style layer by ID and reports whether it was
// present.
func (m *MapHandle) RemoveStyleLayer(layerID string) (bool, error) {
	ptr, err := m.ptr()
	if err != nil {
		return false, err
	}
	defer m.state.KeepAlive()
	var removed bool
	if err := checkNative(func() capi.Status { return capi.MapRemoveStyleLayer(ptr, layerID, &removed) }); err != nil {
		return false, err
	}
	return removed, nil
}

// StyleLayerExists reports whether one style layer ID exists.
func (m *MapHandle) StyleLayerExists(layerID string) (bool, error) {
	ptr, err := m.ptr()
	if err != nil {
		return false, err
	}
	defer m.state.KeepAlive()
	var exists bool
	if err := checkNative(func() capi.Status { return capi.MapStyleLayerExists(ptr, layerID, &exists) }); err != nil {
		return false, err
	}
	return exists, nil
}

// StyleLayerType returns a layer type string and whether the layer exists.
func (m *MapHandle) StyleLayerType(layerID string) (string, bool, error) {
	ptr, err := m.ptr()
	if err != nil {
		return "", false, err
	}
	defer m.state.KeepAlive()
	var layerType string
	var found bool
	if err := checkNative(func() capi.Status { return capi.MapGetStyleLayerType(ptr, layerID, &layerType, &found) }); err != nil {
		return "", false, err
	}
	return layerType, found, nil
}

// StyleLayerIDs returns copied layer IDs in style order.
func (m *MapHandle) StyleLayerIDs() ([]string, error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}
	defer m.state.KeepAlive()
	var ids []string
	if err := checkNative(func() capi.Status { return capi.MapListStyleLayerIDs(ptr, &ids) }); err != nil {
		return nil, err
	}
	return ids, nil
}

// MoveStyleLayer moves one style layer before another layer. Passing an empty
// beforeLayerID moves layerID to the top of the style order.
func (m *MapHandle) MoveStyleLayer(layerID string, beforeLayerID string) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	return checkNative(func() capi.Status { return capi.MapMoveStyleLayer(ptr, layerID, beforeLayerID) })
}

// StyleLayerJSON returns one copied style layer as a style-spec JSON object and
// whether the layer exists.
func (m *MapHandle) StyleLayerJSON(layerID string) (any, bool, error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, false, err
	}
	defer m.state.KeepAlive()
	var value any
	var found bool
	if err := checkNative(func() capi.Status { return capi.MapGetStyleLayerJSON(ptr, layerID, &value, &found) }); err != nil {
		return nil, false, err
	}
	return value, found, nil
}

// SetStyleLightJSON sets the style light from a style-spec light JSON object.
func (m *MapHandle) SetStyleLightJSON(lightJSON any) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	var materialErr error
	err = checkNative(func() capi.Status {
		var status capi.Status
		status, materialErr = capi.MapSetStyleLightJSON(ptr, lightJSON)
		return status
	})
	if materialErr != nil {
		return newBindingError(ErrInvalidArgument, materialErr.Error())
	}
	return err
}

// SetStyleLightProperty sets one style light property.
func (m *MapHandle) SetStyleLightProperty(propertyName string, value any) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	var materialErr error
	err = checkNative(func() capi.Status {
		var status capi.Status
		status, materialErr = capi.MapSetStyleLightProperty(ptr, propertyName, value)
		return status
	})
	if materialErr != nil {
		return newBindingError(ErrInvalidArgument, materialErr.Error())
	}
	return err
}

// StyleLightProperty returns one copied style light property as a style-spec
// JSON value.
func (m *MapHandle) StyleLightProperty(propertyName string) (any, error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}
	defer m.state.KeepAlive()
	var value any
	if err := checkNative(func() capi.Status { return capi.MapGetStyleLightProperty(ptr, propertyName, &value) }); err != nil {
		return nil, err
	}
	return value, nil
}

// SetLayerProperty sets one style layer property.
func (m *MapHandle) SetLayerProperty(layerID string, propertyName string, value any) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	var materialErr error
	err = checkNative(func() capi.Status {
		var status capi.Status
		status, materialErr = capi.MapSetLayerProperty(ptr, layerID, propertyName, value)
		return status
	})
	if materialErr != nil {
		return newBindingError(ErrInvalidArgument, materialErr.Error())
	}
	return err
}

// LayerProperty returns one copied style layer property as a style-spec JSON
// value.
func (m *MapHandle) LayerProperty(layerID string, propertyName string) (any, error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}
	defer m.state.KeepAlive()
	var value any
	if err := checkNative(func() capi.Status { return capi.MapGetLayerProperty(ptr, layerID, propertyName, &value) }); err != nil {
		return nil, err
	}
	return value, nil
}

// SetLayerFilter sets or clears one style layer filter. Passing nil clears the
// filter.
func (m *MapHandle) SetLayerFilter(layerID string, filter any) error {
	ptr, err := m.ptr()
	if err != nil {
		return err
	}
	defer m.state.KeepAlive()
	var materialErr error
	err = checkNative(func() capi.Status {
		var status capi.Status
		status, materialErr = capi.MapSetLayerFilter(ptr, layerID, filter)
		return status
	})
	if materialErr != nil {
		return newBindingError(ErrInvalidArgument, materialErr.Error())
	}
	return err
}

// LayerFilter returns one copied style layer filter as a style-spec JSON value.
func (m *MapHandle) LayerFilter(layerID string) (any, error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}
	defer m.state.KeepAlive()
	var value any
	if err := checkNative(func() capi.Status { return capi.MapGetLayerFilter(ptr, layerID, &value) }); err != nil {
		return nil, err
	}
	return value, nil
}
