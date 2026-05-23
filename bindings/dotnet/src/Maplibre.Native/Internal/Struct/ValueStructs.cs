using System.Runtime.InteropServices;
using System.Text;
using Maplibre.Native.Error;
using Maplibre.Native.Internal.C;
using Maplibre.Native.Json;

namespace Maplibre.Native.Internal.Struct;

internal sealed unsafe class NativeStringView : IDisposable
{
    private readonly nint allocation;

    private NativeStringView(mln_string_view value, nint allocation)
    {
        Value = value;
        this.allocation = allocation;
    }

    internal mln_string_view Value { get; }

    internal static NativeStringView From(string value, string parameterName)
    {
        ArgumentNullException.ThrowIfNull(value, parameterName);
        var bytes = Encoding.UTF8.GetBytes(value);
        var allocation = bytes.Length == 0 ? 0 : (nint)NativeMemory.Alloc((nuint)bytes.Length);
        if (allocation != 0)
        {
            Marshal.Copy(bytes, 0, allocation, bytes.Length);
        }

        return new NativeStringView(
            new mln_string_view { data = (sbyte*)allocation, size = (nuint)bytes.Length },
            allocation);
    }

    public void Dispose()
    {
        if (allocation != 0)
        {
            NativeMemory.Free((void*)allocation);
        }
    }
}

internal sealed unsafe class NativeJsonValue : IDisposable
{
    private readonly List<nint> allocations = [];

    private NativeJsonValue(mln_json_value* pointer)
    {
        Pointer = pointer;
    }

    internal mln_json_value* Pointer { get; }

    internal static NativeJsonValue From(JsonValue value)
    {
        ArgumentNullException.ThrowIfNull(value);
        var root = (mln_json_value*)NativeMemory.Alloc((nuint)sizeof(mln_json_value));
        var native = new NativeJsonValue(root);
        try
        {
            native.Write(Pointer: root, value, depth: 0);
            return native;
        }
        catch
        {
            native.Dispose();
            throw;
        }
    }

    private void Write(mln_json_value* Pointer, JsonValue value, int depth)
    {
        if (depth > JsonValue.MaxDepth)
        {
            throw new InvalidArgumentException(
                MaplibreStatus.InvalidArgument,
                null,
                $"JsonValue exceeds maximum depth {JsonValue.MaxDepth}.");
        }

        *Pointer = new mln_json_value { size = (uint)sizeof(mln_json_value) };
        switch (value)
        {
            case JsonValue.Null:
                Pointer->type = (uint)mln_json_value_type.MLN_JSON_VALUE_TYPE_NULL;
                break;
            case JsonValue.Bool boolean:
                Pointer->type = (uint)mln_json_value_type.MLN_JSON_VALUE_TYPE_BOOL;
                Pointer->data.bool_value = boolean.Value ? (byte)1 : (byte)0;
                break;
            case JsonValue.UInt unsigned:
                Pointer->type = (uint)mln_json_value_type.MLN_JSON_VALUE_TYPE_UINT;
                Pointer->data.uint_value = unsigned.Value;
                break;
            case JsonValue.Int integer:
                Pointer->type = (uint)mln_json_value_type.MLN_JSON_VALUE_TYPE_INT;
                Pointer->data.int_value = integer.Value;
                break;
            case JsonValue.Double number:
                if (double.IsNaN(number.Value) || double.IsInfinity(number.Value))
                {
                    throw new InvalidArgumentException(MaplibreStatus.InvalidArgument, null, "JsonValue.Double must be finite.");
                }

                Pointer->type = (uint)mln_json_value_type.MLN_JSON_VALUE_TYPE_DOUBLE;
                Pointer->data.double_value = number.Value;
                break;
            case JsonValue.String text:
                Pointer->type = (uint)mln_json_value_type.MLN_JSON_VALUE_TYPE_STRING;
                Pointer->data.string_value = AllocateStringView(text.Value);
                break;
            case JsonValue.Array array:
                Pointer->type = (uint)mln_json_value_type.MLN_JSON_VALUE_TYPE_ARRAY;
                Pointer->data.array_value = AllocateArray(array.Values, depth + 1);
                break;
            case JsonValue.Object obj:
                Pointer->type = (uint)mln_json_value_type.MLN_JSON_VALUE_TYPE_OBJECT;
                Pointer->data.object_value = AllocateObject(obj.Members, depth + 1);
                break;
            default:
                throw new InvalidArgumentException(MaplibreStatus.InvalidArgument, null, $"Unsupported JsonValue type {value.GetType().Name}.");
        }
    }

    private mln_json_array AllocateArray(IReadOnlyList<JsonValue> values, int depth)
    {
        if (values.Count == 0)
        {
            return default;
        }

        var pointer = Allocate<mln_json_value>(values.Count);
        for (var index = 0; index < values.Count; index++)
        {
            Write(&pointer[index], values[index], depth);
        }

        return new mln_json_array { values = pointer, value_count = (nuint)values.Count };
    }

    private mln_json_object AllocateObject(IReadOnlyList<JsonMember> members, int depth)
    {
        if (members.Count == 0)
        {
            return default;
        }

        var memberPointer = Allocate<mln_json_member>(members.Count);
        var valuePointer = Allocate<mln_json_value>(members.Count);
        for (var index = 0; index < members.Count; index++)
        {
            memberPointer[index].key = AllocateStringView(members[index].Key);
            memberPointer[index].value = &valuePointer[index];
            Write(&valuePointer[index], members[index].Value, depth);
        }

        return new mln_json_object { members = memberPointer, member_count = (nuint)members.Count };
    }

    private mln_string_view AllocateStringView(string value)
    {
        var bytes = Encoding.UTF8.GetBytes(value);
        if (bytes.Length == 0)
        {
            return default;
        }

        var allocation = NativeMemory.Alloc((nuint)bytes.Length);
        allocations.Add((nint)allocation);
        Marshal.Copy(bytes, 0, (nint)allocation, bytes.Length);
        return new mln_string_view { data = (sbyte*)allocation, size = (nuint)bytes.Length };
    }

    private T* Allocate<T>(int count) where T : unmanaged
    {
        var allocation = NativeMemory.AllocZeroed((nuint)(sizeof(T) * count));
        allocations.Add((nint)allocation);
        return (T*)allocation;
    }

    public void Dispose()
    {
        foreach (var allocation in allocations)
        {
            NativeMemory.Free((void*)allocation);
        }

        if (Pointer is not null)
        {
            NativeMemory.Free(Pointer);
        }
    }
}
