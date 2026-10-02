using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Query;
using Maplibre.NativeFfi.Runtime;
using Maplibre.NativeFfi.Style;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

/// <summary>
/// Records that hold a byte array or a list snapshot it, hand out copies, and compare contents.
/// </summary>
public sealed class ValueEqualityTests
{
    private static readonly byte[] Bytes = [1, 2, 3, 4, 5, 6, 7, 8];

    private static readonly OfflineRegionDefinition Definition =
        new OfflineRegionDefinition.TilePyramid(
            new OfflineTilePyramidRegionDefinition(
                "provider-test://style.json",
                new LatLngBounds(new LatLng(0, 0), new LatLng(1, 1)),
                0,
                10,
                1,
                false
            )
        );

    private static readonly Dictionary<
        string,
        (Func<byte[], object> Make, Func<object, byte[]> Read)
    > ByteValued = new()
    {
        ["image pixels"] = (
            bytes => new PremultipliedRgba8Image(2, 1, 8, bytes),
            value => ((PremultipliedRgba8Image)value).Pixels
        ),
        ["offline region metadata"] = (
            bytes => new OfflineRegionInfo(7, Definition, bytes),
            value => ((OfflineRegionInfo)value).Metadata
        ),
        ["offline region geometry"] = (
            bytes => new OfflineGeometryRegionDefinition(
                "provider-test://style.json",
                bytes,
                0,
                10,
                1,
                false
            ),
            value => ((OfflineGeometryRegionDefinition)value).Geometry
        ),
        ["unknown event payload"] = (
            bytes => new RuntimeEvent.PayloadValue.Unknown(3, bytes),
            value => ((RuntimeEvent.PayloadValue.Unknown)value).PayloadBytes
        ),
        ["queried feature state"] = (
            bytes => new QueriedFeature { Feature = [9], State = bytes },
            value => ((QueriedFeature)value).State!
        ),
        ["resource response body"] = (
            bytes => new ResourceResponse { Status = ResourceResponseStatus.Ok, Bytes = bytes },
            value => ((ResourceResponse)value).Bytes
        ),
    };

    private static readonly Dictionary<
        string,
        (Func<string[], object> Make, Func<object, string[]> Read)
    > ListValued = new()
    {
        ["rendered query layers"] = (
            ids => new RenderedFeatureQueryOptions { LayerIds = ids },
            value => ((RenderedFeatureQueryOptions)value).LayerIds!
        ),
        ["source query layers"] = (
            ids => new SourceFeatureQueryOptions { SourceLayerIds = ids },
            value => ((SourceFeatureQueryOptions)value).SourceLayerIds!
        ),
        ["tile URLs"] = (
            urls => new StyleSourceTileUrlsResult(urls),
            value => ((StyleSourceTileUrlsResult)value).TileUrls
        ),
    };

    public static TheoryData<string> ByteValuedRecords => [.. ByteValued.Keys];

    public static TheoryData<string> ListValuedRecords => [.. ListValued.Keys];

    [Theory]
    [MemberData(nameof(ByteValuedRecords))]
    public void AByteValuedRecordOwnsAndComparesItsBytes(string record)
    {
        var (make, read) = ByteValued[record];
        var source = Bytes.ToArray();
        var value = make(source);
        source[0] = 99;

        Assert.Equal(make(Bytes), value);
        Assert.Equal(make(Bytes).GetHashCode(), value.GetHashCode());
        Assert.NotEqual(make([1, 2, 3, 4, 5, 6, 7, 9]), value);
        read(value)[1] = 99;
        Assert.Equal(Bytes, read(value));
    }

    [Theory]
    [MemberData(nameof(ListValuedRecords))]
    public void AListValuedRecordOwnsAndComparesItsElements(string record)
    {
        var (make, read) = ListValued[record];
        var source = new[] { "a", "b" };
        var value = make(source);
        source[0] = "z";

        Assert.Equal(make(["a", "b"]), value);
        Assert.Equal(make(["a", "b"]).GetHashCode(), value.GetHashCode());
        Assert.NotEqual(make(["a"]), value);
        read(value)[1] = "z";
        Assert.Equal(["a", "b"], read(value));
    }

    // The native field mask tells an absent layer filter from an empty one.
    [Fact]
    public void AnAbsentListDiffersFromAnEmptyOne()
    {
        Assert.NotEqual(
            new RenderedFeatureQueryOptions(),
            new RenderedFeatureQueryOptions { LayerIds = [] }
        );
    }
}
