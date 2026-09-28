using Maplibre.NativeFfi.Internal.C;

namespace Maplibre.NativeFfi.Tests;

/// <summary>
/// Handle values for tests that exercise binding-owned bookkeeping without a
/// live native object.
/// </summary>
/// <remarks>
/// Each value carries the kind byte the C API assigns to the type it stands in
/// for, so one reaching a diagnostic reads as an obviously fabricated handle of
/// the right kind. The C API rejects these as handles it never created.
/// </remarks>
internal static class SyntheticHandles
{
    private const ulong RuntimeKind = 0x01UL << 56;
    private const ulong MapKind = 0x02UL << 56;
    private const ulong ResourceRequestKind = 0x0CUL << 56;

    internal static MlnRuntime Runtime(ulong ordinal = 1) => new(RuntimeKind | ordinal);

    internal static MlnMap Map(ulong ordinal = 1) => new(MapKind | ordinal);

    internal static MlnResourceRequest ResourceRequest(ulong ordinal = 1) =>
        new(ResourceRequestKind | ordinal);
}
