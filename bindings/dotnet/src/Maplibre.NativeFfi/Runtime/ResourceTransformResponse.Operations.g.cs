// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using Maplibre.NativeFfi.Base;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Memory;
using Maplibre.NativeFfi.Internal.Pointer;
using Maplibre.NativeFfi.Internal.Status;
using Maplibre.NativeFfi.Internal.Struct;
using Maplibre.NativeFfi.Logging;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Query;
using Maplibre.NativeFfi.Render;
using Maplibre.NativeFfi.Runtime;
using Maplibre.NativeFfi.Style;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi.Runtime;

public sealed unsafe partial class ResourceTransformResponse
{
    public void SetUrl(string url)
    {
        using var scope = new NativeCallScope();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_resource_transform_response_set_url"
        );
        var bufferUrl = scope.Utf8(url);
        mln_diagnostic diagnostic;
        NativeStatus.Check(
            NativeMethods.mln_resource_transform_response_set_url(
                Pointer,
                (sbyte*)bufferUrl.data,
                checked((nuint)bufferUrl.size),
                NativeDiagnostic.Prepare(&diagnostic)
            ),
            &diagnostic
        );
        scope.Accept();
    }
}
