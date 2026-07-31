package main

import "fmt"

type renderTargetMode int

const (
	modeOwnedTexture renderTargetMode = iota
	modeBorrowedTexture
	modeNativeSurface
)

func parseRenderTargetMode(value string) (renderTargetMode, bool) {
	switch value {
	case "owned-texture":
		return modeOwnedTexture, true
	case "borrowed-texture":
		return modeBorrowedTexture, true
	case "native-surface":
		return modeNativeSurface, true
	default:
		return 0, false
	}
}

func (mode renderTargetMode) String() string {
	switch mode {
	case modeOwnedTexture:
		return "owned-texture"
	case modeBorrowedTexture:
		return "borrowed-texture"
	case modeNativeSurface:
		return "native-surface"
	default:
		return fmt.Sprintf("unknown(%d)", mode)
	}
}

func (mode renderTargetMode) statusLine() string {
	switch mode {
	case modeOwnedTexture:
		return "samples MapLibre-owned texture frames into the host swapchain"
	case modeBorrowedTexture:
		return "renders into a host-owned texture, then samples it into the host swapchain"
	case modeNativeSurface:
		return "renders directly to the host window surface"
	default:
		return "unknown render target"
	}
}

type viewport struct {
	logicalWidth   uint32
	logicalHeight  uint32
	windowWidth    uint32
	windowHeight   uint32
	physicalWidth  uint32
	physicalHeight uint32
	scaleFactor    float64
}
