using System.Runtime.CompilerServices;

// Every declaration in NativeMethods.g.cs and NativeTypes.g.cs is blittable, so
// source-generated imports pass values and pointers through unchanged.
[assembly: DisableRuntimeMarshalling]

namespace Maplibre.NativeFfi.Internal.C;

internal static unsafe partial class NativeMethods
{
    internal const string LibraryName = "maplibre-native-c";
}

/// <summary>The issued id that every generated handle struct carries.</summary>
internal interface IMlnHandle
{
    ulong Value { get; }
}

/// <summary>
/// The diagnostic that every status-returning call takes last, which the
/// generated declarations name but do not declare.
/// </summary>
internal struct mln_diagnostic
{
    public uint size;
    public Message message;

    [InlineArray(4096)]
    public struct Message
    {
        private sbyte element;
    }
}
