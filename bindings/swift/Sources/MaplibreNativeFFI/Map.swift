extension MapHandle {
  func closeBlockingForTests() throws {
    guard let teardown = try startClose() else { return }
    try mapNativeFailure { try teardown.valueBlocking() }
  }
}
