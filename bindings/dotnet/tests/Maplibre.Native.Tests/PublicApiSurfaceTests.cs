using System.Reflection;
using Xunit;

namespace Maplibre.Native.Tests;

public sealed class PublicApiSurfaceTests
{
    // Support invariant for the Architecture and Minimality requirements: this
    // guards the supported .NET public surface against accidental API expansion.
    [Fact]
    public void ExpectedPublicTypesExist()
    {
        var assembly = typeof(Maplibre).Assembly;
        var expected = new[]
        {
            "Maplibre.Native.Camera.AnimationOptions",
            "Maplibre.Native.Camera.BoundOptions",
            "Maplibre.Native.Camera.CameraFitOptions",
            "Maplibre.Native.Camera.CameraOptions",
            "Maplibre.Native.Camera.EdgeInsets",
            "Maplibre.Native.Camera.FreeCameraOptions",
            "Maplibre.Native.Camera.UnitBezier",
            "Maplibre.Native.Error.InvalidArgumentException",
            "Maplibre.Native.Error.InvalidStateException",
            "Maplibre.Native.Error.MaplibreException",
            "Maplibre.Native.Error.MaplibreStatus",
            "Maplibre.Native.Error.NativeErrorException",
            "Maplibre.Native.Error.UnsupportedFeatureException",
            "Maplibre.Native.Error.WrongThreadException",
            "Maplibre.Native.Geo.CanonicalTileId",
            "Maplibre.Native.Geo.Feature",
            "Maplibre.Native.Geo.FeatureIdentifier",
            "Maplibre.Native.Geo.FeatureIdentifier+Double",
            "Maplibre.Native.Geo.FeatureIdentifier+Int",
            "Maplibre.Native.Geo.FeatureIdentifier+Null",
            "Maplibre.Native.Geo.FeatureIdentifier+String",
            "Maplibre.Native.Geo.FeatureIdentifier+UInt",
            "Maplibre.Native.Geo.GeoJson",
            "Maplibre.Native.Geo.GeoJson+FeatureCollection",
            "Maplibre.Native.Geo.GeoJson+FeatureValue",
            "Maplibre.Native.Geo.GeoJson+GeometryValue",
            "Maplibre.Native.Geo.Geometry",
            "Maplibre.Native.Geo.Geometry+Collection",
            "Maplibre.Native.Geo.Geometry+Empty",
            "Maplibre.Native.Geo.Geometry+LineString",
            "Maplibre.Native.Geo.Geometry+MultiLineString",
            "Maplibre.Native.Geo.Geometry+MultiPoint",
            "Maplibre.Native.Geo.Geometry+MultiPolygon",
            "Maplibre.Native.Geo.Geometry+Point",
            "Maplibre.Native.Geo.Geometry+Polygon",
            "Maplibre.Native.Geo.LatLng",
            "Maplibre.Native.Geo.LatLngBounds",
            "Maplibre.Native.Geo.ProjectedMeters",
            "Maplibre.Native.Geo.Quaternion",
            "Maplibre.Native.Geo.ScreenBox",
            "Maplibre.Native.Geo.ScreenPoint",
            "Maplibre.Native.Geo.TileId",
            "Maplibre.Native.Geo.Vec3",
            "Maplibre.Native.Json.JsonMember",
            "Maplibre.Native.Json.JsonValue",
            "Maplibre.Native.Json.JsonValue+Array",
            "Maplibre.Native.Json.JsonValue+Bool",
            "Maplibre.Native.Json.JsonValue+Double",
            "Maplibre.Native.Json.JsonValue+Int",
            "Maplibre.Native.Json.JsonValue+Null",
            "Maplibre.Native.Json.JsonValue+Object",
            "Maplibre.Native.Json.JsonValue+String",
            "Maplibre.Native.Json.JsonValue+UInt",
            "Maplibre.Native.Log.LogCallback",
            "Maplibre.Native.Log.LogEvent",
            "Maplibre.Native.Log.LogRecord",
            "Maplibre.Native.Log.LogSeverity",
            "Maplibre.Native.Log.LogSeverityMask",
            "Maplibre.Native.Map.ConstrainMode",
            "Maplibre.Native.Map.DebugOptions",
            "Maplibre.Native.Map.MapHandle",
            "Maplibre.Native.Map.MapMode",
            "Maplibre.Native.Map.MapOptions",
            "Maplibre.Native.Map.MapProjectionHandle",
            "Maplibre.Native.Map.NorthOrientation",
            "Maplibre.Native.Map.ProjectionModeOptions",
            "Maplibre.Native.Map.RenderingStats",
            "Maplibre.Native.Map.TileLodMode",
            "Maplibre.Native.Map.TileOperation",
            "Maplibre.Native.Map.TileOptions",
            "Maplibre.Native.Map.ViewportMode",
            "Maplibre.Native.Map.ViewportOptions",
            "Maplibre.Native.Maplibre",
            "Maplibre.Native.NativePointer",
            "Maplibre.Native.NetworkStatus",
            "Maplibre.Native.Offline.OfflineRegionDefinition",
            "Maplibre.Native.Offline.OfflineRegionDefinition+GeometryRegion",
            "Maplibre.Native.Offline.OfflineRegionDefinition+TilePyramid",
            "Maplibre.Native.Offline.OfflineRegionDownloadState",
            "Maplibre.Native.Offline.OfflineRegionInfo",
            "Maplibre.Native.Offline.OfflineRegionStatus",
            "Maplibre.Native.Query.FeatureExtensionResult",
            "Maplibre.Native.Query.FeatureExtensionResult+FeatureCollection",
            "Maplibre.Native.Query.FeatureExtensionResult+Unknown",
            "Maplibre.Native.Query.FeatureExtensionResult+Value",
            "Maplibre.Native.Query.FeatureStateSelector",
            "Maplibre.Native.Query.QueriedFeature",
            "Maplibre.Native.Query.RenderedFeatureQueryOptions",
            "Maplibre.Native.Query.RenderedQueryGeometry",
            "Maplibre.Native.Query.RenderedQueryGeometry+Box",
            "Maplibre.Native.Query.RenderedQueryGeometry+LineString",
            "Maplibre.Native.Query.RenderedQueryGeometry+Point",
            "Maplibre.Native.Query.SourceFeatureQueryOptions",
            "Maplibre.Native.Render.EglContextDescriptor",
            "Maplibre.Native.Render.MetalBorrowedTextureDescriptor",
            "Maplibre.Native.Render.MetalContextDescriptor",
            "Maplibre.Native.Render.MetalOwnedTextureDescriptor",
            "Maplibre.Native.Render.MetalOwnedTextureFrame",
            "Maplibre.Native.Render.MetalOwnedTextureFrameHandle",
            "Maplibre.Native.Render.MetalSurfaceDescriptor",
            "Maplibre.Native.Render.NativeBuffer",
            "Maplibre.Native.Render.OpenGLBorrowedTextureDescriptor",
            "Maplibre.Native.Render.OpenGLContextDescriptor",
            "Maplibre.Native.Render.OpenGLContextProvider",
            "Maplibre.Native.Render.OpenGLOwnedTextureDescriptor",
            "Maplibre.Native.Render.OpenGLOwnedTextureFrame",
            "Maplibre.Native.Render.OpenGLOwnedTextureFrameHandle",
            "Maplibre.Native.Render.OpenGLSurfaceDescriptor",
            "Maplibre.Native.Render.PremultipliedRgba8Image",
            "Maplibre.Native.Render.RenderBackend",
            "Maplibre.Native.Render.RenderMode",
            "Maplibre.Native.Render.RenderSessionHandle",
            "Maplibre.Native.Render.RenderTargetExtent",
            "Maplibre.Native.Render.TextureImageInfo",
            "Maplibre.Native.Render.VulkanBorrowedTextureDescriptor",
            "Maplibre.Native.Render.VulkanContextDescriptor",
            "Maplibre.Native.Render.VulkanOwnedTextureDescriptor",
            "Maplibre.Native.Render.VulkanOwnedTextureFrame",
            "Maplibre.Native.Render.VulkanOwnedTextureFrameHandle",
            "Maplibre.Native.Render.VulkanSurfaceDescriptor",
            "Maplibre.Native.Render.WglContextDescriptor",
            "Maplibre.Native.Resource.ByteRange",
            "Maplibre.Native.Resource.ResourceErrorReason",
            "Maplibre.Native.Resource.ResourceKind",
            "Maplibre.Native.Resource.ResourceLoadingMethod",
            "Maplibre.Native.Resource.ResourcePriority",
            "Maplibre.Native.Resource.ResourceProviderCallback",
            "Maplibre.Native.Resource.ResourceProviderDecision",
            "Maplibre.Native.Resource.ResourceRequest",
            "Maplibre.Native.Resource.ResourceRequestHandle",
            "Maplibre.Native.Resource.ResourceResponse",
            "Maplibre.Native.Resource.ResourceResponseStatus",
            "Maplibre.Native.Resource.ResourceStoragePolicy",
            "Maplibre.Native.Resource.ResourceTransformCallback",
            "Maplibre.Native.Resource.ResourceTransformRequest",
            "Maplibre.Native.Resource.ResourceUsage",
            "Maplibre.Native.Runtime.AmbientCacheOperation",
            "Maplibre.Native.Runtime.OfflineOperationHandle",
            "Maplibre.Native.Runtime.OfflineOperationKind",
            "Maplibre.Native.Runtime.OfflineOperationResultKind",
            "Maplibre.Native.Runtime.OwnerThread",
            "Maplibre.Native.Runtime.RuntimeEvent",
            "Maplibre.Native.Runtime.RuntimeEventPayload",
            "Maplibre.Native.Runtime.RuntimeEventPayload+None",
            "Maplibre.Native.Runtime.RuntimeEventPayload+OfflineOperationCompleted",
            "Maplibre.Native.Runtime.RuntimeEventPayload+OfflineRegionResponseError",
            "Maplibre.Native.Runtime.RuntimeEventPayload+OfflineRegionStatusChanged",
            "Maplibre.Native.Runtime.RuntimeEventPayload+OfflineRegionTileCountLimit",
            "Maplibre.Native.Runtime.RuntimeEventPayload+RenderFrame",
            "Maplibre.Native.Runtime.RuntimeEventPayload+RenderMap",
            "Maplibre.Native.Runtime.RuntimeEventPayload+StyleImageMissing",
            "Maplibre.Native.Runtime.RuntimeEventPayload+TileAction",
            "Maplibre.Native.Runtime.RuntimeEventPayload+Unknown",
            "Maplibre.Native.Runtime.RuntimeEventSourceType",
            "Maplibre.Native.Runtime.RuntimeEventType",
            "Maplibre.Native.Runtime.RuntimeHandle",
            "Maplibre.Native.Runtime.RuntimeOptions",
            "Maplibre.Native.Style.CustomGeometrySourceCallback",
            "Maplibre.Native.Style.CustomGeometrySourceOptions",
            "Maplibre.Native.Style.LocationIndicatorImageKind",
            "Maplibre.Native.Style.RasterDemEncoding",
            "Maplibre.Native.Style.SourceInfo",
            "Maplibre.Native.Style.SourceType",
            "Maplibre.Native.Style.StyleImage",
            "Maplibre.Native.Style.StyleImageInfo",
            "Maplibre.Native.Style.StyleImageOptions",
            "Maplibre.Native.Style.TileScheme",
            "Maplibre.Native.Style.TileSourceOptions",
            "Maplibre.Native.Style.VectorTileEncoding",
        };
        var actual = assembly.GetExportedTypes().Select(type => type.FullName).Order().ToArray();

        var expectedSorted = expected.Order().ToArray();
        Assert.True(
            expectedSorted.SequenceEqual(actual),
            "Expected public types:\n"
                + string.Join('\n', expectedSorted)
                + "\n\nActual public types:\n"
                + string.Join('\n', actual)
        );
    }

