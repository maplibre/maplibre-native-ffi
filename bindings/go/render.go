package maplibre

import (
	"runtime"
	"sync"

	"github.com/maplibre/maplibre-native-ffi/bindings/go/internal/capi"
	"github.com/maplibre/maplibre-native-ffi/bindings/go/internal/handle"
	internalstatus "github.com/maplibre/maplibre-native-ffi/bindings/go/internal/status"
)

// RenderBackendMask preserves the render backend bits reported by the native
// library. Unknown future bits remain in the mask.
type RenderBackendMask uint32

const (
	RenderBackendMetal  RenderBackendMask = RenderBackendMask(capi.RenderBackendFlagMetal)
	RenderBackendVulkan RenderBackendMask = RenderBackendMask(capi.RenderBackendFlagVulkan)
)

// Has reports whether all backend bits in backend are present.
func (mask RenderBackendMask) Has(backend RenderBackendMask) bool {
	return mask&backend == backend
}

// NativePointer is a borrowed opaque backend-native address.
//
// It grants no memory access and transfers no ownership. The binding converts
// it to unsafe.Pointer only at cgo boundaries for C APIs that accept opaque
// backend handles.
type NativePointer uintptr

// RenderTargetExtent is a logical render target extent in UI pixels.
type RenderTargetExtent struct {
	Width       uint32
	Height      uint32
	ScaleFactor float64
}

// TextureImageInfo describes CPU readback image metadata.
type TextureImageInfo struct {
	Width      uint32
	Height     uint32
	Stride     uint32
	ByteLength uint64
}

// MetalContextDescriptor contains Metal backend context handles.
type MetalContextDescriptor struct {
	Device NativePointer
}

// VulkanContextDescriptor contains Vulkan backend context handles.
type VulkanContextDescriptor struct {
	Instance                 NativePointer
	PhysicalDevice           NativePointer
	Device                   NativePointer
	GraphicsQueue            NativePointer
	GraphicsQueueFamilyIndex uint32
}

// MetalSurfaceDescriptor describes a Metal-backed surface render target.
type MetalSurfaceDescriptor struct {
	Extent  RenderTargetExtent
	Context MetalContextDescriptor
	Layer   NativePointer
}

// VulkanSurfaceDescriptor describes a Vulkan-backed surface render target.
type VulkanSurfaceDescriptor struct {
	Extent  RenderTargetExtent
	Context VulkanContextDescriptor
	Surface NativePointer
}

// MetalOwnedTextureDescriptor describes a Metal session-owned texture render target.
type MetalOwnedTextureDescriptor struct {
	Extent  RenderTargetExtent
	Context MetalContextDescriptor
}

// MetalBorrowedTextureDescriptor describes a Metal caller-owned texture render target.
type MetalBorrowedTextureDescriptor struct {
	Extent  RenderTargetExtent
	Texture NativePointer
}

// VulkanOwnedTextureDescriptor describes a Vulkan session-owned texture render target.
type VulkanOwnedTextureDescriptor struct {
	Extent  RenderTargetExtent
	Context VulkanContextDescriptor
}

// VulkanBorrowedTextureDescriptor describes a Vulkan caller-owned texture render target.
type VulkanBorrowedTextureDescriptor struct {
	Extent        RenderTargetExtent
	Context       VulkanContextDescriptor
	Image         NativePointer
	ImageView     NativePointer
	Format        uint32
	InitialLayout uint32
	FinalLayout   uint32
}

// RenderSessionHandle owns a map render session.
type RenderSessionHandle struct {
	state  *handle.State[capi.RenderSession]
	parent *MapHandle
	mu     sync.Mutex
	frame  bool
}

// MetalOwnedTextureFrame is an acquired session-owned Metal texture frame.
// Backend handles are borrowed and remain valid only until Close. Close the
// frame on the render session owner thread before resizing, rendering, reading
// back, detaching, closing the session, or acquiring another frame.
type MetalOwnedTextureFrame struct {
	Generation  uint64
	Width       uint32
	Height      uint32
	ScaleFactor float64
	Texture     NativePointer
	Device      NativePointer
	PixelFormat uint64

	session *RenderSessionHandle
	raw     capi.MetalOwnedTextureFrame
	mu      sync.Mutex
	closed  bool
}

