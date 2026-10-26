"""Render the TypeScript owner classes, top-level functions, and addon interface.

Every generated method calls one runtime helper for its execution shape:
`run`, `command`, `query`, and `attach` for completions,
`closeSync` and `closeAsync` for a release, `answerSync` for an answer that
consumes its receiver, `adopt` for a handle that native
code transfers to a new owner, and `ViewScope` for a borrowed view. A call
passes the raw handle ID, and native code reports a released one.

The delivery table tells the realm how each drained delivery reaches the host
value that a registration rooted: which callback runs, with which copied
arguments, and which handle the host takes from the batch first.
"""

from __future__ import annotations

import re

from ..semantic import output_member
from .typescript import Lowered, attachment_name, owner_native, scoped_name
from .typescript_values import (
    Unsupported,
    Values,
    doc,
    identifier,
    owner_class,
    parameter_name,
    public_name,
    registration,
)

HEADER = "// Generated from the C headers by tools/bindgen. Do not edit.\n"
# What a call that waits for a thread to start does in TypeScript, in place of
# the C notes on blocking.
THREAD_START_NOTE = (
    "Runs on a thread of its own, so a browser page can call it, and resolves "
    "once the call returns. A pending Promise keeps the event loop alive. When "
    "the realm ends first, the binding disposes the result."
)
# The handles that `mln_runtime_event_source_type` names as an event's source.
EVENT_SOURCES = frozenset({"mln_runtime", "mln_map"})


def signature(entry: Lowered) -> str:
    parts = []
    for name, ts, default in entry.parameters:
        if default == "undefined":
            parts.append(f"{name}?: {ts}")
        elif default:
            parts.append(f"{name}: {ts} = {default}")
        else:
            parts.append(f"{name}: {ts}")
    return ", ".join(parts)


def construct(bound, entry: Lowered, owned, handle_expression: str) -> str:
    """Constructs the owner of an adopted handle, or disposes the handle."""
    public = owner_class(owned.handle)
    arguments = ["ADOPT", handle_expression]
    if owned.handle.parent in bound.public_handles:
        if not owned.parent_parameter:
            raise Unsupported("owned output requires its parent owner")
        first = entry.plan.function.parameters[0].name
        arguments.append(
            "this"
            if entry.owner and first == owned.parent_parameter
            else parameter_name(owned.parent_parameter)
        )
    if entry.registrations:
        arguments.append("registrations")
    return f"adopt({handle_expression}, () => new {public}({', '.join(arguments)}), {disposer(owned.handle)})"


def disposer(handle) -> str:
    # The handle has no owner yet, so the disposal passes no owner token, and
    # it passes no completion to a dispose that takes one.
    completion = ", undefined" if handle.dispose_completion else ""
    return f"(disposed) => native.{handle.cleanup}(disposed, undefined{completion})"


def render_dispose(bound, handle, indent: str) -> str:
    """The synchronous dispose of a handle whose dispose is also its release:
    the release passes a completion, and this passes none."""
    text = doc(bound, handle.dispose, indent)
    text += f"{indent}dispose(): void {{\n"
    text += f"{indent}  closeSync(this, (handle, token) =>\n"
    text += f"{indent}    native.{handle.dispose}(handle, token, undefined),\n"
    text += f"{indent}  );\n"
    return text + f"{indent}}}\n"


def holder(entry: Lowered) -> str:
    """Where a call's accepted registrations live: its receiver, or the realm."""
    return "rootsOf(this)" if entry.owner else ""


