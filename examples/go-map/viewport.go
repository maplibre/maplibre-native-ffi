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

	safeWindowWidth := maxInt(windowWidth, 1)
	safeWindowHeight := maxInt(windowHeight, 1)
	safePhysicalWidth := maxInt(physicalWidth, 1)
	safePhysicalHeight := maxInt(physicalHeight, 1)
	sizeScale := math.Max(
		float64(safePhysicalWidth)/float64(safeWindowWidth),
		float64(safePhysicalHeight)/float64(safeWindowHeight),
	)

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

func (v viewport) log(label string) {
	fmt.Printf("%s: logical=%dx%d physical=%dx%d scale=%.2f\n", label, v.logicalWidth, v.logicalHeight, v.physicalWidth, v.physicalHeight, v.scaleFactor)
}

func scaledLogicalSize(physical int, scale float64) uint32 {
	return uint32(math.Max(math.Ceil(float64(physical)/scale), 1))
}

func maxInt(a, b int) int {
	if a > b {
		return a
	}
	return b
}
