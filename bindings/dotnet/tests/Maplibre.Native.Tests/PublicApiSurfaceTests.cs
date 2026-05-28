using Xunit;

namespace Maplibre.Native.Tests;

public sealed class PublicApiSurfaceTests
{
    [Fact]
    public void ExpectedPublicTypesExist()
    {
        var assembly = typeof(Maplibre).Assembly;
        var expected = new[]
        {
            "Maplibre.Native.Maplibre",
            "Maplibre.Native.NativePointer",
            "Maplibre.Native.Error.MaplibreException",
            "Maplibre.Native.Camera.CameraOptions",
            "Maplibre.Native.Camera.AnimationOptions",
            "Maplibre.Native.Camera.EdgeInsets",
            "Maplibre.Native.Geo.LatLng",
            "Maplibre.Native.Geo.Geometry",
            "Maplibre.Native.Geo.Feature",
            "Maplibre.Native.Json.JsonValue",
            "Maplibre.Native.Log.LogRecord",
            "Maplibre.Native.Log.LogSeverity",
            "Maplibre.Native.Map.MapHandle",
            "Maplibre.Native.Map.MapProjectionHandle",
            "Maplibre.Native.Map.ViewportOptions",
            "Maplibre.Native.Offline.OfflineRegionDefinition",
            "Maplibre.Native.Query.RenderedQueryGeometry",
            "Maplibre.Native.Query.QueriedFeature",
            "Maplibre.Native.Render.RenderSessionHandle",
            "Maplibre.Native.Render.RenderBackend",
            "Maplibre.Native.Render.OpenGLContextProvider",
            "Maplibre.Native.Render.OpenGLContextDescriptor",
            "Maplibre.Native.Render.WglContextDescriptor",
            "Maplibre.Native.Render.EglContextDescriptor",
            "Maplibre.Native.Render.OpenGLSurfaceDescriptor",
            "Maplibre.Native.Render.OpenGLOwnedTextureDescriptor",
            "Maplibre.Native.Render.OpenGLBorrowedTextureDescriptor",
            "Maplibre.Native.Render.OpenGLOwnedTextureFrame",
            "Maplibre.Native.Render.OpenGLOwnedTextureFrameHandle",
            "Maplibre.Native.Render.NativeBuffer",
            "Maplibre.Native.Resource.ResourceRequest",
            "Maplibre.Native.Resource.ResourceRequestHandle",
            "Maplibre.Native.Runtime.RuntimeHandle",
            "Maplibre.Native.Runtime.RuntimeEvent",
            "Maplibre.Native.Runtime.OfflineOperationHandle",
            "Maplibre.Native.Style.SourceInfo",
            "Maplibre.Native.Style.StyleImage",
        };

        foreach (var typeName in expected)
        {
            Assert.NotNull(assembly.GetType(typeName));
        }
    }
}
