package maplibre

// Distinct Go types over the C handle id, which is the same uint64 for every
// handle kind.
type (
	nativeRuntime       uint64
	nativeMap           uint64
	nativeRenderSession uint64
)
