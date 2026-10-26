"""Generate Go APIs from the shared C ownership and value model."""

import re

from tools.bindgen import docs
from tools.bindgen.compiler import compile_api, served
from tools.bindgen.model import ModelError
from tools.bindgen.protocol import COMPLETION_RESULT

# The build constraint that selects each platform the module builds for. An
# OpenHarmony build is a GOOS=linux build, and GOOS=android satisfies linux.
PLATFORMS = {
    "android": "android",
    "linux": "linux && !android",
    "macos": "darwin",
    "ohos": "linux && !android",
}
TARGETS = frozenset(PLATFORMS)


def constraint(plan) -> str | None:
    """The build constraint of an operation that serves only some platforms."""
    platforms = served(plan.function, TARGETS)
    if platforms is None:
        return None
    terms = sorted({PLATFORMS[platform] for platform in platforms})
    if {"android", "linux && !android"} <= set(terms):
        terms = sorted({*terms} - {"android", "linux && !android"} | {"linux"})
    if len(terms) == 1:
        return terms[0]
    return " || ".join(f"({term})" if "&&" in term else term for term in terms)


GO_KEYWORDS = {
    "break",
    "default",
    "func",
    "interface",
    "select",
    "case",
    "defer",
    "go",
    "map",
    "struct",
    "chan",
    "else",
    "goto",
    "package",
    "switch",
    "const",
    "fallthrough",
    "if",
    "range",
    "type",
    "continue",
    "for",
    "import",
    "return",
    "var",
}


def native_identifier(value: str) -> str:
    return "_" + value if value in GO_KEYWORDS else value


def name(value: str) -> str:
    return "".join(word[:1].upper() + word[1:] for word in value.split("_"))


def doc(bound, native: str, declared: str | None, *, operation: bool = False) -> str:
    """The Go doc comment of a declaration, or empty when it has none."""
    return docs.go_comment(
        bound.doc(native), symbols(bound), declared, operation=operation
    )


def symbols(bound) -> docs.Symbol:
    """How Go spells the C declarations that its summaries name."""
    return bound.symbols("go", _symbols)


def _symbols(bound) -> docs.Symbol:
    from .go_operations import declared_name, receiver_type_name
    from .go_values import Values, enum_member, public, union_name, variant_name

    values = Values(bound)
    records = [value for value in bound.values.values() if value.kind == "record"]

    def type_(native: str) -> str | None:
        if handle := bound.public_handles.get(native):
            if handle.dispose in bound.source.runtime_exports:
                return None
            return values.owner(native)
        value = bound.values.get(native)
        # A callback is a func type, and a slice record is a Go slice, so
        # neither has a name. The completion runtime presents a command's
        # result as CommandCompletion.
        if value is None or values.slice(value) or native == COMPLETION_RESULT:
            return None
        if value.response:
            return public(native) + "Scope"
        if value.kind == "union":
            return union_name(records, value)
        if value.kind in {"record", "enum"}:
            return public(native)
        return None

    def operation(plan) -> str | None:
        try:
            method = declared_name(plan, bound)
        except ModelError:
            return None
        receiver = receiver_type_name(plan, values)
        return f"{receiver}.{method}" if receiver else method

    def field(value, member) -> str | None:
        typ = type_(value.native)
        if typ is None or value.response or value.union_member:
            return None
        if value.kind == "union":
            return variant_name(typ, member)
        return f"{typ}.{name(member.name)}"

    def flag(value, mask_flag) -> str | None:
        return f"{public(value.native)}.{name(mask_flag.member)}"

    return docs.resolver(
        bound,
        type=type_,
        operation=operation,
        constant=enum_member,
        field=field,
        flag=flag,
        extra={
            f"{COMPLETION_RESULT}.{member}": f"CommandCompletion.{name(member)}"
            for member in ("generation", "render_update_generation")
        },
    )


