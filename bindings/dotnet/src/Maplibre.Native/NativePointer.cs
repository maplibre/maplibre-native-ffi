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
public readonly struct NativePointer : IEquatable<NativePointer>
{
    private NativePointer(nint address)
    {
        Address = address;
    }

    /// <summary>
    /// Creates a borrowed backend-native pointer value from a host-owned address.
    /// </summary>
    /// <remarks>
    /// The caller keeps the backend object alive and synchronized for the full
    /// borrow window documented by the API receiving this value.
    /// </remarks>
    public static NativePointer FromBorrowedAddress(nint address) => new(address);

    internal static NativePointer FromNativeAddress(nint address) => new(address);

    /// <summary>A null native pointer.</summary>
    public static NativePointer Null { get; } = default;

    /// <summary>The borrowed opaque address value.</summary>
    public nint Address { get; }

    /// <summary>Whether the address is zero.</summary>
    public bool IsNull => Address == 0;

    public bool Equals(NativePointer other) => Address == other.Address;

    public override bool Equals(object? obj) => obj is NativePointer other && Equals(other);

    public override int GetHashCode() => Address.GetHashCode();

    public static bool operator ==(NativePointer left, NativePointer right) => left.Equals(right);

    public static bool operator !=(NativePointer left, NativePointer right) => !left.Equals(right);

    public override string ToString() => $"0x{Address:x}";
}
