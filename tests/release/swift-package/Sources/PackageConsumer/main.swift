import MaplibreNativeC
import MaplibreNativeFFI

precondition(cVersion() == mln_c_version())
// The package task checks which backends the package linked.
print(supportedRenderBackendMask().rawValue)
