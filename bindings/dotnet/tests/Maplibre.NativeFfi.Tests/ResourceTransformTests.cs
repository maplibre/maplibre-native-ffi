using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Memory;
using Maplibre.NativeFfi.Internal.Struct;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed class ResourceTransformTests
{
    [Fact]
    public unsafe void AScopedResponseExpiresWithItsCallbackAndRejectsOtherThreads()
    {
        ResourceTransformResponse? retained = null;
        string? copied = null;
        Exception? crossThread = null;
        using var scope = new NativeCallScope();
        var native = GeneratedValues.NativeResourceTransform(
            new ResourceTransform
            {
                Callback = (_, url, response) =>
                {
                    copied = url;
                    retained = response;
                    var otherThread = new Thread(() =>
                        crossThread = Record.Exception(() => response.SetUrl("other"))
                    );
                    otherThread.Start();
                    otherThread.Join();
                    throw new FormatException("Host callback failed.");
                },
            },
            scope
        );
        var response = new mln_resource_transform_response
        {
            size = (uint)sizeof(mln_resource_transform_response),
        };
        var url = scope.CString("transform-test://é/style.json");
        Assert.Equal(
            mln_status.MLN_STATUS_NATIVE_ERROR,
            native.callback(native.user_data, 1, url, &response)
        );
        Assert.Equal("transform-test://é/style.json", copied);
        Assert.IsType<InvalidOperationException>(crossThread);
        Assert.NotNull(retained);
        Assert.Throws<InvalidOperationException>(() => retained.SetUrl("other"));
    }
}
