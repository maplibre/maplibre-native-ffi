using Maplibre.NativeFfi.Map;

namespace Maplibre.NativeFfi.Camera;

/// <summary>Camera change kinds reported in the raw <c>Code</c> field of camera
/// will-change and did-change runtime events.</summary>
public enum CameraChangeMode : uint
{
    /// <summary>The camera reached its new value without an animated transition.</summary>
    Immediate = 0,

    /// <summary>The camera moved as part of an animated transition.</summary>
    Animated = 1,
}
