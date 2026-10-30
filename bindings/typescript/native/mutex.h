#pragma once

#ifdef _WIN32
#include <windows.h>

typedef SRWLOCK mln_ts_mutex;
#define MLN_TS_MUTEX_INITIALIZER SRWLOCK_INIT

static inline void mln_ts_mutex_init(mln_ts_mutex* mutex) {
  InitializeSRWLock(mutex);
}
static inline void mln_ts_mutex_destroy(mln_ts_mutex* mutex) { (void)mutex; }
static inline void mln_ts_mutex_lock(mln_ts_mutex* mutex) {
  AcquireSRWLockExclusive(mutex);
}
static inline void mln_ts_mutex_unlock(mln_ts_mutex* mutex) {
  ReleaseSRWLockExclusive(mutex);
}
#else
#include <pthread.h>

typedef pthread_mutex_t mln_ts_mutex;
#define MLN_TS_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER

static inline void mln_ts_mutex_init(mln_ts_mutex* mutex) {
  pthread_mutex_init(mutex, NULL);
}
static inline void mln_ts_mutex_destroy(mln_ts_mutex* mutex) {
  pthread_mutex_destroy(mutex);
}
static inline void mln_ts_mutex_lock(mln_ts_mutex* mutex) {
  pthread_mutex_lock(mutex);
}
static inline void mln_ts_mutex_unlock(mln_ts_mutex* mutex) {
  pthread_mutex_unlock(mutex);
}
#endif
