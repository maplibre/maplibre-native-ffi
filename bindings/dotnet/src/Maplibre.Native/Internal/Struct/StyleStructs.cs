using System.Runtime.InteropServices;
using Maplibre.Native.Geo;
using Maplibre.Native.Internal.C;
using Maplibre.Native.Internal.Memory;
using Maplibre.Native.Render;
using Maplibre.Native.Style;

namespace Maplibre.Native.Internal.Struct;

internal sealed unsafe class NativeTileSourceOptions : IDisposable
{
    private readonly NativeStringView? attribution;

    private NativeTileSourceOptions(
        mln_style_tile_source_options value,
        NativeStringView? attribution
    )
    {
        Value = value;
        this.attribution = attribution;
    }

    internal mln_style_tile_source_options Value { get; }

    internal static NativeTileSourceOptions From(TileSourceOptions options)
    {
        ArgumentNullException.ThrowIfNull(options);
        NativeStringView? attribution = null;
        try
        {
            var native = NativeMethods.mln_style_tile_source_options_default();
            if (options.MinimumZoom is { } minimumZoom)
            {
                native.fields |= (uint)
                    mln_style_tile_source_option_field.MLN_STYLE_TILE_SOURCE_OPTION_MIN_ZOOM;
                native.min_zoom = minimumZoom;
            }
            if (options.MaximumZoom is { } maximumZoom)
            {
                native.fields |= (uint)
                    mln_style_tile_source_option_field.MLN_STYLE_TILE_SOURCE_OPTION_MAX_ZOOM;
                native.max_zoom = maximumZoom;
            }
            if (options.Attribution is { } attributionValue)
            {
                native.fields |= (uint)
                    mln_style_tile_source_option_field.MLN_STYLE_TILE_SOURCE_OPTION_ATTRIBUTION;
                attribution = NativeStringView.From(attributionValue, nameof(options.Attribution));
                native.attribution = attribution.Value;
            }
            if (options.Scheme is { } scheme)
            {
                native.fields |= (uint)
                    mln_style_tile_source_option_field.MLN_STYLE_TILE_SOURCE_OPTION_SCHEME;
                native.scheme = (uint)scheme;
            }
            if (options.Bounds is { } bounds)
            {
                native.fields |= (uint)
                    mln_style_tile_source_option_field.MLN_STYLE_TILE_SOURCE_OPTION_BOUNDS;
                native.bounds = MapStructs.ToNative(bounds);
            }
            if (options.TileSize is { } tileSize)
            {
                native.fields |= (uint)
                    mln_style_tile_source_option_field.MLN_STYLE_TILE_SOURCE_OPTION_TILE_SIZE;
                native.tile_size = tileSize;
            }
            if (options.VectorEncoding is { } vectorEncoding)
            {
                native.fields |= (uint)
                    mln_style_tile_source_option_field.MLN_STYLE_TILE_SOURCE_OPTION_VECTOR_ENCODING;
                native.vector_encoding = (uint)vectorEncoding;
            }
            if (options.RasterEncoding is { } rasterEncoding)
            {
                native.fields |= (uint)
                    mln_style_tile_source_option_field.MLN_STYLE_TILE_SOURCE_OPTION_RASTER_ENCODING;
                native.raster_encoding = (uint)rasterEncoding;
            }

            return new NativeTileSourceOptions(native, attribution);
        }
        catch
        {
            attribution?.Dispose();
            throw;
        }
    }

    public void Dispose()
    {
        attribution?.Dispose();
    }
}

internal sealed unsafe class NativeGeoJsonSourceOptions : IDisposable
{
    private readonly NativeJsonValue? clusterProperties;

    private NativeGeoJsonSourceOptions(
        mln_geojson_source_options value,
        NativeJsonValue? clusterProperties
    )
    {
        Value = value;
        this.clusterProperties = clusterProperties;
    }

    internal mln_geojson_source_options Value { get; }

