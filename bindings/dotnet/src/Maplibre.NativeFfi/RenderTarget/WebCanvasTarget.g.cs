// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A canvas to present to.
/// </summary>
/// <remarks>
/// <para>The session makes the canvas's "webgpu" or "webgl2" context with the
/// device's backend, and sets the canvas's size to <c>logical_size</c> at the
/// map's scale factor. Attached without a device, it renders with the build's
/// backend: a WebGPU build asks the browser for a device, and attaches once it
/// has one. For an OpenGL device that wraps your WebGL2 context, the canvas
/// must be the context's own.</para>
/// <para>See <c>mln_web_canvas_target</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.</para>
/// </remarks>
public readonly partial record struct WebCanvasTarget
{
    public WebCanvasTarget() { }

    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public WebCanvasTarget(LogicalSize LogicalSize, uint Canvas, string Format)
    {
        this.LogicalSize = LogicalSize;
        this.Canvas = Canvas;
        this.Format = Format;
    }

    public required LogicalSize LogicalSize { get; init; }

    /// <summary>
    /// An <c>OffscreenCanvas</c>, such as one from
    /// <c>HTMLCanvasElement.transferControlToOffscreen()</c> that you posted to
    /// the library's Web Worker, or an <c>HTMLCanvasElement</c> when the
    /// library runs on the page's thread.
    /// </summary>
    public required uint Canvas { get; init; }

    /// <summary>
    /// For a WebGPU device, the <c>GPUTextureFormat</c> to configure the canvas
    /// with, such as <c>bgra8unorm</c>. Empty for a format that the device's
    /// adapter prefers.
    /// </summary>
    public string Format { get; init; } = "";
}