def method_body(bound, values, entry: Lowered, discard=False) -> list[str]:
    call = f"native.{entry.plan.name}({', '.join(entry.arguments)})"
    if discard:
        call = f"native.{entry.plan.name}({', '.join([*entry.arguments, 'undefined'])})"
        lines = [f"{call};"]
        if entry.registrations:
            lines.append(f"registrations.commit({holder(entry)});")
            return [
                "withRegistrations((registrations) => {",
                *[f"  {line}" for line in lines],
                "});",
            ]
        return lines
    arguments = ", ".join([*entry.arguments, "completion"])
    start = f"(completion) => native.{entry.plan.name}({arguments})"
    if entry.registrations and entry.plan.completion and entry.shape != "attach":
        # The receiver roots the registrations once native code accepts them.
        start = (
            f"(completion) => {{ native.{entry.plan.name}({arguments}); "
            f"registrations.commit({holder(entry)}); }}"
        )
    shape = entry.shape
    if shape == "close":
        lines = [f"closeSync(this, (handle, token) => {call});"]
    elif shape == "answer":
        lines = [f"answerSync(this, (handle, token) => {call});"]
    elif shape == "close_async":
        lines = [f"return closeAsync(this, (handle, token) => submitRun({start}));"]
    elif shape == "run":
        lines = [f"return run({start});"]
    elif shape == "command":
        lines = [f"return command({start});"]
    elif shape == "query":
        result = entry.returns.removeprefix("Promise<").removesuffix(">")
        lines = [f"return query<{result}>({start});"]
    elif shape == "attach":
        member = identifier(output_member(entry.owned.parameter))
        lines = [
            f"const [{member}, ready] = attach({start}, (handle) => {construct(bound, entry, entry.owned, 'handle')});",
            f"return {{ {member}, ready }};",
        ]
    elif shape == "thread_start":
        arguments = ", ".join([*entry.arguments, "start"])
        adopted = construct(bound, entry, entry.owned, "handle")
        lines = [
            f"return threadStart((start) => native.{entry.plan.name}({arguments}), (handle) => {adopted});"
        ]
    elif shape == "owned":
        lines = [f"const handle = {call};"]
        adopted = construct(bound, entry, entry.owned, "handle")
        if entry.absent:
            lines.append(f"return handle === undefined ? undefined : {adopted};")
        else:
            lines.append(f"return {adopted};")
    elif shape == "drain":
        # Nothing queued reads as no elements.
        lines = [f"return {call} ?? [];"]
    elif shape == "view":
        view = entry.plan.view
        scope = (
            f"new ViewScope(ADOPT, this, (handle) => native.{view.begin}(handle), "
            f"(scope) => native.{view.end}(scope))"
        )
        lines = [
            f"return {scope}.use((scope) => use(new {scoped_name(entry.view)}(ADOPT, scope, {call})));"
        ]
    elif entry.registrations:
        result = "const result = " if entry.returns != "void" else ""
        commit = f"registrations.commit({holder(entry)});"
        if entry.declined:
            commit = f"if (!result) {commit}"
        lines = [f"{result}{call};", commit]
        if result:
            lines.append("return result;")
    else:
        lines = [f"return {call};" if entry.returns != "void" else f"{call};"]
    if entry.registrations:
        if len(lines) == 1 and lines[0].startswith("return "):
            body = lines[0].removeprefix("return ").removesuffix(";")
            return [f"return withRegistrations((registrations) => {body});"]
        return [
            "return withRegistrations((registrations) => {",
            *[f"  {line}" for line in lines],
            "});",
        ]
    return lines


def render_method(bound, values, entry: Lowered, indent: str) -> str:
    body = method_body(bound, values, entry)
    head = (
        "static " if entry.plan.factory else "" if entry.owner else "export function "
    )
    notes = (THREAD_START_NOTE,) if entry.shape == "thread_start" else ()
    text = doc(bound, entry.plan.name, indent, notes)
    generic = "<T>" if entry.shape == "view" else ""
    text += f"{indent}{head}{entry.method}{generic}({signature(entry)}): {entry.returns} {{\n"
    text += "".join(f"{indent}  {line}\n" for line in body)
    return text + f"{indent}}}\n"


def render_discarding(bound, values, public, entries) -> str:
    """The view of an owner whose commands discard their completions."""
    name = f"{public}Discarding"
    text = (
        f"/**\n * The commands of a {{@link {public}}} that discard their outcomes:\n"
        " * each returns once native code accepts it and allocates no completion.\n"
        " * A later completion on the same map fences them.\n */\n"
        f"export class {name} {{\n"
        f"  readonly #owner: {public};\n\n"
        "  /**\n   * Created by the API, which passes a key that only it holds.\n"
        "   *\n   * @internal\n   */\n"
        f"  constructor(key: AdoptKey, owner: {public}) {{\n"
        "    checkKey(key);\n"
        "    this.#owner = owner;\n"
        "  }\n"
    )
    for entry in sorted(entries, key=lambda e: e.method):
        body = method_body(bound, values, entry, discard=True)
        body = [
            line.replace("this.id", "this.#owner.id").replace(
                "rootsOf(this)", "rootsOf(this.#owner)"
            )
            for line in body
        ]
        text += "\n" + doc(bound, entry.plan.name, "  ")
        text += f"  {entry.method}({signature(entry)}): void {{\n"
        text += "".join(f"    {line}\n" for line in body)
        text += "  }\n"
    return text + "}\n"


