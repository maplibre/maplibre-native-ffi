#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>

#include <mln/style/conversion_impl.hpp>
#include <mln/style/expression/expression.hpp>
#include <mln/style/expression/is_constant.hpp>
#include <mln/style/expression/parsing_context.hpp>
#include <mln/style/expression/type.hpp>
#include <mln/style/expression/value.hpp>
#include <mln/style/rapidjson_conversion.hpp>
#include <mln/tile/tile_id.hpp>
#include <mln/util/feature.hpp>
#include <mln/util/geojson.hpp>
#include <mln/util/rapidjson.hpp>

#include "style/style_expression.hpp"

#include "diagnostics/diagnostics.hpp"
#include "geojson/geojson.hpp"
#include "style/style_value.hpp"

namespace {

namespace expression = mln::style::expression;
namespace type = mln::style::expression::type;

auto style_expression_table()
  -> mln::core::HandleTable<mln::core::StyleExpressionObject>& {
  return mln::core::handle_table<mln::core::StyleExpressionObject>();
}

auto to_native_type_kind(mln_style_expression_type_kind kind)
  -> std::optional<type::Type> {
  switch (kind) {
    case MLN_STYLE_EXPRESSION_TYPE_VALUE:
      return type::Value;
    case MLN_STYLE_EXPRESSION_TYPE_NUMBER:
      return type::Number;
    case MLN_STYLE_EXPRESSION_TYPE_STRING:
      return type::String;
    case MLN_STYLE_EXPRESSION_TYPE_BOOLEAN:
      return type::Boolean;
    case MLN_STYLE_EXPRESSION_TYPE_COLOR:
      return type::Color;
    case MLN_STYLE_EXPRESSION_TYPE_OBJECT:
      return type::Object;
    case MLN_STYLE_EXPRESSION_TYPE_FORMATTED:
      return type::Formatted;
    case MLN_STYLE_EXPRESSION_TYPE_IMAGE:
      return type::Image;
    case MLN_STYLE_EXPRESSION_TYPE_PADDING:
      return type::Padding;
    case MLN_STYLE_EXPRESSION_TYPE_VARIABLE_ANCHOR_OFFSET_COLLECTION:
      return type::VariableAnchorOffsetCollection;
    default:
      return std::nullopt;
  }
}

auto to_native_type(const mln_style_expression_type& expected)
  -> std::optional<type::Type> {
  if (expected.kind != MLN_STYLE_EXPRESSION_TYPE_ARRAY) {
    auto converted = to_native_type_kind(expected.kind);
    if (!converted) {
      mln::core::set_thread_error("expected_type.kind is unknown");
    }
    return converted;
  }
  if (expected.item_kind == MLN_STYLE_EXPRESSION_TYPE_ARRAY) {
    mln::core::set_thread_error("expected_type.item_kind must not be an array");
    return std::nullopt;
  }
  auto item = to_native_type_kind(expected.item_kind);
  if (!item) {
    mln::core::set_thread_error("expected_type.item_kind is unknown");
    return std::nullopt;
  }
  return type::Array(
    *item, expected.item_count == 0
             ? std::nullopt
             : std::optional<std::size_t>{expected.item_count}
  );
}

auto to_native_expected_type(
  const mln_style_expression_options* options,
  std::optional<type::Type>& out_expected
) -> mln_status {
  if (options == nullptr) {
    return MLN_STATUS_OK;
  }
  if (options->size < sizeof(mln_style_expression_options)) {
    mln::core::set_thread_error(
      "mln_style_expression_options.size is too small"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if ((options->fields & ~MLN_STYLE_EXPRESSION_OPTION_EXPECTED_TYPE) != 0U) {
    mln::core::set_thread_error(
      "mln_style_expression_options.fields contains unknown bits"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if ((options->fields & MLN_STYLE_EXPRESSION_OPTION_EXPECTED_TYPE) == 0U) {
    return MLN_STATUS_OK;
  }
  out_expected = to_native_type(options->expected_type);
  return out_expected ? MLN_STATUS_OK : MLN_STATUS_INVALID_ARGUMENT;
}

// The inputs that one evaluation reads, converted from their C form. The
// context points into this object, so it stays in place while the expression
// evaluates.
struct EvaluationInputs {
  std::optional<mln::GeoJSONFeature> feature;
  std::optional<mln::FeatureState> feature_state;
  std::optional<mln::GlobalStateMap> global_state;
  std::optional<mln::CanonicalTileID> canonical;
  std::set<std::string> available_images;
};

auto has_input(const mln_style_expression_inputs& inputs, uint32_t field)
  -> bool {
  return (inputs.fields & field) != 0U;
}

template <typename T>
auto finite_input(
  const mln_style_expression_inputs& inputs, uint32_t field, double value,
  const char* name, std::optional<T>& out_value
) -> bool {
  if (!has_input(inputs, field)) {
    return true;
  }
  if (!std::isfinite(value)) {
    auto message = std::string{name} + " must be finite";
    mln::core::set_thread_error(message.c_str());
    return false;
  }
  out_value = static_cast<T>(value);
  return true;
}

auto json_object_input(mln_buffer_view view, const char* name)
  -> std::optional<mln::PropertyMap> {
  auto document = mln::JSDocument{};
  if (!mln::core::parse_json_document(view, name, document)) {
    return std::nullopt;
  }
  if (!document.IsObject()) {
    auto message = std::string{name} + " must contain a JSON object";
    mln::core::set_thread_error(message.c_str());
    return std::nullopt;
  }
  auto value = mapbox::geojson::convert<mln::Value>(document);
  return std::move(value.get<mln::PropertyMap>());
}

auto to_native_inputs(
  const mln_style_expression_inputs& inputs, EvaluationInputs& out_inputs,
  expression::EvaluationContext& out_context
) -> mln_status {
  if (inputs.size < sizeof(mln_style_expression_inputs)) {
    mln::core::set_thread_error(
      "mln_style_expression_inputs.size is too small"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  constexpr auto known_fields =
    static_cast<uint32_t>(MLN_STYLE_EXPRESSION_INPUT_ZOOM) |
    MLN_STYLE_EXPRESSION_INPUT_COLOR_RAMP_PARAMETER |
    MLN_STYLE_EXPRESSION_INPUT_ELEVATION |
    MLN_STYLE_EXPRESSION_INPUT_CANONICAL_TILE_ID;
  if ((inputs.fields & ~known_fields) != 0U) {
    mln::core::set_thread_error(
      "mln_style_expression_inputs.fields contains unknown bits"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  if (
    !finite_input(
      inputs, MLN_STYLE_EXPRESSION_INPUT_ZOOM, inputs.zoom, "zoom",
      out_context.zoom
    ) ||
    !finite_input(
      inputs, MLN_STYLE_EXPRESSION_INPUT_COLOR_RAMP_PARAMETER,
      inputs.color_ramp_parameter, "color_ramp_parameter",
      out_context.colorRampParameter
    ) ||
    !finite_input(
      inputs, MLN_STYLE_EXPRESSION_INPUT_ELEVATION, inputs.elevation,
      "elevation", out_context.elevation
    )
  ) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  if (inputs.feature.size != 0) {
    out_inputs.feature = mln::core::to_native_feature(inputs.feature);
    if (!out_inputs.feature) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
  }
  if (inputs.feature_state.size != 0) {
    out_inputs.feature_state =
      json_object_input(inputs.feature_state, "feature_state");
    if (!out_inputs.feature_state) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    out_context.withFeatureState(&*out_inputs.feature_state);
  }
  if (inputs.global_state.size != 0) {
    out_inputs.global_state =
      json_object_input(inputs.global_state, "global_state");
    if (!out_inputs.global_state) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    out_context.withGlobalState(&*out_inputs.global_state);
  }
  if (has_input(inputs, MLN_STYLE_EXPRESSION_INPUT_CANONICAL_TILE_ID)) {
    const auto tile_status =
      mln::core::validate_canonical_tile_id(inputs.canonical_tile_id);
    if (tile_status != MLN_STATUS_OK) {
      return tile_status;
    }
    out_inputs.canonical =
      mln::core::to_native_canonical_tile_id(inputs.canonical_tile_id);
    out_context.withCanonicalTileID(&*out_inputs.canonical);
  }

  if (inputs.available_image_count != 0 && inputs.available_images == nullptr) {
    mln::core::set_thread_error(
      "available_images must not be null when available_image_count is nonzero"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  for (std::size_t index = 0; index < inputs.available_image_count; ++index) {
    const auto& image = inputs.available_images[index];
    if (!mln::core::validate_bytes(image, "available image ID")) {
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    out_inputs.available_images.emplace(
      static_cast<const char*>(image.data), image.size
    );
  }
  out_context.withAvailableImages(&out_inputs.available_images);
  return MLN_STATUS_OK;
}

// JSON has no number that is not finite, so such a number becomes null, as
// JSON.stringify() writes it.
auto replace_non_finite_numbers(mln::Value& value) -> void {
  value.match(
    [&](double number) -> void {
      if (!std::isfinite(number)) {
        value = mln::NullValue{};
      }
    },
    [](std::vector<mln::Value>& items) -> void {
      for (auto& item : items) {
        replace_non_finite_numbers(item);
      }
    },
    [](mln::PropertyMap& members) -> void {
      for (auto& [key, member] : members) {
        replace_non_finite_numbers(member);
      }
    },
    [](const auto&) -> void {}
  );
}

}  // namespace

namespace mln::core {

auto style_expression_create(
  mln_buffer_view expression, const mln_style_expression_options* options,
  mln_style_expression* out_expression
) -> mln_status {
  if (out_expression == nullptr) {
    set_thread_error("out_expression must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (*out_expression != MLN_HANDLE_NULL) {
    set_thread_error("*out_expression must be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto expected = std::optional<type::Type>{};
  const auto options_status = to_native_expected_type(options, expected);
  if (options_status != MLN_STATUS_OK) {
    return options_status;
  }
  auto document = mln::JSDocument{};
  if (!parse_json_document(expression, "expression", document)) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  // GL JS createExpression() coerces a string result rather than asserting
  // it, as a string layer property does.
  auto context = expected ? expression::ParsingContext{*expected}
                          : expression::ParsingContext{};
  auto parsed = context.parseExpression(
    mln::style::conversion::Convertible{
      static_cast<const mln::JSValue*>(&document)
    },
    expected && *expected == type::String
      ? std::make_optional(expression::TypeAnnotationOption::coerce)
      : std::nullopt
  );
  if (!parsed) {
    auto message = "expression: " + context.getCombinedErrors();
    set_thread_error(message.c_str());
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  // No value of another type holds a collator, so checking the result type
  // keeps every evaluation's result serializable.
  if ((*parsed)->getType().is<type::CollatorType>()) {
    set_thread_error("expression: a collator result has no JSON form");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto object = std::make_shared<StyleExpressionObject>(
    StyleExpressionObject{.expression = std::move(*parsed)}
  );
  *out_expression = style_expression_table().insert(std::move(object));
  return MLN_STATUS_OK;
}

auto style_expression_release(mln_style_expression expression) -> void {
  if (expression == MLN_HANDLE_NULL) {
    return;
  }
  style_expression_table().remove(expression);
}

auto style_expression_get_info(
  mln_style_expression expression, mln_style_expression_info* out_info
) -> mln_status {
  if (out_info == nullptr) {
    set_thread_error("out_info must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (out_info->size < sizeof(mln_style_expression_info)) {
    set_thread_error("mln_style_expression_info.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto object = style_expression_table().lease(expression);
  if (object == nullptr) {
    return recorded_handle_fault_status();
  }
  out_info->is_feature_constant =
    expression::isFeatureConstant(*object->expression);
  out_info->is_zoom_constant = expression::isZoomConstant(*object->expression);
  return MLN_STATUS_OK;
}

auto style_expression_evaluate(
  mln_style_expression expression, const mln_style_expression_inputs* inputs,
  char* out_json, std::size_t json_capacity, std::size_t* out_json_length
) -> mln_status {
  if (out_json == nullptr && json_capacity != 0) {
    set_thread_error("out_json must not be null when json_capacity is nonzero");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (out_json_length == nullptr) {
    set_thread_error("out_json_length must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto object = style_expression_table().lease(expression);
  if (object == nullptr) {
    return recorded_handle_fault_status();
  }

  auto native_inputs = EvaluationInputs{};
  auto context = expression::EvaluationContext{};
  if (inputs != nullptr) {
    const auto inputs_status =
      to_native_inputs(*inputs, native_inputs, context);
    if (inputs_status != MLN_STATUS_OK) {
      return inputs_status;
    }
  }

  const auto result =
    native_inputs.feature
      ? object->expression->evaluate(*native_inputs.feature, context)
      : object->expression->evaluate(context);
  if (!result) {
    auto message = "expression: " + result.error().message;
    set_thread_error(message.c_str());
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto value =
    expression::ValueConverter<mln::Value>::fromExpressionValue(*result);
  replace_non_finite_numbers(value);
  const auto json = serialize_json_value(value);

  *out_json_length = json.size();
  if (json_capacity > json.size()) {
    std::memcpy(out_json, json.data(), json.size());
    out_json[json.size()] = '\0';
  }
  return MLN_STATUS_OK;
}

}  // namespace mln::core
