namespace Maplibre.Native.Tests;

[AttributeUsage(AttributeTargets.Method, AllowMultiple = false)]
public sealed class BindingSpecTestAttribute(params string[] ids) : Attribute
{
    public IReadOnlyList<string> Ids { get; } = ids;
}

[AttributeUsage(AttributeTargets.Method, AllowMultiple = false)]
public sealed class ExtraBindingTestAttribute(string justification) : Attribute
{
    public string Justification { get; } = justification;
}
