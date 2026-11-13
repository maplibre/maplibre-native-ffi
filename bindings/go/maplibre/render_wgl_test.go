//go:build cgo && mln_wgl

package maplibre

import "maplibre.org/maplibre-native-ffi/bindings/go/internal/testsupport"

const (
	buildBackend    = RenderBackendFlagOpenGL
	graphicsBackend = testsupport.BackendWGL
	// Only a Vulkan device takes a queue lock.
	queueLockSupported = false
)

// wrapDevice makes an OpenGL device in the share group of the test graphics'
// WGL context. Only a Vulkan device takes a queue lock.
func wrapDevice(context testsupport.Context, _ QueueLock) (*GPUDeviceHandle, error) {
	return WrapOpenGLGPUDevice(OpenGLContextDescriptorWGLVariant{Value: WGLContextDescriptor{
		DeviceContext:  context.WGLDeviceContext,
		ShareContext:   context.WGLContext,
		GetProcAddress: context.GetProcAddress,
	}})
}

// A session on the shared WGL context takes the caller driver, which runs on
// the thread the fixture made that context current on.
func attachBorrowedTexture(
	m *MapHandle, device *GPUDeviceHandle, size LogicalSize, textures []*testsupport.Texture,
	options RenderSessionAttachOptions,
) (*RenderSessionHandle, *Future[struct{}], error) {
	ring := make([]OpenGLBorrowedTexture, len(textures))
	for index, texture := range textures {
		ring[index] = OpenGLBorrowedTexture{Texture: texture.Info.OpenGLTexture}
	}
	info := textures[0].Info
	return m.Attach(RenderTargetOpenGLBorrowedTexturesVariant{Value: OpenGLBorrowedTexturesTarget{
		LogicalSize:    size,
		PhysicalWidth:  info.Width,
		PhysicalHeight: info.Height,
		Textures:       ring,
		Target:         info.OpenGLTarget,
	}}, device, options)
}

func withFrameView(frame *AcquiredFrameHandle, use func(frameView) error) error {
	return frame.WithOpenGLTexture(func(view OpenGLTextureFrameView) error { return use(view) })
}

func readTexture(view frameView) error {
	_, err := view.(OpenGLTextureFrameView).Texture()
	return err
}
