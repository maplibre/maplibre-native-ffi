using Maplibre.Native.Internal;

namespace Maplibre.Native.Json;

/// <summary>JSON-like value tree that preserves integer width and object order.</summary>
public abstract record JsonValue
{
    public const int MaxDepth = 64;

    private JsonValue() { }

    public sealed record Null : JsonValue
    {
        public static Null Instance { get; } = new();

        private Null() { }
    }

    public sealed record Bool(bool Value) : JsonValue;

    public sealed record UInt(ulong Value) : JsonValue;

    public sealed record Int(long Value) : JsonValue;

    public sealed record Double(double Value) : JsonValue;

    public sealed record String(string Value) : JsonValue;

    public sealed record Array(IReadOnlyList<JsonValue> Values) : JsonValue
    {
        public bool Equals(Array? other) =>
            other is not null && ValueEquality.SequenceEquals(Values, other.Values);

        public override int GetHashCode() => ValueEquality.SequenceHashCode(Values);
    }

    public sealed record Object(IReadOnlyList<JsonMember> Members) : JsonValue
    {
        public bool Equals(Object? other) =>
            other is not null && ValueEquality.SequenceEquals(Members, other.Members);

        public override int GetHashCode() => ValueEquality.SequenceHashCode(Members);
    }
}

/// <summary>JSON object member.</summary>
public readonly record struct JsonMember(string Key, JsonValue Value);
