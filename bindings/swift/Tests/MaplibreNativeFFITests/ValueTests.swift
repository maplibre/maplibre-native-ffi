import Foundation
@testable import MaplibreNativeFFI
import Testing

@Test func inputArenaKeepsJSONBytesAliveForTheCall() throws {
  let arena = NativeInputArena()
  let data = Data(#"{"type":"Point","coordinates":[2,1]}"#.utf8)
  let view = arena.view(data)

  #expect(view.size == data.count)
  let copied = try Data(bytes: #require(view.data), count: view.size)
  #expect(copied == data)
}
