using System.Runtime.InteropServices;
using Maplibre.NativeFfi.Geo;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Memory;
using Maplibre.NativeFfi.Offline;

namespace Maplibre.NativeFfi.Internal.Struct;

internal sealed unsafe class NativeOfflineRegionDefinition : IDisposable
{
    private readonly NativeUtf8String styleUrl;
    private readonly NativeStringView? geometry;

    private NativeOfflineRegionDefinition(
        mln_offline_region_definition value,
        NativeUtf8String styleUrl,
        NativeStringView? geometry
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
        NativeStringView? geometry = null;
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
                    geometry = NativeStringView.From(region.Geometry, nameof(region.Geometry));
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
                                    geometry = geometry.Value,
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
            ValueStructs.CopyBufferView(value.geometry),
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
