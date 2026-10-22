using System.Diagnostics;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using Maplibre.NativeFfi.Error;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

/// <summary>
/// Native registrations whose functions are <c>UnmanagedCallersOnly</c> methods, so their
/// addresses are real C function pointers.
/// </summary>
public sealed unsafe class NativeRegistrationTests
{
    public static TheoryData<TestGraphicsBackend> Backends => RenderFixture.Backends;

    // The attach consumes both wakes. Native code calls them as C functions, and releases each
    // context once after the session closes. A rejected attach releases its context in the
    // binding, and a copy of a consumed wake fails before it reaches native code.
    [Theory]
    [MemberData(nameof(Backends))]
    public void NativeWakesFireAndNativeCodeReleasesTheirContexts(TestGraphicsBackend backend)
    {
        var frame = new Host();
        var driverWork = new Host();
        var attached = default(RenderSessionAttachOptions);
        RenderFixture.Run(
            backend,
            (wake, options) =>
            {
                frame.Wake = wake;
                driverWork.Wake = wake;
                attached = options with
                {
                    FrameWake = Wake.Native(Function(&OnWake), frame.Context, Function(&OnRelease)),
                    DriverWorkWake = Wake.Native(
                        Function(&OnWake),
                        driverWork.Context,
                        Function(&OnRelease)
                    ),
                };
                return attached;
            },
            fixture =>
            {
                fixture.Await(fixture.Session.Completion, "the attachment");
                fixture.RenderFrame();
                var elapsed = Stopwatch.StartNew();
                TestWaits.Wait(frame.Called, elapsed, "the frame wake");
                TestWaits.Wait(driverWork.Called, elapsed, "the driver-work wake");

                // A wake without a callback must not carry a release, so native code rejects
                // the attach, and the binding releases the context of the rejected call.
                var rejected = new Host();
                var rejection = Assert.Throws<MaplibreException>(() =>
                    fixture.Attach(
                        attached with
                        {
                            FrameWake = Wake.Native(
                                NativePointer.Null,
                                rejected.Context,
                                Function(&OnRelease)
                            ),
                            DriverWorkWake = default,
                        }
                    )
                );
                Assert.Equal(Status.InvalidArgument, rejection.Status);
                Assert.Equal(1, rejected.Releases);
                rejected.Free();

                var reuse = Assert.Throws<MaplibreException>(() => fixture.Attach(attached));
                Assert.Equal(Status.InvalidState, reuse.Status);
                Assert.Equal(0, frame.Releases);
                Assert.Equal(0, driverWork.Releases);
            }
        );
        frame.WaitForRelease();
        driverWork.WaitForRelease();
    }

    // Only a Vulkan target takes a queue lock, and another target fails the attach with native
    // code's unsupported status. Either way, the context is released once: by native code after the
    // session closes, or by the binding for the rejected attach.
    [Theory]
    [MemberData(nameof(Backends))]
    public void ANativeQueueLockReachesNativeCodeAndItsContextIsReleasedOnce(
        TestGraphicsBackend backend
    )
    {
        var host = new Host();
        RenderFixture.Configure configure = (_, options) =>
            options with
            {
                QueueLock = QueueLock.Native(
                    Function(&OnLock),
                    Function(&OnUnlock),
                    host.Context,
                    Function(&OnRelease)
                ),
            };
        if (backend != TestGraphicsBackend.Vulkan)
        {
            var rejection = Assert.Throws<MaplibreException>(() =>
                RenderFixture.Run(backend, configure, _ => { })
            );
            Assert.Equal(Status.Unsupported, rejection.Status);
            Assert.Equal(1, host.Releases);
            Assert.Equal(0, host.Locks);
            host.Free();
            return;
        }
        RenderFixture.Run(
            backend,
            configure,
            fixture =>
            {
                fixture.Await(fixture.Session.Completion, "the attachment");
                fixture.RenderFrame();
                Assert.True(host.Locks > 0);
                Assert.Equal(0, host.Releases);
            }
        );
        host.WaitForRelease();
    }

    private static NativePointer Function(delegate* unmanaged[Cdecl]<void*, void> function) =>
        NativePointer.FromBorrowedAddress((nint)function);

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    private static void OnWake(void* context)
    {
        var host = Host.From(context);
        host.Called.Release();
        host.Wake();
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    private static void OnLock(void* context) =>
        Interlocked.Increment(ref Host.From(context).Locks);

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    private static void OnUnlock(void* context) { }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    private static void OnRelease(void* context)
    {
        var host = Host.From(context);
        Interlocked.Increment(ref host.Releases);
        host.Released.Release();
    }

    /// <summary>The state behind one registration's context, a handle to this object.</summary>
    private sealed class Host
    {
        private readonly GCHandle handle;
        internal int Locks;
        internal int Releases;

        internal Host() => handle = GCHandle.Alloc(this);

        internal NativePointer Context =>
            NativePointer.FromBorrowedAddress(GCHandle.ToIntPtr(handle));

        internal Action Wake { get; set; } = static () => { };

        internal SemaphoreSlim Called { get; } = new(0);

        internal SemaphoreSlim Released { get; } = new(0);

        internal static Host From(void* context) =>
            (Host)GCHandle.FromIntPtr((nint)context).Target!;

        /// <summary>Waits for native code to release the context, which it does once.</summary>
        internal void WaitForRelease()
        {
            TestWaits.Wait(Released, Stopwatch.StartNew(), "the context release");
            Assert.Equal(1, Volatile.Read(ref Releases));
            Free();
        }

        // Native code holds the context until it releases it, so only a released host frees it.
        internal void Free() => handle.Free();
    }
}
