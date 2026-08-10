import Foundation
@testable import MaplibreNativeFFI
import Testing

@Test func inputArenaOwnsStableCopiesForTheCall() throws {
  let arena = NativeInputArena()
  defer { withExtendedLifetime(arena) {} }
  var data = Data(#"{"type":"Point","coordinates":[2,1]}"#.utf8)
  let view = arena.view(data)
  data[0] = 0
  for byte in UInt8.min ... 64 {
    _ = arena.view(Data(repeating: byte, count: 32))
  }

  #expect(view.size == data.count)
  let copied = try Data(bytes: #require(view.data), count: view.size)
  #expect(copied == Data(#"{"type":"Point","coordinates":[2,1]}"#.utf8))

  let empty = arena.view(Data())
  #expect(empty.data == nil)
  #expect(empty.size == 0)
}
