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
