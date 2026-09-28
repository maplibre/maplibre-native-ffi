namespace Maplibre.NativeFfi.Internal.Callback;

/// <summary>
/// A callback whose reentry policy admits calls on the handle that registered it.
/// </summary>
internal sealed record NativeOwnedCallback(object Callback, object Owner);
