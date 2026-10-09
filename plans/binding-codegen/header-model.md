# Header model

The shared frontend parses the public C headers with libclang and produces the
language-neutral model in `tools/bindgen/model.py`. Header attributes carry the
semantic information that a C signature erases. `tools/bindgen/schema.py`
validates those contracts before an emitter runs.

## Parser and contracts

The frontend uses the pinned `libclang` Python package. LLVM recommends
[libclang's stable C interface](https://clang.llvm.org/docs/Tooling.html) for
tools that need declaration traversal without depending on its changing C++ AST.
Its [tutorial](https://clang.llvm.org/docs/LibClang.html) describes the cursor
and type APIs used here. Clang parses typedefs, parameter declarations, callback
signatures, record declarations, nested unions, and enum expressions.

The model preserves typedef identity alongside canonical C types. Map and
runtime handles remain distinct even though both use the same integer ABI.
Pointers preserve pointee qualifiers, callbacks preserve their function types,
and records preserve their declared fields. Callback typedefs also preserve
parameter names and metadata. The model represents incomplete records, bit
fields, and fixed or incomplete arrays so that an emitter can reject an
unsupported shape explicitly.

`MLN_BINDING("key=value;...")` expands to a Clang annotation during extraction.
Ordinary builds expand it to nothing. Annotations belong to the declaration,
field, parameter, or typedef whose contract they describe. They contain no
target-language templates or function-name inventories.

A completion's `const void *` payload requires explicit result metadata. For
example, a layer-list query declares its execution class, element type, array
shape, and borrowed ownership. Field annotations describe nested string
encodings and the representation of optional values. Emitters must copy borrowed
results before the completion returns. An owned handle result instead transfers
responsibility for the handle's declared release operation.

All public functions require an execution category. Commands preserve their
disposition and generation receipt; queries produce values; operations cover
other asynchronous work; lifecycle calls change ownership. Immediate reads,
snapshots, event batches, and render-driver calls retain separate categories.
The accepted vocabulary and cross-field checks live in `schema.py`.

## Verification and remaining work

The initial extraction covers all 283 exported functions, 126 records, 76 enums,
and 230 typedefs in the headers. Inventory mode permits unannotated headers
during migration. Validation mode requires complete execution and erased-result
contracts and rejects invalid C, unknown metadata keys, missing result types,
and contradictory pointer directions.

Parser tests exercise typedef identity, pointer qualifiers, callback signatures,
nested unions, array lengths, evaluated enum expressions, and determinism across
checkout paths. Contract tests exercise erased completion types, metadata
mistakes, const output pointers, and variadic functions. Shared emitter tests
mutate valid headers to require ownership transfer, nullable record fields, and
bit-field layouts. Each emitter must either apply those semantics or report the
declaration as unsupported; recognizing an annotation in the schema does not
make its lowering safe.

The parser uses the configured Clang driver's builtin headers and the selected
system SDK. Cross-target extraction requires that target's include paths and
compiler arguments. The current validation runs on macOS; Windows and Linux
toolchain discovery still need CI evidence.

The model describes declarations owned by this library's include directory.
External plugin ABI types remain referenced by their C names and canonical
types. A plugin authoring emitter would need a separate ownership contract for
that independently versioned API. Host compiler types such as `size_t` retain
their typedef identities; emitters must preserve their target width.

Complete safe binding generation also needs field and parameter contracts for
every pointer, count, optional value, mask, callback registration, and ownership
transition. Parsing a declaration does not establish those facts. Each emitter
reports unsupported declarations and their missing contracts explicitly; a
successful inventory is separate from complete binding coverage.
