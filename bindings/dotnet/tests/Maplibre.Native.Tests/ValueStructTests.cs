using System.Runtime.InteropServices;
using Maplibre.Native.Internal.C;
using Maplibre.Native.Internal.Struct;
using Maplibre.Native.Json;
using Xunit;

namespace Maplibre.Native.Tests;

public sealed unsafe class ValueStructTests
{
    [BindingSpecTest("BND-067")]
    [Fact]
    public void JsonObjectPreservesOrderRepeatedNamesAndIntegerWidth()
    {
        using var native = NativeJsonValue.From(
            new JsonValue.Object([
                new JsonMember("value", new JsonValue.UInt(ulong.MaxValue)),
                new JsonMember("value", new JsonValue.Int(-1)),
                new JsonMember("other", new JsonValue.Int(long.MinValue)),
            ])
        );

        Assert.Equal((uint)mln_json_value_type.MLN_JSON_VALUE_TYPE_OBJECT, native.Pointer->type);
        var nativeObject = native.Pointer->data.object_value;
        Assert.Equal(3u, nativeObject.member_count);
        Assert.Equal("value", CopyKey(nativeObject.members[0]));
        Assert.Equal(
            (uint)mln_json_value_type.MLN_JSON_VALUE_TYPE_UINT,
            nativeObject.members[0].value->type
        );
        Assert.Equal(ulong.MaxValue, nativeObject.members[0].value->data.uint_value);
        Assert.Equal("value", CopyKey(nativeObject.members[1]));
        Assert.Equal(
            (uint)mln_json_value_type.MLN_JSON_VALUE_TYPE_INT,
            nativeObject.members[1].value->type
        );
        Assert.Equal(-1, nativeObject.members[1].value->data.int_value);
        Assert.Equal("other", CopyKey(nativeObject.members[2]));
        Assert.Equal(long.MinValue, nativeObject.members[2].value->data.int_value);

        var copied = Assert.IsType<JsonValue.Object>(ValueStructs.ReadJsonValue(native.Pointer));

        Assert.Collection(
            copied.Members,
            member =>
            {
                Assert.Equal("value", member.Key);
                Assert.Equal(new JsonValue.UInt(ulong.MaxValue), member.Value);
            },
            member =>
            {
                Assert.Equal("value", member.Key);
                Assert.Equal(new JsonValue.Int(-1), member.Value);
            },
            member =>
            {
                Assert.Equal("other", member.Key);
                Assert.Equal(new JsonValue.Int(long.MinValue), member.Value);
            }
        );
    }

    [BindingSpecTest("BND-066")]
    [Fact]
    public void JsonSnapshotIsDestroyedWhenCopyingValueFails()
    {
        var destroyCalls = 0;
        var value = (mln_json_value*)NativeMemory.Alloc((nuint)sizeof(mln_json_value));
        *value = new mln_json_value { size = (uint)sizeof(mln_json_value), type = 999 };
        try
        {
            using var methods = ValueStructs.UseJsonSnapshotMethodsForTest(
                (_, outValue) =>
                {
                    *outValue = value;
                    return mln_status.MLN_STATUS_OK;
                },
                _ => destroyCalls++
            );

            Assert.Throws<InvalidOperationException>(() =>
                ValueStructs.ReadJsonSnapshot((mln_json_snapshot*)1234)
            );
        }
        finally
        {
            NativeMemory.Free(value);
        }

        Assert.Equal(1, destroyCalls);
    }

    private static string CopyKey(mln_json_member member) =>
        Marshal.PtrToStringUTF8((nint)member.key.data, checked((int)member.key.size))
        ?? string.Empty;
}