def render_owner(bound, values, native: str, handle, methods) -> str:
    public = owner_class(handle)
    parameters = ["key: AdoptKey", "handle: bigint"]
    options = []
    if handle.parent in bound.public_handles:
        parameters.append(f"parent: {owner_class(bound.handles[handle.parent])}")
        options.append("parent")
    parameters.append("registrations?: Registrations")
    options.append("registrations")
    if handle.abandon:
        options.append("session: true")
    if handle.observable_root_release:
        options.append("root: true")
    summary = doc(bound, native)
    text = "/**\n * Owner of one native handle.\n */\n" if not summary else summary
    text += f"export class {public} extends Owner {{\n"
    text += (
        "  /**\n   * Created by the API, which passes a key that only it holds.\n"
        "   *\n   * @internal\n   */\n"
    )
    text += f"  constructor({', '.join(parameters)}) {{\n"
    text += f'    super(key, handle, "{public}", {{ {", ".join(options)} }});\n'
    text += "  }\n"
    if native in EVENT_SOURCES:
        # Redeclares the inherited ID only to document it for this handle.
        text += (
            "\n  /**\n"
            "   * The native handle value, which runtime events report as their\n"
            "   * source. It stays readable after close, so events drained later\n"
            "   * still match this handle.\n"
            "   */\n"
            "  declare readonly id: bigint;\n"
        )
    discarding = [e for e in methods if e.discardable]
    if discarding:
        text += (
            "\n  /** The commands of this owner that discard their outcomes. */\n"
            f"  get discarding(): {public}Discarding {{\n"
            f"    return new {public}Discarding(ADOPT, this);\n  }}\n"
        )
    # A handle without scoped cleanup gets neither `using` nor `await using`.
    scoped = [] if handle.scoped_release is None else methods
    asynchronous = next((e.method for e in scoped if e.shape == "close_async"), None)
    synchronous = (
        ("dispose" if scoped and handle.dispose_completion else None)
        or next(
            (e.method for e in scoped if e.shape == "close" and e.method == "dispose"),
            None,
        )
        or next(
            (
                e.method
                for e in scoped
                if e.shape == "close"
                and all(d == "undefined" for _, _, d in e.parameters)
            ),
            None,
        )
    )
    # defineDisposers() installs these when the host defines their symbols.
    if asynchronous or synchronous:
        text += (
            f"  /**\n   * Calls `{asynchronous or synchronous}()`, for `await using`.\n"
            "   * Hosts without `Symbol.asyncDispose`, such as ArkTS, leave it undefined.\n"
            "   */\n"
            "  declare [Symbol.asyncDispose]: () => Promise<void>;\n"
        )
    if synchronous:
        text += (
            f"  /**\n   * Calls `{synchronous}()`, for `using`.\n"
            "   * Hosts without `Symbol.dispose`, such as ArkTS, leave it undefined.\n"
            "   */\n"
            "  declare [Symbol.dispose]: () => void;\n"
        )
    rendered = [
        (entry.method, render_method(bound, values, entry, "  ")) for entry in methods
    ]
    if handle.dispose_completion:
        rendered.append(("dispose", render_dispose(bound, handle, "  ")))
    for _, method in sorted(rendered, key=lambda item: item[0]):
        text += "\n" + method
    text += "}\n"
    text += (
        f"defineDisposers({public}, "
        + (f'"{asynchronous}"' if asynchronous else "undefined")
        + ", "
        + (f'"{synchronous}"' if synchronous else "undefined")
        + ");\n"
    )
    if discarding:
        text += "\n" + render_discarding(bound, values, public, discarding)
    return text


def render_attachments(bound, lowered: list[Lowered]) -> list[str]:
    chunks = {}
    for entry in lowered:
        if entry.shape != "attach":
            continue
        name = attachment_name(entry.owned)
        member = identifier(output_member(entry.owned.parameter))
        chunks[name] = (
            f"/** A new {member} and the completion that reports it ready. */\n"
            f"export interface {name} {{\n"
            f"  /** The {member}, which the caller owns before it is ready. */\n"
            f"  readonly {member}: {owner_class(entry.owned.handle)};\n"
            "  /**\n"
            f"   * Settles once the {member} is ready. It rejects with a `MaplibreError`\n"
            "   * when it fails to become ready, so await it or attach a rejection\n"
            "   * handler: an unhandled rejection ends a Node process by default.\n"
            "   */\n"
            "  readonly ready: Promise<void>;\n}\n"
        )
    return [chunks[name] for name in sorted(chunks)]


