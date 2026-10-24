using System.Diagnostics;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Status;

namespace Maplibre.NativeFfi;

/// <summary>The terminal outcome of an accepted ordered command.</summary>
public readonly record struct CommandCompletion(
    CommandDisposition Disposition,
    ulong Generation,
    ulong RenderUpdateGeneration,
    int RawStatus,
    string Diagnostic
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

    /// <summary>Copies each element of the result's array, which native lays out value_size bytes apart.</summary>
    internal static T[] Values<T>(mln_completion_result* result)
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
        if (result->value_size < sizeof(T))
        {
            throw new InvalidOperationException(
                "Native completion stride is below the element size."
            );
        }
        var values = new T[checked((int)result->value_count)];
        var bytes = (byte*)result->value;
        for (var index = 0; index < values.Length; index++)
            values[index] = Unsafe.ReadUnaligned<T>(
                bytes + checked((nuint)index * result->value_size)
            );
        return values;
    }

    /// <summary>Copies the one value of a result.</summary>
    internal static T Single<TNative, T>(mln_completion_result* result, Func<TNative, T> copy)
        where TNative : unmanaged => copy(Value<TNative>(result));

    /// <summary>Copies the value of a result that may hold none.</summary>
    internal static T? Optional<TNative, T>(mln_completion_result* result, Func<TNative, T?> copy)
        where TNative : unmanaged
        where T : class => result->value_count == 0 ? null : copy(Value<TNative>(result));

    /// <inheritdoc cref="Optional{TNative, T}"/>
    internal static T? OptionalValue<TNative, T>(
        mln_completion_result* result,
        Func<TNative, T> copy
    )
        where TNative : unmanaged
        where T : struct => result->value_count == 0 ? null : copy(Value<TNative>(result));

    /// <summary>Copies each value of a result's array.</summary>
    internal static T[] Array<TNative, T>(mln_completion_result* result, Func<TNative, T> copy)
        where TNative : unmanaged
    {
        var values = Values<TNative>(result);
        var copied = new T[values.Length];
        for (var index = 0; index < values.Length; index++)
            copied[index] = copy(values[index]);
        return copied;
    }

    /// <summary>Copies each value of a result's array, which may be absent.</summary>
    internal static T[]? OptionalArray<TNative, T>(
        mln_completion_result* result,
        Func<TNative, T> copy
    )
        where TNative : unmanaged => result->value == null ? null : Array(result, copy);

    /// <summary>Starts a command completion, which reports a failure as its disposition.</summary>
    internal static mln_completion* StartCommand(
        mln_completion* completion,
        CancellationToken cancellationToken,
        out Pending<CommandCompletion> pending
    ) =>
        Start(
            completion,
            static result => new CommandCompletion(
                (CommandDisposition)result->disposition,
                result->generation,
                result->render_update_generation,
                result->status,
                ValueStructs.CopyUtf8View(result->diagnostic)
            ),
            cancellationToken,
            out pending,
            acceptErrorStatus: true
        );

    /// <summary>Starts a completion that carries no value.</summary>
    internal static mln_completion* StartUnit(
        mln_completion* completion,
        CancellationToken cancellationToken,
        out Pending<bool> pending
    ) => Start(completion, static _ => true, cancellationToken, out pending);

    /// <summary>
    /// Fills <paramref name="completion"/> for one native call and returns it.
    /// The caller passes the result to <see cref="Pending{T}.Submitted"/>.
    /// </summary>
    /// <remarks>
    /// A generated call starts its completion as the last argument that can
    /// throw, so a started completion always reaches its submission. Cancelling
    /// <paramref name="cancellationToken"/> cancels the task, not the native
    /// work, which continues to its terminal disposition. A value that arrives
    /// after cancellation has no receiver, so a disposable one is disposed.
    /// </remarks>
    internal static mln_completion* Start<T>(
        mln_completion* completion,
        CompletionConverter<T> convert,
        CancellationToken cancellationToken,
        out Pending<T> pending,
        bool acceptErrorStatus = false
    )
    {
        var state = new State<T>(convert, acceptErrorStatus, cancellationToken);
        var root = GCHandle.Alloc(state);
        *completion = new mln_completion
        {
            size = (uint)sizeof(mln_completion),
            callback = &Complete,
            user_data = (void*)GCHandle.ToIntPtr(root),
            release_user_data = &Release,
        };
        pending = new Pending<T>(state, root);
        return completion;
    }

    /// <summary>A started completion, until the call that took it returns.</summary>
    internal readonly struct Pending<T>
    {
        private readonly State<T> state;
        private readonly GCHandle root;

        internal Pending(State<T> state, GCHandle root)
        {
            this.state = state;
            this.root = root;
        }

        /// <summary>
        /// Returns the task of an accepted call. A rejected call never runs its
        /// completion, so this releases it and throws the call's status.
        /// </summary>
        internal Task<T> Submitted(mln_status status)
        {
            if (status == mln_status.MLN_STATUS_OK)
                return state.Task;
            state.Reject();
            root.Free();
            NativeStatus.Check(status, NativeDiagnostic.Current);
            throw new UnreachableException();
        }
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

    internal abstract class StateBase
    {
        internal abstract void Complete(mln_completion_result* result);
    }

    internal sealed class State<T> : StateBase
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
                if (!acceptErrorStatus && result->status != (int)mln_status.MLN_STATUS_OK)
                    NativeStatus.Check(
                        result->status,
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
