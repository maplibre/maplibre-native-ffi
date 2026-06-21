using System.Runtime.InteropServices;
using Maplibre.Native.Geo;
using Maplibre.Native.Internal.C;
using Maplibre.Native.Internal.Memory;
using Maplibre.Native.Internal.Status;
using Maplibre.Native.Json;
using Maplibre.Native.Query;

namespace Maplibre.Native.Internal.Struct;

internal unsafe delegate mln_status FeatureQueryResultCount(
    mln_feature_query_result* result,
    nuint* outCount
);

internal unsafe delegate mln_status FeatureQueryResultGet(
    mln_feature_query_result* result,
    nuint index,
    mln_queried_feature* outFeature
);

internal unsafe delegate void FeatureQueryResultDestroy(mln_feature_query_result* result);

internal sealed unsafe class NativeRenderedQueryGeometry : IDisposable
{
    private nint points;

    private NativeRenderedQueryGeometry(mln_rendered_query_geometry value, nint points)
    {
        Value = value;
        this.points = points;
    }

    internal mln_rendered_query_geometry Value { get; }

    internal static NativeRenderedQueryGeometry From(RenderedQueryGeometry geometry)
    {
        ArgumentNullException.ThrowIfNull(geometry);
        switch (geometry)
        {
            case RenderedQueryGeometry.Point point:
                return new NativeRenderedQueryGeometry(
                    NativeMethods.mln_rendered_query_geometry_point(
                        MapStructs.ToNative(point.Value)
                    ),
                    0
                );
            case RenderedQueryGeometry.Box box:
                return new NativeRenderedQueryGeometry(
                    NativeMethods.mln_rendered_query_geometry_box(
                        new mln_screen_box
                        {
                            min = MapStructs.ToNative(box.Value.Min),
                            max = MapStructs.ToNative(box.Value.Max),
                        }
                    ),
                    0
                );
            case RenderedQueryGeometry.LineString line:
                var count = line.Points.Count;
                var nativePoints = NativeAllocation.AllocArray<mln_screen_point>(count);
                var allocation = (nint)nativePoints;
                for (var index = 0; index < count; index++)
                {
                    nativePoints[index] = MapStructs.ToNative(line.Points[index]);
                }
                try
                {
                    return new NativeRenderedQueryGeometry(
                        NativeMethods.mln_rendered_query_geometry_line_string(
                            nativePoints,
                            (nuint)count
                        ),
                        allocation
                    );
                }
                catch
                {
                    if (allocation != 0)
                    {
                        NativeMemory.Free((void*)allocation);
                    }
                    throw;
                }
            default:
                throw new ArgumentException(
                    $"Unsupported rendered query geometry type {geometry.GetType().Name}.",
                    nameof(geometry)
                );
        }
    }

    public void Dispose()
    {
        if (points != 0)
        {
            NativeMemory.Free((void*)points);
            points = 0;
        }
    }
}

internal sealed unsafe class NativeRenderedFeatureQueryOptions : IDisposable
{
    private readonly List<NativeStringView> layerIds = [];
    private readonly NativeJsonValue? filter;
    private nint layerIdArray;

    private NativeRenderedFeatureQueryOptions(
        mln_rendered_feature_query_options value,
        NativeJsonValue? filter
    )
    {
        Value = value;
        this.filter = filter;
    }

    internal mln_rendered_feature_query_options Value { get; private set; }

    internal static NativeRenderedFeatureQueryOptions From(RenderedFeatureQueryOptions options)
    {
        ArgumentNullException.ThrowIfNull(options);
        var filter = options.Filter is null ? null : NativeJsonValue.From(options.Filter);
        var native = new NativeRenderedFeatureQueryOptions(
            new mln_rendered_feature_query_options
            {
                size = (uint)sizeof(mln_rendered_feature_query_options),
            },
            filter
        );
        try
        {
            var value = native.Value;
            if (options.LayerIds is { } ids)
            {
                value.fields |= (uint)
                    mln_rendered_feature_query_option_field.MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS;
                value.layer_id_count = (nuint)ids.Count;
                if (ids.Count > 0)
                {
                    var pointer = NativeAllocation.AllocArray<mln_string_view>(ids.Count);
                    native.layerIdArray = (nint)pointer;
                    for (var index = 0; index < ids.Count; index++)
                    {
                        var view = NativeStringView.From(ids[index], $"LayerIds[{index}]");
                        native.layerIds.Add(view);
                        pointer[index] = view.Value;
                    }
                    value.layer_ids = pointer;
                }
            }
            value.filter = filter?.Pointer;
            native.Value = value;
            return native;
        }
        catch
        {
            native.Dispose();
            throw;
        }
    }

