using Maplibre.Native.Geo;
using Maplibre.Native.Internal.C;
using Maplibre.Native.Internal.Struct;
using Maplibre.Native.Json;
using Maplibre.Native.Query;
using Xunit;

namespace Maplibre.Native.Tests;

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
                Filter = new JsonValue.Bool(true),
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
            (uint)mln_json_value_type.MLN_JSON_VALUE_TYPE_BOOL,
            rendered.Value.filter->type
        );

        using var source = NativeSourceFeatureQueryOptions.From(
            new SourceFeatureQueryOptions
            {
                SourceLayerIds = ["landuse"],
                Filter = new JsonValue.String("visible"),
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
            (uint)mln_json_value_type.MLN_JSON_VALUE_TYPE_STRING,
            source.Value.filter->type
        );
    }

    [BindingSpecTest("BND-106")]
    [Fact]
    public void QueriedFeatureCopiesNestedFeatureAndOptionalFields()
    {
        var coordinates = stackalloc mln_lat_lng[2];
        coordinates[0] = new mln_lat_lng { latitude = 1, longitude = 2 };
        coordinates[1] = new mln_lat_lng { latitude = 3, longitude = 4 };
        var geometry = new mln_geometry
        {
            size = (uint)sizeof(mln_geometry),
            type = (uint)mln_geometry_type.MLN_GEOMETRY_TYPE_LINE_STRING,
            data =
            {
                line_string = new mln_coordinate_span
                {
                    coordinates = coordinates,
                    coordinate_count = 2,
                },
            },
        };

        using var propertyKey = NativeStringView.From("name", "propertyKey");
        using var propertyValue = NativeJsonValue.From(new JsonValue.String("park"));
        var properties = stackalloc mln_json_member[1];
        properties[0] = new mln_json_member
        {
            key = propertyKey.Value,
            value = propertyValue.Pointer,
        };
        using var identifier = NativeStringView.From("feature-1", "identifier");
        var feature = new mln_feature
        {
            size = (uint)sizeof(mln_feature),
            geometry = &geometry,
            properties = properties,
            property_count = 1,
            identifier_type = (uint)mln_feature_identifier_type.MLN_FEATURE_IDENTIFIER_TYPE_STRING,
            identifier = { string_value = identifier.Value },
        };

        using var sourceId = NativeStringView.From("source", "sourceId");
        using var sourceLayerId = NativeStringView.From("layer", "sourceLayerId");
        using var state = NativeJsonValue.From(
            new JsonValue.Object([new JsonMember("hover", new JsonValue.Bool(true))])
        );
        var queried = QueryStructs.ReadQueriedFeature(
            new mln_queried_feature
            {
                size = (uint)sizeof(mln_queried_feature),
                fields = (uint)(
                    mln_queried_feature_field.MLN_QUERIED_FEATURE_SOURCE_ID
                    | mln_queried_feature_field.MLN_QUERIED_FEATURE_SOURCE_LAYER_ID
                    | mln_queried_feature_field.MLN_QUERIED_FEATURE_STATE
                ),
                feature = feature,
                source_id = sourceId.Value,
                source_layer_id = sourceLayerId.Value,
                state = state.Pointer,
            }
        );

        Assert.Equal("source", queried.SourceId);
        Assert.Equal("layer", queried.SourceLayerId);
        Assert.IsType<JsonValue.Object>(queried.State);
        var line = Assert.IsType<Geometry.LineString>(queried.Feature.Geometry);
        Assert.Equal(new LatLng(3, 4), line.Coordinates[1]);
        Assert.Equal(
            new JsonMember("name", new JsonValue.String("park")),
            queried.Feature.Properties[0]
        );
        var id = Assert.IsType<FeatureIdentifier.String>(queried.Feature.Identifier);
        Assert.Equal("feature-1", id.Value);
    }

    [BindingSpecTest("BND-066")]
    [Fact]
    public void FeatureQueryResultIsDestroyedWhenCopyingFeatureFails()
    {
        var destroyCalls = 0;
        using var methods = QueryStructs.UseFeatureQueryResultMethodsForTest(
            (_, count) =>
            {
                *count = 1;
                return mln_status.MLN_STATUS_OK;
            },
            (_, _, feature) =>
            {
                *feature = new mln_queried_feature
                {
                    size = (uint)sizeof(mln_queried_feature),
                    feature = new mln_feature
                    {
                        size = (uint)sizeof(mln_feature),
                        identifier_type = 999,
                    },
                };
                return mln_status.MLN_STATUS_OK;
            },
            _ => destroyCalls++
        );

        Assert.Throws<InvalidOperationException>(() =>
            QueryStructs.ReadFeatureQueryResult((mln_feature_query_result*)1234)
        );

        Assert.Equal(1, destroyCalls);
    }

    [BindingSpecTest("BND-106")]
    [Fact]
    public void NativeFeatureNullIdentifierMaterializesNullIdentifier()
    {
        var feature = QueryStructs.ReadFeature(
            new mln_feature
            {
                size = (uint)sizeof(mln_feature),
                identifier_type = (uint)
                    mln_feature_identifier_type.MLN_FEATURE_IDENTIFIER_TYPE_NULL,
            }
        );

        Assert.Same(FeatureIdentifier.Null.Instance, feature.Identifier);
    }

    // Support invariant for copied query output: malformed native discriminants
    // fail deterministically instead of fabricating public feature values.
    [Fact]
    public void UnknownNativeFeatureDiscriminantsThrow()
    {
        var geometryError = Assert.Throws<InvalidOperationException>(() =>
            QueryStructs.ReadGeometry(
                new mln_geometry { size = (uint)sizeof(mln_geometry), type = 999 }
            )
        );
        Assert.Contains("mln_geometry_type", geometryError.Message, StringComparison.Ordinal);
        Assert.Contains("999", geometryError.Message, StringComparison.Ordinal);

        var identifierError = Assert.Throws<InvalidOperationException>(() =>
            QueryStructs.ReadFeature(
                new mln_feature { size = (uint)sizeof(mln_feature), identifier_type = 999 }
            )
        );
        Assert.Contains(
            "mln_feature_identifier_type",
            identifierError.Message,
            StringComparison.Ordinal
        );
        Assert.Contains("999", identifierError.Message, StringComparison.Ordinal);
    }
}