    internal static NativeGeoJsonSourceOptions From(GeoJsonSourceOptions options)
    {
        ArgumentNullException.ThrowIfNull(options);
        NativeJsonValue? clusterProperties = null;
        try
        {
            var native = NativeMethods.mln_geojson_source_options_default();
            if (options.MinimumZoom is { } minimumZoom)
            {
                native.fields |= (uint)
                    mln_geojson_source_option_field.MLN_GEOJSON_SOURCE_OPTION_MIN_ZOOM;
                native.min_zoom = minimumZoom;
            }
            if (options.MaximumZoom is { } maximumZoom)
            {
                native.fields |= (uint)
                    mln_geojson_source_option_field.MLN_GEOJSON_SOURCE_OPTION_MAX_ZOOM;
                native.max_zoom = maximumZoom;
            }
            if (options.Tolerance is { } tolerance)
            {
                native.fields |= (uint)
                    mln_geojson_source_option_field.MLN_GEOJSON_SOURCE_OPTION_TOLERANCE;
                native.tolerance = tolerance;
            }
            if (options.ClusterMaximumZoom is { } clusterMaximumZoom)
            {
                native.fields |= (uint)
                    mln_geojson_source_option_field.MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM;
                native.cluster_max_zoom = clusterMaximumZoom;
            }
            if (options.ClusterProperties is { } clusterPropertiesValue)
            {
                native.fields |= (uint)
                    mln_geojson_source_option_field.MLN_GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES;
                clusterProperties = NativeJsonValue.From(clusterPropertiesValue);
                native.cluster_properties = clusterProperties.Pointer;
            }
            if (options.TileSize is { } tileSize)
            {
                native.fields |= (uint)
                    mln_geojson_source_option_field.MLN_GEOJSON_SOURCE_OPTION_TILE_SIZE;
                native.tile_size = tileSize;
            }
            if (options.Buffer is { } buffer)
            {
                native.fields |= (uint)
                    mln_geojson_source_option_field.MLN_GEOJSON_SOURCE_OPTION_BUFFER;
                native.buffer = buffer;
            }
            if (options.ClusterRadius is { } clusterRadius)
            {
                native.fields |= (uint)
                    mln_geojson_source_option_field.MLN_GEOJSON_SOURCE_OPTION_CLUSTER_RADIUS;
                native.cluster_radius = clusterRadius;
            }
            if (options.ClusterMinimumPoints is { } clusterMinimumPoints)
            {
                native.fields |= (uint)
                    mln_geojson_source_option_field.MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MIN_POINTS;
                native.cluster_min_points = clusterMinimumPoints;
            }
            if (options.LineMetrics is { } lineMetrics)
            {
                native.fields |= (uint)
                    mln_geojson_source_option_field.MLN_GEOJSON_SOURCE_OPTION_LINE_METRICS;
                native.line_metrics = lineMetrics ? (byte)1 : (byte)0;
            }
            if (options.Cluster is { } cluster)
            {
                native.fields |= (uint)
                    mln_geojson_source_option_field.MLN_GEOJSON_SOURCE_OPTION_CLUSTER;
                native.cluster = cluster ? (byte)1 : (byte)0;
            }
            if (options.SynchronousUpdate is { } synchronousUpdate)
            {
                native.fields |= (uint)
                    mln_geojson_source_option_field.MLN_GEOJSON_SOURCE_OPTION_SYNCHRONOUS_UPDATE;
                native.synchronous_update = synchronousUpdate ? (byte)1 : (byte)0;
            }

            return new NativeGeoJsonSourceOptions(native, clusterProperties);
        }
        catch
        {
            clusterProperties?.Dispose();
            throw;
        }
    }

    public void Dispose()
    {
        clusterProperties?.Dispose();
    }
}

internal sealed unsafe class NativeStyleImage : IDisposable
{
    private readonly nint pixels;

    private NativeStyleImage(mln_premultiplied_rgba8_image value, nint pixels)
    {
        Value = value;
        this.pixels = pixels;
    }

    internal mln_premultiplied_rgba8_image Value { get; }

