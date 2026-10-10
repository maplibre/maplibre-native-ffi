using Maplibre.NativeFfi.Error;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Memory;
using Maplibre.NativeFfi.Internal.Struct;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

/// <summary>One representative of each generated value shape, crossing to native and back.</summary>
public sealed class GeneratedValueTests
{
    private static byte[] EmptyGeoJsonSource =>
        """{"type":"geojson","data":{"type":"FeatureCollection","features":[]}}"""u8.ToArray();

    [Fact]
    public async Task StringsCrossAsTerminatedAndAsExplicitLengthViews()
    {
        await using var fixture = await NativeFixture.WithEmptyStyleAsync();

        // A source ID crosses as an explicit-length view, so an embedded NUL is part of the ID.
        var added = await fixture.Map.AddStyleSourceJsonAsync(
            "before\0after",
            EmptyGeoJsonSource,
            TestWaits.Token
        );
        Assert.Equal(CommandDisposition.Committed, added.Disposition);
        Assert.NotNull(await fixture.Map.GetStyleSourceAsync("before\0after", TestWaits.Token));
        Assert.Null(await fixture.Map.GetStyleSourceAsync("before", TestWaits.Token));

        // A style URL crosses NUL-terminated, where an embedded NUL would cut it short.
        var error = Assert.Throws<InvalidArgumentException>(() =>
        {
            _ = fixture.Map.SetStyleUrlAsync("style\0.json", TestWaits.Token);
        });
        Assert.Contains("embedded NUL", error.Diagnostic, StringComparison.Ordinal);
    }

    [Fact]
    public async Task PresenceFieldsSendOnlyTheValuesTheCallerSet()
    {
        await using var fixture = await NativeFixture.CreateAsync();

        await fixture.Map.UpdateCameraAsync(
            CameraUpdate.Default with
            {
                Camera = new CameraOptions { Center = new LatLng(10, 20), Zoom = 3 },
            },
            TestWaits.Token
        );
        // Only the bearing is present, so the center and zoom stay as they were.
        await fixture.Map.UpdateCameraAsync(
            CameraUpdate.Default with
            {
                Camera = new CameraOptions { Bearing = 30 },
            },
            TestWaits.Token
        );
        var camera = (await fixture.Map.CameraQueryAsync(TestWaits.Token)).Camera;

        Assert.NotNull(camera.Center);
        Assert.Equal(10, camera.Center.Value.Latitude, 6);
        Assert.Equal(20, camera.Center.Value.Longitude, 6);
        Assert.NotNull(camera.Zoom);
        Assert.Equal(3, camera.Zoom.Value, 6);
        Assert.NotNull(camera.Bearing);
        Assert.Equal(30, camera.Bearing.Value, 6);
    }

    [Fact]
    public async Task AnArrayIsCopiedWhenTheCallIsSubmitted()
    {
        await using var fixture = await NativeFixture.WithEmptyStyleAsync();
        string[] tiles = ["provider-test://tiles/{z}/{x}/{y}.pbf"];

        var added = fixture.Map.AddVectorSourceTilesAsync("tiles", tiles, null, TestWaits.Token);
        tiles[0] = "provider-test://changed/{z}/{x}/{y}.pbf";
        Assert.Equal(CommandDisposition.Committed, (await added).Disposition);

        var source = await fixture.Map.GetStyleSourceAsync("tiles", TestWaits.Token);
        Assert.NotNull(source?.Tilejson);
        Assert.Equal(["provider-test://tiles/{z}/{x}/{y}.pbf"], source.Tilejson.Value.TileUrls);
    }

    // A parameterless constructor starts each member at the header's annotated default, so the
    // record it builds matches what the native default function returns.
    [Fact]
    public void ARecordBuiltFromItsParameterlessConstructorEqualsTheNativeDefault()
    {
        Assert.Equal(MapOptions.Default, new MapOptions());
    }

    [Fact]
    public unsafe void AnInputUnionWritesTheArmItHolds()
    {
        using var scope = new NativeCallScope();

        var box = GeneratedValues.NativeRenderedQueryGeometry(
            new RenderedQueryGeometry.Box(
                new ScreenBox(new ScreenPoint(3, 4), new ScreenPoint(5, 6))
            ),
            scope
        );
        var line = GeneratedValues.NativeRenderedQueryGeometry(
            new RenderedQueryGeometry.LineString(
                new ScreenLineString([new ScreenPoint(7, 8), new ScreenPoint(9, 10)])
            ),
            scope
        );

        Assert.Equal(
            (uint)mln_rendered_query_geometry_type.MLN_RENDERED_QUERY_GEOMETRY_TYPE_BOX,
            box.type
        );
        Assert.Equal(6, box.data.box.max.y);
        Assert.Equal(
            (uint)mln_rendered_query_geometry_type.MLN_RENDERED_QUERY_GEOMETRY_TYPE_LINE_STRING,
            line.type
        );
        Assert.Equal(2u, line.data.line_string.point_count);
        Assert.Equal(9, line.data.line_string.points[1].x);
    }

    [Fact]
    public unsafe void AnOutputUnionWithAnUnknownTagKeepsTheTag()
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
}
