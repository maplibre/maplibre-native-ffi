using System.Runtime.InteropServices;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Memory;
using Maplibre.NativeFfi.Internal.Struct;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Runtime;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed unsafe class OfflineStructTests
{
    [BindingSpecTest("", "")]
    [Fact]
    public void OfflineRegionDefinitionsMaterializeNativeShape()
    {
        using var scope = new NativeCallScope();
        var tile = GeneratedValues.NativeOfflineRegionDefinition(
            new OfflineRegionDefinition.TilePyramid(
                new OfflineTilePyramidRegionDefinition(
                    "maplibre://style",
                    new LatLngBounds(new LatLng(1, 2), new LatLng(3, 4)),
                    5,
                    6,
                    2,
                    true
                )
            ),
            scope
        );

        Assert.Equal(
            (uint)mln_offline_region_definition_type.MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID,
            tile.type
        );
        Assert.Equal(
            "maplibre://style",
            Marshal.PtrToStringUTF8((nint)tile.data.tile_pyramid.style_url)
        );
        Assert.Equal(1, tile.data.tile_pyramid.bounds.southwest.latitude);
        Assert.Equal(6, tile.data.tile_pyramid.max_zoom);
        Assert.Equal(1, tile.data.tile_pyramid.include_ideographs);

        var geometry = GeneratedValues.NativeOfflineRegionDefinition(
            new OfflineRegionDefinition.Geometry(
                new OfflineGeometryRegionDefinition(
                    "maplibre://geometry",
                    """{"type":"Point","coordinates":[8,7]}"""u8.ToArray(),
                    9,
                    10,
                    3,
                    false
                )
            ),
            scope
        );

        Assert.Equal(
            (uint)mln_offline_region_definition_type.MLN_OFFLINE_REGION_DEFINITION_GEOMETRY,
            geometry.type
        );
        Assert.Equal(
            "maplibre://geometry",
            Marshal.PtrToStringUTF8((nint)geometry.data.geometry.style_url)
        );
        Assert.Equal(
            """{"type":"Point","coordinates":[8,7]}""",
            RuntimeStructs.CopyUtf8(
                geometry.data.geometry.geometry.data,
                geometry.data.geometry.geometry.size
            )
        );
        Assert.Equal(0, geometry.data.geometry.include_ideographs);
    }

    [BindingSpecTest("")]
    [Fact]
    public void OfflineRegionInfoCopiesDefinitionAndMetadata()
    {
        using var styleUrl = NativeUtf8String.FromNullableString("maplibre://snapshot", "styleUrl");
        var metadata = stackalloc byte[] { 1, 2, 3 };
        var info = GeneratedValues.CopyOfflineRegionInfo(
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
                            bounds = GeneratedValues.NativeLatLngBounds(
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
        Assert.Equal("maplibre://snapshot", definition.Value.StyleUrl);
        Assert.Equal(new LatLng(3, 4), definition.Value.Bounds.Northeast);
    }

    [BindingSpecTest("")]
    [Fact]
    public void OfflineRegionInfoSnapshotsMetadataAndReturnsCopies()
    {
        var source = new byte[] { 1, 2, 3 };
        var info = new OfflineRegionInfo(
            42,
            new OfflineRegionDefinition.TilePyramid(
                new OfflineTilePyramidRegionDefinition(
                    "maplibre://snapshot",
                    new LatLngBounds(new LatLng(1, 2), new LatLng(3, 4)),
                    5,
                    6,
                    2,
                    true
                )
            ),
            source
        );
        source[0] = 9;

        var first = info.Metadata;
        Assert.Equal([1, 2, 3], first);
        first[0] = 8;
        Assert.Equal([1, 2, 3], info.Metadata);
    }

    [Fact]
    public void UnknownOfflineRegionDefinitionRetainsItsTag()
    {
        var copied = GeneratedValues.CopyOfflineRegionDefinition(
            new mln_offline_region_definition
            {
                size = (uint)sizeof(mln_offline_region_definition),
                type = 999,
            }
        );
        var unknown = Assert.IsType<OfflineRegionDefinition.Unknown>(copied);
        Assert.Equal(999u, unknown.Tag);
    }

    [BindingSpecTest("")]
    [Fact]
    public void OfflineRegionStatusCopiesNativeFields()
    {
        var status = GeneratedValues.CopyOfflineRegionStatus(
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
