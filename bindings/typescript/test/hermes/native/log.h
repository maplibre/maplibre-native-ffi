#pragma once

#include <cstdio>
#ifdef __ANDROID__
#include <android/log.h>
#endif

inline void testLog(const char* message) {
  std::fprintf(stderr, "%s\n", message);
#ifdef __ANDROID__
  __android_log_write(ANDROID_LOG_INFO, "HermesTest", message);
#endif
}
