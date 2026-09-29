"""Emit static native capture functions for deferred foreign callbacks.

Completion copies preserve an asynchronous result past its callback. Deferred
callback copies answer a synchronous callback early with its declared deferred
result and preserve the call's arguments for a host that runs it later.
"""

from __future__ import annotations

import zlib

from .compiler import compile_api
from .model import Api, CType, ModelError
from .semantic import BoundApi, FieldPlan, ValuePlan


def copy_kind(native: str) -> str:
    return "MLN_ADAPTER_COMPLETION_COPY_" + native.removeprefix("mln_").upper()


def capture_id(native: str, ownership: str = "borrowed") -> int:
    return zlib.crc32(f"{ownership}:{native}".encode())


def deferred_constant(native: str) -> str:
    return "MLN_ADAPTER_DEFERRED_" + native.removeprefix("mln_").upper()


def deferred_id(native: str) -> int:
    return zlib.crc32(f"deferred:{native}".encode())


def arguments_record(native: str) -> str:
    """The generated record that holds one deferred call's copied arguments."""
    return "mln_adapter_" + native.removeprefix("mln_") + "_arguments"


def deferred_callbacks(bound: BoundApi):
    """Deferred callback plans with their typedefs, sorted by name."""
    return [
        (callback, bound.source.typedefs_by_name[name])
        for name, callback in sorted(bound.callbacks.items())
        if callback.deferred
    ]


def arguments_plan(callback) -> ValuePlan:
    """A record plan whose fields are the callback's non-context parameters."""
    name = arguments_record(callback.native)
    return ValuePlan(
        kind="record",
        native=name,
        ctype=CType("record", name, "struct " + name, name),
        fields=tuple(
            FieldPlan(parameter.name, parameter.value)
            for parameter in callback.parameters
            if parameter.name != callback.context
        ),
    )


def _field_metadata(parameter) -> str:
    """Parameter metadata restated for a field whose copy the record owns."""
    metadata = {
        key: value
        for key, value in parameter.metadata.items()
        if key not in {"direction", "consumes", "kind", "lifetime", "ownership"}
    }
    if parameter.type.kind == "pointer":
        metadata.update(ownership="borrowed", lifetime="owner")
    if not metadata:
        return ""
    items = ";".join(f"{key}={value}" for key, value in metadata.items())
    return f' MLN_BINDING("{items}")'


def _declaration(type_: CType, name: str) -> str:
    spelling = type_.spelling
    if type_.kind not in {"pointer", "typedef", "elaborated"} and (
        "(" in spelling or "[" in spelling
    ):
        raise ModelError([f"{name}: deferred argument needs a plain declarator"])
    if type_.kind == "pointer" and spelling.endswith(" *"):
        return f"{spelling.removesuffix(' *')}* {name}"
    return f"{spelling} {name}"


