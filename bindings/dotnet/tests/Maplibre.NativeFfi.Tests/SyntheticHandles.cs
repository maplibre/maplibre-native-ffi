using Maplibre.NativeFfi.Internal.C;

namespace Maplibre.NativeFfi.Tests;

/// <summary>
/// Handle values for tests that exercise binding-owned bookkeeping without a
/// live native object.
/// </summary>
/// <remarks>
/// Each value carries the kind byte the C API assigns to the type it stands in
/// for, so a synthetic handle that reaches a diagnostic reads as an obviously
/// fabricated handle of the right kind rather than a plausible one. Passing one
/// to the C API is rejected as a handle this process never created.
/// </remarks>
internal static class SyntheticHandles
{
    private const ulong RuntimeKind = 0x01UL << 56;
    private const ulong MapKind = 0x02UL << 56;
    private const ulong RenderSessionKind = 0x04UL << 56;
    private const ulong OfflineRegionListKind = 0x06UL << 56;
    private const ulong JsonSnapshotKind = 0x07UL << 56;
    private const ulong FeatureQueryResultKind = 0x09UL << 56;
    private const ulong ResourceRequestKind = 0x0CUL << 56;

    internal static MlnRuntime Runtime(ulong ordinal = 1) => new(RuntimeKind | ordinal);

    internal static MlnMap Map(ulong ordinal = 1) => new(MapKind | ordinal);

    internal static MlnRenderSession RenderSession(ulong ordinal = 1) =>
        new(RenderSessionKind | ordinal);

    internal static MlnOfflineRegionList OfflineRegionList(ulong ordinal = 1) =>
        new(OfflineRegionListKind | ordinal);

    internal static MlnJsonSnapshot JsonSnapshot(ulong ordinal = 1) =>
        new(JsonSnapshotKind | ordinal);

    internal static MlnFeatureQueryResult FeatureQueryResult(ulong ordinal = 1) =>
        new(FeatureQueryResultKind | ordinal);

    internal static MlnResourceRequest ResourceRequest(ulong ordinal = 1) =>
        new(ResourceRequestKind | ordinal);
}
