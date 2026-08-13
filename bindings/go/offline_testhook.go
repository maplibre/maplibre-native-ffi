package maplibre

/*
#include "maplibre_native_c.h"
*/
import "C"

func startOperationZeroIDForTest(runtime *RuntimeHandle) (*OperationHandle[struct{}], error) {
	return startOperation[struct{}](
		runtime,
		operationRegionSetObserved,
		operationResultNone,
		func(handle nativeRuntime, out *C.mln_operation) int32 {
			*out = 0
			return 0
		},
	)
}
