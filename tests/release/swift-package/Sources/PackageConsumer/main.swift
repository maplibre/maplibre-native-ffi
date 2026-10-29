import MaplibreNativeC
import MaplibreNativeFFI

precondition(cVersion() == mln_c_version())
precondition(supportedRenderBackendMask().rawValue != 0)
