package maplibre

import (
	"github.com/maplibre/maplibre-native-ffi/bindings/go/internal/capi"
	"github.com/maplibre/maplibre-native-ffi/bindings/go/internal/handle"
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

// RenderSessionHandle owns a map render session.
type RenderSessionHandle struct {
	state  *handle.State[capi.RenderSession]
	parent *MapHandle
}

func (extent RenderTargetExtent) toCAPI() capi.RenderTargetExtent {
	return capi.RenderTargetExtent{Width: extent.Width, Height: extent.Height, ScaleFactor: extent.ScaleFactor}
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
	state, err := handle.New(session, "RenderSessionHandle")
	if err != nil {
		return nil, newBindingError(ErrInvalidArgument, err.Error())
	}
	return &RenderSessionHandle{state: state, parent: m}, nil
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
	state, err := handle.New(session, "RenderSessionHandle")
	if err != nil {
		return nil, newBindingError(ErrInvalidArgument, err.Error())
	}
	return &RenderSessionHandle{state: state, parent: m}, nil
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

// Resize changes the render session target extent.
func (session *RenderSessionHandle) Resize(extent RenderTargetExtent) error {
	ptr, err := session.ptr()
	if err != nil {
		return err
	}
	defer session.state.KeepAlive()
	defer session.parent.state.KeepAlive()
	return checkNative(func() capi.Status { return capi.RenderSessionResize(ptr, extent.toCAPI()) })
}

// RenderUpdate renders one frame/update into the attached render target.
func (session *RenderSessionHandle) RenderUpdate() error {
	ptr, err := session.ptr()
	if err != nil {
		return err
	}
	defer session.state.KeepAlive()
	defer session.parent.state.KeepAlive()
	return checkNative(func() capi.Status { return capi.RenderSessionRenderUpdate(ptr) })
}

// Detach detaches the render target from the session.
func (session *RenderSessionHandle) Detach() error {
	ptr, err := session.ptr()
	if err != nil {
		return err
	}
	defer session.state.KeepAlive()
	defer session.parent.state.KeepAlive()
	return checkNative(func() capi.Status { return capi.RenderSessionDetach(ptr) })
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

// Close destroys this render session. A successful close makes later calls
// no-ops. A failed close leaves the native handle live so callers can retry on
// the owner thread.
func (session *RenderSessionHandle) Close() error {
	if session == nil || session.state == nil {
		return newBindingError(ErrInvalidArgument, "RenderSessionHandle is nil")
	}
	defer session.parent.state.KeepAlive()
	return checkNative(func() capi.Status { return session.state.Close(capi.RenderSessionDestroy) })
}
