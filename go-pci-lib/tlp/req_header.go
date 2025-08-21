package tlp

import "bytes"

// RequestHeader extends TlpHeader and includes the second header dword
// on Memory, IO, and Config Request TLPs.
type RequestHeader struct {
	TlpHeader
	// Requester ID.
	ReqID DeviceID
	// Unique tag for all outstanding requests.
	Tag uint8
	// First Byte Enable (4b).
	FirstBE uint8
	// Last Byte Enable (4b).
	LastBE uint8
}

func (h *RequestHeader) toBuffer(buf *bytes.Buffer) {
	h.TlpHeader.toBuffer(buf)

	dw := make([]byte, dwordLen)
	copy(dw[0:2], h.ReqID.toBytes())
	dw[2] = byte(h.Tag)
	dw[3] = byte(h.LastBE<<4 | h.FirstBE)
	buf.Write(dw)
}

func (h *RequestHeader) fromBuffer(buf *bytes.Buffer) {
	h.TlpHeader.fromBuffer(buf)

	dw := buf.Next(dwordLen)
	h.ReqID.fromBytes(dw[0:2])
	h.Tag = dw[2]
	h.FirstBE = uint8(getSubField(dw[3], 0, 0xf))
	h.LastBE = uint8(getSubField(dw[3], 4, 0xf))
}

func (h *RequestHeader) setByteEnables() {
	// See section 2.2.5. First/Last DW Byte Enables Rules.
	h.FirstBE = 0xf
	if h.Length == 1 {
		h.LastBE = 0
	} else {
		h.LastBE = 0xf
	}
}
