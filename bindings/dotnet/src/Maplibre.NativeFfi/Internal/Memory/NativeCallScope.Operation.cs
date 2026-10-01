using System.Runtime.CompilerServices;
using Maplibre.NativeFfi.Error;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Pointer;
using Maplibre.NativeFfi.Runtime;

namespace Maplibre.NativeFfi.Internal.Memory;

/// <summary>
/// The scope of one generated operation. It checks that the calling thread may
/// enter the operation, keeps the receiver reachable and borrowed handles open
/// until disposal, and submits completions, accepting the scope's
/// registrations once native code admits the operation.
/// </summary>
internal sealed unsafe partial class NativeCallScope
{
    private readonly INativeOwner? owner;
    private List<INativeReader>? readers;

    /// <summary>Enters <paramref name="operation"/> on behalf of its receiver, or of no receiver.</summary>
    internal NativeCallScope(INativeOwner? owner, string operation)
    {
        NativeCall.Enter(owner, operation).Dispose();
        this.owner = owner;
    }

    /// <summary>
    /// Borrows the handle of an owner argument for this call, so the owner
    /// cannot release it while native code reads it.
    /// </summary>
    internal T Use<T>(
        INativeOwner<T>? argument,
        [CallerArgumentExpression(nameof(argument))] string? name = null
    )
        where T : unmanaged, IMlnHandle
    {
        ArgumentNullException.ThrowIfNull(argument, name);
        var state = argument.State;
        var handle = state.BeginRead();
        try
        {
            (readers ??= []).Add(state);
        }
        catch
        {
            state.EndRead();
            throw;
        }
        return handle;
    }

    /// <summary>Encodes a NUL-terminated string parameter, which cannot carry an embedded NUL.</summary>
    internal sbyte* CStringArgument(
        string text,
        [CallerArgumentExpression(nameof(text))] string? name = null
    )
    {
        ArgumentNullException.ThrowIfNull(text, name);
        if (text.Contains('\0'))
            throw new InvalidArgumentException(
                MaplibreStatus.InvalidArgument,
                null,
                $"{name} contains an embedded NUL character.",
                null
            );
        return Terminated(text);
    }

    /// <summary>Submits an ordered command.</summary>
    internal Task<CommandCompletion> Command(
        CompletionSubmit submit,
        CancellationToken cancellationToken
    ) => Accepted(NativeCompletion.SubmitCommand(submit)).WaitAsync(cancellationToken);

    /// <summary>Submits an operation that completes without a value.</summary>
    internal Task Run(CompletionSubmit submit, CancellationToken cancellationToken = default) =>
        Accepted(NativeCompletion.Submit(submit, static _ => true)).WaitAsync(cancellationToken);

    /// <summary>Submits an operation whose completion carries one value.</summary>
    internal Task<T> Query<TNative, T>(
        CompletionSubmit submit,
        Func<TNative, T> copy,
        CancellationToken cancellationToken = default
    )
        where TNative : unmanaged =>
        Accepted(
                NativeCompletion.Submit(
                    submit,
                    result => copy(NativeCompletion.Value<TNative>(result))
                )
            )
            .WaitAsync(cancellationToken);

    /// <summary>Submits an operation whose completion carries zero or one reference value.</summary>
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
                        result->value_count == 0
                            ? null
                            : copy(NativeCompletion.Value<TNative>(result))
                )
            )
            .WaitAsync(cancellationToken);

    /// <summary>Submits an operation whose completion carries zero or one value-type value.</summary>
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
                            : copy(NativeCompletion.Value<TNative>(result))
                )
            )
            .WaitAsync(cancellationToken);

    /// <summary>Submits an operation whose completion carries an array.</summary>
    internal Task<T[]> QueryArray<TNative, T>(
        CompletionSubmit submit,
        Func<TNative, T> copy,
        CancellationToken cancellationToken
    )
        where TNative : unmanaged =>
        Accepted(NativeCompletion.Submit(submit, result => CopyValues(result, copy)))
            .WaitAsync(cancellationToken);

    /// <summary>Submits an operation whose completion carries an array or a null array.</summary>
    internal Task<T[]?> QueryOptionalArray<TNative, T>(
        CompletionSubmit submit,
        Func<TNative, T> copy,
        CancellationToken cancellationToken
    )
        where TNative : unmanaged =>
        Accepted(
                NativeCompletion.Submit(
                    submit,
                    result => result->value == null ? null : CopyValues(result, copy)
                )
            )
            .WaitAsync(cancellationToken);

    /// <summary>
    /// Submits an attachment whose owner exists at once, then adopts the
    /// handle that native code wrote together with the attachment's completion.
    /// </summary>
    internal TOwner Attach<TRaw, TOwner>(OutputSubmit<TRaw> submit, Func<TRaw, Task, TOwner> adopt)
        where TRaw : unmanaged
        where TOwner : INativeOwner
    {
        var output = Out<TRaw>();
        var attachment = NativeCompletion.SubmitUnit(
            (completion, diagnostic) => submit(output, completion, diagnostic)
        );
        var created = adopt(*output, attachment);
        Accept(registrations is null ? null : created.CallbackOwner);
        return created;
    }

    // An operation's registrations live as long as its receiver, or the process
    // when it has none.
    private Task<T> Accepted<T>(Task<T> task)
    {
        Accept(registrations is null ? null : owner?.CallbackOwner);
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
        if (readers is not null)
            foreach (var reader in readers)
                reader.EndRead();
        readers = null;
        GC.KeepAlive(owner);
    }
}

/// <summary>Calls a C function that writes one output and takes a completion.</summary>
internal unsafe delegate mln_status OutputSubmit<T>(
    T* output,
    mln_completion* completion,
    mln_diagnostic* diagnostic
)
    where T : unmanaged;
