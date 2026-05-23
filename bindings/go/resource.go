package maplibre

import "github.com/maplibre/maplibre-native-ffi/bindings/go/internal/capi"

// ResourceKind identifies the native resource category for a request.
type ResourceKind uint32

const (
	ResourceKindUnknown     ResourceKind = ResourceKind(capi.ResourceKindUnknown)
	ResourceKindStyle       ResourceKind = ResourceKind(capi.ResourceKindStyle)
	ResourceKindSource      ResourceKind = ResourceKind(capi.ResourceKindSource)
	ResourceKindTile        ResourceKind = ResourceKind(capi.ResourceKindTile)
	ResourceKindGlyphs      ResourceKind = ResourceKind(capi.ResourceKindGlyphs)
	ResourceKindSpriteImage ResourceKind = ResourceKind(capi.ResourceKindSpriteImage)
	ResourceKindSpriteJSON  ResourceKind = ResourceKind(capi.ResourceKindSpriteJSON)
	ResourceKindImage       ResourceKind = ResourceKind(capi.ResourceKindImage)
)

// ResourceTransformRequest describes a URL transform request copied for Go.
type ResourceTransformRequest struct {
	Kind    ResourceKind
	RawKind uint32
	URL     string
}

// ResourceTransformCallback rewrites network resource URLs. Return
// replace=false or an empty URL to keep the original URL.
type ResourceTransformCallback func(ResourceTransformRequest) (replacementURL string, replace bool)
