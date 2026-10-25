#pragma once

#include <cstddef>
#include <memory>

#include <mln/style/expression/expression.hpp>

#include "handles/handle_table.hpp"
#include "maplibre_native_c/base.h"
#include "maplibre_native_c/style_expression.h"

namespace mln::core {

// Immutable after creation, and evaluation only reads it, so any number of
// threads may evaluate one expression at once.
struct StyleExpressionObject {
  std::unique_ptr<mln::style::expression::Expression> expression;
};

template <>
struct HandleTraits<StyleExpressionObject> {
  static constexpr auto kind = HandleKind::StyleExpression;
  static constexpr auto leasable = true;
};

auto style_expression_create(
  mln_buffer_view expression, const mln_style_expression_options* options,
  mln_style_expression* out_expression
) -> mln_status;
auto style_expression_release(mln_style_expression expression) -> void;
auto style_expression_get_info(
  mln_style_expression expression, mln_style_expression_info* out_info
) -> mln_status;
auto style_expression_evaluate(
  mln_style_expression expression, const mln_style_expression_inputs* inputs,
  char* out_json, std::size_t json_capacity, std::size_t* out_json_length
) -> mln_status;

}  // namespace mln::core
