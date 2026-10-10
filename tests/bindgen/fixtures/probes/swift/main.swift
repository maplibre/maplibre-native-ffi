import Foundation

func check(
  _ condition: Bool,
  _ message: @autoclosure () -> String,
  line: Int = #line
) {
  if !condition {
    FileHandle.standardError.write(Data("line \(line): \(message())\n".utf8))
    exit(1)
  }
}

// A library that reports another C ABI version fails the program's first call
// before it reaches C. The call takes only scalars, so no value's default reads
// a native struct first.
if ProcessInfo.processInfo.environment["MLN_PROBE_C_VERSION"] != nil {
  do {
    _ = try Maplibre.keywordCombine(defer: 1, self: 1, raw: 1, bindingArg0: 1)
    check(false, "a mismatched C ABI version was not reported")
  } catch let error as MaplibreError {
    check(error.kind == .abiVersionMismatch, "ABI mismatch: \(error)")
  }
  exit(0)
}

let point = ProbePoint(type: 9.5, gain: 3.25)
let input = ProbeOptions(
  title: "",
  point: point,
  left: [point],
  right: [point, point]
)
let output = try Maplibre.probeRoundtrip(input: input)
check(output == input, "round trip: \(output)")
try check(
  Maplibre.probeRoundtrip(input: ProbeOptions(left: [])).left == [],
  "present empty array"
)
try check(
  Maplibre.probeRoundtrip(input: ProbeOptions()) == ProbeOptions(),
  "absent fields"
)
for text in [nil, "", "text"] as [String?] {
  let result = try Maplibre.probeNullableText(text: text)
  check(
    result.text == text,
    "nullable text \(String(describing: text)): \(result)"
  )
}

let entry = try Maplibre.keywordCombine(
  defer: 5,
  self: 2,
  raw: 7,
  bindingArg0: 11
)
check(
  entry == KeywordEntry(type: 3, defer: 7, raw: 11),
  "keyword parameters: \(entry)"
)
/// A copy of a native default keeps its values and leaves a registration unset.
let hooks = ProbeHooks.default
check(
  hooks.limit == 4 && hooks.signal.callback == nil,
  "hooks default: \(hooks)"
)
do {
  _ = try Maplibre.probeRoundtrip(input: ProbeOptions(right: Array(
    repeating: point,
    count: 9
  )))
  check(false, "native failure was not reported")
} catch let error as MaplibreError {
  check(
    error.kind == .invalidArgument && error.rawStatus == -1,
    "status: \(error)"
  )
  check(
    error.diagnostic == "right holds more than 8 points",
    "diagnostic: \(error)"
  )
}