    public void Dispose()
    {
        foreach (var view in layerIds)
        {
            view.Dispose();
        }
        if (layerIdArray != 0)
        {
            NativeMemory.Free((void*)layerIdArray);
            layerIdArray = 0;
        }
        filter?.Dispose();
    }
}

internal sealed unsafe class NativeSourceFeatureQueryOptions : IDisposable
{
    private readonly List<NativeStringView> sourceLayerIds = [];
    private readonly NativeJsonValue? filter;
    private nint sourceLayerIdArray;

    private NativeSourceFeatureQueryOptions(
        mln_source_feature_query_options value,
        NativeJsonValue? filter
    )
    {
        Value = value;
        this.filter = filter;
    }

    internal mln_source_feature_query_options Value { get; private set; }

    internal static NativeSourceFeatureQueryOptions From(SourceFeatureQueryOptions options)
    {
        ArgumentNullException.ThrowIfNull(options);
        var filter = options.Filter is null ? null : NativeJsonValue.From(options.Filter);
        var native = new NativeSourceFeatureQueryOptions(
            new mln_source_feature_query_options
            {
                size = (uint)sizeof(mln_source_feature_query_options),
            },
            filter
        );
        try
        {
            var value = native.Value;
            if (options.SourceLayerIds is { } ids)
            {
                value.fields |= (uint)
                    mln_source_feature_query_option_field.MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS;
                value.source_layer_id_count = (nuint)ids.Count;
                if (ids.Count > 0)
                {
                    var pointer = NativeAllocation.AllocArray<mln_string_view>(ids.Count);
                    native.sourceLayerIdArray = (nint)pointer;
                    for (var index = 0; index < ids.Count; index++)
                    {
                        var view = NativeStringView.From(ids[index], $"SourceLayerIds[{index}]");
                        native.sourceLayerIds.Add(view);
                        pointer[index] = view.Value;
                    }
                    value.source_layer_ids = pointer;
                }
            }
            value.filter = filter?.Pointer;
            native.Value = value;
            return native;
        }
        catch
        {
            native.Dispose();
            throw;
        }
    }

    public void Dispose()
    {
        foreach (var view in sourceLayerIds)
        {
            view.Dispose();
        }
        if (sourceLayerIdArray != 0)
        {
            NativeMemory.Free((void*)sourceLayerIdArray);
            sourceLayerIdArray = 0;
        }
        filter?.Dispose();
    }
}

internal static unsafe class QueryStructs
{
    private static readonly FeatureQueryResultCount DefaultFeatureQueryResultCount = static (
        result,
        outCount
    ) => NativeMethods.mln_feature_query_result_count(result, outCount);
    private static readonly FeatureQueryResultGet DefaultFeatureQueryResultGet = static (
        result,
        index,
        outFeature
    ) => NativeMethods.mln_feature_query_result_get(result, index, outFeature);
    private static readonly FeatureQueryResultDestroy DefaultFeatureQueryResultDestroy =
        static result => NativeMethods.mln_feature_query_result_destroy(result);

    [ThreadStatic]
    private static FeatureQueryResultCount? featureQueryResultCountForTest;

    [ThreadStatic]
    private static FeatureQueryResultGet? featureQueryResultGetForTest;

    [ThreadStatic]
    private static FeatureQueryResultDestroy? featureQueryResultDestroyForTest;

    internal static IReadOnlyList<QueriedFeature> ReadFeatureQueryResult(
        mln_feature_query_result* result
    )
    {
        if (result is null)
        {
            return [];
        }

        try
        {
            nuint count = 0;
            NativeStatus.Check(FeatureQueryCount(result, &count));
            var features = new QueriedFeature[checked((int)count)];
            for (nuint index = 0; index < count; index++)
            {
                var feature = new mln_queried_feature { size = (uint)sizeof(mln_queried_feature) };
                NativeStatus.Check(FeatureQueryGet(result, index, &feature));
                features[(int)index] = ReadQueriedFeature(feature);
            }
            return features;
        }
        finally
        {
            FeatureQueryDestroy(result);
        }
    }

    internal static IDisposable UseFeatureQueryResultMethodsForTest(
        FeatureQueryResultCount count,
        FeatureQueryResultGet get,
        FeatureQueryResultDestroy destroy
    )
    {
        var previousCount = featureQueryResultCountForTest;
        var previousGet = featureQueryResultGetForTest;
        var previousDestroy = featureQueryResultDestroyForTest;
        featureQueryResultCountForTest = count;
        featureQueryResultGetForTest = get;
        featureQueryResultDestroyForTest = destroy;
        return new RestoreFeatureQueryResultMethods(previousCount, previousGet, previousDestroy);
    }

