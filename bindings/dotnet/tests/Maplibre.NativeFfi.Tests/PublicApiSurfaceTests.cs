using System.Reflection;
using Maplibre.NativeFfi.Style;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed class PublicApiSurfaceTests
{
    [Fact]
    public void NativeImplementationTypesStayOutOfPublicSurface()
    {
        var publicTypes = typeof(Maplibre).Assembly.GetExportedTypes();

        Assert.DoesNotContain(publicTypes, type => type.Namespace?.Contains(".Internal") == true);
        Assert.DoesNotContain(
            publicTypes,
            type => type.Name.StartsWith("mln_", StringComparison.Ordinal)
        );
        Assert.DoesNotContain(publicTypes, type => type.Name == "NativeMethods");
    }

    [Fact]
    public void PublicSurfaceDoesNotExposeRawPointersOrNativeSizedCarriers()
    {
        var violations = new List<string>();
        foreach (var type in typeof(Maplibre).Assembly.GetExportedTypes())
        {
            if (!typeof(Delegate).IsAssignableFrom(type))
            {
                foreach (var constructor in type.GetConstructors())
                {
                    InspectParameters(type, constructor, constructor.GetParameters(), violations);
                }
            }

            foreach (
                var method in type.GetMethods(
                    BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static
                )
            )
            {
                if (
                    !method.IsSpecialName || method.Name.StartsWith("op_", StringComparison.Ordinal)
                )
                {
                    InspectType(type, method, method.ReturnType, "return", violations);
                    InspectParameters(type, method, method.GetParameters(), violations);
                }
            }

            foreach (
                var property in type.GetProperties(
                    BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static
                )
            )
            {
                InspectType(type, property, property.PropertyType, "property", violations);
            }

            foreach (
                var field in type.GetFields(
                    BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static
                )
            )
            {
                InspectType(type, field, field.FieldType, "field", violations);
            }
        }

        Assert.Empty(violations);
    }

    // Raw address conversion is explicitly borrowed backend interop.
    [Fact]
    public void NativePointerUsesBorrowedAddressFactory()
    {
        Assert.Null(
            typeof(NativePointer).GetConstructor(
                BindingFlags.Public | BindingFlags.Instance,
                binder: null,
                [typeof(nint)],
                modifiers: null
            )
        );
        Assert.NotNull(
            typeof(NativePointer).GetMethod(
                nameof(NativePointer.FromBorrowedAddress),
                BindingFlags.Public | BindingFlags.Static,
                binder: null,
                [typeof(nint)],
                modifiers: null
            )
        );
    }

    // Optional cancellation tokens follow the .NET asynchronous API convention. Other default
    // parameter values would create shortcut workflows outside the C API shape.
    [Fact]
    public void PublicSurfaceUsesDefaultsOnlyForCancellationTokens()
    {
        var violations = new List<string>();
        foreach (var type in typeof(Maplibre).Assembly.GetExportedTypes())
        {
            foreach (var constructor in type.GetConstructors())
            {
                InspectDefaultParameters(constructor, constructor.GetParameters(), violations);
            }

            foreach (
                var method in type.GetMethods(
                    BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static
                )
            )
            {
                InspectDefaultParameters(method, method.GetParameters(), violations);
            }
        }

        Assert.Empty(violations);
    }

    // Every wrapper over a C completion hands the caller a task to await, so it takes a
    // cancellation token. A create is the exception: its task carries the only reference to a
    // native handle, and abandoning that task would leak it.
    [Fact]
    public void CompletionWrappersTakeACancellationTokenUnlessTheyProduceAHandle()
    {
        var handleProducing = new[] { "MapCreateAsync", "ProjectionCreateAsync" };
        var consumesTheHandle = new[] { "CloseAsync" };
        var violations = new List<string>();

        foreach (var type in typeof(Maplibre).Assembly.GetExportedTypes())
        {
            foreach (
                var method in type.GetMethods(
                    BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static
                )
            )
            {
                if (
                    method.DeclaringType != type
                    || method.IsSpecialName
                    || !typeof(Task).IsAssignableFrom(method.ReturnType)
                )
                {
                    continue;
                }

                var takesToken = method
                    .GetParameters()
                    .Any(parameter => parameter.ParameterType == typeof(CancellationToken));
                if (
                    handleProducing.Contains(method.Name) || consumesTheHandle.Contains(method.Name)
                )
                {
                    if (takesToken)
                    {
                        violations.Add($"{type.FullName}.{method.Name} takes a token");
                    }
                    continue;
                }

                if (!takesToken)
                {
                    violations.Add($"{type.FullName}.{method.Name} takes no token");
                }
            }
        }

        Assert.Empty(violations);
    }

    [Fact]
    public void OwnedNativeHandlesDoNotExposePublicConstructors()
    {
        var assembly = typeof(Maplibre).Assembly;
        var ownedHandleTypeNames = new[]
        {
            "Maplibre.NativeFfi.Map.MapHandle",
            "Maplibre.NativeFfi.Map.MapProjectionHandle",
            "Maplibre.NativeFfi.Render.AcquiredFrameHandle",
            "Maplibre.NativeFfi.Render.RenderSessionHandle",
            "Maplibre.NativeFfi.Runtime.ResourceRequestHandle",
            "Maplibre.NativeFfi.Runtime.RuntimeHandle",
        };

        var violations = ownedHandleTypeNames
            .Select(name => assembly.GetType(name, throwOnError: true)!)
            .SelectMany(type =>
                type.GetConstructors(BindingFlags.Public | BindingFlags.Instance)
                    .Select(constructor => $"{type.FullName}.{constructor}")
            )
            .ToArray();

        Assert.Empty(violations);
    }

    private static void InspectParameters(
        Type declaringType,
        MemberInfo member,
        IEnumerable<ParameterInfo> parameters,
        List<string> violations
    )
    {
        foreach (var parameter in parameters)
        {
            InspectType(
                declaringType,
                member,
                parameter.ParameterType,
                parameter.Name ?? "parameter",
                violations
            );
        }
    }

    private static void InspectDefaultParameters(
        MemberInfo member,
        IEnumerable<ParameterInfo> parameters,
        List<string> violations
    )
    {
        foreach (var parameter in parameters)
        {
            if (
                (parameter.HasDefaultValue || parameter.IsOptional)
                && parameter.ParameterType != typeof(CancellationToken)
            )
            {
                violations.Add(
                    $"{member.DeclaringType?.FullName}.{member.Name} has default parameter {parameter.Name}."
                );
            }
        }
    }

    private static void InspectType(
        Type declaringType,
        MemberInfo member,
        Type type,
        string role,
        List<string> violations
    )
    {
        foreach (var exposedType in Flatten(type))
        {
            if (IsAllowedNativePointerCarrier(declaringType, member, exposedType))
            {
                continue;
            }

            if (exposedType.IsPointer)
            {
                violations.Add(
                    $"{member.DeclaringType?.FullName}.{member.Name} exposes pointer {role} {exposedType}."
                );
            }

            if (exposedType == typeof(nint) || exposedType == typeof(nuint))
            {
                violations.Add(
                    $"{member.DeclaringType?.FullName}.{member.Name} exposes native-sized {role} {exposedType.Name}."
                );
            }
        }
    }

    private static bool IsAllowedNativePointerCarrier(
        Type declaringType,
        MemberInfo member,
        Type exposedType
    )
    {
        if (declaringType != typeof(NativePointer))
        {
            return false;
        }

        if (member is PropertyInfo { Name: nameof(NativePointer.Address), CanWrite: false })
        {
            return exposedType == typeof(nint);
        }

        if (
            member
                is MethodInfo
                {
                    Name: nameof(NativePointer.FromBorrowedAddress),
                    IsStatic: true,
                    ReturnType: var returnType,
                } method
            && returnType == typeof(NativePointer)
        )
        {
            var parameters = method.GetParameters();
            return exposedType == typeof(nint)
                && parameters.Length == 1
                && parameters[0].ParameterType == typeof(nint);
        }

        return false;
    }

    private static IEnumerable<Type> Flatten(Type type)
    {
        if (type.HasElementType)
        {
            foreach (var nested in Flatten(type.GetElementType()!))
            {
                yield return nested;
            }
        }

        yield return type;

        if (!type.IsGenericType)
        {
            yield break;
        }

        foreach (var argument in type.GetGenericArguments())
        {
            foreach (var nested in Flatten(argument))
            {
                yield return nested;
            }
        }
    }
}
