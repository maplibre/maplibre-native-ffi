// Snippets for the docs site's Render to a texture page. They compile
// with the
// tests, which never run them.

#if os(Android) || os(Linux) || os(macOS) || os(Windows)
  import MaplibreNativeFFI

  @MainActor private var session: RenderSessionHandle!

  /// The frame wake from the Render to a window page.
  private func onFrameWake() {}

  /// Stand-ins for the host's mutex on its graphics queue.
  private func lockQueue() {}

  private func unlockQueue() {}

  /// The host's draw, which samples the image view.
  private func drawMap(_: UInt64) {}

  private func destroyTextures(_: [UInt64], _: [UInt64]) {}

  @MainActor private func drawFrame() throws {
    // #region acquire
    guard let frame = try session.acquireFrame() else {
      return // No new frame yet.
    }

    // Sample the texture, and wait for those reads to finish.
    try frame
      .withVulkanTexture { texture in try drawMap(texture.imageView) }

    try frame.release(consumerCompletion: GpuSync(kind: .cpuComplete))
    // #endregion acquire
  }

  @MainActor private func renderToTexture(
    map: MapHandle,
    vulkan: VulkanContextDescriptor,
    images: [UInt64],
    views: [UInt64],
    newImages: [UInt64],
    newViews: [UInt64]
  ) async throws {
    // #region attach-options
    let options = RenderSessionAttachOptions(
      driver: .coreWorker,
      frameWake: Wake(callback: onFrameWake),
      // Only if your renderer also submits to the graphics queue.
      queueLock: QueueLock(lock: lockQueue, unlock: unlockQueue)
    )
    // #endregion attach-options

    // #region attach-borrowed
    let ring = VulkanBorrowedTextureDescriptor(
      // The map's size in logical pixels.
      logicalSize: LogicalSize(width: 800, height: 600),
      // The size of your textures in device pixels.
      physicalWidth: 1600,
      physicalHeight: 1200,
      context: vulkan,
      // Your VkImage and VkImageView for each texture of the ring.
      textures: [
        VulkanBorrowedTexture(image: images[0], imageView: views[0]),
        VulkanBorrowedTexture(image: images[1], imageView: views[1]),
      ],
      format: 37, // VK_FORMAT_R8G8B8A8_UNORM
      finalLayout: 5 // VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    )

    let borrowed = try map.attachVulkanBorrowedTexture(
      descriptor: ring,
      options: options
    )
    session = borrowed.session
    try await borrowed.ready.value
    // #endregion attach-borrowed

    try drawFrame()

    // #region resize-borrowed
    let resized = VulkanBorrowedTextureReplacement(
      logicalSize: LogicalSize(width: 1024, height: 768),
      physicalWidth: 2048,
      physicalHeight: 1536,
      // Your new textures, at the new size.
      textures: [
        VulkanBorrowedTexture(
          image: newImages[0],
          imageView: newViews[0]
        ),
        VulkanBorrowedTexture(
          image: newImages[1],
          imageView: newViews[1]
        ),
      ]
    )

    try await session.setVulkanBorrowedTextureTarget(
      replacement: resized
    )
    // vkDestroyImageView and vkDestroyImage
    destroyTextures(images, views)
    // #endregion resize-borrowed
  }
#endif
