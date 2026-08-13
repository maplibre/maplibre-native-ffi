using System.Reflection;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed class PublicApiSurfaceTests
{
    [Fact]
    public void ExpectedPublicTypesExist()
    {
        var assembly = typeof(Maplibre).Assembly;
        var expected = new[]
        {
            "Maplibre.NativeFfi.Camera.AnimationOptions",
            "Maplibre.NativeFfi.Camera.BoundOptions",
            "Maplibre.NativeFfi.Camera.BoundsConstraint",
            "Maplibre.NativeFfi.Camera.BoundsConstraint+Bounded",
            "Maplibre.NativeFfi.Camera.BoundsConstraint+Unbounded",
            "Maplibre.NativeFfi.Camera.CameraChangeMode",
            "Maplibre.NativeFfi.Camera.CameraSnapshot",
            "Maplibre.NativeFfi.Camera.CameraUpdate",
            "Maplibre.NativeFfi.Camera.CameraUpdateMode",
            "Maplibre.NativeFfi.Camera.CameraFitOptions",
            "Maplibre.NativeFfi.Camera.CameraOptions",
            "Maplibre.NativeFfi.Camera.EdgeInsets",
            "Maplibre.NativeFfi.Camera.FreeCameraOptions",
            "Maplibre.NativeFfi.Camera.GesturePhase",
            "Maplibre.NativeFfi.Camera.UnitBezier",
            "Maplibre.NativeFfi.Error.InvalidArgumentException",
            "Maplibre.NativeFfi.Error.InvalidStateException",
            "Maplibre.NativeFfi.Error.MaplibreException",
            "Maplibre.NativeFfi.Error.MaplibreStatus",
            "Maplibre.NativeFfi.Error.NativeErrorException",
            "Maplibre.NativeFfi.Error.UnsupportedFeatureException",
            "Maplibre.NativeFfi.Error.WrongThreadException",
            "Maplibre.NativeFfi.Geo.CanonicalTileId",
            "Maplibre.NativeFfi.Geo.LatLng",
            "Maplibre.NativeFfi.Geo.LatLngBounds",
            "Maplibre.NativeFfi.Geo.ProjectedMeters",
            "Maplibre.NativeFfi.Geo.Quaternion",
            "Maplibre.NativeFfi.Geo.ScreenBox",
            "Maplibre.NativeFfi.Geo.ScreenPoint",
            "Maplibre.NativeFfi.Geo.TileId",
            "Maplibre.NativeFfi.Geo.Vec3",
            "Maplibre.NativeFfi.Log.LogCallback",
            "Maplibre.NativeFfi.Log.LogEvent",
            "Maplibre.NativeFfi.Log.LogRecord",
            "Maplibre.NativeFfi.Log.LogSeverity",
            "Maplibre.NativeFfi.Log.LogSeverityMask",
            "Maplibre.NativeFfi.Map.ConstrainMode",
            "Maplibre.NativeFfi.Map.DebugOptions",
            "Maplibre.NativeFfi.Map.LogicalExtent",
            "Maplibre.NativeFfi.Map.MapHandle",
            "Maplibre.NativeFfi.Map.MapMode",
            "Maplibre.NativeFfi.Map.MapOptions",
            "Maplibre.NativeFfi.Map.MapSnapshot",
            "Maplibre.NativeFfi.Map.MapProjectionHandle",
            "Maplibre.NativeFfi.Map.NorthOrientation",
            "Maplibre.NativeFfi.Map.ProjectionModeOptions",
            "Maplibre.NativeFfi.Map.RenderingStats",
            "Maplibre.NativeFfi.Map.TileLodMode",
            "Maplibre.NativeFfi.Map.TileOperation",
            "Maplibre.NativeFfi.Map.TileOptions",
            "Maplibre.NativeFfi.Map.ViewportMode",
            "Maplibre.NativeFfi.Map.ViewportOptions",
            "Maplibre.NativeFfi.Maplibre",
            "Maplibre.NativeFfi.NativePointer",
            "Maplibre.NativeFfi.NetworkStatus",
            "Maplibre.NativeFfi.Offline.OfflineRegionDefinition",
            "Maplibre.NativeFfi.Offline.OfflineRegionDefinition+GeometryRegion",
            "Maplibre.NativeFfi.Offline.OfflineRegionDefinition+TilePyramid",
            "Maplibre.NativeFfi.Offline.OfflineRegionDownloadState",
            "Maplibre.NativeFfi.Offline.OfflineRegionInfo",
            "Maplibre.NativeFfi.Offline.OfflineRegionStatus",
            "Maplibre.NativeFfi.Query.FeatureStateSelector",
            "Maplibre.NativeFfi.Query.RenderedFeatureQueryOptions",
            "Maplibre.NativeFfi.Query.RenderedQueryGeometry",
            "Maplibre.NativeFfi.Query.RenderedQueryGeometry+Box",
            "Maplibre.NativeFfi.Query.RenderedQueryGeometry+LineString",
            "Maplibre.NativeFfi.Query.RenderedQueryGeometry+Point",
            "Maplibre.NativeFfi.Query.SourceFeatureQueryOptions",
            "Maplibre.NativeFfi.Render.EglContextDescriptor",
            "Maplibre.NativeFfi.Render.MetalBorrowedTextureDescriptor",
            "Maplibre.NativeFfi.Render.MetalContextDescriptor",
            "Maplibre.NativeFfi.Render.MetalOwnedTextureDescriptor",
            "Maplibre.NativeFfi.Render.MetalOwnedTextureFrame",
            "Maplibre.NativeFfi.Render.MetalOwnedTextureFrameHandle",
            "Maplibre.NativeFfi.Render.MetalSurfaceDescriptor",
            "Maplibre.NativeFfi.Render.NativeBuffer",
            "Maplibre.NativeFfi.Render.OpenGLBorrowedTextureDescriptor",
            "Maplibre.NativeFfi.Render.OpenGLClientApi",
            "Maplibre.NativeFfi.Render.OpenGLContextDescriptor",
            "Maplibre.NativeFfi.Render.OpenGLContextOwnership",
            "Maplibre.NativeFfi.Render.OpenGLContextProvider",
            "Maplibre.NativeFfi.Render.OpenGLOwnedTextureDescriptor",
            "Maplibre.NativeFfi.Render.OpenGLOwnedTextureFrame",
            "Maplibre.NativeFfi.Render.OpenGLOwnedTextureFrameHandle",
            "Maplibre.NativeFfi.Render.OpenGLSurfaceDescriptor",
            "Maplibre.NativeFfi.Render.PremultipliedRgba8Image",
            "Maplibre.NativeFfi.Render.RenderBackend",
            "Maplibre.NativeFfi.Render.RenderMode",
            "Maplibre.NativeFfi.Render.RenderResult",
            "Maplibre.NativeFfi.Render.RenderSessionHandle",
            "Maplibre.NativeFfi.Render.RenderTargetExtent",
            "Maplibre.NativeFfi.Render.TextureImageInfo",
            "Maplibre.NativeFfi.Render.VulkanBorrowedTextureDescriptor",
            "Maplibre.NativeFfi.Render.VulkanContextDescriptor",
            "Maplibre.NativeFfi.Render.VulkanOwnedTextureDescriptor",
            "Maplibre.NativeFfi.Render.VulkanOwnedTextureFrame",
            "Maplibre.NativeFfi.Render.VulkanOwnedTextureFrameHandle",
            "Maplibre.NativeFfi.Render.VulkanSurfaceDescriptor",
            "Maplibre.NativeFfi.Render.WglContextDescriptor",
            "Maplibre.NativeFfi.Resource.ByteRange",
            "Maplibre.NativeFfi.Resource.HttpHeader",
            "Maplibre.NativeFfi.Resource.HttpHeaderTransformCallback",
            "Maplibre.NativeFfi.Resource.HttpHeaderTransformRequest",
            "Maplibre.NativeFfi.Resource.ResourceErrorReason",
            "Maplibre.NativeFfi.Resource.ResourceKind",
            "Maplibre.NativeFfi.Resource.ResourceLoadingMethod",
            "Maplibre.NativeFfi.Resource.ResourcePriority",
            "Maplibre.NativeFfi.Resource.ResourceProviderCallback",
            "Maplibre.NativeFfi.Resource.ResourceProviderDecision",
            "Maplibre.NativeFfi.Resource.ResourceRequest",
            "Maplibre.NativeFfi.Resource.ResourceRequestHandle",
            "Maplibre.NativeFfi.Resource.ResourceResponse",
            "Maplibre.NativeFfi.Resource.ResourceResponseStatus",
            "Maplibre.NativeFfi.Resource.ResourceStoragePolicy",
            "Maplibre.NativeFfi.Resource.ResourceTransformCallback",
            "Maplibre.NativeFfi.Resource.ResourceTransformRequest",
            "Maplibre.NativeFfi.Resource.ResourceUsage",
            "Maplibre.NativeFfi.Runtime.AmbientCacheOperation",
            "Maplibre.NativeFfi.Runtime.CommandDisposition",
            "Maplibre.NativeFfi.Runtime.NotificationEndpointKind",
            "Maplibre.NativeFfi.Runtime.OperationCompletion",
            "Maplibre.NativeFfi.Runtime.OperationHandle",
            "Maplibre.NativeFfi.Runtime.RuntimeEvent",
            "Maplibre.NativeFfi.Runtime.RuntimeEventBatch",
            "Maplibre.NativeFfi.Runtime.RuntimeEventMask",
            "Maplibre.NativeFfi.Runtime.RuntimeEventPayload",
            "Maplibre.NativeFfi.Runtime.RuntimeEventPayload+CameraTransitionFinished",
            "Maplibre.NativeFfi.Runtime.RuntimeEventPayload+CommandFinished",
            "Maplibre.NativeFfi.Runtime.RuntimeEventPayload+None",
            "Maplibre.NativeFfi.Runtime.RuntimeEventPayload+OfflineRegionResponseError",
            "Maplibre.NativeFfi.Runtime.RuntimeEventPayload+OfflineRegionStatusChanged",
            "Maplibre.NativeFfi.Runtime.RuntimeEventPayload+OfflineRegionTileCountLimit",
            "Maplibre.NativeFfi.Runtime.RuntimeEventPayload+RenderFrame",
            "Maplibre.NativeFfi.Runtime.RuntimeEventPayload+RenderMap",
            "Maplibre.NativeFfi.Runtime.RuntimeEventPayload+TileAction",
            "Maplibre.NativeFfi.Runtime.RuntimeEventPayload+Unknown",
            "Maplibre.NativeFfi.Runtime.RuntimeEventSourceType",
            "Maplibre.NativeFfi.Runtime.RuntimeEventType",
            "Maplibre.NativeFfi.Runtime.ReadyEndpoint",
            "Maplibre.NativeFfi.Runtime.RuntimeHandle",
            "Maplibre.NativeFfi.Runtime.RuntimeOptions",
            "Maplibre.NativeFfi.Style.CustomGeometrySourceCallback",
            "Maplibre.NativeFfi.Style.CustomGeometrySourceOptions",
            "Maplibre.NativeFfi.Style.GeoJsonSourceOptions",
            "Maplibre.NativeFfi.Style.LocationIndicatorImageKind",
            "Maplibre.NativeFfi.Style.RasterDemEncoding",
            "Maplibre.NativeFfi.Style.SourceInfo",
            "Maplibre.NativeFfi.Style.SourceType",
            "Maplibre.NativeFfi.Style.ImageContent",
            "Maplibre.NativeFfi.Style.ImageStretch",
            "Maplibre.NativeFfi.Style.StyleImage",
            "Maplibre.NativeFfi.Style.StyleImageInfo",
            "Maplibre.NativeFfi.Style.StyleImageOptions",
            "Maplibre.NativeFfi.Style.StyleImageTextFit",
            "Maplibre.NativeFfi.Style.StyleLayerVisibility",
            "Maplibre.NativeFfi.Style.StyleTransitionOptions",
            "Maplibre.NativeFfi.Style.TileJson",
            "Maplibre.NativeFfi.Style.TileScheme",
            "Maplibre.NativeFfi.Style.TileSourceOptions",
            "Maplibre.NativeFfi.Style.VectorTileEncoding",
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

    [Fact]
    public void OwnedNativeHandlesDoNotExposePublicConstructors()
    {
        var assembly = typeof(Maplibre).Assembly;
        var ownedHandleTypeNames = new[]
        {
            "Maplibre.NativeFfi.Map.MapHandle",
            "Maplibre.NativeFfi.Map.MapProjectionHandle",
            "Maplibre.NativeFfi.Render.MetalOwnedTextureFrameHandle",
            "Maplibre.NativeFfi.Render.OpenGLOwnedTextureFrameHandle",
            "Maplibre.NativeFfi.Render.RenderSessionHandle",
            "Maplibre.NativeFfi.Render.VulkanOwnedTextureFrameHandle",
            "Maplibre.NativeFfi.Resource.ResourceRequestHandle",
            "Maplibre.NativeFfi.Runtime.OperationHandle",
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
