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
  } catch let error as AbiVersionMismatchError {
    check(
      error.expected == expectedCAbiVersion && error.actual != error.expected,
      "ABI mismatch: \(error)"
    )
  }
  exit(0)
}

// A record built from its initializer defaults equals the native default,
// including its nested record.
try Maplibre.probeSettingsCheck(ProbeSettings())

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
try check(Maplibre.probeTakeParcel() == nil, "absent parcel")
let parcel = try Maplibre.probeTakeParcel()
check(parcel?.id == 42, "published parcel: \(String(describing: parcel))")
try parcel?.close()
do {
  _ = try Maplibre.probeTakeParcel()
  check(false, "an exhausted parcel was not reported")
} catch let error as MaplibreError {
  check(
    error.status == .invalidArgument && error.diagnostic == "exhausted",
    "exhausted parcel: \(error)"
  )
}

// A drain decodes each plain record at the stride native reports and releases
// its batch, so the next drain reaches native, which has nothing queued.
let ledger = try Maplibre.probeLedgerOpen()
let readings = try ledger.drain().map(\.value)
check(readings == [7, 9], "strided readings: \(readings)")
try check(ledger.drain().isEmpty, "drained readings")
try ledger.close()

try check(Maplibre.probeReadLevel() == nil, "absent level")
try check(Maplibre.probeReadLevel() == 0.5, "published level")

// Caller-sized text that fits the runtime's first storage, and text that
// does not, which the runtime fetches again with enough storage.
try check(Maplibre.probeZerosJson(count: 0) == "[]", "empty text")
try check(Maplibre.probeZerosJson(count: 3) == "[0,0,0]", "short text")
let longText = try Maplibre.probeZerosJson(count: 300)
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
  Maplibre.probeBatchPoints(batchPoints) == [
    ProbeBatchPoint(x: 5, y: 3),
    ProbeBatchPoint(x: 11, y: 8),
    ProbeBatchPoint(x: 17, y: -2),
  ],
  "caller-sized points"
)
try check(
  Maplibre.probeBatchScores(points: batchPoints) == [7, 18, 14],
  "caller-sized scalars"
)
try check(
  Maplibre.probeBatchPoints([]).isEmpty,
  "empty caller-sized points"
)
try check(
  Maplibre.probeBatchScores(points: []).isEmpty,
  "empty caller-sized scalars"
)
