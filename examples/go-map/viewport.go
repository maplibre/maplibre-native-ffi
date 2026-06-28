package main

import (
	"fmt"
	"math"

	"github.com/jfreymuth/go-sdl3/sdl"
	maplibre "github.com/maplibre/maplibre-native-ffi/bindings/go"
)

const (
	initialWindowWidth  = 960
	initialWindowHeight = 640
)

func currentViewport(window *sdl.Window) viewport {
	windowWidth, windowHeight, err := window.Size()
	if err != nil {
		windowWidth, windowHeight = initialWindowWidth, initialWindowHeight
	}
	physicalWidth, physicalHeight, err := window.SizeInPixels()
	if err != nil {
		physicalWidth, physicalHeight = windowWidth, windowHeight
	}

	safeWindowWidth := maxInt(windowWidth, 0)
	safeWindowHeight := maxInt(windowHeight, 0)
	safePhysicalWidth := maxInt(physicalWidth, 0)
	safePhysicalHeight := maxInt(physicalHeight, 0)
	sizeScale := math.Max(
		float64(safePhysicalWidth)/float64(maxInt(safeWindowWidth, 1)),
		float64(safePhysicalHeight)/float64(maxInt(safeWindowHeight, 1)),
	)
	if sizeScale <= 0 {
		sizeScale = 1
	}

	fallbackScale := sizeScale
	if density, err := window.PixelDensity(); err == nil && density > 0 {
		fallbackScale = float64(density)
	}
	scale := fallbackScale
	if displayScale, err := window.DisplayScale(); err == nil && displayScale > 0 {
		scale = float64(displayScale)
	}

	return viewport{
		logicalWidth:   scaledLogicalSize(safePhysicalWidth, scale),
		logicalHeight:  scaledLogicalSize(safePhysicalHeight, scale),
		windowWidth:    uint32(safeWindowWidth),
		windowHeight:   uint32(safeWindowHeight),
		physicalWidth:  uint32(safePhysicalWidth),
		physicalHeight: uint32(safePhysicalHeight),
		scaleFactor:    scale,
	}
}

func (v viewport) extent() maplibre.RenderTargetExtent {
	return maplibre.RenderTargetExtent{Width: v.logicalWidth, Height: v.logicalHeight, ScaleFactor: v.scaleFactor}
}

func (v viewport) empty() bool {
	return v.logicalWidth == 0 || v.logicalHeight == 0 || v.physicalWidth == 0 || v.physicalHeight == 0
}

func (v viewport) log(label string) {
	fmt.Printf("%s: logical=%dx%d physical=%dx%d scale=%.2f\n", label, v.logicalWidth, v.logicalHeight, v.physicalWidth, v.physicalHeight, v.scaleFactor)
}

func scaledLogicalSize(physical int, scale float64) uint32 {
	if physical <= 0 {
		return 0
	}
	return uint32(math.Max(math.Ceil(float64(physical)/scale), 1))
}

func maxInt(a, b int) int {
	if a > b {
		return a
	}
	return b
}
