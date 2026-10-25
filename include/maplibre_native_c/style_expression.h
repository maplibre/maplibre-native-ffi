/**
 * @file maplibre_native_c/style_expression.h
 * Public C API declarations for evaluating style expressions without a map.
 *
 * An expression parses once into an immutable handle that evaluates against
 * caller-supplied inputs, the way a layer evaluates it while it renders. It
 * needs no runtime, map, or render session, and every declaration in this
 * header is synchronous and callable from any thread.
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

/** Style expression types that a parsed expression may be required to have. */
typedef enum mln_style_expression_type_kind : uint32_t {
  /** Any value. */
  MLN_STYLE_EXPRESSION_TYPE_VALUE = 0,
  MLN_STYLE_EXPRESSION_TYPE_NUMBER = 1,
  MLN_STYLE_EXPRESSION_TYPE_STRING = 2,
  MLN_STYLE_EXPRESSION_TYPE_BOOLEAN = 3,
  MLN_STYLE_EXPRESSION_TYPE_COLOR = 4,
  MLN_STYLE_EXPRESSION_TYPE_OBJECT = 5,
  MLN_STYLE_EXPRESSION_TYPE_FORMATTED = 6,
  /** A resolved image, as the image expression returns. */
  MLN_STYLE_EXPRESSION_TYPE_IMAGE = 7,
  MLN_STYLE_EXPRESSION_TYPE_PADDING = 8,
  MLN_STYLE_EXPRESSION_TYPE_VARIABLE_ANCHOR_OFFSET_COLLECTION = 9,
  /** An array whose items have item_kind, which is not itself an array. */
  MLN_STYLE_EXPRESSION_TYPE_ARRAY = 10,
} mln_style_expression_type_kind;

/** A style expression type. */
typedef struct mln_style_expression_type {
  mln_style_expression_type_kind kind;
  /** For an array, its items' kind; ignored for any other kind. */
  mln_style_expression_type_kind item_kind;
  /** For an array, its fixed length, or zero for any length. */
  uint32_t item_count;
} mln_style_expression_type;

/** Fields present in mln_style_expression_options. */
typedef enum MLN_BINDING(
  "kind=bitmask"
) mln_style_expression_option_field : uint32_t {
  /** expected_type is set. */
  MLN_STYLE_EXPRESSION_OPTION_EXPECTED_TYPE = 1U << 0U,
} mln_style_expression_option_field;

/** A set of mln_style_expression_option_field bits. */
typedef uint32_t mln_style_expression_option_fields;

/** Options for parsing a style expression. */
typedef struct mln_style_expression_options {
  uint32_t size;
  /** Bitwise combination of mln_style_expression_option_field values. */
  mln_style_expression_option_fields fields;
  /**
   * The type that the expression must produce. A string type coerces the
   * result to a string, as a string layer property does; every other type
   * asserts it. Without it, the expression keeps the type it infers.
   */
  mln_style_expression_type expected_type
    MLN_BINDING("mask=fields;bit=MLN_STYLE_EXPRESSION_OPTION_EXPECTED_TYPE");
} mln_style_expression_options;

/** Fields present in mln_style_expression_inputs. */
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

/**
 * What one evaluation reads.
 *
 * An input whose field bit is clear, or a JSON input that is empty, is absent.
 * An expression that needs the zoom, feature data, or the color ramp parameter
 * fails to evaluate without it, and an absent state reads as null.
 */
