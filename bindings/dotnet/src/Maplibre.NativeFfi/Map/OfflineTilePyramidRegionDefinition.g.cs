// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using Maplibre.NativeFfi.Base;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Logging;
using Maplibre.NativeFfi.Query;
using Maplibre.NativeFfi.Render;
using Maplibre.NativeFfi.Runtime;
using Maplibre.NativeFfi.Style;

namespace Maplibre.NativeFfi.Map;

public readonly partial record struct OfflineTilePyramidRegionDefinition(
    string StyleUrl,
    LatLngBounds Bounds,
    double MinZoom,
    double MaxZoom,
    float PixelRatio,
    bool IncludeIdeographs
);
