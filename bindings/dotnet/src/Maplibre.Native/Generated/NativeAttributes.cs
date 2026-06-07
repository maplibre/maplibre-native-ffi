using System.Diagnostics;

namespace Maplibre.Native.Internal.C;

/// <summary>Records the native type spelling used in generated C declarations.</summary>
[AttributeUsage(
    AttributeTargets.Struct
        | AttributeTargets.Enum
        | AttributeTargets.Property
        | AttributeTargets.Field
        | AttributeTargets.Parameter
        | AttributeTargets.ReturnValue,
    AllowMultiple = false,
    Inherited = true
)]
[Conditional("DEBUG")]
internal sealed class NativeTypeNameAttribute(string name) : Attribute
{
    /// <summary>The native type spelling.</summary>
    public string Name { get; } = name;
}