def documented_operation(bound, native: str, source: str) -> str:
    """An operation's source with its comment before its function."""
    match = re.search(r"^func (?:\([^)]*\) )?(\w+)", source, re.MULTILINE)
    if not match:
        return source
    comment = doc(bound, native, match[1], operation=True)
    return source[: match.start()] + comment + source[match.start() :]


def native_call(function, *arguments, diagnostic="diagnostic"):
    """Calls a C function, passing diagnostic last when it takes one."""
    if function.diagnostic:
        arguments = (*arguments, diagnostic)
    return f"C.{function.name}({', '.join(arguments)})"


def event_sources(api) -> set[str]:
    """The handles that a runtime event can name as its source."""
    return {
        "mln_" + value.name.removeprefix("MLN_RUNTIME_EVENT_SOURCE_").lower()
        for enum in api.enums
        if enum.name == "mln_runtime_event_source_type"
        for value in enum.values
    }


def public_dispose(handle) -> bool:
    """Whether a handle has a C dispose besides a void release: a separate
    dispose, or a release that disposes and also takes a completion."""
    return bool(
        handle.dispose
        and (handle.dispose != handle.release or handle.dispose_completion)
    )


def owner_members(owner: str, event_source: bool) -> str:
    """The state members that every handle owner declares."""
    id_doc = (
        "// Id returns the native handle value, which runtime events report as their\n"
        "// source. It stays readable after the handle closes, so events drained later\n"
        "// still match this handle.\n"
        if event_source
        else "// Id returns the native handle value. It stays readable after the handle\n"
        "// closes.\n"
    )
    return (
        id_doc
        + f"func (handle *{owner}) Id() uint64 {{ return handle.owner().id() }}\n"
        "// IsClosed reports whether the binding has accepted a release, dispose, or\n"
        "// discard of the handle. A nil handle is closed.\n"
        f"func (handle *{owner}) IsClosed() bool {{ return handle.owner().isClosed() }}\n"
        f"// String formats the handle as {owner}(id=0x<hex>, closed=<true|false>).\n"
        f'func (handle *{owner}) String() string {{ return bindingDescribe("{owner}", handle.owner()) }}\n'
    )


def lower(api):
    from .go_callbacks import sources as callback_sources
    from .go_operations import lower as lower_operations
    from .go_operations import operation

    bound = compile_api(api, TARGETS)
    values, lowered, generated, unsupported = lower_operations(bound)
    # The operations that serve every platform, and those of each constraint.
    operations, constrained = [], {}
    for plan, source in lowered:
        if (build := constraint(plan)) is None:
            operations.append(source)
        else:
            constrained.setdefault(build, []).append((plan, source))
    for value in bound.public_values.values():
        if value.kind == "enum":
            values.require(value)
    owners = []
    used_support = set()
    sources = event_sources(bound.source)
    for native, handle in sorted(bound.public_handles.items()):
        if handle.dispose in bound.source.runtime_exports:
            continue
        owner = values.owner(native)
        disposer = handle.cleanup or handle.release
        declaration = next(
            (p.function for p in bound.operations if p.name == disposer), None
        )
        if not handle.cleanup and (
            not declaration
            or declaration.return_type.canonical != "void"
            or len(declaration.parameters) != 1
        ):
            raise ModelError(
                [f"{native}: automatic ownership requires an unconditional disposer"]
            )
        # A dispose that is also the release takes a completion, which a
        # disposer leaves null.
        function = bound.source.functions_by_name[disposer]
        dispose = native_call(
            function,
            *(
                "nil" if p.name == handle.dispose_completion else f"C.{native}(raw)"
                for p in function.parameters
            ),
            diagnostic="nil",
        )
        owners.append(
            doc(bound, native, owner)
            + f'type {owner} struct {{ *bindingOwner }}\nfunc adopt{owner}(raw uint64, parent any) *{owner} {{ owner := &{owner}{{bindingAdopt(raw,parent,"{owner}",func(raw uint64){{ {dispose} }})}}; return owner }}\n'
            f"// owner returns the handle's owner state, or nil for a nil handle.\nfunc (handle *{owner}) owner() *bindingOwner {{ if handle == nil {{ return nil }}; return handle.bindingOwner }}\n"
            + owner_members(owner, native in sources)
        )
        used_support.add(disposer)
        if public_dispose(handle):
            plan = bound.operations_by_name[handle.dispose]
            operations.append(
                documented_operation(
                    bound, plan.name, operation(plan, values, dispose=True)
                )
            )
    go, header, c = [], [], []
    for record in list(values.records.values()):
        if record.registration:
            a, b, d = callback_sources(values, record)
            go.append(a)
            header.append(b)
            c.append(d)
    for plan in bound.operations:
        if (
            plan.name in generated
            and plan.receiver
            and plan.completion
            and plan.completion.discardable
        ):
            if (build := constraint(plan)) is None:
                operations.append(operation(plan, values, discard=True))
            else:
                constrained[build].append((plan, operation(plan, values, discard=True)))
        if plan.role == "support" and plan.name in used_support:
            generated.append(plan.name)
            unsupported.pop(plan.name, None)
    command_owners = {
        values.owner(p.value.native)
        for plan in bound.operations
        if plan.name in generated and plan.completion and plan.completion.discardable
        for p in plan.inputs
        if p.name == plan.receiver
    }
    for owner in sorted(command_owners):
        owners.append(
            f"type {owner}Discarding struct {{ owner *{owner} }}\n"
            f"func (receiver *{owner}) Discarding() {owner}Discarding {{ return {owner}Discarding{{receiver}} }}\n"
        )
    return (
        values.sources() + "\n" + "\n".join(owners + operations + go),
        {
            build: [source for _, source in items]
            for build, items in constrained.items()
        },
        {
            build: sorted(
                {p for plan, _ in items for p in served(plan.function, TARGETS)}
            )
            for build, items in constrained.items()
        },
        header,
        c,
        generated,
        unsupported,
    )


