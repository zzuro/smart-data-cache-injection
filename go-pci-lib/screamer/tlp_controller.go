package screamer

import (
	"bytes"
	"encoding/binary"
	"fmt"

	"go.uber.org/zap"
)

// TLPController sends and receives TLP packets.
// Implements io.ReadWriter.
type TLPController struct {
	d          *Screamer
	readBuffer *bytes.Buffer
	partialTlp *bytes.Buffer
	tlpQueue   []*bytes.Buffer
	logger     *zap.Logger
}

// Extracts TLB buffers from the response buffer.
// Based on DeviceFPGA_RxTlpSynchronous().
func (c *TLPController) decodeResponseBuffer() error {
	var d dataBlock
	for c.readBuffer.Len() >= binary.Size(d) {
		if err := binary.Read(bytes.NewReader(c.readBuffer.Next(binary.Size(d))), le, &d); err != nil {
			return fmt.Errorf("binary.Read(r, le, &d) = %v, want nil error", err)
		}
		if (d.Status & statusPropertiesMask) != statusWithDataBlocks {
			continue
		}
		// Status (32bits) encodes information about the attached data blocks in each
		// of its 4bits nibbles.
		// Iterate over each 4b nibble in status, and extract TLP data bytes.
		for j := 0; j < len(d.Data); j, d.Status = j+1, d.Status>>4 {
			if (int(d.Status) & int(configFlagCore)) == 0x00 {
				// PCIe TLP. Append data to result.
				c.partialTlp.Write(d.Data[j][:])
			}
			if (int(d.Status) & readTLPLastMask) == readTLPPCIeAndLastData {
				c.tlpQueue = append(c.tlpQueue, c.partialTlp)
				c.partialTlp = new(bytes.Buffer)
			}
		}
	}
	return nil
}

// Read synchronously reads a TLP buffer.
// Based on DeviceFPGA_RxTlpSynchronous().
func (c *TLPController) Read(p []byte) (int, error) {
	if len(c.tlpQueue) == 0 {
		// Read encoded TLP buffer.
		res := make([]byte, maxReceiveSize)
		n, err := c.d.dataRead(res)
		if err != nil {
			return 0, fmt.Errorf("d.dataRead(res) = _, %v, want nil error", err)
		}
		if n == 0 {
			return n, nil
		}

		// c.logger.Info("[TLP-RX]: received", zap.Int("num", n))
		// fmt.Printf(hex.Dump(res[:n]))

		// Append to unprocessed buffer.
		c.readBuffer.Write(res[:n])

		if err = c.decodeResponseBuffer(); err != nil {
			return 0, fmt.Errorf("failed to decode response buffer: %v", err)
		}
	}

	if len(c.tlpQueue) == 0 {
		return 0, nil
	}

	// c.logger.Info("TLPs in RX queue:", zap.Int("num", len(c.tlpQueue)))

	// Copy TLP to result buffer.
	tlp := c.tlpQueue[0].Bytes()
	c.tlpQueue[0] = nil
	c.tlpQueue = c.tlpQueue[1:]

	n := min(len(p), len(tlp))

	// fmt.Println(len(p), len(tlp))
	// fmt.Println("------------------------------")

	copy(p, tlp[:n])
	// c.logger.Info("[TLP-RX]: received", zap.Int("num", n))

	/* 	response, err := genTLP.NewCplFromBytes(p[:n])
	   	if err != nil {
	   		c.logger.Info("[TLP-RX]: error decoding response", zap.Int("num", n))
	   	} */

	return n, nil
}

func (c *TLPController) Write(p []byte) (int, error) {
	if len(p)%dwordLen > 0 {
		return 0, fmt.Errorf("TLP buffer should be dword aligned, got %d", len(p))
	}
	if len(p) < 3*dwordLen {
		return 0, fmt.Errorf("TLP buffer should contain at least 3 dwords, got %d", len(p))
	}
	if len(p) > maxTransmitSize/2 {
		return 0, fmt.Errorf("TLP buffer too big, want %d, got %d", maxTransmitSize/2, len(p))
	}
	// c.logger.Info("[TLP-TX]: sending", zap.Int("num", len(p)))
	// fmt.Printf(hex.Dump(p))
	return c.d.writeTLP(p)
}

// NewTLPController builds a new TLPController which may be used to send
// and receive TLP packets.
// Caller should not use TLPController after calling d.Close().
func NewTLPController(d *Screamer, logger *zap.Logger) *TLPController {
	logger.Info("generating TLP Controller")
	return &TLPController{
		d: d, readBuffer: new(bytes.Buffer),
		partialTlp: new(bytes.Buffer),
		logger:     logger,
	}
}
