// Expanding one supercluster cluster into the source features it contains.

#include <maplibre_native_c.h>
#include <string.h>

static mln_string_view sv(const char* text) {
  return (mln_string_view){.data = text, .size = strlen(text)};
}

static mln_json_value json_uint(uint64_t value) {
  return (mln_json_value){
    .size = sizeof(mln_json_value),
    .type = MLN_JSON_VALUE_TYPE_UINT,
    .data = {.uint_value = value},
  };
}

static mln_json_value json_object(
  const mln_json_member* members, size_t member_count
) {
  const mln_json_object object = {
    .members = members, .member_count = member_count
  };
  return (mln_json_value){
    .size = sizeof(mln_json_value),
    .type = MLN_JSON_VALUE_TYPE_OBJECT,
    .data = {.object_value = object},
  };
}

static void read_leaves(mln_feature_extension_result result) {
  mln_feature_extension_result_info info = {.size = sizeof(info)};
  if (mln_feature_extension_result_get(result, &info) != MLN_STATUS_OK) {
    return;
  }
  if (info.type != MLN_FEATURE_EXTENSION_RESULT_TYPE_FEATURE_COLLECTION) {
    return;
  }
  const mln_feature_collection leaves = info.data.feature_collection;
  for (size_t index = 0; index < leaves.feature_count; index++) {
    // leaves.features[index] is one point that the cluster contains.
  }
}

void list_cluster_leaves(
  mln_render_session session, const mln_feature* cluster
) {
  const mln_json_value limit = json_uint(10);
  const mln_json_value offset = json_uint(0);
  const mln_json_member members[] = {
    {.key = sv("limit"), .value = &limit},
    {.key = sv("offset"), .value = &offset},
  };
  const mln_json_value arguments = json_object(members, 2);

  mln_feature_extension_result result = MLN_HANDLE_NULL;
  const mln_status queried = mln_render_session_query_feature_extensions(
    session, sv("places"), cluster, sv("supercluster"), sv("leaves"),
    &arguments, &result
  );
  if (queried == MLN_STATUS_OK) read_leaves(result);

  mln_feature_extension_result_destroy(result);
}
