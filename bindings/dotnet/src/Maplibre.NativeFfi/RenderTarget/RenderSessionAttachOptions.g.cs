// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Options for attaching a render session to a map.
/// </summary>
/// <remarks>
/// See <c>mln_render_session_attach_options</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct RenderSessionAttachOptions
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public RenderSessionAttachOptions(
        RenderDriverKind Driver,
        GpuSyncKindFlag ProducerSyncKinds,
        Wake FrameWake,
        Wake RedrawWake,
        Wake DriverWorkWake,
        bool KeepDepth
    )
    {
        this.Driver = Driver;
        this.ProducerSyncKinds = ProducerSyncKinds;
        this.FrameWake = FrameWake;
        this.RedrawWake = RedrawWake;
        this.DriverWorkWake = DriverWorkWake;
        this.KeepDepth = KeepDepth;
    }

    /// <summary>
    /// The thread that runs the session's graphics work, which depends on the
    /// target:
    /// </summary>
    /// <remarks>
    /// <list type="bullet">
    /// <item><description><c>RenderDriverKind.CallerGraphicsThread</c> for a
    /// borrowed ring of WebGPU textures.</description></item>
    /// <item><description><c>RenderDriverKind.CoreWorker</c> for an offscreen
    /// target, a window, or an exported ring.</description></item>
    /// <item><description>Either one for a web canvas. In a browser both run on
    /// the thread that loaded the library: <c>RenderDriverKind.CoreWorker</c>
    /// in the library's own tasks, and
    /// <c>RenderDriverKind.CallerGraphicsThread</c> when you service the
    /// session.</description></item>
    /// <item><description>Either one for a Metal layer, a Vulkan surface, an
    /// OpenGL surface, or a borrowed ring of Metal, Vulkan, or OpenGL textures.
    /// A build may drive these on <c>RenderDriverKind.CoreWorker</c> only, and
    /// fails an attach with <c>RenderDriverKind.CallerGraphicsThread</c>
    /// then.</description></item>
    /// </list>
    /// <para>An attach with another driver fails with
    /// <c>Status.Unsupported</c>.</para>
    /// </remarks>
    public required RenderDriverKind Driver { get; init; }

    /// <summary>
    /// The kinds of <c>GpuSync</c> that you can wait on before you read an
    /// acquired frame. The session uses one of them that its target supports.
    /// Zero lets the session pick a kind that names no object to wait on.
    /// </summary>
    public GpuSyncKindFlag ProducerSyncKinds { get; init; }

    /// <summary>
    /// Called when frame results are waiting for
    /// <c>RenderSessionHandle.DrainFrameResults()</c>. It isn't called again
    /// until a drain takes them.
    /// </summary>
    public Wake FrameWake { get; init; }

    /// <summary>
    /// Called when the session has something new to draw. Demand a frame at
    /// your next display refresh. It isn't called again until you demand a
    /// frame, or while the session has no target.
    /// </summary>
    public Wake RedrawWake { get; init; }

    /// <summary>
    /// Called when a session driven by
    /// <c>RenderDriverKind.CallerGraphicsThread</c> has work for
    /// <c>RenderSessionHandle.ServiceDriverWork()</c>. It isn't called again
    /// until a service runs. It's never called on your graphics thread during a
    /// service: the service runs the work that comes while it runs, so you can
    /// call <c>RenderSessionHandle.ServiceDriverWork()</c> from the wake.
    /// </summary>
    public Wake DriverWorkWake { get; init; }

    /// <summary>
    /// Whether an exported ring keeps the depth and stencil of each frame for
    /// you, so that you can draw into the frame depth-tested against the map,
    /// as <c>FrameProjection</c> says. The getter of the device's API, such as
    /// <c>AcquiredFrameHandle.WithVulkanTexture()</c>, gets a frame's depth
    /// texture on the device, and <c>AcquiredFrameHandle.WithOpaqueFd()</c>
    /// shares it across APIs when <c>GpuDeviceCapabilities.DepthExportKinds</c>
    /// has the ring's kind. Keeping depth costs memory bandwidth, most on
    /// tile-based GPUs. A borrowed ring keeps depth in the textures that you
    /// give it instead, and the other targets keep none, so an attach to them
    /// with <c>keep_depth</c> fails with <c>Status.Unsupported</c>.
    /// </summary>
    public bool KeepDepth { get; init; }
}
