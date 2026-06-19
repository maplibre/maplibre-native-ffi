#include <TargetConditionals.h>

#if TARGET_OS_SIMULATOR
__attribute__((weak)) const void* MTLIOErrorDomain = 0;
__attribute__((weak)) const void* MTLTensorDomain = 0;
#endif
