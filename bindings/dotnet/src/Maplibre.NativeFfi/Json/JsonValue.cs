using Maplibre.NativeFfi.Internal;

namespace Maplibre.NativeFfi.Json;

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
        private readonly IReadOnlyList<JsonValue> values = ValueEquality.Snapshot(Values);

        public IReadOnlyList<JsonValue> Values
        {
            get => values;
            init => values = ValueEquality.Snapshot(value);
        }

        public bool Equals(Array? other) =>
            other is not null && ValueEquality.SequenceEquals(values, other.values);

        public override int GetHashCode() => ValueEquality.SequenceHashCode(values);
    }

    public sealed record Object(IReadOnlyList<JsonMember> Members) : JsonValue
    {
        private readonly IReadOnlyList<JsonMember> members = ValueEquality.Snapshot(Members);

        public IReadOnlyList<JsonMember> Members
        {
            get => members;
            init => members = ValueEquality.Snapshot(value);
        }

        public bool Equals(Object? other) =>
            other is not null && ValueEquality.SequenceEquals(members, other.members);

        public override int GetHashCode() => ValueEquality.SequenceHashCode(members);
    }
}

/// <summary>JSON object member.</summary>
public readonly record struct JsonMember(string Key, JsonValue Value);
