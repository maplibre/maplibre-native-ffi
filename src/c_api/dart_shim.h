#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "maplibre_native_c.h"

#ifdef __cplusplus
#define MLN_DART_SHIM_NOEXCEPT noexcept
extern "C" {
#else
#define MLN_DART_SHIM_NOEXCEPT
#endif

// NOLINTBEGIN(modernize-use-using,modernize-use-trailing-return-type)

typedef struct mln_dart_resource_rewrite_rule {
  uint32_t kind;
  const char* url;
  const char* replacement_url;
} mln_dart_resource_rewrite_rule;

typedef struct mln_dart_resource_rewrite_rules {
  const mln_dart_resource_rewrite_rule* rules;
  size_t count;
} mln_dart_resource_rewrite_rules;

typedef struct mln_dart_resource_provider_rule {
  uint32_t kind;
  const char* url;
  mln_resource_response response;
} mln_dart_resource_provider_rule;

typedef struct mln_dart_resource_provider_rules {
  const mln_dart_resource_provider_rule* rules;
  size_t count;
} mln_dart_resource_provider_rules;

typedef struct mln_dart_queued_resource_provider_route {
  uint32_t kind;
  const char* url;
} mln_dart_queued_resource_provider_route;

typedef void (*mln_dart_queued_resource_request_listener)(void* request);

typedef struct mln_dart_queued_resource_provider {
  const mln_dart_queued_resource_provider_route* routes;
  size_t route_count;
  mln_dart_queued_resource_request_listener listener;
} mln_dart_queued_resource_provider;

typedef struct mln_dart_queued_resource_request {
  void* owner;
  mln_resource_request_handle* handle;
  const char* url;
  uint32_t kind;
  uint32_t loading_method;
  uint32_t priority;
  uint32_t usage;
  uint32_t storage_policy;
  bool has_range;
  uint64_t range_start;
  uint64_t range_end;
  bool has_prior_modified;
  int64_t prior_modified_unix_ms;
  bool has_prior_expires;
  int64_t prior_expires_unix_ms;
  const char* prior_etag;
  const uint8_t* prior_data;
  size_t prior_data_size;
} mln_dart_queued_resource_request;

typedef void (*mln_dart_log_record_listener)(void* record);

typedef struct mln_dart_log_callback_state {
  mln_dart_log_record_listener listener;
  uint32_t consume;
} mln_dart_log_callback_state;

typedef struct mln_dart_log_record {
  void* owner;
  bool retire_callback;
  uint32_t severity;
  uint32_t event;
  int64_t code;
  const char* message;
} mln_dart_log_record;

MLN_API void* mln_dart_handle_leak_token_create(
  const char* type_name, void* handle
) MLN_DART_SHIM_NOEXCEPT;
MLN_API void mln_dart_handle_leak_token_destroy(
  void* token
) MLN_DART_SHIM_NOEXCEPT;
MLN_API void mln_dart_handle_leak_report(void* token) MLN_DART_SHIM_NOEXCEPT;

MLN_API uint32_t mln_dart_log_callback(
  void* user_data, uint32_t severity, uint32_t event, int64_t code,
  const char* message
) MLN_DART_SHIM_NOEXCEPT;
MLN_API mln_status mln_dart_log_set_callback(
  mln_dart_log_callback_state* state
) MLN_DART_SHIM_NOEXCEPT;
MLN_API void mln_dart_log_record_destroy(void* record) MLN_DART_SHIM_NOEXCEPT;

MLN_API mln_status mln_dart_resource_transform_rewrite_callback(
  void* user_data, uint32_t kind, const char* url,
  mln_resource_transform_response* out_response
) MLN_DART_SHIM_NOEXCEPT;
MLN_API uint32_t mln_dart_resource_provider_rules_callback(
  void* user_data, const mln_resource_request* request,
  mln_resource_request_handle* handle
) MLN_DART_SHIM_NOEXCEPT;
MLN_API uint32_t mln_dart_queued_resource_provider_callback(
  void* user_data, const mln_resource_request* request,
  mln_resource_request_handle* handle
) MLN_DART_SHIM_NOEXCEPT;
MLN_API void mln_dart_resource_provider_request_destroy(
  void* request
) MLN_DART_SHIM_NOEXCEPT;
MLN_API void mln_dart_queued_resource_provider_retire(
  mln_dart_queued_resource_provider* provider
) MLN_DART_SHIM_NOEXCEPT;
MLN_API void mln_dart_custom_geometry_callbacks_retire(
  mln_custom_geometry_source_tile_callback fetch_tile,
  mln_custom_geometry_source_tile_callback cancel_tile, void* user_data
) MLN_DART_SHIM_NOEXCEPT;
MLN_API uint64_t mln_dart_resource_request_token_create(
  mln_resource_request_handle* handle
) MLN_DART_SHIM_NOEXCEPT;
MLN_API mln_status mln_dart_resource_request_token_cancelled(
  uint64_t token, bool* out_cancelled
) MLN_DART_SHIM_NOEXCEPT;
MLN_API mln_status mln_dart_resource_request_token_complete(
  uint64_t token, const mln_resource_response* response
) MLN_DART_SHIM_NOEXCEPT;
MLN_API mln_status
mln_dart_resource_request_token_release(uint64_t token) MLN_DART_SHIM_NOEXCEPT;
MLN_API mln_status
mln_dart_resource_request_token_wait(uint64_t token) MLN_DART_SHIM_NOEXCEPT;
MLN_API void mln_dart_test_invoke_custom_geometry_tile_callback(
  mln_custom_geometry_source_tile_callback callback, void* user_data,
  mln_canonical_tile_id tile_id
) MLN_DART_SHIM_NOEXCEPT;
MLN_API uint32_t mln_dart_test_emit_log(
  uint32_t severity, uint32_t event, int64_t code, const char* message
) MLN_DART_SHIM_NOEXCEPT;

// NOLINTEND(modernize-use-using,modernize-use-trailing-return-type)

#ifdef __cplusplus
}
#endif

#undef MLN_DART_SHIM_NOEXCEPT
