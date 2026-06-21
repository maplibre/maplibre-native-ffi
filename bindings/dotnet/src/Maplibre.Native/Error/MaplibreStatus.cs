namespace Maplibre.Native.Error;

/// <summary>Status category reported by MapLibre Native C API calls.</summary>
public enum MaplibreStatus
{
    /// <summary>The operation succeeded.</summary>
    Ok = 0,

    /// <summary>A pointer, size field, mask, or handle argument was invalid.</summary>
    InvalidArgument = -1,

    /// <summary>The object is valid but not currently in a state that permits the call.</summary>
    InvalidState = -2,

    /// <summary>The handle is thread-affine and the call ran on the wrong thread.</summary>
    WrongThread = -3,

    /// <summary>The entry point or requested behavior is unavailable in this build.</summary>
    Unsupported = -4,

    /// <summary>A native MapLibre error or C++ exception was converted to status.</summary>
    NativeError = -5,

    /// <summary>The loaded C ABI version is incompatible with this binding.</summary>
    AbiMismatch = -1000,

    /// <summary>The native library returned a status unknown to this binding.</summary>
    Unknown = int.MinValue,
}
