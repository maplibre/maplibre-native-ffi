using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Memory;
using Maplibre.NativeFfi.Internal.Struct;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Query;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed unsafe class QueryStructTests
{
    [BindingSpecTest("")]
    [Fact]
    public void RenderedQueryGeometryMaterializesPublicShapes()
    {
        using var scope = new NativeCallScope();
        var point = GeneratedValues.NativeRenderedQueryGeometry(
            new RenderedQueryGeometry.Point(new ScreenPoint(1, 2)),
            scope
        );
        Assert.Equal(
            (uint)mln_rendered_query_geometry_type.MLN_RENDERED_QUERY_GEOMETRY_TYPE_POINT,
            point.type
        );
        Assert.Equal(1, point.data.point.x);
        Assert.Equal(2, point.data.point.y);

        var box = GeneratedValues.NativeRenderedQueryGeometry(
            new RenderedQueryGeometry.Box(
                new ScreenBox(new ScreenPoint(3, 4), new ScreenPoint(5, 6))
            ),
            scope
        );
        Assert.Equal(
            (uint)mln_rendered_query_geometry_type.MLN_RENDERED_QUERY_GEOMETRY_TYPE_BOX,
            box.type
        );
        Assert.Equal(3, box.data.box.min.x);
        Assert.Equal(6, box.data.box.max.y);

        var line = GeneratedValues.NativeRenderedQueryGeometry(
            new RenderedQueryGeometry.LineString(
                new ScreenLineString([new ScreenPoint(7, 8), new ScreenPoint(9, 10)])
            ),
            scope
        );
        Assert.Equal(
            (uint)mln_rendered_query_geometry_type.MLN_RENDERED_QUERY_GEOMETRY_TYPE_LINE_STRING,
            line.type
        );
        Assert.Equal(2u, line.data.line_string.point_count);
        Assert.Equal(9, line.data.line_string.points[1].x);
    }

    [BindingSpecTest("", "")]
    [Fact]
    public void QueryOptionsMaterializeOptionalFieldsAndFilters()
    {
        using var scope = new NativeCallScope();
        var rendered = GeneratedValues.NativeRenderedFeatureQueryOptions(
            new RenderedFeatureQueryOptions
            {
                LayerIds = ["roads", "labels"],
                Filter = "true"u8.ToArray(),
            },
            scope
        );
        Assert.Equal(
            (uint)
                mln_rendered_feature_query_option_field.MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS,
            rendered.fields
        );
        Assert.Equal(2u, rendered.layer_id_count);
        Assert.Equal(
            "roads",
            RuntimeStructs.CopyUtf8(rendered.layer_ids[0].data, rendered.layer_ids[0].size)
        );
        Assert.Equal("true", RuntimeStructs.CopyUtf8(rendered.filter->data, rendered.filter->size));

        var source = GeneratedValues.NativeSourceFeatureQueryOptions(
            new SourceFeatureQueryOptions
            {
                SourceLayerIds = ["landuse"],
                Filter = "\"visible\""u8.ToArray(),
            },
            scope
        );
        Assert.Equal(
            (uint)
                mln_source_feature_query_option_field.MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS,
            source.fields
        );
        Assert.Equal(1u, source.source_layer_id_count);
        Assert.Equal(
            "landuse",
            RuntimeStructs.CopyUtf8(
                source.source_layer_ids[0].data,
                source.source_layer_ids[0].size
            )
        );
        Assert.Equal(
            "\"visible\"",
            RuntimeStructs.CopyUtf8(source.filter->data, source.filter->size)
        );
    }
}
