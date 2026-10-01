// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi.Render;

public readonly partial record struct RenderFrameResult(
    RenderResult Disposition,
    ulong Token,
    ulong MapUpdateGeneration,
    ulong ExtentGeneration,
    ulong FrameGeneration,
    bool NeedsRepaint
);
