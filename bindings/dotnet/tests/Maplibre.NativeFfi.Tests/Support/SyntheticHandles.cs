using Maplibre.NativeFfi.Internal.C;

namespace Maplibre.NativeFfi.Tests;

/// <summary>
/// Handle values for tests that exercise binding-owned bookkeeping without a
/// live native object.
/// </summary>
/// <remarks>
/// Each value carries the kind byte the C API assigns to the type it stands in
/// for, so one reaching a diagnostic reads as an obviously fabricated handle of
/// the right kind. Generation zero is never issued by the C API, so these
/// cannot alias live handles, even when other tests create native objects.
/// </remarks>
internal static class SyntheticHandles
{
    private const ulong ResourceRequestKind = 0x0CUL << 56;

    internal static MlnResourceRequest ResourceRequest(ulong ordinal = 1) =>
        new(ResourceRequestKind | (ordinal << 36));
}
