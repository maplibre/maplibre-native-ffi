using Maplibre.Native.Runtime;
using Xunit;

namespace Maplibre.Native.Tests;

public sealed class RuntimeOfflineOperationTests
{
    [Fact]
    public void AmbientCacheOperationCanBeStartedAndDiscarded()
    {
        NativeLibraryTestSupport.SkipUnlessNativeLibraryIsAvailable();
        using var runtime = RuntimeHandle.Create();

        using var operation = runtime.StartAmbientCacheOperation(AmbientCacheOperation.Invalidate);

        Assert.NotEqual(0u, operation.Id);
        Assert.Equal(OfflineOperationKind.AmbientCache, operation.Kind);
        Assert.Equal(OfflineOperationResultKind.None, operation.ResultKind);
        Assert.False(operation.IsClosed);

        operation.Close();
        operation.Close();

        Assert.True(operation.IsClosed);
    }

    [Fact]
    public void OperationCloseAfterRuntimeCloseMarksOperationClosed()
    {
        NativeLibraryTestSupport.SkipUnlessNativeLibraryIsAvailable();
        var runtime = RuntimeHandle.Create();
        using var operation = runtime.StartAmbientCacheOperation(AmbientCacheOperation.Invalidate);

        runtime.Close();

        operation.Close();
        Assert.True(operation.IsClosed);
        operation.Close();
    }
}
