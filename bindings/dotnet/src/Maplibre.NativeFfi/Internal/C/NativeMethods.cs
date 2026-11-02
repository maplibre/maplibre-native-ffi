using System.Runtime.CompilerServices;

// Every declaration in NativeMethods.g.cs and NativeTypes.g.cs is blittable, so
// source-generated imports pass values and pointers through unchanged.
[assembly: DisableRuntimeMarshalling]

namespace Maplibre.NativeFfi.Internal.C;

internal static unsafe partial class NativeMethods
{
    internal const string LibraryName = "maplibre-native-c";
}

internal interface IMlnHandle
{
    ulong Value { get; }
}