def render_views(bound, values: Values, lowered: list[Lowered]) -> list[str]:
    chunks = {}
    for entry in lowered:
        if entry.shape != "view" or entry.view in chunks:
            continue
        value = bound.values[entry.view]
        record = public_name(value.native)
        name = scoped_name(value.native)
        view = entry.plan.view
        # The scope guards only what the view's owner lends; a copy of any
        # other member stays readable after the scope ends.
        getters = "".join(
            f'\n  get {identifier(f.name)}(): {record}["{identifier(f.name)}"] {{\n'
            + (
                f"    return this.#scope.active(this.#value).{identifier(f.name)};\n  }}\n"
                if f.scoped
                else f"    return this.#value.{identifier(f.name)};\n  }}\n"
            )
            for f in values.fields(value)
        )
        method = f"{owner_class(view.owner)}.{entry.method}"
        chunks[entry.view] = (
            f"/**\n * A `{record}` whose native objects stay valid only while the\n"
            f" * callback of {{@link {method}}} runs. Its other members stay\n"
            " * readable after the callback returns.\n */\n"
            f"export class {name} {{\n"
            f"  readonly #value: {record};\n"
            "  readonly #scope: ViewScope;\n\n"
            "  /**\n   * Created by the API, which passes a key that only it holds.\n"
            "   *\n   * @internal\n   */\n"
            f"  constructor(key: AdoptKey, scope: ViewScope, value: {record}) {{\n"
            "    checkKey(key);\n"
            "    this.#value = value;\n"
            "    this.#scope = scope;\n"
            "  }\n" + getters + "}\n"
        )
    return [chunks[name] for name in sorted(chunks)]


def delivered(bound, values: Values):
    """Each queue-delivered callback that a generated registration names, with
    the registrations and fields that name it, by delivery constant."""
    result = {}
    for value in values.used.values():
        if value.kind != "record" or not value.registration:
            continue
        entry = registration(bound, value)
        if entry.form != "queue":
            continue
        for field in entry.callbacks:
            callback = bound.callbacks[field.value.native]
            result.setdefault(callback.delivery.constant, (callback, []))[1].append(
                (entry, field)
            )
    return result


def render_delivery(bound, values: Values, callback, uses) -> str:
    """One entry of the delivery table: the host callback of a rooted
    registration, run with the delivery's copied arguments."""
    forms = {(entry.function, field.name) for entry, field in uses}
    if len(forms) != 1:
        raise Unsupported(f"{callback.native}: registrations name it differently")
    entry, field = uses[0]
    public = public_name(entry.value.native)
    if entry.function:
        function_type = public
        target = f"target as {public}"
    else:
        member = identifier(field.name)
        function_type = f'NonNullable<{public}["{member}"]>'
        target = f"(target as {public}).{member}"
    names = [parameter_name(p.name) for p in callback.arguments]
    owned = [
        (p, name)
        for p, name in zip(callback.arguments, names)
        if values.delivered_owner(callback, p)
    ]
    lines = [f"const callback = {target};"]
    if field.value.nullable and not entry.function:
        # An unset callback drops its deliveries, and the batch releases what
        # they own.
        lines.append("if (callback === undefined) return;")
    decoded = []
    for parameter, name in zip(callback.arguments, names):
        key = identifier(parameter.name)
        decoded.append(f"payload.{key}")
    lines.append(
        f"const [{', '.join(names)}] = [{', '.join(decoded)}] as Parameters<{function_type}>;"
        if names
        else ""
    )
    if owned:
        if len(owned) != 1:
            raise Unsupported(f"{callback.native}: one delivered owner at most")
        parameter, name = owned[0]
        handle = values.delivered_handle(parameter)
        # The owner takes the handle from the batch before the callback runs,
        # so closing it after a failure releases an unanswered request, which
        # native code then settles.
        lines += [
            f"const owner = takeDelivered(index, {name} as unknown as bigint, (handle) => new {owner_class(handle)}(ADOPT, handle), {disposer(handle)});",
            "try {",
            f"  callback({', '.join('owner' if n == name else n for n in names)});",
            "} catch (error) {",
            "  owner.close();",
            "  throw error;",
            "}",
        ]
    else:
        lines.append(f"callback({', '.join(names)});")
    lines = [line for line in lines if line]
    uses_index = any("index" in line for line in lines)
    payload = "payload" if names else "_payload"
    head = f"[{bound.enum_number(callback.delivery.constant)}]: (target, {payload}{', index' if uses_index else ''}) => {{"
    return (
        f"  // {callback.delivery.constant}\n  {head}\n"
        + "".join(f"    {line}\n" for line in lines)
        + "  },\n"
    )


