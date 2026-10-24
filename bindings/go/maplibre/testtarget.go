package maplibre

// OpenHarmony builds with GOOS=linux, so the C compiler identifies the target.
// A _test.go file cannot use cgo; untagged tests read the constant from here.

/*
#if defined(__OHOS__)
#define MLN_GO_TEST_HEADER_TRANSFORM_SUPPORTED 0
#else
#define MLN_GO_TEST_HEADER_TRANSFORM_SUPPORTED 1
#endif
*/
import "C"

const headerTransformSupported = C.MLN_GO_TEST_HEADER_TRANSFORM_SUPPORTED != 0
