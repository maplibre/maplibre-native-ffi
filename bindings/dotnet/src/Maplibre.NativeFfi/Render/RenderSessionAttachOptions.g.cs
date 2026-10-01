// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
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
