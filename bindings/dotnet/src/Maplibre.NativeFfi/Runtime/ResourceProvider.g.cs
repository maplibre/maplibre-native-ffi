// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Resource provider state.
/// </summary>
/// <remarks>
/// See <c>mln_resource_provider</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
public readonly record struct ResourceProvider
{
    public ResourceProvider(
        Func<ResourceRequestInfo, ResourceRequestHandle, ResourceProviderDecision>? Callback,
        ImmutableArray<ResourceRoute> Routes
    )
    {
        this.Callback = Callback;
        this.Routes = Routes;
    }

    /// <summary>
    /// Decides each request that the routes leave to the provider.
    /// </summary>
    public Func<
        ResourceRequestInfo,
        ResourceRequestHandle,
        ResourceProviderDecision
    >? Callback { get; init; }

    /// <summary>
    /// The requests that reach the provider; empty to claim every request.
    /// </summary>
    public ImmutableArray<ResourceRoute> Routes
    {
        get => RoutesStorage.AsImmutable();
        init => RoutesStorage = ValueArray.From(value);
    }
    internal ValueArray<ResourceRoute> RoutesStorage { get; init; }
}
