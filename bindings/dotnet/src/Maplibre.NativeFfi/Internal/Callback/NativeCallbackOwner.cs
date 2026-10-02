namespace Maplibre.NativeFfi.Internal.Callback;

internal sealed class NativeCallbackOwner
{
    internal static NativeCallbackOwner Global { get; } = new();
    private readonly object gate = new();
    private NativeCallbackRoot? first;
    private bool retired;

    internal NativeCallbackOwner Add(NativeCallbackRoot root)
    {
        lock (gate)
        {
            if (retired)
                return Global.Add(root);
            root.Next = first;
            if (first is not null)
                first.Previous = root;
            first = root;
            return this;
        }
    }

    internal void Remove(NativeCallbackRoot root)
    {
        lock (gate)
        {
            if (root.Previous is null)
                first = root.Next;
            else
                root.Previous.Next = root.Next;
            if (root.Next is not null)
                root.Next.Previous = root.Previous;
            root.Previous = null;
            root.Next = null;
        }
    }

    // Retirement keeps callback values live until native quiescence releases each token.
    internal void Retire()
    {
        if (ReferenceEquals(this, Global))
            return;
        lock (gate)
            retired = true;
        while (true)
        {
            NativeCallbackRoot? root;
            lock (gate)
                root = first;
            if (root is null)
                return;
            root.Retain(Global);
        }
    }
}
