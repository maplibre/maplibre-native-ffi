using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Status;

namespace Maplibre.NativeFfi.Runtime;

/// <summary>The terminal outcome of an accepted ordered command.</summary>
public readonly record struct CommandCompletion(
    CommandDisposition Disposition,
    ulong Generation,
    int RawStatus,
    string Diagnostic
);

internal unsafe delegate mln_status CompletionSubmit(mln_completion* completion);
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

    internal static Task<CommandCompletion> SubmitCommand(CompletionSubmit submit) =>
        Submit(
            submit,
            static result => new CommandCompletion(
                (CommandDisposition)result->disposition,
                result->generation,
                (int)result->status,
                StateBase.Diagnostic(result->diagnostic)
            ),
            true
        );

    internal static Task SubmitUnit(CompletionSubmit submit) => Submit(submit, static _ => true);

    internal static Task<T> Submit<T>(
        CompletionSubmit submit,
        CompletionConverter<T> convert,
        bool acceptErrorStatus = false
    )
    {
        var state = new State<T>(convert, acceptErrorStatus);
        var root = GCHandle.Alloc(state);
        var completion = new mln_completion
        {
            size = (uint)sizeof(mln_completion),
            callback = &Complete,
            user_data = (void*)GCHandle.ToIntPtr(root),
            release_user_data = &Release,
        };
        mln_status status;
        try
        {
            status = submit(&completion);
        }
        catch
        {
            root.Free();
            throw;
        }
        if (status != mln_status.MLN_STATUS_OK)
        {
            root.Free();
            NativeStatus.Check(status);
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

        internal static string Diagnostic(mln_buffer_view view) =>
            view.data is null || view.size == 0
                ? string.Empty
                : Marshal.PtrToStringUTF8((nint)view.data, checked((int)view.size)) ?? string.Empty;
    }

    private sealed class State<T>(CompletionConverter<T> convert, bool acceptErrorStatus)
        : StateBase
    {
        private readonly TaskCompletionSource<T> source = new(
            TaskCreationOptions.RunContinuationsAsynchronously
        );

        internal Task<T> Task => source.Task;

        internal override void Complete(mln_completion_result* result)
        {
            try
            {
                if (!acceptErrorStatus)
                {
                    NativeStatus.Check((int)result->status, Diagnostic(result->diagnostic));
                }
                source.TrySetResult(convert(result));
            }
            catch (Exception error)
            {
                source.TrySetException(error);
            }
        }
    }
}