// VulkanOwnedTextureFrame is an acquired session-owned Vulkan texture frame.
// Backend handles are borrowed and remain valid only until Close. Close the
// frame on the render session owner thread before resizing, rendering, reading
// back, detaching, closing the session, or acquiring another frame.
type VulkanOwnedTextureFrame struct {
	Generation  uint64
	Width       uint32
	Height      uint32
	ScaleFactor float64
	Image       NativePointer
	ImageView   NativePointer
	Device      NativePointer
	Format      uint32
	Layout      uint32

	session *RenderSessionHandle
	raw     capi.VulkanOwnedTextureFrame
	mu      sync.Mutex
	closed  bool
}

func (extent RenderTargetExtent) toCAPI() capi.RenderTargetExtent {
	return capi.RenderTargetExtent{Width: extent.Width, Height: extent.Height, ScaleFactor: extent.ScaleFactor}
}

func textureImageInfoFromCAPI(info capi.TextureImageInfo) TextureImageInfo {
	return TextureImageInfo{Width: info.Width, Height: info.Height, Stride: info.Stride, ByteLength: info.ByteLength}
}

func (descriptor MetalSurfaceDescriptor) toCAPI() capi.MetalSurfaceDescriptor {
	return capi.MetalSurfaceDescriptor{
		Extent: descriptor.Extent.toCAPI(),
		Context: capi.MetalContextDescriptor{
			Device: uintptr(descriptor.Context.Device),
		},
		Layer: uintptr(descriptor.Layer),
	}
}

func (descriptor VulkanSurfaceDescriptor) toCAPI() capi.VulkanSurfaceDescriptor {
	return capi.VulkanSurfaceDescriptor{
		Extent: descriptor.Extent.toCAPI(),
		Context: capi.VulkanContextDescriptor{
			Instance:                 uintptr(descriptor.Context.Instance),
			PhysicalDevice:           uintptr(descriptor.Context.PhysicalDevice),
			Device:                   uintptr(descriptor.Context.Device),
			GraphicsQueue:            uintptr(descriptor.Context.GraphicsQueue),
			GraphicsQueueFamilyIndex: descriptor.Context.GraphicsQueueFamilyIndex,
		},
		Surface: uintptr(descriptor.Surface),
	}
}

func (descriptor MetalOwnedTextureDescriptor) toCAPI() capi.MetalOwnedTextureDescriptor {
	return capi.MetalOwnedTextureDescriptor{
		Extent:  descriptor.Extent.toCAPI(),
		Context: capi.MetalContextDescriptor{Device: uintptr(descriptor.Context.Device)},
	}
}

func (descriptor MetalBorrowedTextureDescriptor) toCAPI() capi.MetalBorrowedTextureDescriptor {
	return capi.MetalBorrowedTextureDescriptor{
		Extent:  descriptor.Extent.toCAPI(),
		Texture: uintptr(descriptor.Texture),
	}
}

func (descriptor VulkanOwnedTextureDescriptor) toCAPI() capi.VulkanOwnedTextureDescriptor {
	return capi.VulkanOwnedTextureDescriptor{
		Extent:  descriptor.Extent.toCAPI(),
		Context: descriptor.Context.toCAPI(),
	}
}

func (descriptor VulkanBorrowedTextureDescriptor) toCAPI() capi.VulkanBorrowedTextureDescriptor {
	return capi.VulkanBorrowedTextureDescriptor{
		Extent:        descriptor.Extent.toCAPI(),
		Context:       descriptor.Context.toCAPI(),
		Image:         uintptr(descriptor.Image),
		ImageView:     uintptr(descriptor.ImageView),
		Format:        descriptor.Format,
		InitialLayout: descriptor.InitialLayout,
		FinalLayout:   descriptor.FinalLayout,
	}
}

func (context VulkanContextDescriptor) toCAPI() capi.VulkanContextDescriptor {
	return capi.VulkanContextDescriptor{
		Instance:                 uintptr(context.Instance),
		PhysicalDevice:           uintptr(context.PhysicalDevice),
		Device:                   uintptr(context.Device),
		GraphicsQueue:            uintptr(context.GraphicsQueue),
		GraphicsQueueFamilyIndex: context.GraphicsQueueFamilyIndex,
	}
}

func newRenderSessionHandle(parent *MapHandle, session *capi.RenderSession) (*RenderSessionHandle, error) {
	state, err := handle.New(session, "RenderSessionHandle", parent)
	if err != nil {
		return nil, newBindingError(ErrInvalidArgument, err.Error())
	}
	return &RenderSessionHandle{state: state, parent: parent}, nil
}