def coverage(api):
    bound = compile_api(api, TARGETS)
    *_, generated, unsupported = lower(bound)
    return {
        "generated": sorted(generated),
        "unsupported": unsupported,
    }


PREAMBLE = """// Code generated by tools/bindgen; DO NOT EDIT.
{build}package maplibre
/*
#cgo pkg-config: maplibre-native-c
#include <stdlib.h>
#include <stdbool.h>
#include "maplibre_native_c/plugin.h"
#include "generated_callbacks.h"
*/
import "C"
"""


def go_source(chunks: str, build: str | None = None) -> str:
    """A file of the package that holds `chunks`, built only under `build`."""
    return (
        PREAMBLE.format(build=f"//go:build {build}\n\n" if build else "")
        + "\n".join(
            f'import "{module}"'
            for module in ("unsafe", "runtime", "runtime/cgo", "weak", "fmt")
            if module.rsplit("/", 1)[-1] + "." in chunks
        )
        + "\n"
        + chunks
    )


def generate(api):
    bound = compile_api(api, TARGETS)
    chunks, constrained, platforms, header, c, _, _ = lower(bound)
    source = go_source(chunks)
    files = {
        f"generated_api_{'_'.join(platforms[build])}.go": go_source(
            "\n".join(operations), build
        )
        for build, operations in constrained.items()
    }
    declarations = (
        """// Code generated by tools/bindgen; DO NOT EDIT.
#ifndef MLN_GO_GENERATED_CALLBACKS_H
#define MLN_GO_GENERATED_CALLBACKS_H
#include "maplibre_native_c.h"
#include "binding_callback.h"
void binding_release(void*);
"""
        + "\n".join(header)
        + "\n"
        + "\n#endif\n"
    )
    callbacks = """// Code generated by tools/bindgen; DO NOT EDIT.
#include "generated_callbacks.h"
extern void mlnGoCallbackRelease(void*);
void binding_release(void *p) { mlnGoCallbackRelease(p); }
""" + "\n".join(c)
    return {
        **files,
        "generated_api.go": source,
        "generated_callbacks.h": declarations,
        "generated_callbacks.c": callbacks,
    }
