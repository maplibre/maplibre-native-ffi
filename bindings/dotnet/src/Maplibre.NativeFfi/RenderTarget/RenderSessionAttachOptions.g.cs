// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Common attachment policy copied before an attach call returns.
/// </summary>
/// <remarks>
/// See <c>mln_render_session_attach_options</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct RenderSessionAttachOptions(
    RenderDriverKind Driver,
    uint RequestedTextureRingDepth,
    Wake FrameWake,
    Wake DriverWorkWake
)
{
    public RenderSessionAttachOptions()
        : this(RenderDriverKind.CallerGraphicsThread, 1, default!, default!) { }

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