def _deferred(bound: BoundApi) -> tuple[list[str], list[str]]:
    """Declare argument records and generate each deferred callback's adapter."""
    entries = deferred_callbacks(bound)
    if not entries:
        return [], []
    source = bound.source
    identities = {0: "released"}
    for callback, _ in entries:
        identity = deferred_id(callback.native)
        if identity in identities:
            raise ModelError(
                [
                    f"deferred identity collision: {callback.native}, {identities[identity]}"
                ]
            )
        identities[identity] = callback.native
    header = [
        "typedef enum mln_adapter_deferred_callback : uint32_t {",
        *(
            f"  {deferred_constant(callback.native)} = {deferred_id(callback.native)}U,"
            for callback, _ in entries
        ),
        "} mln_adapter_deferred_callback;",
    ]
    output = []
    for callback, typedef in entries:
        plan = arguments_plan(callback)
        _validate_capture_ownership(plan)
        record = plan.native
        header.extend(
            [
                f"/** Copied arguments of one deferred {callback.native} call. */",
                f"typedef struct {record} {{",
                *(
                    f"  {_declaration(parameter.type, parameter.name)}{_field_metadata(parameter)};"
                    for parameter in typedef.parameters
                    if parameter.name != callback.context
                ),
                f"}} {record};",
            ]
        )
        result = typedef.type.pointee.result.spelling
        arguments = ", ".join(
            _declaration(parameter.type, parameter.name)
            for parameter in typedef.parameters
        )
        values = ", ".join(field.name for field in plan.fields)
        output.extend(
            [
                "template <bool Writing>",
                f"auto capture_{record}([[maybe_unused]] Arena<Writing>& arena, {record} source) -> {record} {{",
                "  auto result = source;",
                *_capture(plan, "source", "result", "", 1),
                "  return result;",
                "}",
                f"inline auto deferred_{callback.native}({arguments}) noexcept -> {result} {{",
                f"  const auto arguments = {record}{{{values}}};",
                f"  const auto deferred = defer({callback.context}, {deferred_constant(callback.native)}, arguments,",
                f"    [](auto& arena, const {record}& copy) {{ return capture_{record}(arena, copy); }});",
                f"  return static_cast<{result}>(deferred ? {callback.deferred} : {callback.failure});",
                "}",
            ]
        )
    output.extend(
        [
            "inline auto deferred_function(std::uint32_t kind) noexcept -> void* {",
            "  switch (kind) {",
            *(
                f"    case {deferred_constant(callback.native)}: return reinterpret_cast<void*>(&deferred_{callback.native});"
                for callback, _ in entries
            ),
            "    default: return nullptr;",
            "  }",
            "}",
            "// Releases the decision handle of a record the host destroyed without",
            "// adopting it. Releasing a claimed, unanswered request fails it.",
            "inline auto deferred_discard([[maybe_unused]] std::uint32_t kind, [[maybe_unused]] const void* copied) noexcept -> void {",
            "  switch (kind) {",
        ]
    )
    for callback, _ in entries:
        decision = callback.decision
        if decision is None:
            continue
        release = source.functions_by_name[decision.handle.release]
        release_call = f"{decision.handle.release}(arguments.{decision.parameter})"
        output.extend(
            [
                f"    case {deferred_constant(callback.native)}: {{",
                f"      const auto& arguments = *static_cast<const {arguments_record(callback.native)}*>(copied);",
                f"      static_cast<void>({release_call});"
                if release.return_type.kind != "void"
                else f"      {release_call};",
                "      break;",
                "    }",
            ]
        )
    output.extend(["    default: break;", "  }", "}", ""])
    return header, output


