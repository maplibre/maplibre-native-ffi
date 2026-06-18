using System.Runtime.InteropServices;
using Maplibre.Native.Geo;
using Maplibre.Native.Internal.C;
using Maplibre.Native.Internal.Memory;
using Maplibre.Native.Json;

namespace Maplibre.Native.Internal.Struct;

internal sealed unsafe class NativeGeometry : IDisposable
{
    private readonly List<IDisposable> owned = [];
    private readonly List<nint> allocations = [];

    private NativeGeometry(mln_geometry* pointer)
    {
        Pointer = pointer;
    }

    internal mln_geometry* Pointer { get; }

    internal static NativeGeometry From(Geometry geometry)
    {
        ArgumentNullException.ThrowIfNull(geometry);
        var pointer = (mln_geometry*)NativeMemory.AllocZeroed((nuint)sizeof(mln_geometry));
        var native = new NativeGeometry(pointer);
        try
        {
            native.Write(pointer, geometry, 0);
            return native;
        }
        catch
        {
            native.Dispose();
            throw;
        }
    }

    private void Write(mln_geometry* target, Geometry geometry, int depth)
    {
        if (depth > Geometry.MaxCollectionDepth)
        {
            throw new Error.InvalidArgumentException(
                Error.MaplibreStatus.InvalidArgument,
                null,
                $"Geometry exceeds maximum collection depth {Geometry.MaxCollectionDepth}.",
                null
            );
        }

        *target = new mln_geometry { size = (uint)sizeof(mln_geometry) };
        switch (geometry)
        {
            case Geometry.Empty:
                target->type = (uint)mln_geometry_type.MLN_GEOMETRY_TYPE_EMPTY;
                break;
            case Geometry.Point point:
                target->type = (uint)mln_geometry_type.MLN_GEOMETRY_TYPE_POINT;
                target->data.point = CoreStructs.ToNative(point.Coordinate);
                break;
            case Geometry.LineString line:
                target->type = (uint)mln_geometry_type.MLN_GEOMETRY_TYPE_LINE_STRING;
                target->data.line_string = CoordinateSpan(line.Coordinates);
                break;
            case Geometry.Polygon polygon:
                target->type = (uint)mln_geometry_type.MLN_GEOMETRY_TYPE_POLYGON;
                target->data.polygon = Polygon(polygon.Rings);
                break;
            case Geometry.MultiPoint multiPoint:
                target->type = (uint)mln_geometry_type.MLN_GEOMETRY_TYPE_MULTI_POINT;
                target->data.multi_point = CoordinateSpan(multiPoint.Coordinates);
                break;
            case Geometry.MultiLineString multiLine:
                target->type = (uint)mln_geometry_type.MLN_GEOMETRY_TYPE_MULTI_LINE_STRING;
                target->data.multi_line_string = MultiLine(multiLine.Lines);
                break;
            case Geometry.MultiPolygon multiPolygon:
                target->type = (uint)mln_geometry_type.MLN_GEOMETRY_TYPE_MULTI_POLYGON;
                target->data.multi_polygon = MultiPolygon(multiPolygon.Polygons);
                break;
            case Geometry.Collection collection:
                target->type = (uint)mln_geometry_type.MLN_GEOMETRY_TYPE_GEOMETRY_COLLECTION;
                target->data.geometry_collection = Collection(collection.Geometries, depth + 1);
                break;
        }
    }

    private mln_coordinate_span CoordinateSpan(IReadOnlyList<LatLng> coordinates)
    {
        if (coordinates.Count == 0)
        {
            return default;
        }

        var pointer = Allocate<mln_lat_lng>(coordinates.Count);
        for (var index = 0; index < coordinates.Count; index++)
        {
            pointer[index] = CoreStructs.ToNative(coordinates[index]);
        }

        return new mln_coordinate_span
        {
            coordinates = pointer,
            coordinate_count = (nuint)coordinates.Count,
        };
    }

    private mln_polygon_geometry Polygon(IReadOnlyList<IReadOnlyList<LatLng>> rings)
    {
        if (rings.Count == 0)
        {
            return default;
        }

        var pointer = Allocate<mln_coordinate_span>(rings.Count);
        for (var index = 0; index < rings.Count; index++)
        {
            pointer[index] = CoordinateSpan(rings[index]);
        }

        return new mln_polygon_geometry { rings = pointer, ring_count = (nuint)rings.Count };
    }

