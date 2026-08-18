using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Memory;
using Maplibre.NativeFfi.Internal.Struct;
using Maplibre.NativeFfi.Render;
using Maplibre.NativeFfi.Runtime;
using Maplibre.NativeFfi.Style;

namespace Maplibre.NativeFfi.Map;

public sealed unsafe partial class MapHandle
{
    public Task<SourceInfo?> StyleSourceInfoAsync(
        string sourceId,
        CancellationToken cancellationToken = default
    )
    {
        using var source = NativeStringView.From(sourceId, nameof(sourceId));
        return NativeCompletion
            .Submit(
                completion =>
                    NativeMethods.mln_map_get_style_source_info(Handle, source.Value, completion),
                result => ReadStyleSource(sourceId, result)
            )
            .WaitAsync(cancellationToken);
    }

    public Task<LayerInfo?> StyleLayerInfoAsync(
        string layerId,
        CancellationToken cancellationToken = default
    )
    {
        using var layer = NativeStringView.From(layerId, nameof(layerId));
        return NativeCompletion
            .Submit(
                completion =>
                    NativeMethods.mln_map_get_style_layer_info(Handle, layer.Value, completion),
                result => ReadStyleLayer(layerId, result)
            )
            .WaitAsync(cancellationToken);
    }

    public Task<StyleImage?> StyleImageAsync(
        string imageId,
        CancellationToken cancellationToken = default
    )
    {
        using var image = NativeStringView.From(imageId, nameof(imageId));
        return NativeCompletion
            .Submit(
                completion =>
                    NativeMethods.mln_map_get_style_image_info(Handle, image.Value, completion),
                ReadStyleImage
            )
            .WaitAsync(cancellationToken);
    }

    private static SourceInfo? ReadStyleSource(string sourceId, mln_completion_result* completion)
    {
        if (completion->value_count == 0)
        {
            return null;
        }
        var value = NativeCompletion.Value<mln_style_source_result>(completion);
        var info = value.info;
        var fields = (mln_style_source_info_field)info.fields;
        string? attribution =
            info.has_attribution != 0
                ? RuntimeStructs.CopyUtf8((sbyte*)value.attribution.data, value.attribution.size)
                : null;
        string? url = fields.HasFlag(mln_style_source_info_field.MLN_STYLE_SOURCE_INFO_URL)
            ? RuntimeStructs.CopyUtf8((sbyte*)value.url.data, value.url.size)
            : null;
        TileJson? tileJson = null;
        if (fields.HasFlag(mln_style_source_info_field.MLN_STYLE_SOURCE_INFO_TILEJSON))
        {
            var urls = new string[checked((int)value.tile_url_count)];
            for (nuint index = 0; index < value.tile_url_count; index++)
            {
                var view = value.tile_urls[index];
                urls[(int)index] = RuntimeStructs.CopyUtf8((sbyte*)view.data, view.size);
            }
            tileJson = new TileJson(
                urls,
                info.min_zoom,
                info.max_zoom,
                (TileScheme)info.scheme,
                info.scheme,
                fields.HasFlag(mln_style_source_info_field.MLN_STYLE_SOURCE_INFO_BOUNDS)
                    ? MapStructs.FromNative(info.bounds)
                    : null
            );
        }
        return new SourceInfo(
            sourceId,
            (SourceType)info.type,
            info.type,
            info.is_volatile != 0,
            attribution,
            url,
            tileJson,
            fields.HasFlag(mln_style_source_info_field.MLN_STYLE_SOURCE_INFO_TILE_SIZE)
                ? info.tile_size
                : null,
            fields.HasFlag(mln_style_source_info_field.MLN_STYLE_SOURCE_INFO_VECTOR_ENCODING)
                ? (VectorTileEncoding)info.vector_encoding
                : null,
            fields.HasFlag(mln_style_source_info_field.MLN_STYLE_SOURCE_INFO_VECTOR_ENCODING)
                ? info.vector_encoding
                : null,
            fields.HasFlag(mln_style_source_info_field.MLN_STYLE_SOURCE_INFO_RASTER_ENCODING)
                ? (RasterDemEncoding)info.raster_encoding
                : null,
            fields.HasFlag(mln_style_source_info_field.MLN_STYLE_SOURCE_INFO_RASTER_ENCODING)
                ? info.raster_encoding
                : null
        );
    }

    private static LayerInfo? ReadStyleLayer(string layerId, mln_completion_result* completion)
    {
        if (completion->value_count == 0)
        {
            return null;
        }
        var value = NativeCompletion.Value<mln_style_layer_result>(completion);
        var info = value.info;
        var fields = (mln_style_layer_info_field)info.fields;
        return new LayerInfo(
            layerId,
            RuntimeStructs.CopyUtf8((sbyte*)info.type.data, info.type.size),
            info.min_zoom,
            info.max_zoom,
            (StyleLayerVisibility)info.visibility,
            info.visibility,
            fields.HasFlag(mln_style_layer_info_field.MLN_STYLE_LAYER_INFO_SOURCE_ID)
                ? RuntimeStructs.CopyUtf8((sbyte*)value.source_id.data, value.source_id.size)
                : null,
            fields.HasFlag(mln_style_layer_info_field.MLN_STYLE_LAYER_INFO_SOURCE_LAYER)
                ? RuntimeStructs.CopyUtf8((sbyte*)value.source_layer.data, value.source_layer.size)
                : null
        );
    }

    private static StyleImage? ReadStyleImage(mln_completion_result* completion)
    {
        if (completion->value_count == 0)
        {
            return null;
        }
        var value = NativeCompletion.Value<mln_style_image_result>(completion);
        var info = StyleStructs.FromNative(value.info);
        var pixels = ValueStructs.CopyBufferView(value.pixels);
        return new StyleImage(
            new PremultipliedRgba8Image(
                pixels,
                new TextureImageInfo(info.Width, info.Height, info.Stride, (ulong)pixels.Length)
            ),
            new StyleImageOptions
            {
                PixelRatio = info.PixelRatio,
                Sdf = info.Sdf,
                StretchX = CopyStretches(value.stretch_x, value.stretch_x_count),
                StretchY = CopyStretches(value.stretch_y, value.stretch_y_count),
                Content = info.Content,
                TextFitWidth = info.TextFitWidth,
                TextFitHeight = info.TextFitHeight,
            }
        );
    }

    private static IReadOnlyList<ImageStretch>? CopyStretches(
        mln_image_stretch* values,
        nuint count
    )
    {
        if (count == 0)
        {
            return null;
        }
        var copied = new ImageStretch[checked((int)count)];
        for (nuint index = 0; index < count; index++)
        {
            copied[(int)index] = new ImageStretch(values[index].from, values[index].to);
        }
        return copied;
    }
}