def generate(api: Api | BoundApi) -> dict[str, str]:
    bound = compile_api(api)
    source = bound.source
    roots = {}
    arrays = set()
    owned = {}
    for function in source.functions:
        native = function.metadata.get("result")
        if not native or native == "void":
            continue
        operation = bound.operations_by_name.get(function.name)
        if operation is None:
            raise ModelError(list(bound.unsupported[function.name]))
        plan = operation.result
        if plan is None:
            raise ModelError(
                [f"{function.name}: completion capture requires result plan"]
            )
        if function.metadata.get("shape") == "array":
            arrays.add(native)
            if plan.element is None:
                raise ModelError(
                    [f"{function.name}: capture array requires an element plan"]
                )
            plan = plan.element
        if function.metadata.get("ownership") == "owned":
            if not plan.handle or not plan.handle.dispose:
                raise ModelError(
                    [f"{function.name}: owned capture requires a disposal operation"]
                )
            owned[native] = plan
        elif native in source.records_by_name:
            roots[native] = plan
    roots.update(owned)
    for plan in roots.values():
        _validate_capture_ownership(plan)
    identities = {0: "flat"}
    for native in roots:
        identity = capture_id(native, "owned" if native in owned else "borrowed")
        if identity in identities:
            raise ModelError(
                [f"capture identity collision: {native}, {identities[identity]}"]
            )
        identities[identity] = native
    header = [
        "// Generated by tools/bindgen. Do not edit.",
        "#ifndef MAPLIBRE_NATIVE_C_CALLBACK_CAPTURE_GENERATED_H",
        "#define MAPLIBRE_NATIVE_C_CALLBACK_CAPTURE_GENERATED_H",
        "#include <stdint.h>",
        *(
            f'#include "{path}"'
            for path in sorted(
                {"maplibre_native_c/base.h"}
                | {typedef.location.path for _, typedef in deferred_callbacks(bound)}
            )
        ),
        "typedef enum mln_adapter_completion_copy_kind : uint32_t {",
        "  MLN_ADAPTER_COMPLETION_COPY_FLAT = 0,",
    ]
    header.extend(
        f"  {copy_kind(native)} = {capture_id(native, 'owned' if native in owned else 'borrowed')}U,"
        for native in sorted(roots)
    )
    from . import native_ports

    port_header, port_output = native_ports.generate(bound)
    deferred_header, deferred_output = _deferred(bound)
    header.extend(
        [
            "} mln_adapter_completion_copy_kind;",
            *port_header,
            *deferred_header,
            "#endif",
            "",
        ]
    )

    output = ["// Generated by tools/bindgen. Do not edit."]
    for native, plan in sorted(roots.items()):
        body = _capture(plan, "source", "result", "", 1)
        output.extend(
            [
                "template <bool Writing>",
                f"auto capture_{native}([[maybe_unused]] Arena<Writing>& arena, {native} source) -> {native} {{",
                "  auto result = source;",
                *body,
                "  return result;",
                "}",
            ]
        )
    output.extend(
        [
            "inline auto discard(std::uint32_t kind, const mln_completion_result& result) noexcept -> void {",
            "  if (result.status != MLN_STATUS_OK || result.value == nullptr) return;",
            "  switch (kind) {",
        ]
    )
    for native, plan in sorted(owned.items()):
        assert plan.handle is not None and plan.handle.dispose is not None
        output.extend(
            [
                f"    case {copy_kind(native)}:",
                "      for (std::size_t index = 0; index < result.value_count; ++index)",
                f"        static_cast<void>({plan.handle.dispose}(static_cast<const {native}*>(result.value)[index]));",
                "      break;",
            ]
        )
    output.extend(["    default: break;", "  }", "}"])
    output.extend(
        [
            "inline auto valid_kind(std::uint32_t kind) -> bool {",
            "  switch (kind) {",
            "    case MLN_ADAPTER_COMPLETION_COPY_FLAT:",
            *(f"    case {copy_kind(native)}:" for native in sorted(roots)),
            "      return true;",
            "    default: return false;",
            "  }",
            "}",
            "template <bool Writing>",
            "auto value(Arena<Writing>& arena, const mln_completion_result& result,",
            "           std::uint32_t kind, std::size_t element_size) -> const void* {",
            "  if (result.value_count == 0) return result.value == nullptr ? nullptr : empty_pointer<std::max_align_t>();",
            '  if (result.value == nullptr) throw std::invalid_argument{"null completion result"};',
            "  switch (kind) {",
            "    case MLN_ADAPTER_COMPLETION_COPY_FLAT: {",
            "      if (element_size == 0 || result.value_count > std::numeric_limits<std::size_t>::max() / element_size)",
            '        throw std::invalid_argument{"invalid completion element size"};',
            "      const auto size = result.value_count * element_size;",
            "      constexpr auto alignment = sizeof(std::max_align_t);",
            "      const auto count = size / alignment + (size % alignment != 0);",
            "      auto* target = arena.template allocate<std::max_align_t>(count);",
            "      if constexpr (Writing) std::memcpy(target, result.value, size);",
            "      return target;",
            "    }",
        ]
    )
    for native in sorted(roots):
        output.extend(
            [
                f"    case {copy_kind(native)}:",
                *(
                    []
                    if native in arrays
                    else [
                        '      if (result.value_count != 1) throw std::invalid_argument{"invalid completion count"};'
                    ]
                ),
                f"      return span(arena, static_cast<const {native}*>(result.value), result.value_count,",
                f"        [](auto& storage, auto source) {{ return capture_{native}(storage, source); }});",
            ]
        )
    output.extend(
        [
            '    default: throw std::invalid_argument{"unknown completion capture kind"};',
            "  }",
            "}",
            "",
        ]
    )
    output.append(
        "inline auto dispose_owner(const char* native_type, std::uint64_t handle) noexcept -> mln_status {"
    )
    output.append("  if (native_type == nullptr) return MLN_STATUS_INVALID_ARGUMENT;")
    for handle in sorted(bound.handles.values(), key=lambda item: item.native):
        if not handle.dispose:
            continue
        operation = source.functions_by_name[handle.dispose]
        call = f"{handle.dispose}(static_cast<{handle.native}>(handle))"
        body = (
            f"{call}; return MLN_STATUS_OK;"
            if operation.return_type.kind == "void"
            else f"return {call};"
        )
        output.append(
            f'  if (std::strcmp(native_type, "{handle.native}") == 0) {{ {body} }}'
        )
    output.extend(["  return MLN_STATUS_INVALID_ARGUMENT;", "}", ""])
    output.extend(deferred_output)
    return {
        "include/maplibre_native_c/callback_capture_generated.h": "\n".join(header),
        "src/c_api/callback_capture_generated.inc": "\n".join(output),
        "src/c_api/callback_port_generated.inc": port_output,
    }