    private mln_multi_line_geometry MultiLine(IReadOnlyList<IReadOnlyList<LatLng>> lines)
    {
        if (lines.Count == 0)
        {
            return default;
        }

        var pointer = Allocate<mln_coordinate_span>(lines.Count);
        for (var index = 0; index < lines.Count; index++)
        {
            pointer[index] = CoordinateSpan(lines[index]);
        }

        return new mln_multi_line_geometry { lines = pointer, line_count = (nuint)lines.Count };
    }

    private mln_multi_polygon_geometry MultiPolygon(
        IReadOnlyList<IReadOnlyList<IReadOnlyList<LatLng>>> polygons
    )
    {
        if (polygons.Count == 0)
        {
            return default;
        }

        var pointer = Allocate<mln_polygon_geometry>(polygons.Count);
        for (var index = 0; index < polygons.Count; index++)
        {
            pointer[index] = Polygon(polygons[index]);
        }

        return new mln_multi_polygon_geometry
        {
            polygons = pointer,
            polygon_count = (nuint)polygons.Count,
        };
    }

    private mln_geometry_collection Collection(IReadOnlyList<Geometry> geometries, int depth)
    {
        if (geometries.Count == 0)
        {
            return default;
        }

        var pointer = Allocate<mln_geometry>(geometries.Count);
        for (var index = 0; index < geometries.Count; index++)
        {
            Write(&pointer[index], geometries[index], depth);
        }

        return new mln_geometry_collection
        {
            geometries = pointer,
            geometry_count = (nuint)geometries.Count,
        };
    }

    private T* Allocate<T>(int count)
        where T : unmanaged
    {
        var pointer = NativeAllocation.AllocZeroedArray<T>(count);
        allocations.Add((nint)pointer);
        return pointer;
    }

    public void Dispose()
    {
        foreach (var item in owned)
        {
            item.Dispose();
        }
        foreach (var allocation in allocations)
        {
            NativeMemory.Free((void*)allocation);
        }
        NativeMemory.Free(Pointer);
    }
}

internal sealed unsafe class NativeFeature : IDisposable
{
    private readonly NativeGeometry geometry;
    private readonly List<NativeJsonValue> propertyValues = [];
    private readonly List<NativeStringView> stringViews = [];
    private nint properties;

    private NativeFeature(mln_feature value, NativeGeometry geometry)
    {
        Value = value;
        this.geometry = geometry;
    }

    internal mln_feature Value { get; private set; }

    internal static NativeFeature From(Feature feature)
    {
        ArgumentNullException.ThrowIfNull(feature);
        var geometry = NativeGeometry.From(feature.Geometry);
        var native = new NativeFeature(
            new mln_feature { size = (uint)sizeof(mln_feature), geometry = geometry.Pointer },
            geometry
        );
        try
        {
            native.SetProperties(feature.Properties);
            native.SetIdentifier(feature.Identifier);
            return native;
        }
        catch
        {
            native.Dispose();
            throw;
        }
    }

    private void SetProperties(IReadOnlyList<JsonMember> source)
    {
        if (source.Count == 0)
        {
            return;
        }

        var pointer = NativeAllocation.AllocZeroedArray<mln_json_member>(source.Count);
        properties = (nint)pointer;
        for (var index = 0; index < source.Count; index++)
        {
            var key = NativeStringView.From(source[index].Key, $"Properties[{index}].Key");
            var value = NativeJsonValue.From(source[index].Value);
            stringViews.Add(key);
            propertyValues.Add(value);
            pointer[index].key = key.Value;
            pointer[index].value = value.Pointer;
        }

        var current = Value;
        current.properties = pointer;
        current.property_count = (nuint)source.Count;
        Value = current;
    }

