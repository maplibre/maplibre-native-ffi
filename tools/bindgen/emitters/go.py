"""Generate Go APIs from the shared C ownership and value model."""

import re

from tools.bindgen import docs
from tools.bindgen.compiler import compile_api, served
from tools.bindgen.model import Lowered, ModelError, Unsupported
from tools.bindgen.protocol import COMPLETION_RESULT

# The build constraint that selects each platform the module builds for. An
# OpenHarmony build is a GOOS=linux build, and GOOS=android satisfies linux.
PLATFORMS = {
    "android": "android",
    "linux": "linux && !android",
    "macos": "darwin",
    "ohos": "linux && !android",
    "windows": "windows",
}
TARGETS = frozenset(PLATFORMS)
DIRECTORY = "bindings/go/maplibre"


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


# The words of C names that Go spells as initialisms, as Go Code Review
# Comments and staticcheck's ST1003 require. Matching whole words of the C name
# leaves words such as identity, idle, glyph, and global alone.
INITIALISMS = {
    "api": "API",
    "cpu": "CPU",
    "egl": "EGL",
    "gl": "GL",
    "gles": "GLES",
    "gpu": "GPU",
    "http": "HTTP",
    "id": "ID",
    "ids": "IDs",
    "json": "JSON",
    "url": "URL",
    "urls": "URLs",
    "wgl": "WGL",
}


def _word(value: str) -> str:
    return INITIALISMS.get(value) or value[:1].upper() + value[1:]


def name(value: str) -> str:
    """The exported Go name of a snake_case C name: each word capitalized, and
    each initialism in capitals."""
    return "".join(_word(word) for word in value.split("_"))


def camel(value: str) -> str:
    """The unexported Go name of a snake_case C name: the first word as written,
    then the rest as in `name`."""
    first, *rest = value.split("_")
    return first + name("_".join(rest)) if rest else first


def doc(
    bound,
    native: str,
    declared: str | None,
    *,
    operation: bool = False,
    discard: bool = False,
) -> str:
    """The Go doc comment of a declaration, or empty when it has none. With
    `discard`, it documents a form that passes no completion."""
    found = bound.doc(native)
    return docs.go_comment(
        docs.discarding(found) if discard else found,
        symbols(bound),
        declared,
        operation=operation,
    )


def symbols(bound) -> docs.Symbol:
    """How Go spells the C declarations that its summaries name."""
    return bound.symbols("go", _symbols)


def _symbols(bound) -> docs.Symbol:
    from .go_callbacks import parameter_name
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
        except Unsupported:
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
        define=constant_name,
        extra={
            f"{COMPLETION_RESULT}.{member}": f"CommandCompletion.{name(member)}"
            for member in ("generation", "render_update_generation")
        },
        parameter=parameter_name,
    )


def constant_name(constant) -> str:
    """A constant macro's exported name: `TextureRingMaxDepth`."""
    return name(constant.name.removeprefix("MLN_").lower())


def constants_source(bound) -> str:
    """The public constant macros, each typed as its C type's Go integer, in
    one group whose members carry their own comments."""
    constants = bound.public_constants
    if not constants:
        return ""
    return (
        "const (\n"
        + "".join(
            doc(bound, constant.name, constant_name(constant))
            + f"{constant_name(constant)} {constant.type.removesuffix('_t')} = {constant.value}\n"
            for constant in constants
        )
        + ")\n"
    )


def documented_operation(
    bound, native: str, source: str, *, discard: bool = False
) -> str:
    """An operation's source with its comment before its function."""
    match = re.search(r"^func (?:\([^)]*\) )?(\w+)", source, re.MULTILINE)
    if not match:
        return source
    comment = doc(bound, native, match[1], operation=True, discard=discard)
    return source[: match.start()] + comment + source[match.start() :]


def native_call(function, *arguments, diagnostic="diagnostic"):
    """Calls a C function, passing diagnostic last when it takes one."""
    if function.diagnostic:
        arguments = (*arguments, diagnostic)
    return f"C.{function.name}({', '.join(arguments)})"


