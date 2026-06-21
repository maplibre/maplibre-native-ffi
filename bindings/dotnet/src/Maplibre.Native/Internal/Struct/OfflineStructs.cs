using System.Runtime.InteropServices;
using Maplibre.Native.Geo;
using Maplibre.Native.Internal.C;
using Maplibre.Native.Internal.Memory;
using Maplibre.Native.Internal.Status;
using Maplibre.Native.Offline;

namespace Maplibre.Native.Internal.Struct;

internal unsafe delegate mln_status OfflineRegionSnapshotGet(
    mln_offline_region_snapshot* snapshot,
    mln_offline_region_info* outInfo
);

internal unsafe delegate void OfflineRegionSnapshotDestroy(mln_offline_region_snapshot* snapshot);

internal unsafe delegate mln_status OfflineRegionListCount(
    mln_offline_region_list* list,
    nuint* outCount
);

internal unsafe delegate mln_status OfflineRegionListGet(
    mln_offline_region_list* list,
    nuint index,
    mln_offline_region_info* outInfo
);

internal unsafe delegate void OfflineRegionListDestroy(mln_offline_region_list* list);

internal sealed unsafe class NativeOfflineRegionDefinition : IDisposable
{
    private readonly NativeUtf8String styleUrl;
    private readonly NativeGeometry? geometry;

    private NativeOfflineRegionDefinition(
        mln_offline_region_definition value,
        NativeUtf8String styleUrl,
        NativeGeometry? geometry
    )
    {
        Value = value;
        this.styleUrl = styleUrl;
        this.geometry = geometry;
    }

    internal mln_offline_region_definition Value { get; }

    internal static NativeOfflineRegionDefinition From(OfflineRegionDefinition definition)
    {
        ArgumentNullException.ThrowIfNull(definition);
        NativeUtf8String? styleUrl = null;
        NativeGeometry? geometry = null;
        try
        {
            switch (definition)
            {
                case OfflineRegionDefinition.TilePyramid tile:
                    styleUrl = NativeUtf8String.FromNullableString(
                        tile.StyleUrl,
                        nameof(tile.StyleUrl)
                    );
                    return new NativeOfflineRegionDefinition(
                        new mln_offline_region_definition
                        {
                            size = (uint)sizeof(mln_offline_region_definition),
                            type = (uint)
                                mln_offline_region_definition_type.MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID,
                            data =
                            {
                                tile_pyramid = new mln_offline_tile_pyramid_region_definition
                                {
                                    size = (uint)sizeof(mln_offline_tile_pyramid_region_definition),
                                    style_url = styleUrl.Pointer,
                                    bounds = MapStructs.ToNative(tile.Bounds),
                                    min_zoom = tile.MinimumZoom,
                                    max_zoom = tile.MaximumZoom,
                                    pixel_ratio = tile.PixelRatio,
                                    include_ideographs = tile.IncludeIdeographs ? (byte)1 : (byte)0,
                                },
                            },
                        },
                        styleUrl,
                        null
                    );
                case OfflineRegionDefinition.GeometryRegion region:
                    styleUrl = NativeUtf8String.FromNullableString(
                        region.StyleUrl,
                        nameof(region.StyleUrl)
                    );
                    geometry = NativeGeometry.From(region.Geometry);
                    return new NativeOfflineRegionDefinition(
                        new mln_offline_region_definition
                        {
                            size = (uint)sizeof(mln_offline_region_definition),
                            type = (uint)
                                mln_offline_region_definition_type.MLN_OFFLINE_REGION_DEFINITION_GEOMETRY,
                            data =
                            {
                                geometry = new mln_offline_geometry_region_definition
                                {
                                    size = (uint)sizeof(mln_offline_geometry_region_definition),
                                    style_url = styleUrl.Pointer,
                                    geometry = geometry.Pointer,
                                    min_zoom = region.MinimumZoom,
                                    max_zoom = region.MaximumZoom,
                                    pixel_ratio = region.PixelRatio,
                                    include_ideographs = region.IncludeIdeographs
                                        ? (byte)1
                                        : (byte)0,
                                },
                            },
                        },
                        styleUrl,
                        geometry
                    );
                default:
                    throw new ArgumentException(
                        $"Unsupported offline region definition type {definition.GetType().Name}.",
                        nameof(definition)
                    );
            }
        }
        catch
        {
            styleUrl?.Dispose();
            geometry?.Dispose();
            throw;
        }
    }

