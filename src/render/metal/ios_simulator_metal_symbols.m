#include <TargetConditionals.h>

#if TARGET_OS_SIMULATOR
const void* MTLIOErrorDomain = 0;
const void* MTLTensorDomain = 0;
#endif
