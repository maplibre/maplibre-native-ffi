// The native probe that test/support/native_probe.c defines.

#ifndef MLN_TS_TEST_NATIVE_PROBE_H
#define MLN_TS_TEST_NATIVE_PROBE_H

#include <node_api.h>

/**
 * nativeProbe(listener) -> { wake, lock, unlock, release, userData }, as
 * bigint addresses.
 */
napi_value mln_ts_test_native_probe(napi_env env, napi_callback_info info);

#endif
