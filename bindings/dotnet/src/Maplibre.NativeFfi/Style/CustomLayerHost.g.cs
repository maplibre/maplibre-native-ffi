// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// The callbacks of a custom layer, which draws with your own GPU commands in
/// the map's frame.
/// </summary>
/// <remarks>
/// <para>A session runs the callbacks only when it can hand you its commands as
/// one of <c>CustomLayerRenderParameters.PassValue</c>'s variants, so a session
/// on another backend, such as WebGPU outside a browser, doesn't draw the
/// layer. Nor does a browser WebGPU session yet: the library's WebGPU (wgpu)
/// exposes no command encoder or render pass to hand you. Leave unset the
/// callbacks that you don't need.</para>
/// <para>Runs on the thread that runs the session's graphics work: a library
/// thread, or yours with <c>RenderDriverKind.CallerGraphicsThread</c>. It can't
/// call the library.</para>
/// <para>Runs on the thread that runs the session's graphics work, as
/// <c>CustomLayerPrerenderCallback</c> says. It can't call the library.</para>
/// <para>See <c>mln_custom_layer_host</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.</para>
/// </remarks>
public readonly partial record struct CustomLayerHost(
    CustomLayerPrerenderCallback? Prerender,
    CustomLayerRenderCallback? Render,
    CustomLayerSessionEndCallback? Deinitialize,
    CustomLayerSessionEndCallback? ContextLost
)
{
    /// <summary>
    /// Creates a native registration from C functions and a context that the
    /// host supplies. The binding copies them unchanged, so native code calls
    /// each function without entering the .NET runtime.
    /// </summary>
    /// <remarks>
    /// The first call that carries this value or a copy of it consumes the
    /// registration. Native code releases the context of a call that it accepts,
    /// and the binding calls the release for a call that native code does not
    /// accept. A later call that carries the registration throws an
    /// invalid-state <see cref="MaplibreException"/> before it reaches
    /// native code.
    /// </remarks>
    /// <param name="prerender">The C function that native code calls as <c>prerender</c>.</param>
    /// <param name="render">The C function that native code calls as <c>render</c>.</param>
    /// <param name="deinitialize">The C function that native code calls as <c>deinitialize</c>.</param>
    /// <param name="contextLost">The C function that native code calls as <c>context_lost</c>.</param>
    /// <param name="userData">The context that native code passes to each function.</param>
    /// <param name="releaseUserData">The C function that releases <paramref name="userData"/>, or <see cref="NativePointer.Null"/>.</param>
    public static CustomLayerHost Native(
        NativePointer prerender,
        NativePointer render,
        NativePointer deinitialize,
        NativePointer contextLost,
        NativePointer userData,
        NativePointer releaseUserData
    ) =>
        default(CustomLayerHost) with
        {
            NativeRegistration = new(
                [prerender.Address, render.Address, deinitialize.Address, contextLost.Address],
                userData.Address,
                releaseUserData.Address
            ),
        };

    internal NativeRegistration? NativeRegistration { get; init; }
}
