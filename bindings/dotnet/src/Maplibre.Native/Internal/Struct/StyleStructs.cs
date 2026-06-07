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
    internal static mln_style_image_options ToNative(StyleImageOptions options)
    {
        ArgumentNullException.ThrowIfNull(options);
        var native = NativeMethods.mln_style_image_options_default();
        if (options.PixelRatio is { } pixelRatio)
        {
            native.fields |= (uint)mln_style_image_option_field.MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO;
            native.pixel_ratio = pixelRatio;
        }
        if (options.Sdf is { } sdf)
        {
            native.fields |= (uint)mln_style_image_option_field.MLN_STYLE_IMAGE_OPTION_SDF;
            native.sdf = sdf ? (byte)1 : (byte)0;
        }
        return native;
    }

    internal static StyleImageInfo FromNative(mln_style_image_info info) =>
        new(
            info.width,
            info.height,
            info.stride,
            info.byte_length,
            info.pixel_ratio,
            info.sdf != 0
        );

    internal static mln_canonical_tile_id ToNative(CanonicalTileId tileId) =>
        new()
        {
            z = tileId.Z,
            x = tileId.X,
            y = tileId.Y,
        };
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
