// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A resource provider, for <c>RuntimeHandle.SetResourceProviderAsync()</c>.
/// </summary>
/// <remarks>
/// See <c>mln_resource_provider</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct ResourceProvider
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public ResourceProvider(ResourceProviderCallback Callback, ImmutableArray<ResourceRoute> Routes)
    {
        this.Callback = Callback;
        this.Routes = Routes;
    }

    public required ResourceProviderCallback Callback { get; init; }

    /// <summary>
    /// The requests that the provider serves. Without routes, it serves every
    /// network request.
    /// </summary>
    public ImmutableArray<ResourceRoute> Routes
    {
        get => RoutesStorage.AsImmutable();
        init => RoutesStorage = ValueArray.From(value);
    }
    internal ValueArray<ResourceRoute> RoutesStorage { get; init; }
}
