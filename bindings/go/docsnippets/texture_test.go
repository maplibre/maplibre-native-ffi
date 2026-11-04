// Snippets for the docs site's Render to a texture page. They compile with the
// tests, which never run them.
package docsnippets

import (
	"context"
	"errors"
	"sync"

	"github.com/maplibre/maplibre-native-ffi/bindings/go/maplibre"
)

// session and onFrameWake come from the Render to a window page's file.

// The host's draw, which samples the image view.
func drawMap(imageView uint64) {}

func destroyTextures(images, views [2]uint64) {}

func drawFrame() error {
	// #region acquire
	frame, err := session.AcquireFrame()
	if err != nil {
		return err
	}
	if frame == nil {
		return nil // No new frame yet.
	}

	// Sample the texture, and wait for those reads to finish.
	err = frame.WithVulkanTexture(
		func(texture maplibre.VulkanTextureFrameView) error {
			imageView, err := texture.ImageView()
			if err != nil {
				return err
			}
			drawMap(imageView)
			return nil
		},
	)

	return errors.Join(err, frame.Close(maplibre.GPUSync{Kind: maplibre.GPUSyncKindCPUComplete}))
	// #endregion acquire
}

func renderToTexture(
	ctx context.Context,
	m *maplibre.MapHandle,
	vulkan maplibre.VulkanContextDescriptor,
	queue *sync.Mutex,
	images, views, newImages, newViews [2]uint64,
) error {
	// #region attach-options
	options := maplibre.RenderSessionAttachOptions{
		Driver:    maplibre.RenderDriverKindCoreWorker,
		FrameWake: maplibre.Wake{Callback: onFrameWake},
		// Only if your renderer also submits to the graphics queue, under
		// the same mutex.
		QueueLock: maplibre.QueueLock{
			Lock: queue.Lock, Unlock: queue.Unlock,
		},
	}
	// #endregion attach-options

	// #region attach-borrowed
	ring := maplibre.VulkanBorrowedTextureDescriptor{
		// The map's size in logical pixels.
		LogicalSize: maplibre.LogicalSize{Width: 800, Height: 600},
		// The size of your textures in device pixels.
		PhysicalWidth:  1600,
		PhysicalHeight: 1200,
		Context:        vulkan,
		// Your VkImage and VkImageView for each texture of the ring.
		Textures: []maplibre.VulkanBorrowedTexture{
			{Image: images[0], ImageView: views[0]},
			{Image: images[1], ImageView: views[1]},
		},
		Format:      37, // VK_FORMAT_R8G8B8A8_UNORM
		FinalLayout: 5,  // VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	}

	borrowed, ready, err := m.AttachVulkanBorrowedTexture(ring, options)
	if err != nil {
		return err
	}
	session = borrowed
	if _, err := ready.Await(ctx); err != nil {
		return err
	}
	// #endregion attach-borrowed

	if err := drawFrame(); err != nil {
		return err
	}

	// #region resize-borrowed
	resized := maplibre.VulkanBorrowedTextureReplacement{
		LogicalSize:    maplibre.LogicalSize{Width: 1024, Height: 768},
		PhysicalWidth:  2048,
		PhysicalHeight: 1536,
		// Your new textures, at the new size.
		Textures: []maplibre.VulkanBorrowedTexture{
			{Image: newImages[0], ImageView: newViews[0]},
			{Image: newImages[1], ImageView: newViews[1]},
		},
	}

	retargeted, err := session.SetVulkanBorrowedTextureTarget(resized)
	if err != nil {
		return err
	}
	if _, err := retargeted.Await(ctx); err != nil {
		return err
	}
	// vkDestroyImageView and vkDestroyImage
	destroyTextures(images, views)
	// #endregion resize-borrowed
	return nil
}

// An example without an output comment compiles but never runs.
func Example_texture() {
	var none [2]uint64
	_ = renderToTexture(context.Background(), nil, maplibre.VulkanContextDescriptor{}, &sync.Mutex{}, none, none, none, none)
}
