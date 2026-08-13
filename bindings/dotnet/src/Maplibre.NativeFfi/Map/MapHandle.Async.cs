using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Struct;
using Maplibre.NativeFfi.Render;
using Maplibre.NativeFfi.Style;

namespace Maplibre.NativeFfi.Map;

public sealed partial class MapHandle
{
    /// <summary>Gets copied style source metadata when the source exists.</summary>
    public async Task<SourceInfo?> StyleSourceInfoAsync(
        string sourceId,
        CancellationToken cancellationToken = default
    )
    {
        var source = await QueryStyleSourceInfoAsync(sourceId, cancellationToken)
            .ConfigureAwait(false);
        if (!source.Found)
        {
            return null;
        }

        var info = source.Info;
        string? attribution = null;
        if (info.has_attribution != 0)
        {
            attribution = await CopyStyleSourceStringAsync(sourceId, true, cancellationToken)
                .ConfigureAwait(false);
            if (attribution is null)
            {
                return null;
            }
        }

        var fields = (mln_style_source_info_field)info.fields;
        string? url = null;
        if (fields.HasFlag(mln_style_source_info_field.MLN_STYLE_SOURCE_INFO_URL))
        {
            url = await CopyStyleSourceStringAsync(sourceId, false, cancellationToken)
                .ConfigureAwait(false);
            if (url is null)
            {
                return null;
            }
        }

        TileJson? tileJson = null;
        if (fields.HasFlag(mln_style_source_info_field.MLN_STYLE_SOURCE_INFO_TILEJSON))
        {
            var tileUrls = await CopyStyleSourceTileUrlsAsync(sourceId, cancellationToken)
                .ConfigureAwait(false);
            if (tileUrls is null)
            {
                return null;
            }
            tileJson = new TileJson(
                tileUrls,
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

    /// <summary>Copies a style image's stretchable intervals when it exists.</summary>
    public async Task<(
        IReadOnlyList<ImageStretch> StretchX,
        IReadOnlyList<ImageStretch> StretchY
    )?> StyleImageStretchesAsync(string imageId, CancellationToken cancellationToken = default)
    {
        var info = await StyleImageInfoAsync(imageId, cancellationToken).ConfigureAwait(false);
        return info is null
            ? null
            : await TakeStyleImageStretchesAsync(imageId, info, cancellationToken)
                .ConfigureAwait(false);
    }

    /// <summary>Copies a style image as premultiplied RGBA8 pixels when it exists.</summary>
    public async Task<StyleImage?> CopyStyleImagePremultipliedRgba8Async(
        string imageId,
        CancellationToken cancellationToken = default
    )
    {
        var info = await StyleImageInfoAsync(imageId, cancellationToken).ConfigureAwait(false);
        if (info is null)
        {
            return null;
        }
        var pixels = await CopyStyleImagePixelsAsync(imageId, cancellationToken)
            .ConfigureAwait(false);
        if (pixels is null)
        {
            return null;
        }
        var stretches = await StyleImageStretchesAsync(imageId, cancellationToken)
            .ConfigureAwait(false);
        return new StyleImage(
            new PremultipliedRgba8Image(
                pixels,
                new TextureImageInfo(info.Width, info.Height, info.Stride, (ulong)pixels.Length)
            ),
            new StyleImageOptions
            {
                PixelRatio = info.PixelRatio,
                Sdf = info.Sdf,
                StretchX = NullIfEmpty(stretches?.StretchX),
                StretchY = NullIfEmpty(stretches?.StretchY),
                Content = info.Content,
                TextFitWidth = info.TextFitWidth,
                TextFitHeight = info.TextFitHeight,
            }
        );
    }
}