// AttachMetalSurface attaches a Metal native surface render target to this map.
func (m *MapHandle) AttachMetalSurface(descriptor MetalSurfaceDescriptor) (*RenderSessionHandle, error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}
	defer m.state.KeepAlive()

	var session *capi.RenderSession
	if err := checkNative(func() capi.Status { return capi.MetalSurfaceAttach(ptr, descriptor.toCAPI(), &session) }); err != nil {
		return nil, err
	}
	return newRenderSessionHandle(m, session)
}

// AttachVulkanSurface attaches a Vulkan native surface render target to this map.
func (m *MapHandle) AttachVulkanSurface(descriptor VulkanSurfaceDescriptor) (*RenderSessionHandle, error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}
	defer m.state.KeepAlive()

	var session *capi.RenderSession
	if err := checkNative(func() capi.Status { return capi.VulkanSurfaceAttach(ptr, descriptor.toCAPI(), &session) }); err != nil {
		return nil, err
	}
	return newRenderSessionHandle(m, session)
}

// AttachMetalOwnedTexture attaches a Metal session-owned texture render target.
func (m *MapHandle) AttachMetalOwnedTexture(descriptor MetalOwnedTextureDescriptor) (*RenderSessionHandle, error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}
	defer m.state.KeepAlive()

	var session *capi.RenderSession
	if err := checkNative(func() capi.Status { return capi.MetalOwnedTextureAttach(ptr, descriptor.toCAPI(), &session) }); err != nil {
		return nil, err
	}
	return newRenderSessionHandle(m, session)
}

// AttachMetalBorrowedTexture attaches a Metal caller-owned texture render target.
func (m *MapHandle) AttachMetalBorrowedTexture(descriptor MetalBorrowedTextureDescriptor) (*RenderSessionHandle, error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}
	defer m.state.KeepAlive()

	var session *capi.RenderSession
	if err := checkNative(func() capi.Status { return capi.MetalBorrowedTextureAttach(ptr, descriptor.toCAPI(), &session) }); err != nil {
		return nil, err
	}
	return newRenderSessionHandle(m, session)
}

// AttachVulkanOwnedTexture attaches a Vulkan session-owned texture render target.
func (m *MapHandle) AttachVulkanOwnedTexture(descriptor VulkanOwnedTextureDescriptor) (*RenderSessionHandle, error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}
	defer m.state.KeepAlive()

	var session *capi.RenderSession
	if err := checkNative(func() capi.Status { return capi.VulkanOwnedTextureAttach(ptr, descriptor.toCAPI(), &session) }); err != nil {
		return nil, err
	}
	return newRenderSessionHandle(m, session)
}

// AttachVulkanBorrowedTexture attaches a Vulkan caller-owned texture render target.
func (m *MapHandle) AttachVulkanBorrowedTexture(descriptor VulkanBorrowedTextureDescriptor) (*RenderSessionHandle, error) {
	ptr, err := m.ptr()
	if err != nil {
		return nil, err
	}
	defer m.state.KeepAlive()

	var session *capi.RenderSession
	if err := checkNative(func() capi.Status { return capi.VulkanBorrowedTextureAttach(ptr, descriptor.toCAPI(), &session) }); err != nil {
		return nil, err
	}
	return newRenderSessionHandle(m, session)
}

func (session *RenderSessionHandle) ptr() (*capi.RenderSession, error) {
	if session == nil || session.state == nil {
		return nil, newBindingError(ErrInvalidArgument, "RenderSessionHandle is nil")
	}
	ptr, live := session.state.Ptr()
	if !live {
		return nil, newBindingError(ErrInvalidArgument, "RenderSessionHandle is closed")
	}
	return ptr, nil
}

func (session *RenderSessionHandle) withNoAcquiredFrame(call func() error) error {
	session.mu.Lock()
	defer session.mu.Unlock()
	if session.frame {
		return newBindingError(ErrInvalidState, "texture frame is still acquired")
	}
	return call()
}

func (session *RenderSessionHandle) markFrameReleased() {
	session.mu.Lock()
	session.frame = false
	session.mu.Unlock()
}

// Resize changes the render session target extent.
func (session *RenderSessionHandle) Resize(extent RenderTargetExtent) error {
	ptr, err := session.ptr()
	if err != nil {
		return err
	}
	defer session.state.KeepAlive()
	defer session.parent.state.KeepAlive()
	return session.withNoAcquiredFrame(func() error {
		return checkNative(func() capi.Status { return capi.RenderSessionResize(ptr, extent.toCAPI()) })
	})
}

