// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Resource transform state: a callback, or native rewrite rules.
/// </summary>
/// <remarks>
/// See <c>mln_resource_transform</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
public readonly record struct ResourceTransform
{
    public ResourceTransform(
        Action<ResourceKind, string, ResourceTransformResponse>? Callback,
        ResourceRewriteRule[] Rules
    )
    {
        this.Callback = Callback;
        this.Rules = Rules;
    }

    /// <summary>
    /// The callback, or null to apply rules.
    /// </summary>
    public Action<ResourceKind, string, ResourceTransformResponse>? Callback { get; init; }

    /// <summary>
    /// Rewrite rules for a transform without a callback; empty with one.
    /// </summary>
    public ResourceRewriteRule[] Rules
    {
        get => RulesStorage.ToArray();
        init => RulesStorage = ValueArray.Copy(value);
    }
    internal ValueArray<ResourceRewriteRule> RulesStorage { get; init; }
}
