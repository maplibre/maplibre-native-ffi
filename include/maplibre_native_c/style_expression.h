/**
 * @file maplibre_native_c/style_expression.h
 * Parse and evaluate style expressions without a map.
 */

#ifndef MAPLIBRE_NATIVE_C_STYLE_EXPRESSION_H
#define MAPLIBRE_NATIVE_C_STYLE_EXPRESSION_H

#ifndef __cplusplus
#include <stdbool.h>
#endif

#include <stddef.h>
#include <stdint.h>

#include "base.h"
#include "style.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The type of a style expression's result, or of the items of an array result.
 */
typedef enum mln_style_expression_type_kind : uint32_t {
  /** Any type. */
  MLN_STYLE_EXPRESSION_TYPE_VALUE = 0,
  MLN_STYLE_EXPRESSION_TYPE_NUMBER = 1,
  MLN_STYLE_EXPRESSION_TYPE_STRING = 2,
  MLN_STYLE_EXPRESSION_TYPE_BOOLEAN = 3,
  MLN_STYLE_EXPRESSION_TYPE_COLOR = 4,
  MLN_STYLE_EXPRESSION_TYPE_OBJECT = 5,
  MLN_STYLE_EXPRESSION_TYPE_FORMATTED = 6,
  MLN_STYLE_EXPRESSION_TYPE_IMAGE = 7,
  MLN_STYLE_EXPRESSION_TYPE_PADDING = 8,
  MLN_STYLE_EXPRESSION_TYPE_VARIABLE_ANCHOR_OFFSET_COLLECTION = 9,
  MLN_STYLE_EXPRESSION_TYPE_ARRAY = 10,
} mln_style_expression_type_kind;

/** The type of a style expression's result. */
typedef struct mln_style_expression_type {
  mln_style_expression_type_kind kind;
  /**
   * The type of each item when `kind` is MLN_STYLE_EXPRESSION_TYPE_ARRAY. It
   * can't be an array itself.
   */
  mln_style_expression_type_kind item_kind;
  /** The length of an array, or 0 for any length. */
  uint32_t item_count;
} mln_style_expression_type;

/** The fields that mln_style_expression_options sets. */
typedef enum MLN_BINDING(
  "kind=bitmask"
) mln_style_expression_option_field : uint32_t {
  MLN_STYLE_EXPRESSION_OPTION_EXPECTED_TYPE = 1U << 0U,
} mln_style_expression_option_field;

/** A set of mln_style_expression_option_field bits. */
typedef uint32_t mln_style_expression_option_fields;

/** Options for parsing a style expression. */
typedef struct mln_style_expression_options {
  uint32_t size;
  mln_style_expression_option_fields fields;
  /** The type that the result must have. */
  mln_style_expression_type expected_type
    MLN_BINDING("mask=fields;bit=MLN_STYLE_EXPRESSION_OPTION_EXPECTED_TYPE");
} mln_style_expression_options;

/** The fields that mln_style_expression_inputs sets. */
typedef enum MLN_BINDING(
  "kind=bitmask"
) mln_style_expression_input_field : uint32_t {
  MLN_STYLE_EXPRESSION_INPUT_ZOOM = 1U << 0U,
  MLN_STYLE_EXPRESSION_INPUT_COLOR_RAMP_PARAMETER = 1U << 1U,
  MLN_STYLE_EXPRESSION_INPUT_ELEVATION = 1U << 2U,
  MLN_STYLE_EXPRESSION_INPUT_CANONICAL_TILE_ID = 1U << 3U,
} mln_style_expression_input_field;

/** A set of mln_style_expression_input_field bits. */
typedef uint32_t mln_style_expression_input_fields;

/** The inputs that a style expression reads while it evaluates. */
typedef struct mln_style_expression_inputs {
  uint32_t size;
  mln_style_expression_input_fields fields;
  double zoom MLN_BINDING("mask=fields;bit=MLN_STYLE_EXPRESSION_INPUT_ZOOM");
  /** The value that `heatmap-density` and `line-progress` read. */
  double color_ramp_parameter MLN_BINDING(
    "mask=fields;bit=MLN_STYLE_EXPRESSION_INPUT_COLOR_RAMP_PARAMETER"
  );
  /** The value that `elevation` reads when `color_ramp_parameter` isn't set. */
  double elevation
    MLN_BINDING("mask=fields;bit=MLN_STYLE_EXPRESSION_INPUT_ELEVATION");
  /**
   * The feature to evaluate against, as a GeoJSON feature, or empty for none.
   */
  mln_buffer_view feature MLN_BINDING("encoding=json");
  /** The feature's state, as a JSON object, or empty for none. */
  mln_buffer_view feature_state MLN_BINDING("encoding=json");
  /**
   * The values that `global-state` reads, as a JSON object, or empty for none.
   */
  mln_buffer_view global_state MLN_BINDING("encoding=json");
  /**
   * The tile that holds the feature. `within` and `distance` need it to read
   * the feature's geometry.
   */
  mln_canonical_tile_id canonical_tile_id
    MLN_BINDING("mask=fields;bit=MLN_STYLE_EXPRESSION_INPUT_CANONICAL_TILE_ID");
  /** The IDs of the images that `image` treats as available. */
  const mln_buffer_view* available_images
    MLN_BINDING("length=available_image_count");
  size_t available_image_count;
} mln_style_expression_inputs;

/** What a style expression's result depends on. */
typedef struct mln_style_expression_info {
  uint32_t size;
  /** True when the result doesn't depend on the feature or its state. */
  bool is_feature_constant;
  /** True when the result doesn't depend on the zoom level. */
  bool is_zoom_constant;
} mln_style_expression_info;

/**
 * Parses a style
 * [expression](https://maplibre.org/maplibre-style-spec/expressions/) to
 * evaluate without a map.
 *
 * Returns:
 * - MLN_STATUS_INVALID_ARGUMENT when `expression` isn't a valid expression, or
 *   its result can't have the expected type.
 */
MLN_API mln_status mln_style_expression_create(
  mln_buffer_view expression MLN_BINDING("encoding=json"),
  const mln_style_expression_options* options MLN_BINDING("nullable=true"),
  mln_style_expression* out_expression MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/** Releases a parsed style expression. */
MLN_API void mln_style_expression_release(
  mln_style_expression expression
) MLN_NOEXCEPT;

/** Reads what a style expression's result depends on. */
MLN_API mln_status mln_style_expression_get_info(
  mln_style_expression expression,
  mln_style_expression_info* out_info MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Evaluates a style expression and writes its result as JSON.
 *
 * A number in the result that isn't finite becomes null.
 *
 * Returns:
 * - MLN_STATUS_INVALID_ARGUMENT when the expression fails to evaluate, such as
 *   when it reads an input that `inputs` doesn't set.
 *
 * Lifetimes:
 * The function writes the result and a terminating NUL to `out_json` only when
 * `json_capacity` is greater than the result's length. It always sets
 * `out_json_length` to the length without the NUL.
 */
MLN_API mln_status mln_style_expression_evaluate(
  mln_style_expression expression,
  const mln_style_expression_inputs* inputs MLN_BINDING("nullable=true"),
  char* out_json MLN_BINDING(
    "direction=out;encoding=json;length=json_capacity;"
    "result_length=out_json_length"
  ),
  size_t json_capacity, size_t* out_json_length MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_STYLE_EXPRESSION_H