// RenderUpdate renders one frame/update into the attached render target.
func (session *RenderSessionHandle) RenderUpdate() error {
	ptr, err := session.ptr()
	if err != nil {
		return err
	}
	defer session.state.KeepAlive()
	defer session.parent.state.KeepAlive()
	return session.withNoAcquiredFrame(func() error {
		return checkNative(func() capi.Status { return capi.RenderSessionRenderUpdate(ptr) })
	})
}

// Detach detaches the render target from the session.
func (session *RenderSessionHandle) Detach() error {
	ptr, err := session.ptr()
	if err != nil {
		return err
	}
	defer session.state.KeepAlive()
	defer session.parent.state.KeepAlive()
	return session.withNoAcquiredFrame(func() error {
		return checkNative(func() capi.Status { return capi.RenderSessionDetach(ptr) })
	})
}

// ReduceMemoryUse asks the render session to release cached render resources.
func (session *RenderSessionHandle) ReduceMemoryUse() error {
	ptr, err := session.ptr()
	if err != nil {
		return err
	}
	defer session.state.KeepAlive()
	defer session.parent.state.KeepAlive()
	return checkNative(func() capi.Status { return capi.RenderSessionReduceMemoryUse(ptr) })
}

// ClearData clears render-session data.
func (session *RenderSessionHandle) ClearData() error {
	ptr, err := session.ptr()
	if err != nil {
		return err
	}
	defer session.state.KeepAlive()
	defer session.parent.state.KeepAlive()
	return checkNative(func() capi.Status { return capi.RenderSessionClearData(ptr) })
}

// DumpDebugLogs dumps render-session debug logs.
func (session *RenderSessionHandle) DumpDebugLogs() error {
	ptr, err := session.ptr()
	if err != nil {
		return err
	}
	defer session.state.KeepAlive()
	defer session.parent.state.KeepAlive()
	return checkNative(func() capi.Status { return capi.RenderSessionDumpDebugLogs(ptr) })
}

// ReadPremultipliedRGBA8 reads the latest session-owned texture frame into a
// new byte slice.
func (session *RenderSessionHandle) ReadPremultipliedRGBA8() ([]byte, TextureImageInfo, error) {
	ptr, err := session.ptr()
	if err != nil {
		return nil, TextureImageInfo{}, err
	}
	defer session.state.KeepAlive()
	defer session.parent.state.KeepAlive()

	var rawInfo capi.TextureImageInfo
	failure := internalstatus.CheckCall(func() capi.Status {
		return capi.TextureReadPremultipliedRGBA8(ptr, nil, &rawInfo)
	})
	info := textureImageInfoFromCAPI(rawInfo)
	if failure == nil {
		return nil, info, nil
	}
	if failure.Status != capi.StatusInvalidArgument || info.ByteLength == 0 {
		return nil, info, newStatusError(failure)
	}
	buffer := make([]byte, info.ByteLength)
	info, err = session.ReadPremultipliedRGBA8Into(buffer)
	if err != nil {
		return nil, info, err
	}
	return buffer, info, nil
}

// ReadPremultipliedRGBA8Into reads the latest session-owned texture frame into
// caller-owned storage.
func (session *RenderSessionHandle) ReadPremultipliedRGBA8Into(buffer []byte) (TextureImageInfo, error) {
	ptr, err := session.ptr()
	if err != nil {
		return TextureImageInfo{}, err
	}
	defer session.state.KeepAlive()
	defer session.parent.state.KeepAlive()
	var rawInfo capi.TextureImageInfo
	if err := checkNative(func() capi.Status { return capi.TextureReadPremultipliedRGBA8(ptr, buffer, &rawInfo) }); err != nil {
		runtime.KeepAlive(buffer)
		return textureImageInfoFromCAPI(rawInfo), err
	}
	runtime.KeepAlive(buffer)
	return textureImageInfoFromCAPI(rawInfo), nil
}

