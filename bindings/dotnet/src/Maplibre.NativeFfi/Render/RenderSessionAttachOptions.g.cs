// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;
using static Maplibre.NativeFfi.Internal.Struct.NativeValues;

namespace Maplibre.NativeFfi.Render;

public readonly partial record struct RenderSessionAttachOptions(
    RenderDriverKind Driver,
    uint RequestedTextureRingDepth,
    Wake FrameWake,
    Wake DriverWorkWake
)
{
    public static RenderSessionAttachOptions Default
    {
        get
        {
            using var call = Enter(null, "mln_render_session_attach_options_default");
            return CopyRenderSessionAttachOptions(
                NativeMethods.mln_render_session_attach_options_default()
            );
        }
    }
}
