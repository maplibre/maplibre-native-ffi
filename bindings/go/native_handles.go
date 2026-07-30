package maplibre

// Distinct Go types over the C handle id. The C API spells every handle as the
// same uint64, so these are what keep a map from being passed where a runtime
// is expected.
type (
	nativeRuntime       uint64
	nativeMap           uint64
	nativeRenderSession uint64
	nativeWakeSource    uint64
)