// AcquireMetalTextureFrame acquires the latest Metal session-owned texture
// frame. While the frame is live, resize, render update, detach, readback,
// session close, and another frame acquire are invalid.
func (session *RenderSessionHandle) AcquireMetalTextureFrame() (*MetalOwnedTextureFrame, error) {
	ptr, err := session.ptr()
	if err != nil {
		return nil, err
	}
	defer session.state.KeepAlive()
	defer session.parent.state.KeepAlive()
	session.mu.Lock()
	defer session.mu.Unlock()
	if session.frame {
		return nil, newBindingError(ErrInvalidState, "texture frame is still acquired")
	}
	var rawFrame capi.MetalOwnedTextureFrame
	if err := checkNative(func() capi.Status { return capi.MetalOwnedTextureAcquireFrame(ptr, &rawFrame) }); err != nil {
		return nil, err
	}
	session.frame = true
	return &MetalOwnedTextureFrame{
		Generation:  rawFrame.Generation,
		Width:       rawFrame.Width,
		Height:      rawFrame.Height,
		ScaleFactor: rawFrame.ScaleFactor,
		Texture:     NativePointer(rawFrame.Texture),
		Device:      NativePointer(rawFrame.Device),
		PixelFormat: rawFrame.PixelFormat,
		session:     session,
		raw:         rawFrame,
	}, nil
}

// AcquireVulkanTextureFrame acquires the latest Vulkan session-owned texture
// frame. While the frame is live, resize, render update, detach, readback,
// session close, and another frame acquire are invalid.
func (session *RenderSessionHandle) AcquireVulkanTextureFrame() (*VulkanOwnedTextureFrame, error) {
	ptr, err := session.ptr()
	if err != nil {
		return nil, err
	}
	defer session.state.KeepAlive()
	defer session.parent.state.KeepAlive()
	session.mu.Lock()
	defer session.mu.Unlock()
	if session.frame {
		return nil, newBindingError(ErrInvalidState, "texture frame is still acquired")
	}
	var rawFrame capi.VulkanOwnedTextureFrame
	if err := checkNative(func() capi.Status { return capi.VulkanOwnedTextureAcquireFrame(ptr, &rawFrame) }); err != nil {
		return nil, err
	}
	session.frame = true
	return &VulkanOwnedTextureFrame{
		Generation:  rawFrame.Generation,
		Width:       rawFrame.Width,
		Height:      rawFrame.Height,
		ScaleFactor: rawFrame.ScaleFactor,
		Image:       NativePointer(rawFrame.Image),
		ImageView:   NativePointer(rawFrame.ImageView),
		Device:      NativePointer(rawFrame.Device),
		Format:      rawFrame.Format,
		Layout:      rawFrame.Layout,
		session:     session,
		raw:         rawFrame,
	}, nil
}

// Close releases this acquired Metal texture frame on the session owner thread.
// A second Close is a no-op after a successful release; failed releases remain
// retryable.
func (frame *MetalOwnedTextureFrame) Close() error {
	if frame == nil || frame.session == nil {
		return newBindingError(ErrInvalidArgument, "MetalOwnedTextureFrame is nil")
	}
	frame.mu.Lock()
	defer frame.mu.Unlock()
	if frame.closed {
		return nil
	}
	ptr, err := frame.session.ptr()
	if err != nil {
		return err
	}
	defer frame.session.state.KeepAlive()
	defer frame.session.parent.state.KeepAlive()
	if err := checkNative(func() capi.Status { return capi.MetalOwnedTextureReleaseFrame(ptr, frame.raw) }); err != nil {
		return err
	}
	frame.closed = true
	frame.session.markFrameReleased()
	return nil
}

// Close releases this acquired Vulkan texture frame on the session owner
// thread. A second Close is a no-op after a successful release; failed releases
// remain retryable.
func (frame *VulkanOwnedTextureFrame) Close() error {
	if frame == nil || frame.session == nil {
		return newBindingError(ErrInvalidArgument, "VulkanOwnedTextureFrame is nil")
	}
	frame.mu.Lock()
	defer frame.mu.Unlock()
	if frame.closed {
		return nil
	}
	ptr, err := frame.session.ptr()
	if err != nil {
		return err
	}
	defer frame.session.state.KeepAlive()
	defer frame.session.parent.state.KeepAlive()
	if err := checkNative(func() capi.Status { return capi.VulkanOwnedTextureReleaseFrame(ptr, frame.raw) }); err != nil {
		return err
	}
	frame.closed = true
	frame.session.markFrameReleased()
	return nil
}

// Close destroys this render session. A successful close makes later calls
// no-ops. A failed close leaves the native handle live so callers can retry on
// the owner thread.
func (session *RenderSessionHandle) Close() error {
	if session == nil || session.state == nil {
		return newBindingError(ErrInvalidArgument, "RenderSessionHandle is nil")
	}
	defer session.parent.state.KeepAlive()
	return session.withNoAcquiredFrame(func() error {
		return checkNative(func() capi.Status { return session.state.Close(capi.RenderSessionDestroy) })
	})
}
