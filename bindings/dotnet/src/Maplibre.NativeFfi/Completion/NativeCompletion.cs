using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Status;

namespace Maplibre.NativeFfi;

/// <summary>The terminal outcome of an accepted ordered command.</summary>
public readonly record struct CommandCompletion(
    CommandDisposition Disposition,
    ulong Generation,
    int RawStatus,
    string Diagnostic
);

internal unsafe delegate mln_status CompletionSubmit(
    mln_completion* completion,
    mln_diagnostic* diagnostic
);
internal unsafe delegate T CompletionConverter<T>(mln_completion_result* result);

internal static unsafe class NativeCompletion
{
    internal static T Value<T>(mln_completion_result* result)
        where T : unmanaged
    {
        if (result->value is null || result->value_count != 1)
        {
            throw new InvalidOperationException("Native completion returned no value.");
        }
        return *(T*)result->value;
    }

    internal static ReadOnlySpan<T> Values<T>(mln_completion_result* result)
        where T : unmanaged
    {
        if (result->value_count == 0)
        {
            return [];
        }
        if (result->value is null)
        {
            throw new InvalidOperationException("Native completion returned a null array.");
        }
        return new ReadOnlySpan<T>(result->value, checked((int)result->value_count));
    }

    internal static Task<CommandCompletion> SubmitCommand(
        CompletionSubmit submit,
        CancellationToken cancellationToken
    ) =>
        Submit(
            submit,
            static result => new CommandCompletion(
                (CommandDisposition)result->disposition,
                result->generation,
                (int)result->status,
                ValueStructs.CopyUtf8View(result->diagnostic)
            ),
            cancellationToken,
            true
        );

    internal static Task SubmitUnit(CompletionSubmit submit) =>
        Submit(submit, static _ => true, CancellationToken.None);

    /// <summary>
    /// Submits a completion-based call and returns the task it resolves.
    /// </summary>
    /// <remarks>
    /// Cancelling <paramref name="cancellationToken"/> cancels the task, not
    /// the native work, which continues to its terminal disposition. A value
    /// that arrives after cancellation has no receiver, so a disposable one is
    /// disposed.
    /// </remarks>
    internal static Task<T> Submit<T>(
        CompletionSubmit submit,
        CompletionConverter<T> convert,
        CancellationToken cancellationToken,
        bool acceptErrorStatus = false
    )
    {
        var state = new State<T>(convert, acceptErrorStatus, cancellationToken);
        var root = GCHandle.Alloc(state);
        var completion = new mln_completion
        {
            size = (uint)sizeof(mln_completion),
            callback = &Complete,
            user_data = (void*)GCHandle.ToIntPtr(root),
            release_user_data = &Release,
        };
        mln_diagnostic diagnostic;
        mln_status status;
        try
        {
            status = submit(&completion, NativeDiagnostic.Prepare(&diagnostic));
        }
        catch
        {
            state.Reject();
            root.Free();
            throw;
        }
        if (status != mln_status.MLN_STATUS_OK)
        {
            state.Reject();
            root.Free();
            NativeStatus.Check(status, &diagnostic);
        }
        return state.Task;
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    private static void Complete(void* userData, mln_completion_result* result)
    {
        try
        {
            ((StateBase)GCHandle.FromIntPtr((nint)userData).Target!).Complete(result);
        }
        catch
        {
            // Exceptions never cross the C boundary. StateBase records converter failures.
        }
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    private static void Release(void* userData)
    {
        GCHandle.FromIntPtr((nint)userData).Free();
    }

    private abstract class StateBase
    {
        internal abstract void Complete(mln_completion_result* result);
    }

    private sealed class State<T> : StateBase
    {
        private readonly CompletionConverter<T> convert;
        private readonly bool acceptErrorStatus;
        private readonly TaskCompletionSource<T> source = new(
            TaskCreationOptions.RunContinuationsAsynchronously
        );
        private readonly CancellationTokenRegistration cancellation;

        // Registering before submission leaves the completion nothing to race:
        // native code calls Complete only after this constructor returns.
        internal State(
            CompletionConverter<T> convert,
            bool acceptErrorStatus,
            CancellationToken cancellationToken
        )
        {
            this.convert = convert;
            this.acceptErrorStatus = acceptErrorStatus;
            cancellation = cancellationToken.Register(
                static (state, token) => ((TaskCompletionSource<T>)state!).TrySetCanceled(token),
                source
            );
        }

        internal Task<T> Task => source.Task;

        internal void Reject() => cancellation.Dispose();

        internal override void Complete(mln_completion_result* result)
        {
            cancellation.Dispose();
            T value;
            try
            {
                if (!acceptErrorStatus && result->status != mln_status.MLN_STATUS_OK)
                    NativeStatus.Check(
                        (int)result->status,
                        ValueStructs.CopyUtf8View(result->diagnostic)
                    );
                value = convert(result);
            }
            catch (Exception error)
            {
                source.TrySetException(error);
                return;
            }
            // A cancelled wait leaves the value to no one, so an owned handle
            // is disposed now rather than by its finalizer.
            if (!source.TrySetResult(value))
                (value as IDisposable)?.Dispose();
        }
    }
}
