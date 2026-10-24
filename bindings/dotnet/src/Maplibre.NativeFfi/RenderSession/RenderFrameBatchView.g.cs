// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A borrowed view of one owned frame-result batch.
/// </summary>
/// <remarks>
/// See <c>mln_render_frame_batch_view</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.
/// </remarks>
public readonly record struct RenderFrameBatchView
{
    public RenderFrameBatchView(ImmutableArray<RenderFrameResult> Results)
    {
        this.Results = Results;
    }

    /// <summary>
    /// Borrowed array of result_count terminal frame results in completion
    /// order.
    /// </summary>
    public ImmutableArray<RenderFrameResult> Results
    {
        get => ResultsStorage.AsImmutable();
        init => ResultsStorage = ValueArray.From(value);
    }
    internal ValueArray<RenderFrameResult> ResultsStorage { get; init; }
}
