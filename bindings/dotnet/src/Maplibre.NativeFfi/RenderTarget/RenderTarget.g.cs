// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// What a render session renders into: one kind of target, and its descriptor.
/// </summary>
/// <remarks>
/// <para>Every target has a size in logical pixels, which sets the map's size.
/// The session renders it at the map's scale factor. A borrowed or exported
/// ring hands each frame to you: acquire it with
/// <c>RenderSessionHandle.AcquireFrame()</c>, and release it with your GPU's
/// synchronization once you have read it.</para>
/// <para>The session uses the objects that the descriptor names until a
/// replacement or release of the target completes, a detach or release of the
/// session completes, or an abandon of it returns, except as a kind's
/// descriptor says. A window target presents without waiting for the display,
/// so demand at most one frame per display refresh.</para>
/// <para>See <c>mln_render_target</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.</para>
/// </remarks>
public abstract record RenderTarget
{
    private RenderTarget() { }

    public sealed record Offscreen(OffscreenTarget Value) : RenderTarget;

    public sealed record MetalLayer(MetalLayerTarget Value) : RenderTarget;

    public sealed record Win32Window(Win32WindowTarget Value) : RenderTarget;

    public sealed record XlibWindow(XlibWindowTarget Value) : RenderTarget;

    public sealed record WaylandSurface(WaylandSurfaceTarget Value) : RenderTarget;

    public sealed record AndroidWindow(AndroidWindowTarget Value) : RenderTarget;

    public sealed record OhosWindow(OhosWindowTarget Value) : RenderTarget;

    public sealed record VulkanSurface(VulkanSurfaceTarget Value) : RenderTarget;

    public sealed record OpenGlSurface(OpenGlSurfaceTarget Value) : RenderTarget;

    public sealed record MetalBorrowedTextures(MetalBorrowedTexturesTarget Value) : RenderTarget;

    public sealed record VulkanBorrowedTextures(VulkanBorrowedTexturesTarget Value) : RenderTarget;

    public sealed record OpenGlBorrowedTextures(OpenGlBorrowedTexturesTarget Value) : RenderTarget;

    public sealed record WebGpuBorrowedTextures(WebGpuBorrowedTexturesTarget Value) : RenderTarget;

    public sealed record ExportedRing(ExportedRingTarget Value) : RenderTarget;

    public sealed record WebCanvas(WebCanvasTarget Value) : RenderTarget;
}