    // Support invariant for the Architecture requirements: internal/generated
    // declarations stay outside the supported safe public API.
    [Fact]
    public void GeneratedAndInternalTypesStayOutOfPublicSurface()
    {
        var publicTypes = typeof(Maplibre).Assembly.GetExportedTypes();

        Assert.DoesNotContain(publicTypes, type => type.Namespace?.Contains(".Internal") == true);
        Assert.DoesNotContain(
            publicTypes,
            type => type.Name.StartsWith("mln_", StringComparison.Ordinal)
        );
        Assert.DoesNotContain(publicTypes, type => type.Name == "NativeMethods");
    }

    // Support invariant for the Architecture requirements: public signatures use
    // binding values instead of raw C or host FFI carrier types.
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

    // Support invariant for the NativePointer requirements: raw address conversion
    // is explicitly borrowed backend interop, not ordinary handle construction.
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

    // Support invariant for the Minimality requirement: C# optional/default
    // parameters create shortcut workflows outside the low-level C API shape.
    [Fact]
    public void PublicSurfaceDoesNotUseDefaultParameterValues()
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

    // Support invariant for Handle copying: public code cannot fabricate live
    // owned handles from ordinary constructors.
    [Fact]
    public void OwnedNativeHandlesDoNotExposePublicConstructors()
    {
        var assembly = typeof(Maplibre).Assembly;
        var ownedHandleTypeNames = new[]
        {
            "Maplibre.Native.Map.MapHandle",
            "Maplibre.Native.Map.MapProjectionHandle",
            "Maplibre.Native.Render.MetalOwnedTextureFrameHandle",
            "Maplibre.Native.Render.OpenGLOwnedTextureFrameHandle",
            "Maplibre.Native.Render.RenderSessionHandle",
            "Maplibre.Native.Render.VulkanOwnedTextureFrameHandle",
            "Maplibre.Native.Resource.ResourceRequestHandle",
            "Maplibre.Native.Runtime.OfflineOperationHandle",
            "Maplibre.Native.Runtime.RuntimeHandle",
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
            if (parameter.HasDefaultValue || parameter.IsOptional)
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
