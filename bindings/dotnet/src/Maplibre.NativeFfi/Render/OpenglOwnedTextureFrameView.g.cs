// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using Maplibre.NativeFfi.Base;
using Maplibre.NativeFfi.Internal.Pointer;
using Maplibre.NativeFfi.Logging;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Query;
using Maplibre.NativeFfi.Runtime;
using Maplibre.NativeFfi.Style;

namespace Maplibre.NativeFfi.Render;

public sealed class OpenglOwnedTextureFrameView
{
    private readonly OpenglOwnedTextureFrame value;
    private readonly NativeViewScope scope;

    internal OpenglOwnedTextureFrameView(OpenglOwnedTextureFrame value, NativeViewScope scope)
    {
        this.value = value;
        this.scope = scope;
    }

    public ulong Generation
    {
        get
        {
            scope.EnsureActive();
            return value.Generation;
        }
    }
    public uint Width
    {
        get
        {
            scope.EnsureActive();
            return value.Width;
        }
    }
    public uint Height
    {
        get
        {
            scope.EnsureActive();
            return value.Height;
        }
    }
    public double ScaleFactor
    {
        get
        {
            scope.EnsureActive();
            return value.ScaleFactor;
        }
    }
    public ulong FrameId
    {
        get
        {
            scope.EnsureActive();
            return value.FrameId;
        }
    }
    public uint Texture
    {
        get
        {
            scope.EnsureActive();
            return value.Texture;
        }
    }
    public uint Target
    {
        get
        {
            scope.EnsureActive();
            return value.Target;
        }
    }
    public uint InternalFormat
    {
        get
        {
            scope.EnsureActive();
            return value.InternalFormat;
        }
    }
    public uint Format
    {
        get
        {
            scope.EnsureActive();
            return value.Format;
        }
    }
    public uint Type
    {
        get
        {
            scope.EnsureActive();
            return value.Type;
        }
    }
}
