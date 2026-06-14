namespace Maplibre.Native.Examples.DotnetMap;

internal static class MacObjectiveC
{
    public static nint Retain(nint obj)
    {
        _ = obj;
        throw new NotImplementedException("Objective-C retain is not implemented yet.");
    }

    public static void Release(nint obj)
    {
        _ = obj;
        throw new NotImplementedException("Objective-C release is not implemented yet.");
    }

    public static nint MetalSystemDefaultDevice()
    {
        throw new NotImplementedException("MTLCreateSystemDefaultDevice is not implemented yet.");
    }
}
