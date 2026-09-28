// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using Maplibre.NativeFfi.Base;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Logging;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Query;
using Maplibre.NativeFfi.Render;
using Maplibre.NativeFfi.Runtime;

namespace Maplibre.NativeFfi.Style;

public sealed record StyleImageOptions
{
    private ImageStretch[]? storageStretchX;
    public ImageStretch[]? StretchX
    {
        get => storageStretchX?.ToArray();
        set => storageStretchX = value?.ToArray();
    }
    internal ImageStretch[]? StretchXStorage
    {
        get => storageStretchX;
        init => storageStretchX = value;
    }
    private ImageStretch[]? storageStretchY;
    public ImageStretch[]? StretchY
    {
        get => storageStretchY?.ToArray();
        set => storageStretchY = value?.ToArray();
    }
    internal ImageStretch[]? StretchYStorage
    {
        get => storageStretchY;
        init => storageStretchY = value;
    }
    public ImageContent? Content { get; set; }
    public StyleImageTextFit? TextFitWidth { get; set; }
    public StyleImageTextFit? TextFitHeight { get; set; }
    public float? PixelRatio { get; set; }
    public bool? Sdf { get; set; }

    public bool Equals(StyleImageOptions? other) =>
        other is not null
        && global::Maplibre.NativeFfi.Internal.ValueEquality.SequenceEquals(
            StretchXStorage,
            other.StretchXStorage
        )
        && global::Maplibre.NativeFfi.Internal.ValueEquality.SequenceEquals(
            StretchYStorage,
            other.StretchYStorage
        )
        && EqualityComparer<ImageContent?>.Default.Equals(Content, other.Content)
        && EqualityComparer<StyleImageTextFit?>.Default.Equals(TextFitWidth, other.TextFitWidth)
        && EqualityComparer<StyleImageTextFit?>.Default.Equals(TextFitHeight, other.TextFitHeight)
        && EqualityComparer<float?>.Default.Equals(PixelRatio, other.PixelRatio)
        && EqualityComparer<bool?>.Default.Equals(Sdf, other.Sdf);

    public override int GetHashCode()
    {
        var hash = new HashCode();
        hash.Add(
            global::Maplibre.NativeFfi.Internal.ValueEquality.SequenceHashCode(StretchXStorage)
        );
        hash.Add(
            global::Maplibre.NativeFfi.Internal.ValueEquality.SequenceHashCode(StretchYStorage)
        );
        hash.Add(Content);
        hash.Add(TextFitWidth);
        hash.Add(TextFitHeight);
        hash.Add(PixelRatio);
        hash.Add(Sdf);
        return hash.ToHashCode();
    }

    public static StyleImageOptions Default
    {
        get
        {
            global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
                null,
                "mln_style_image_options_default"
            );
            global::Maplibre.NativeFfi.Internal.Loader.NativeLibraryLoader.EnsureLoaded();
            return global::Maplibre.NativeFfi.Internal.Struct.GeneratedValues.CopyStyleImageOptions(
                global::Maplibre.NativeFfi.Internal.C.NativeMethods.mln_style_image_options_default()
            );
        }
    }
}