def _validate_capture_ownership(plan, *, nested=False):
    if plan.kind == "handle" and plan.ownership == "owned" and nested:
        raise ModelError(
            [f"{plan.native}: nested owned capture requires recursive disposal"]
        )
    for field in plan.fields:
        _validate_capture_ownership(field.value, nested=True)
    if plan.element is not None:
        _validate_capture_ownership(plan.element, nested=True)


def _capture(plan, source: str, target: str, parent: str, depth: int) -> list[str]:
    indent = "  " * depth
    if plan.kind in {"scalar", "enum", "handle"}:
        if plan.kind == "handle" and plan.ownership == "owned" and parent:
            raise ModelError(
                [f"{plan.native}: nested owned capture requires recursive disposal"]
            )
        return []
    if plan.kind == "buffer":
        if plan.buffer_form == "view":
            return [f"{indent}{target} = buffer(arena, {source});"]
        if plan.length == "nul":
            return [f"{indent}{target} = string(arena, {source});"]
        if plan.length:
            pointer = (
                f"static_cast<const std::byte*>({source})"
                if plan.ctype.pointee and plan.ctype.pointee.kind == "void"
                else source
            )
            return [
                f"{indent}{target} = bytes(arena, {pointer}, {parent}.{plan.length});"
            ]
        raise ModelError([f"{plan.native}: capture buffer requires length"])
    if plan.kind == "record":
        lines = []
        for field in plan.fields:
            child = field.value
            selector = field.presence
            child_source, child_target = (
                f"{source}.{field.name}",
                f"{target}.{field.name}",
            )
            if child.kind == "union":
                tag = selector.tag if selector else child.tag
                if not tag:
                    raise ModelError(
                        [f"{plan.native}.{field.name}: capture union requires tag"]
                    )
                lines.append(f"{indent}switch ({source}.{tag}) {{")
                for variant in child.fields:
                    if not variant.presence or not variant.presence.variant:
                        raise ModelError(
                            [
                                f"{plan.native}.{field.name}: capture union requires variants"
                            ]
                        )
                    lines.append(f"{indent}  case {variant.presence.variant}:")
                    lines.extend(
                        _capture(
                            variant.value,
                            f"{child_source}.{variant.name}",
                            f"{child_target}.{variant.name}",
                            child_source,
                            depth + 2,
                        )
                    )
                    lines.append(f"{indent}    break;")
                lines.extend(
                    [
                        f'{indent}  default: throw std::invalid_argument{{"unknown completion variant"}};',
                        f"{indent}}}",
                    ]
                )
            elif selector and selector.mask:
                condition = (
                    f"({source}.{selector.mask} & {selector.bit}) != 0"
                    if selector.bit
                    else f"{source}.{selector.mask}"
                )
                lines.append(f"{indent}if ({condition}) {{")
                lines.extend(
                    _capture(child, child_source, child_target, source, depth + 1)
                )
                lines.extend(
                    [
                        f"{indent}}} else {{",
                        f"{indent}  {child_target} = {{}};",
                        f"{indent}}}",
                    ]
                )
            else:
                lines.extend(_capture(child, child_source, child_target, source, depth))
        return lines
    if plan.kind in {"array", "reference"}:
        length = plan.length if plan.kind == "array" else "1"
        if length is None:
            raise ModelError([f"{plan.native}: capture array requires length"])
        count = str(length) if str(length).isdigit() else f"{parent}.{length}"
        if plan.kind == "reference" and plan.nullable:
            count = f"({source} == nullptr ? 0 : 1)"
        name = f"element_{depth}"
        if plan.ctype.kind == "array":
            lines = [
                f"{indent}for (std::size_t index_{depth} = 0; index_{depth} < {count}; ++index_{depth}) {{"
            ]
            lines.extend(
                _capture(
                    plan.element,
                    f"{source}[index_{depth}]",
                    f"{target}[index_{depth}]",
                    parent,
                    depth + 1,
                )
            )
            lines.append(f"{indent}}}")
            return lines
        if plan.element.kind in {"scalar", "enum", "handle"}:
            return [f"{indent}{target} = bytes(arena, {source}, {count});"]
        lines = [
            f"{indent}{target} = span(arena, {source}, {count}, [&]([[maybe_unused]] auto& arena, auto {name}) {{",
            f"{indent}  auto copied_{depth} = {name};",
        ]
        lines.extend(_capture(plan.element, name, f"copied_{depth}", parent, depth + 1))
        lines.extend([f"{indent}  return copied_{depth};", f"{indent}}});"])
        return lines
    raise ModelError([f"{plan.native}: native capture does not support {plan.kind}"])
