using Maplibre.NativeFfi.Error;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Status;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Runtime;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

/// <summary>
/// Handle-identity behaviour the C API owns, reached through internal handle
/// accessors because the safe public API cannot express these calls.
/// </summary>
public sealed unsafe class NativeHandleIdentityTests
{
    [BindingSpecTest("BND-045")]
    [Fact]
    public void ReleasedMapIdReplayedAfterANewMapReportsInvalidArgument()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());

        var first = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });
        var releasedId = first.Handle;
        first.Close();

        // The released slot is the one the next map takes, so the replayed id
        // names a retired generation of a slot that is live again.
        using var second = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });

        var error = Assert.Throws<InvalidArgumentException>(() => GetSize(releasedId));

        Assert.Equal(MaplibreStatus.InvalidArgument, error.Status);
        Assert.Contains("stale", error.Diagnostic, StringComparison.OrdinalIgnoreCase);

        // The live map is unaffected by the replay.
        var (width, _, _) = GetSize(second.Handle);
        Assert.Equal(512u, width);
    }

    [BindingSpecTest("BND-047")]
    [Fact]
    public void MapIdPassedToARuntimeOperationReportsInvalidArgument()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });

        // MlnMap and MlnRuntime are distinct CLR types, so this call has no
        // expression in the safe public API and needs the raw id.
        var wrongKind = new MlnRuntime(map.Handle.Value);

        var error = Assert.Throws<InvalidArgumentException>(() =>
            NativeStatus.Check(NativeMethods.mln_runtime_pump(wrongKind, 0, -1))
        );

        Assert.Equal(MaplibreStatus.InvalidArgument, error.Status);
        Assert.Contains("map", error.Diagnostic, StringComparison.OrdinalIgnoreCase);
        Assert.Contains("runtime", error.Diagnostic, StringComparison.OrdinalIgnoreCase);
    }

    [BindingSpecTest("BND-049")]
    [Fact]
    public void MapIdCalledFromAnotherThreadReportsWrongThread()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());
        using var map = MapHandle.Create(runtime, new MapOptions { Width = 512, Height = 512 });

        var live = map.Handle;
        Exception? thrown = null;

        var thread = new Thread(() =>
        {
            thrown = Record.Exception(() => GetSize(live));
        });
        thread.Start();
        thread.Join();

        // The id is live, so the owner-thread rule decides rather than identity.
        var error = Assert.IsType<WrongThreadException>(thrown);
        Assert.Equal(MaplibreStatus.WrongThread, error.Status);
        Assert.DoesNotContain("stale", error.Diagnostic, StringComparison.OrdinalIgnoreCase);
    }

    private static (uint Width, uint Height, double ScaleFactor) GetSize(MlnMap map)
    {
        uint width = 0;
        uint height = 0;
        double scaleFactor = 0;
        NativeStatus.Check(NativeMethods.mln_map_get_size(map, &width, &height, &scaleFactor));
        return (width, height, scaleFactor);
    }
}
