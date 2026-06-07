namespace Maplibre.Native;

/// <summary>
/// Borrowed opaque backend-native address.
/// </summary>
/// <remarks>
/// <para>
/// This value grants no memory access, transfers no ownership, and is accepted
/// only by APIs whose C contract already accepts opaque host-owned handles.
/// </para>
/// </remarks>
public readonly record struct NativePointer(nint Address)
{
    /// <summary>A null native pointer.</summary>
    public static NativePointer Null { get; } = new(0);

    /// <summary>Whether the address is zero.</summary>
    public bool IsNull => Address == 0;
}
