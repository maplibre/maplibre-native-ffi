package maplibre

import "github.com/maplibre/maplibre-native-ffi/bindings/go/internal/capi"

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

func styleSourceInfoFromCAPI(info capi.StyleSourceInfo) StyleSourceInfo {
	return StyleSourceInfo{
		Type:            StyleSourceType(info.Type),
		IDSize:          info.IDSize,
		IsVolatile:      info.IsVolatile,
		HasAttribution:  info.HasAttribution,
		AttributionSize: info.AttributionSize,
	}
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
