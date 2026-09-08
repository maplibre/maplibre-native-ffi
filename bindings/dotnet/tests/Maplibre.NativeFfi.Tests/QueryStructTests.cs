using Maplibre.NativeFfi.Geo;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Struct;
using Maplibre.NativeFfi.Query;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed unsafe class QueryStructTests
{
    [BindingSpecTest("BND-060")]
    [Fact]
    public void RenderedQueryGeometryMaterializesPublicShapes()
    {
        using var point = NativeRenderedQueryGeometry.From(
            new RenderedQueryGeometry.Point(new ScreenPoint(1, 2))
        );
        Assert.Equal(
            (uint)mln_rendered_query_geometry_type.MLN_RENDERED_QUERY_GEOMETRY_TYPE_POINT,
            point.Value.type
        );
        Assert.Equal(1, point.Value.data.point.x);
        Assert.Equal(2, point.Value.data.point.y);

        using var box = NativeRenderedQueryGeometry.From(
            new RenderedQueryGeometry.Box(
                new ScreenBox(new ScreenPoint(3, 4), new ScreenPoint(5, 6))
            )
        );
        Assert.Equal(
            (uint)mln_rendered_query_geometry_type.MLN_RENDERED_QUERY_GEOMETRY_TYPE_BOX,
            box.Value.type
        );
        Assert.Equal(3, box.Value.data.box.min.x);
        Assert.Equal(6, box.Value.data.box.max.y);

        using var line = NativeRenderedQueryGeometry.From(
            new RenderedQueryGeometry.LineString([new ScreenPoint(7, 8), new ScreenPoint(9, 10)])
        );
        Assert.Equal(
            (uint)mln_rendered_query_geometry_type.MLN_RENDERED_QUERY_GEOMETRY_TYPE_LINE_STRING,
            line.Value.type
        );
        Assert.Equal(2u, line.Value.data.line_string.point_count);
        Assert.Equal(9, line.Value.data.line_string.points[1].x);
    }

    [BindingSpecTest("BND-060", "BND-061")]
    [Fact]
    public void QueryOptionsMaterializeOptionalFieldsAndFilters()
    {
        using var rendered = NativeRenderedFeatureQueryOptions.From(
            new RenderedFeatureQueryOptions
            {
                LayerIds = ["roads", "labels"],
                Filter = "true"u8.ToArray(),
            }
        );
        Assert.Equal(
            (uint)
                mln_rendered_feature_query_option_field.MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS,
            rendered.Value.fields
        );
        Assert.Equal(2u, rendered.Value.layer_id_count);
        Assert.Equal(
            "roads",
            RuntimeStructs.CopyUtf8(
                rendered.Value.layer_ids[0].data,
                rendered.Value.layer_ids[0].size
            )
        );
        Assert.Equal(
            "true",
            RuntimeStructs.CopyUtf8(rendered.Value.filter->data, rendered.Value.filter->size)
        );

        using var source = NativeSourceFeatureQueryOptions.From(
            new SourceFeatureQueryOptions
            {
                SourceLayerIds = ["landuse"],
                Filter = "\"visible\""u8.ToArray(),
            }
        );
        Assert.Equal(
            (uint)
                mln_source_feature_query_option_field.MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS,
            source.Value.fields
        );
        Assert.Equal(1u, source.Value.source_layer_id_count);
        Assert.Equal(
            "landuse",
            RuntimeStructs.CopyUtf8(
                source.Value.source_layer_ids[0].data,
                source.Value.source_layer_ids[0].size
            )
        );
        Assert.Equal(
            "\"visible\"",
            RuntimeStructs.CopyUtf8(source.Value.filter->data, source.Value.filter->size)
        );
    }

    [BindingSpecTest("BND-061")]
    [Fact]
    public void FeatureStateSelectorMaterializesOptionalFields()
    {
        using var selector = NativeFeatureStateSelector.From(
            new FeatureStateSelector
            {
                SourceId = "source",
                SourceLayerId = "layer",
                FeatureId = "feature",
                StateKey = "hover",
            }
        );

        var value = selector.Value;
        Assert.Equal(
            (uint)(
                mln_feature_state_selector_field.MLN_FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID
                | mln_feature_state_selector_field.MLN_FEATURE_STATE_SELECTOR_FEATURE_ID
                | mln_feature_state_selector_field.MLN_FEATURE_STATE_SELECTOR_STATE_KEY
            ),
            value.fields
        );
        Assert.Equal("source", RuntimeStructs.CopyUtf8(value.source_id.data, value.source_id.size));
        Assert.Equal(
            "layer",
            RuntimeStructs.CopyUtf8(value.source_layer_id.data, value.source_layer_id.size)
        );
        Assert.Equal(
            "feature",
            RuntimeStructs.CopyUtf8(value.feature_id.data, value.feature_id.size)
        );
        Assert.Equal("hover", RuntimeStructs.CopyUtf8(value.state_key.data, value.state_key.size));
    }
}