    private void SetIdentifier(FeatureIdentifier identifier)
    {
        var current = Value;
        switch (identifier)
        {
            case FeatureIdentifier.Null:
                current.identifier_type = (uint)
                    mln_feature_identifier_type.MLN_FEATURE_IDENTIFIER_TYPE_NULL;
                break;
            case FeatureIdentifier.UInt unsigned:
                current.identifier_type = (uint)
                    mln_feature_identifier_type.MLN_FEATURE_IDENTIFIER_TYPE_UINT;
                current.identifier.uint_value = unsigned.Value;
                break;
            case FeatureIdentifier.Int integer:
                current.identifier_type = (uint)
                    mln_feature_identifier_type.MLN_FEATURE_IDENTIFIER_TYPE_INT;
                current.identifier.int_value = integer.Value;
                break;
            case FeatureIdentifier.Double number:
                current.identifier_type = (uint)
                    mln_feature_identifier_type.MLN_FEATURE_IDENTIFIER_TYPE_DOUBLE;
                current.identifier.double_value = number.Value;
                break;
            case FeatureIdentifier.String text:
                current.identifier_type = (uint)
                    mln_feature_identifier_type.MLN_FEATURE_IDENTIFIER_TYPE_STRING;
                var view = NativeStringView.From(text.Value, nameof(identifier));
                stringViews.Add(view);
                current.identifier.string_value = view.Value;
                break;
        }
        Value = current;
    }

    public void Dispose()
    {
        geometry.Dispose();
        foreach (var value in propertyValues)
        {
            value.Dispose();
        }
        foreach (var view in stringViews)
        {
            view.Dispose();
        }
        if (properties != 0)
        {
            NativeMemory.Free((void*)properties);
        }
    }
}

internal sealed unsafe class NativeGeoJson : IDisposable
{
    private readonly List<IDisposable> owned = [];
    private readonly List<nint> allocations = [];

    private NativeGeoJson(mln_geojson* pointer)
    {
        Pointer = pointer;
    }

    internal mln_geojson* Pointer { get; }

    internal static NativeGeoJson From(GeoJson value)
    {
        ArgumentNullException.ThrowIfNull(value);
        var pointer = (mln_geojson*)NativeMemory.AllocZeroed((nuint)sizeof(mln_geojson));
        var native = new NativeGeoJson(pointer);
        try
        {
            native.Write(value);
            return native;
        }
        catch
        {
            native.Dispose();
            throw;
        }
    }

    private void Write(GeoJson value)
    {
        *Pointer = new mln_geojson { size = (uint)sizeof(mln_geojson) };
        switch (value)
        {
            case GeoJson.GeometryValue geometryValue:
                Pointer->type = (uint)mln_geojson_type.MLN_GEOJSON_TYPE_GEOMETRY;
                var geometry = NativeGeometry.From(geometryValue.Geometry);
                owned.Add(geometry);
                Pointer->data.geometry = geometry.Pointer;
                break;
            case GeoJson.FeatureValue featureValue:
                Pointer->type = (uint)mln_geojson_type.MLN_GEOJSON_TYPE_FEATURE;
                var feature = NativeFeature.From(featureValue.Feature);
                owned.Add(feature);
                var featurePointer = Allocate<mln_feature>(1);
                *featurePointer = feature.Value;
                Pointer->data.feature = featurePointer;
                break;
            case GeoJson.FeatureCollection collection:
                Pointer->type = (uint)mln_geojson_type.MLN_GEOJSON_TYPE_FEATURE_COLLECTION;
                Pointer->data.feature_collection = FeatureCollection(collection.Features);
                break;
        }
    }

    private mln_feature_collection FeatureCollection(IReadOnlyList<Feature> features)
    {
        if (features.Count == 0)
        {
            return default;
        }

        var pointer = Allocate<mln_feature>(features.Count);
        for (var index = 0; index < features.Count; index++)
        {
            var feature = NativeFeature.From(features[index]);
            owned.Add(feature);
            pointer[index] = feature.Value;
        }

        return new mln_feature_collection
        {
            features = pointer,
            feature_count = (nuint)features.Count,
        };
    }

    private T* Allocate<T>(int count)
        where T : unmanaged
    {
        var pointer = NativeAllocation.AllocZeroedArray<T>(count);
        allocations.Add((nint)pointer);
        return pointer;
    }

    public void Dispose()
    {
        foreach (var item in owned)
        {
            item.Dispose();
        }
        foreach (var allocation in allocations)
        {
            NativeMemory.Free((void*)allocation);
        }
        NativeMemory.Free(Pointer);
    }
}
