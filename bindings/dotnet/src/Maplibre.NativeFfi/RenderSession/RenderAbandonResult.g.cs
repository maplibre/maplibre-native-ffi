// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <param name="Disposition">
/// One <c>mln_render_abandon_disposition</c> value.
/// </param>
/// <param name="QuarantinedResourceCount">
/// Backend resource groups intentionally retained until process exit.
/// </param>
public readonly partial record struct RenderAbandonResult(
    RenderAbandonDisposition Disposition,
    uint QuarantinedResourceCount
);
