using System.Runtime.InteropServices;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Memory;
using Maplibre.NativeFfi.Query;

namespace Maplibre.NativeFfi.Internal.Struct;

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
        return geometry switch
        {
            RenderedQueryGeometry.Point point => new(
                NativeMethods.mln_rendered_query_geometry_point(MapStructs.ToNative(point.Value)),
                0
            ),
            RenderedQueryGeometry.Box box => new(
                NativeMethods.mln_rendered_query_geometry_box(
                    new mln_screen_box
                    {
                        min = MapStructs.ToNative(box.Value.Min),
                        max = MapStructs.ToNative(box.Value.Max),
                    }
                ),
                0
            ),
            RenderedQueryGeometry.LineString line => FromLineString(line),
            _ => throw new ArgumentException(
                $"Unsupported rendered query geometry type {geometry.GetType().Name}.",
                nameof(geometry)
            ),
        };
    }

    private static NativeRenderedQueryGeometry FromLineString(RenderedQueryGeometry.LineString line)
    {
        var nativePoints = NativeAllocation.AllocArray<mln_screen_point>(line.Points.Count);
        for (var index = 0; index < line.Points.Count; index++)
        {
            nativePoints[index] = MapStructs.ToNative(line.Points[index]);
        }
        return new NativeRenderedQueryGeometry(
            NativeMethods.mln_rendered_query_geometry_line_string(
                nativePoints,
                (nuint)line.Points.Count
            ),
            (nint)nativePoints
        );
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
    private readonly NativeStringView? filter;
    private nint layerIdArray;

    private NativeRenderedFeatureQueryOptions(
        mln_rendered_feature_query_options value,
        NativeStringView? filter
    )
    {
        Value = value;
        this.filter = filter;
    }

    internal mln_rendered_feature_query_options Value { get; private set; }

    internal static NativeRenderedFeatureQueryOptions From(RenderedFeatureQueryOptions options)
    {
        ArgumentNullException.ThrowIfNull(options);
        var filter = options.Filter is null
            ? null
            : NativeStringView.From(options.Filter, nameof(options.Filter));
        var seeded = NativeMethods.mln_rendered_feature_query_options_default();
        seeded.filter = filter?.Pointer;
        var native = new NativeRenderedFeatureQueryOptions(seeded, filter);
        try
        {
            if (options.LayerIds is { } ids)
            {
                var value = native.Value;
                value.fields |= (uint)
                    mln_rendered_feature_query_option_field.MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS;
                value.layer_id_count = (nuint)ids.Count;
                if (ids.Count > 0)
                {
                    var pointer = NativeAllocation.AllocArray<mln_buffer_view>(ids.Count);
                    native.layerIdArray = (nint)pointer;
                    for (var index = 0; index < ids.Count; index++)
                    {
                        var view = NativeStringView.From(ids[index], $"LayerIds[{index}]");
                        native.layerIds.Add(view);
                        pointer[index] = view.Value;
                    }
                    value.layer_ids = pointer;
                }
                native.Value = value;
            }
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
        foreach (var value in layerIds)
            value.Dispose();
        if (layerIdArray != 0)
            NativeMemory.Free((void*)layerIdArray);
        filter?.Dispose();
    }
}

internal sealed unsafe class NativeSourceFeatureQueryOptions : IDisposable
{
    private readonly List<NativeStringView> sourceLayerIds = [];
    private readonly NativeStringView? filter;
    private nint sourceLayerIdArray;

    private NativeSourceFeatureQueryOptions(
        mln_source_feature_query_options value,
        NativeStringView? filter
    )
    {
        Value = value;
        this.filter = filter;
    }

    internal mln_source_feature_query_options Value { get; private set; }

    internal static NativeSourceFeatureQueryOptions From(SourceFeatureQueryOptions options)
    {
        ArgumentNullException.ThrowIfNull(options);
        var filter = options.Filter is null
            ? null
            : NativeStringView.From(options.Filter, nameof(options.Filter));
        var seeded = NativeMethods.mln_source_feature_query_options_default();
        seeded.filter = filter?.Pointer;
        var native = new NativeSourceFeatureQueryOptions(seeded, filter);
        try
        {
            if (options.SourceLayerIds is { } ids)
            {
                var value = native.Value;
                value.fields |= (uint)
                    mln_source_feature_query_option_field.MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS;
                value.source_layer_id_count = (nuint)ids.Count;
                if (ids.Count > 0)
                {
                    var pointer = NativeAllocation.AllocArray<mln_buffer_view>(ids.Count);
                    native.sourceLayerIdArray = (nint)pointer;
                    for (var index = 0; index < ids.Count; index++)
                    {
                        var view = NativeStringView.From(ids[index], $"SourceLayerIds[{index}]");
                        native.sourceLayerIds.Add(view);
                        pointer[index] = view.Value;
                    }
                    value.source_layer_ids = pointer;
                }
                native.Value = value;
            }
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
        foreach (var value in sourceLayerIds)
            value.Dispose();
        if (sourceLayerIdArray != 0)
            NativeMemory.Free((void*)sourceLayerIdArray);
        filter?.Dispose();
    }
}
