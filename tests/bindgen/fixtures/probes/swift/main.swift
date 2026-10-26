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
// before it reaches C, except the version query, which the check itself makes.
if ProcessInfo.processInfo.environment["MLN_PROBE_C_VERSION"] != nil {
  check(cVersion() != expectedCAbiVersion, "the version query checked")
  do {
    _ = try keywordCombine(defer: 1, self: 1, raw: 1, bindingArg0: 1)
    check(false, "a mismatched C ABI version was not reported")
  } catch let error as AbiVersionMismatchError {
    check(
      error.expected == expectedCAbiVersion && error.actual != error.expected,
      "ABI mismatch: \(error)"
    )
  }
  exit(0)
}

// A record built from its initializer defaults is zeroed with its size set,
// including its nested record.
try probeSettingsCheck(ProbeSettings())

let point = ProbePoint(type: 9.5, gain: 3.25)
let input = ProbeOptions(
  title: "",
  point: point,
  left: [point],
  right: [point, point]
)
let output = try probeRoundtrip(input: input)
check(output == input, "round trip: \(output)")
try check(
  probeRoundtrip(input: ProbeOptions(left: [])).left == [],
  "present empty array"
)
try check(
  probeRoundtrip(input: ProbeOptions()) == ProbeOptions(),
  "absent fields"
)

let entry = try keywordCombine(
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
  _ = try probeRoundtrip(input: ProbeOptions(right: Array(
    repeating: point,
    count: 9
  )))
  check(false, "native failure was not reported")
} catch let error as MaplibreError {
  check(
    error.status == .invalidArgument,
    "status: \(error)"
  )
  check(
    error.diagnostic == "right holds more than 8 points",
    "diagnostic: \(error)"
  )
}

// A status that the C API declares as absence returns nil, and any other
// failure still reports its diagnostic.
try check(probeTakeParcel() == nil, "absent parcel")
let parcel = try probeTakeParcel()
check(parcel?.id == 42, "published parcel: \(String(describing: parcel))")
try parcel?.close()
do {
  _ = try probeTakeParcel()
  check(false, "an exhausted parcel was not reported")
} catch let error as MaplibreError {
  check(
    error.status == .invalidArgument && error.diagnostic == "exhausted",
    "exhausted parcel: \(error)"
  )
}

// A drain decodes each plain record at the stride native reports and releases
// its batch, so the next drain reaches native, which has nothing queued.
let ledger = try ProbeLedgerHandle.open()
let readings = try ledger.drain().map(\.value)
check(readings == [7, 9], "strided readings: \(readings)")
try check(ledger.drain().isEmpty, "drained readings")
try ledger.close()

try check(probeReadLevel() == nil, "absent level")
try check(probeReadLevel() == 0.5, "published level")

// Caller-sized text that fits the runtime's first storage, and text that
// does not, which the runtime fetches again with enough storage.
try check(probeZerosJson(count: 0) == "[]", "empty text")
try check(probeZerosJson(count: 3) == "[0,0,0]", "short text")
let longText = try probeZerosJson(count: 300)
check(
  longText.utf8.count == 601 && longText.hasPrefix("[0,0,")
    && longText.hasSuffix(",0]"),
  "long text"
)

/// A caller-sized output array holds one element per input element, in input
/// order.
let batchPoints = [
  ProbeBatchPoint(x: 2, y: 5),
  ProbeBatchPoint(x: 7, y: 11),
  ProbeBatchPoint(x: -3, y: 17),
]
try check(
  probeBatchPoints(batchPoints) == [
    ProbeBatchPoint(x: 5, y: 3),
    ProbeBatchPoint(x: 11, y: 8),
    ProbeBatchPoint(x: 17, y: -2),
  ],
  "caller-sized points"
)
try check(
  probeBatchScores(points: batchPoints) == [7, 18, 14],
  "caller-sized scalars"
)
try check(
  probeBatchPoints([]).isEmpty,
  "empty caller-sized points"
)
try check(
  probeBatchScores(points: []).isEmpty,
  "empty caller-sized scalars"
)
