package tlp

import (
	"bytes"
	"fmt"
)

// TlpHeader is the first header dword, common on all TLPs.
// See section 2.2.1. Common Packet Header Fields.
type TlpHeader struct {
	// Format and type.
	Type TlpType
	// Traffic class (3b).
	TC TrafficClass
	// Indicates that a Memory Request is an LN Read or LN Write (1b).
	LN bool
	// Presence of TLP Processing Hints (1b).
	TH bool
	// Presence of TLP digest in the form of a single DW at the end of the TLP (1b).
	TD bool
	// Indicates the TLP is poisoned (1b).
	EP bool
	// Attributes (3b): no-snoop, relaxed ordering, id-based ordering.
	NS  bool
	RO  bool
	IBO bool
	// Address Type (2b).
	AT AddressType
	// Length of data payload in DW (10b).
	Length int
}

func (h *TlpHeader) fromBuffer(buf *bytes.Buffer) {
	dw := buf.Next(dwordLen)
	h.Type = TlpType(uint8(dw[0]))
	h.TC = TrafficClass(getSubField(dw[1], 4, 7))
	h.IBO = getBit(dw[1], 2)
	h.LN = getBit(dw[1], 1)
	h.TH = getBit(dw[1], 0)
	h.TD = getBit(dw[2], 7)
	h.EP = getBit(dw[2], 6)
	h.RO = getBit(dw[2], 5)
	h.NS = getBit(dw[2], 4)
	h.AT = AddressType(getSubField(dw[2], 2, 3))
	h.Length = getSubField(dw[2], 0, 3)<<8 | int(dw[3])
}

// setLength sets the encoded TLP data length based on Table 2-4
// Length[9:0] Field Encoding.
func (h *TlpHeader) setLength(bytesLen int) error {
	if bytesLen&3 > 0 {
		return fmt.Errorf("%w: TLP length %d is not dword aligned", ErrBadLength, bytesLen)
	}
	if bytesLen > maxDataLen {
		return fmt.Errorf("%w: TLP length %d is too big, expected <= %d", ErrBadLength, bytesLen, maxDataLen)
	}

	h.Length = bytesLen >> 2
	if h.Length == 1024 {
		h.Length = 0
	}
	return nil
}

// DataLength decodes h.Length to data length.
// See Table 2-4 Length[9:0] Field Encoding.
func (h *TlpHeader) DataLength() int {
	l := h.Length
	if l == 0 {
		l = 1024
	}
	return l * dwordLen
}
