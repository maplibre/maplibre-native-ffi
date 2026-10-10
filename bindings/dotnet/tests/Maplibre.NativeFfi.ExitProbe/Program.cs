// Leaves a runtime, a map, a held resource request, and process-global callbacks live, then
// returns from Main. The test suite runs this and expects a clean exit: native threads still
// running at exit must not call into a runtime that is shutting down.
using Maplibre.NativeFfi;
using NativeMaplibre = Maplibre.NativeFfi.Maplibre;

NativeMaplibre.LoadNativeLibrary();
NativeMaplibre.LogSetAsyncSeverityMask(LogSeverityMask.All);
NativeMaplibre.LogSetCallback(new LogHandler((_, _, _, _) => 0));

var held = new TaskCompletionSource<ResourceRequestHandle>(
    TaskCreationOptions.RunContinuationsAsynchronously
);
var runtime = RuntimeHandle.Create(RuntimeOptions.Default with { EventWake = new Wake(() => { }) });
await runtime.SetResourceProviderAsync(
    new ResourceProvider(
        (_, request) =>
        {
            request.SetCancelCallback(new ResourceRequestCancelHandler(() => { }));
            held.TrySetResult(request);
            return ResourceProviderDecision.Handle;
        }
    )
);
var map = await runtime.CreateMapAsync(MapOptions.Default);
await map.SetStyleUrlAsync("exit-probe://style.json");
var request = await held.Task.WaitAsync(TimeSpan.FromSeconds(30));

Console.WriteLine($"Exiting with live handles {runtime.Id:x}, {map.Id:x}, and {request.Id:x}.");
GC.KeepAlive(runtime);
GC.KeepAlive(map);
GC.KeepAlive(request);
return 0;
