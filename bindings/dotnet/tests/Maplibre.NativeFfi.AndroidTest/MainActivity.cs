using Android.App;
using Android.OS;
using Android.Util;
using Java.Interop;

namespace Maplibre.NativeFfi.Tests;

[Activity(MainLauncher = true, Exported = true)]
public sealed class MainActivity : Activity
{
    protected override void OnCreate(Bundle? savedInstanceState)
    {
        base.OnCreate(savedInstanceState);
        // The environment belongs to this thread; initialization borrows it and the Context.
        MaplibreNative.InitAndroid(
            NativePointer.FromBorrowedAddress(JniEnvironment.EnvironmentPointer),
            NativePointer.FromBorrowedAddress(ApplicationContext!.PeerReference.Handle)
        );
        System.Environment.SetEnvironmentVariable("MLN_TEST_TIMEOUT_SCALE", "3");
        _ = Task.Run(RunTests);
    }

    private async Task RunTests()
    {
        var status = await MobileTestRunner.Run(line => Log.Info("MaplibreDotnetTests", line));
        Log.Info("MaplibreDotnetTests", $"__MLN_DOTNET_TEST_STATUS__={status}");
        RunOnUiThread(Finish);
    }
}
