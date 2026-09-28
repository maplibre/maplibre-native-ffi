namespace Maplibre.NativeFfi.Tests;

[AttributeUsage(AttributeTargets.Method, AllowMultiple = false)]
public sealed class BindingSpecTestAttribute(params string[] ids) : Attribute
{
    public IReadOnlyList<string> Ids { get; } = ids;
}
