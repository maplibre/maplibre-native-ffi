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

public sealed unsafe partial class HttpHeaderTransformResponse
{
    public void Set(string name, string value)
    {
        using var scope = new NativeCallScope();
        global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
            this,
            "mln_http_header_transform_response_set"
        );
        var bufferName = scope.Utf8(name);
        var bufferValue = scope.Utf8(value);
        mln_diagnostic diagnostic;
        NativeStatus.Check(
            NativeMethods.mln_http_header_transform_response_set(
                Pointer,
                (sbyte*)bufferName.data,
                checked((nuint)bufferName.size),
                (sbyte*)bufferValue.data,
                checked((nuint)bufferValue.size),
                NativeDiagnostic.Prepare(&diagnostic)
            ),
            &diagnostic
        );
        scope.Accept();
    }
}
