// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi.Runtime;

public readonly partial record struct ResourceProvider(
    Func<ResourceRequest, ResourceRequestHandle, ResourceProviderDecision>? Callback
);