    internal static NativeStyleImage From(PremultipliedRgba8Image image)
    {
        ArgumentNullException.ThrowIfNull(image);
        var bytes = image.Bytes ?? [];
        var pixels = bytes.Length == 0 ? 0 : (nint)NativeMemory.Alloc((nuint)bytes.Length);
        try
        {
            if (pixels != 0)
            {
                Marshal.Copy(bytes, 0, pixels, bytes.Length);
            }

            var info = image.Info;
            var native = NativeMethods.mln_premultiplied_rgba8_image_default();
            native.width = info.Width;
            native.height = info.Height;
            native.stride = info.Stride;
            native.byte_length = (nuint)bytes.Length;
            native.pixels = (byte*)pixels;
            var result = new NativeStyleImage(native, pixels);
            pixels = 0;
            return result;
        }
        finally
        {
            if (pixels != 0)
            {
                NativeMemory.Free((void*)pixels);
            }
        }
    }

    public void Dispose()
    {
        if (pixels != 0)
        {
            NativeMemory.Free((void*)pixels);
        }
    }
}

internal static class StyleStructs
{
    internal static mln_style_transition_options ToNative(StyleTransitionOptions options)
    {
        ArgumentNullException.ThrowIfNull(options);
        var native = NativeMethods.mln_style_transition_options_default();
        native.enable_placement_transitions = (byte)(options.EnablePlacementTransitions ? 1 : 0);
        if (options.Duration is { } duration)
        {
            native.fields |= (uint)
                mln_style_transition_option_field.MLN_STYLE_TRANSITION_OPTION_DURATION;
            native.duration_ms = duration;
        }
        if (options.Delay is { } delay)
        {
            native.fields |= (uint)
                mln_style_transition_option_field.MLN_STYLE_TRANSITION_OPTION_DELAY;
            native.delay_ms = delay;
        }
        return native;
    }

    internal static StyleTransitionOptions FromNative(mln_style_transition_options options) =>
        new()
        {
            Duration =
                (
                    options.fields
                    & (uint)mln_style_transition_option_field.MLN_STYLE_TRANSITION_OPTION_DURATION
                ) != 0
                    ? options.duration_ms
                    : null,
            Delay =
                (
                    options.fields
                    & (uint)mln_style_transition_option_field.MLN_STYLE_TRANSITION_OPTION_DELAY
                ) != 0
                    ? options.delay_ms
                    : null,
            EnablePlacementTransitions = options.enable_placement_transitions != 0,
        };

    internal static StyleImageInfo FromNative(mln_style_image_info info) =>
        new(
            info.width,
            info.height,
            info.stride,
            info.byte_length,
            info.pixel_ratio,
            info.sdf != 0,
            info.stretch_x_count,
            info.stretch_y_count,
            info.has_content != 0
                ? new ImageContent(
                    info.content.left,
                    info.content.top,
                    info.content.right,
                    info.content.bottom
                )
                : null,
            info.has_text_fit_width != 0 ? (StyleImageTextFit)info.text_fit_width : null,
            info.has_text_fit_height != 0 ? (StyleImageTextFit)info.text_fit_height : null
        );

    internal static mln_canonical_tile_id ToNative(CanonicalTileId tileId) =>
        new()
        {
            z = tileId.Z,
            x = tileId.X,
            y = tileId.Y,
        };
}

/// <summary>
/// Holds native style image options plus the stretch storage the C API borrows for the call.
/// </summary>
internal sealed unsafe class NativeStyleImageOptions : IDisposable
{
    private readonly mln_image_stretch* stretchX;
    private readonly mln_image_stretch* stretchY;

    private NativeStyleImageOptions(
        mln_style_image_options value,
        mln_image_stretch* stretchX,
        mln_image_stretch* stretchY
    )
    {
        Value = value;
        this.stretchX = stretchX;
        this.stretchY = stretchY;
    }

    internal mln_style_image_options Value;

