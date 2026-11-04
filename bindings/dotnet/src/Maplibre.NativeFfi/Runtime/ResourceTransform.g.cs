// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// How to rewrite request URLs, for
/// <c>RuntimeHandle.SetResourceTransformAsync()</c>.
/// </summary>
/// <remarks>
/// <para>Set either <c>callback</c> or <c>rules</c>, not both.</para>
/// <para>See <c>mln_resource_transform</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.</para>
/// </remarks>
public readonly partial record struct ResourceTransform
{
    public ResourceTransform(
        ResourceTransformCallback? Callback,
        ImmutableArray<ResourceTransformRule> Rules
    )
    {
        this.Callback = Callback;
        this.Rules = Rules;
    }

    public ResourceTransformCallback? Callback { get; init; }

    /// <summary>
    /// The first rule that matches a request adds its parameters, and no other.
    /// A request that no rule matches keeps its URL.
    /// </summary>
    public ImmutableArray<ResourceTransformRule> Rules
    {
        get => RulesStorage.AsImmutable();
        init => RulesStorage = ValueArray.From(value);
    }
    internal ValueArray<ResourceTransformRule> RulesStorage { get; init; }
}
