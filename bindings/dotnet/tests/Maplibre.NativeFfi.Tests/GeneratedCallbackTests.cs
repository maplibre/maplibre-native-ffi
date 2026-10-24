using System.Runtime.CompilerServices;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Callback;
using Maplibre.NativeFfi.Internal.Memory;
using Maplibre.NativeFfi.Internal.Pointer;
using Maplibre.NativeFfi.Internal.Struct;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed unsafe class GeneratedCallbackTests
{
    [Fact]
    public void GeneratedCallbacksCopyArgumentsAndContainFailures()
    {
        CanonicalTileId? fetched = null;
        var options = new CustomGeometrySourceOptions
        {
            FetchTile = tile => fetched = tile,
            CancelTile = _ => throw new InvalidOperationException("callback failure"),
        };
        using var scope = new NativeCallScope();
        var native = GeneratedValues.NativeCustomGeometrySourceOptions(options, scope);
        var tile = new mln_canonical_tile_id
        {
            z = 1,
            x = 2,
            y = 3,
        };
        native.fetch_tile(native.user_data, tile);
        native.cancel_tile(native.user_data, tile);
        tile.x = 99;
        Assert.Equal(new CanonicalTileId(1, 2, 3), fetched);
        native.release_user_data(native.user_data);
        scope.Accept();
    }

    [Fact]
    public void RegistrationsStayRootedUntilNativeRelease()
    {
        var first = PrepareOwned();
        var late = PrepareOwned();
        Gc.Collect();
        Assert.True(first.Root.IsAlive);
        Assert.True(late.Root.IsAlive);
        first.Native.fetch_tile(first.Native.user_data, default);
        first.Native.release_user_data(first.Native.user_data);
        late.Native.release_user_data(late.Native.user_data);
        Gc.Collect();
        Assert.False(first.Root.IsAlive);
        Assert.False(late.Root.IsAlive);
    }

    [Fact]
    public void FailedOwnerConstructionKeepsAcceptedCallbacksUntilNativeRelease()
    {
        var (native, root) = PrepareFailedOwner();
        Gc.Collect();
        Assert.True(root.IsAlive);
        native.fetch_tile(native.user_data, default);
        native.release_user_data(native.user_data);
        Gc.Collect();
        Assert.False(root.IsAlive);
    }

    [MethodImpl(MethodImplOptions.NoInlining)]
    private static (
        mln_custom_geometry_source_options Native,
        WeakReference Root
    ) PrepareFailedOwner()
    {
        using var scope = new NativeCallScope();
        var native = GeneratedValues.NativeCustomGeometrySourceOptions(
            new CustomGeometrySourceOptions { FetchTile = _ => { } },
            scope
        );
        var weak = new WeakReference(NativeCallbackRoot.Value(native.user_data));
        // A generated attachment accepts its registrations once native admits the call, before it
        // adopts the handle, so a failed adoption leaves them to native.
        scope.Accept();
        var disposed = false;
        var failure = new FormatException("Owner construction failed.");
        Assert.Same(
            failure,
            Assert.Throws<FormatException>(() =>
                NativeHandleState<MlnRuntime>.Adopt<RuntimeHandle>(
                    SyntheticHandles.Runtime(1234),
                    () => throw failure,
                    (_, _) =>
                    {
                        disposed = true;
                        return mln_status.MLN_STATUS_OK;
                    }
                )
            )
        );
        Assert.True(disposed);
        return (native, weak);
    }

    [MethodImpl(MethodImplOptions.NoInlining)]
    private static (mln_custom_geometry_source_options Native, WeakReference Root) PrepareOwned()
    {
        using var scope = new NativeCallScope();
        var native = GeneratedValues.NativeCustomGeometrySourceOptions(
            new CustomGeometrySourceOptions { FetchTile = _ => { } },
            scope
        );
        var weak = new WeakReference(NativeCallbackRoot.Value(native.user_data));
        scope.Accept();
        return (native, weak);
    }
}
