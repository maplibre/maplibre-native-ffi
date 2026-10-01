// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi.Runtime;

public readonly partial record struct RuntimeEventRenderFrame(
    RenderMode Mode,
    bool NeedsRepaint,
    bool PlacementChanged,
    RenderingStats Stats
);
