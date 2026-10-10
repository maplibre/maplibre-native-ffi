// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

public readonly partial record struct RenderSessionAttachOptions(
    RenderDriverKind Driver,
    uint RequestedTextureRingDepth,
    Wake FrameWake,
    Wake DriverWorkWake,
    QueueLock QueueLock
)
{
    public static RenderSessionAttachOptions Default
    {
        get
        {
            using var call = NativeCall.Enter(null, "mln_render_session_attach_options_default");
            return GeneratedValues.CopyRenderSessionAttachOptions(
                NativeMethods.mln_render_session_attach_options_default()
            );
        }
    }
}