    public void Dispose()
    {
        geometry?.Dispose();
        styleUrl.Dispose();
    }
}

internal static unsafe class OfflineStructs
{
    private static readonly OfflineRegionSnapshotGet DefaultSnapshotGet = static (
        snapshot,
        outInfo
    ) => NativeMethods.mln_offline_region_snapshot_get(snapshot, outInfo);
    private static readonly OfflineRegionSnapshotDestroy DefaultSnapshotDestroy = static snapshot =>
        NativeMethods.mln_offline_region_snapshot_destroy(snapshot);
    private static readonly OfflineRegionListCount DefaultListCount = static (list, outCount) =>
        NativeMethods.mln_offline_region_list_count(list, outCount);
    private static readonly OfflineRegionListGet DefaultListGet = static (list, index, outInfo) =>
        NativeMethods.mln_offline_region_list_get(list, index, outInfo);
    private static readonly OfflineRegionListDestroy DefaultListDestroy = static list =>
        NativeMethods.mln_offline_region_list_destroy(list);

    [ThreadStatic]
    private static OfflineRegionSnapshotGet? snapshotGetForTest;

    [ThreadStatic]
    private static OfflineRegionSnapshotDestroy? snapshotDestroyForTest;

    [ThreadStatic]
    private static OfflineRegionListCount? listCountForTest;

    [ThreadStatic]
    private static OfflineRegionListGet? listGetForTest;

    [ThreadStatic]
    private static OfflineRegionListDestroy? listDestroyForTest;

    internal static OfflineRegionInfo ReadSnapshot(mln_offline_region_snapshot* snapshot)
    {
        if (snapshot is null)
        {
            throw new InvalidOperationException("Native offline region snapshot was null.");
        }

        try
        {
            var info = new mln_offline_region_info { size = (uint)sizeof(mln_offline_region_info) };
            NativeStatus.Check(SnapshotGet(snapshot, &info));
            return ReadInfo(info);
        }
        finally
        {
            SnapshotDestroy(snapshot);
        }
    }

    internal static IReadOnlyList<OfflineRegionInfo> ReadList(mln_offline_region_list* list)
    {
        if (list is null)
        {
            return [];
        }

        try
        {
            nuint count = 0;
            NativeStatus.Check(ListCount(list, &count));
            var regions = new OfflineRegionInfo[checked((int)count)];
            for (nuint index = 0; index < count; index++)
            {
                var info = new mln_offline_region_info
                {
                    size = (uint)sizeof(mln_offline_region_info),
                };
                NativeStatus.Check(ListGet(list, index, &info));
                regions[(int)index] = ReadInfo(info);
            }
            return regions;
        }
        finally
        {
            ListDestroy(list);
        }
    }

    internal static IDisposable UseOfflineSnapshotMethodsForTest(
        OfflineRegionSnapshotGet get,
        OfflineRegionSnapshotDestroy destroy
    )
    {
        var previousGet = snapshotGetForTest;
        var previousDestroy = snapshotDestroyForTest;
        snapshotGetForTest = get;
        snapshotDestroyForTest = destroy;
        return new RestoreOfflineSnapshotMethods(previousGet, previousDestroy);
    }

    internal static IDisposable UseOfflineListMethodsForTest(
        OfflineRegionListCount count,
        OfflineRegionListGet get,
        OfflineRegionListDestroy destroy
    )
    {
        var previousCount = listCountForTest;
        var previousGet = listGetForTest;
        var previousDestroy = listDestroyForTest;
        listCountForTest = count;
        listGetForTest = get;
        listDestroyForTest = destroy;
        return new RestoreOfflineListMethods(previousCount, previousGet, previousDestroy);
    }

