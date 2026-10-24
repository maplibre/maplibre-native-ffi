using System.Text;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Memory;
using Maplibre.NativeFfi.Internal.Struct;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

/// <summary>One representative of each generated value shape, crossing to native and back.</summary>
public sealed class GeneratedValueTests
{
    [Fact]
    public void CallerSizedTextReturnsResultsLongerThanItsFirstStorage()
    {
        using var zoom = StyleExpressionHandle.Create(
            """["interpolate", ["linear"], ["zoom"], 0, 0, 10, 100]"""u8.ToArray(),
            null
        );
        Assert.Equal("50.0"u8.ToArray(), zoom.Evaluate(new StyleExpressionInputs { Zoom = 5 }));

        // The result outgrows the call's first storage, so the call runs again.
        var text = Encoding.UTF8.GetBytes($"\"{new string('a', 1000)}\"");
        using var expression = StyleExpressionHandle.Create(text, null);
        Assert.Equal(text, expression.Evaluate(null));
    }

    [Fact]
    public async Task SynchronousCallsFillCallerSizedOutputsInInputOrder()
    {
        await using var fixture = await NativeFixture.CreateAsync();
        using var projection = MapProjectionHandle.Create(fixture.Map.GetSnapshot().View);
        LatLng[] coordinates = [new(0, 0), new(10, 20), new(-12, -30)];
        var points = projection.PixelsForLatLngs(coordinates);
        Assert.Equal(coordinates.Select(projection.PixelForLatLng), points);
        Assert.Equal(points.Select(projection.LatLngForPixel), projection.LatLngsForPixels(points));
        Assert.Equal(
            points.Select(projection.LatLngForPixelUnwrapped),
            projection.LatLngsForPixelsUnwrapped(points)
        );
        Assert.Empty(projection.PixelsForLatLngs([]));
        Assert.Empty(projection.LatLngsForPixels([]));
        Assert.Empty(projection.LatLngsForPixelsUnwrapped([]));
    }

    private static byte[] EmptyGeoJsonSource =>
        """{"type":"geojson","data":{"type":"FeatureCollection","features":[]}}"""u8.ToArray();

    private static unsafe void AssertNativeLentStringDecodes()
    {
        using var scope = new NativeCallScope();
        Assert.Equal("native-lent é", NativeCallScope.CopyCString(scope.CString("native-lent é")));
    }

    [Fact]
    public async Task HostStringsUseViewsAndNativeLentStringsDecode()
    {
        AssertNativeLentStringDecodes();
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

        // Native rejects a NUL in a URL view before a C-string consumer reads it.
        var error = Assert.Throws<MaplibreException>(() =>
        {
            _ = fixture.Map.SetStyleUrlAsync("style\0.json", TestWaits.Token);
        });
        Assert.Contains("embedded NUL", error.Diagnostic, StringComparison.Ordinal);
        Assert.Equal(Status.InvalidArgument, error.Status);
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
        var camera = (await fixture.Map.GetCameraAsync(TestWaits.Token)).Camera;

        Assert.NotNull(camera.Center);
        Assert.Equal(10, camera.Center.Value.Latitude, 6);
        Assert.Equal(20, camera.Center.Value.Longitude, 6);
        Assert.NotNull(camera.Zoom);
        Assert.Equal(3, camera.Zoom.Value, 6);
        Assert.NotNull(camera.Bearing);
        Assert.Equal(30, camera.Bearing.Value, 6);
    }

    [Fact]
    public void ARecordBuiltFromItsParameterlessConstructorEqualsTheNativeDefault()
    {
        Assert.Equal(MapOptions.Default, new MapOptions());
        Assert.Equal(LogicalExtent.Default, new LogicalExtent());
        Assert.Equal(HttpHeaderRule.Default, new HttpHeaderRule());
        Assert.Equal(PremultipliedRgba8Image.Default, new PremultipliedRgba8Image());
        Assert.Equal(
            HttpHeaderRule.Default with
            {
                Url = "/headers",
            },
            new HttpHeaderRule { Url = "/headers" }
        );
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
