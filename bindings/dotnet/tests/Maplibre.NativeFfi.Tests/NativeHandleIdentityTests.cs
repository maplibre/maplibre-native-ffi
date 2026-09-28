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
    [BindingSpecTest("")]
    [Fact]
    public void ReleasedMapIdReplayedAfterANewMapReportsInvalidArgument()
    {
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);

        var first = TestHandles.CreateMap(
            runtime,
            MapOptions.Default with
            {
                InitialExtent = MapOptions.Default.InitialExtent with { Width = 512, Height = 512 },
            }
        );
        var releasedId = first.Handle;
        first.Close();

        // The released slot is the one the next map takes, so the replayed id
        // names a retired generation of a slot that is live again.
        using var second = TestHandles.CreateMap(
            runtime,
            MapOptions.Default with
            {
                InitialExtent = MapOptions.Default.InitialExtent with { Width = 512, Height = 512 },
            }
        );

        var error = Assert.Throws<InvalidArgumentException>(() => GetSnapshot(releasedId));

        Assert.Equal(MaplibreStatus.InvalidArgument, error.Status);
        Assert.Contains("stale", error.Diagnostic, StringComparison.OrdinalIgnoreCase);

        // The live map is unaffected by the replay.
        Assert.Equal(512u, GetSnapshot(second.Handle).logical_extent.width);
    }

    [BindingSpecTest("")]
    [Fact]
    public void MapIdPassedToARuntimeOperationReportsInvalidArgument()
    {
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        using var map = TestHandles.CreateMap(
            runtime,
            MapOptions.Default with
            {
                InitialExtent = MapOptions.Default.InitialExtent with { Width = 512, Height = 512 },
            }
        );

        // MlnMap and MlnRuntime are distinct CLR types, so this call has no
        // expression in the safe public API and needs the raw id.
        var wrongKind = new MlnRuntime(map.Handle.Value);

        var error = Assert.Throws<InvalidArgumentException>(() =>
        {
            ulong mask = 0;
            NativeStatus.Check(NativeMethods.mln_runtime_get_event_mask(wrongKind, &mask));
        });

        Assert.Equal(MaplibreStatus.InvalidArgument, error.Status);
        Assert.Contains("map", error.Diagnostic, StringComparison.OrdinalIgnoreCase);
        Assert.Contains("runtime", error.Diagnostic, StringComparison.OrdinalIgnoreCase);
    }

    [BindingSpecTest("")]
    [Fact]
    public void MapIdCanBeReadFromAnotherThread()
    {
        using var runtime = RuntimeHandle.Create(RuntimeOptions.Default);
        using var map = TestHandles.CreateMap(
            runtime,
            MapOptions.Default with
            {
                InitialExtent = MapOptions.Default.InitialExtent with { Width = 512, Height = 512 },
            }
        );
        mln_map_snapshot snapshot = default;

        var thread = new Thread(() => snapshot = GetSnapshot(map.Handle));
        thread.Start();
        thread.Join();

        Assert.Equal(512u, snapshot.logical_extent.width);
    }

    private static mln_map_snapshot GetSnapshot(MlnMap map)
    {
        var snapshot = new mln_map_snapshot { size = (uint)sizeof(mln_map_snapshot) };
        NativeStatus.Check(NativeMethods.mln_map_snapshot_get(map, &snapshot));
        return snapshot;
    }
}
