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

public readonly record struct PremultipliedRgba8Image
{
    public PremultipliedRgba8Image(uint Width, uint Height, uint Stride, byte[] Pixels)
        : this(Width, Height, Stride, Pixels, false) { }

    internal PremultipliedRgba8Image(
        uint Width,
        uint Height,
        uint Stride,
        byte[] Pixels,
        bool adopt
    )
    {
        this.Width = Width;
        this.Height = Height;
        this.Stride = Stride;
        this.storagePixels = adopt ? Pixels : Pixels?.ToArray() ?? [];
    }

    public uint Width { get; init; }
    public uint Height { get; init; }
    public uint Stride { get; init; }
    private readonly byte[]? storagePixels;
    public byte[] Pixels
    {
        get => storagePixels?.ToArray() ?? [];
        init => storagePixels = value?.ToArray() ?? [];
    }
    internal byte[] PixelsStorage
    {
        get => storagePixels ?? [];
        init => storagePixels = value;
    }

    public bool Equals(PremultipliedRgba8Image other) =>
        EqualityComparer<uint>.Default.Equals(Width, other.Width)
        && EqualityComparer<uint>.Default.Equals(Height, other.Height)
        && EqualityComparer<uint>.Default.Equals(Stride, other.Stride)
        && global::Maplibre.NativeFfi.Internal.ValueEquality.SequenceEquals(
            PixelsStorage,
            other.PixelsStorage
        );

    public override int GetHashCode()
    {
        var hash = new HashCode();
        hash.Add(Width);
        hash.Add(Height);
        hash.Add(Stride);
        hash.Add(global::Maplibre.NativeFfi.Internal.ValueEquality.SequenceHashCode(PixelsStorage));
        return hash.ToHashCode();
    }

    public static PremultipliedRgba8Image Default
    {
        get
        {
            global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
                null,
                "mln_premultiplied_rgba8_image_default"
            );
            global::Maplibre.NativeFfi.Internal.Loader.NativeLibraryLoader.EnsureLoaded();
            return global::Maplibre.NativeFfi.Internal.Struct.GeneratedValues.CopyPremultipliedRgba8Image(
                global::Maplibre.NativeFfi.Internal.C.NativeMethods.mln_premultiplied_rgba8_image_default()
            );
        }
    }
}
