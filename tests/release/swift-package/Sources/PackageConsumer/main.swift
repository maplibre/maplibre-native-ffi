import MaplibreNativeC
import MaplibreNativeFFI

precondition(cVersion() == mln_c_version())
// The package task checks which backend the package's traits linked.
print(supportedRenderBackendMask().rawValue)
