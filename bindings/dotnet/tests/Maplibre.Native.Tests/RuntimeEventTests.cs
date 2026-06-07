using System.Runtime.CompilerServices;
using System.Text;
using Maplibre.Native.Internal.C;
using Maplibre.Native.Internal.Struct;
using Maplibre.Native.Render;
using Maplibre.Native.Runtime;
using Xunit;

namespace Maplibre.Native.Tests;

public sealed unsafe class RuntimeEventTests
{
    [Fact]
    public void CopiesRuntimeEventMessageAndRenderPayload()
    {
        var message = Encoding.UTF8.GetBytes("hello");
        fixed (byte* messagePointer = message)
        {
            var payload = new mln_runtime_event_render_map
            {
                size = (uint)Unsafe.SizeOf<mln_runtime_event_render_map>(),
                mode = (uint)mln_render_mode.MLN_RENDER_MODE_FULL,
            };
            var raw = RuntimeStructs.EmptyNativeEvent();
            raw.type = (uint)mln_runtime_event_type.MLN_RUNTIME_EVENT_MAP_RENDER_MAP_FINISHED;
            raw.source_type = (uint)mln_runtime_event_source_type.MLN_RUNTIME_EVENT_SOURCE_RUNTIME;
            raw.payload_type = (uint)
                mln_runtime_event_payload_type.MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP;
            raw.payload = &payload;
            raw.payload_size = (nuint)Unsafe.SizeOf<mln_runtime_event_render_map>();
            raw.message = (sbyte*)messagePointer;
            raw.message_size = (nuint)message.Length;

            var copied = RuntimeStructs.ReadEvent(raw);

            Assert.Equal(RuntimeEventType.MapRenderMapFinished, copied.Type);
            Assert.Equal("hello", copied.Message);
            Assert.Null(copied.RuntimeSource);
            Assert.Null(copied.MapSource);
            var renderMap = Assert.IsType<RuntimeEventPayload.RenderMap>(copied.Payload);
            Assert.Equal(RenderMode.Full, renderMap.Mode);
            Assert.Equal((uint)mln_render_mode.MLN_RENDER_MODE_FULL, renderMap.RawMode);
        }
    }

    [Fact]
    public void UndersizedKnownPayloadBecomesUnknown()
    {
        var payload = new mln_runtime_event_render_frame
        {
            size = (uint)Unsafe.SizeOf<mln_runtime_event_render_frame>(),
            mode = (uint)mln_render_mode.MLN_RENDER_MODE_FULL,
        };
        var raw = RuntimeStructs.EmptyNativeEvent();
        raw.payload_type = (uint)
            mln_runtime_event_payload_type.MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME;
        raw.payload = &payload;
        raw.payload_size = 1;

        var copied = RuntimeStructs.ReadEvent(raw);

        var unknown = Assert.IsType<RuntimeEventPayload.Unknown>(copied.Payload);
        Assert.Equal(
            (uint)mln_runtime_event_payload_type.MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME,
            unknown.RawPayloadType
        );
        Assert.Equal(1u, unknown.PayloadSize);
    }

    [Fact]
    public void EmptyRuntimePollReturnsNull()
    {
        NativeLibraryTestSupport.SkipUnlessNativeLibraryIsAvailable();
        using var runtime = RuntimeHandle.Create();

        Assert.Null(runtime.PollEvent());
    }
}