    private static FeatureQueryResultCount FeatureQueryCount =>
        featureQueryResultCountForTest ?? DefaultFeatureQueryResultCount;

    private static FeatureQueryResultGet FeatureQueryGet =>
        featureQueryResultGetForTest ?? DefaultFeatureQueryResultGet;

    private static FeatureQueryResultDestroy FeatureQueryDestroy =>
        featureQueryResultDestroyForTest ?? DefaultFeatureQueryResultDestroy;

    private sealed class RestoreFeatureQueryResultMethods(
        FeatureQueryResultCount? previousCount,
        FeatureQueryResultGet? previousGet,
        FeatureQueryResultDestroy? previousDestroy
    ) : IDisposable
    {
        public void Dispose()
        {
            featureQueryResultCountForTest = previousCount;
            featureQueryResultGetForTest = previousGet;
            featureQueryResultDestroyForTest = previousDestroy;
        }
    }

    internal static FeatureExtensionResult ReadFeatureExtensionResult(
        mln_feature_extension_result* result
    )
    {
        if (result is null)
        {
            return new FeatureExtensionResult.Unknown(0);
        }

        try
        {
            var info = new mln_feature_extension_result_info
            {
                size = (uint)sizeof(mln_feature_extension_result_info),
            };
            NativeStatus.Check(NativeMethods.mln_feature_extension_result_get(result, &info));
            return (mln_feature_extension_result_type)info.type switch
            {
                mln_feature_extension_result_type.MLN_FEATURE_EXTENSION_RESULT_TYPE_VALUE =>
                    new FeatureExtensionResult.Value(ValueStructs.ReadJsonValue(info.data.value)),
                mln_feature_extension_result_type.MLN_FEATURE_EXTENSION_RESULT_TYPE_FEATURE_COLLECTION =>
                    new FeatureExtensionResult.FeatureCollection(
                        ReadFeatureCollection(info.data.feature_collection)
                    ),
                _ => new FeatureExtensionResult.Unknown(info.type),
            };
        }
        finally
        {
            NativeMethods.mln_feature_extension_result_destroy(result);
        }
    }

    internal static QueriedFeature ReadQueriedFeature(mln_queried_feature value)
    {
        var fields = (mln_queried_feature_field)value.fields;
        var sourceId = fields.HasFlag(mln_queried_feature_field.MLN_QUERIED_FEATURE_SOURCE_ID)
            ? RuntimeStructs.CopyUtf8(value.source_id.data, value.source_id.size)
            : null;
        var sourceLayerId = fields.HasFlag(
            mln_queried_feature_field.MLN_QUERIED_FEATURE_SOURCE_LAYER_ID
        )
            ? RuntimeStructs.CopyUtf8(value.source_layer_id.data, value.source_layer_id.size)
            : null;
        var state =
            fields.HasFlag(mln_queried_feature_field.MLN_QUERIED_FEATURE_STATE)
            && value.state is not null
                ? ValueStructs.ReadJsonValue(value.state)
                : null;
        return new QueriedFeature(ReadFeature(value.feature), sourceId, sourceLayerId, state);
    }

    internal static IReadOnlyList<Feature> ReadFeatureCollection(mln_feature_collection collection)
    {
        var count = checked((int)collection.feature_count);
        var features = new Feature[count];
        for (var index = 0; index < count; index++)
        {
            features[index] = ReadFeature(collection.features[index]);
        }
        return features;
    }

    internal static Feature ReadFeature(mln_feature value)
    {
        var geometry = value.geometry is null
            ? Geometry.Empty.Instance
            : ReadGeometry(*value.geometry);
        var properties = new JsonMember[checked((int)value.property_count)];
        for (var index = 0; index < properties.Length; index++)
        {
            var member = value.properties[index];
            properties[index] = new JsonMember(
                RuntimeStructs.CopyUtf8(member.key.data, member.key.size),
                ValueStructs.ReadJsonValue(member.value)
            );
        }
        return new Feature(geometry, properties, ReadIdentifier(value));
    }

