// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi;

/// <summary>
/// A runtime owns maps and the network and cache settings that they share.
/// </summary>
/// <remarks>
/// See <c>mln_runtime</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html">C API reference</see>.
/// </remarks>
public sealed unsafe partial class RuntimeHandle
    : IDisposable,
        IAsyncDisposable,
        INativeOwner<MlnRuntime>
{
    private readonly NativeHandleState<MlnRuntime> state;
    private volatile Task teardown = Task.CompletedTask;

    internal RuntimeHandle(MlnRuntime handle)
    {
        state = new(handle, StartRelease, nameof(RuntimeHandle), Abandon);
    }

    internal static RuntimeHandle Adopt(MlnRuntime handle) =>
        NativeHandleState<MlnRuntime>.Adopt(handle, () => new RuntimeHandle(handle), Abandon);

    private static mln_status Abandon(MlnRuntime live, mln_diagnostic* diagnostic) =>
        NativeMethods.mln_runtime_dispose(live, diagnostic);

    NativeHandleState<MlnRuntime> INativeOwner<MlnRuntime>.State => state;
    internal MlnRuntime Handle => state.Handle;

    /// <summary>
    /// The native handle value, which runtime events report as their
    /// source. It stays readable after release, so events drained later
    /// still match this handle.
    /// </summary>
    public ulong Id => state.Handle.Value;

    /// <summary>
    /// Whether a release, dispose, or discard has ended the handle.
    /// </summary>
    public bool IsClosed => state.IsClosed;

    /// <summary>
    /// Names the handle's type, its <see cref="Id" />, and whether it is closed.
    /// </summary>
    public override string ToString() => state.ToString();

    /// <summary>
    /// Releases a runtime in any state, without waiting.
    /// </summary>
    /// <remarks>
    /// <para>The runtime finishes its teardown after its maps are
    /// released.</para>
    /// <para>See <c>mln_runtime_dispose</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.</para>
    /// </remarks>
    public void Dispose()
    {
        state.Retire();
    }

    /// <summary>
    /// Stops sending the events of the runtime and its maps to a callback.
    /// </summary>
    /// <remarks>
    /// See <c>mln_runtime_clear_event_handler</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
    /// </remarks>
    public Task<CommandCompletion> ClearEventHandlerAsync()
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_runtime_clear_event_handler(
            Handle,
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Stops adding headers to the runtime's HTTP requests.
    /// </summary>
    /// <remarks>
    /// See <c>mln_runtime_clear_http_header_transform</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
    /// </remarks>
    public Task<CommandCompletion> ClearHttpHeaderTransformAsync()
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_runtime_clear_http_header_transform(
            Handle,
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Removes the runtime's resource provider, so the library loads every
    /// request itself.
    /// </summary>
    /// <remarks>
    /// See <c>mln_runtime_clear_resource_provider</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
    /// </remarks>
    public Task<CommandCompletion> ClearResourceProviderAsync()
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_runtime_clear_resource_provider(
            Handle,
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Stops rewriting the URLs of the runtime's network requests.
    /// </summary>
    /// <remarks>
    /// See <c>mln_runtime_clear_resource_transform</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
    /// </remarks>
    public Task<CommandCompletion> ClearResourceTransformAsync()
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_runtime_clear_resource_transform(
            Handle,
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Creates a runtime.
    /// </summary>
    /// <remarks>
    /// <para>A runtime holds maps and the resource loading, cache, and offline
    /// regions that they share. In a browser, create runtimes on a Web Worker,
    /// not on the main thread.</para>
    /// <para>See <c>mln_runtime_create</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.</para>
    /// </remarks>
    public static RuntimeHandle Create(RuntimeOptions? options = null)
    {
        using var scope = new NativeCallScope(null);
        var nativeOptions = options is null
            ? default(mln_runtime_options)
            : NativeRuntimeOptions(options.Value, scope);
        MlnRuntime outRuntime = default;
        Check(
            NativeMethods.mln_runtime_create(
                options is null ? null : &nativeOptions,
                &outRuntime,
                Diagnostic
            )
        );
        scope.Accept();
        return RuntimeHandle.Adopt(outRuntime);
    }

    /// <summary>
    /// Creates a map on <c>runtime</c>.
    /// </summary>
    /// <remarks>
    /// See <c>mln_runtime_create_map</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
    /// </remarks>
    public MapHandle CreateMap(MapOptions options)
    {
        using var scope = new NativeCallScope(this);
        var nativeOptions = NativeMapOptions(options);
        MlnMap outMap = default;
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_runtime_create_map(
            Handle,
            &nativeOptions,
            &outMap,
            NativeCompletion.StartUnit(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        var nativeReady = nativePending.Submitted(nativeStatus);
        scope.Accept();
        return MapHandle.Adopt(this, outMap, nativeReady);
    }

    /// <summary>
    /// Creates an offline region in the runtime's cache file.
    /// </summary>
    /// <remarks>
    /// <para><c>metadata</c> holds any bytes to store with the region, such as
    /// its name. The region doesn't download until
    /// <c>RuntimeHandle.SetOfflineRegionDownloadStateAsync()</c> makes its
    /// download active.</para>
    /// <para>See <c>mln_runtime_create_offline_region</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.</para>
    /// </remarks>
    public Task<OfflineRegionInfo> CreateOfflineRegionAsync(
        OfflineRegionDefinition definition,
        ReadOnlySpan<byte> metadata
    )
    {
        fixed (byte* pinnedMetadata = metadata)
        {
            using var scope = new NativeCallScope(this);
            var nativeDefinition = NativeOfflineRegionDefinition(definition, scope);
            mln_completion nativeCompletion;
            var nativeStatus = NativeMethods.mln_runtime_create_offline_region(
                Handle,
                &nativeDefinition,
                (byte*)pinnedMetadata,
                checked((nuint)metadata.Length),
                NativeCompletion.Start<OfflineRegionInfo>(
                    &nativeCompletion,
                    static nativeResult =>
                        NativeCompletion.Single<mln_offline_region_info, OfflineRegionInfo>(
                            nativeResult,
                            CopyOfflineRegionInfo
                        ),
                    out var nativePending
                ),
                Diagnostic
            );
            return scope.Accepted(nativePending.Submitted(nativeStatus));
        }
    }

    /// <summary>
    /// Deletes the offline region <c>regionId</c> from the runtime's cache
    /// file.
    /// </summary>
    /// <remarks>
    /// <para>See <c>mln_runtime_delete_offline_region</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.NotFound</c> when the cache file has no
    /// region <c>regionId</c>.</description></item>
    /// </list>
    /// </remarks>
    public Task DeleteOfflineRegionAsync(long regionId)
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_runtime_delete_offline_region(
            Handle,
            regionId,
            NativeCompletion.StartUnit(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Reads the offline region <c>regionId</c>.
    /// </summary>
    /// <remarks>
    /// <para>The result is null when the cache file has no region with that
    /// ID.</para>
    /// <para>See <c>mln_runtime_get_offline_region</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.</para>
    /// </remarks>
    public Task<OfflineRegionInfo?> GetOfflineRegionAsync(long regionId)
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_runtime_get_offline_region(
            Handle,
            regionId,
            NativeCompletion.Start<OfflineRegionInfo?>(
                &nativeCompletion,
                static nativeResult =>
                    NativeCompletion.OptionalValue<mln_offline_region_info, OfflineRegionInfo>(
                        nativeResult,
                        CopyOfflineRegionInfo
                    ),
                out var nativePending
            ),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Reads the download progress of the offline region <c>regionId</c>.
    /// </summary>
    /// <remarks>
    /// <para>While the download is active, the counts cover what it has
    /// visited. Once it's inactive, they cover every resource the region holds,
    /// including those that only earlier versions of its style listed, so the
    /// completed counts can exceed the required counts.</para>
    /// <para>See <c>mln_runtime_get_offline_region_status</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.NotFound</c> when the cache file has no
    /// region <c>regionId</c>.</description></item>
    /// </list>
    /// </remarks>
    public Task<OfflineRegionStatus> GetOfflineRegionStatusAsync(long regionId)
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_runtime_get_offline_region_status(
            Handle,
            regionId,
            NativeCompletion.Start<OfflineRegionStatus>(
                &nativeCompletion,
                static nativeResult =>
                    NativeCompletion.Single<mln_offline_region_status, OfflineRegionStatus>(
                        nativeResult,
                        CopyOfflineRegionStatus
                    ),
                out var nativePending
            ),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Makes maps check each resource of the offline region <c>regionId</c>
    /// with the server before they use it.
    /// </summary>
    /// <remarks>
    /// <para>The region keeps its resources. Starting its download again
    /// fetches the invalidated resources for offline use. If the style or a
    /// TileJSON document is then missing or invalid, the download reports an
    /// offline region response error and goes on from the stored copy.
    /// Resources that the current style no longer lists stay in the region
    /// until it's deleted.</para>
    /// <para>See <c>mln_runtime_invalidate_offline_region</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.NotFound</c> when the cache file has no
    /// region <c>regionId</c>.</description></item>
    /// </list>
    /// </remarks>
    public Task InvalidateOfflineRegionAsync(long regionId)
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_runtime_invalidate_offline_region(
            Handle,
            regionId,
            NativeCompletion.StartUnit(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Lists the offline regions in the runtime's cache file.
    /// </summary>
    /// <remarks>
    /// See <c>mln_runtime_list_offline_regions</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
    /// </remarks>
    public Task<OfflineRegionInfo[]> ListOfflineRegionsAsync()
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_runtime_list_offline_regions(
            Handle,
            NativeCompletion.Start<OfflineRegionInfo[]>(
                &nativeCompletion,
                static nativeResult =>
                    NativeCompletion.Array<mln_offline_region_info, OfflineRegionInfo>(
                        nativeResult,
                        CopyOfflineRegionInfo
                    ),
                out var nativePending
            ),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Copies the offline regions of another cache file into the runtime's
    /// cache file.
    /// </summary>
    /// <remarks>
    /// <para>A region whose definition and metadata match one that the
    /// runtime's file already has isn't copied again. The result lists the
    /// runtime's copy of each region in the other file.</para>
    /// <para>See <c>mln_runtime_merge_offline_regions</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.InvalidArgument</c> when
    /// <c>sideDatabasePath</c> isn't a readable database
    /// file.</description></item>
    /// <item><description><c>Status.NativeError</c> when the merge would exceed
    /// the tile limit that
    /// <c>RuntimeEventType.OfflineRegionTileCountLimitExceeded</c>
    /// describes.</description></item>
    /// </list>
    /// </remarks>
    public Task<OfflineRegionInfo[]> MergeOfflineRegionsAsync(string sideDatabasePath)
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_runtime_merge_offline_regions(
            Handle,
            scope.Utf8(sideDatabasePath),
            NativeCompletion.Start<OfflineRegionInfo[]>(
                &nativeCompletion,
                static nativeResult =>
                    NativeCompletion.Array<mln_offline_region_info, OfflineRegionInfo>(
                        nativeResult,
                        CopyOfflineRegionInfo
                    ),
                out var nativePending
            ),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Releases a runtime.
    /// </summary>
    /// <remarks>
    /// <para>Release the runtime's maps first.</para>
    /// <para>See <c>mln_runtime_release</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.</para>
    /// </remarks>
    public Task ReleaseAsync()
    {
        state.Close(idempotent: false);
        return teardown;
    }

    ValueTask IAsyncDisposable.DisposeAsync() => state.Close() ? new(teardown) : default;

    private mln_status StartRelease(MlnRuntime handle, mln_diagnostic* _)
    {
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_runtime_release(
            handle,
            NativeCompletion.StartUnit(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        teardown = nativePending.Submitted(nativeStatus);
        return mln_status.MLN_STATUS_OK;
    }

    /// <summary>
    /// Runs a maintenance action on the runtime's cache database.
    /// </summary>
    /// <remarks>
    /// See <c>mln_runtime_run_cache_database_operation</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
    /// </remarks>
    public Task RunCacheDatabaseOperationAsync(CacheDatabaseOperation operation)
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_runtime_run_cache_database_operation(
            Handle,
            (mln_cache_database_operation)operation,
            NativeCompletion.StartUnit(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Sends the events of the runtime and its maps to a callback.
    /// </summary>
    /// <remarks>
    /// <para>The callback replaces the one set before.</para>
    /// <para>See <c>mln_runtime_set_event_handler</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.</para>
    /// </remarks>
    public Task<CommandCompletion> SetEventHandlerAsync(RuntimeEventHandler handler)
    {
        using var scope = new NativeCallScope(this);
        var nativeHandler = NativeRuntimeEventHandler(handler, scope);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_runtime_set_event_handler(
            Handle,
            &nativeHandler,
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Adds headers to the runtime's HTTP requests.
    /// </summary>
    /// <remarks>
    /// <para>The transform replaces the one set before. In a browser or on
    /// OpenHarmony, serve the requests with
    /// <c>RuntimeHandle.SetResourceProviderAsync()</c> instead.</para>
    /// <para>Runs on a library thread.</para>
    /// <para>See <c>mln_runtime_set_http_header_transform</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.Unsupported</c> when the library runs in a
    /// browser or on OpenHarmony.</description></item>
    /// </list>
    /// </remarks>
    public Task<CommandCompletion> SetHttpHeaderTransformAsync(HttpHeaderTransform transform)
    {
        using var scope = new NativeCallScope(this);
        var nativeTransform = NativeHttpHeaderTransform(transform, scope);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_runtime_set_http_header_transform(
            Handle,
            &nativeTransform,
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Limits the size of the runtime's ambient cache.
    /// </summary>
    /// <remarks>
    /// <para><c>size</c> is in bytes. Offline regions don't count toward it.
    /// The limit defaults to 50 MB. In the browser, a runtime without a cache
    /// file defaults to 0, because the browser's HTTP cache already holds what
    /// maps load.</para>
    /// <para>See <c>mln_runtime_set_maximum_ambient_cache_size</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.</para>
    /// </remarks>
    public Task SetMaximumAmbientCacheSizeAsync(ulong size)
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_runtime_set_maximum_ambient_cache_size(
            Handle,
            size,
            NativeCompletion.StartUnit(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Starts or pauses the download of the offline region <c>regionId</c>.
    /// </summary>
    /// <remarks>
    /// <para>Setting the state that the download already has does nothing. A
    /// download stops when the runtime closes.</para>
    /// <para>See <c>mln_runtime_set_offline_region_download_state</c> in the
    /// <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.NotFound</c> when the cache file has no
    /// region <c>regionId</c>.</description></item>
    /// </list>
    /// </remarks>
    public Task SetOfflineRegionDownloadStateAsync(long regionId, OfflineRegionDownloadState state)
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_runtime_set_offline_region_download_state(
            Handle,
            regionId,
            (mln_offline_region_download_state)state,
            NativeCompletion.StartUnit(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Selects the events that the runtime reports about the download of the
    /// offline region <c>regionId</c>.
    /// </summary>
    /// <remarks>
    /// <para><c>events</c> holds bits of
    /// <c>RuntimeEventFlag.AllRuntimeEvents</c> and replaces the region's
    /// previous selection. <c>RuntimeEventFlag.None</c> stops the region's
    /// events.</para>
    /// <para>See <c>mln_runtime_set_offline_region_observed</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.NotFound</c> when the cache file has no
    /// region <c>regionId</c>.</description></item>
    /// </list>
    /// </remarks>
    public Task SetOfflineRegionObservedAsync(long regionId, RuntimeEventFlag events)
    {
        using var scope = new NativeCallScope(this);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_runtime_set_offline_region_observed(
            Handle,
            regionId,
            (ulong)events,
            NativeCompletion.StartUnit(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Serves the runtime's network requests through a callback.
    /// </summary>
    /// <remarks>
    /// <para>The provider receives the network requests that its routes match,
    /// and the library loads the rest as usual. A provider can serve URL
    /// schemes that the library can't load itself. The provider replaces the
    /// one set before.</para>
    /// <para>See <c>mln_runtime_set_resource_provider</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.</para>
    /// </remarks>
    public Task<CommandCompletion> SetResourceProviderAsync(ResourceProvider provider)
    {
        using var scope = new NativeCallScope(this);
        var nativeProvider = NativeResourceProvider(provider, scope);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_runtime_set_resource_provider(
            Handle,
            &nativeProvider,
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Rewrites the URLs of the runtime's network requests.
    /// </summary>
    /// <remarks>
    /// <para>The transform replaces the one set before.</para>
    /// <para>See <c>mln_runtime_set_resource_transform</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.</para>
    /// </remarks>
    public Task<CommandCompletion> SetResourceTransformAsync(ResourceTransform transform)
    {
        using var scope = new NativeCallScope(this);
        var nativeTransform = NativeResourceTransform(transform, scope);
        mln_completion nativeCompletion;
        var nativeStatus = NativeMethods.mln_runtime_set_resource_transform(
            Handle,
            &nativeTransform,
            NativeCompletion.StartCommand(&nativeCompletion, out var nativePending),
            Diagnostic
        );
        return scope.Accepted(nativePending.Submitted(nativeStatus));
    }

    /// <summary>
    /// Replaces the metadata of the offline region <c>regionId</c>.
    /// </summary>
    /// <remarks>
    /// <para>See <c>mln_runtime_update_offline_region_metadata</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.NotFound</c> when the cache file has no
    /// region <c>regionId</c>.</description></item>
    /// </list>
    /// </remarks>
    public Task<OfflineRegionInfo> UpdateOfflineRegionMetadataAsync(
        long regionId,
        ReadOnlySpan<byte> metadata
    )
    {
        fixed (byte* pinnedMetadata = metadata)
        {
            using var scope = new NativeCallScope(this);
            mln_completion nativeCompletion;
            var nativeStatus = NativeMethods.mln_runtime_update_offline_region_metadata(
                Handle,
                regionId,
                (byte*)pinnedMetadata,
                checked((nuint)metadata.Length),
                NativeCompletion.Start<OfflineRegionInfo>(
                    &nativeCompletion,
                    static nativeResult =>
                        NativeCompletion.Single<mln_offline_region_info, OfflineRegionInfo>(
                            nativeResult,
                            CopyOfflineRegionInfo
                        ),
                    out var nativePending
                ),
                Diagnostic
            );
            return scope.Accepted(nativePending.Submitted(nativeStatus));
        }
    }

    /// <summary>
    /// Submits each command without a completion. Native logs a failure as
    /// a warning, and the caller sees none.
    /// </summary>
    public DiscardingCommands Discarding => new(this);

    /// <summary>
    /// The commands of a handle, each submitted without a completion.
    /// Native logs a failure as a warning, and the caller sees none.
    /// </summary>
    public readonly struct DiscardingCommands
    {
        private readonly RuntimeHandle owner;

        internal DiscardingCommands(RuntimeHandle owner)
        {
            this.owner = owner;
        }

        public void ClearEventHandler()
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            Check(NativeMethods.mln_runtime_clear_event_handler(receiverHandle, null, Diagnostic));
            scope.Accept();
        }

        public void ClearHttpHeaderTransform()
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            Check(
                NativeMethods.mln_runtime_clear_http_header_transform(
                    receiverHandle,
                    null,
                    Diagnostic
                )
            );
            scope.Accept();
        }

        public void ClearResourceProvider()
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            Check(
                NativeMethods.mln_runtime_clear_resource_provider(receiverHandle, null, Diagnostic)
            );
            scope.Accept();
        }

        public void ClearResourceTransform()
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            Check(
                NativeMethods.mln_runtime_clear_resource_transform(receiverHandle, null, Diagnostic)
            );
            scope.Accept();
        }

        public void SetEventHandler(RuntimeEventHandler handler)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            var nativeHandler = NativeRuntimeEventHandler(handler, scope);
            Check(
                NativeMethods.mln_runtime_set_event_handler(
                    receiverHandle,
                    &nativeHandler,
                    null,
                    Diagnostic
                )
            );
            scope.Accept();
        }

        public void SetHttpHeaderTransform(HttpHeaderTransform transform)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            var nativeTransform = NativeHttpHeaderTransform(transform, scope);
            Check(
                NativeMethods.mln_runtime_set_http_header_transform(
                    receiverHandle,
                    &nativeTransform,
                    null,
                    Diagnostic
                )
            );
            scope.Accept();
        }

        public void SetResourceProvider(ResourceProvider provider)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            var nativeProvider = NativeResourceProvider(provider, scope);
            Check(
                NativeMethods.mln_runtime_set_resource_provider(
                    receiverHandle,
                    &nativeProvider,
                    null,
                    Diagnostic
                )
            );
            scope.Accept();
        }

        public void SetResourceTransform(ResourceTransform transform)
        {
            var receiverHandle = owner.Handle;
            using var scope = new NativeCallScope(owner);
            var nativeTransform = NativeResourceTransform(transform, scope);
            Check(
                NativeMethods.mln_runtime_set_resource_transform(
                    receiverHandle,
                    &nativeTransform,
                    null,
                    Diagnostic
                )
            );
            scope.Accept();
        }
    }
}