def render_deliveries(bound, values: Values) -> str:
    deliveries = delivered(bound, values)
    entries = "".join(
        render_delivery(bound, values, callback, uses)
        for _, (callback, uses) in sorted(
            deliveries.items(), key=lambda item: bound.enum_number(item[0])
        )
    )
    return (
        "// How each drained delivery reaches the host value that its registration\n"
        "// rooted, by delivery type.\n"
        "defineDeliveries({\n"
        + "".join(f"{line}\n" for line in entries.splitlines())
        + "});\n"
    )


def code(text: str) -> str:
    """TypeScript text without its comments, whose prose imports nothing."""
    return re.sub(r"/\*.*?\*/|//[^\n]*", "", text, flags=re.DOTALL)


def value_imports(values: Values, text: str) -> str:
    names = sorted({public_name(native) for native in values.used})
    used = [name for name in names if re.search(rf"\b{name}\b", text)]
    if not used:
        return ""
    return f'import type {{ {", ".join(used)} }} from "./generated_values.ts";\n'


def runtime_imports(text: str) -> str:
    modules = {
        "./internal/addon.ts": ["native"],
        "./internal/completions.ts": [
            "type CommandCompletion",
            "attach",
            "command",
            "query",
            "run",
            "submitRun",
            "threadStart",
        ],
        "./internal/owners.ts": [
            "ADOPT",
            "type AdoptKey",
            "Owner",
            "ViewScope",
            "adopt",
            "answerSync",
            "checkKey",
            "closeAsync",
            "closeSync",
            "defineDisposers",
            "rootsOf",
            "takeDelivered",
        ],
        "./internal/realm.ts": [
            "type Registrations",
            "defineDeliveries",
            "withRegistrations",
        ],
    }
    lines = []
    for module, names in modules.items():
        used = [
            name
            for name in names
            if re.search(rf"\b{name.removeprefix('type ')}\b", text)
        ]
        if used:
            lines.append(f'import {{ {", ".join(used)} }} from "{module}";\n')
    return "".join(lines)


def render_operations(bound, values: Values, lowered: list[Lowered]):
    by_owner: dict[str | None, list[Lowered]] = {}
    for entry in lowered:
        # A factory is a static method of the handle that it creates.
        by_owner.setdefault(owner_native(entry), []).append(entry)
    chunks = []
    for entry in sorted(by_owner.get(None, []), key=lambda e: e.method):
        chunks.append(render_method(bound, values, entry, ""))
    for native, handle in sorted(bound.public_handles.items()):
        chunks.append(
            render_owner(bound, values, native, handle, by_owner.get(native, []))
        )
    chunks += render_attachments(bound, lowered)
    chunks += render_views(bound, values, lowered)
    chunks.append(render_deliveries(bound, values))
    body = "\n".join(chunks)
    operations = (
        HEADER
        + "\n"
        + runtime_imports(code(body))
        + value_imports(values, code(body))
        + "\n"
        + body
    )
    interface = []
    for entry in sorted(lowered, key=lambda e: e.plan.name):
        parameters = ", ".join(f"{name}: {ts}" for name, ts in entry.native_parameters)
        interface.append(f"  {entry.plan.name}({parameters}): {entry.native_result};\n")
    for begin, end in sorted(view_scopes(lowered)):
        interface.append(f"  {begin}(handle: bigint): bigint;\n")
        interface.append(f"  {end}(scope: bigint): void;\n")
    interface_text = "".join(interface)
    native = (
        HEADER
        + "\n"
        + value_imports(values, interface_text)
        + "\n"
        + "/** The addon function of each generated operation, by C function name. */\n"
        + "export interface NativeOperations {\n"
        + interface_text
        + "}\n"
    )
    if (version := bound.source.c_version) is not None:
        native += (
            "\n/** The C ABI contract version of the headers this binding was generated from. */\n"
            f"export const MLN_C_VERSION = {version};\n"
        )
    return {
        "bindings/typescript/src/generated_operations.ts": operations,
        "bindings/typescript/src/generated_native.ts": native,
    }


def view_scopes(lowered: list[Lowered]) -> set[tuple[str, str]]:
    """The begin and end functions of each borrowed view scope that a
    generated view uses."""
    return {
        (entry.plan.view.begin, entry.plan.view.end)
        for entry in lowered
        if entry.shape == "view"
    }
