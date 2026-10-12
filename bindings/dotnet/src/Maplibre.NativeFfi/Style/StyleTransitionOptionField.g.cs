// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// Field mask values for <c>mln_style_transition_options</c>.
/// </summary>
/// <remarks>
/// See <c>mln_style_transition_option_field</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
/// </remarks>
[Flags]
public enum StyleTransitionOptionField : uint
{
    Duration = 1,
    Delay = 2,
    EnablePlacementTransitions = 4,
}