typedef struct mln_style_expression_inputs {
  uint32_t size;
  /** Bitwise combination of mln_style_expression_input_field values. */
  mln_style_expression_input_fields fields;
  /** The value of the zoom expression. */
  double zoom MLN_BINDING("mask=fields;bit=MLN_STYLE_EXPRESSION_INPUT_ZOOM");
  /**
   * The value of the heatmap-density and line-progress expressions, and of
   * the elevation expression in a color-relief layer.
   */
  double color_ramp_parameter MLN_BINDING(
    "mask=fields;bit=MLN_STYLE_EXPRESSION_INPUT_COLOR_RAMP_PARAMETER"
  );
  /** The value of the elevation expression outside a color-relief layer. */
  double elevation
    MLN_BINDING("mask=fields;bit=MLN_STYLE_EXPRESSION_INPUT_ELEVATION");
  /** One UTF-8 GeoJSON Feature that feature data expressions read. */
  mln_buffer_view feature MLN_BINDING("encoding=json");
  /** A UTF-8 JSON object that the feature-state expression reads. */
  mln_buffer_view feature_state MLN_BINDING("encoding=json");
  /** A UTF-8 JSON object that the global-state expression reads. */
  mln_buffer_view global_state MLN_BINDING("encoding=json");
  /**
   * The tile that holds the feature, which the within and distance
   * expressions need to place its geometry.
   */
  mln_canonical_tile_id canonical_tile_id
    MLN_BINDING("mask=fields;bit=MLN_STYLE_EXPRESSION_INPUT_CANONICAL_TILE_ID");
  /**
   * The image IDs that the image expression reports as available. The
   * expression reports any other ID as unavailable.
   */
  const mln_buffer_view* available_images
    MLN_BINDING("length=available_image_count");
  size_t available_image_count;
} mln_style_expression_inputs;

/** What a parsed expression depends on. */
typedef struct mln_style_expression_info {
  uint32_t size;
  /** Whether the result is the same for every feature and feature state. */
  bool is_feature_constant;
  /** Whether the result is the same at every zoom. */
  bool is_zoom_constant;
} mln_style_expression_info;

/**
 * Parses a style expression.
 *
 * expression is UTF-8 MapLibre style-spec expression JSON, such as
 * ["get", "name"] or a literal value. Parsing follows MapLibre GL JS
 * createExpression() and checks the result against options->expected_type when
 * it is set. An expression whose result is a collator has no JSON form, so it
 * is rejected.
 *
 * *out_expression must be MLN_HANDLE_NULL on entry. On success it receives an
 * owned handle the host releases with mln_style_expression_release().
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when expression is empty or not valid JSON,
 *   options is undersized, carries unknown fields, or names an unknown type
 *   kind or an array of arrays, out_expression is null or
 *   *out_expression is not MLN_HANDLE_NULL, or the expression does not parse.
 *   The diagnostic lists each parse error with its location in the
 *   expression.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 */
MLN_API mln_status mln_style_expression_create(
  mln_buffer_view expression MLN_BINDING("encoding=json"),
  const mln_style_expression_options* options MLN_BINDING("nullable=true"),
  mln_style_expression* out_expression MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Releases a style expression.
 *
 * An evaluation already running on another thread keeps the expression until
 * it returns. A null or already-released handle is a no-op.
 */
MLN_API void mln_style_expression_release(
  mln_style_expression expression
) MLN_NOEXCEPT;

/**
 * Copies what a parsed expression depends on into out_info.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when expression is an invalid handle, or
 *   out_info is null or undersized.
 * - MLN_STATUS_INVALID_STATE when expression has been released.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 */
MLN_API mln_status mln_style_expression_get_info(
  mln_style_expression expression,
  mln_style_expression_info* out_info MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Evaluates a parsed expression and writes its result as UTF-8 JSON.
 *
 * A null inputs sets no input. A color serializes as an
 * ["rgba", r, g, b, a] array, a formatted value as the format expression that
 * makes it, and an image as an object with its name and whether it is
 * available. A number that is not finite serializes as null, as
 * JSON.stringify() writes it.
 *
 * *out_json_length receives the result's length in bytes, without a null
 * terminator. When json_capacity exceeds that length, out_json receives the
 * result and a null terminator; otherwise out_json is unchanged, and a caller
 * evaluates again with at least *out_json_length + 1 bytes. out_json may be
 * null when json_capacity is zero. Evaluation reads only its inputs, so the
 * same inputs produce the same result.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when expression is an invalid handle; inputs
 *   is undersized, carries unknown fields, or sets a number that is not
 *   finite, a feature that is not one GeoJSON Feature, a state that is not a
 *   JSON object, a tile ID outside its zoom, or available_images that is null
 *   with a nonzero count; out_json is null with a nonzero json_capacity;
 *   out_json_length is null; or the expression fails to evaluate against the
 *   inputs, which the diagnostic describes.
 * - MLN_STATUS_INVALID_STATE when expression has been released.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
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
