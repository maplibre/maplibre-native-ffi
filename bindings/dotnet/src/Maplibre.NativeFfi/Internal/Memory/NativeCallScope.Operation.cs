using System.Runtime.CompilerServices;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Loader;
using Maplibre.NativeFfi.Internal.Pointer;

namespace Maplibre.NativeFfi.Internal.Memory;

/// <summary>
/// The scope of one generated operation. It keeps the receiver and handle
/// arguments reachable until disposal and submits completions, accepting its
/// registrations once native code admits the operation.
/// </summary>
internal sealed unsafe partial class NativeCallScope
{
    private readonly INativeOwner? owner;
    private List<object>? arguments;

    internal NativeCallScope(INativeOwner? owner)
    {
        if (owner is null)
            NativeLibraryLoader.EnsureLoaded();
        this.owner = owner;
    }

    /// <summary>
    /// Keeps an owner argument reachable for this call and returns its raw id.
    /// </summary>
    internal T Use<T>(
        INativeOwner<T>? argument,
        [CallerArgumentExpression(nameof(argument))] string? name = null
    )
        where T : unmanaged, IMlnHandle
    {
        ArgumentNullException.ThrowIfNull(argument, name);
        (arguments ??= []).Add(argument);
        return argument.State.Handle;
    }

    internal Task<CommandCompletion> Command(
        CompletionSubmit submit,
        CancellationToken cancellationToken
    ) => Accepted(NativeCompletion.SubmitCommand(submit, cancellationToken));

    internal void Discard(CompletionSubmit submit)
    {
        NativeCall.Check(submit(null, NativeCall.Diagnostic));
        Accept();
    }

    internal Task Run(CompletionSubmit submit, CancellationToken cancellationToken) =>
        Accepted(NativeCompletion.Submit(submit, static _ => true, cancellationToken));

    /// <summary>
    /// Submits an operation whose completion carries one value. A disposable
    /// value that arrives after cancellation is disposed.
    /// </summary>
    internal Task<T> Query<TNative, T>(
        CompletionSubmit submit,
        Func<TNative, T> copy,
        CancellationToken cancellationToken
    )
        where TNative : unmanaged =>
        Accepted(
            NativeCompletion.Submit(
                submit,
                result => copy(NativeCompletion.Value<TNative>(result)),
                cancellationToken
            )
        );

    internal Task<T?> QueryOptional<TNative, T>(
        CompletionSubmit submit,
        Func<TNative, T?> copy,
        CancellationToken cancellationToken
    )
        where TNative : unmanaged
        where T : class =>
        Accepted(
            NativeCompletion.Submit(
                submit,
                result =>
                    result->value_count == 0 ? null : copy(NativeCompletion.Value<TNative>(result)),
                cancellationToken
            )
        );

    internal Task<T?> QueryOptionalValue<TNative, T>(
        CompletionSubmit submit,
        Func<TNative, T> copy,
        CancellationToken cancellationToken
    )
        where TNative : unmanaged
        where T : struct =>
        Accepted(
            NativeCompletion.Submit(
                submit,
                result =>
                    result->value_count == 0
                        ? (T?)null
                        : copy(NativeCompletion.Value<TNative>(result)),
                cancellationToken
            )
        );

    internal Task<T[]> QueryArray<TNative, T>(
        CompletionSubmit submit,
        Func<TNative, T> copy,
        CancellationToken cancellationToken
    )
        where TNative : unmanaged =>
        Accepted(
            NativeCompletion.Submit(submit, result => CopyValues(result, copy), cancellationToken)
        );

    internal Task<T[]?> QueryOptionalArray<TNative, T>(
        CompletionSubmit submit,
        Func<TNative, T> copy,
        CancellationToken cancellationToken
    )
        where TNative : unmanaged =>
        Accepted(
            NativeCompletion.Submit(
                submit,
                result => result->value == null ? null : CopyValues(result, copy),
                cancellationToken
            )
        );

    internal TOwner Attach<TRaw, TOwner>(OutputSubmit<TRaw> submit, Func<TRaw, Task, TOwner> adopt)
        where TRaw : unmanaged
        where TOwner : INativeOwner
    {
        var output = Out<TRaw>();
        var attachment = NativeCompletion.SubmitUnit(
            (completion, diagnostic) => submit(output, completion, diagnostic)
        );
        Accept();
        return adopt(*output, attachment);
    }

    // Native releases accepted registrations when their callbacks are quiescent.
    private Task<T> Accepted<T>(Task<T> task)
    {
        Accept();
        return task;
    }

    private static T[] CopyValues<TNative, T>(mln_completion_result* result, Func<TNative, T> copy)
        where TNative : unmanaged
    {
        var values = NativeCompletion.Values<TNative>(result);
        var copied = new T[values.Length];
        for (var index = 0; index < values.Length; index++)
            copied[index] = copy(values[index]);
        return copied;
    }

    partial void EndOperation()
    {
        GC.KeepAlive(arguments);
        arguments = null;
        GC.KeepAlive(owner);
    }
}

internal unsafe delegate mln_status OutputSubmit<T>(
    T* output,
    mln_completion* completion,
    mln_diagnostic* diagnostic
)
    where T : unmanaged;