def public_dispose(handle) -> bool:
    """Whether a handle has a C dispose besides its release."""
    return bool(handle.dispose and handle.dispose != handle.release)


def owner_members(owner: str, event_source: bool) -> str:
    id_doc = (
        "// ID returns the native handle value, which runtime events report as their\n"
        "// source. It stays readable after the handle is released, so events drained\n"
        "// later still match this handle.\n"
        if event_source
        else "// ID returns the native handle value. It stays readable after the handle\n"
        "// is released.\n"
    )
    return (
        id_doc
        + f"func (handle *{owner}) ID() uint64 {{ return handle.owner().id() }}\n"
        "// IsClosed reports whether the binding has accepted a release, dispose, or\n"
        "// discard of the handle. A nil handle is closed.\n"
        f"func (handle *{owner}) IsClosed() bool {{ return handle.owner().isClosed() }}\n"
        f"// String formats the handle as {owner}(id=0x<hex>, closed=<true|false>).\n"
        f'func (handle *{owner}) String() string {{ return bindingDescribe("{owner}", handle.owner()) }}\n'
    )


def _lower(api):
    from .go_callbacks import sources as callback_sources
    from .go_operations import lower as lower_operations
    from .go_operations import operation

    bound = compile_api(api, TARGETS)
    values, lowered, generated, unsupported = lower_operations(bound)
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
        function = bound.source.functions_by_name[disposer]
        dispose = native_call(function, f"C.{native}(raw)", diagnostic="nil")
        borrow = (
            f'func borrow{owner}(raw uint64) *{owner} {{ return &{owner}{{bindingBorrow(raw, "{owner}")}} }}\n'
            if native in bound.borrowed_handles
            else ""
        )
        owners.append(
            doc(bound, native, owner)
            + f'type {owner} struct {{ *bindingOwner }}\nfunc adopt{owner}(raw uint64, parent any) *{owner} {{ owner := &{owner}{{bindingAdopt(raw,parent,"{owner}",func(raw uint64){{ {dispose} }})}}; return owner }}\n'
            + borrow
            + f"func (handle *{owner}) owner() *bindingOwner {{ if handle == nil {{ return nil }}; return handle.bindingOwner }}\n"
            + owner_members(owner, native in bound.event_sources)
        )
        used_support.add(disposer)
        if handle.dispose and public_dispose(handle):
            plan = bound.operations_by_name[handle.dispose]
            operations.append(
                documented_operation(
                    bound,
                    plan.name,
                    operation(plan, values, dispose=True),
                    discard=True,
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
        values.sources()
        + "\n"
        + constants_source(bound)
        + "\n"
        + "\n".join(owners + operations + go),
        {
            build: [source for _, source in items]
            for build, items in constrained.items()
        },
        {
            build: sorted(
                {p for plan, _ in items for p in served(plan.function, TARGETS) or ()}
            )
            for build, items in constrained.items()
        },
        header,
        c,
        generated,
        unsupported,
    )


PREAMBLE = """// Code generated by tools/bindgen; DO NOT EDIT.
{build}package maplibre
/*
#include <stdlib.h>
#include <stdbool.h>
#include "generated_callbacks.h"
*/
import "C"
"""


def go_source(chunks: str, build: str | None = None) -> str:
    """A file of the package that holds `chunks`, built only under `build`."""
    code = re.sub(r"(?m)^\s*//.*$", "", chunks)
    return (
        PREAMBLE.format(build=f"//go:build {build}\n\n" if build else "")
        + "\n".join(
            f'import "{module}"'
            for module in ("unsafe", "runtime", "runtime/cgo", "weak", "fmt")
            if module.rsplit("/", 1)[-1] + "." in code
        )
        + "\n"
        + chunks
    )


def lower(api) -> Lowered:
    bound = compile_api(api, TARGETS)
    chunks, constrained, platforms, header, c, generated, unsupported = _lower(bound)
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
    files.update(
        {
            "generated_api.go": source,
            "generated_callbacks.h": declarations,
            "generated_callbacks.c": callbacks,
        }
    )
    return Lowered(
        {f"{DIRECTORY}/{path}": text for path, text in files.items()},
        unsupported,
        sorted(generated),
    )
