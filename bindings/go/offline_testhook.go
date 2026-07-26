package maplibre

/*
#include "maplibre_native_c.h"
*/
import "C"

func startOfflineOperationZeroIDForTest(runtime *RuntimeHandle) (*OfflineOperationHandle[struct{}], error) {
	return startOfflineOperation[struct{}](
		runtime,
		OfflineOperationRegionSetObserved,
		OfflineOperationResultNone,
		func(ptr *nativeRuntime, out *C.mln_offline_operation_id) int32 {
			*out = 0
			return 0
		},
	)
}
