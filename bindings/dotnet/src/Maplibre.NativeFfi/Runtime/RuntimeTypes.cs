using Maplibre.NativeFfi.Camera;
using Maplibre.NativeFfi.Error;
using Maplibre.NativeFfi.Internal;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Render;
using Maplibre.NativeFfi.Runtime;

namespace Maplibre.NativeFfi.Runtime;

public enum CommandDisposition : uint
{
    Committed = 0,
    Superseded = 1,
    Failed = 2,
    Cancelled = 3,
}