    internal static Geometry ReadGeometry(mln_geometry geometry) =>
        (mln_geometry_type)geometry.type switch
        {
            mln_geometry_type.MLN_GEOMETRY_TYPE_EMPTY => Geometry.Empty.Instance,
            mln_geometry_type.MLN_GEOMETRY_TYPE_POINT => new Geometry.Point(
                CoreStructs.FromNative(geometry.data.point)
            ),
            mln_geometry_type.MLN_GEOMETRY_TYPE_LINE_STRING => new Geometry.LineString(
                ReadCoordinateSpan(geometry.data.line_string)
            ),
            mln_geometry_type.MLN_GEOMETRY_TYPE_POLYGON => new Geometry.Polygon(
                ReadPolygon(geometry.data.polygon)
            ),
            mln_geometry_type.MLN_GEOMETRY_TYPE_MULTI_POINT => new Geometry.MultiPoint(
                ReadCoordinateSpan(geometry.data.multi_point)
            ),
            mln_geometry_type.MLN_GEOMETRY_TYPE_MULTI_LINE_STRING => new Geometry.MultiLineString(
                ReadMultiLine(geometry.data.multi_line_string)
            ),
            mln_geometry_type.MLN_GEOMETRY_TYPE_MULTI_POLYGON => new Geometry.MultiPolygon(
                ReadMultiPolygon(geometry.data.multi_polygon)
            ),
            mln_geometry_type.MLN_GEOMETRY_TYPE_GEOMETRY_COLLECTION => new Geometry.Collection(
                ReadGeometryCollection(geometry.data.geometry_collection)
            ),
            _ => throw new InvalidOperationException(
                $"ReadGeometry received unknown mln_geometry_type value {geometry.type}."
            ),
        };

    private static FeatureIdentifier ReadIdentifier(mln_feature value) =>
        (mln_feature_identifier_type)value.identifier_type switch
        {
            mln_feature_identifier_type.MLN_FEATURE_IDENTIFIER_TYPE_NULL => FeatureIdentifier
                .Null
                .Instance,
            mln_feature_identifier_type.MLN_FEATURE_IDENTIFIER_TYPE_UINT =>
                new FeatureIdentifier.UInt(value.identifier.uint_value),
            mln_feature_identifier_type.MLN_FEATURE_IDENTIFIER_TYPE_INT =>
                new FeatureIdentifier.Int(value.identifier.int_value),
            mln_feature_identifier_type.MLN_FEATURE_IDENTIFIER_TYPE_DOUBLE =>
                new FeatureIdentifier.Double(value.identifier.double_value),
            mln_feature_identifier_type.MLN_FEATURE_IDENTIFIER_TYPE_STRING =>
                new FeatureIdentifier.String(
                    RuntimeStructs.CopyUtf8(
                        value.identifier.string_value.data,
                        value.identifier.string_value.size
                    )
                ),
            _ => throw new InvalidOperationException(
                $"ReadIdentifier received unknown mln_feature_identifier_type value {value.identifier_type}."
            ),
        };

    private static IReadOnlyList<LatLng> ReadCoordinateSpan(mln_coordinate_span span)
    {
        var coordinates = new LatLng[checked((int)span.coordinate_count)];
        for (var index = 0; index < coordinates.Length; index++)
        {
            coordinates[index] = CoreStructs.FromNative(span.coordinates[index]);
        }
        return coordinates;
    }

    private static IReadOnlyList<IReadOnlyList<LatLng>> ReadPolygon(mln_polygon_geometry polygon)
    {
        var rings = new IReadOnlyList<LatLng>[checked((int)polygon.ring_count)];
        for (var index = 0; index < rings.Length; index++)
        {
            rings[index] = ReadCoordinateSpan(polygon.rings[index]);
        }
        return rings;
    }

    private static IReadOnlyList<IReadOnlyList<LatLng>> ReadMultiLine(
        mln_multi_line_geometry multiLine
    )
    {
        var lines = new IReadOnlyList<LatLng>[checked((int)multiLine.line_count)];
        for (var index = 0; index < lines.Length; index++)
        {
            lines[index] = ReadCoordinateSpan(multiLine.lines[index]);
        }
        return lines;
    }

    private static IReadOnlyList<IReadOnlyList<IReadOnlyList<LatLng>>> ReadMultiPolygon(
        mln_multi_polygon_geometry multiPolygon
    )
    {
        var polygons = new IReadOnlyList<IReadOnlyList<LatLng>>[
            checked((int)multiPolygon.polygon_count)
        ];
        for (var index = 0; index < polygons.Length; index++)
        {
            polygons[index] = ReadPolygon(multiPolygon.polygons[index]);
        }
        return polygons;
    }

    private static IReadOnlyList<Geometry> ReadGeometryCollection(
        mln_geometry_collection collection
    )
    {
        var geometries = new Geometry[checked((int)collection.geometry_count)];
        for (var index = 0; index < geometries.Length; index++)
        {
            geometries[index] = ReadGeometry(collection.geometries[index]);
        }
        return geometries;
    }
}
