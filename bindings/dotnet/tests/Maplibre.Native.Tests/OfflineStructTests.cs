using System.Runtime.InteropServices;
using Maplibre.Native.Geo;
using Maplibre.Native.Internal.C;
using Maplibre.Native.Internal.Memory;
using Maplibre.Native.Internal.Struct;
using Maplibre.Native.Offline;
using Xunit;

namespace Maplibre.Native.Tests;

public sealed unsafe class OfflineStructTests
{
    [BindingSpecTest("BND-060", "BND-061")]
    [Fact]
    public void OfflineRegionDefinitionsMaterializeNativeShape()
    {
        using var tile = NativeOfflineRegionDefinition.From(
            new OfflineRegionDefinition.TilePyramid(
                "maplibre://style",
                new LatLngBounds(new LatLng(1, 2), new LatLng(3, 4)),
                5,
                6,
                2,
                true
            )
        );

        Assert.Equal(
            (uint)mln_offline_region_definition_type.MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID,
            tile.Value.type
        );
        Assert.Equal(
            "maplibre://style",
            Marshal.PtrToStringUTF8((nint)tile.Value.data.tile_pyramid.style_url)
        );
        Assert.Equal(1, tile.Value.data.tile_pyramid.bounds.southwest.latitude);
        Assert.Equal(6, tile.Value.data.tile_pyramid.max_zoom);
        Assert.Equal(1, tile.Value.data.tile_pyramid.include_ideographs);

        using var geometry = NativeOfflineRegionDefinition.From(
            new OfflineRegionDefinition.GeometryRegion(
                "maplibre://geometry",
                new Geometry.Point(new LatLng(7, 8)),
                9,
                10,
                3,
                false
            )
        );

        Assert.Equal(
            (uint)mln_offline_region_definition_type.MLN_OFFLINE_REGION_DEFINITION_GEOMETRY,
            geometry.Value.type
        );
        Assert.Equal(
            "maplibre://geometry",
            Marshal.PtrToStringUTF8((nint)geometry.Value.data.geometry.style_url)
        );
        Assert.Equal(
            (uint)mln_geometry_type.MLN_GEOMETRY_TYPE_POINT,
            geometry.Value.data.geometry.geometry->type
        );
        Assert.Equal(8, geometry.Value.data.geometry.geometry->data.point.longitude);
        Assert.Equal(0, geometry.Value.data.geometry.include_ideographs);
    }

    [BindingSpecTest("BND-063")]
    [Fact]
    public void OfflineRegionInfoCopiesDefinitionAndMetadata()
    {
        using var styleUrl = NativeUtf8String.FromNullableString("maplibre://snapshot", "styleUrl");
        var metadata = stackalloc byte[] { 1, 2, 3 };
        var info = OfflineStructs.ReadInfo(
            new mln_offline_region_info
            {
                size = (uint)sizeof(mln_offline_region_info),
                id = 42,
                definition = new mln_offline_region_definition
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
                            bounds = MapStructs.ToNative(
                                new LatLngBounds(new LatLng(1, 2), new LatLng(3, 4))
                            ),
                            min_zoom = 5,
                            max_zoom = 6,
                            pixel_ratio = 2,
                            include_ideographs = 1,
                        },
                    },
                },
                metadata = metadata,
                metadata_size = 3,
            }
        );

        Assert.Equal(42, info.Id);
        Assert.Equal([1, 2, 3], info.Metadata);
        var definition = Assert.IsType<OfflineRegionDefinition.TilePyramid>(info.Definition);
        Assert.Equal("maplibre://snapshot", definition.StyleUrl);
        Assert.Equal(new LatLng(3, 4), definition.Bounds.Northeast);
    }

    [BindingSpecTest("BND-069")]
    [Fact]
    public void OfflineRegionInfoSnapshotsMetadataAndReturnsCopies()
    {
        var source = new byte[] { 1, 2, 3 };
        var info = new OfflineRegionInfo(
            42,
            new OfflineRegionDefinition.TilePyramid(
                "maplibre://snapshot",
                new LatLngBounds(new LatLng(1, 2), new LatLng(3, 4)),
                5,
                6,
                2,
                true
            ),
            source
        );
        source[0] = 9;

        var first = info.Metadata;
        Assert.Equal([1, 2, 3], first);
        first[0] = 8;
        Assert.Equal([1, 2, 3], info.Metadata);
    }

    // Support invariant for copied offline output: malformed native definition
    // discriminants fail deterministically instead of fabricating public values.
    [Fact]
    public void UnknownOfflineRegionDefinitionTypeThrows()
    {
        var error = Assert.Throws<InvalidOperationException>(() =>
            OfflineStructs.ReadDefinition(
                new mln_offline_region_definition
                {
                    size = (uint)sizeof(mln_offline_region_definition),
                    type = 999,
                }
            )
        );

        Assert.Contains(
            "mln_offline_region_definition_type",
            error.Message,
            StringComparison.Ordinal
        );
        Assert.Contains("999", error.Message, StringComparison.Ordinal);
    }

    [BindingSpecTest("BND-066")]
    [Fact]
    public void OfflineRegionListIsDestroyedWhenCopyingItemFails()
    {
        var destroyCalls = 0;
        using var methods = OfflineStructs.UseOfflineListMethodsForTest(
            (_, count) =>
            {
                *count = 1;
                return mln_status.MLN_STATUS_OK;
            },
            (_, _, info) =>
            {
                *info = new mln_offline_region_info
                {
                    size = (uint)sizeof(mln_offline_region_info),
                    definition = new mln_offline_region_definition
                    {
                        size = (uint)sizeof(mln_offline_region_definition),
                        type = 999,
                    },
                };
                return mln_status.MLN_STATUS_OK;
            },
            _ => destroyCalls++
        );

        Assert.Throws<InvalidOperationException>(() =>
            OfflineStructs.ReadList((mln_offline_region_list*)1234)
        );

        Assert.Equal(1, destroyCalls);
    }

    [BindingSpecTest("BND-060")]
    [Fact]
    public void OfflineRegionStatusCopiesNativeFields()
    {
        var status = OfflineStructs.ReadStatus(
            new mln_offline_region_status
            {
                download_state = (uint)
                    mln_offline_region_download_state.MLN_OFFLINE_REGION_DOWNLOAD_ACTIVE,
                completed_resource_count = 1,
                completed_resource_size = 2,
                completed_tile_count = 3,
                required_tile_count = 4,
                completed_tile_size = 5,
                required_resource_count = 6,
                required_resource_count_is_precise = 1,
                complete = 1,
            }
        );

        Assert.Equal(OfflineRegionDownloadState.Active, status.DownloadState);
        Assert.Equal(6u, status.RequiredResourceCount);
        Assert.True(status.RequiredResourceCountIsPrecise);
        Assert.True(status.Complete);
    }
}