    internal static NativeStyleImageOptions From(StyleImageOptions options)
    {
        ArgumentNullException.ThrowIfNull(options);
        mln_image_stretch* stretchX = null;
        mln_image_stretch* stretchY = null;
        try
        {
            var native = NativeMethods.mln_style_image_options_default();
            if (options.PixelRatio is { } pixelRatio)
            {
                native.fields |= (uint)
                    mln_style_image_option_field.MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO;
                native.pixel_ratio = pixelRatio;
            }
            if (options.Sdf is { } sdf)
            {
                native.fields |= (uint)mln_style_image_option_field.MLN_STYLE_IMAGE_OPTION_SDF;
                native.sdf = sdf ? (byte)1 : (byte)0;
            }
            if (options.StretchX is { } valuesX)
            {
                native.fields |= (uint)
                    mln_style_image_option_field.MLN_STYLE_IMAGE_OPTION_STRETCH_X;
                stretchX = Allocate(valuesX);
                native.stretch_x = stretchX;
                native.stretch_x_count = (nuint)valuesX.Count;
            }
            if (options.StretchY is { } valuesY)
            {
                native.fields |= (uint)
                    mln_style_image_option_field.MLN_STYLE_IMAGE_OPTION_STRETCH_Y;
                stretchY = Allocate(valuesY);
                native.stretch_y = stretchY;
                native.stretch_y_count = (nuint)valuesY.Count;
            }
            if (options.Content is { } content)
            {
                native.fields |= (uint)mln_style_image_option_field.MLN_STYLE_IMAGE_OPTION_CONTENT;
                native.content = new mln_image_content
                {
                    left = content.Left,
                    top = content.Top,
                    right = content.Right,
                    bottom = content.Bottom,
                };
            }
            if (options.TextFitWidth is { } textFitWidth)
            {
                native.fields |= (uint)
                    mln_style_image_option_field.MLN_STYLE_IMAGE_OPTION_TEXT_FIT_WIDTH;
                native.text_fit_width = (uint)textFitWidth;
            }
            if (options.TextFitHeight is { } textFitHeight)
            {
                native.fields |= (uint)
                    mln_style_image_option_field.MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT;
                native.text_fit_height = (uint)textFitHeight;
            }
            return new NativeStyleImageOptions(native, stretchX, stretchY);
        }
        catch
        {
            if (stretchX is not null)
            {
                NativeMemory.Free(stretchX);
            }
            if (stretchY is not null)
            {
                NativeMemory.Free(stretchY);
            }
            throw;
        }
    }

    private static mln_image_stretch* Allocate(IReadOnlyList<ImageStretch> stretches)
    {
        if (stretches.Count == 0)
        {
            return null;
        }
        var array = (mln_image_stretch*)
            NativeMemory.Alloc((nuint)stretches.Count, (nuint)sizeof(mln_image_stretch));
        for (var index = 0; index < stretches.Count; index += 1)
        {
            array[index].from = stretches[index].From;
            array[index].to = stretches[index].To;
        }
        return array;
    }

    public void Dispose()
    {
        if (stretchX is not null)
        {
            NativeMemory.Free(stretchX);
        }
        if (stretchY is not null)
        {
            NativeMemory.Free(stretchY);
        }
    }
}

internal sealed unsafe class NativeStringViewArray : IDisposable
{
    private readonly NativeStringView[] values;
    private readonly nint array;

    private NativeStringViewArray(NativeStringView[] values, nint array)
    {
        this.values = values;
        this.array = array;
    }

    internal mln_string_view* Pointer => (mln_string_view*)array;
    internal nuint Count => (nuint)values.Length;

    internal static NativeStringViewArray From(IReadOnlyList<string> strings, string parameterName)
    {
        ArgumentNullException.ThrowIfNull(strings, parameterName);
        var views = new NativeStringView[strings.Count];
        var array = (nint)NativeAllocation.AllocZeroedArray<mln_string_view>(strings.Count);
        try
        {
            var pointer = (mln_string_view*)array;
            for (var index = 0; index < strings.Count; index++)
            {
                views[index] = NativeStringView.From(strings[index], $"{parameterName}[{index}]");
                pointer[index] = views[index].Value;
            }

            return new NativeStringViewArray(views, array);
        }
        catch
        {
            foreach (var view in views)
            {
                view?.Dispose();
            }
            if (array != 0)
            {
                NativeMemory.Free((void*)array);
            }
            throw;
        }
    }

    public void Dispose()
    {
        foreach (var view in values)
        {
            view.Dispose();
        }
        if (array != 0)
        {
            NativeMemory.Free((void*)array);
        }
    }
}
