/**
 * Each language's snippet files, beside its tests, where its build compiles
 * them. A file is named after the page it serves: `create_a_map.c` or
 * `CreateAMap.kt` for `create-a-map`.
 */

type Files = Record<string, string>;

const files: Record<string, Files> = {
  c: import.meta.glob("../../tests/native/docsnippets/*.c", {
    query: "?raw",
    import: "default",
    eager: true,
  }),
  dart: import.meta.glob("../../bindings/dart/test/docsnippets/*.dart", {
    query: "?raw",
    import: "default",
    eager: true,
  }),
  dotnet: import.meta.glob("../../bindings/dotnet/tests/*/DocSnippets/*.cs", {
    query: "?raw",
    import: "default",
    eager: true,
  }),
  go: import.meta.glob("../../bindings/go/docsnippets/*.go", {
    query: "?raw",
    import: "default",
    eager: true,
  }),
  kotlin: import.meta.glob(
    "../../bindings/kotlin/src/*/kotlin/**/docsnippets/*.kt",
    { query: "?raw", import: "default", eager: true },
  ),
  python: import.meta.glob("../../bindings/python/tests/docsnippets/*.py", {
    query: "?raw",
    import: "default",
    eager: true,
  }),
  rust: import.meta.glob(
    "../../bindings/rust/crates/maplibre-native-ffi/examples/*.rs",
    { query: "?raw", import: "default", eager: true },
  ),
  swift: import.meta.glob("../../bindings/swift/Tests/*/DocSnippets/*.swift", {
    query: "?raw",
    import: "default",
    eager: true,
  }),
  typescript: import.meta.glob(
    "../../bindings/typescript/test/docsnippets/*.ts",
    { query: "?raw", import: "default", eager: true },
  ),
  zig: import.meta.glob("../../bindings/zig/tests/docsnippets/*.zig", {
    query: "?raw",
    import: "default",
    eager: true,
  }),
};

/** A page's snippet source in a language, or undefined if it has none. */
export function snippetSource(
  language: string,
  page: string,
): string | undefined {
  return Object.entries(files[language] ?? {}).find(
    ([path]) => key(path) === key(page),
  )?.[1];
}

/** `create-a-map`, `create_a_map_test.go`, and `CreateAMap.kt` match. */
function key(name: string): string {
  return name
    .replace(/^.*\//, "")
    .replace(/\.[^.]+$/, "")
    .toLowerCase()
    .replace(/[^a-z0-9]/g, "")
    .replace(/test$/, "");
}
