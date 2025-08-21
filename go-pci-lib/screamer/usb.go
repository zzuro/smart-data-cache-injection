package screamer

import (
	"sync/atomic"

	"github.com/google/gousb"
	"go.uber.org/zap"
)

// sharedUSBContext manages a refcount for given gousb.Context object.
// This allows sharing a single context by multiple Screamer devices.
// Context is automatically closed once all references are dropped.
type sharedUSBContext struct {
	usbCtx   usbContext
	refCount int32

	logger *zap.Logger
}

func newUSBContext(logger *zap.Logger) *sharedUSBContext {
	return &sharedUSBContext{
		usbCtx:   newUSBContextAdapter(gousb.NewContext()),
		refCount: 1,
		logger:   logger,
	}
}

func (c *sharedUSBContext) Ref() *sharedUSBContext {
	atomic.AddInt32(&c.refCount, 1)
	return c
}

func (c *sharedUSBContext) Close() {
	if atomic.AddInt32(&c.refCount, -1) == 0 {
		c.logger.Info("Closing usb context")
		c.usbCtx.Close()
		c.usbCtx = nil
	}
}