    private static OfflineRegionSnapshotGet SnapshotGet => snapshotGetForTest ?? DefaultSnapshotGet;

    private static OfflineRegionSnapshotDestroy SnapshotDestroy =>
        snapshotDestroyForTest ?? DefaultSnapshotDestroy;

    private static OfflineRegionListCount ListCount => listCountForTest ?? DefaultListCount;

    private static OfflineRegionListGet ListGet => listGetForTest ?? DefaultListGet;

    private static OfflineRegionListDestroy ListDestroy => listDestroyForTest ?? DefaultListDestroy;

    private sealed class RestoreOfflineSnapshotMethods(
        OfflineRegionSnapshotGet? previousGet,
        OfflineRegionSnapshotDestroy? previousDestroy
    ) : IDisposable
    {
        public void Dispose()
        {
            snapshotGetForTest = previousGet;
            snapshotDestroyForTest = previousDestroy;
        }
    }

    private sealed class RestoreOfflineListMethods(
        OfflineRegionListCount? previousCount,
        OfflineRegionListGet? previousGet,
        OfflineRegionListDestroy? previousDestroy
    ) : IDisposable
    {
        public void Dispose()
        {
            listCountForTest = previousCount;
            listGetForTest = previousGet;
            listDestroyForTest = previousDestroy;
        }
    }

    internal static OfflineRegionInfo ReadInfo(mln_offline_region_info info) =>
        new(info.id, ReadDefinition(info.definition), CopyBytes(info.metadata, info.metadata_size));

    internal static OfflineRegionDefinition ReadDefinition(mln_offline_region_definition definition)
    {
        return (mln_offline_region_definition_type)definition.type switch
        {
            mln_offline_region_definition_type.MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID =>
                ReadTilePyramid(definition.data.tile_pyramid),
            mln_offline_region_definition_type.MLN_OFFLINE_REGION_DEFINITION_GEOMETRY =>
                ReadGeometryRegion(definition.data.geometry),
            _ => throw new InvalidOperationException(
                $"ReadDefinition received unknown mln_offline_region_definition_type value {definition.type}."
            ),
        };
    }

    internal static OfflineRegionStatus ReadStatus(mln_offline_region_status value) =>
        new(
            (OfflineRegionDownloadState)value.download_state,
            value.completed_resource_count,
            value.completed_resource_size,
            value.completed_tile_count,
            value.required_tile_count,
            value.completed_tile_size,
            value.required_resource_count,
            value.required_resource_count_is_precise != 0,
            value.complete != 0
        );

    private static OfflineRegionDefinition.TilePyramid ReadTilePyramid(
        mln_offline_tile_pyramid_region_definition value
    ) =>
        new(
            CopyCString(value.style_url),
            MapStructs.FromNative(value.bounds),
            value.min_zoom,
            value.max_zoom,
            value.pixel_ratio,
            value.include_ideographs != 0
        );

    private static OfflineRegionDefinition.GeometryRegion ReadGeometryRegion(
        mln_offline_geometry_region_definition value
    ) =>
        new(
            CopyCString(value.style_url),
            value.geometry is null
                ? Geometry.Empty.Instance
                : QueryStructs.ReadGeometry(*value.geometry),
            value.min_zoom,
            value.max_zoom,
            value.pixel_ratio,
            value.include_ideographs != 0
        );

    private static byte[] CopyBytes(byte* pointer, nuint byteLength)
    {
        if (pointer is null || byteLength == 0)
        {
            return [];
        }
        var bytes = new byte[checked((int)byteLength)];
        Marshal.Copy((nint)pointer, bytes, 0, bytes.Length);
        return bytes;
    }

    private static string CopyCString(sbyte* pointer) =>
        pointer is null ? string.Empty : Marshal.PtrToStringUTF8((nint)pointer) ?? string.Empty;
}
